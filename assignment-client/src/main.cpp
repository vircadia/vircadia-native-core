//
//  main.cpp
//  assignment-client
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <arpa/inet.h>
#include <sys/time.h>

#include <Assignment.h>
#include <AudioMixer.h>
#include <AvatarMixer.h>
#include <NodeList.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>

const long long ASSIGNMENT_REQUEST_INTERVAL_USECS = 1 * 1000 * 1000;


int main(int argc, const char* argv[]) {
    
    setvbuf(stdout, NULL, _IOLBF, 0);
    
    sockaddr_in customAssignmentSocket = {};
    
    // grab the overriden assignment-server hostname from argv, if it exists
    const char* customAssignmentServer = getCmdOption(argc, argv, "-a");
    if (customAssignmentServer) {
        customAssignmentSocket = socketForHostnameAndHostOrderPort(customAssignmentServer, ASSIGNMENT_SERVER_PORT);
    }
    
    const char* NUM_FORKS_PARAMETER = "-n";
    const char* numForksIncludingParentString = getCmdOption(argc, argv, NUM_FORKS_PARAMETER);
    
    if (numForksIncludingParentString) {
        int numForksIncludingParent = atoi(numForksIncludingParentString);
        qDebug() << "Starting" << numForksIncludingParent << "assignment clients.";
        
        int processID = 0;
        
        // fire off as many children as we need (this is one less than the parent since the parent will run as well)
        for (int i = 0; i < numForksIncludingParent - 1; i++) {
            processID = fork();
            
            if (processID == 0) {
                // this is one of the children, break so we don't start a fork bomb
                break;
            }
        }
    }
    
    // create a NodeList as an unassigned client
    NodeList* nodeList = NodeList::createInstance(NODE_TYPE_UNASSIGNED);
    
    // set the custom assignment socket if we have it
    if (customAssignmentSocket.sin_addr.s_addr != 0) {
        nodeList->setAssignmentServerSocket((sockaddr*) &customAssignmentSocket);
    }
    
    // change the timeout on the nodelist socket to be as often as we want to re-request
    nodeList->getNodeSocket()->setBlockingReceiveTimeoutInUsecs(ASSIGNMENT_REQUEST_INTERVAL_USECS);
    
    timeval lastRequest = {};
    
    unsigned char packetData[MAX_PACKET_SIZE];
    ssize_t receivedBytes = 0;
    
    // grab the assignment pool from argv, if it was passed
    const char* ASSIGNMENT_POOL_PARAMETER = "-p";
    const char* assignmentPool = getCmdOption(argc, argv, ASSIGNMENT_POOL_PARAMETER);

    // create a request assignment, accept all assignments, pass the desired pool (if it exists)
    Assignment requestAssignment(Assignment::Request, Assignment::All, assignmentPool);
    
    while (true) {
        if (usecTimestampNow() - usecTimestamp(&lastRequest) >= ASSIGNMENT_REQUEST_INTERVAL_USECS) {
            gettimeofday(&lastRequest, NULL);
            // if we're here we have no assignment, so send a request
            nodeList->sendAssignment(requestAssignment);
        }
        
        if (nodeList->getNodeSocket()->receive(packetData, &receivedBytes) &&
            packetData[0] == PACKET_TYPE_DEPLOY_ASSIGNMENT && packetVersionMatch(packetData)) {
            
            // construct the deployed assignment from the packet data
            Assignment deployedAssignment(packetData, receivedBytes);
            
            qDebug() << "Received an assignment - " << deployedAssignment << "\n";
            
            // switch our nodelist DOMAIN_IP to the ip receieved in the assignment
            if (deployedAssignment.getDomainSocket()->sa_family == AF_INET) {
                in_addr domainSocketAddr = ((sockaddr_in*) deployedAssignment.getDomainSocket())->sin_addr;
                nodeList->setDomainIP(inet_ntoa(domainSocketAddr));
                
                qDebug() << "Changed domain IP to " << inet_ntoa(domainSocketAddr);
            }
            
            if (deployedAssignment.getType() == Assignment::AudioMixer) {
                AudioMixer::run();
            } else {
                AvatarMixer::run();
            }
            
            qDebug() << "Assignment finished or never started - waiting for new assignment";
            
            // reset our NodeList by switching back to unassigned and clearing the list
            nodeList->setOwnerType(NODE_TYPE_UNASSIGNED);
            nodeList->clear();
        }
    }
}