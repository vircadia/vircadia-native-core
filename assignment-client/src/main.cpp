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
    
    // create a NodeList as an unassigned client
    NodeList* nodeList = NodeList::createInstance(NODE_TYPE_UNASSIGNED);
    
    // change the timeout on the nodelist socket to be as often as we want to re-request
    nodeList->getNodeSocket()->setBlockingReceiveTimeoutInUsecs(ASSIGNMENT_REQUEST_INTERVAL_USECS);
    
    timeval lastRequest = {};
    
    unsigned char packetData[MAX_PACKET_SIZE];
    ssize_t receivedBytes = 0;
    
    // grab the assignment pool from argv, if it was passed
    const char* assignmentPool = getCmdOption(argc, argv, "-p");
    
    // create a request assignment, accept all assignments, pass the desired pool (if it exists)
    Assignment requestAssignment(Assignment::Request, Assignment::All, assignmentPool);
    
    while (true) {
        if (usecTimestampNow() - usecTimestamp(&lastRequest) >= ASSIGNMENT_REQUEST_INTERVAL_USECS) {
            gettimeofday(&lastRequest, NULL);
            // if we're here we have no assignment, so send a request
            qDebug() << "Sending an assignment request -" << requestAssignment;
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
                qDebug() << "Running as an avatar mixer!";
                AvatarMixer::run();
            }
            
            qDebug() << "Assignment finished or never started - waiting for new assignment";
            
            // reset our NodeList by switching back to unassigned and clearing the list
            nodeList->setOwnerType(NODE_TYPE_UNASSIGNED);
            nodeList->clear();
        }
    }
}