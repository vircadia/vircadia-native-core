//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "MainWindowOpenGLDisplayPlugin.h"

#include <QOpenGLContext>
#include <QWidget>
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

void MainWindowOpenGLDisplayPlugin::display(
    GLuint finalTexture, const glm::uvec2& sceneSize) {
    OpenGLDisplayPlugin::display(finalTexture, sceneSize);
    return;

    using namespace oglplus;
    glClearColor(0, 1, 0, 1);
    uvec2 size = toGlm(_window->size());
    Context::Viewport(size.x, size.y);
    Context::Clear().ColorBuffer();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, finalTexture);
    glBegin(GL_QUADS);
        glTexCoord2f(0, 0);
        glVertex2f(-1, -1);
        glTexCoord2f(1, 0);
        glVertex2f(1, -1);
        glTexCoord2f(1, 1);
        glVertex2f(1, 1);
        glTexCoord2f(0, 1);
        glVertex2f(-1, 1);
    glEnd();
}
