//
//  LoginDialog.h
//  interface/src/ui
//
//  Created by Ryan Huffman on 4/23/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_LoginDialog_h
#define hifi_LoginDialog_h

#include <QObject>
#include "FramelessDialog.h"

namespace Ui {
    class LoginDialog;
}

class LoginDialog : public FramelessDialog {
    Q_OBJECT

public:
    LoginDialog(QWidget* parent);
    ~LoginDialog();

public slots:
    void toggleQAction();
    void showLoginForCurrentDomain();
    
protected slots:
    void reset();
    void handleLoginClicked();
    void handleLoginCompleted(const QUrl& authURL);
    void handleLoginFailed();

protected:
    void moveEvent(QMoveEvent* event);

private:
    Ui::LoginDialog* _ui = nullptr;
};

#endif // hifi_LoginDialog_h
