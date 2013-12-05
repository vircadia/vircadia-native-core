//
//  main.cpp
//  Domain Server 
//
//  Created by Philip Rosedale on 11/20/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//
//  The Domain Server keeps a list of nodes that have connected to it, and echoes that list of
//  nodes out to nodes when they check in.
//
//  The connection is stateless... the domain server will set you inactive if it does not hear from
//  you in LOGOFF_CHECK_INTERVAL milliseconds, meaning your info will not be sent to other users.
//

#include <QtCore/QCoreApplication>

#include <Logging.h>

#include "DomainServer.h"

int main(int argc, char* argv[]) {
    
    setvbuf(stdout, NULL, _IOLBF, 0);
    
    qInstallMessageHandler(Logging::verboseMessageHandler);
    
    DomainServer domainServer(argc, argv);
    
    return domainServer.exec();
}

