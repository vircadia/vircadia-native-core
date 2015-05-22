//
//  WindowDisplayPlugin.cpp
//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "WindowDisplayPlugin.h"
#include "RenderUtil.h"

#include <QCoreApplication>

WindowDisplayPlugin::WindowDisplayPlugin() {
    connect(&_timer, &QTimer::timeout, this, [&] {
        emit requestRender();
    });
}

const QString WindowDisplayPlugin::NAME("WindowDisplayPlugin");

const QString & WindowDisplayPlugin::getName() {
    return NAME;
}

void WindowDisplayPlugin::activate() {
    GlWindowDisplayPlugin::activate();
    _window->installEventFilter(this);
    _window->show();
    _timer.start(8);
}

void WindowDisplayPlugin::deactivate() {
    _timer.stop();
    GlWindowDisplayPlugin::deactivate();
}

bool WindowDisplayPlugin::eventFilter(QObject* object, QEvent* event) {
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
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

#if 0

//
//  MainWindow.h
//  interface
//
//  Created by Mohammed Nafees on 04/06/2014.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __hifi__MainWindow__
#define __hifi__MainWindow__

#include <QWindow>
#include <QTimer>

#include <SettingHandle.h>

#define MSECS_PER_FRAME_WHEN_THROTTLED 66

class MainWindow : public QWindow {
public:
    explicit MainWindow(QWindow* parent = NULL);

    // Some helpers for compatiblity with QMainWindow
    void activateWindow() {
        requestActivate();
    }

    bool isMinimized() const {
        return windowState() == Qt::WindowMinimized;
    }

    void stopFrameTimer();

    bool isThrottleRendering() const;

    int getDeviceWidth() const;
    int getDeviceHeight() const;
    QSize getDeviceSize() const { return QSize(getDeviceWidth(), getDeviceHeight()); }

    /*



    private slots:
    void activeChanged(Qt::ApplicationState state);
    void throttleRender();
    bool eventFilter(QObject*, QEvent* event);
    */
    public slots:
    void restoreGeometry();
    void saveGeometry();

signals:
    void windowGeometryChanged(QRect geometry);
    void windowShown(bool shown);

protected:
    virtual void moveEvent(QMoveEvent* event);
    virtual void resizeEvent(QResizeEvent* event);
    virtual void showEvent(QShowEvent* event);
    virtual void hideEvent(QHideEvent* event);
    virtual void changeEvent(QEvent* event);
    virtual void windowStateChanged(Qt::WindowState windowState);
    virtual void activeChanged();

    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int width, int height);

private:
    Setting::Handle<QRect> _windowGeometry;
    Setting::Handle<int> _windowState;
    Qt::WindowState _lastState{ Qt::WindowNoState };
    QOpenGLContext * _context{ nullptr };
    QTimer _frameTimer;
    bool _throttleRendering{ false };
    int _idleRenderInterval{ MSECS_PER_FRAME_WHEN_THROTTLED };
};

#endif /* defined(__hifi__MainWindow__) */




//
//  MainWindow.cpp
//  interface
//
//  Created by Mohammed Nafees on 04/06/2014.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QApplication>
#include <QDesktopWidget>
#include <QEvent>
#include <QMoveEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QHideEvent>
#include <QWindowStateChangeEvent>

#include "Application.h"

#include "MainWindow.h"
#include "Menu.h"
#include "Util.h"

#include <QOpenGLContext>


MainWindow::MainWindow(QWindow * parent) :
QWindow(parent),
_windowGeometry("WindowGeometry"),
_windowState("WindowState", 0) {
    setSurfaceType(QSurface::OpenGLSurface);

    QSurfaceFormat format;
    // Qt Quick may need a depth and stencil buffer. Always make sure these are available.
    format.setDepthBufferSize(16);
    format.setStencilBufferSize(8);
    format.setVersion(4, 1);
    // Ugh....
    format.setProfile(QSurfaceFormat::OpenGLContextProfile::CompatibilityProfile);
    setFormat(format);

    _context = new QOpenGLContext;
    _context->setFormat(format);
    _context->create();

    show();
}

void MainWindow::restoreGeometry() {
    QRect geometry = _windowGeometry.get(qApp->desktop()->availableGeometry());
    setGeometry(geometry);

    // Restore to maximized or full screen after restoring to windowed so that going windowed goes to good position and sizes.
    int state = _windowState.get(Qt::WindowNoState) & ~Qt::WindowActive;
    if (state != Qt::WindowNoState) {
        setWindowState((Qt::WindowState)state);
    }
}

void MainWindow::saveGeometry() {
    // Did not use geometry() on purpose,
    // see http://doc.qt.io/qt-5/qsettings.html#restoring-the-state-of-a-gui-application
    _windowState.set((int)windowState());

    // Save position and size only if windowed so that have good values for windowed after starting maximized or full screen.
    if (windowState() == Qt::WindowNoState) {
        _windowGeometry.set(geometry());
    }
}

