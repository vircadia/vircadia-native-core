//
//  GVRMainWindow.h
//  gvr-interface/src
//
//  Created by Stephen Birarda on 1/20/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GVRMainWindow_h
#define hifi_GVRMainWindow_h

#include <QtWidgets/QMainWindow>

class QMenuBar;
class QVBoxLayout;

class GVRMainWindow : public QMainWindow {
    Q_OBJECT
public:
    GVRMainWindow(QWidget* parent = 0);
    ~GVRMainWindow();
public slots:
    void showAddressBar();
private:
    void setupMenuBar();
    
    QVBoxLayout* _mainLayout;
    QMenuBar* _menuBar;
};

#endif // hifi_GVRMainWindow_h
