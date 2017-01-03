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

class RenderForwardTask : public render::Task {
public:
    using JobModel = Model<RenderForwardTask>;

    RenderForwardTask(RenderFetchCullSortTask::Output items);
};

class PrepareFramebuffer {
public:
    using Inputs = gpu::FramebufferPointer;
    using JobModel = render::Job::ModelO<PrepareFramebuffer, Inputs>;

    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext,
            gpu::FramebufferPointer& framebuffer);

private:
    gpu::FramebufferPointer _framebuffer;
};

class Draw {
public:
    using Inputs = render::ItemBounds;
    using JobModel = render::Job::ModelI<Draw, Inputs>;

    Draw(const render::ShapePlumberPointer& shapePlumber) : _shapePlumber(shapePlumber) {}
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext,
            const Inputs& items);

private:
    render::ShapePlumberPointer _shapePlumber;
};

class DrawBackground {
public:
    using Inputs = render::ItemBounds;
    using JobModel = render::Job::ModelI<DrawBackground, Inputs>;

    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext,
            const Inputs& background);
};

class DrawBounds {
public:
    class Config : public render::JobConfig {
    public:
        Config() : JobConfig(false) {}
    };

    using Inputs = render::ItemBounds;
    using JobModel = render::Job::ModelI<DrawBounds, Inputs, Config>;

    void configure(const Config& configuration) {}
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext,
            const Inputs& items);

private:
    const gpu::PipelinePointer getPipeline();
    gpu::PipelinePointer _boundsPipeline;
    int _cornerLocation { -1 };
    int _scaleLocation { -1 };
};

#endif // hifi_RenderForwardTask_h
