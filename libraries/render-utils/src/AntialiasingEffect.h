//
//  AntialiasingEffect.h
//  libraries/render-utils/src/
//
//  Created by Raffi Bedikian on 8/30/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AntialiasingEffect_h
#define hifi_AntialiasingEffect_h

#include <DependencyManager.h>

#include "render/DrawTask.h"

class Antialiasing {
public:

    Antialiasing();

    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);
    using JobModel = render::Task::Job::Model<Antialiasing>;

    const gpu::PipelinePointer& getAntialiasingPipeline();
    const gpu::PipelinePointer& getBlendPipeline();

private:

    // Uniforms for AA
    gpu::int32 _texcoordOffsetLoc;

    gpu::FramebufferPointer _antialiasingBuffer;

    gpu::TexturePointer _antialiasingTexture;

    gpu::PipelinePointer _antialiasingPipeline;
    gpu::PipelinePointer _blendPipeline;

};

#endif // hifi_AntialiasingEffect_h
