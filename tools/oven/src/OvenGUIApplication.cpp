//
//  OvenGUIApplication.cpp
//  tools/src/oven
//
//  Created by Stephen Birarda on 2/20/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OvenGUIApplication.h"

OvenGUIApplication::OvenGUIApplication(int argc, char* argv[]) :
    QApplication(argc, argv)
{
    // setup the GUI
    _mainWindow.show();
}
