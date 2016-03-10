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
    using Parent = OculusBaseDisplayPlugin;
public:
    void activate() override;
    const QString& getName() const override { return NAME; }

    float getTargetFrameRate() override { return TARGET_RATE_Oculus; }

protected:
    void hmdPresent() override;
    void customizeContext() override;
    void uncustomizeContext() override;
    void updateFrameData() override;

private:
    static const QString NAME;
    bool _enablePreview { false };
    bool _monoPreview { true };

    SwapFboPtr       _sceneFbo;
};

