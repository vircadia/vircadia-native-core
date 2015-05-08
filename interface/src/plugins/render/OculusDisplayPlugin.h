//
//  OculusBaseDisplayPlugin.h
//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "HmdDisplayPlugin.h"
#include <functional>

class OculusDisplayPlugin : public HmdDisplayPlugin {
public:
    virtual bool isSupported();

    virtual void init();
    virtual void deinit();

    virtual void activate();
    virtual void deactivate();

    virtual void overrideOffAxisFrustum(
        float& left, float& right, float& bottom, float& top,
        float& nearVal, float& farVal,
        glm::vec4& nearClipPlane, glm::vec4& farClipPlane) const;

};
