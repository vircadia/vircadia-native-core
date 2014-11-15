//
//  ToolWindow.cpp
//  interface/src/ui
//
//  Created by Ryan Huffman on 11/13/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"
#include "ToolWindow.h"
#include "UIUtil.h"

const int DEFAULT_WIDTH = 300;

ToolWindow::ToolWindow(QWidget* parent) :
    QMainWindow(parent),
    _hasShown(false),
    _lastGeometry() {
}

bool ToolWindow::event(QEvent* event) {
    QEvent::Type type = event->type();
    if (type == QEvent::Show) {
        if (!_hasShown) {
            _hasShown = true;

            QMainWindow* mainWindow = Application::getInstance()->getWindow();
            QRect mainGeometry = mainWindow->geometry();

            int titleBarHeight = UIUtil::getWindowTitleBarHeight(this);
            int menuBarHeight = Menu::getInstance()->geometry().height();
            int topMargin = titleBarHeight + menuBarHeight;

            _lastGeometry = QRect(mainGeometry.topLeft().x(),  mainGeometry.topLeft().y() + topMargin,
                                  DEFAULT_WIDTH, mainGeometry.height() - topMargin);
        }
        setGeometry(_lastGeometry);
        return true;
    } else if (type == QEvent::Hide) {
        _lastGeometry = geometry();
        return true;
    }

    return QMainWindow::event(event);
}

void ToolWindow::onChildVisibilityUpdated(bool visible) {
    if (visible) {
        setVisible(true);
    } else {
        bool hasVisible = false;
        QList<QDockWidget*> dockWidgets = findChildren<QDockWidget*>();
        for (int i = 0; i < dockWidgets.count(); i++) {
            if (dockWidgets[i]->isVisible()) {
                hasVisible = true;
                break;
            }
        }
        setVisible(hasVisible);
    }
}

void ToolWindow::addDockWidget(Qt::DockWidgetArea area, QDockWidget* dockWidget) {
    QMainWindow::addDockWidget(area, dockWidget);

    connect(dockWidget, &QDockWidget::visibilityChanged, this, &ToolWindow::onChildVisibilityUpdated);
}

void ToolWindow::addDockWidget(Qt::DockWidgetArea area, QDockWidget* dockWidget, Qt::Orientation orientation) {
    QMainWindow::addDockWidget(area, dockWidget, orientation);

    connect(dockWidget, &QDockWidget::visibilityChanged, this, &ToolWindow::onChildVisibilityUpdated);
}

void ToolWindow::removeDockWidget(QDockWidget* dockWidget) {
    QMainWindow::removeDockWidget(dockWidget);

    disconnect(dockWidget, &QDockWidget::visibilityChanged, this, &ToolWindow::onChildVisibilityUpdated);
}
