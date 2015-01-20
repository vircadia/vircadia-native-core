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

#include <QtWidgets/QMenuBar>

#include "GVRInterface.h"
#include "RenderingClient.h"

#include "GVRMainWindow.h"

GVRMainWindow::GVRMainWindow(QWidget* parent) :
    QMainWindow(parent)
{
    QMenu *fileMenu = new QMenu("File");
    QMenu *helpMenu = new QMenu("Help");
    
    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(helpMenu);
    
    QAction *goToAddress = new QAction("Go to Address", fileMenu);
    QAction *aboutQt = new QAction("About Qt", helpMenu);
    
    fileMenu->addAction(goToAddress);
    helpMenu->addAction(aboutQt);

    QObject::connect(goToAddress, &QAction::triggered, qApp->getClient(), &RenderingClient::showAddressBar);
    QObject::connect(aboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
}
