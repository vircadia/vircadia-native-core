//
//  ToolWindow.h
//  interface/src/ui
//
//  Created by Ryan Huffman on 11/13/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ToolWindow_h
#define hifi_ToolWindow_h

#include <QDockWidget>
#include <QEvent>
#include <QMainWindow>
#include <QRect>
#include <QWidget>

class ToolWindow : public QMainWindow {
    Q_OBJECT
public:
    ToolWindow(QWidget* parent = NULL);

    virtual bool event(QEvent* event);
    virtual void addDockWidget(Qt::DockWidgetArea area, QDockWidget* dockWidget);
    virtual void addDockWidget(Qt::DockWidgetArea area, QDockWidget* dockWidget, Qt::Orientation orientation);
    virtual void removeDockWidget(QDockWidget* dockWidget);

    virtual bool eventFilter(QObject* sender, QEvent* event);

public slots:
    void onChildVisibilityUpdated(bool visible);


private:
    // Indicates whether this window was hidden by itself (because the main window lost focus).
    bool _selfHidden;
    bool _hasShown;
    QRect _lastGeometry;
};

#endif // hifi_ToolWindow_h
