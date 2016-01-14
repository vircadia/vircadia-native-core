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

class DebugDeferredBuffer {
public:
    using JobModel = render::Task::Job::Model<DebugDeferredBuffer>;
    
    DebugDeferredBuffer();
    
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);
    
private:
    enum Modes : uint8_t {
        DiffuseMode = 0,
        AlphaMode,
        SpecularMode,
        RoughnessMode,
        NormalMode,
        DepthMode,
        LightingMode,
        ShadowMode,
        
        CustomMode // Needs to stay last
    };
    struct CustomPipeline {
        gpu::PipelinePointer pipeline;
        mutable QFileInfo info;
    };
    using StandardPipelines = std::array<gpu::PipelinePointer, CustomMode>;
    using CustomPipelines = std::unordered_map<std::string, CustomPipeline>;
    
    bool pipelineNeedsUpdate(Modes mode, std::string customFile = std::string()) const;
    const gpu::PipelinePointer& getPipeline(Modes mode, std::string customFile = std::string());
    std::string getShaderSourceCode(Modes mode, std::string customFile = std::string());
    
    StandardPipelines _pipelines;
    CustomPipelines _customPipelines;
};

#endif // hifi_DebugDeferredBuffer_h