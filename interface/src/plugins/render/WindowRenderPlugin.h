//
//  WindowRenderPlugin.h
//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "SimpleRenderPlugin.h"

#include <QWindow>
#include <QOpenGLContext>
#include <QTimer>

class WindowRenderPlugin : public SimpleRenderPlugin<QWindow> {
    Q_OBJECT
public:
    static const QString NAME;

    WindowRenderPlugin();

    virtual const QString & getName();

    virtual void activate();
    virtual void deactivate();
    virtual bool eventFilter(QObject* object, QEvent* event);
    virtual QSize getRecommendedFramebufferSize() const;

protected:
    virtual void makeCurrent();
    virtual void doneCurrent();
    virtual void swapBuffers();

private:
    QTimer _timer;
    QOpenGLContext * _context{ nullptr };
};
