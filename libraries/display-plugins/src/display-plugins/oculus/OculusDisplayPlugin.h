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

class OculusDisplayPlugin : public OculusBaseDisplayPlugin {
public:
    virtual void deactivate() override;
    virtual const QString & getName() const override;

protected:
    virtual void display(GLuint finalTexture, const glm::uvec2& sceneSize) override;
    virtual void customizeContext() override;
    // Do not perform swap in finish
    virtual void finishFrame() override;

private:
    static const QString NAME;
    bool _enableMirror{ false };

#if (OVR_MAJOR_VERSION >= 6)
    SwapFboPtr       _sceneFbo;
#endif
};

