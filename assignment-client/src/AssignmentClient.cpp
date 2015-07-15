//
//  AssignmentClient.cpp
//  assignment-client/src
//
//  Created by Stephen Birarda on 11/25/2013.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <assert.h>

#include <QProcess>
#include <QSettings>
#include <QSharedMemory>
#include <QThread>
#include <QTimer>

#include <AccountManager.h>
#include <AddressManager.h>
#include <Assignment.h>
#include <AvatarHashMap.h>
#include <EntityScriptingInterface.h>
#include <LogHandler.h>
#include <LogUtils.h>
#include <LimitedNodeList.h>
#include <NodeList.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <ShutdownEventListener.h>
#include <SoundCache.h>

#include "AssignmentFactory.h"
#include "AssignmentActionFactory.h"

#include "AssignmentClient.h"

const QString ASSIGNMENT_CLIENT_TARGET_NAME = "assignment-client";
const long long ASSIGNMENT_REQUEST_INTERVAL_MSECS = 1 * 1000;

int hifiSockAddrMeta = qRegisterMetaType<HifiSockAddr>("HifiSockAddr");

AssignmentClient::AssignmentClient(Assignment::Type requestAssignmentType, QString assignmentPool,
                                   QUuid walletUUID, QString assignmentServerHostname, quint16 assignmentServerPort,
                                   quint16 assignmentMonitorPort) :
    _assignmentServerHostname(DEFAULT_ASSIGNMENT_SERVER_HOSTNAME)
{
    LogUtils::init();

    QSettings::setDefaultFormat(QSettings::IniFormat);

    // create a NodeList as an unassigned client
    DependencyManager::registerInheritance<LimitedNodeList, NodeList>();
    auto addressManager = DependencyManager::set<AddressManager>();
    auto nodeList = DependencyManager::set<NodeList>(NodeType::Unassigned); // Order is important

    auto animationCache = DependencyManager::set<AnimationCache>();
    auto avatarHashMap = DependencyManager::set<AvatarHashMap>();
    auto entityScriptingInterface = DependencyManager::set<EntityScriptingInterface>();

    DependencyManager::registerInheritance<EntityActionFactoryInterface, AssignmentActionFactory>();
    auto actionFactory = DependencyManager::set<AssignmentActionFactory>();

    // make up a uuid for this child so the parent can tell us apart.  This id will be changed
    // when the domain server hands over an assignment.
    QUuid nodeUUID = QUuid::createUuid();
    nodeList->setSessionUUID(nodeUUID);

    // set the logging target to the the CHILD_TARGET_NAME
    LogHandler::getInstance().setTargetName(ASSIGNMENT_CLIENT_TARGET_NAME);

    // make sure we output process IDs for a child AC otherwise it's insane to parse
    LogHandler::getInstance().setShouldOutputPID(true);

    // setup our _requestAssignment member variable from the passed arguments
    _requestAssignment = Assignment(Assignment::RequestCommand, requestAssignmentType, assignmentPool);

    // check for a wallet UUID on the command line or in the config
    // this would represent where the user running AC wants funds sent to
    if (!walletUUID.isNull()) {
        qDebug() << "The destination wallet UUID for credits is" << uuidStringWithoutCurlyBraces(walletUUID);
        _requestAssignment.setWalletUUID(walletUUID);
    }

    // check for an overriden assignment server hostname
    if (assignmentServerHostname != "") {
        // change the hostname for our assignment server
        _assignmentServerHostname = assignmentServerHostname;
    }

    _assignmentServerSocket = HifiSockAddr(_assignmentServerHostname, assignmentServerPort, true);
    nodeList->setAssignmentServerSocket(_assignmentServerSocket);

    qDebug() << "Assignment server socket is" << _assignmentServerSocket;

    // call a timer function every ASSIGNMENT_REQUEST_INTERVAL_MSECS to ask for assignment, if required
    qDebug() << "Waiting for assignment -" << _requestAssignment;

    if (_assignmentServerHostname != "localhost") {
        qDebug () << "- will attempt to connect to domain-server on" << _assignmentServerSocket.getPort();
    }

    connect(&_requestTimer, SIGNAL(timeout()), SLOT(sendAssignmentRequest()));
    _requestTimer.start(ASSIGNMENT_REQUEST_INTERVAL_MSECS);

    // connect our readPendingDatagrams method to the readyRead() signal of the socket
    connect(&nodeList->getNodeSocket(), &QUdpSocket::readyRead, this, &AssignmentClient::readPendingDatagrams);

    // connections to AccountManager for authentication
    connect(&AccountManager::getInstance(), &AccountManager::authRequired,
            this, &AssignmentClient::handleAuthenticationRequest);

    // Create Singleton objects on main thread
    NetworkAccessManager::getInstance();

    // did we get an assignment-client monitor port?
    if (assignmentMonitorPort > 0) {
        _assignmentClientMonitorSocket = HifiSockAddr(DEFAULT_ASSIGNMENT_CLIENT_MONITOR_HOSTNAME, assignmentMonitorPort);

        qDebug() << "Assignment-client monitor socket is" << _assignmentClientMonitorSocket;

        // Hook up a timer to send this child's status to the Monitor once per second
        setUpStatsToMonitor();
    }
}


