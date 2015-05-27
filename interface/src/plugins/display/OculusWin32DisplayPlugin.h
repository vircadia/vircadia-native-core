//
//  Created by Bradley Austin Davis on 2015/05/26.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "OculusBaseDisplayPlugin.h"

class OculusWin32DisplayPlugin : public OculusBaseDisplayPlugin {
public:
    virtual bool isSupported();
};
