//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "OculusBaseDisplayPlugin.h"
#include <QTimer>

class OffscreenGlCanvas;
class OculusWin32DisplayPlugin : public OculusBaseDisplayPlugin {
public:
    virtual bool isSupported();
    virtual const QString & getName();

    virtual void activate(PluginContainer * container);
    virtual void deactivate();
//    virtual QSize getDeviceSize() const final;
//    virtual glm::ivec2 getCanvasSize() const final;

    void display(GLuint sceneTexture, const glm::uvec2& sceneSize,
        GLuint overlayTexture, const glm::uvec2& overlaySize);

protected:
    virtual void customizeWindow(PluginContainer * container);
    virtual void customizeContext(PluginContainer * container);

private:
    static const QString NAME;
};
