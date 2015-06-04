//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "../MainWindowOpenGLDisplayPlugin.h"

#include <OVR_CAPI.h>

class OculusBaseDisplayPlugin : public MainWindowOpenGLDisplayPlugin {
public:
    // Stereo specific methods
    virtual bool isHmd() const override { return true; }
    virtual glm::mat4 getProjection(Eye eye, const glm::mat4& baseProjection) const override;
    virtual glm::mat4 getModelview(Eye eye, const glm::mat4& baseModelview) const override;
    virtual void activate(PluginContainer * container) override;
    virtual void preRender() override;
    virtual QSize getRecommendedFramebufferSize() const override;
    virtual void resetSensors() override;
    virtual glm::ivec2 getCanvasSize() const override { return ivec2(1920, 1080); }
    virtual glm::mat4 getEyePose(Eye eye) const override;
    virtual glm::mat4 getHeadPose() const override;

protected:
    ovrHmd _hmd;
    unsigned int _frameIndex{ 0 };

    ovrEyeRenderDesc _eyeRenderDescs[2];
    ovrPosef _eyePoses[2];
    ovrVector3f _eyeOffsets[2];
    mat4 _eyeProjections[2];
    mat4 _compositeEyeProjections[2];
    QSize _desiredFramebufferSize;
};
