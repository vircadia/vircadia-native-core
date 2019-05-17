//
//  Created by Bradley Austin Davis on 2015/09/05
//  Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Procedural.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QDateTime>

#include <gpu/Batch.h>
#include <SharedUtil.h>
#include <NumericalConstants.h>
#include <GLMHelpers.h>
#include <NetworkingConstants.h>
#include <shaders/Shaders.h>

#include "ShaderConstants.h"
#include "Logging.h"

Q_LOGGING_CATEGORY(proceduralLog, "hifi.gpu.procedural")

// User-data parsing constants
static const QString PROCEDURAL_USER_DATA_KEY = "ProceduralEntity";
static const QString FRAGMENT_URL_KEY = "fragmentShaderURL";
static const QString URL_KEY = "shaderUrl";
static const QString VERSION_KEY = "version";
static const QString UNIFORMS_KEY = "uniforms";
static const QString CHANNELS_KEY = "channels";

// Shader replace strings
static const std::string PROCEDURAL_BLOCK = "//PROCEDURAL_BLOCK";
static const std::string PROCEDURAL_VERSION = "//PROCEDURAL_VERSION";

bool operator==(const ProceduralData& a, const ProceduralData& b) {
    return ((a.version == b.version) &&
            (a.fragmentShaderUrl == b.fragmentShaderUrl) &&
            (a.uniforms == b.uniforms) &&
            (a.channels == b.channels));
}

QJsonValue ProceduralData::getProceduralData(const QString& proceduralJson) {
    if (proceduralJson.isEmpty()) {
        return QJsonValue();
    }

    QJsonParseError parseError;
    auto doc = QJsonDocument::fromJson(proceduralJson.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return QJsonValue();
    }

    auto object = doc.object();
    auto userDataIt = object.find(PROCEDURAL_USER_DATA_KEY);
    if (userDataIt != object.end()) {
        return userDataIt.value();
    }

    return object;
}

ProceduralData ProceduralData::parse(const QString& proceduralData) {
    ProceduralData result;
    result.parse(getProceduralData(proceduralData).toObject());
    return result;
}

void ProceduralData::parse(const QJsonObject& proceduralData) {
    if (proceduralData.isEmpty()) {
        return;
    }

    {
        auto versionJson = proceduralData[VERSION_KEY];
        if (versionJson.isDouble()) {
            version = (uint8_t)(floor(versionJson.toDouble()));
            // invalid version
            if (!(version == 1 || version == 2 || version == 3 || version == 4)) {
                return;
            }
        } else {
            // All unversioned shaders default to V1
            version = 1;
        }
    }

    { // Fragment shader URL (either fragmentShaderURL or shaderUrl)
        auto rawShaderUrl = proceduralData[FRAGMENT_URL_KEY].toString();
        fragmentShaderUrl = DependencyManager::get<ResourceManager>()->normalizeURL(rawShaderUrl);

        if (fragmentShaderUrl.isEmpty()) {
            rawShaderUrl = proceduralData[URL_KEY].toString();
            fragmentShaderUrl = DependencyManager::get<ResourceManager>()->normalizeURL(rawShaderUrl);
        }
    }

    // Empty shader URL isn't valid
    if (fragmentShaderUrl.isEmpty()) {
        return;
    }

    uniforms = proceduralData[UNIFORMS_KEY].toObject();
    channels = proceduralData[CHANNELS_KEY].toArray();
}

std::function<void(gpu::StatePointer)> Procedural::opaqueStencil = [](gpu::StatePointer state) {};
std::function<void(gpu::StatePointer)> Procedural::transparentStencil = [](gpu::StatePointer state) {};

