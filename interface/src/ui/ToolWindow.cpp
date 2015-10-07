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
#include "MainWindow.h"
#include "ToolWindow.h"
#include "UIUtil.h"

const int DEFAULT_WIDTH = 300;

ToolWindow::ToolWindow(QWidget* parent) :
    QMainWindow(parent),
    _selfHidden(false),
    _hasShown(false),
    _lastGeometry() {

#   ifndef Q_OS_LINUX
    setDockOptions(QMainWindow::ForceTabbedDocks);
#   endif
    qApp->installEventFilter(this);
}

bool ToolWindow::event(QEvent* event) {
    QEvent::Type type = event->type();
    if (type == QEvent::Show) {
        if (!_hasShown) {
            _hasShown = true;

            QMainWindow* mainWindow = qApp->getWindow();
            QRect mainGeometry = mainWindow->geometry();

            int titleBarHeight = UIUtil::getWindowTitleBarHeight(this);
            int topMargin = titleBarHeight;

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

bool ToolWindow::eventFilter(QObject* sender, QEvent* event) {
#   ifndef Q_OS_LINUX
    switch (event->type()) {
        case QEvent::WindowStateChange:
            if (qApp->getWindow()->isMinimized()) {
                // If we are already visible, we are self-hiding
                _selfHidden = isVisible();
                setVisible(false);
            } else {
                if (_selfHidden) {
                    setVisible(true);
                }
            }
            break;
        case QEvent::ApplicationDeactivate:
            _selfHidden = isVisible();
            setVisible(false);
            break;
        case QEvent::ApplicationActivate:
            if (_selfHidden) {
                setVisible(true);
            }
            break;
        default:
            break;
    }
#   endif
    return false;
}

void ToolWindow::onChildVisibilityUpdated(bool visible) {
    if (!_selfHidden && visible) {
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
        // If a child was hidden and we don't have any children still visible, hide ourself.
        if (!hasVisible) {
            setVisible(false);
        }
    }
}

void ToolWindow::addDockWidget(Qt::DockWidgetArea area, QDockWidget* dockWidget) {
    QList<QDockWidget*> dockWidgets = findChildren<QDockWidget*>();

    QMainWindow::addDockWidget(area, dockWidget);

    // We want to force tabbing, so retabify all of our widgets.
    QDockWidget* lastDockWidget = dockWidget;
    foreach (QDockWidget* nextDockWidget, dockWidgets) {
        tabifyDockWidget(lastDockWidget, nextDockWidget);
        lastDockWidget = nextDockWidget;
    }

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
