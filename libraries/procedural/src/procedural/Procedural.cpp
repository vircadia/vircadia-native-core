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

#include <gpu/Batch.h>
#include <SharedUtil.h>
#include <NumericalConstants.h>
#include <GLMHelpers.h>

#include "ProceduralCommon_frag.h"

// Userdata parsing constants
static const QString PROCEDURAL_USER_DATA_KEY = "ProceduralEntity";
static const QString URL_KEY = "shaderUrl";
static const QString VERSION_KEY = "version";
static const QString UNIFORMS_KEY = "uniforms";
static const QString CHANNELS_KEY = "channels";

// Shader replace strings
static const std::string PROCEDURAL_BLOCK = "//PROCEDURAL_BLOCK";
static const std::string PROCEDURAL_COMMON_BLOCK = "//PROCEDURAL_COMMON_BLOCK";
static const std::string PROCEDURAL_VERSION = "//PROCEDURAL_VERSION";

static const std::string STANDARD_UNIFORM_NAMES[Procedural::NUM_STANDARD_UNIFORMS] = {
    "iDate",
    "iGlobalTime",
    "iFrameCount",
    "iWorldScale",
    "iWorldPosition",
    "iWorldOrientation",
    "iChannelResolution"
};

// Example
//{
//    "ProceduralEntity": {
//        "shaderUrl": "file:///C:/Users/bdavis/Git/hifi/examples/shaders/test.fs",
//    }
//}
QJsonValue Procedural::getProceduralData(const QString& proceduralJson) {
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

Procedural::Procedural() : _state { std::make_shared<gpu::State>() } {
    _proceduralDataDirty = false;
}

Procedural::Procedural(const QString& userDataJson) : Procedural() {
    parse(userDataJson);
    _proceduralDataDirty = true;
}

void Procedural::parse(const QString& userDataJson) {
    auto proceduralData = getProceduralData(userDataJson);
    // Instead of parsing, prep for a parse on the rendering thread
    // This will be called by Procedural::ready
    std::lock_guard<std::mutex> lock(_proceduralDataMutex);
    _proceduralData = proceduralData.toObject();

    // Mark as dirty after modifying _proceduralData, but before releasing lock
    // to avoid setting it after parsing has begun
    _proceduralDataDirty = true;
}

bool Procedural::parseVersion(const QJsonValue& version) {
    if (version.isDouble()) {
        _version = (uint8_t)(floor(version.toDouble()));
    } else {
        // All unversioned shaders default to V1
        _version = 1;
    }
    return (_version == 1 || _version == 2);
}

bool Procedural::parseUrl(const QUrl& shaderUrl) {
    if (!shaderUrl.isValid()) {
        if (!shaderUrl.isEmpty()) {
            qWarning() << "Invalid shader URL: " << shaderUrl;
        }
        _networkShader.reset();
        return false;
    }

    _shaderUrl = shaderUrl;
    _shaderDirty = true;

    if (_shaderUrl.isLocalFile()) {
        _shaderPath = _shaderUrl.toLocalFile();
        qDebug() << "Shader path: " << _shaderPath;
        if (!QFile(_shaderPath).exists()) {
            _networkShader.reset();
            return false;;
        }
    } else {
        qDebug() << "Shader url: " << _shaderUrl;
        _networkShader = ShaderCache::instance().getShader(_shaderUrl);
    }

    return true;
}

bool Procedural::parseUniforms(const QJsonObject& uniforms) {
    if (_parsedUniforms != uniforms) {
        _parsedUniforms = uniforms;
        _uniformsDirty = true;
    }

    return true;
}

bool Procedural::parseTextures(const QJsonArray& channels) {
    if (_parsedChannels != channels) {
        _parsedChannels = channels;

        auto textureCache = DependencyManager::get<TextureCache>();
        size_t channelCount = std::min(MAX_PROCEDURAL_TEXTURE_CHANNELS, (size_t)_parsedChannels.size());
        size_t channel = 0;
        for (; channel < channelCount; ++channel) {
            QString url = _parsedChannels.at((int)channel).toString();
            _channels[channel] = textureCache->getTexture(QUrl(url));
        }
        for (; channel < MAX_PROCEDURAL_TEXTURE_CHANNELS; ++channel) {
            // Release those textures no longer in use
            _channels[channel] = textureCache->getTexture(QUrl());
        }

        _channelsDirty = true;
    }

    return true;
}

void Procedural::parse(const QJsonObject& proceduralData) {
    _enabled = false;

    auto version = proceduralData[VERSION_KEY];
    auto shaderUrl = proceduralData[URL_KEY].toString();
    shaderUrl = ResourceManager::normalizeURL(shaderUrl);
    auto uniforms = proceduralData[UNIFORMS_KEY].toObject();
    auto channels = proceduralData[CHANNELS_KEY].toArray();

    bool isValid = true;

    // Run through parsing regardless of validity to clear old cached resources
    isValid = parseVersion(version) && isValid;
    isValid = parseUrl(shaderUrl) && isValid;
    isValid = parseUniforms(uniforms) && isValid;
    isValid = parseTextures(channels) && isValid;

    if (!proceduralData.isEmpty() && isValid) {
        _enabled = true;
    }
}

bool Procedural::ready() {
    // Load any changes to the procedural
    // Check for changes atomically, in case they are currently being made
    if (_proceduralDataDirty) {
        std::lock_guard<std::mutex> lock(_proceduralDataMutex);
        parse(_proceduralData);

        // Reset dirty flag after reading _proceduralData, but before releasing lock
        // to avoid resetting it after more data is set
        _proceduralDataDirty = false;
    }

    if (!_enabled) {
        return false;
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

    return true;
}

void Procedural::prepare(gpu::Batch& batch, const glm::vec3& position, const glm::vec3& size, const glm::quat& orientation) {
    _entityDimensions = size;
    _entityPosition = position;
    _entityOrientation = glm::mat3_cast(orientation);
    if (_shaderUrl.isLocalFile()) {
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

    if (!_pipeline || _shaderDirty) {
        if (!_vertexShader) {
            _vertexShader = gpu::Shader::createVertex(_vertexSource);
        }

        // Build the fragment shader
        std::string fragmentShaderSource = _fragmentSource;
        size_t replaceIndex = fragmentShaderSource.find(PROCEDURAL_COMMON_BLOCK);
        if (replaceIndex != std::string::npos) {
            fragmentShaderSource.replace(replaceIndex, PROCEDURAL_COMMON_BLOCK.size(), ProceduralCommon_frag);
        }

        replaceIndex = fragmentShaderSource.find(PROCEDURAL_VERSION);
        if (replaceIndex != std::string::npos) {
            if (_version == 1) {
                fragmentShaderSource.replace(replaceIndex, PROCEDURAL_VERSION.size(), "#define PROCEDURAL_V1 1");
            } else if (_version == 2) {
                fragmentShaderSource.replace(replaceIndex, PROCEDURAL_VERSION.size(), "#define PROCEDURAL_V2 1");
            }
        }
        replaceIndex = fragmentShaderSource.find(PROCEDURAL_BLOCK);
        if (replaceIndex != std::string::npos) {
            fragmentShaderSource.replace(replaceIndex, PROCEDURAL_BLOCK.size(), _shaderSource.toLocal8Bit().data());
        }

        // Leave this here for debugging
        // qDebug() << "FragmentShader:\n" << fragmentShaderSource.c_str();

        _fragmentShader = gpu::Shader::createPixel(fragmentShaderSource);
        _shader = gpu::Shader::createProgram(_vertexShader, _fragmentShader);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("iChannel0"), 0));
        slotBindings.insert(gpu::Shader::Binding(std::string("iChannel1"), 1));
        slotBindings.insert(gpu::Shader::Binding(std::string("iChannel2"), 2));
        slotBindings.insert(gpu::Shader::Binding(std::string("iChannel3"), 3));
        gpu::Shader::makeProgram(*_shader, slotBindings);

        _pipeline = gpu::Pipeline::create(_shader, _state);
        for (size_t i = 0; i < NUM_STANDARD_UNIFORMS; ++i) {
            const std::string& name = STANDARD_UNIFORM_NAMES[i];
            _standardUniformSlots[i] = _shader->getUniforms().findLocation(name);
        }
        _start = usecTimestampNow();
        _frameCount = 0;
    }

    batch.setPipeline(_pipeline);

    if (_shaderDirty || _uniformsDirty) {
        setupUniforms();
    }

    if (_shaderDirty || _uniformsDirty || _channelsDirty) {
        setupChannels(_shaderDirty || _uniformsDirty);
    }

    _shaderDirty = _uniformsDirty = _channelsDirty = false;

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
                gpuTexture->autoGenerateMips(-1);
            }
            batch.setResourceTexture((gpu::uint32)i, gpuTexture);
        }
    }
}

