//
//  Created by Bradley Austin Davis on 2015/09/05
//  Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderableProceduralItem.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include <ShaderCache.h>
#include <EntityItem.h>
#include <TextureCache.h>
#include <DeferredLightingEffect.h>
#include <gpu/Batch.h>

#include "RenderableProceduralItemShader.h"
#include "../render-utils/simple_vert.h"

static const char* const UNIFORM_TIME_NAME= "iGlobalTime";
static const char* const UNIFORM_SCALE_NAME = "iWorldScale";
static const QString PROCEDURAL_USER_DATA_KEY = "ProceduralEntity";

static const QString URL_KEY = "shaderUrl";
static const QString VERSION_KEY = "version";
static const QString UNIFORMS_KEY = "uniforms";

RenderableProceduralItem::ProceduralInfo::ProceduralInfo(EntityItem* entity) : _entity(entity) {
    parse();
}

void RenderableProceduralItem::ProceduralInfo::parse() {
    _enabled = false;
    QJsonObject userData;
    {
        const QString& userDataJson = _entity->getUserData();
        if (userDataJson.isEmpty()) {
            return;
        }
        QJsonParseError parseError;
        auto doc = QJsonDocument::fromJson(userDataJson.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            return;
        }
        userData = doc.object();
    }

    // Example
    //{
    //    "ProceduralEntity": {
    //        "shaderUrl": "file:///C:/Users/bdavis/Git/hifi/examples/shaders/test.fs",
    //        "color" : "#FFFFFF"
    //    }
    //}
    auto proceduralData = userData[PROCEDURAL_USER_DATA_KEY];
    if (proceduralData.isNull()) {
        return;
    }

    parse(proceduralData.toObject());
}

void RenderableProceduralItem::ProceduralInfo::parse(const QJsonObject& proceduralData) {
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

bool RenderableProceduralItem::ProceduralInfo::ready() {
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

void RenderableProceduralItem::ProceduralInfo::prepare(gpu::Batch& batch) {
    if (_shaderUrl.isLocalFile()) {
        auto lastModified = QFileInfo(_shaderPath).lastModified().toMSecsSinceEpoch();
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
            _vertexShader = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(simple_vert)));
        }
        QString framentShaderSource;
        switch (_version) {
            case 1:
                framentShaderSource = SHADER_TEMPLATE_V1.arg(_shaderSource);
                break;

            default:
            case 2:
                framentShaderSource = SHADER_TEMPLATE_V2.arg(_shaderSource);
                break;
        }
        _fragmentShader = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(framentShaderSource.toLocal8Bit().data())));
        _shader = gpu::ShaderPointer(gpu::Shader::createProgram(_vertexShader, _fragmentShader));
        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("normalFittingMap"), DeferredLightingEffect::NORMAL_FITTING_MAP_SLOT));
        gpu::Shader::makeProgram(*_shader, slotBindings);
        auto state = std::make_shared<gpu::State>();
        state->setCullMode(gpu::State::CULL_NONE);
        state->setDepthTest(true, true, gpu::LESS_EQUAL);
        state->setBlendFunction(false,
            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
            gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
        _pipeline = gpu::PipelinePointer(gpu::Pipeline::create(_shader, state));
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
                            valueArray[0].toDouble(),
                            valueArray[1].toDouble());
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
    auto scale = _entity->getDimensions();
    batch._glUniform3f(_scaleSlot, scale.x, scale.y, scale.z);
    batch.setResourceTexture(DeferredLightingEffect::NORMAL_FITTING_MAP_SLOT, DependencyManager::get<TextureCache>()->getNormalFittingTexture());
}


glm::vec4 RenderableProceduralItem::ProceduralInfo::getColor(const glm::vec4& entityColor) {
    if (_version == 1) {
        return glm::vec4(1);
    }
    return entityColor;
}