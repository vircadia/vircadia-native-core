//
//  GLCanvas.cpp
//  interface/src
//
//  Created by Stephen Birarda on 8/14/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QMimeData>
#include <QUrl>
#include <QWindow>

#include "Application.h"
#include "GLCanvas.h"
#include "MainWindow.h"

const int MSECS_PER_FRAME_WHEN_THROTTLED = 66;

GLCanvas::GLCanvas() : QGLWidget(QGL::NoDepthBuffer | QGL::NoStencilBuffer),
    _throttleRendering(false),
    _idleRenderInterval(MSECS_PER_FRAME_WHEN_THROTTLED)
{
#ifdef Q_OS_LINUX
    // Cause GLCanvas::eventFilter to be called.
    // It wouldn't hurt to do this on Mac and PC too; but apparently it's only needed on linux.
    qApp->installEventFilter(this);
#endif
}

void GLCanvas::stopFrameTimer() {
    _frameTimer.stop();
}

bool GLCanvas::isThrottleRendering() const {
    return (_throttleRendering 
        || (Application::getInstance()->getWindow()->isMinimized() && Application::getInstance()->isThrottleFPSEnabled()));
}

int GLCanvas::getDeviceWidth() const {
    return width() * (windowHandle() ? (float)windowHandle()->devicePixelRatio() : 1.0f);
}

int GLCanvas::getDeviceHeight() const {
    return height() * (windowHandle() ? (float)windowHandle()->devicePixelRatio() : 1.0f);
}

void GLCanvas::initializeGL() {
    Application::getInstance()->initializeGL();
    setAttribute(Qt::WA_AcceptTouchEvents);
    setAcceptDrops(true);
    connect(Application::getInstance(), SIGNAL(applicationStateChanged(Qt::ApplicationState)), this, SLOT(activeChanged(Qt::ApplicationState)));
    connect(&_frameTimer, SIGNAL(timeout()), this, SLOT(throttleRender()));

    // Note, we *DO NOT* want Qt to automatically swap buffers for us.  This results in the "ringing" bug mentioned in WL#19514 when we're throttling the framerate.
    setAutoBufferSwap(false);
}

void GLCanvas::paintGL() {
    PROFILE_RANGE(__FUNCTION__);
    if (!_throttleRendering &&
            (!Application::getInstance()->getWindow()->isMinimized() || !Application::getInstance()->isThrottleFPSEnabled())) {
        Application::getInstance()->paintGL();
    }
}

void GLCanvas::resizeGL(int width, int height) {
    Application::getInstance()->resizeGL();
}

void GLCanvas::activeChanged(Qt::ApplicationState state) {
    switch (state) {
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
            if (!_throttleRendering && !Application::getInstance()->isAboutToQuit() 
                && Application::getInstance()->isThrottleFPSEnabled()) {
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
        case QEvent::Wheel:
        case QEvent::DragEnter:
        case QEvent::Drop:
            if (QCoreApplication::sendEvent(QCoreApplication::instance(), event)) {
                return true;
            }
            break;
        case QEvent::Paint:
            // Ignore paint events that occur after we've decided to quit
            if (Application::getInstance()->isAboutToQuit()) {
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

bool GLCanvas::eventFilter(QObject*, QEvent* event) {
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
