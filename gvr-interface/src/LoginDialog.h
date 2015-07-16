//
//  LoginDialog.h
//  gvr-interface/src
//
//  Created by Stephen Birarda on 2015-02-03.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_LoginDialog_h
#define hifi_LoginDialog_h

#include <QtWidgets/QDialog>

class QLineEdit;

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    LoginDialog(QWidget* parent = 0);
signals:
    void credentialsEntered(const QString& username, const QString& password);
private slots:
    void loginButtonClicked();
private:
    void setupGUI();
    
    QLineEdit* _usernameLineEdit;
    QLineEdit* _passwordLineEdit;
};

#endif // hifi_LoginDialog_h