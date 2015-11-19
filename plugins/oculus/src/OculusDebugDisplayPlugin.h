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
    virtual const QString & getName() const override;
    virtual bool isSupported() const override;

protected:
    virtual void display(GLuint finalTexture, const glm::uvec2& sceneSize) override;
    virtual void customizeContext() override;
    // Do not perform swap in finish
    virtual void finishFrame() override;

private:
    static const QString NAME;
};

