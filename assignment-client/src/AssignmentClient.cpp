//
//  AssignmentClient.cpp
//  hifi
//
//  Created by Stephen Birarda on 11/25/2013.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <Assignment.h>
#include <Logging.h>
#include <NodeList.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>

#include "AssignmentFactory.h"

#include "AssignmentClient.h"

const char ASSIGNMENT_CLIENT_TARGET_NAME[] = "assignment-client";
const long long ASSIGNMENT_REQUEST_INTERVAL_MSECS = 1 * 1000;

int hifiSockAddrMeta = qRegisterMetaType<HifiSockAddr>("HifiSockAddr");

AssignmentClient::AssignmentClient(int &argc, char **argv) :
    QCoreApplication(argc, argv),
    _currentAssignment(NULL)
{
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
    NodeList* nodeList = NodeList::createInstance(NODE_TYPE_UNASSIGNED);
    
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
    qDebug() << "Waiting for assignment -" << _requestAssignment << "\n";
    
    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(sendAssignmentRequest()));
    timer->start(ASSIGNMENT_REQUEST_INTERVAL_MSECS);
    
    // connect our readPendingDatagrams method to the readyRead() signal of the socket
    connect(&nodeList->getNodeSocket(), SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
}

void AssignmentClient::sendAssignmentRequest() {
    if (!_currentAssignment) {
        NodeList::getInstance()->sendAssignment(_requestAssignment);
    }
}

void AssignmentClient::readPendingDatagrams() {
    NodeList* nodeList = NodeList::getInstance();
    
    static unsigned char packetData[1500];
    static qint64 receivedBytes = 0;
    static HifiSockAddr senderSockAddr;
    
    while (nodeList->getNodeSocket().hasPendingDatagrams()) {
        
        if ((receivedBytes = nodeList->getNodeSocket().readDatagram((char*) packetData, MAX_PACKET_SIZE,
                                                                    senderSockAddr.getAddressPointer(),
                                                                    senderSockAddr.getPortPointer()))
            && packetVersionMatch(packetData)) {
            
            if (_currentAssignment) {
                // have the threaded current assignment handle this datagram
                QMetaObject::invokeMethod(_currentAssignment, "processDatagram", Qt::QueuedConnection,
                                          Q_ARG(QByteArray, QByteArray((char*) packetData, receivedBytes)),
                                          Q_ARG(HifiSockAddr, senderSockAddr));
            } else if (packetData[0] == PACKET_TYPE_DEPLOY_ASSIGNMENT || packetData[0] == PACKET_TYPE_CREATE_ASSIGNMENT) {
                
                if (_currentAssignment) {
                    qDebug() << "Dropping received assignment since we are currently running one.\n";
                } else {
                    // construct the deployed assignment from the packet data
                    _currentAssignment = AssignmentFactory::unpackAssignment(packetData, receivedBytes);
                    
                    qDebug() << "Received an assignment -" << *_currentAssignment << "\n";
                    
                    // switch our nodelist domain IP and port to whoever sent us the assignment
                    if (packetData[0] == PACKET_TYPE_CREATE_ASSIGNMENT) {
                        nodeList->setDomainSockAddr(senderSockAddr);
                        nodeList->setOwnerUUID(_currentAssignment->getUUID());
                        
                        qDebug("Destination IP for assignment is %s\n",
                               nodeList->getDomainIP().toString().toStdString().c_str());
                        
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
                        
                        // Starts an event loop, and emits workerThread->started()
                        workerThread->start();
                    } else {
                        qDebug("Received a bad destination socket for assignment.\n");
                    }
                }
            } else {
                // have the NodeList attempt to handle it
                nodeList->processNodeData(senderSockAddr, packetData, receivedBytes);
            }
        }
    }
}

void AssignmentClient::assignmentCompleted() {
    // reset the logging target to the the CHILD_TARGET_NAME
    Logging::setTargetName(ASSIGNMENT_CLIENT_TARGET_NAME);
    
    qDebug("Assignment finished or never started - waiting for new assignment\n");
    
    _currentAssignment = NULL;
    
    NodeList* nodeList = NodeList::getInstance();
    
    // move the NodeList back to our thread
    nodeList->moveToThread(thread());
    
    // reset our NodeList by switching back to unassigned and clearing the list
    nodeList->setOwnerType(NODE_TYPE_UNASSIGNED);
    nodeList->reset();
}
