//
//  MainWindow.h
//  interface
//
//  Created by Mohammed Nafees on 04/06/2014.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.

#ifndef __hifi__MainWindow__
#define __hifi__MainWindow__

#include <QMainWindow>

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);

signals:
    void windowGeometryChanged(QRect geometry);
    void windowShown(bool shown);

protected:
    virtual void moveEvent(QMoveEvent *e);
    virtual void resizeEvent(QResizeEvent *e);
    virtual void showEvent(QShowEvent *e);
    virtual void hideEvent(QHideEvent *e);
    virtual void changeEvent(QEvent *e);
};

#endif /* defined(__hifi__MainWindow__) */
