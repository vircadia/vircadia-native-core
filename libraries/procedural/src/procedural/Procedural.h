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

#include <gpu/Shader.h>
#include <gpu/Pipeline.h>
#include <gpu/Batch.h>
#include <model-networking/ShaderCache.h>

// FIXME better encapsulation
// FIXME better mechanism for extending to things rendered using shaders other than simple.slv
struct Procedural {
    static QJsonValue getProceduralData(const QString& proceduralJson);

    Procedural(const QString& userDataJson);
    void parse(const QString& userDataJson);
    void parse(const QJsonObject&);
    bool ready();
    void prepare(gpu::Batch& batch, const glm::vec3& size);
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
    int32_t _timeSlot{ gpu::Shader::INVALID_LOCATION };
    int32_t _scaleSlot{ gpu::Shader::INVALID_LOCATION };
    uint64_t _start{ 0 };
    NetworkShaderPointer _networkShader;
    QJsonObject _uniforms;

    gpu::PipelinePointer _pipeline;
    gpu::ShaderPointer _vertexShader;
    gpu::ShaderPointer _fragmentShader;
    gpu::ShaderPointer _shader;
    gpu::StatePointer _state;
};

#endif