void AssignmentClient::stopAssignmentClient() {
    qDebug() << "Forced stop of assignment-client.";

    _requestTimer.stop();
    _statsTimerACM.stop();

    if (_currentAssignment) {
        // grab the thread for the current assignment
        QThread* currentAssignmentThread = _currentAssignment->thread();

        // ask the current assignment to stop
        QMetaObject::invokeMethod(_currentAssignment, "stop", Qt::BlockingQueuedConnection);

        // ask the current assignment to delete itself on its thread
        _currentAssignment->deleteLater();

        // when this thread is destroyed we don't need to run our assignment complete method
        disconnect(currentAssignmentThread, &QThread::destroyed, this, &AssignmentClient::assignmentCompleted);

        // wait on the thread from that assignment - it will be gone once the current assignment deletes
        currentAssignmentThread->quit();
        currentAssignmentThread->wait();
    }
}


void AssignmentClient::aboutToQuit() {
    stopAssignmentClient();

    // clear the log handler so that Qt doesn't call the destructor on LogHandler
    qInstallMessageHandler(0);
}


void AssignmentClient::setUpStatsToMonitor() {
    // send a stats packet every 1 seconds
    connect(&_statsTimerACM, &QTimer::timeout, this, &AssignmentClient::sendStatsPacketToACM);
    _statsTimerACM.start(1000);
}

void AssignmentClient::sendStatsPacketToACM() {
    // tell the assignment client monitor what this assignment client is doing (if anything)
    QJsonObject statsObject;
    auto nodeList = DependencyManager::get<NodeList>();

    if (_currentAssignment) {
        statsObject["assignment_type"] = _currentAssignment->getTypeName();
    } else {
        statsObject["assignment_type"] = "none";
    }
    nodeList->sendStats(statsObject, _assignmentClientMonitorSocket);
}

void AssignmentClient::sendAssignmentRequest() {
    if (!_currentAssignment) {

        auto nodeList = DependencyManager::get<NodeList>();

        if (_assignmentServerHostname == "localhost") {
            // we want to check again for the local domain-server port in case the DS has restarted
            quint16 localAssignmentServerPort;
            if (nodeList->getLocalServerPortFromSharedMemory(DOMAIN_SERVER_LOCAL_PORT_SMEM_KEY, localAssignmentServerPort)) {
                if (localAssignmentServerPort != _assignmentServerSocket.getPort()) {
                    qDebug() << "Port for local assignment server read from shared memory is"
                        << localAssignmentServerPort;

                    _assignmentServerSocket.setPort(localAssignmentServerPort);
                    nodeList->setAssignmentServerSocket(_assignmentServerSocket);
                }
            } else {
                qDebug() << "Failed to read local assignment server port from shared memory"
                    << "- will send assignment request to previous assignment server socket.";
            }

        }

        nodeList->sendAssignment(_requestAssignment);
    }
}

