//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "OpenGlDisplayPlugin.h"

class WindowDisplayPlugin : public OpenGlDisplayPlugin {
    Q_OBJECT
public:
    static const QString NAME;

    WindowDisplayPlugin();

    virtual const QString & getName();

    void display(GLuint sceneTexture, const glm::uvec2& sceneSize,
        GLuint overlayTexture, const glm::uvec2& overlaySize);

protected:
    // Called by the activate method to specify the initial 
    // window geometry flags, etc
    virtual void customizeWindow(PluginContainer * container);
};
