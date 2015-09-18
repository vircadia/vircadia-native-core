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

#include <gpu-networking/ShaderCache.h>
#include <gpu/Batch.h>
#include <SharedUtil.h>
#include <NumericalConstants.h>

#include "ProceduralShaders.h"

static const char* const UNIFORM_TIME_NAME= "iGlobalTime";
static const char* const UNIFORM_SCALE_NAME = "iWorldScale";
static const QString PROCEDURAL_USER_DATA_KEY = "ProceduralEntity";

static const QString URL_KEY = "shaderUrl";
static const QString VERSION_KEY = "version";
static const QString UNIFORMS_KEY = "uniforms";
static const std::string PROCEDURAL_BLOCK = "//PROCEDURAL_BLOCK";
static const std::string PROCEDURAL_COMMON_BLOCK = "//PROCEDURAL_COMMON_BLOCK";
static const std::string PROCEDURAL_VERSION = "//PROCEDURAL_VERSION";


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


Procedural::Procedural(const QString& userDataJson) {
    parse(userDataJson);
    _state = std::make_shared<gpu::State>();
}

void Procedural::parse(const QString& userDataJson) {
    _enabled = false;
    auto proceduralData = getProceduralData(userDataJson);
    if (proceduralData.isObject()) {
        parse(proceduralData.toObject());
    }
}

void Procedural::parse(const QJsonObject& proceduralData) {
    // grab the version number
    {
        auto version = proceduralData[VERSION_KEY];
        if (version.isDouble()) {
            _version = (uint8_t)(floor(version.toDouble()));
        }
    }

    // Get the path to the shader
    {
        QString shaderUrl = proceduralData[URL_KEY].toString();
        _shaderUrl = QUrl(shaderUrl);
        if (!_shaderUrl.isValid()) {
            qWarning() << "Invalid shader URL: " << shaderUrl;
            return;
        }

        if (_shaderUrl.isLocalFile()) {
            _shaderPath = _shaderUrl.toLocalFile();
            qDebug() << "Shader path: " << _shaderPath;
            if (!QFile(_shaderPath).exists()) {
                return;
            }
        } else {
            qDebug() << "Shader url: " << _shaderUrl;
            _networkShader = ShaderCache::instance().getShader(_shaderUrl);
        }
    }

    // Grab any custom uniforms
    {
        auto uniforms = proceduralData[UNIFORMS_KEY];
        if (uniforms.isObject()) {
            _uniforms = uniforms.toObject();;
        }
    }
    _enabled = true;
}

bool Procedural::ready() {
    if (!_enabled) {
        return false;
    }

    if (!_shaderPath.isEmpty()) {
        return true;
    }

    if (_networkShader) {
        return _networkShader->isLoaded();
    }

    return false;
}

void Procedural::prepare(gpu::Batch& batch, const glm::vec3& size) {
    if (_shaderUrl.isLocalFile()) {
        auto lastModified = (quint64) QFileInfo(_shaderPath).lastModified().toMSecsSinceEpoch();
        if (lastModified > _shaderModified) {
            QFile file(_shaderPath);
            file.open(QIODevice::ReadOnly);
            _shaderSource = QTextStream(&file).readAll();
            _pipelineDirty = true;
            _shaderModified = lastModified;
        }
    } else if (_networkShader && _networkShader->isLoaded()) {
        _shaderSource = _networkShader->_source;
    }

    if (!_pipeline || _pipelineDirty) {
        _pipelineDirty = true;
        if (!_vertexShader) {
            _vertexShader = gpu::ShaderPointer(gpu::Shader::createVertex(_vertexSource));
        }

        // Build the fragment shader
        std::string fragmentShaderSource = _fragmentSource;
        size_t replaceIndex = fragmentShaderSource.find(PROCEDURAL_COMMON_BLOCK);
        if (replaceIndex != std::string::npos) {
            fragmentShaderSource.replace(replaceIndex, PROCEDURAL_COMMON_BLOCK.size(), SHADER_COMMON);
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
        //qDebug() << "FragmentShader:\n" << fragmentShaderSource.c_str();
        _fragmentShader = gpu::ShaderPointer(gpu::Shader::createPixel(fragmentShaderSource));
        _shader = gpu::ShaderPointer(gpu::Shader::createProgram(_vertexShader, _fragmentShader));
        gpu::Shader::makeProgram(*_shader);
        _pipeline = gpu::PipelinePointer(gpu::Pipeline::create(_shader, _state));
        _timeSlot = _shader->getUniforms().findLocation(UNIFORM_TIME_NAME);
        _scaleSlot = _shader->getUniforms().findLocation(UNIFORM_SCALE_NAME);
        _start = usecTimestampNow();
    }

    batch.setPipeline(_pipeline);

    if (_pipelineDirty) {
        _pipelineDirty = false;
        // Set any userdata specified uniforms 
        foreach(QString key, _uniforms.keys()) {
            std::string uniformName = key.toLocal8Bit().data();
            int32_t slot = _shader->getUniforms().findLocation(uniformName);
            if (gpu::Shader::INVALID_LOCATION == slot) {
                continue;
            }
            QJsonValue value = _uniforms[key];
            if (value.isDouble()) {
                batch._glUniform1f(slot, value.toDouble());
            } else if (value.isArray()) {
                auto valueArray = value.toArray();
                switch (valueArray.size()) {
                    case 0:
                        break;

                    case 1:
                        batch._glUniform1f(slot, valueArray[0].toDouble());
                        break;
                    case 2:
                        batch._glUniform2f(slot, 
                            valueArray[0].toDouble(),
                            valueArray[1].toDouble());
                        break;
                    case 3:
                        batch._glUniform3f(slot, 
                            valueArray[0].toDouble(),
                            valueArray[1].toDouble(),
                            valueArray[2].toDouble());
                            break;
                    case 4:
                    default:
                        batch._glUniform4f(slot,
                            valueArray[0].toDouble(),
                            valueArray[1].toDouble(),
                            valueArray[2].toDouble(),
                            valueArray[3].toDouble());
                        break;

                }
                valueArray.size();
            }
        }
    }

    // Minimize floating point error by doing an integer division to milliseconds, before the floating point division to seconds
    float time = (float)((usecTimestampNow() - _start) / USECS_PER_MSEC) / MSECS_PER_SECOND;
    batch._glUniform1f(_timeSlot, time);
    // FIXME move into the 'set once' section, since this doesn't change over time
    batch._glUniform3f(_scaleSlot, size.x, size.y, size.z);
}


glm::vec4 Procedural::getColor(const glm::vec4& entityColor) {
    if (_version == 1) {
        return glm::vec4(1);
    }
    return entityColor;
}
