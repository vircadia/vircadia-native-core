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

#include <OVR_CAPI_GL.h>

class OculusBaseDisplayPlugin : public HmdDisplayPlugin {
public:
    virtual bool isSupported() const override;

    virtual void init() override final;
    virtual void deinit() override final;

    virtual void activate() override;
    virtual void deactivate() override;

    // Stereo specific methods
    virtual void resetSensors() override final;
    virtual glm::mat4 getHeadPose(uint32_t frameIndex) const override;

protected:
    virtual void customizeContext() override;

protected:
    ovrSession _session;
    ovrGraphicsLuid _luid;
    float _ipd{ OVR_DEFAULT_IPD };
    ovrEyeRenderDesc _eyeRenderDescs[2];
    ovrFovPort _eyeFovs[2];
    ovrHmdDesc _hmdDesc;
    ovrLayerEyeFov _sceneLayer;
    ovrViewScaleDesc _viewScaleDesc;
};
