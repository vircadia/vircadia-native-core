//
//  AssignmentClientapp.cpp
//  assignment-client/src
//
//  Created by Seth Alves on 2/19/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QCommandLineParser>

#include <LogHandler.h>
#include <SharedUtil.h>

#include "Assignment.h"
#include "AssignmentClient.h"
#include "AssignmentClientMonitor.h"
#include "AssignmentClientApp.h"


AssignmentClientApp::AssignmentClientApp(int argc, char* argv[]) :
    QApplication(argc, argv)
{
#   ifndef WIN32
    setvbuf(stdout, NULL, _IOLBF, 0);
#   endif

    // use the verbose message handler in Logging
    qInstallMessageHandler(LogHandler::verboseMessageHandler);

    // parse command-line
    QCommandLineParser parser;
    parser.setApplicationDescription("High Fidelity Assignment Client");
    parser.addHelpOption();

    const QCommandLineOption helpOption = parser.addHelpOption();

    const QCommandLineOption numChildsOption("n", "number of children to fork", "child-count");
    parser.addOption(numChildsOption);

    const QCommandLineOption idOption("i", "assignment client id", "uuid");
    parser.addOption(idOption);

    if (!parser.parse(QCoreApplication::arguments())) {
        qCritical() << parser.errorText() << endl;
        parser.showHelp();
        Q_UNREACHABLE();
    }

    if (parser.isSet(helpOption)) {
        parser.showHelp();
        Q_UNREACHABLE();
    }

    if (parser.isSet(numChildsOption) && parser.isSet(idOption)) {
        qCritical() << "using both -i and -n doesn't make sense.";
        parser.showHelp();
        Q_UNREACHABLE();
    }

    unsigned int numForks = 0;
    if (parser.isSet(numChildsOption)) {
        numForks = parser.value(numChildsOption).toInt();
    }

    QUuid nodeUUID = QUuid::createUuid();
    if (parser.isSet(idOption)) {
        nodeUUID = QUuid(parser.value(idOption));
    }

    if (numForks) {
        AssignmentClientMonitor monitor(argc, argv, numForks);
        monitor.exec();
    } else {
        AssignmentClient client(argc, argv, nodeUUID);
        client.exec();
    }
}


AssignmentClientApp::~AssignmentClientApp() {
}
