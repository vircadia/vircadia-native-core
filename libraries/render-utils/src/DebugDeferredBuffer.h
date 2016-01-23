//
//  DebugDeferredBuffer.h
//  libraries/render-utils/src
//
//  Created by Clement on 12/3/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DebugDeferredBuffer_h
#define hifi_DebugDeferredBuffer_h

#include <QFileInfo>

#include <render/DrawTask.h>

class DebugDeferredBufferConfig : public render::Job::Config {
    Q_OBJECT
public:
    DebugDeferredBufferConfig() : render::Job::Config(false) {}

    Q_PROPERTY(int mode MEMBER mode WRITE setMode NOTIFY dirty)
    Q_PROPERTY(glm::vec4 size MEMBER size NOTIFY dirty)
    void setMode(int newMode);
 
    int mode{ 0 };
    glm::vec4 size{ 0, 0, 0, 0 };
signals:
    void dirty();
};

class DebugDeferredBuffer {
public:
    using Config = DebugDeferredBufferConfig;
    using JobModel = render::Job::Model<DebugDeferredBuffer, Config>;
    
    DebugDeferredBuffer();

    void configure(const Config& config) {
        _mode = (Mode)config.mode;
        _size = config.size;
    }
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);
    
protected:
    friend class Config;

    enum Mode : uint8_t {
        Diffuse = 0,
        Specular,
        Roughness,
        Normal,
        Depth,
        Lighting,
        Shadow,
        PyramidDepth,
        AmbientOcclusion,
        AmbientOcclusionBlurred,
        Custom // Needs to stay last
    };

private:
    Mode _mode;
    glm::vec4 _size;

    struct CustomPipeline {
        gpu::PipelinePointer pipeline;
        mutable QFileInfo info;
    };
    using StandardPipelines = std::array<gpu::PipelinePointer, Custom>;
    using CustomPipelines = std::unordered_map<std::string, CustomPipeline>;
    
    bool pipelineNeedsUpdate(Mode mode, std::string customFile = std::string()) const;
    const gpu::PipelinePointer& getPipeline(Mode mode, std::string customFile = std::string());
    std::string getShaderSourceCode(Mode mode, std::string customFile = std::string());
    
    StandardPipelines _pipelines;
    CustomPipelines _customPipelines;
};

#endif // hifi_DebugDeferredBuffer_h