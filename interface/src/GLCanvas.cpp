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

#include "Application.h"
#include "GLCanvas.h"

#include <QMimeData>
#include <QUrl>
#include <QWindow>

#include "MainWindow.h"
#include "Menu.h"

static QGLFormat& getDesiredGLFormat() {
    // Specify an OpenGL 3.3 format using the Core profile.
    // That is, no old-school fixed pipeline functionality
    static QGLFormat glFormat;
    static std::once_flag once;
    std::call_once(once, [] {
        glFormat.setVersion(4, 1);
        glFormat.setProfile(QGLFormat::CoreProfile); // Requires >=Qt-4.8.0
        glFormat.setSampleBuffers(false);
        glFormat.setDepth(false);
        glFormat.setStencil(false);
    });
    return glFormat;
}

GLCanvas::GLCanvas() : QGLWidget(getDesiredGLFormat()) {
#ifdef Q_OS_LINUX
    // Cause GLCanvas::eventFilter to be called.
    // It wouldn't hurt to do this on Mac and PC too; but apparently it's only needed on linux.
    qApp->installEventFilter(this);
#endif
}

int GLCanvas::getDeviceWidth() const {
    return width() * (windowHandle() ? (float)windowHandle()->devicePixelRatio() : 1.0f);
}

int GLCanvas::getDeviceHeight() const {
    return height() * (windowHandle() ? (float)windowHandle()->devicePixelRatio() : 1.0f);
}

void GLCanvas::initializeGL() {
    setAttribute(Qt::WA_AcceptTouchEvents);
    setAcceptDrops(true);
    // Note, we *DO NOT* want Qt to automatically swap buffers for us.  This results in the "ringing" bug mentioned in WL#19514 when we're throttling the framerate.
    setAutoBufferSwap(false);
}

void GLCanvas::paintGL() {
    PROFILE_RANGE(__FUNCTION__);
    
    // FIXME - I'm not sure why this still remains, it appears as if this GLCanvas gets a single paintGL call near
    // the beginning of the application starting up. I'm not sure if we really need to call Application::paintGL()
    // in this case, since the display plugins eventually handle all the painting
    bool isThrottleFPSEnabled = Menu::getInstance()->isOptionChecked(MenuOption::ThrottleFPSIfNotFocus);
    if (!qApp->getWindow()->isMinimized() || !isThrottleFPSEnabled) {
        qApp->paintGL();
    }
}

void GLCanvas::resizeGL(int width, int height) {
    qApp->resizeGL();
}

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
            if (qApp->isAboutToQuit()) {
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
