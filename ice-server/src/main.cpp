//
//  main.cpp
//  ice-server/src
//
//  Created by Stephen Birarda on 10/01/12.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QCoreApplication>

#include <LogHandler.h>

#include "IceServer.h"

int main(int argc, char* argv[]) {
#ifndef WIN32
    setvbuf(stdout, NULL, _IOLBF, 0);
#endif

    qInstallMessageHandler(LogHandler::verboseMessageHandler);
    qInfo() << "Starting.";
    
    IceServer iceServer(argc, argv);
    return iceServer.exec();
}