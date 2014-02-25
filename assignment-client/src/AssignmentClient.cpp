//
//  AssignmentClient.cpp
//  hifi
//
//  Created by Stephen Birarda on 11/25/2013.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QProcess>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <AccountManager.h>
#include <Assignment.h>
#include <Logging.h>
#include <NodeList.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>

#include "AssignmentFactory.h"

#include "AssignmentClient.h"

const QString ASSIGNMENT_CLIENT_TARGET_NAME = "assignment-client";
const long long ASSIGNMENT_REQUEST_INTERVAL_MSECS = 1 * 1000;

int hifiSockAddrMeta = qRegisterMetaType<HifiSockAddr>("HifiSockAddr");

AssignmentClient::AssignmentClient(int &argc, char **argv) :
    QCoreApplication(argc, argv),
    _currentAssignment(NULL)
{
    setOrganizationName("High Fidelity");
    setOrganizationDomain("highfidelity.io");
    setApplicationName("assignment-client");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    
    // register meta type is required for queued invoke method on Assignment subclasses
    
    // set the logging target to the the CHILD_TARGET_NAME
    Logging::setTargetName(ASSIGNMENT_CLIENT_TARGET_NAME);
    
    const char ASSIGNMENT_TYPE_OVVERIDE_OPTION[] = "-t";
    const char* assignmentTypeString = getCmdOption(argc, (const char**)argv, ASSIGNMENT_TYPE_OVVERIDE_OPTION);
    
    Assignment::Type requestAssignmentType = Assignment::AllTypes;
    
    if (assignmentTypeString) {
        // the user is asking to only be assigned to a particular type of assignment
        // so set that as the ::overridenAssignmentType to be used in requests
        requestAssignmentType = (Assignment::Type) atoi(assignmentTypeString);
    }
    
    const char ASSIGNMENT_POOL_OPTION[] = "--pool";
    const char* requestAssignmentPool = getCmdOption(argc, (const char**) argv, ASSIGNMENT_POOL_OPTION);
    
    
    // setup our _requestAssignment member variable from the passed arguments
    _requestAssignment = Assignment(Assignment::RequestCommand, requestAssignmentType, requestAssignmentPool);
    
    // create a NodeList as an unassigned client
    NodeList* nodeList = NodeList::createInstance(NodeType::Unassigned);
    
    const char CUSTOM_ASSIGNMENT_SERVER_HOSTNAME_OPTION[] = "-a";
    const char CUSTOM_ASSIGNMENT_SERVER_PORT_OPTION[] = "-p";
    
    // grab the overriden assignment-server hostname from argv, if it exists
    const char* customAssignmentServerHostname = getCmdOption(argc, (const char**)argv, CUSTOM_ASSIGNMENT_SERVER_HOSTNAME_OPTION);
    const char* customAssignmentServerPortString = getCmdOption(argc,(const char**)argv, CUSTOM_ASSIGNMENT_SERVER_PORT_OPTION);
    
    HifiSockAddr customAssignmentSocket;
    
    if (customAssignmentServerHostname || customAssignmentServerPortString) {
        
        // set the custom port or default if it wasn't passed
        unsigned short assignmentServerPort = customAssignmentServerPortString
        ? atoi(customAssignmentServerPortString) : DEFAULT_DOMAIN_SERVER_PORT;
        
        // set the custom hostname or default if it wasn't passed
        if (!customAssignmentServerHostname) {
            customAssignmentServerHostname = DEFAULT_ASSIGNMENT_SERVER_HOSTNAME;
        }
        
        customAssignmentSocket = HifiSockAddr(customAssignmentServerHostname, assignmentServerPort);
    }
    
    // set the custom assignment socket if we have it
    if (!customAssignmentSocket.isNull()) {
        nodeList->setAssignmentServerSocket(customAssignmentSocket);
    }
    
    // call a timer function every ASSIGNMENT_REQUEST_INTERVAL_MSECS to ask for assignment, if required
    qDebug() << "Waiting for assignment -" << _requestAssignment;
    
    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(sendAssignmentRequest()));
    timer->start(ASSIGNMENT_REQUEST_INTERVAL_MSECS);
    
    // connect our readPendingDatagrams method to the readyRead() signal of the socket
    connect(&nodeList->getNodeSocket(), &QUdpSocket::readyRead, this, &AssignmentClient::readPendingDatagrams);
    
    // connections to AccountManager for authentication
    connect(&AccountManager::getInstance(), &AccountManager::authRequired,
            this, &AssignmentClient::handleAuthenticationRequest);
}

