//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <display-plugins/WindowOpenGLDisplayPlugin.h>

#include <QTimer>

#include <OVR_CAPI.h>

const float TARGET_RATE_OculusLegacy = 75.0f;

class OculusLegacyDisplayPlugin : public WindowOpenGLDisplayPlugin {
public:
    OculusLegacyDisplayPlugin();
    virtual bool isSupported() const override;
    virtual const QString& getName() const override { return NAME; }

    virtual void activate() override;
    virtual void deactivate() override;

    virtual int getHmdScreen() const override;

    // Stereo specific methods
    virtual bool isHmd() const override { return true; }
    virtual glm::mat4 getProjection(Eye eye, const glm::mat4& baseProjection) const override;
    virtual glm::uvec2 getRecommendedRenderSize() const override;
    virtual glm::uvec2 getRecommendedUiSize() const override { return uvec2(1920, 1080); }
    virtual void resetSensors() override;
    virtual glm::mat4 getEyeToHeadTransform(Eye eye) const override;
    virtual glm::mat4 getHeadPose(uint32_t frameIndex) const override;

    virtual float getTargetFrameRate() override;

protected:
    virtual void customizeContext() override;
#if 0
    virtual void uncustomizeContext() override;
    virtual void internalPresent() override;
#endif
    
private:
    static const QString NAME;

    ovrHmd _hmd;
    mutable ovrTrackingState _trackingState;
    ovrEyeRenderDesc _eyeRenderDescs[2];
    mutable ovrPosef _eyePoses[2];
    ovrVector3f _eyeOffsets[2];
    ovrFovPort _eyeFovs[2];
    mat4 _eyeProjections[3];
    mat4 _compositeEyeProjections[2];
    uvec2 _desiredFramebufferSize;
    //ovrTexture _eyeTextures[2]; // FIXME - not currently in use
    mutable int _hmdScreen { -1 };
    bool _hswDismissed { false };
};


