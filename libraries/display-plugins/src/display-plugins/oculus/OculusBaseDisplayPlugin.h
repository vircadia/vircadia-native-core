//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "../MainWindowOpenGLDisplayPlugin.h"

class OculusBaseDisplayPlugin : public MainWindowOpenGLDisplayPlugin {
public:
    OculusBaseDisplayPlugin();
    // Stereo specific methods
    virtual bool isHmd() const override { return true; }
    virtual glm::mat4 getProjection(Eye eye, const glm::mat4& baseProjection) const override;
    virtual glm::mat4 getModelview(Eye eye, const glm::mat4& baseModelview) const override;
    virtual void activate() override;
    virtual void preRender() override;
    virtual glm::uvec2 getRecommendedRenderSize() const override;
    virtual glm::uvec2 getRecommendedUiSize() const override { return uvec2(1920, 1080); }
    virtual void resetSensors() override;
    virtual glm::mat4 getEyePose(Eye eye) const override;
    virtual glm::mat4 getHeadPose() const override;
protected:
    float _ipd;
};
