//
//  RenderForwardTask.h
//  render-utils/src/
//
//  Created by Zach Pomerantz on 12/13/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderForwardTask_h
#define hifi_RenderForwardTask_h

#include <gpu/Pipeline.h>
#include <render/CullTask.h>
#include "LightingModel.h"

using RenderForwardTaskConfig = render::GPUTaskConfig;

class RenderForwardTask : public render::Task {
public:
    using Config = RenderForwardTaskConfig;
    RenderForwardTask(render::CullFunctor cullFunctor);

    void configure(const Config& config) {}
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

    using JobModel = Model<RenderForwardTask, Config>;
};

class PrepareFramebuffer {
public:
    using JobModel = render::Job::ModelO<PrepareFramebuffer, gpu::FramebufferPointer>;

    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, gpu::FramebufferPointer& framebuffer);

private:
    gpu::FramebufferPointer _framebuffer;
};

class DrawBounds {
public:
    using Inputs = render::ItemBounds;
    using JobModel = render::Job::ModelI<DrawBounds, Inputs>;

    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& items);

private:
    const gpu::PipelinePointer getPipeline();
    gpu::PipelinePointer _boundsPipeline;
    int _cornerLocation { -1 };
    int _scaleLocation { -1 };
};

#endif // hifi_RenderForwardTask_h