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
static const QString URL_KEY = "shaderUrl";
static const QString VERSION_KEY = "version";
static const QString UNIFORMS_KEY = "uniforms";
static const QString CHANNELS_KEY = "channels";

// Shader replace strings
static const std::string PROCEDURAL_BLOCK = "//PROCEDURAL_BLOCK";
static const std::string PROCEDURAL_VERSION = "//PROCEDURAL_VERSION";

bool operator==(const ProceduralData& a, const ProceduralData& b) {
    return ((a.version == b.version) && (a.shaderUrl == b.shaderUrl) && (a.uniforms == b.uniforms) &&
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

    return doc.object()[PROCEDURAL_USER_DATA_KEY];
}

ProceduralData ProceduralData::parse(const QString& userDataJson) {
    ProceduralData result;
    result.parse(getProceduralData(userDataJson).toObject());
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
            if (!(version == 1 || version == 2)) {
                return;
            }
        } else {
            // All unversioned shaders default to V1
            version = 1;
        }
    }

    auto rawShaderUrl = proceduralData[URL_KEY].toString();
    shaderUrl = DependencyManager::get<ResourceManager>()->normalizeURL(rawShaderUrl);

    // Empty shader URL isn't valid
    if (shaderUrl.isEmpty()) {
        return;
    }

    uniforms = proceduralData[UNIFORMS_KEY].toObject();
    channels = proceduralData[CHANNELS_KEY].toArray();
}

// Example
//{
//    "ProceduralEntity": {
//        "shaderUrl": "file:///C:/Users/bdavis/Git/hifi/examples/shaders/test.fs",
//    }
//}

Procedural::Procedural() {
    _transparentState->setCullMode(gpu::State::CULL_NONE);
    _transparentState->setDepthTest(true, true, gpu::LESS_EQUAL);
    _transparentState->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
        gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);

    _standardInputsBuffer = std::make_shared<gpu::Buffer>(sizeof(StandardInputs), nullptr);
}