void AssignmentClient::readPendingDatagrams() {
    auto nodeList = DependencyManager::get<NodeList>();

    QByteArray receivedPacket;
    HifiSockAddr senderSockAddr;

    while (nodeList->getNodeSocket().hasPendingDatagrams()) {
        receivedPacket.resize(nodeList->getNodeSocket().pendingDatagramSize());
        nodeList->getNodeSocket().readDatagram(receivedPacket.data(), receivedPacket.size(),
                                               senderSockAddr.getAddressPointer(), senderSockAddr.getPortPointer());

        if (nodeList->packetVersionAndHashMatch(receivedPacket)) {
            if (packetTypeForPacket(receivedPacket) == PacketTypeCreateAssignment) {

                qDebug() << "Received a PacketTypeCreateAssignment - attempting to unpack.";

                // construct the deployed assignment from the packet data
                _currentAssignment = AssignmentFactory::unpackAssignment(receivedPacket);

                if (_currentAssignment) {
                    qDebug() << "Received an assignment -" << *_currentAssignment;

                    // switch our DomainHandler hostname and port to whoever sent us the assignment

                    nodeList->getDomainHandler().setSockAddr(senderSockAddr, _assignmentServerHostname);
                    nodeList->getDomainHandler().setAssignmentUUID(_currentAssignment->getUUID());

                    qDebug() << "Destination IP for assignment is" << nodeList->getDomainHandler().getIP().toString();

                    // start the deployed assignment
                    QThread* workerThread = new QThread;
                    workerThread->setObjectName("ThreadedAssignment Worker");

                    connect(workerThread, &QThread::started, _currentAssignment.data(), &ThreadedAssignment::run);

                    // Once the ThreadedAssignment says it is finished - we ask it to deleteLater
                    // This is a queued connection so that it is put into the event loop to be processed by the worker
                    // thread when it is ready.
                    connect(_currentAssignment.data(), &ThreadedAssignment::finished, _currentAssignment.data(),
                            &ThreadedAssignment::deleteLater, Qt::QueuedConnection);

                    // once it is deleted, we quit the worker thread
                    connect(_currentAssignment.data(), &ThreadedAssignment::destroyed, workerThread, &QThread::quit);

                    // have the worker thread remove itself once it is done
                    connect(workerThread, &QThread::finished, workerThread, &QThread::deleteLater);

                    // once the worker thread says it is done, we consider the assignment completed
                    connect(workerThread, &QThread::destroyed, this, &AssignmentClient::assignmentCompleted);

                    _currentAssignment->moveToThread(workerThread);

                    // move the NodeList to the thread used for the _current assignment
                    nodeList->moveToThread(workerThread);

                    // let the assignment handle the incoming datagrams for its duration
                    disconnect(&nodeList->getNodeSocket(), 0, this, 0);
                    connect(&nodeList->getNodeSocket(), &QUdpSocket::readyRead, _currentAssignment.data(),
                            &ThreadedAssignment::readPendingDatagrams);

                    // Starts an event loop, and emits workerThread->started()
                    workerThread->start();
                } else {
                    qDebug() << "Received an assignment that could not be unpacked. Re-requesting.";
                }
            } else if (packetTypeForPacket(receivedPacket) == PacketTypeStopNode) {
                if (senderSockAddr.getAddress() == QHostAddress::LocalHost ||
                    senderSockAddr.getAddress() == QHostAddress::LocalHostIPv6) {
                    qDebug() << "AssignmentClientMonitor at" << senderSockAddr << "requested stop via PacketTypeStopNode.";

                    QCoreApplication::quit();
                } else {
                    qDebug() << "Got a stop packet from other than localhost.";
                }
            } else {
                // have the NodeList attempt to handle it
                nodeList->processNodeData(senderSockAddr, receivedPacket);
            }
        }
    }
}

void AssignmentClient::handleAuthenticationRequest() {
    const QString DATA_SERVER_USERNAME_ENV = "HIFI_AC_USERNAME";
    const QString DATA_SERVER_PASSWORD_ENV = "HIFI_AC_PASSWORD";

    // this node will be using an authentication server, let's make sure we have a username/password
    QProcessEnvironment sysEnvironment = QProcessEnvironment::systemEnvironment();

    QString username = sysEnvironment.value(DATA_SERVER_USERNAME_ENV);
    QString password = sysEnvironment.value(DATA_SERVER_PASSWORD_ENV);

    AccountManager& accountManager = AccountManager::getInstance();

    if (!username.isEmpty() && !password.isEmpty()) {
        // ask the account manager to log us in from the env variables
        accountManager.requestAccessToken(username, password);
    } else {
        qDebug() << "Authentication was requested against" << qPrintable(accountManager.getAuthURL().toString())
            << "but both or one of" << qPrintable(DATA_SERVER_USERNAME_ENV)
            << "/" << qPrintable(DATA_SERVER_PASSWORD_ENV) << "are not set. Unable to authenticate.";

        return;
    }
}

void AssignmentClient::assignmentCompleted() {

    // we expect that to be here the previous assignment has completely cleaned up
    assert(_currentAssignment.isNull());

    // reset our current assignment pointer to NULL now that it has been deleted
    _currentAssignment = NULL;

    // reset the logging target to the the CHILD_TARGET_NAME
    LogHandler::getInstance().setTargetName(ASSIGNMENT_CLIENT_TARGET_NAME);

    qDebug() << "Assignment finished or never started - waiting for new assignment.";

    auto nodeList = DependencyManager::get<NodeList>();

    // have us handle incoming NodeList datagrams again, and make sure our ThreadedAssignment isn't handling them
    connect(&nodeList->getNodeSocket(), &QUdpSocket::readyRead, this, &AssignmentClient::readPendingDatagrams);

    // reset our NodeList by switching back to unassigned and clearing the list
    nodeList->setOwnerType(NodeType::Unassigned);
    nodeList->reset();
    nodeList->resetNodeInterestSet();
}
