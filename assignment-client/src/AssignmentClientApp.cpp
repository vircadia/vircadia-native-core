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
#include <QThread>

#include <LogHandler.h>
#include <SharedUtil.h>
#include <HifiConfigVariantMap.h>
#include <ShutdownEventListener.h>

#include "Assignment.h"
#include "AssignmentClient.h"
#include "AssignmentClientMonitor.h"
#include "AssignmentClientApp.h"
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>


AssignmentClientApp::AssignmentClientApp(int argc, char* argv[]) :
    QCoreApplication(argc, argv)
{
#   ifndef WIN32
    setvbuf(stdout, NULL, _IOLBF, 0);
#   endif

    // setup a shutdown event listener to handle SIGTERM or WM_CLOSE for us
#   ifdef _WIN32
    installNativeEventFilter(&ShutdownEventListener::getInstance());
#   else
    ShutdownEventListener::getInstance();
#   endif

    // parse command-line
    QCommandLineParser parser;
    parser.setApplicationDescription("High Fidelity Assignment Client");
    parser.addHelpOption();

    const QCommandLineOption helpOption = parser.addHelpOption();

    const QCommandLineOption clientTypeOption(ASSIGNMENT_TYPE_OVERRIDE_OPTION,
                                              "run single assignment client of given type", "type");
    parser.addOption(clientTypeOption);

    const QCommandLineOption poolOption(ASSIGNMENT_POOL_OPTION, "set assignment pool", "pool-name");
    parser.addOption(poolOption);

    const QCommandLineOption portOption(ASSIGNMENT_CLIENT_LISTEN_PORT_OPTION,
                                        "UDP port for this assignment client (or monitor)", "port");
    parser.addOption(portOption);

    const QCommandLineOption walletDestinationOption(ASSIGNMENT_WALLET_DESTINATION_ID_OPTION,
                                                     "set wallet destination", "wallet-uuid");
    parser.addOption(walletDestinationOption);

    const QCommandLineOption assignmentServerHostnameOption(CUSTOM_ASSIGNMENT_SERVER_HOSTNAME_OPTION,
                                                            "set assignment-server hostname", "hostname");
    parser.addOption(assignmentServerHostnameOption);

    const QCommandLineOption assignmentServerPortOption(CUSTOM_ASSIGNMENT_SERVER_PORT_OPTION,
                                                        "set assignment-server port", "port");
    parser.addOption(assignmentServerPortOption);

    const QCommandLineOption numChildsOption(ASSIGNMENT_NUM_FORKS_OPTION, "number of children to fork", "child-count");
    parser.addOption(numChildsOption);

    const QCommandLineOption minChildsOption(ASSIGNMENT_MIN_FORKS_OPTION, "minimum number of children", "child-count");
    parser.addOption(minChildsOption);

    const QCommandLineOption maxChildsOption(ASSIGNMENT_MAX_FORKS_OPTION, "maximum number of children", "child-count");
    parser.addOption(maxChildsOption);

    const QCommandLineOption monitorPortOption(ASSIGNMENT_CLIENT_MONITOR_PORT_OPTION, "assignment-client monitor port", "port");
    parser.addOption(monitorPortOption);

    const QCommandLineOption httpStatusPortOption(ASSIGNMENT_HTTP_STATUS_PORT, "http status server port", "http-status-port");
    parser.addOption(httpStatusPortOption);

    const QCommandLineOption logDirectoryOption(ASSIGNMENT_LOG_DIRECTORY, "directory to store logs", "log-directory");
    parser.addOption(logDirectoryOption);

    if (!parser.parse(QCoreApplication::arguments())) {
        qCritical() << parser.errorText() << endl;
        parser.showHelp();
        Q_UNREACHABLE();
    }

    if (parser.isSet(helpOption)) {
        parser.showHelp();
        Q_UNREACHABLE();
    }

    const QVariantMap argumentVariantMap = HifiConfigVariantMap::mergeCLParametersWithJSONConfig(arguments());

    unsigned int numForks = 0;
    if (parser.isSet(numChildsOption)) {
        numForks = parser.value(numChildsOption).toInt();
    }

    unsigned int minForks = 0;
    if (parser.isSet(minChildsOption)) {
        minForks = parser.value(minChildsOption).toInt();
    }

    unsigned int maxForks = 0;
    if (parser.isSet(maxChildsOption)) {
        maxForks = parser.value(maxChildsOption).toInt();
    }

    unsigned short monitorPort = 0;
    if (parser.isSet(monitorPortOption)) {
        monitorPort = parser.value(monitorPortOption).toUShort();
    }

    if (!numForks && minForks) {
        // if the user specified --min but not -n, set -n to --min
        numForks = minForks;
    }

    quint16 httpStatusPort { 0 };
    if (parser.isSet(httpStatusPortOption)) {
        httpStatusPort = parser.value(httpStatusPortOption).toUShort();
    }

    QString logDirectory;

    if (parser.isSet(logDirectoryOption)) {
        logDirectory = parser.value(logDirectoryOption);
    }


    Assignment::Type requestAssignmentType = Assignment::AllTypes;
    if (argumentVariantMap.contains(ASSIGNMENT_TYPE_OVERRIDE_OPTION)) {
        requestAssignmentType = (Assignment::Type) argumentVariantMap.value(ASSIGNMENT_TYPE_OVERRIDE_OPTION).toInt();
    }
    if (parser.isSet(clientTypeOption)) {
        requestAssignmentType = (Assignment::Type) parser.value(clientTypeOption).toInt();
    }

    QString assignmentPool;
    // check for an assignment pool passed on the command line or in the config
    if (argumentVariantMap.contains(ASSIGNMENT_POOL_OPTION)) {
        assignmentPool = argumentVariantMap.value(ASSIGNMENT_POOL_OPTION).toString();
    }
    if (parser.isSet(poolOption)) {
        assignmentPool = parser.value(poolOption);
    }

    QUuid walletUUID;
    if (argumentVariantMap.contains(ASSIGNMENT_WALLET_DESTINATION_ID_OPTION)) {
        walletUUID = argumentVariantMap.value(ASSIGNMENT_WALLET_DESTINATION_ID_OPTION).toString();
    }
    if (parser.isSet(walletDestinationOption)) {
        walletUUID = parser.value(walletDestinationOption);
    }

    QString assignmentServerHostname;
    if (argumentVariantMap.contains(ASSIGNMENT_WALLET_DESTINATION_ID_OPTION)) {
        assignmentServerHostname = argumentVariantMap.value(CUSTOM_ASSIGNMENT_SERVER_HOSTNAME_OPTION).toString();
    }
    if (parser.isSet(assignmentServerHostnameOption)) {
        assignmentServerHostname = parser.value(assignmentServerHostnameOption);
    }

    // check for an overriden assignment server port
    quint16 assignmentServerPort = DEFAULT_DOMAIN_SERVER_PORT;
    if (argumentVariantMap.contains(ASSIGNMENT_WALLET_DESTINATION_ID_OPTION)) {
        assignmentServerPort = argumentVariantMap.value(CUSTOM_ASSIGNMENT_SERVER_PORT_OPTION).toUInt();
    }

    if (parser.isSet(assignmentServerPortOption)) {
        assignmentServerPort = parser.value(assignmentServerPortOption).toInt();
    }

    // check for an overidden listen port
    quint16 listenPort = 0;
    if (argumentVariantMap.contains(ASSIGNMENT_CLIENT_LISTEN_PORT_OPTION)) {
        listenPort = argumentVariantMap.value(ASSIGNMENT_CLIENT_LISTEN_PORT_OPTION).toUInt();
    }

    if (parser.isSet(numChildsOption)) {
        if (minForks && minForks > numForks) {
            qCritical() << "--min can't be more than -n";
            parser.showHelp();
            Q_UNREACHABLE();
        }
        if (maxForks && maxForks < numForks) {
            qCritical() << "--max can't be less than -n";
            parser.showHelp();
            Q_UNREACHABLE();
        }
    }

    QThread::currentThread()->setObjectName("main thread");

    DependencyManager::registerInheritance<LimitedNodeList, NodeList>();

    if (numForks || minForks || maxForks) {
        AssignmentClientMonitor* monitor =  new AssignmentClientMonitor(numForks, minForks, maxForks,
                                                                        requestAssignmentType, assignmentPool,
                                                                        listenPort, walletUUID, assignmentServerHostname,
                                                                        assignmentServerPort, httpStatusPort, logDirectory);
        monitor->setParent(this);
        connect(this, &QCoreApplication::aboutToQuit, monitor, &AssignmentClientMonitor::aboutToQuit);
    } else {
        AssignmentClient* client = new AssignmentClient(requestAssignmentType, assignmentPool, listenPort,
                                                        walletUUID, assignmentServerHostname,
                                                        assignmentServerPort, monitorPort);
        client->setParent(this);
        connect(this, &QCoreApplication::aboutToQuit, client, &AssignmentClient::aboutToQuit);
    }
}
