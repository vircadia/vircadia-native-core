//
//  WindowDisplayPlugin.cpp
//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "WindowDisplayPlugin.h"
#include "RenderUtil.h"
#include "Application.h"

WindowDisplayPlugin::WindowDisplayPlugin() {
    connect(&_timer, &QTimer::timeout, this, [&] {
//        if (qApp->getActiveDisplayPlugin() == this) {
            emit requestRender();
//        }
    });
}

const QString WindowDisplayPlugin::NAME("QWindow 2D Renderer");

const QString & WindowDisplayPlugin::getName() {
    return NAME;
}

void WindowDisplayPlugin::activate() {
    GlWindowDisplayPlugin::activate();
    _window->show();
    _timer.start(8);
}

void WindowDisplayPlugin::deactivate() {
    _timer.stop();
    GlWindowDisplayPlugin::deactivate();
}