void MainWindow::moveEvent(QMoveEvent* event) {
    emit windowGeometryChanged(QRect(event->pos(), size()));
    QWindow::moveEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    emit windowGeometryChanged(QRect(QPoint(x(), y()), event->size()));
    QWindow::resizeEvent(event);
}

void MainWindow::showEvent(QShowEvent* event) {
    if (event->spontaneous()) {
        emit windowShown(true);
    }
    QWindow::showEvent(event);
}

void MainWindow::hideEvent(QHideEvent* event) {
    if (event->spontaneous()) {
        emit windowShown(false);
    }
    QWindow::hideEvent(event);
}

void MainWindow::windowStateChanged(Qt::WindowState windowState) {
    // If we're changing from minimized to non-minimized or vice versas, emit 
    // a windowShown signal (i.e. don't emit the signal if we're going from 
    // fullscreen to nostate
    if ((_lastState == Qt::WindowMinimized) ^ (windowState == Qt::WindowMinimized)) {
        emit windowShown(windowState != Qt::WindowMinimized);
    }

    bool fullscreen = windowState == Qt::WindowFullScreen;
    if (fullscreen != Menu::getInstance()->isOptionChecked(MenuOption::Fullscreen)) {
        Menu::getInstance()->setIsOptionChecked(MenuOption::Fullscreen, fullscreen);
    }

    _lastState = windowState;
    QWindow::windowStateChanged(windowState);
}

void MainWindow::activeChanged() {
    if (isActive()) {
        emit windowShown(true);
    } else {
        emit windowShown(false);
    }
    QWindow::activeChanged();
}

void MainWindow::stopFrameTimer() {
    _frameTimer.stop();
}

bool MainWindow::isThrottleRendering() const {
    return _throttleRendering || isMinimized();
}


int MainWindow::getDeviceWidth() const {
    return width() * devicePixelRatio();
}

int MainWindow::getDeviceHeight() const {
    return height() * devicePixelRatio();
}



void MainWindow::initializeGL() {
    Application::getInstance()->initializeGL();
    //    setAttribute(Qt::WA_AcceptTouchEvents);
    //    setAcceptDrops(true);
    connect(Application::getInstance(), SIGNAL(applicationStateChanged(Qt::ApplicationState)), this, SLOT(activeChanged(Qt::ApplicationState)));
    connect(&_frameTimer, SIGNAL(timeout()), this, SLOT(throttleRender()));
}

void GLCanvas::resizeGL(int width, int height) {
    Application::getInstance()->resizeGL(width, height);
}

void GLCanvas::paintGL() {
    if (!_throttleRendering && !Application::getInstance()->getWindow()->isMinimized()) {
        //Need accurate frame timing for the oculus rift
        if (OculusManager::isConnected()) {
            OculusManager::beginFrameTiming();
        }

        Application::getInstance()->paintGL();

        if (!OculusManager::isConnected()) {
            swapBuffers();
        } else {
            if (OculusManager::allowSwap()) {
                swapBuffers();
            }
            OculusManager::endFrameTiming();
        }
    }
}



/*

GLCanvas::GLCanvas() : QGLWidget(QGL::NoDepthBuffer | QGL::NoStencilBuffer),
#ifdef Q_OS_LINUX
// Cause GLCanvas::eventFilter to be called.
// It wouldn't hurt to do this on Mac and PC too; but apparently it's only needed on linux.
qApp->installEventFilter(this);
#endif
}

void GLCanvas::keyPressEvent(QKeyEvent* event) {
Application::getInstance()->keyPressEvent(event);
}

void GLCanvas::keyReleaseEvent(QKeyEvent* event) {
Application::getInstance()->keyReleaseEvent(event);
}

void GLCanvas::focusOutEvent(QFocusEvent* event) {
Application::getInstance()->focusOutEvent(event);
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
if (!_throttleRendering && !Application::getInstance()->isAboutToQuit()) {
_frameTimer.start(_idleRenderInterval);
_throttleRendering = true;
}
break;
}
}

void GLCanvas::throttleRender() {
_frameTimer.start(_idleRenderInterval);
if (!Application::getInstance()->getWindow()->isMinimized()) {
//Need accurate frame timing for the oculus rift
if (OculusManager::isConnected()) {
OculusManager::beginFrameTiming();
}

Application::getInstance()->paintGL();
swapBuffers();

if (OculusManager::isConnected()) {
OculusManager::endFrameTiming();
}
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
const QMimeData* mimeData = event->mimeData();
foreach(QUrl url, mimeData->urls()) {
auto urlString = url.toString();
if (Application::getInstance()->canAcceptURL(urlString)) {
event->acceptProposedAction();
break;
}
}
}

void GLCanvas::dropEvent(QDropEvent* event) {
Application::getInstance()->dropEvent(event);
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

*/
#endif
