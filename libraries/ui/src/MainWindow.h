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

#include <SettingHandle.h>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = NULL);
    ~MainWindow();
    
public slots:
    void restoreGeometry();
    void saveGeometry();
    
signals:
    void windowGeometryChanged(QRect geometry);
    void windowShown(bool shown);

protected:
    virtual void closeEvent(QCloseEvent* event) override;
    virtual void moveEvent(QMoveEvent* event) override;
    virtual void resizeEvent(QResizeEvent* event) override;
    virtual void showEvent(QShowEvent* event) override;
    virtual void hideEvent(QHideEvent* event) override;
    virtual void changeEvent(QEvent* event) override;
    virtual void dragEnterEvent(QDragEnterEvent *e) override;
    virtual void dropEvent(QDropEvent *e) override;
    
private:
    Setting::Handle<QRect> _windowGeometry;
    Setting::Handle<int> _windowState;
};

#endif /* defined(__hifi__MainWindow__) */
