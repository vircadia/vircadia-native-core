//
//  main.cpp
//  domain-server/src
//
//  Created by Philip Rosedale on 11/20/12.
//  Copyright 2012 High Fidelity, Inc.
//
//  The Domain Server keeps a list of nodes that have connected to it, and echoes that list of
//  nodes out to nodes when they check in.
//
//  The connection is stateless... the domain server will set you inactive if it does not hear from
//  you in LOGOFF_CHECK_INTERVAL milliseconds, meaning your info will not be sent to other users.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <LogHandler.h>
#include <SharedUtil.h>
#include <BuildInfo.h>

#include "DomainServer.h"

int main(int argc, char* argv[]) {
    disableQtBearerPoll(); // Fixes wifi ping spikes

    QCoreApplication::setApplicationName(BuildInfo::DOMAIN_SERVER_NAME);
    QCoreApplication::setOrganizationName(BuildInfo::MODIFIED_ORGANIZATION);
    QCoreApplication::setOrganizationDomain(BuildInfo::ORGANIZATION_DOMAIN);
    QCoreApplication::setApplicationVersion(BuildInfo::VERSION);

#ifndef WIN32
    setvbuf(stdout, NULL, _IOLBF, 0);
#endif

    qInstallMessageHandler(LogHandler::verboseMessageHandler);
    qInfo() << "Starting.";

    int currentExitCode = 0;

    // use a do-while to handle domain-server restart
    do {
        DomainServer domainServer(argc, argv);
        currentExitCode = domainServer.exec();
    } while (currentExitCode == DomainServer::EXIT_CODE_REBOOT);

    qInfo() << "Quitting.";
    return currentExitCode;
}

