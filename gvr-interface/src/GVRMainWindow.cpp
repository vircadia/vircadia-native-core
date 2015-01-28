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

#include <AddressManager.h>

#include "InterfaceView.h"
#include "RenderingClient.h"

#include "GVRMainWindow.h"

GVRMainWindow::GVRMainWindow(QWidget* parent) :
    QMainWindow(parent)
{
    QMenu* fileMenu = new QMenu("File");
    QMenu* helpMenu = new QMenu("Help");
    
    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(helpMenu);
    
    QAction* goToAddress = new QAction("Go to Address", fileMenu);
    QAction* aboutQt = new QAction("About Qt", helpMenu);
    
    fileMenu->addAction(goToAddress);
    helpMenu->addAction(aboutQt);

    connect(goToAddress, &QAction::triggered, this, &GVRMainWindow::showAddressBar);
    connect(aboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
    
    QWidget* baseWidget = new QWidget(this);
    
    // setup a layout so we can vertically align to top
    _mainLayout = new QVBoxLayout(baseWidget);
    _mainLayout->setAlignment(Qt::AlignTop);
    
    // set the layout on the base widget
    baseWidget->setLayout(_mainLayout);
    
    setCentralWidget(baseWidget);
    
    // add the interface view
    InterfaceView* interfaceView = new InterfaceView(baseWidget);
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
