//
//  main.cpp
//  assignment-client
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <sys/time.h>

#include <Assignment.h>
#include <AudioMixer.h>
#include <NodeList.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>

const int ASSIGNMENT_REQUEST_INTERVAL_USECS = 1 * 1000 * 1000;

int main(int argc, const char* argv[]) { 
    
    // create a NodeList as an unassigned client
    NodeList* nodeList = NodeList::createInstance(NODE_TYPE_UNASSIGNED);
    nodeList->getNodeSocket()->setBlocking(false);
    
    timeval lastRequest = {};
    
    unsigned char packetData[MAX_PACKET_SIZE];
    ssize_t receivedBytes = 0;
    
    Assignment requestAssignment(Assignment::Request, Assignment::All);
    
    while (true) {
        if (usecTimestampNow() - usecTimestamp(&lastRequest) >= ASSIGNMENT_REQUEST_INTERVAL_USECS) {
            gettimeofday(&lastRequest, NULL);
            
            // send an assignment request to the Nodelist
            qDebug("Sending assignment request.\n");
            nodeList->sendAssignment(requestAssignment);
        }        
        
        while (nodeList->getNodeSocket()->receive(packetData, &receivedBytes)) {
            if (packetData[0] == PACKET_TYPE_CREATE_ASSIGNMENT && packetVersionMatch(packetData)) {
                Assignment::Type assignmentType = (Assignment::Type) *(packetData + numBytesForPacketHeader(packetData));
                
                qDebug() << "Received an assignment of type" << assignmentType << "\n";
                
                AudioMixer::run();
                
                // reset our NodeList by switching back to unassigned and clearing the list
                nodeList->setOwnerType(NODE_TYPE_UNASSIGNED);
                nodeList->clear();
            }
        }
    }
}