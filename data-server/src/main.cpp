//
//  main.cpp
//  data-server
//
//  Created by Stephen Birarda on 1/1/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QCoreApplication>

#include <Logging.h>

#include "DataServer.h"

const char DATA_SERVER_LOGGING_TARGET_NAME[] = "data-server";

int main(int argc, char* argv[]) {
    
    setvbuf(stdout, NULL, _IOLBF, 0);
    
    qInstallMessageHandler(Logging::verboseMessageHandler);
    
    Logging::setTargetName(DATA_SERVER_LOGGING_TARGET_NAME);
    
    DataServer dataServer(argc, argv);
    
    return dataServer.exec();
}