void Procedural::setupUniforms() {
    _uniforms.clear();
    // Set any userdata specified uniforms 
    foreach(QString key, _parsedUniforms.keys()) {
        std::string uniformName = key.toLocal8Bit().data();
        int32_t slot = _shader->getUniforms().findLocation(uniformName);
        if (gpu::Shader::INVALID_LOCATION == slot) {
            continue;
        }
        QJsonValue value = _parsedUniforms[key];
        if (value.isDouble()) {
            float v = value.toDouble();
            _uniforms.push_back([=](gpu::Batch& batch) {
                batch._glUniform1f(slot, v);
            });
        } else if (value.isArray()) {
            auto valueArray = value.toArray();
            switch (valueArray.size()) {
                case 0:
                    break;

                case 1: {
                    float v = valueArray[0].toDouble();
                    _uniforms.push_back([=](gpu::Batch& batch) {
                        batch._glUniform1f(slot, v);
                    });
                    break;
                }

                case 2: {
                    glm::vec2 v{ valueArray[0].toDouble(), valueArray[1].toDouble() };
                    _uniforms.push_back([=](gpu::Batch& batch) {
                        batch._glUniform2f(slot, v.x, v.y);
                    });
                    break;
                }

                case 3: {
                    glm::vec3 v{
                        valueArray[0].toDouble(),
                        valueArray[1].toDouble(),
                        valueArray[2].toDouble(),
                    };
                    _uniforms.push_back([=](gpu::Batch& batch) {
                        batch._glUniform3f(slot, v.x, v.y, v.z);
                    });
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
                    _uniforms.push_back([=](gpu::Batch& batch) {
                        batch._glUniform4f(slot, v.x, v.y, v.z, v.w);
                    });
                    break;
                }
            }
        }
    }

    if (gpu::Shader::INVALID_LOCATION != _standardUniformSlots[TIME]) {
        _uniforms.push_back([=](gpu::Batch& batch) {
            // Minimize floating point error by doing an integer division to milliseconds, before the floating point division to seconds
            float time = (float)((usecTimestampNow() - _start) / USECS_PER_MSEC) / MSECS_PER_SECOND;
            batch._glUniform(_standardUniformSlots[TIME], time);
        });
    }

    if (gpu::Shader::INVALID_LOCATION != _standardUniformSlots[DATE]) {
        _uniforms.push_back([=](gpu::Batch& batch) {
            QDateTime now = QDateTime::currentDateTimeUtc();
            QDate date = now.date();
            QTime time = now.time();
            vec4 v;
            v.x = date.year();
            // Shadertoy month is 0 based
            v.y = date.month() - 1;
            // But not the day... go figure
            v.z = date.day();
            float fractSeconds = (time.msec() / 1000.0f);
            v.w = (time.hour() * 3600) + (time.minute() * 60) + time.second() + fractSeconds;
            batch._glUniform(_standardUniformSlots[DATE], v);
        });
    }

    if (gpu::Shader::INVALID_LOCATION != _standardUniformSlots[FRAME_COUNT]) {
        _uniforms.push_back([=](gpu::Batch& batch) {
            batch._glUniform(_standardUniformSlots[FRAME_COUNT], ++_frameCount);
        });
    }

    if (gpu::Shader::INVALID_LOCATION != _standardUniformSlots[SCALE]) {
        // FIXME move into the 'set once' section, since this doesn't change over time
        _uniforms.push_back([=](gpu::Batch& batch) {
            batch._glUniform(_standardUniformSlots[SCALE], _entityDimensions);
        });
    }

    if (gpu::Shader::INVALID_LOCATION != _standardUniformSlots[ORIENTATION]) {
        // FIXME move into the 'set once' section, since this doesn't change over time
        _uniforms.push_back([=](gpu::Batch& batch) {
            batch._glUniform(_standardUniformSlots[ORIENTATION], _entityOrientation);
        });
    }

    if (gpu::Shader::INVALID_LOCATION != _standardUniformSlots[POSITION]) {
        // FIXME move into the 'set once' section, since this doesn't change over time
        _uniforms.push_back([=](gpu::Batch& batch) {
            batch._glUniform(_standardUniformSlots[POSITION], _entityPosition);
        });
    }
}

void Procedural::setupChannels(bool shouldCreate) {
    if (gpu::Shader::INVALID_LOCATION != _standardUniformSlots[CHANNEL_RESOLUTION]) {
        if (!shouldCreate) {
            // Instead of modifying the last element, just remove and recreate it.
            _uniforms.pop_back();
        }
        _uniforms.push_back([=](gpu::Batch& batch) {
            vec3 channelSizes[MAX_PROCEDURAL_TEXTURE_CHANNELS];
            for (size_t i = 0; i < MAX_PROCEDURAL_TEXTURE_CHANNELS; ++i) {
                if (_channels[i]) {
                    channelSizes[i] = vec3(_channels[i]->getWidth(), _channels[i]->getHeight(), 1.0);
                }
            }
            batch._glUniform3fv(_standardUniformSlots[CHANNEL_RESOLUTION], MAX_PROCEDURAL_TEXTURE_CHANNELS, &channelSizes[0].x);
        });
    }
}

glm::vec4 Procedural::getColor(const glm::vec4& entityColor) {
    if (_version == 1) {
        return glm::vec4(1);
    }
    return entityColor;
}
