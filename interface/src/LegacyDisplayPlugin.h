//
//  LegacyDisplayPlugin.h
//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <display-plugins/OpenGlDisplayPlugin.h>
#include "GLCanvas.h"

class LegacyDisplayPlugin : public OpenGlDisplayPlugin {
    Q_OBJECT
public:
    LegacyDisplayPlugin();
    static const QString NAME;
    virtual const QString & getName();

    virtual void activate(PluginContainer * container);
    virtual void deactivate();

    virtual QSize getDeviceSize() const;
    virtual ivec2 getCanvasSize() const;
    virtual bool hasFocus() const;
    virtual PickRay computePickRay(const glm::vec2 & pos) const;
    virtual bool isMouseOnScreen() const { return true; }
    virtual bool isThrottled() const;
    virtual void preDisplay();
    virtual void idle();
    virtual ivec2 getTrueMousePosition() const;

    virtual QWindow* getWindow() const;
    virtual void installEventFilter(QObject* filter);
    virtual void removeEventFilter(QObject* filter);

protected:
    virtual void makeCurrent() final;
    virtual void doneCurrent() final;
    virtual void swapBuffers() final;

private:
    GLCanvas * _window;
    QTimer _timer;
};
