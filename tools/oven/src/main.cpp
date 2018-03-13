//
//  main.cpp
//  tools/oven/src
//
//  Created by Stephen Birarda on 3/28/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

#include "OvenCLIApplication.h"
#include "OvenGUIApplication.h"

#include <BuildInfo.h>
#include <SettingInterface.h>
#include <SharedUtil.h>

int main (int argc, char** argv) {
    setupHifiApplication("Oven");

    // init the settings interface so we can save and load settings
    Setting::init();

    // figure out if we're launching our GUI application or just the simple command line interface
    if (argc > 1) {
        OvenCLIApplication app { argc, argv };
        return app.exec();
    } else {
        OvenGUIApplication app { argc, argv };
        return app.exec();
    }
}
