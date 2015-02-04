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

#include <Settings.h>

#include "MainWindow.h"
#include "Menu.h"
#include "Util.h"

namespace SettingHandles {
    const SettingHandle<QRect> windowGeometry("WindowGeometry");
}


MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
}

void MainWindow::restoreGeometry() {
    // Did not use setGeometry() on purpose,
    // see http://doc.qt.io/qt-5/qsettings.html#restoring-the-state-of-a-gui-application
    QRect geometry = SettingHandles::windowGeometry.get(qApp->desktop()->availableGeometry());
    move(geometry.topLeft());
    resize(geometry.size());
}

void MainWindow::saveGeometry() {
    // Did not use geometry() on purpose,
    // see http://doc.qt.io/qt-5/qsettings.html#restoring-the-state-of-a-gui-application
    QRect geometry(pos(), size());
    SettingHandles::windowGeometry.set(geometry);
}

void MainWindow::moveEvent(QMoveEvent* event) {
    emit windowGeometryChanged(QRect(event->pos(), size()));
    QMainWindow::moveEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    emit windowGeometryChanged(QRect(QPoint(x(), y()), event->size()));
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
             windowState() == Qt::WindowMinimized) {
            emit windowShown(false);
        } else {
            emit windowShown(true);
        }
        
        if (isFullScreen() != Menu::getInstance()->isOptionChecked(MenuOption::Fullscreen)) {
            Menu::getInstance()->setIsOptionChecked(MenuOption::Fullscreen, isFullScreen());
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
