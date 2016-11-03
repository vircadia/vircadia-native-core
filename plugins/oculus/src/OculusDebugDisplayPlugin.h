//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "OculusBaseDisplayPlugin.h"

class OculusDebugDisplayPlugin : public OculusBaseDisplayPlugin {
public:
    const QString& getName() const override { return NAME; }
    grouping getGrouping() const override { return DEVELOPER; }
    bool isSupported() const override;

protected:
    void hmdPresent() override {}
    bool isHmdMounted() const override { return true; }

private:
    static const QString NAME;
};

