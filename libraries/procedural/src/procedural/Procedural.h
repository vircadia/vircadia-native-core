//
//  Created by Bradley Austin Davis on 2015/09/05
//  Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_RenderableProcedrualItem_h
#define hifi_RenderableProcedrualItem_h

#include <QtCore/qglobal.h>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

#include <gpu/Shader.h>
#include <gpu/Pipeline.h>
#include <gpu/Batch.h>
#include <model-networking/ShaderCache.h>
#include <model-networking/TextureCache.h>

using UniformLambdas = std::list<std::function<void(gpu::Batch& batch)>>;
const size_t MAX_PROCEDURAL_TEXTURE_CHANNELS{ 4 };

// FIXME better encapsulation
// FIXME better mechanism for extending to things rendered using shaders other than simple.slv
struct Procedural {
    static QJsonValue getProceduralData(const QString& proceduralJson);

    Procedural(const QString& userDataJson);
    void parse(const QString& userDataJson);
    void parse(const QJsonObject&);
    bool ready();
    void prepare(gpu::Batch& batch, const glm::vec3& position, const glm::vec3& size);
    void setupUniforms();
    glm::vec4 getColor(const glm::vec4& entityColor);

    bool _enabled{ false };
    uint8_t _version{ 1 };

    std::string _vertexSource;
    std::string _fragmentSource;

    QString _shaderSource;
    QString _shaderPath;
    QUrl _shaderUrl;
    quint64 _shaderModified{ 0 };
    bool _pipelineDirty{ true };

    enum StandardUniforms {
        DATE,
        TIME,
        FRAME_COUNT,
        SCALE,
        POSITION,
        CHANNEL_RESOLUTION,
        NUM_STANDARD_UNIFORMS
    };

    int32_t _standardUniformSlots[NUM_STANDARD_UNIFORMS];

    uint64_t _start{ 0 };
    int32_t _frameCount{ 0 };
    NetworkShaderPointer _networkShader;
    QJsonObject _parsedUniforms;
    QJsonArray _parsedChannels;

    UniformLambdas _uniforms;
    NetworkTexturePointer _channels[MAX_PROCEDURAL_TEXTURE_CHANNELS];
    gpu::PipelinePointer _pipeline;
    gpu::ShaderPointer _vertexShader;
    gpu::ShaderPointer _fragmentShader;
    gpu::ShaderPointer _shader;
    gpu::StatePointer _state;
    glm::vec3 _entityDimensions;
    glm::vec3 _entityPosition;
};

#endif
