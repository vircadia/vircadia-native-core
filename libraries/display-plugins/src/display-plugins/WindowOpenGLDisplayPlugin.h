//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "OpenGLDisplayPlugin.h"

class QWidget;

class WindowOpenGLDisplayPlugin : public OpenGLDisplayPlugin {
public:
    virtual bool hasFocus() const override;
    virtual void activate() override;
    virtual void deactivate() override;

protected:
    virtual glm::uvec2 getSurfaceSize() const override final;
    virtual glm::uvec2 getSurfacePixels() const override final;
    
    QWidget* _window { nullptr };
};
