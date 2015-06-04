//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "MainWindowOpenGLDisplayPlugin.h"

#include <GlWindow.h>
#include <QOpenGLContext>
#include <plugins/PluginContainer.h>
#include <QWidget>
#include <QMainWindow>

MainWindowOpenGLDisplayPlugin::MainWindowOpenGLDisplayPlugin() {
}

void MainWindowOpenGLDisplayPlugin::activate(PluginContainer * container) {
    WindowOpenGLDisplayPlugin::activate(container);
}

void MainWindowOpenGLDisplayPlugin::customizeWindow(PluginContainer * container) {
    // Can't set the central widget here, because it seems to mess up the context creation.
}

void MainWindowOpenGLDisplayPlugin::customizeContext(PluginContainer * container) {
    WindowOpenGLDisplayPlugin::customizeContext(container);
    QWidget* widget = QWidget::createWindowContainer(_window);
    auto mainWindow = container->getAppMainWindow();
    mainWindow->setCentralWidget(widget);
    mainWindow->resize(mainWindow->geometry().size());
    _window->resize(_window->geometry().size());
}

void MainWindowOpenGLDisplayPlugin::deactivate() {
    WindowOpenGLDisplayPlugin::deactivate();
}