Procedural::Procedural() {
    _opaqueState->setCullMode(gpu::State::CULL_NONE);
    _opaqueState->setDepthTest(true, true, gpu::LESS_EQUAL);
    _opaqueState->setBlendFunction(false,
        gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
        gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
    opaqueStencil(_opaqueState);

    _transparentState->setCullMode(gpu::State::CULL_NONE);
    _transparentState->setDepthTest(true, true, gpu::LESS_EQUAL);
    _transparentState->setBlendFunction(true,
        gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
        gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
    transparentStencil(_transparentState);

    _standardInputsBuffer = std::make_shared<gpu::Buffer>(sizeof(StandardInputs), nullptr);
}

void Procedural::setProceduralData(const ProceduralData& proceduralData) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (proceduralData == _data) {
        return;
    }

    _enabled = false;

    if (proceduralData.version != _data.version) {
        _data.version = proceduralData.version;
        _shaderDirty = true;
    }

    if (proceduralData.uniforms != _data.uniforms) {
        // If the uniform keys changed, we need to recreate the whole shader to handle the reflection
        if (proceduralData.uniforms.keys() != _data.uniforms.keys()) {
            _shaderDirty = true;
        }
        _data.uniforms = proceduralData.uniforms;
        _uniformsDirty = true;
    }

    if (proceduralData.channels != _data.channels) {
        _data.channels = proceduralData.channels;
        // Must happen on the main thread
        auto textureCache = DependencyManager::get<TextureCache>();
        size_t channelCount = std::min(MAX_PROCEDURAL_TEXTURE_CHANNELS, (size_t)proceduralData.channels.size());
        size_t channel;
        for (channel = 0; channel < MAX_PROCEDURAL_TEXTURE_CHANNELS; ++channel) {
            if (channel < channelCount) {
                QString url = proceduralData.channels.at((int)channel).toString();
                _channels[channel] = textureCache->getTexture(QUrl(url));
            } else {
                // Release those textures no longer in use
                _channels[channel] = textureCache->getTexture(QUrl());
            }
        }
    }

    if (proceduralData.fragmentShaderUrl != _data.fragmentShaderUrl) {
        _data.fragmentShaderUrl = proceduralData.fragmentShaderUrl;
        const auto& shaderUrl = _data.fragmentShaderUrl;

        _shaderDirty = true;
        _networkFragmentShader.reset();
        _fragmentShaderPath.clear();
        _fragmentShaderSource.clear();

        if (shaderUrl.isEmpty() || !shaderUrl.isValid()) {
            return;
        }

        if (shaderUrl.isLocalFile()) {
            if (!QFileInfo(shaderUrl.toLocalFile()).exists()) {
                return;
            }
            _fragmentShaderPath = shaderUrl.toLocalFile();
        } else if (shaderUrl.scheme() == URL_SCHEME_QRC) {
            _fragmentShaderPath = ":" + shaderUrl.path();
        } else {
            _networkFragmentShader = ShaderCache::instance().getShader(shaderUrl);
        }
    }

    _enabled = true;
}

bool Procedural::isReady() const {
#if defined(USE_GLES)
    return false;
#endif

    std::lock_guard<std::mutex> lock(_mutex);

    if (!_enabled) {
        return false;
    }

    if (!_hasStartedFade) {
        _fadeStartTime = usecTimestampNow();
    }

    // Do we have a network or local shader, and if so, is it loaded?
    if (_fragmentShaderPath.isEmpty() && (!_networkFragmentShader || !_networkFragmentShader->isLoaded())) {
        return false;
    }

    // Do we have textures, and if so, are they loaded?
    for (size_t i = 0; i < MAX_PROCEDURAL_TEXTURE_CHANNELS; ++i) {
        if (_channels[i] && !_channels[i]->isLoaded()) {
            return false;
        }
    }

    if (!_hasStartedFade) {
        _hasStartedFade = true;
        _isFading = true;
    }

    return true;
}

void Procedural::prepare(gpu::Batch& batch,
                         const glm::vec3& position,
                         const glm::vec3& size,
                         const glm::quat& orientation,
                         const uint64_t& created,
                         const ProceduralProgramKey key) {
    std::lock_guard<std::mutex> lock(_mutex);
    _entityDimensions = size;
    _entityPosition = position;
    _entityOrientation = glm::mat3_cast(orientation);
    _entityCreated = created;
    if (!_fragmentShaderPath.isEmpty()) {
        auto lastModified = (uint64_t)QFileInfo(_fragmentShaderPath).lastModified().toMSecsSinceEpoch();
        if (lastModified > _fragmentShaderModified) {
            QFile file(_fragmentShaderPath);
            file.open(QIODevice::ReadOnly);
            _fragmentShaderSource = QTextStream(&file).readAll();
            _shaderDirty = true;
            _fragmentShaderModified = lastModified;
        }
    } else if (_fragmentShaderSource.isEmpty() && _networkFragmentShader && _networkFragmentShader->isLoaded()) {
        _fragmentShaderSource = _networkFragmentShader->_source;
        _shaderDirty = true;
    }

    if (_shaderDirty) {
        _proceduralPipelines.clear();
    }

    auto pipeline = _proceduralPipelines.find(key);
    bool recompiledShader = false;
    if (pipeline == _proceduralPipelines.end()) {
        gpu::Shader::Source vertexSource;
        if (key.isSkinnedDQ()) {
            vertexSource = _vertexSourceSkinnedDQ;
        } else if (key.isSkinned()) {
            vertexSource = _vertexSourceSkinned;
        } else {
            vertexSource = _vertexSource;
        }

        gpu::Shader::Source& fragmentSource = (key.isTransparent() && _transparentFragmentSource.valid()) ? _transparentFragmentSource : _opaqueFragmentSource;

        // Build the fragment shader
        fragmentSource.replacements.clear();
        fragmentSource.replacements[PROCEDURAL_VERSION] = "#define PROCEDURAL_V" + std::to_string(_data.version);
        fragmentSource.replacements[PROCEDURAL_BLOCK] = _fragmentShaderSource.toStdString();

        // Set any userdata specified uniforms
        int customSlot = procedural::slot::uniform::Custom;
        for (const auto& key : _data.uniforms.keys()) {
            std::string uniformName = key.toLocal8Bit().data();
            fragmentSource.reflection.uniforms[uniformName] = customSlot;
            ++customSlot;
        }

        // Leave this here for debugging
        //qCDebug(proceduralLog) << "FragmentShader:\n" << fragmentSource.getSource(shader::Dialect::glsl450, shader::Variant::Mono).c_str();

        gpu::ShaderPointer vertexShader = gpu::Shader::createVertex(vertexSource);
        gpu::ShaderPointer fragmentShader = gpu::Shader::createPixel(fragmentSource);
        gpu::ShaderPointer program = gpu::Shader::createProgram(vertexShader, fragmentShader);

        _proceduralPipelines[key] = gpu::Pipeline::create(program, key.isTransparent() ? _transparentState : _opaqueState);

        _lastCompile = usecTimestampNow();
        if (_firstCompile == 0) {
            _firstCompile = _lastCompile;
        }
        _frameCount = 0;
        recompiledShader = true;
    }

    // FIXME: need to handle forward rendering
    batch.setPipeline(recompiledShader ? _proceduralPipelines[key] : pipeline->second);

    bool recreateUniforms = _shaderDirty || _uniformsDirty || recompiledShader || _prevKey != key;
    if (recreateUniforms) {
        setupUniforms();
    }

    _prevKey = key;
    _shaderDirty = _uniformsDirty = false;

    for (auto lambda : _uniforms) {
        lambda(batch);
    }

    static gpu::Sampler sampler;
    static std::once_flag once;
    std::call_once(once, [&] {
        sampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR);
    });

    for (size_t i = 0; i < MAX_PROCEDURAL_TEXTURE_CHANNELS; ++i) {
        if (_channels[i] && _channels[i]->isLoaded()) {
            auto gpuTexture = _channels[i]->getGPUTexture();
            if (gpuTexture) {
                gpuTexture->setSampler(sampler);
                gpuTexture->setAutoGenerateMips(true);
            }
            batch.setResourceTexture((gpu::uint32)(procedural::slot::texture::Channel0 + i), gpuTexture);
        }
    }
}


