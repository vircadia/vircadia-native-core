//
//  AddressBarDialog.h
//  interface/src/ui
//
//  Created by Stojce Slavkovski on 9/22/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AddressBarDialog_h
#define hifi_AddressBarDialog_h

#include "FramelessDialog.h"

#include <QLineEdit>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QVBoxLayout>


class AddressBarDialog : public FramelessDialog {
    Q_OBJECT

public:
    AddressBarDialog();
    
private:
    void setupUI();
    
    QVBoxLayout *verticalLayout;
    QHBoxLayout *addressLayout;
    QSpacerItem *leftSpacer;
    QLineEdit *addressLineEdit;
    QSpacerItem *buttonSpacer;
    QPushButton *goButton;
    QSpacerItem *rightSpacer;
    QPushButton *closeButton;
    
private slots:
    void accept();
    
};

#endif // hifi_AddressBarDialog_h
