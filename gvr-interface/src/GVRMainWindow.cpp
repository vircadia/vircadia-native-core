//
//  GVRMainWindow.cpp
//  gvr-interface/src
//
//  Created by Stephen Birarda on 1/20/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtWidgets/QApplication>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMenuBar>

#ifndef ANDROID
#include <QtWidgets/QDesktopWidget>
#endif

#include <AddressManager.h>

#include "InterfaceView.h"
#include "RenderingClient.h"

#include "GVRMainWindow.h"

GVRMainWindow::GVRMainWindow(QWidget* parent) :
    QMainWindow(parent),
    _menuBar(NULL)
{
    
#ifndef ANDROID
    const int NOTE_4_WIDTH = 2560;
    const int NOTE_4_HEIGHT = 1440;
    setFixedSize(NOTE_4_WIDTH / 2, NOTE_4_HEIGHT / 2);
#endif
    
    setupMenuBar();
    
    QWidget* baseWidget = new QWidget(this);
    
    // setup a layout so we can vertically align to top
    _mainLayout = new QVBoxLayout(baseWidget);
    _mainLayout->setAlignment(Qt::AlignTop);
    
    // set the layout on the base widget
    baseWidget->setLayout(_mainLayout);
    
    setCentralWidget(baseWidget);
    
    // add the interface view
    new InterfaceView(baseWidget);
}

GVRMainWindow::~GVRMainWindow() {
    delete _menuBar;
}

void GVRMainWindow::setupMenuBar() {
    QMenu* fileMenu = new QMenu("File");
    QMenu* helpMenu = new QMenu("Help");
    
    _menuBar = new QMenuBar(0);
    
    _menuBar->addMenu(fileMenu);
    _menuBar->addMenu(helpMenu);
    
    QAction* goToAddress = new QAction("Go to Address", fileMenu);
    connect(goToAddress, &QAction::triggered, this, &GVRMainWindow::showAddressBar);
    fileMenu->addAction(goToAddress);
    
    QAction* aboutQt = new QAction("About Qt", helpMenu);
    connect(aboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
    helpMenu->addAction(aboutQt);
    
    setMenuBar(_menuBar);
}

void GVRMainWindow::showAddressBar() {
    // setup the address QInputDialog
    QInputDialog* addressDialog = new QInputDialog(this);
    addressDialog->setLabelText("Address");
    
    // add the address dialog to the main layout
    _mainLayout->addWidget(addressDialog);
    
    connect(addressDialog, &QInputDialog::textValueSelected, 
            DependencyManager::get<AddressManager>().data(), &AddressManager::handleLookupString);
}
