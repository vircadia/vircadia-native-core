//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "DisplayPlugin.h"

class NullDisplayPlugin : public DisplayPlugin {
public:
    static const QString NAME;

    virtual ~NullDisplayPlugin() final {}
    virtual const QString & getName();

    void activate(PluginContainer * container);
    void deactivate();

    virtual QSize getDeviceSize() const;
    virtual glm::ivec2 getCanvasSize() const;
    virtual bool hasFocus() const;
    virtual glm::ivec2 getRelativeMousePosition() const;
    virtual glm::ivec2 getTrueMousePosition() const;
    virtual PickRay computePickRay(const glm::vec2 & pos) const;
    virtual bool isMouseOnScreen() const;
    virtual QWindow* getWindow() const;
    virtual void preRender();
    virtual void preDisplay();
    virtual void display(GLuint sceneTexture, const glm::uvec2& sceneSize);
    virtual void finishFrame();
};
