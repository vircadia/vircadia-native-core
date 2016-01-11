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
    virtual const QString& getName() const override { return NAME; }
    virtual grouping getGrouping() const override { return DEVELOPER; }
    virtual bool isSupported() const override;

protected:
    virtual void customizeContext() override;

private:
    static const QString NAME;
};

