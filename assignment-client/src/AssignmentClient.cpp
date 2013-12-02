//
//  AssignmentClient.cpp
//  hifi
//
//  Created by Stephen Birarda on 11/25/2013.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

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
    _requestAssignment(Assignment::RequestCommand, requestAssignmentType, requestAssignmentPool)
{
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
}

void AssignmentClient::sendAssignmentRequest() {
    NodeList::getInstance()->sendAssignment(_requestAssignment);
}
