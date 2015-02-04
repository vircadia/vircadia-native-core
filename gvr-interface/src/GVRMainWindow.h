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

#if defined(ANDROID) && defined(HAVE_LIBOVR)
#include <KeyState.h>
#endif

class QKeyEvent;
class QMenuBar;
class QVBoxLayout;

class GVRMainWindow : public QMainWindow {
    Q_OBJECT
public:
    GVRMainWindow(QWidget* parent = 0);
    ~GVRMainWindow();
public slots:
    void showAddressBar();
    void showLoginDialog();
    
    void showLoginFailure();
    
#if defined(ANDROID) && defined(HAVE_LIBOVR)
    OVR::KeyState& getBackKeyState() { return _backKeyState; }
#endif
    
protected:
    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);
private slots:
    void refreshLoginAction();
private:
    void setupMenuBar();
    
#if defined(ANDROID) && defined(HAVE_LIBOVR)
    OVR::KeyState _backKeyState;
    bool _wasBackKeyDown;
#endif
    
    QVBoxLayout* _mainLayout;
    QMenuBar* _menuBar;
    QAction* _loginAction;
};

#endif // hifi_GVRMainWindow_h
