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

// FIXME better encapsulation
// FIXME better mechanism for extending to things rendered using shaders other than simple.slv
struct Procedural {
public:
    static QJsonValue getProceduralData(const QString& proceduralJson);

    Procedural();
    Procedural(const QString& userDataJson);
    void parse(const QString& userDataJson);

    bool ready();
    bool enabled() { return _enabled; }
    void prepare(gpu::Batch& batch, const glm::vec3& position, const glm::vec3& size, const glm::quat& orientation);
    const gpu::ShaderPointer& getShader() const { return _shader; }

    glm::vec4 getColor(const glm::vec4& entityColor);
    quint64 getFadeStartTime() { return _fadeStartTime; }
    bool isFading() { return _doesFade && _isFading; }
    void setIsFading(bool isFading) { _isFading = isFading; }
    void setDoesFade(bool doesFade) { _doesFade = doesFade; }

    uint8_t _version { 1 };

    std::string _vertexSource;
    std::string _fragmentSource;

    gpu::StatePointer _opaqueState { std::make_shared<gpu::State>() };
    gpu::StatePointer _transparentState { std::make_shared<gpu::State>() };

    enum StandardUniforms {
        DATE,
        TIME,
        FRAME_COUNT,
        SCALE,
        POSITION,
        ORIENTATION,
        CHANNEL_RESOLUTION,
        NUM_STANDARD_UNIFORMS
    };

protected:
    // Procedural metadata
    bool _enabled { false };
    uint64_t _start { 0 };
    int32_t _frameCount { 0 };

    // Rendering object descriptions, from userData
    QJsonObject _proceduralData;
    std::mutex _proceduralDataMutex;
    QString _shaderSource;
    QString _shaderPath;
    QUrl _shaderUrl;
    quint64 _shaderModified { 0 };
    NetworkShaderPointer _networkShader;
    QJsonObject _parsedUniforms;
    QJsonArray _parsedChannels;
    std::atomic_bool _proceduralDataDirty;
    bool _shaderDirty { true };
    bool _uniformsDirty { true };
    bool _channelsDirty { true };

    // Rendering objects
    UniformLambdas _uniforms;
    int32_t _standardUniformSlots[NUM_STANDARD_UNIFORMS];
    NetworkTexturePointer _channels[MAX_PROCEDURAL_TEXTURE_CHANNELS];
    gpu::PipelinePointer _opaquePipeline;
    gpu::PipelinePointer _transparentPipeline;
    gpu::ShaderPointer _vertexShader;
    gpu::ShaderPointer _fragmentShader;
    gpu::ShaderPointer _shader;

    // Entity metadata
    glm::vec3 _entityDimensions;
    glm::vec3 _entityPosition;
    glm::mat3 _entityOrientation;

private:
    // This should only be called from the render thread, as it shares data with Procedural::prepare
    void parse(const QJsonObject&);
    bool parseVersion(const QJsonValue& version);
    bool parseUrl(const QUrl& url);
    bool parseUniforms(const QJsonObject& uniforms);
    bool parseTextures(const QJsonArray& channels);

    void setupUniforms();
    void setupChannels(bool shouldCreate);

    quint64 _fadeStartTime;
    bool _hasStartedFade { false };
    bool _isFading { false };
    bool _doesFade { true };
};

#endif
