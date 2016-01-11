//
//  Created by Bradley Austin Davis on 2015/06/12
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <QtGlobal>

#include <openvr.h>

#include <display-plugins/WindowOpenGLDisplayPlugin.h>

const float TARGET_RATE_OpenVr = 90.0f;  // FIXME: get from sdk tracked device property? This number is vive-only.

class OpenVrDisplayPlugin : public WindowOpenGLDisplayPlugin {
public:
    virtual bool isSupported() const override;
    virtual const QString& getName() const override { return NAME; }
    virtual bool isHmd() const override { return true; }

    virtual float getTargetFrameRate() override { return TARGET_RATE_OpenVr; }

    virtual void activate() override;
    virtual void deactivate() override;

    virtual void customizeContext() override;

    virtual glm::uvec2 getRecommendedRenderSize() const override;
    virtual glm::uvec2 getRecommendedUiSize() const override { return uvec2(1920, 1080); }

    // Stereo specific methods
    virtual glm::mat4 getProjection(Eye eye, const glm::mat4& baseProjection) const override;
    virtual void resetSensors() override;

    virtual glm::mat4 getEyeToHeadTransform(Eye eye) const override;
    virtual glm::mat4 getHeadPose(uint32_t frameIndex) const override;
    virtual void submitSceneTexture(uint32_t frameIndex, uint32_t sceneTexture, const glm::uvec2& sceneSize) override;

protected:
    virtual void internalPresent() override;

private:
    vr::IVRSystem* _hmd { nullptr };
    static const QString NAME;
};

