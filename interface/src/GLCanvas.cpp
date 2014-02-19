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

GLCanvas::GLCanvas() : QGLWidget(QGLFormat(QGL::NoDepthBuffer, QGL::NoStencilBuffer)) {
}

void GLCanvas::initializeGL() {
    Application::getInstance()->initializeGL();
    setAttribute(Qt::WA_AcceptTouchEvents);
    setAcceptDrops(true);
}

void GLCanvas::paintGL() {
    Application::getInstance()->paintGL();
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
