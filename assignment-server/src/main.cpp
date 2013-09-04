//
//  main.cpp
//  assignment-server
//
//  Created by Stephen Birarda on 7/1/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <arpa/inet.h>
#include <fstream>
#include <queue>

#include <QtCore/QString>

#include <Assignment.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <UDPSocket.h>

const int MAX_PACKET_SIZE_BYTES = 1400;

int main(int argc, const char* argv[]) {
    
    std::queue<Assignment> assignmentQueue;
    
    sockaddr_in senderSocket;
    unsigned char senderData[MAX_PACKET_SIZE_BYTES] = {};
    ssize_t receivedBytes = 0;
    
    UDPSocket serverSocket(ASSIGNMENT_SERVER_PORT);
    
    unsigned char assignmentPacket[MAX_PACKET_SIZE_BYTES];
    int numSendHeaderBytes = populateTypeAndVersion(assignmentPacket, PACKET_TYPE_DEPLOY_ASSIGNMENT);
    
    while (true) {
        if (serverSocket.receive((sockaddr*) &senderSocket, &senderData, &receivedBytes)) {
            if (senderData[0] == PACKET_TYPE_REQUEST_ASSIGNMENT) {
                // construct the requested assignment from the packet data
                Assignment requestAssignment(senderData, receivedBytes);
                
                
                // grab the first assignment in the queue, if it exists
                if (assignmentQueue.size() > 0) {
                    Assignment firstAssignment = assignmentQueue.front();
                    
                    bool eitherHasPool = (firstAssignment.getPool() || requestAssignment.getPool());
                    
                    // make sure there is a pool match for the created and requested assignment
                    // or that neither has a designated pool
                    if ((eitherHasPool && strcmp(firstAssignment.getPool(), requestAssignment.getPool()))
                        || !eitherHasPool) {
                        assignmentQueue.pop();
                        
                        int numAssignmentBytes = firstAssignment.packToBuffer(assignmentPacket + numSendHeaderBytes);
                        
                        // send the assignment
                        serverSocket.send((sockaddr*) &senderSocket, assignmentPacket, numSendHeaderBytes + numAssignmentBytes);
                    }
                }
            } else if (senderData[0] == PACKET_TYPE_CREATE_ASSIGNMENT && packetVersionMatch(senderData)) {
                // construct the create assignment from the packet data
                Assignment createdAssignment(senderData, receivedBytes);
                
                qDebug() << "Received an assignment:" << createdAssignment;
                
                // assignment server is on a public server
                // assume that the address we now have for the sender is the public address/port
                // and store that with the assignment so it can be given to the requestor later
                createdAssignment.setDomainSocket((sockaddr*) &senderSocket);
                
                // until we have a GUID setup just keep the latest assignment
                if (assignmentQueue.size() > 0) {
                    assignmentQueue.pop();
                }
                
                // add this assignment to the queue
                assignmentQueue.push(createdAssignment);
            }
        }
    }    
}
