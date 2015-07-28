
//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "MainWindowOpenGLDisplayPlugin.h"

#include <QOpenGLContext>
#include <QMainWindow>

#include <GlWindow.h>
#include <plugins/PluginContainer.h>

MainWindowOpenGLDisplayPlugin::MainWindowOpenGLDisplayPlugin() {
}

GlWindow* MainWindowOpenGLDisplayPlugin::createWindow(PluginContainer * container) {
    return container->getVisibleWindow();
}

void MainWindowOpenGLDisplayPlugin::customizeWindow(PluginContainer * container) {
}

void MainWindowOpenGLDisplayPlugin::destroyWindow() {
    _window = nullptr;
}
