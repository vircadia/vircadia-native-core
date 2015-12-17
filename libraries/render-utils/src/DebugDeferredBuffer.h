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

#include <render/DrawTask.h>

class DebugDeferredBuffer {
public:
    using JobModel = render::Job::Model<DebugDeferredBuffer>;
    
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
        
        CustomMode // Needs to stay last
    };
    
    const std::string CUSTOM_FILE { "/Users/clement/Desktop/custom.slh" };
    
    bool pipelineNeedsUpdate(Modes mode) const;
    const gpu::PipelinePointer& getPipeline(Modes mode);
    std::string getShaderSourceCode(Modes mode);
    
    std::array<gpu::PipelinePointer, CustomMode + 1> _pipelines;
};

#endif // hifi_DebugDeferredBuffer_h