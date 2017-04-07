//
//  Oven.h
//  tools/oven/src
//
//  Created by Stephen Birarda on 4/5/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Oven_h
#define hifi_Oven_h

#include <QtWidgets/QApplication>

class OvenMainWindow;

class Oven : public QApplication {
    Q_OBJECT

public:
    Oven(int argc, char* argv[]);

private:
    void setupGUI();

    OvenMainWindow* _mainWindow;
};


#endif // hifi_Oven_h
