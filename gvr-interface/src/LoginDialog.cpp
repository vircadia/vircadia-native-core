//
//  LoginDialog.cpp
//  gvr-interface/src
//
//  Created by Stephen Birarda on 2015-02-03.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "LoginDialog.h"

#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>

LoginDialog::LoginDialog(QWidget* parent) :
    QDialog(parent)
{
    setupGUI();
    setWindowTitle("Login");
    setModal(true);
}

void LoginDialog::setupGUI() {
    // setup a grid layout
    QGridLayout* formGridLayout = new QGridLayout(this);
 
    _usernameLineEdit = new QLineEdit(this);
    
    QLabel* usernameLabel = new QLabel(this);
    usernameLabel->setText("Username");
    usernameLabel->setBuddy(_usernameLineEdit);
    
    formGridLayout->addWidget(usernameLabel, 0, 0);
    formGridLayout->addWidget(_usernameLineEdit, 1, 0);
    
    _passwordLineEdit = new QLineEdit(this);
    _passwordLineEdit->setEchoMode(QLineEdit::Password);

    QLabel* passwordLabel = new QLabel(this);
    passwordLabel->setText("Password");
    passwordLabel->setBuddy(_passwordLineEdit);
    
    formGridLayout->addWidget(passwordLabel, 2, 0);
    formGridLayout->addWidget(_passwordLineEdit, 3, 0);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(this);
    
    QPushButton* okButton = buttons->addButton(QDialogButtonBox::Ok);
    QPushButton* cancelButton = buttons->addButton(QDialogButtonBox::Cancel);
    
    okButton->setText("Login");
    
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::close);
    connect(okButton, &QPushButton::clicked, this, &LoginDialog::loginButtonClicked);
    
    formGridLayout->addWidget(buttons, 4, 0, 1, 2);
    
    setLayout(formGridLayout);
}

void LoginDialog::loginButtonClicked() {
    emit credentialsEntered(_usernameLineEdit->text(), _passwordLineEdit->text());
    close();
}
