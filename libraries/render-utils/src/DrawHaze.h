//
//  DrawHaze.h
//  libraries/render-utils/src
//
//  Created by Nissim Hadar on 9/1/2017.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_utils_DrawHaze_h
#define hifi_render_utils_DrawHaze_h

#include <DependencyManager.h>
#include <NumericalConstants.h>

#include <gpu/Resource.h>
#include <gpu/Pipeline.h>
#include <render/Forward.h>
#include <render/DrawTask.h>

#include "SurfaceGeometryPass.h"
#include "LightingModel.h"

#include "HazeStage.h"
#include "LightStage.h"

using LinearDepthFramebufferPointer = std::shared_ptr<LinearDepthFramebuffer>;

class DrawHaze {
public:
    using Inputs = render::VaryingSet6<HazeStage::FramePointer, gpu::FramebufferPointer, LinearDepthFramebufferPointer, DeferredFrameTransformPointer, LightingModelPointer, LightStage::FramePointer>;
    using JobModel = render::Job::ModelI<DrawHaze, Inputs>;

    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

private:
    gpu::PipelinePointer _hazePipeline;
};

#endif // hifi_render_utils_DrawHaze_h
