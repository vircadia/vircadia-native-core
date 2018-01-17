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
#include <graphics/Haze.h>

#include "SurfaceGeometryPass.h"

using LinearDepthFramebufferPointer = std::shared_ptr<LinearDepthFramebuffer>;

class MakeHazeConfig : public render::Job::Config {
    Q_OBJECT

    Q_PROPERTY(glm::vec3 hazeColor MEMBER hazeColor WRITE setHazeColor NOTIFY dirty);
    Q_PROPERTY(float hazeGlareAngle MEMBER hazeGlareAngle WRITE setHazeGlareAngle NOTIFY dirty);

    Q_PROPERTY(glm::vec3 hazeGlareColor MEMBER hazeGlareColor WRITE setHazeGlareColor NOTIFY dirty);
    Q_PROPERTY(float hazeBaseReference MEMBER hazeBaseReference WRITE setHazeBaseReference NOTIFY dirty);

    Q_PROPERTY(bool isHazeActive MEMBER isHazeActive WRITE setHazeActive NOTIFY dirty);
    Q_PROPERTY(bool isAltitudeBased MEMBER isAltitudeBased WRITE setAltitudeBased NOTIFY dirty);
    Q_PROPERTY(bool isHazeAttenuateKeyLight MEMBER isHazeAttenuateKeyLight WRITE setHazeAttenuateKeyLight NOTIFY dirty);
    Q_PROPERTY(bool isModulateColorActive MEMBER isModulateColorActive WRITE setModulateColorActive NOTIFY dirty);
    Q_PROPERTY(bool isHazeEnableGlare MEMBER isHazeEnableGlare WRITE setHazeEnableGlare NOTIFY dirty);

    Q_PROPERTY(float hazeRange MEMBER hazeRange WRITE setHazeRange NOTIFY dirty);
    Q_PROPERTY(float hazeHeight MEMBER hazeHeight WRITE setHazeAltitude NOTIFY dirty);

    Q_PROPERTY(float hazeKeyLightRange MEMBER hazeKeyLightRange WRITE setHazeKeyLightRange NOTIFY dirty);
    Q_PROPERTY(float hazeKeyLightAltitude MEMBER hazeKeyLightAltitude WRITE setHazeKeyLightAltitude NOTIFY dirty);

    Q_PROPERTY(float hazeBackgroundBlend MEMBER hazeBackgroundBlend WRITE setHazeBackgroundBlend NOTIFY dirty);

public:
    MakeHazeConfig() : render::Job::Config() {}

    glm::vec3 hazeColor{ graphics::Haze::INITIAL_HAZE_COLOR };
    float hazeGlareAngle{ graphics::Haze::INITIAL_HAZE_GLARE_ANGLE };

    glm::vec3 hazeGlareColor{ graphics::Haze::INITIAL_HAZE_GLARE_COLOR };
    float hazeBaseReference{ graphics::Haze::INITIAL_HAZE_BASE_REFERENCE };

    bool isHazeActive{ false };
    bool isAltitudeBased{ false };
    bool isHazeAttenuateKeyLight{ false };
    bool isModulateColorActive{ false };
    bool isHazeEnableGlare{ false };

    float hazeRange{ graphics::Haze::INITIAL_HAZE_RANGE };
    float hazeHeight{ graphics::Haze::INITIAL_HAZE_HEIGHT };

    float hazeKeyLightRange{ graphics::Haze::INITIAL_KEY_LIGHT_RANGE };
    float hazeKeyLightAltitude{ graphics::Haze::INITIAL_KEY_LIGHT_ALTITUDE };

    float hazeBackgroundBlend{ graphics::Haze::INITIAL_HAZE_BACKGROUND_BLEND };

public slots:
    void setHazeColor(const glm::vec3 value) { hazeColor = value; emit dirty(); }
    void setHazeGlareAngle(const float value) { hazeGlareAngle = value; emit dirty(); }

