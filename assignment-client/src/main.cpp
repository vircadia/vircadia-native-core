//
//  main.cpp
//  assignment-client/src
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <LogHandler.h>
#include <SharedUtil.h>

#include "Assignment.h"
#include "AssignmentClient.h"
#include "AssignmentClientMonitor.h"

int main(int argc, char* argv[]) {
#ifndef WIN32
    setvbuf(stdout, NULL, _IOLBF, 0);
#endif

    // use the verbose message handler in Logging
    qInstallMessageHandler(LogHandler::verboseMessageHandler);
    
    const char* numForksString = getCmdOption(argc, (const char**)argv, NUM_FORKS_PARAMETER);
    
    int numForks = 0;
    
    if (numForksString) {
        numForks = atoi(numForksString);
    }
    
    if (numForks) {
        AssignmentClientMonitor monitor(argc, argv, numForks);
        return monitor.exec();
    } else {
        AssignmentClient client(argc, argv);
        return client.exec();
    }
}
