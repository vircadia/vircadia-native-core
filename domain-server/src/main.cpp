//
//  main.cpp
//  domain-server/src
//
//  Created by Philip Rosedale on 11/20/12.
//  Copyright 2012 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QCoreApplication>

#include <Logging.h>

#include "DomainServer.h"

int main(int argc, char* argv[]) {
    
#ifndef WIN32
    setvbuf(stdout, NULL, _IOLBF, 0);
#endif

    qInstallMessageHandler(Logging::verboseMessageHandler);
    DomainServer domainServer(argc, argv);
    
    return domainServer.exec();
}

