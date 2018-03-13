//
//  OvenGUIApplication.h
//  tools/oven/src
//
//  Created by Stephen Birarda on 2/20/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OvenGUIApplication_h
#define hifi_OvenGUIApplication_h

#include <QtWidgets/QApplication>

#include "ui/OvenMainWindow.h"

#include "Oven.h"

class OvenGUIApplication : public QApplication, public Oven {
    Q_OBJECT
public:
    OvenGUIApplication(int argc, char* argv[]);

    static OvenGUIApplication* instance() { return dynamic_cast<OvenGUIApplication*>(QApplication::instance()); }
    
    OvenMainWindow* getMainWindow() { return &_mainWindow; }
private:
    OvenMainWindow _mainWindow;
};

#endif // hifi_OvenGUIApplication_h
