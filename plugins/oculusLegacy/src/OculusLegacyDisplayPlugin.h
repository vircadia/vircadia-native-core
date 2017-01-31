//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <display-plugins/hmd/HmdDisplayPlugin.h>

#include <QTimer>

#include <OVR_CAPI.h>

const float TARGET_RATE_OculusLegacy = 75.0f;
class GLWindow;

class OculusLegacyDisplayPlugin : public HmdDisplayPlugin {
	using Parent = HmdDisplayPlugin;
public:
    OculusLegacyDisplayPlugin();
    bool isSupported() const override;
    const QString getName() const override { return NAME; }

    void init() override;

    int getHmdScreen() const override;

    // Stereo specific methods
    void resetSensors() override;
    bool beginFrameRender(uint32_t frameIndex) override;

    float getTargetFrameRate() const override;

protected:
    bool internalActivate() override;
    void internalDeactivate() override;

    void customizeContext() override;
    void uncustomizeContext() override;
    void hmdPresent() override;
    bool isHmdMounted() const override { return true; }

private:
    static const char* NAME;

    GLWindow* _hmdWindow{ nullptr };
    ovrHmd _hmd;
    mutable ovrTrackingState _trackingState;
    ovrEyeRenderDesc _eyeRenderDescs[2];
    ovrVector3f _ovrEyeOffsets[2];

    ovrFovPort _eyeFovs[2];
    ovrTexture _eyeTextures[2]; // FIXME - not currently in use
    mutable int _hmdScreen { -1 };
    bool _hswDismissed { false };
};


