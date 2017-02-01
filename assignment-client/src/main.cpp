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

#include "AssignmentClientApp.h"
#include <BuildInfo.h>

int main(int argc, char* argv[]) {
    disableQtBearerPoll(); // Fixes wifi ping spikes

    QCoreApplication::setApplicationName(BuildInfo::ASSIGNMENT_CLIENT_NAME);
    QCoreApplication::setOrganizationName(BuildInfo::MODIFIED_ORGANIZATION);
    QCoreApplication::setOrganizationDomain(BuildInfo::ORGANIZATION_DOMAIN);
    QCoreApplication::setApplicationVersion(BuildInfo::VERSION);

    qInstallMessageHandler(LogHandler::verboseMessageHandler);
    qInfo() << "Starting.";

    AssignmentClientApp app(argc, argv);
    
    int acReturn = app.exec();
    qDebug() << "assignment-client process" <<  app.applicationPid() << "exiting with status code" << acReturn;

    qInfo() << "Quitting.";
    return acReturn;
}
