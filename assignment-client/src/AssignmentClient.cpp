//
//  AssignmentClient.cpp
//  hifi
//
//  Created by Stephen Birarda on 11/25/2013.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <Assignment.h>
#include <Logging.h>
#include <NodeList.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>

#include "AssignmentFactory.h"

#include "AssignmentClient.h"

const char ASSIGNMENT_CLIENT_TARGET_NAME[] = "assignment-client";
const long long ASSIGNMENT_REQUEST_INTERVAL_USECS = 1 * 1000 * 1000;

AssignmentClient::AssignmentClient(int &argc, char **argv,
                                   Assignment::Type requestAssignmentType,
                                   const sockaddr_in& customAssignmentServerSocket,
                                   const char* requestAssignmentPool) :
    QCoreApplication(argc, argv),
    _requestAssignmentType(requestAssignmentType),
    _customAssignmentServerSocket(customAssignmentServerSocket),
    _requestAssignmentPool(requestAssignmentPool)
{
    
}

int AssignmentClient::exec() {
    // set the logging target to the the CHILD_TARGET_NAME
    Logging::setTargetName(ASSIGNMENT_CLIENT_TARGET_NAME);
    
    // create a NodeList as an unassigned client
    NodeList* nodeList = NodeList::createInstance(NODE_TYPE_UNASSIGNED);
    
    // set the custom assignment socket if we have it
    if (_customAssignmentServerSocket.sin_addr.s_addr != 0) {
        nodeList->setAssignmentServerSocket((sockaddr*) &_customAssignmentServerSocket);
    }
    
    // change the timeout on the nodelist socket to be as often as we want to re-request
    nodeList->getNodeSocket()->setBlockingReceiveTimeoutInUsecs(ASSIGNMENT_REQUEST_INTERVAL_USECS);
    
    timeval lastRequest = {};
    
    unsigned char packetData[MAX_PACKET_SIZE];
    ssize_t receivedBytes = 0;
    
    sockaddr_in senderSocket = {};
    
    // create a request assignment, accept assignments defined by the overidden type
    Assignment requestAssignment(Assignment::RequestCommand, _requestAssignmentType, _requestAssignmentPool);
    
    qDebug() << "Waiting for assignment -" << requestAssignment << "\n";
    
    while (true) {
        if (usecTimestampNow() - usecTimestamp(&lastRequest) >= ASSIGNMENT_REQUEST_INTERVAL_USECS) {
            gettimeofday(&lastRequest, NULL);
            
            // if we're here we have no assignment, so send a request
            nodeList->sendAssignment(requestAssignment);
        }
        
        if (nodeList->getNodeSocket()->receive((sockaddr*) &senderSocket, packetData, &receivedBytes) &&
            (packetData[0] == PACKET_TYPE_DEPLOY_ASSIGNMENT || packetData[0] == PACKET_TYPE_CREATE_ASSIGNMENT)
            && packetVersionMatch(packetData)) {
            
            // construct the deployed assignment from the packet data
            Assignment* deployedAssignment = AssignmentFactory::unpackAssignment(packetData, receivedBytes);
            
            qDebug() << "Received an assignment -" << *deployedAssignment << "\n";
            
            // switch our nodelist domain IP and port to whoever sent us the assignment
            if (packetData[0] == PACKET_TYPE_CREATE_ASSIGNMENT) {
                
                nodeList->setDomainIP(QHostAddress((sockaddr*) &senderSocket));
                nodeList->setDomainPort(ntohs(senderSocket.sin_port));
                
                nodeList->setOwnerUUID(deployedAssignment->getUUID());
                
                qDebug("Destination IP for assignment is %s\n", nodeList->getDomainIP().toString().toStdString().c_str());
                
                // run the deployed assignment
                deployedAssignment->run();
            } else {
                qDebug("Received a bad destination socket for assignment.\n");
            }
            
            qDebug("Assignment finished or never started - waiting for new assignment\n");
            
            // delete the deployedAssignment
            delete deployedAssignment;
            
            // reset our NodeList by switching back to unassigned and clearing the list
            nodeList->setOwnerType(NODE_TYPE_UNASSIGNED);
            nodeList->reset();
            
            // set the NodeList socket back to blocking
            nodeList->getNodeSocket()->setBlocking(true);
            
            // reset the logging target to the the CHILD_TARGET_NAME
            Logging::setTargetName(ASSIGNMENT_CLIENT_TARGET_NAME);
        }
    }
    
    return 0;
}
