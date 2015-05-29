//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "DisplayPlugin.h"

#include <OVR_CAPI.h>

class OculusBaseDisplayPlugin : public DisplayPlugin {
public:
    // Stereo specific methods
    virtual bool isHmd() const { return true; }
    virtual glm::mat4 getProjection(Eye eye, const glm::mat4& baseProjection) const;
    virtual glm::mat4 getModelview(Eye eye, const glm::mat4& baseModelview) const;
    virtual void activate(PluginContainer * container);
    virtual void deactivate();
    virtual void preRender();
    virtual void configureRendering() = 0;
protected:
    ovrHmd _hmd;
    unsigned int _frameIndex{ 0 };

    ovrPosef _eyePoses[2];
    ovrVector3f _eyeOffsets[2];
    glm::mat4 _eyeProjections[2];
};