    void setHazeGlareColor(const glm::vec3 value) { hazeGlareColor = value; emit dirty(); }
    void setHazeBaseReference(const float value) { hazeBaseReference = value; ; emit dirty(); }

    void setHazeActive(const bool active) { isHazeActive = active; emit dirty(); }
    void setAltitudeBased(const bool active) { isAltitudeBased = active; emit dirty(); }
    void setHazeAttenuateKeyLight(const bool active) { isHazeAttenuateKeyLight = active; emit dirty(); }
    void setModulateColorActive(const bool active) { isModulateColorActive = active; emit dirty(); }
    void setHazeEnableGlare(const bool active) { isHazeEnableGlare = active; emit dirty(); }

    void setHazeRange(const float value) { hazeRange = value; emit dirty(); }
    void setHazeAltitude(const float value) { hazeHeight = value; emit dirty(); }

    void setHazeKeyLightRange(const float value) { hazeKeyLightRange = value; emit dirty(); }
    void setHazeKeyLightAltitude(const float value) { hazeKeyLightAltitude = value; emit dirty(); }

    void setHazeBackgroundBlend(const float value) { hazeBackgroundBlend = value; ; emit dirty(); }

signals:
    void dirty();
};

class MakeHaze {
public:
    using Config = MakeHazeConfig;
    using JobModel = render::Job::ModelO<MakeHaze, graphics::HazePointer, Config>;

    MakeHaze();

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, graphics::HazePointer& haze);

private:
    graphics::HazePointer _haze;
};

class HazeConfig : public render::Job::Config {
public:
    HazeConfig() : render::Job::Config(true) {}

    // attributes
    glm::vec3 hazeColor{ graphics::Haze::INITIAL_HAZE_COLOR };
    float hazeGlareAngle{ graphics::Haze::INITIAL_HAZE_GLARE_ANGLE };

    glm::vec3 hazeGlareColor{ graphics::Haze::INITIAL_HAZE_GLARE_COLOR };
    float hazeBaseReference{ graphics::Haze::INITIAL_HAZE_BASE_REFERENCE };

    bool isHazeActive{ false };   // Setting this to true will set haze to on
    bool isAltitudeBased{ false };
    bool isHazeAttenuateKeyLight{ false };
    bool isModulateColorActive{ false };
    bool isHazeEnableGlare{ false };

    float hazeRange{ graphics::Haze::INITIAL_HAZE_RANGE };
    float hazeHeight{ graphics::Haze::INITIAL_HAZE_HEIGHT };

    float hazeKeyLightRange{ graphics::Haze::INITIAL_KEY_LIGHT_RANGE };
    float hazeKeyLightAltitude{ graphics::Haze::INITIAL_KEY_LIGHT_ALTITUDE };

    float hazeBackgroundBlend{ graphics::Haze::INITIAL_HAZE_BACKGROUND_BLEND };

    // methods
    void setHazeColor(const glm::vec3 value);
    void setHazeGlareAngle(const float value);

    void setHazeGlareColor(const glm::vec3 value);
    void setHazeBaseReference(const float value);

    void setHazeActive(const bool active);
    void setAltitudeBased(const bool active);
    void setHazeAttenuateKeyLight(const bool active);
    void setModulateColorActive(const bool active);
    void setHazeEnableGlare(const bool active);

    void setHazeRange(const float value);
    void setHazeAltitude(const float value);

    void setHazeKeyLightRange(const float value);
    void setHazeKeyLightAltitude(const float value);

    void setHazeBackgroundBlend(const float value);
};

class DrawHaze {
public:
    using Inputs = render::VaryingSet5<graphics::HazePointer, gpu::FramebufferPointer, LinearDepthFramebufferPointer, DeferredFrameTransformPointer, gpu::FramebufferPointer>;
    using Config = HazeConfig;
    using JobModel = render::Job::ModelI<DrawHaze, Inputs, Config>;

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

private:
    gpu::PipelinePointer _hazePipeline;
};

#endif // hifi_render_utils_DrawHaze_h
