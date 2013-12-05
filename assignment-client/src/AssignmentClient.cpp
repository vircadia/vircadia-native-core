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

AssignmentClient::AssignmentClient(int &argc, char **argv,
                                   Assignment::Type requestAssignmentType,
                                   const HifiSockAddr& customAssignmentServerSocket,
                                   const char* requestAssignmentPool) :
    QCoreApplication(argc, argv),
    _requestAssignment(Assignment::RequestCommand, requestAssignmentType, requestAssignmentPool),
    _currentAssignment(NULL)
{
    // register meta type is required for queued invoke method on Assignment subclasses
    qRegisterMetaType<HifiSockAddr>("HifiSockAddr");
    
    // set the logging target to the the CHILD_TARGET_NAME
    Logging::setTargetName(ASSIGNMENT_CLIENT_TARGET_NAME);
    
    // create a NodeList as an unassigned client
    NodeList* nodeList = NodeList::createInstance(NODE_TYPE_UNASSIGNED);
    
    // set the custom assignment socket if we have it
    if (!customAssignmentServerSocket.isNull()) {
        nodeList->setAssignmentServerSocket(customAssignmentServerSocket);
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
    
    // reset our NodeList by switching back to unassigned and clearing the list
    nodeList->setOwnerType(NODE_TYPE_UNASSIGNED);
    nodeList->reset();
}
