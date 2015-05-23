//
//  SimpleDisplayPlugin.cpp
//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "SimpleDisplayPlugin.h"
#include <QOpenGLContext>
#include <QCursor>
#include <QCoreApplication>

#include <RenderUtil.h>
#include "DependencyManager.h"
#include "TextureCache.h"
#include "gpu/GLBackend.h"
#include "OffscreenUi.h"

void SimpleGlDisplayPlugin::activate() {
    makeCurrent();
    doneCurrent();
}

void SimpleGlDisplayPlugin::display(GLuint sceneTexture, const glm::uvec2& sceneSize,
                                    GLuint overlayTexture, const glm::uvec2& overlaySize) {
    makeCurrent();
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glClearColor(0, 0, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    
    glViewport(0, 0, getDeviceSize().width(), getDeviceSize().height());
    if (sceneTexture) {
        glBindTexture(GL_TEXTURE_2D, sceneTexture);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0);
        glVertex2f(-1, -1);
        glTexCoord2f(1, 0);
        glVertex2f(+1, -1);
        glTexCoord2f(1, 1);
        glVertex2f(+1, +1);
        glTexCoord2f(0, 1);
        glVertex2f(-1, +1);
        glEnd();
    }

    if (overlayTexture) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindTexture(GL_TEXTURE_2D, overlayTexture);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0);
        glVertex2f(-1, -1);
        glTexCoord2f(1, 0);
        glVertex2f(+1, -1);
        glTexCoord2f(1, 1);
        glVertex2f(+1, +1);
        glTexCoord2f(0, 1);
        glVertex2f(-1, +1);
        glEnd();
    }

    
    glDisable(GL_BLEND);

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    //Q_ASSERT(!glGetError());
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    glFinish();
}

bool GlWindowDisplayPlugin::eventFilter(QObject* object, QEvent* event) {
    if (qApp->eventFilter(object, event)) {
        return true;
    }

    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    if (offscreenUi->eventFilter(object, event)) {
        return true;
    }

    switch (event->type()) {
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
            QCoreApplication::sendEvent(QCoreApplication::instance(), event);
            break;

        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        case QEvent::Resize:
        case QEvent::MouseMove:
            QCoreApplication::sendEvent(QCoreApplication::instance(), event);
            break;
        default:
            break;
    }
    return false;
}


static QSurfaceFormat getPluginFormat() {
    QSurfaceFormat format;
    // Qt Quick may need a depth and stencil buffer. Always make sure these are available.
    format.setDepthBufferSize(16);
    format.setStencilBufferSize(8);
    format.setVersion(4, 1);
#ifdef DEBUG
    format.setOption(QSurfaceFormat::DebugContext);
#endif
    format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
    return format;
}


void GlWindowDisplayPlugin::activate() {
    Q_ASSERT(nullptr == _window);
    _window = new GlWindow(getPluginFormat(), QOpenGLContext::currentContext());
    _window->installEventFilter(this);
    DependencyManager::get<OffscreenUi>()->setProxyWindow(_window);
}

void GlWindowDisplayPlugin::deactivate() {
    Q_ASSERT(nullptr != _window);
    _window->hide();
    _window->destroy();
    _window->deleteLater();
    _window = nullptr;
}

QSize GlWindowDisplayPlugin::getDeviceSize() const {
    return _window->geometry().size() * _window->devicePixelRatio();
}

glm::ivec2 GlWindowDisplayPlugin::getCanvasSize() const {
    return toGlm(_window->geometry().size());
}

bool GlWindowDisplayPlugin::hasFocus() const {
    return _window->isActive();
}
