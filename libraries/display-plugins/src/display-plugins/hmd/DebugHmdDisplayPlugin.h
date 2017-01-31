//
//  Created by Bradley Austin Davis on 2016/07/31
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "HmdDisplayPlugin.h"

class DebugHmdDisplayPlugin : public HmdDisplayPlugin {
    using Parent = HmdDisplayPlugin;

public:
    const QString getName() const override { return NAME; }
    grouping getGrouping() const override { return DEVELOPER; }

    bool isSupported() const override;
    void resetSensors() override final;
    bool beginFrameRender(uint32_t frameIndex) override;
    float getTargetFrameRate() const override { return 90; }


protected:
    void updatePresentPose() override;
    void hmdPresent() override {}
    bool isHmdMounted() const override { return true; }
    void customizeContext() override;
    bool internalActivate() override;
private:
    static const QString NAME;
};
