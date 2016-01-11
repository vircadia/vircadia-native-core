//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "OculusBaseDisplayPlugin.h"

struct SwapFramebufferWrapper;
using SwapFboPtr = QSharedPointer<SwapFramebufferWrapper>;

const float TARGET_RATE_Oculus = 75.0f;

class OculusDisplayPlugin : public OculusBaseDisplayPlugin {
public:
    virtual void activate() override;
    virtual const QString& getName() const override { return NAME; }
    virtual void setEyeRenderPose(uint32_t frameIndex, Eye eye, const glm::mat4& pose) override final;

    virtual float getTargetFrameRate() override { return TARGET_RATE_Oculus; }

protected:
    virtual void internalPresent() override;
    virtual void customizeContext() override;
    virtual void uncustomizeContext() override;

private:
    using EyePoses = std::pair<ovrPosef, ovrPosef>;
    static const QString NAME;
    bool _enablePreview { false };
    bool _monoPreview { true };
    QMap<uint32_t, EyePoses> _frameEyePoses;

    SwapFboPtr       _sceneFbo;
};

