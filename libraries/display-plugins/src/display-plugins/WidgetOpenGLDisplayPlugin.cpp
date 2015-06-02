//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "WidgetOpenGLDisplayPlugin.h"

#include <QMainWindow>

#include <GlWindow.h>

#include <QOpenGLContext>
#include <QOpenGLDebugLogger>
#include <QWidget>
#include <QGLContext>
#include <QGLWidget>
#include "plugins/PluginContainer.h"

WidgetOpenGLDisplayPlugin::WidgetOpenGLDisplayPlugin() {
}

const QString WidgetOpenGLDisplayPlugin::NAME("QWindow 2D Renderer");

const QString & WidgetOpenGLDisplayPlugin::getName() {
    return NAME;
}

class CustomOpenGLWidget : public QGLWidget {
public:
    explicit CustomOpenGLWidget(const QGLFormat& format, QWidget* parent = 0,
        const QGLWidget* shareWidget = 0, Qt::WindowFlags f = 0) : QGLWidget(format, parent, shareWidget, f) {
        setAutoBufferSwap(false);
    }
protected:

    void paintGL() { }
    void resizeGL() {}
    void initializeGL() {}
};

void WidgetOpenGLDisplayPlugin::activate(PluginContainer * container) {
    OpenGLDisplayPlugin::activate(container);

    Q_ASSERT(nullptr == _widget);
    _widget = new CustomOpenGLWidget(QGL::NoDepthBuffer | QGL::NoStencilBuffer);
    QOpenGLContext * sourceContext = QOpenGLContext::currentContext();
    QOpenGLContext * newContext = new QOpenGLContext();
    _widget->setContext(
        QGLContext::fromOpenGLContext(newContext),
        QGLContext::fromOpenGLContext(sourceContext));

    QMainWindow* mainWindow = container->getAppMainWindow();
    mainWindow->setCentralWidget(_widget);

    makeCurrent();
    customizeContext(container);

    _widget->installEventFilter(this);
}

void WidgetOpenGLDisplayPlugin::deactivate() {
    OpenGLDisplayPlugin::deactivate();
    _widget->deleteLater();
    _widget = nullptr;
}

void WidgetOpenGLDisplayPlugin::makeCurrent() {
    _widget->makeCurrent();

}

void WidgetOpenGLDisplayPlugin::doneCurrent() {
    _widget->doneCurrent();
}

void WidgetOpenGLDisplayPlugin::swapBuffers() {
    _widget->swapBuffers();
}

glm::ivec2 WidgetOpenGLDisplayPlugin::getTrueMousePosition() const {
    return toGlm(_widget->mapFromGlobal(QCursor::pos()));
}

QSize WidgetOpenGLDisplayPlugin::getDeviceSize() const {
    return _widget->geometry().size() * _widget->devicePixelRatio();
}

glm::ivec2 WidgetOpenGLDisplayPlugin::getCanvasSize() const {
    return toGlm(_widget->geometry().size());
}

bool WidgetOpenGLDisplayPlugin::hasFocus() const {
    return _widget->hasFocus();
}

void WidgetOpenGLDisplayPlugin::installEventFilter(QObject* filter) {
    _widget->installEventFilter(filter);
}

void WidgetOpenGLDisplayPlugin::removeEventFilter(QObject* filter) {
    _widget->removeEventFilter(filter);
}

QWindow* WidgetOpenGLDisplayPlugin::getWindow() const {
    return _widget->windowHandle();
}