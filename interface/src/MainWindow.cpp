//
//  MainWindow.cpp
//  interface
//
//  Created by Mohammed Nafees on 04/06/2014.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.

#include "MainWindow.h"

#include <QEvent>
#include <QMoveEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QHideEvent>
#include <QWindowStateChangeEvent>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
}

void MainWindow::moveEvent(QMoveEvent *e)
{
    emit windowGeometryChanged(QRect(e->pos(), size()));
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    emit windowGeometryChanged(QRect(QPoint(x(), y()), e->size()));
}

void MainWindow::showEvent(QShowEvent *e)
{
    if (e->spontaneous()) {
        emit windowShown(true);
    }
}

void MainWindow::hideEvent(QHideEvent *e)
{
    if (e->spontaneous()) {
        emit windowShown(false);
    }
}

void MainWindow::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::WindowStateChange) {
        QWindowStateChangeEvent *event = static_cast<QWindowStateChangeEvent*>(e);
        if ((event->oldState() == Qt::WindowNoState ||
             event->oldState() == Qt::WindowMaximized) &&
             windowState() == Qt::WindowMinimized) {
            emit windowShown(false);
        } else {
            emit windowShown(true);
        }
    } else if (e->type() == QEvent::ActivationChange) {
        if (isActiveWindow()) {
            emit windowShown(true);
        } else {
            emit windowShown(false);
        }
    }
}
