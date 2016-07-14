//
//
//  Created by Bradley Austin Davis on 2015/12/03
//  Derived from interface/src/GLCanvas.cpp created by Stephen Birarda on 8/14/13.
//  Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include <QtGlobal>

#include "GLWidget.h"

#include <mutex>

#include <QtCore/QMimeData>
#include <QtCore/QUrl>
#include <QtCore/QCoreApplication>

#include <QtGui/QOpenGLContext>
#include <QtGui/QKeyEvent>
#include <QtGui/QWindow>


#include "GLHelpers.h"


GLWidget::GLWidget() : QGLWidget(getDefaultGLFormat()) {
#ifdef Q_OS_LINUX
    // Cause GLWidget::eventFilter to be called.
    // It wouldn't hurt to do this on Mac and PC too; but apparently it's only needed on linux.
    qApp->installEventFilter(this);
#endif
}

int GLWidget::getDeviceWidth() const {
    return width() * (windowHandle() ? (float)windowHandle()->devicePixelRatio() : 1.0f);
}

int GLWidget::getDeviceHeight() const {
    return height() * (windowHandle() ? (float)windowHandle()->devicePixelRatio() : 1.0f);
}

void GLWidget::initializeGL() {
    setAttribute(Qt::WA_AcceptTouchEvents);
    grabGesture(Qt::PinchGesture);
    setAcceptDrops(true);
    // Note, we *DO NOT* want Qt to automatically swap buffers for us.  This results in the "ringing" bug mentioned in WL#19514 when we're throttling the framerate.
    setAutoBufferSwap(false);

    makeCurrent();
    if (isValid() && context() && context()->contextHandle()) {
#if defined(Q_OS_WIN)
        _vsyncSupported = context()->contextHandle()->hasExtension("WGL_EXT_swap_control");
#elif defined(Q_OS_MAC)
        _vsyncSupported = true;
#else
        // TODO: write the proper code for linux
#endif
    }
}

void GLWidget::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
}

void GLWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
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
    return QGLWidget::event(event);
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
                    QGLWidget::event(event);
                }
                return true;
            }
        }
        default:
            break;
    }
    return false;
}

bool GLWidget::isVsyncSupported() const {
    return _vsyncSupported;
}

