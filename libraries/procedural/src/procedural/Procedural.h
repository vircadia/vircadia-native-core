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

#include <atomic>

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



struct ProceduralData {
    static QJsonValue getProceduralData(const QString& proceduralJson);
    static ProceduralData parse(const QString& userDataJson);
    // This should only be called from the render thread, as it shares data with Procedural::prepare
    void parse(const QJsonObject&);

    // Rendering object descriptions, from userData
    uint8_t version { 0 };
    QUrl shaderUrl;
    QJsonObject uniforms;
    QJsonArray channels;
};


// WARNING with threaded rendering it is the RESPONSIBILITY OF THE CALLER to ensure that 
// calls to `setProceduralData` happen on the main thread and that calls to `ready` and `prepare` 
// are treated atomically, and that they cannot happen concurrently with calls to `setProceduralData`
// FIXME better encapsulation
// FIXME better mechanism for extending to things rendered using shaders other than simple.slv
struct Procedural {
public:
    Procedural();
    void setProceduralData(const ProceduralData& proceduralData);

    bool isReady() const;
    bool isEnabled() const { return _enabled; }
    void prepare(gpu::Batch& batch, const glm::vec3& position, const glm::vec3& size, const glm::quat& orientation, const glm::vec4& color = glm::vec4(1));
    const gpu::ShaderPointer& getOpaqueShader() const { return _opaqueShader; }

    glm::vec4 getColor(const glm::vec4& entityColor);
    quint64 getFadeStartTime() const { return _fadeStartTime; }
    bool isFading() const { return _doesFade && _isFading; }
    void setIsFading(bool isFading) { _isFading = isFading; }
    void setDoesFade(bool doesFade) { _doesFade = doesFade; }

    gpu::Shader::Source _vertexSource;
    gpu::Shader::Source _opaqueFragmentSource;
    gpu::Shader::Source _transparentFragmentSource;

    gpu::StatePointer _opaqueState { std::make_shared<gpu::State>() };
    gpu::StatePointer _transparentState { std::make_shared<gpu::State>() };


protected:
    // DO NOT TOUCH
    // We have to pack these in a particular way to match the ProceduralCommon.slh
    // layout.  
    struct StandardInputs {
        vec4 date;
        vec4 position; 
        vec4 scale;
        float time;
        int frameCount;
        vec2 _spare1;
        vec4 resolution[4];
        mat4 orientation;
    };

    static_assert(0 == offsetof(StandardInputs, date), "ProceduralOffsets");
    static_assert(16 == offsetof(StandardInputs, position), "ProceduralOffsets");
    static_assert(32 == offsetof(StandardInputs, scale), "ProceduralOffsets");
    static_assert(48 == offsetof(StandardInputs, time), "ProceduralOffsets");
    static_assert(52 == offsetof(StandardInputs, frameCount), "ProceduralOffsets");
    static_assert(56 == offsetof(StandardInputs, _spare1), "ProceduralOffsets");
    static_assert(64 == offsetof(StandardInputs, resolution), "ProceduralOffsets");
    static_assert(128 == offsetof(StandardInputs, orientation), "ProceduralOffsets");

    // Procedural metadata
    ProceduralData _data;

    bool _enabled { false };
    uint64_t _start { 0 };
    int32_t _frameCount { 0 };

    // Rendering object descriptions, from userData
    QString _shaderSource;
    QString _shaderPath;
    quint64 _shaderModified { 0 };
    NetworkShaderPointer _networkShader;
    bool _dirty { false };
    bool _shaderDirty { true };
    bool _uniformsDirty { true };

    // Rendering objects
    UniformLambdas _uniforms;
    NetworkTexturePointer _channels[MAX_PROCEDURAL_TEXTURE_CHANNELS];
    gpu::PipelinePointer _opaquePipeline;
    gpu::PipelinePointer _transparentPipeline;
    StandardInputs _standardInputs;
    gpu::BufferPointer _standardInputsBuffer;
    gpu::ShaderPointer _vertexShader;
    gpu::ShaderPointer _opaqueFragmentShader;
    gpu::ShaderPointer _transparentFragmentShader;
    gpu::ShaderPointer _opaqueShader;
    gpu::ShaderPointer _transparentShader;

    // Entity metadata
    glm::vec3 _entityDimensions;
    glm::vec3 _entityPosition;
    glm::mat3 _entityOrientation;

private:
    // This should only be called from the render thread, as it shares data with Procedural::prepare
    void setupUniforms(bool transparent);

    mutable quint64 _fadeStartTime { 0 };
    mutable bool _hasStartedFade { false };
    mutable bool _isFading { false };
    bool _doesFade { true };
    bool _prevTransparent { false };
};

#endif
