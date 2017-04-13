//
//  Oven.cpp
//  tools/oven/src
//
//  Created by Stephen Birarda on 4/5/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QDebug>

#include <SettingInterface.h>

#include "ui/OvenMainWindow.h"

#include "Oven.h"

static const QString OUTPUT_FOLDER = "/Users/birarda/code/hifi/lod/test-oven/export";

Oven::Oven(int argc, char* argv[]) :
    QApplication(argc, argv)
{
    QCoreApplication::setOrganizationName("High Fidelity");
    QCoreApplication::setApplicationName("Oven");

    // init the settings interface so we can save and load settings
    Setting::init();

    // check if we were passed any command line arguments that would tell us just to run without the GUI

    // setup the GUI
    _mainWindow = new OvenMainWindow;
    _mainWindow->show();
}

