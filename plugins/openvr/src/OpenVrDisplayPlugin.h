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

#include <display-plugins/hmd/HmdDisplayPlugin.h>

const float TARGET_RATE_OpenVr = 90.0f;  // FIXME: get from sdk tracked device property? This number is vive-only.

class OpenVrDisplayPlugin : public HmdDisplayPlugin {
    using Parent = HmdDisplayPlugin;
public:
    virtual bool isSupported() const override;
    virtual const QString& getName() const override { return NAME; }

    virtual float getTargetFrameRate() const override { return TARGET_RATE_OpenVr; }

    virtual void customizeContext() override;

    // Stereo specific methods
    virtual void resetSensors() override;
    virtual void beginFrameRender(uint32_t frameIndex) override;

protected:
    bool internalActivate() override;
    void internalDeactivate() override;

    void hmdPresent() override;
    bool isHmdMounted() const override;
    void postPreview() override;

private:
    vr::IVRSystem* _system { nullptr };
    std::atomic<vr::EDeviceActivityLevel> _hmdActivityLevel { vr::k_EDeviceActivityLevel_Unknown };
    static const QString NAME;
    mutable Mutex _poseMutex;
};
