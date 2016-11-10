//
//
//  Created by Bradley Austin Davis on 2015/12/03
//  Derived from interface/src/GLCanvas.cpp created by Stephen Birarda on 8/14/13.
//  Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLWidget.h"

#include "Config.h"

#include <mutex>

#include <QtGlobal>
#include <QtCore/QMimeData>
#include <QtCore/QUrl>
#include <QtCore/QCoreApplication>

#include <QtGui/QKeyEvent>
#include <QtGui/QPaintEngine>
#include <QtGui/QWindow>

#include "Context.h"
#include "GLHelpers.h"

class GLPaintEngine : public QPaintEngine {
    bool begin(QPaintDevice *pdev) override { return true; }
    bool end() override { return true; } 
    void updateState(const QPaintEngineState &state) override { }
    void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr) override { }
    Type type() const override { return OpenGL2; }
};

GLWidget::GLWidget() {
#ifdef Q_OS_LINUX
    // Cause GLWidget::eventFilter to be called.
    // It wouldn't hurt to do this on Mac and PC too; but apparently it's only needed on linux.
    qApp->installEventFilter(this);
#endif
    setAttribute(Qt::WA_AcceptTouchEvents);
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
    grabGesture(Qt::PinchGesture);
    setAcceptDrops(true);
    _paintEngine = new GLPaintEngine();
}

GLWidget::~GLWidget() {
    delete _paintEngine;
    _paintEngine = nullptr;
}

int GLWidget::getDeviceWidth() const {
    return width() * (windowHandle() ? (float)windowHandle()->devicePixelRatio() : 1.0f);
}

int GLWidget::getDeviceHeight() const {
    return height() * (windowHandle() ? (float)windowHandle()->devicePixelRatio() : 1.0f);
}

void GLWidget::createContext() {
    _context = new gl::Context();
    _context->setWindow(windowHandle());
    _context->create();
    _context->makeCurrent();
    _context->clear();
}

bool GLWidget::makeCurrent() {
    gl::Context::makeCurrent(_context->qglContext(), windowHandle());
    return _context->makeCurrent();
}

QOpenGLContext* GLWidget::qglContext() {
    return _context->qglContext();
}

void GLWidget::doneCurrent() {
    _context->doneCurrent();
}

bool GLWidget::event(QEvent* event) {
    switch (event->type()) {
        case QEvent::MouseMove:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        case QEvent::Resize:
        case QEvent::TouchBegin:
        case QEvent::TouchEnd:
        case QEvent::TouchUpdate:
        case QEvent::Gesture:
        case QEvent::Wheel:
        case QEvent::DragEnter:
        case QEvent::Drop:
            if (QCoreApplication::sendEvent(QCoreApplication::instance(), event)) {
                return true;
            }
            break;

        default:
            break;
    }
    return QWidget::event(event);
}

// Pressing Alt (and Meta) key alone activates the menubar because its style inherits the
// SHMenuBarAltKeyNavigation from QWindowsStyle. This makes it impossible for a scripts to
// receive keyPress events for the Alt (and Meta) key in a reliable manner.
//
// This filter catches events before QMenuBar can steal the keyboard focus.
// The idea was borrowed from
// http://www.archivum.info/qt-interest@trolltech.com/2006-09/00053/Re-(Qt4)-Alt-key-focus-QMenuBar-(solved).html

bool GLWidget::eventFilter(QObject*, QEvent* event) {
    switch (event->type()) {
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::ShortcutOverride:
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Alt || keyEvent->key() == Qt::Key_Meta) {
                if (event->type() == QEvent::KeyPress) {
                    keyPressEvent(keyEvent);
                } else if (event->type() == QEvent::KeyRelease) {
                    keyReleaseEvent(keyEvent);
                } else {
                    QWidget::event(event);
                }
                return true;
            }
        }
        default:
            break;
    }
    return false;
}


bool GLWidget::nativeEvent(const QByteArray &eventType, void *message, long *result) {
#ifdef Q_OS_WIN32
    MSG* win32message = static_cast<MSG*>(message);
    switch (win32message->message) {
        case WM_ERASEBKGND:
            *result = 1L;
            return TRUE;

        default: 
            break;
    }
#endif
    return QWidget::nativeEvent(eventType, message, result);
}

QPaintEngine* GLWidget::paintEngine() const {
    return _paintEngine;
}
