//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "WindowOpenGLDisplayPlugin.h"

class MainWindowOpenGLDisplayPlugin : public WindowOpenGLDisplayPlugin {
    Q_OBJECT
public:
    MainWindowOpenGLDisplayPlugin();
    virtual void display(GLuint sceneTexture, const glm::uvec2& sceneSize) override;

protected:
    virtual GlWindow* createWindow(PluginContainer * container) override final;
    virtual void customizeWindow(PluginContainer * container) override final;
    virtual void destroyWindow() override final;
};