void AssignmentClient::sendAssignmentRequest() {
    if (!_currentAssignment) {
        NodeList::getInstance()->sendAssignment(_requestAssignment);
    }
}

void AssignmentClient::readPendingDatagrams() {
    NodeList* nodeList = NodeList::getInstance();
    
    QByteArray receivedPacket;
    HifiSockAddr senderSockAddr;
    
    while (nodeList->getNodeSocket().hasPendingDatagrams()) {
        receivedPacket.resize(nodeList->getNodeSocket().pendingDatagramSize());
        nodeList->getNodeSocket().readDatagram(receivedPacket.data(), receivedPacket.size(),
                                               senderSockAddr.getAddressPointer(), senderSockAddr.getPortPointer());
        
        if (nodeList->packetVersionAndHashMatch(receivedPacket)) {
            if (packetTypeForPacket(receivedPacket) == PacketTypeCreateAssignment) {
                // construct the deployed assignment from the packet data
                _currentAssignment = AssignmentFactory::unpackAssignment(receivedPacket);
                
                if (_currentAssignment) {
                    qDebug() << "Received an assignment -" << *_currentAssignment;
                    
                    // switch our nodelist domain IP and port to whoever sent us the assignment
                    
                    nodeList->getDomainInfo().setSockAddr(senderSockAddr);
                    nodeList->getDomainInfo().setAssignmentUUID(_currentAssignment->getUUID());
                    
                    qDebug() << "Destination IP for assignment is" << nodeList->getDomainInfo().getIP().toString();
                    
                    // start the deployed assignment
                    QThread* workerThread = new QThread(this);
                    
                    connect(workerThread, SIGNAL(started()), _currentAssignment, SLOT(run()));
                    
                    connect(_currentAssignment, SIGNAL(finished()), this, SLOT(assignmentCompleted()));
                    connect(_currentAssignment, SIGNAL(finished()), workerThread, SLOT(quit()));
                    connect(_currentAssignment, SIGNAL(finished()), _currentAssignment, SLOT(deleteLater()));
                    connect(workerThread, SIGNAL(finished()), workerThread, SLOT(deleteLater()));
                    
                    _currentAssignment->moveToThread(workerThread);
                    
                    // move the NodeList to the thread used for the _current assignment
                    nodeList->moveToThread(workerThread);
                    
                    // let the assignment handle the incoming datagrams for its duration
                    disconnect(&nodeList->getNodeSocket(), 0, this, 0);
                    connect(&nodeList->getNodeSocket(), &QUdpSocket::readyRead, _currentAssignment,
                            &ThreadedAssignment::readPendingDatagrams);
                    
                    // Starts an event loop, and emits workerThread->started()
                    workerThread->start();
                } else {
                    qDebug() << "Received an assignment that could not be unpacked. Re-requesting.";
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
    // reset the logging target to the the CHILD_TARGET_NAME
    Logging::setTargetName(ASSIGNMENT_CLIENT_TARGET_NAME);
    
    qDebug("Assignment finished or never started - waiting for new assignment.");
    
    NodeList* nodeList = NodeList::getInstance();
    
    // have us handle incoming NodeList datagrams again
    disconnect(&nodeList->getNodeSocket(), 0, _currentAssignment, 0);
    connect(&nodeList->getNodeSocket(), &QUdpSocket::readyRead, this, &AssignmentClient::readPendingDatagrams);
    
    _currentAssignment = NULL;
    
    // reset our NodeList by switching back to unassigned and clearing the list
    nodeList->setOwnerType(NodeType::Unassigned);
    nodeList->reset();
    nodeList->resetNodeInterestSet();
}
