
//
//  Created by Bradley Austin Davis on 2018/11/22
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include <QtGui/QGuiApplication>

#include <Trace.h>

#include "PlayerWindow.h"

void initWebView();

int main(int argc, char** argv) {
    setupHifiApplication("uiApp");
    QGuiApplication app(argc, argv);
    initWebView();
    {
        DependencyManager::set<tracing::Tracer>();
        PlayerWindow window;
        app.exec();
    }
    return 0;
}


