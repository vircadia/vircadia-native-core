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

int main(int argc, char* argv[]) {
    
    setvbuf(stdout, NULL, _IOLBF, 0);
    
    qInstallMessageHandler(Logging::verboseMessageHandler);
    
    DataServer dataServer(argc, argv);
    
    return dataServer.exec();
}