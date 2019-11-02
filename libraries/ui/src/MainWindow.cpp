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

#include "MainWindow.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QEvent>
#include <QMoveEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QHideEvent>
#include <QWindowStateChangeEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QtGui/QWindow>
#include <QtCore/QDebug>

#include "ui/Logging.h"
#include "DockWidget.h"

#include <QSizePolicy>
#include <QLayout>

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    _windowGeometry("WindowGeometry"),
    _windowState("WindowState", 0)
{
    setAttribute(Qt::WA_NoSystemBackground);
    setAcceptDrops(true);
    setStyleSheet("QMainWindow::separator {width: 1px; border: none;}");
}

MainWindow::~MainWindow() {
    qCDebug(uiLogging) << "Destroying main window";
}

QWindow* MainWindow::findMainWindow() {
    auto windows = qApp->topLevelWindows();
    QWindow* result = nullptr;
    for (const auto& window : windows) {
        if (window->objectName().contains("MainWindow")) {
            result = window;
            break;
        }
    }
    return result;
}

void MainWindow::restoreGeometry() {
    // Did not use setGeometry() on purpose,
    // see http://doc.qt.io/qt-5/qsettings.html#restoring-the-state-of-a-gui-application
    QRect windowGeometry = QGuiApplication::primaryScreen()->availableGeometry();
#if defined(Q_OS_MAC)
    const float MACOS_INITIAL_WINDOW_SCALE = 0.8f;
    windowGeometry.setSize((windowGeometry.size() * MACOS_INITIAL_WINDOW_SCALE));
#endif
    QRect geometry = _windowGeometry.get(windowGeometry);
#if defined(Q_OS_MAC)
    move(geometry.center());
#else
    move(geometry.topLeft());
#endif
    resize(geometry.size());

    // Restore to maximized or full screen after restoring to windowed so that going windowed goes to good position and sizes.
    Qt::WindowStates state = (Qt::WindowStates)_windowState.get(Qt::WindowNoState);
    if (state != Qt::WindowNoState) {
        setWindowState(state);
    }
}

void MainWindow::saveGeometry() {
    // Did not use geometry() on purpose,
    // see http://doc.qt.io/qt-5/qsettings.html#restoring-the-state-of-a-gui-application
    _windowState.set((int)windowState());

    // Save position and size only if windowed so that have good values for windowed after starting maximized or full screen.
    if (windowState() == Qt::WindowNoState) {
        QRect geometry(pos(), size());
        _windowGeometry.set(geometry);
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // It is the job of Application::quit() to shut things down properly when it is finished with its event loop.
    // But if we don't explicitly ignore this event now, the window and application event loop will close
    // before we've had a chance to act on the aboutToClose signal. This will leaves a zombie process on all platforms.
    // To repro:
    //   Open a QML modal dialog (e.g., select an avatar to wear), but don't dismiss it.
    //   Close the application with the operating system window close button (not the menu Quit)
    //   With ignore: App will wait until you accept or dismiss the dialog and log "Normal exit".
    //   Without ignore: App will close immediately, and nothing will log about quitting or exit.
    event->ignore();
    qApp->quit();
}

void MainWindow::moveEvent(QMoveEvent* event) {
    emit windowGeometryChanged(QRect(QPoint(geometry().x(), geometry().y()), size()));  // Geometry excluding the window frame.
    QMainWindow::moveEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    emit windowGeometryChanged(QRect(QPoint(geometry().x(), geometry().y()), size()));  // Geometry excluding the window frame.
    QMainWindow::resizeEvent(event);
}

void MainWindow::showEvent(QShowEvent* event) {
    if (event->spontaneous()) {
        emit windowShown(true);
    }
    QMainWindow::showEvent(event);
}

void MainWindow::hideEvent(QHideEvent* event) {
    if (event->spontaneous()) {
        emit windowShown(false);
    }
    QMainWindow::hideEvent(event);
}

void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        QWindowStateChangeEvent* stateChangeEvent = static_cast<QWindowStateChangeEvent*>(event);
        if ((stateChangeEvent->oldState() == Qt::WindowNoState ||
            stateChangeEvent->oldState() == Qt::WindowMaximized) &&
            (windowState() & Qt::WindowMinimized) == Qt::WindowMinimized) {
            emit windowShown(false);
            emit windowMinimizedChanged(true);
        } else {
            emit windowShown(true);
            if ((stateChangeEvent->oldState() & Qt::WindowMinimized) == Qt::WindowMinimized) {
                emit windowMinimizedChanged(false);
            }
        }
    } else if (event->type() == QEvent::ActivationChange) {
        if (isActiveWindow()) {
            emit windowShown(true);
        } else {
            emit windowShown(false);
        }
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    QCoreApplication::sendEvent(QCoreApplication::instance(), event);
}
