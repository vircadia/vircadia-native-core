//
//  GLCanvas.cpp
//  hifi
//
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include "svo-viewer-config.h"

#include "svoviewer.h"

#include "GLCanvas.h"
#include <QMimeData>
#include <QUrl>

GLCanvas::GLCanvas() : QGLWidget(QGLFormat(QGL::NoDepthBuffer, QGL::NoStencilBuffer)) {
}

void GLCanvas::initializeGL() {
    SvoViewer::getInstance()->initializeGL();
    setAttribute(Qt::WA_AcceptTouchEvents);
    setAcceptDrops(true);
}

void GLCanvas::paintGL() {
    SvoViewer::getInstance()->paintGL();
}

void GLCanvas::resizeGL(int width, int height) {
    SvoViewer::getInstance()->resizeGL(width, height);
}

void GLCanvas::keyPressEvent(QKeyEvent* event) {
    SvoViewer::getInstance()->keyPressEvent(event);
}

void GLCanvas::keyReleaseEvent(QKeyEvent* event) {
    SvoViewer::getInstance()->keyReleaseEvent(event);
}

void GLCanvas::mouseMoveEvent(QMouseEvent* event) {
    SvoViewer::getInstance()->mouseMoveEvent(event);
}

void GLCanvas::mousePressEvent(QMouseEvent* event) {
    SvoViewer::getInstance()->mousePressEvent(event);
}

void GLCanvas::mouseReleaseEvent(QMouseEvent* event) {
    SvoViewer::getInstance()->mouseReleaseEvent(event);
}

int updateTime = 0;
bool GLCanvas::event(QEvent* event) {
 /*   switch (event->type()) {
        case QEvent::TouchBegin:
            SvoViewer::getInstance()->touchBeginEvent(static_cast<QTouchEvent*>(event));
            event->accept();
            return true;
        case QEvent::TouchEnd:
            SvoViewer::getInstance()->touchEndEvent(static_cast<QTouchEvent*>(event));
            return true;
        case QEvent::TouchUpdate:
            SvoViewer::getInstance()->touchUpdateEvent(static_cast<QTouchEvent*>(event));
            return true;
        default:
            break;
    }*/
    return QGLWidget::event(event);
}

void GLCanvas::wheelEvent(QWheelEvent* event) {
    //SvoViewer::getInstance()->wheelEvent(event);
}

void GLCanvas::dragEnterEvent(QDragEnterEvent* event) {
    /*const QMimeData *mimeData = event->mimeData();
    foreach (QUrl url, mimeData->urls()) {
        if (url.url().toLower().endsWith(SNAPSHOT_EXTENSION)) {
            event->acceptProposedAction();
            break;
        }
    }*/
}

void GLCanvas::dropEvent(QDropEvent* event) {
    //SvoViewer::getInstance()->dropEvent(event);
}
