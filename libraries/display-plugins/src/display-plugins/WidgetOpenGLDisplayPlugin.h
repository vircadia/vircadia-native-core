//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "OpenGLDisplayPlugin.h"

class QGLWidget;
class QOpenGLDebugLogger;
class WidgetOpenGLDisplayPlugin : public OpenGLDisplayPlugin {
    Q_OBJECT
public:
    static const QString NAME;

    WidgetOpenGLDisplayPlugin();

    virtual const QString & getName() override;
    virtual glm::ivec2 getTrueMousePosition() const override;
    virtual QSize getDeviceSize() const override;
    virtual glm::ivec2 getCanvasSize() const override;
    virtual bool hasFocus() const override;
    virtual QWindow* getWindow() const override;

    virtual void activate(PluginContainer * container) override;
    virtual void deactivate() override;

    virtual void installEventFilter(QObject* filter) override;
    virtual void removeEventFilter(QObject* filter) override;

protected:
    virtual void makeCurrent() override;
    virtual void doneCurrent() override;
    virtual void swapBuffers() override;

    QGLWidget* _widget{ nullptr };
};