void Procedural::setupUniforms() {
    _uniforms.clear();
    // Set any userdata specified uniforms
    int slot = procedural::slot::uniform::Custom;
    for (const auto& key : _data.uniforms.keys()) {
        std::string uniformName = key.toLocal8Bit().data();
        QJsonValue value = _data.uniforms[key];
        if (value.isDouble()) {
            float v = value.toDouble();
            _uniforms.push_back([slot, v](gpu::Batch& batch) { batch._glUniform1f(slot, v); });
        } else if (value.isArray()) {
            auto valueArray = value.toArray();
            switch (valueArray.size()) {
            case 0:
                break;

            case 1: {
                float v = valueArray[0].toDouble();
                _uniforms.push_back([slot, v](gpu::Batch& batch) { batch._glUniform1f(slot, v); });
                break;
            }

            case 2: {
                glm::vec2 v{ valueArray[0].toDouble(), valueArray[1].toDouble() };
                _uniforms.push_back([slot, v](gpu::Batch& batch) { batch._glUniform2f(slot, v.x, v.y); });
                break;
            }

            case 3: {
                glm::vec3 v{
                    valueArray[0].toDouble(),
                    valueArray[1].toDouble(),
                    valueArray[2].toDouble(),
                };
                _uniforms.push_back([slot, v](gpu::Batch& batch) { batch._glUniform3f(slot, v.x, v.y, v.z); });
                break;
            }

            default:
            case 4: {
                glm::vec4 v{
                    valueArray[0].toDouble(),
                    valueArray[1].toDouble(),
                    valueArray[2].toDouble(),
                    valueArray[3].toDouble(),
                };
                _uniforms.push_back([slot, v](gpu::Batch& batch) { batch._glUniform4f(slot, v.x, v.y, v.z, v.w); });
                break;
            }
            }
        }
        slot++;
    }

    _uniforms.push_back([this](gpu::Batch& batch) {
        _standardInputs.position = vec4(_entityPosition, 1.0f);
        // Minimize floating point error by doing an integer division to milliseconds, before the floating point division to seconds
        auto now = usecTimestampNow();
        _standardInputs.timeSinceLastCompile = (float)((now - _lastCompile) / USECS_PER_MSEC) / MSECS_PER_SECOND;
        _standardInputs.timeSinceFirstCompile = (float)((now - _firstCompile) / USECS_PER_MSEC) / MSECS_PER_SECOND;
        _standardInputs.timeSinceEntityCreation = (float)((now - _entityCreated) / USECS_PER_MSEC) / MSECS_PER_SECOND;


        // Date
        {
            QDateTime now = QDateTime::currentDateTimeUtc();
            QDate date = now.date();
            QTime time = now.time();
            _standardInputs.date.x = date.year();
            // Shadertoy month is 0 based
            _standardInputs.date.y = date.month() - 1;
            // But not the day... go figure
            _standardInputs.date.z = date.day();
            float fractSeconds = (time.msec() / 1000.0f);
            _standardInputs.date.w = (time.hour() * 3600) + (time.minute() * 60) + time.second() + fractSeconds;
        }

        _standardInputs.scale = vec4(_entityDimensions, 1.0f);
        _standardInputs.frameCount = ++_frameCount;
        _standardInputs.orientation = mat4(_entityOrientation);

        for (size_t i = 0; i < MAX_PROCEDURAL_TEXTURE_CHANNELS; ++i) {
            if (_channels[i]) {
                _standardInputs.resolution[i] = vec4(_channels[i]->getWidth(), _channels[i]->getHeight(), 1.0f, 1.0f);
            } else {
                _standardInputs.resolution[i] = vec4(1.0f);
            }
        }

        _standardInputsBuffer->setSubData(0, _standardInputs);
        batch.setUniformBuffer(procedural::slot::buffer::Inputs, _standardInputsBuffer, 0, sizeof(StandardInputs));
    });
}

glm::vec4 Procedural::getColor(const glm::vec4& entityColor) const {
    if (_data.version == 1) {
        return glm::vec4(1);
    }
    return entityColor;
}

void graphics::ProceduralMaterial::initializeProcedural() {
    _procedural._vertexSource = gpu::Shader::getVertexShaderSource(shader::render_utils::vertex::simple);
    _procedural._vertexSourceSkinned = gpu::Shader::getVertexShaderSource(shader::render_utils::vertex::deformed_simple);
    _procedural._vertexSourceSkinnedDQ = gpu::Shader::getVertexShaderSource(shader::render_utils::vertex::deformed_simple_dq);

    // FIXME: Setup proper uniform slots and use correct pipelines for forward rendering
    _procedural._opaqueFragmentSource = gpu::Shader::getFragmentShaderSource(shader::render_utils::fragment::simple);
    _procedural._transparentFragmentSource = gpu::Shader::getFragmentShaderSource(shader::render_utils::fragment::simple_transparent);
}