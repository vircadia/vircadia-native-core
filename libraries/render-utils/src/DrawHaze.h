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

#include <model/Haze.h>

using LinearDepthFramebufferPointer = std::shared_ptr<LinearDepthFramebuffer>;

class MakeHazeConfig : public render::Job::Config {
    Q_OBJECT

    Q_PROPERTY(glm::vec3 hazeColor MEMBER hazeColor WRITE setHazeColor NOTIFY dirty);
    Q_PROPERTY(float hazeGlareAngle_degs MEMBER hazeGlareAngle_degs WRITE setHazeGlareAngle_degs NOTIFY dirty);

    Q_PROPERTY(glm::vec3 hazeGlareColor MEMBER hazeGlareColor WRITE setHazeGlareColor NOTIFY dirty);
    Q_PROPERTY(float hazeBaseReference_m MEMBER hazeBaseReference_m WRITE setHazeBaseReference NOTIFY dirty);

    Q_PROPERTY(bool isHazeActive MEMBER isHazeActive WRITE setHazeActive NOTIFY dirty);
    Q_PROPERTY(bool isAltitudeBased MEMBER isAltitudeBased WRITE setAltitudeBased NOTIFY dirty);
    Q_PROPERTY(bool isHazeAttenuateKeyLight MEMBER isHazeAttenuateKeyLight WRITE setHazeAttenuateKeyLight NOTIFY dirty);
    Q_PROPERTY(bool isModulateColorActive MEMBER isModulateColorActive WRITE setModulateColorActive NOTIFY dirty);
    Q_PROPERTY(bool isHazeEnableGlare MEMBER isHazeEnableGlare WRITE setHazeEnableGlare NOTIFY dirty);

    Q_PROPERTY(float hazeRange_m MEMBER hazeRange_m WRITE setHazeRange_m NOTIFY dirty);
    Q_PROPERTY(float hazeHeight_m MEMBER hazeHeight_m WRITE setHazeAltitude_m NOTIFY dirty);

    Q_PROPERTY(float hazeKeyLightRange_m MEMBER hazeKeyLightRange_m WRITE setHazeKeyLightRange_m NOTIFY dirty);
    Q_PROPERTY(float hazeKeyLightAltitude_m MEMBER hazeKeyLightAltitude_m WRITE setHazeKeyLightAltitude_m NOTIFY dirty);

    Q_PROPERTY(float hazeBackgroundBlend MEMBER hazeBackgroundBlend WRITE setHazeBackgroundBlend NOTIFY dirty);

public:
    MakeHazeConfig() : render::Job::Config() {}

    glm::vec3 hazeColor{ model::Haze::initialHazeColor };
    float hazeGlareAngle_degs{ model::Haze::initialGlareAngle_degs };

    glm::vec3 hazeGlareColor{ model::Haze::initialHazeGlareColor };
    float hazeBaseReference_m{ model::Haze::initialHazeBaseReference_m };

    bool isHazeActive{ false };
    bool isAltitudeBased{ false };
    bool isHazeAttenuateKeyLight{ false };
    bool isModulateColorActive{ false };
    bool isHazeEnableGlare{ false };

    float hazeRange_m{ model::Haze::initialHazeRange_m };
    float hazeHeight_m{ model::Haze::initialHazeHeight_m };

    float hazeKeyLightRange_m{ model::Haze::initialHazeKeyLightRange_m };
    float hazeKeyLightAltitude_m{ model::Haze::initialHazeKeyLightAltitude_m };

    float hazeBackgroundBlend{ model::Haze::initialHazeBackgroundBlend };

public slots:
    void setHazeColor(const glm::vec3 value) { hazeColor = value; emit dirty(); }
    void setHazeGlareAngle_degs(const float value) { hazeGlareAngle_degs = value; emit dirty(); }

    void setHazeGlareColor(const glm::vec3 value) { hazeGlareColor = value; emit dirty(); }
    void setHazeBaseReference(const float value) { hazeBaseReference_m = value; ; emit dirty(); }

    void setHazeActive(const bool active) { isHazeActive = active; emit dirty(); }
    void setAltitudeBased(const bool active) { isAltitudeBased = active; emit dirty(); }
    void setHazeAttenuateKeyLight(const bool active) { isHazeAttenuateKeyLight = active; emit dirty(); }
    void setModulateColorActive(const bool active) { isModulateColorActive = active; emit dirty(); }
    void setHazeEnableGlare(const bool active) { isHazeEnableGlare = active; emit dirty(); }

    void setHazeRange_m(const float value) { hazeRange_m = value; emit dirty(); }
    void setHazeAltitude_m(const float value) { hazeHeight_m = value; emit dirty(); }

    void setHazeKeyLightRange_m(const float value) { hazeKeyLightRange_m = value; emit dirty(); }
    void setHazeKeyLightAltitude_m(const float value) { hazeKeyLightAltitude_m = value; emit dirty(); }

    void setHazeBackgroundBlend(const float value) { hazeBackgroundBlend = value; ; emit dirty(); }

signals:
    void dirty();
};

class MakeHaze {
public:
    using Config = MakeHazeConfig;
    using JobModel = render::Job::ModelO<MakeHaze, model::HazePointer, Config>;

    MakeHaze();

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, model::HazePointer& haze);

private:
    model::HazePointer _haze;
};

class HazeConfig : public render::Job::Config {
public:
    HazeConfig() : render::Job::Config(true) {}

    // attributes
    glm::vec3 hazeColor{ model::Haze::initialHazeColor };
    float hazeGlareAngle_degs{ model::Haze::initialGlareAngle_degs };

    glm::vec3 hazeGlareColor{ model::Haze::initialHazeGlareColor };
    float hazeBaseReference_m{ model::Haze::initialHazeBaseReference_m };

    bool isHazeActive{ false };   // Setting this to true will set haze to on
    bool isAltitudeBased{ false };
    bool isHazeAttenuateKeyLight{ false };
    bool isModulateColorActive{ false };
    bool isHazeEnableGlare{ false };

    float hazeRange_m{ model::Haze::initialHazeRange_m };
    float hazeHeight_m{ model::Haze::initialHazeHeight_m };

    float hazeKeyLightRange_m{ model::Haze::initialHazeKeyLightRange_m };
    float hazeKeyLightAltitude_m{ model::Haze::initialHazeKeyLightAltitude_m };

    float hazeBackgroundBlend{ model::Haze::initialHazeBackgroundBlend };

    // methods
    void setHazeColor(const glm::vec3 value);
    void setHazeGlareAngle_degs(const float value);

    void setHazeGlareColor(const glm::vec3 value);
    void setHazeBaseReference(const float value);

    void setHazeActive(const bool active);
    void setAltitudeBased(const bool active);
    void setHazeAttenuateKeyLight(const bool active);
    void setModulateColorActive(const bool active);
    void setHazeEnableGlare(const bool active);

    void setHazeRange_m(const float value);
    void setHazeAltitude_m(const float value);

    void setHazeKeyLightRange_m(const float value);
    void setHazeKeyLightAltitude_m(const float value);

    void setHazeBackgroundBlend(const float value);
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
