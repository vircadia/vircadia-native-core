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
    QMainWindow::moveEvent(e);
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    emit windowGeometryChanged(QRect(QPoint(x(), y()), e->size()));
    QMainWindow::resizeEvent(e);
}

void MainWindow::showEvent(QShowEvent *e)
{
    if (e->spontaneous()) {
        emit windowShown(true);
    }
    QMainWindow::showEvent(e);
}

void MainWindow::hideEvent(QHideEvent *e)
{
    if (e->spontaneous()) {
        emit windowShown(false);
    }
    QMainWindow::hideEvent(e);
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
    QMainWindow::changeEvent(e);
}
