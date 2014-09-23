//
//  AddressBarDialog.cpp
//  interface/src/ui
//
//  Created by Stojce Slavkovski on 9/22/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AddressBarDialog.h"
#include "AddressManager.h"
#include "Application.h"

AddressBarDialog::AddressBarDialog() :
    FramelessDialog(Application::getInstance()->getWindow(), 0, FramelessDialog::POSITION_TOP){
        setupUI();
}

void AddressBarDialog::setupUI() {
    
    setModal(true);
    setWindowModality(Qt::WindowModal);
    setHideOnBlur(false);
    
    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setSizePolicy(sizePolicy);
    setMinimumSize(QSize(560, 100));
    setStyleSheet("font-family: Helvetica, Arial, sans-serif;");

    verticalLayout = new QVBoxLayout(this);
    
    addressLayout = new QHBoxLayout();
    addressLayout->setContentsMargins(0, 0, 10, 0);
    
    leftSpacer = new QSpacerItem(20, 20, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    addressLayout->addItem(leftSpacer);
    
    addressLineEdit = new QLineEdit(this);
    addressLineEdit->setPlaceholderText("Go to: domain, @user, #location");
    QSizePolicy sizePolicyLineEdit(QSizePolicy::Preferred, QSizePolicy::Fixed);
    sizePolicyLineEdit.setHorizontalStretch(60);
    addressLineEdit->setSizePolicy(sizePolicyLineEdit);
    addressLineEdit->setMinimumSize(QSize(200, 54));
    addressLineEdit->setMaximumSize(QSize(615, 54));
    QFont font("Helvetica,Arial,sans-serif", 20);
    addressLineEdit->setFont(font);
    addressLineEdit->setStyleSheet("padding: 0 10px;");
    addressLayout->addWidget(addressLineEdit);
    
    buttonSpacer = new QSpacerItem(10, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);
    addressLayout->addItem(buttonSpacer);
    
    goButton = new QPushButton(this);
    goButton->setSizePolicy(sizePolicy);
    goButton->setMinimumSize(QSize(54, 54));
    goButton->setIcon(QIcon(Application::resourcesPath() + "images/arrow.svg"));
    goButton->setStyleSheet("background: #0e7077; color: #e7eeee; border-radius: 4px;");
    goButton->setIconSize(QSize(32, 32));
    goButton->setDefault(true);
    goButton->setFlat(true);
    addressLayout->addWidget(goButton);
    
    rightSpacer = new QSpacerItem(20, 20, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    addressLayout->addItem(rightSpacer);
    
    closeButton = new QPushButton(this);
    closeButton->setSizePolicy(sizePolicy);
    closeButton->setMinimumSize(QSize(16, 16));
    closeButton->setMaximumSize(QSize(16, 16));
    closeButton->setStyleSheet("color: #333");
    QIcon icon(Application::resourcesPath() + "styles/close.svg");
    closeButton->setIcon(icon);
    closeButton->setFlat(true);
    addressLayout->addWidget(closeButton, 0, Qt::AlignRight);
    
    verticalLayout->addLayout(addressLayout);
    
    connect(goButton, &QPushButton::clicked, this, &AddressBarDialog::accept);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
}

void AddressBarDialog::accept() {
    if (!addressLineEdit->text().isEmpty()) {
        goButton->setStyleSheet("background: #333; color: #e7eeee; border-radius: 4px;");
        
        AddressManager& addressManager = AddressManager::getInstance();
        connect(&addressManager, &AddressManager::lookupResultsFinished, this, &QDialog::close);
        addressManager.handleLookupString(addressLineEdit->text());
    }
}