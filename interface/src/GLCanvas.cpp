//
//  GLCanvas.cpp
//  hifi
//
//  Created by Stephen Birarda on 8/14/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "Application.h"

#include "GLCanvas.h"
#include <QMimeData>
#include <QUrl>
#include <QMainWindow>

GLCanvas::GLCanvas() : QGLWidget(QGLFormat(QGL::NoDepthBuffer, QGL::NoStencilBuffer)), _throttleRendering(false), _idleRenderInterval(100) {
}

void GLCanvas::initializeGL() {
    Application::getInstance()->initializeGL();
    setAttribute(Qt::WA_AcceptTouchEvents);
    setAcceptDrops(true);
    connect(Application::getInstance(), &Application::applicationStateChanged, this, &GLCanvas::activeChanged);
    connect(&_frameTimer, &QTimer::timeout, this, &GLCanvas::throttleRender);
}

void GLCanvas::paintGL() {
    if (!_throttleRendering && !Application::getInstance()->getWindow()->isMinimized()) {
        Application::getInstance()->paintGL();
    }
}

void GLCanvas::resizeGL(int width, int height) {
    Application::getInstance()->resizeGL(width, height);
}

void GLCanvas::keyPressEvent(QKeyEvent* event) {
    Application::getInstance()->keyPressEvent(event);
}

void GLCanvas::keyReleaseEvent(QKeyEvent* event) {
    Application::getInstance()->keyReleaseEvent(event);
}

void GLCanvas::mouseMoveEvent(QMouseEvent* event) {
    Application::getInstance()->mouseMoveEvent(event);
}

void GLCanvas::mousePressEvent(QMouseEvent* event) {
    Application::getInstance()->mousePressEvent(event);
}

void GLCanvas::mouseReleaseEvent(QMouseEvent* event) {
    Application::getInstance()->mouseReleaseEvent(event);
}

void GLCanvas::activeChanged() {
    switch (Application::applicationState()) {
        case Qt::ApplicationActive:
            // If we're active, stop the frame timer and the throttle.
            _frameTimer.stop();
            _throttleRendering = false;
            break;
            
        case Qt::ApplicationSuspended:
        case Qt::ApplicationHidden:
            // If we're hidden or are about to suspend, don't render anything.
            _throttleRendering = false;
            _frameTimer.stop();
            break;
            
        default:
            // Otherwise, throttle.
            if (!_throttleRendering) {
                _frameTimer.start(_idleRenderInterval);
                _throttleRendering = true;
            }
            break;
    }
}

void GLCanvas::throttleRender() {
    _frameTimer.start(_idleRenderInterval);
    if (!Application::getInstance()->getWindow()->isMinimized()) {
        Application::getInstance()->paintGL();
    }
}

int updateTime = 0;
bool GLCanvas::event(QEvent* event) {
    switch (event->type()) {
        case QEvent::TouchBegin:
            Application::getInstance()->touchBeginEvent(static_cast<QTouchEvent*>(event));
            event->accept();
            return true;
        case QEvent::TouchEnd:
            Application::getInstance()->touchEndEvent(static_cast<QTouchEvent*>(event));
            return true;
        case QEvent::TouchUpdate:
            Application::getInstance()->touchUpdateEvent(static_cast<QTouchEvent*>(event));
            return true;
        default:
            break;
    }
    return QGLWidget::event(event);
}

void GLCanvas::wheelEvent(QWheelEvent* event) {
    Application::getInstance()->wheelEvent(event);
}

void GLCanvas::dragEnterEvent(QDragEnterEvent* event) {
    const QMimeData *mimeData = event->mimeData();
    foreach (QUrl url, mimeData->urls()) {
        if (url.url().toLower().endsWith(SNAPSHOT_EXTENSION)) {
            event->acceptProposedAction();
            break;
        }
    }
}

void GLCanvas::dropEvent(QDropEvent* event) {
    Application::getInstance()->dropEvent(event);
}
