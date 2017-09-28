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

#include "model/Haze.h"

using LinearDepthFramebufferPointer = std::shared_ptr<LinearDepthFramebuffer>;

class HazeConfig : public render::Job::Config {
public:
    HazeConfig() : render::Job::Config(true) {}

    // attributes
    float hazeColorR{ initialHazeColor.r };
    float hazeColorG{ initialHazeColor.g };
    float hazeColorB{ initialHazeColor.b };
    float directionalLightAngle_degs{ initialDirectionalLightAngle_degs };

    float directionalLightColorR{ initialDirectionalLightColor.r };
    float directionalLightColorG{ initialDirectionalLightColor.g };
    float directionalLightColorB{ initialDirectionalLightColor.b };
    float hazeBaseReference{ initialHazeBaseReference };

    bool isHazeActive{ false };   // Setting this to true will set haze to on
    bool isAltitudeBased{ false };
    bool isDirectionalLightAttenuationActive{ false };
    bool isModulateColorActive{ false };

    float hazeRange_m{ initialHazeRange_m };
    float hazeAltitude_m{ initialHazeAltitude_m };

    float hazeRangeKeyLight_m{ initialHazeRangeKeyLight_m };
    float hazeAltitudeKeyLight_m{ initialHazeAltitudeKeyLight_m };

    float backgroundBlendValue{ initialBackgroundBlendValue };

    // methods
    void setHazeColorR(const float value);
    void setHazeColorG(const float value);
    void setHazeColorB(const float value);
    void setDirectionalLightAngle_degs(const float value);

    void setDirectionalLightColorR(const float value);
    void setDirectionalLightColorG(const float value);
    void setDirectionalLightColorB(const float value);
    void setHazeBaseReference(const float value);

    void setIsHazeActive(const bool active);
    void setIsAltitudeBased(const bool active);
    void setIsdirectionalLightAttenuationActive(const bool active);
    void setIsModulateColorActive(const bool active);

    void setHazeRange_m(const float value);
    void setHazeAltitude_m(const float value);

    void setHazeRangeKeyLight_m(const float value);
    void setHazeAltitudeKeyLight_m(const float value);

    void setBackgroundBlendValue(const float value);
};

class DrawHaze {
public:
    using Inputs = render::VaryingSet5<model::HazePointer, gpu::FramebufferPointer, LinearDepthFramebufferPointer, DeferredFrameTransformPointer, gpu::FramebufferPointer>;
    using Config = HazeConfig;
    using JobModel = render::Job::ModelI<DrawHaze, Inputs, Config>;

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

private:
    gpu::PipelinePointer _hazePipeline;
};

#endif // hifi_render_utils_DrawHaze_h
