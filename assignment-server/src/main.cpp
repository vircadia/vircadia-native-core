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
    int numSendHeaderBytes = populateTypeAndVersion(assignmentPacket, PACKET_TYPE_CREATE_ASSIGNMENT);
    
    while (true) {
        if (serverSocket.receive((sockaddr*) &senderSocket, &senderData, &receivedBytes)) {
            if (senderData[0] == PACKET_TYPE_REQUEST_ASSIGNMENT) {
                qDebug() << "Assignment request received.\n";
                
                // grab the first assignment in the queue, if it exists
                if (assignmentQueue.size() > 0) {
                    Assignment firstAssignment = assignmentQueue.front();
                    assignmentQueue.pop();
                    
                    *(assignmentPacket + numSendHeaderBytes) = firstAssignment.getType();
                    
                    
                    // send the assignment
                    serverSocket.send((sockaddr*) &senderSocket, assignmentPacket, numSendHeaderBytes + sizeof(unsigned char));
                }
            } else if (senderData[0] == PACKET_TYPE_CREATE_ASSIGNMENT && packetVersionMatch(senderData)) {
                // assignment server is on a public server
                // assume that the address we now have for the sender is the public address/port
                // and store that with the assignment so it can be given to the requestor later
//                Assignment newAssignment((Assignment::Type) *(senderData + numBytesForPacketHeader(senderData)), senderSocket);
                
//                qDebug() << "Received assignment of type " << newAssignment.getType() << "\n";
                
                // add this assignment to the queue
//                assignmentQueue.push(newAssignment);
            }
        }
    }    
}
