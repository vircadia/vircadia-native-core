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

#include <QMainWindow>
#include <QSystemTrayIcon>

#include <SettingHandle.h>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = NULL);
    
public slots:
    void restoreGeometry();
    void saveGeometry();
    
signals:
    void windowGeometryChanged(QRect geometry);
    void windowShown(bool shown);

protected:
    virtual void closeEvent(QCloseEvent* event);
    virtual void moveEvent(QMoveEvent* event);
    virtual void resizeEvent(QResizeEvent* event);
    virtual void showEvent(QShowEvent* event);
    virtual void hideEvent(QHideEvent* event);
    virtual void changeEvent(QEvent* event);
    virtual void dragEnterEvent(QDragEnterEvent *e);
    virtual void dropEvent(QDropEvent *e);
    
private:
    Setting::Handle<QRect> _windowGeometry;
    Setting::Handle<int> _windowState;
    QSystemTrayIcon _trayIcon;
};

#endif /* defined(__hifi__MainWindow__) */
