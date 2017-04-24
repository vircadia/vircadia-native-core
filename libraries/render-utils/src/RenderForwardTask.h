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
#include <render/RenderFetchCullSortTask.h>
#include "LightingModel.h"

class RenderForwardTask {
public:
    using Input = RenderFetchCullSortTask::Output;
    using JobModel = render::Task::ModelI<RenderForwardTask, Input>;

    RenderForwardTask() {}

    void build(JobModel& task, const render::Varying& inputs, render::Varying& outputs);
};

class PrepareFramebuffer {
public:
    using Inputs = gpu::FramebufferPointer;
    using JobModel = render::Job::ModelO<PrepareFramebuffer, Inputs>;

    void run(const render::RenderContextPointer& renderContext,
            gpu::FramebufferPointer& framebuffer);

private:
    gpu::FramebufferPointer _framebuffer;
};

class Draw {
public:
    using Inputs = render::ItemBounds;
    using JobModel = render::Job::ModelI<Draw, Inputs>;

    Draw(const render::ShapePlumberPointer& shapePlumber) : _shapePlumber(shapePlumber) {}
    void run(const render::RenderContextPointer& renderContext,
            const Inputs& items);

private:
    render::ShapePlumberPointer _shapePlumber;
};

class Stencil {
public:
    using JobModel = render::Job::Model<Stencil>;

    void run(const render::RenderContextPointer& renderContext);

private:
    const gpu::PipelinePointer getPipeline();
    gpu::PipelinePointer _stencilPipeline;
};

class DrawBackground {
public:
    using Inputs = render::ItemBounds;
    using JobModel = render::Job::ModelI<DrawBackground, Inputs>;

    void run(const render::RenderContextPointer& renderContext,
            const Inputs& background);
};

#endif // hifi_RenderForwardTask_h
