//
//  FramelessDialog.h
//  interface/src/ui
//
//  Created by Stojce Slavkovski on 2/20/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_FramelessDialog_h
#define hifi_FramelessDialog_h

#include <QDialog>

class FramelessDialog : public QDialog {
    Q_OBJECT
    
public:
    FramelessDialog(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    void setStyleSheetFile(const QString& fileName);

protected:
    virtual void mouseMoveEvent(QMouseEvent* mouseEvent);
    virtual void mousePressEvent(QMouseEvent* mouseEvent);
    virtual void mouseReleaseEvent(QMouseEvent* mouseEvent);
    virtual void showEvent(QShowEvent* event);

    bool eventFilter(QObject* sender, QEvent* event);

private:
    bool _isResizing;

};

#endif // hifi_FramelessDialog_h
