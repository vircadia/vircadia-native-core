//
//  Application.cpp
//  interface
//
//  Created by Andrzej Kapolka on 5/10/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <QMenuBar>
#include <QtDebug>
#include <PairingHandler.h>

#include "Application.h"

Application::Application(int& argc, char** argv) : QApplication(argc, argv) {    
    // simple menu bar (will only appear on OS X, for now)
    QMenuBar* menuBar = new QMenuBar();
    QMenu* fileMenu = menuBar->addMenu("File");

    fileMenu->addAction("Pair", this, SLOT(pair()));
}

void Application::pair() {
    PairingHandler::sendPairRequest();
}