void Procedural::setProceduralData(const ProceduralData& proceduralData) {
    if (proceduralData == _data) {
        return;
    }

    _dirty = true;
    _enabled = false;

    if (proceduralData.version != _data.version) {
        _data.version = proceduralData.version;
        _shaderDirty = true;
    }

    if (proceduralData.uniforms != _data.uniforms) {
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

    if (proceduralData.shaderUrl != _data.shaderUrl) {
        _data.shaderUrl = proceduralData.shaderUrl;
        _shaderDirty = true;
        const auto& shaderUrl = _data.shaderUrl;
        _networkShader.reset();
        _shaderPath.clear();

        if (shaderUrl.isEmpty()) {
            return;
        }

        if (!shaderUrl.isValid()) {
            return;
        }

        if (shaderUrl.isLocalFile()) {
            if (!QFileInfo(shaderUrl.toLocalFile()).exists()) {
                return;
            }
            _shaderPath = shaderUrl.toLocalFile();
        } else if (shaderUrl.scheme() == URL_SCHEME_QRC) {
            _shaderPath = ":" + shaderUrl.path();
        } else {
            _networkShader = ShaderCache::instance().getShader(shaderUrl);
        }
    }

    _enabled = true;
}

bool Procedural::isReady() const {
#if defined(USE_GLES)
    return false;
#endif

    if (!_enabled) {
        return false;
    }

    if (!_hasStartedFade) {
        _fadeStartTime = usecTimestampNow();
    }

    // Do we have a network or local shader, and if so, is it loaded?
    if (_shaderPath.isEmpty() && (!_networkShader || !_networkShader->isLoaded())) {
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
    const glm::vec4& color) {
    _entityDimensions = size;
    _entityPosition = position;
    _entityOrientation = glm::mat3_cast(orientation);
    if (!_shaderPath.isEmpty()) {
        auto lastModified = (quint64)QFileInfo(_shaderPath).lastModified().toMSecsSinceEpoch();
        if (lastModified > _shaderModified) {
            QFile file(_shaderPath);
            file.open(QIODevice::ReadOnly);
            _shaderSource = QTextStream(&file).readAll();
            _shaderDirty = true;
            _shaderModified = lastModified;
        }
    } else if (_networkShader && _networkShader->isLoaded()) {
        _shaderSource = _networkShader->_source;
    }

    if (!_opaquePipeline || !_transparentPipeline || _shaderDirty) {
        if (!_vertexShader) {
            _vertexShader = gpu::Shader::createVertex(_vertexSource);
        }

        // Build the fragment shader
        _opaqueFragmentSource.replacements.clear();
        if (_data.version == 1) {
            _opaqueFragmentSource.replacements[PROCEDURAL_VERSION] = "#define PROCEDURAL_V1 1";
        } else if (_data.version == 2) {
            _opaqueFragmentSource.replacements[PROCEDURAL_VERSION] = "#define PROCEDURAL_V2 1";
        }
        _opaqueFragmentSource.replacements[PROCEDURAL_BLOCK] = _shaderSource.toStdString();
        _transparentFragmentSource.replacements = _opaqueFragmentSource.replacements;

        // Set any userdata specified uniforms
        int customSlot = procedural::slot::uniform::Custom;
        for (const auto& key : _data.uniforms.keys()) {
            std::string uniformName = key.toLocal8Bit().data();
            _opaqueFragmentSource.reflection.uniforms[uniformName] = customSlot;
            _transparentFragmentSource.reflection.uniforms[uniformName] = customSlot;
            ++customSlot;
        }

        // Leave this here for debugging
        // qCDebug(procedural) << "FragmentShader:\n" << fragmentShaderSource.c_str();

        // TODO: THis is a simple fix, we need a cleaner way to provide the "hosting" program for procedural custom shaders to be defined together with the required bindings.
        _opaqueFragmentShader = gpu::Shader::createPixel(_opaqueFragmentSource);
        _opaqueShader = gpu::Shader::createProgram(_vertexShader, _opaqueFragmentShader);
        _opaquePipeline = gpu::Pipeline::create(_opaqueShader, _opaqueState);
        if (_transparentFragmentSource.valid()) {
            _transparentFragmentShader = gpu::Shader::createPixel(_transparentFragmentSource);
            _transparentShader = gpu::Shader::createProgram(_vertexShader, _transparentFragmentShader);
            _transparentPipeline = gpu::Pipeline::create(_transparentShader, _transparentState);
        } else {
            _transparentFragmentShader = _opaqueFragmentShader;
            _transparentShader = _opaqueShader;
            _transparentPipeline = _opaquePipeline;
        }
        _start = usecTimestampNow();
        _frameCount = 0;
    }

    bool transparent = color.a < 1.0f;
    batch.setPipeline(transparent ? _transparentPipeline : _opaquePipeline);

    if (_shaderDirty || _uniformsDirty || _prevTransparent != transparent) {
        setupUniforms(transparent);
    }

    _prevTransparent = transparent;
    _shaderDirty = _uniformsDirty = false;

    for (auto lambda : _uniforms) {
        lambda(batch);
    }

    static gpu::Sampler sampler;
    static std::once_flag once;
    std::call_once(once, [&] {
        gpu::Sampler::Desc desc;
        desc._filter = gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR;
    });

    for (size_t i = 0; i < MAX_PROCEDURAL_TEXTURE_CHANNELS; ++i) {
        if (_channels[i] && _channels[i]->isLoaded()) {
            auto gpuTexture = _channels[i]->getGPUTexture();
            if (gpuTexture) {
                gpuTexture->setSampler(sampler);
                gpuTexture->setAutoGenerateMips(true);
            }
            batch.setResourceTexture((gpu::uint32)i, gpuTexture);
        }
    }
}


void Procedural::setupUniforms(bool transparent) {
    _uniforms.clear();
    auto customUniformCount = _data.uniforms.keys().size();
    // Set any userdata specified uniforms
    for (int i = 0; i < customUniformCount; ++i) {
        int slot = procedural::slot::uniform::Custom + i;
        QString key = _data.uniforms.keys().at(i);
        std::string uniformName = key.toLocal8Bit().data();
        QJsonValue value = _data.uniforms[key];
        if (value.isDouble()) {
            float v = value.toDouble();
            _uniforms.push_back([=](gpu::Batch& batch) { batch._glUniform1f(slot, v); });
        } else if (value.isArray()) {
            auto valueArray = value.toArray();
            switch (valueArray.size()) {
            case 0:
                break;

            case 1: {
                float v = valueArray[0].toDouble();
                _uniforms.push_back([=](gpu::Batch& batch) { batch._glUniform1f(slot, v); });
                break;
            }

            case 2: {
                glm::vec2 v{ valueArray[0].toDouble(), valueArray[1].toDouble() };
                _uniforms.push_back([=](gpu::Batch& batch) { batch._glUniform2f(slot, v.x, v.y); });
                break;
            }

            case 3: {
                glm::vec3 v{
                    valueArray[0].toDouble(),
                    valueArray[1].toDouble(),
                    valueArray[2].toDouble(),
                };
                _uniforms.push_back([=](gpu::Batch& batch) { batch._glUniform3f(slot, v.x, v.y, v.z); });
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
                _uniforms.push_back([=](gpu::Batch& batch) { batch._glUniform4f(slot, v.x, v.y, v.z, v.w); });
                break;
            }
            }
        }
    }

    _uniforms.push_back([=](gpu::Batch& batch) {
        _standardInputs.position = vec4(_entityPosition, 1.0f);
        // Minimize floating point error by doing an integer division to milliseconds, before the floating point division to seconds
        _standardInputs.time = (float)((usecTimestampNow() - _start) / USECS_PER_MSEC) / MSECS_PER_SECOND;

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
        batch.setUniformBuffer(0, _standardInputsBuffer, 0, sizeof(StandardInputs));
    });
}

glm::vec4 Procedural::getColor(const glm::vec4& entityColor) {
    if (_data.version == 1) {
        return glm::vec4(1);
    }
    return entityColor;
}
