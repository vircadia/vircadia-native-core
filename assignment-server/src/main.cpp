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

#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <UDPSocket.h>

const int ASSIGNMENT_SERVER_LISTEN_PORT = 7007;
const int MAX_PACKET_SIZE_BYTES = 1400;

struct Assignment {
    char birdIndex;
};

int main(int argc, const char* argv[]) {
    
    std::queue<Assignment> assignmentQueue;
    
    sockaddr_in senderSocket;
    unsigned char senderData[MAX_PACKET_SIZE_BYTES] = {};
    ssize_t receivedBytes = 0;
    
    UDPSocket serverSocket(ASSIGNMENT_SERVER_LISTEN_PORT);
    
    int numHeaderBytes = numBytesForPacketHeader((unsigned char*) &PACKET_TYPE_SEND_ASSIGNMENT);
    unsigned char assignmentPacket[numHeaderBytes + sizeof(char)];
    populateTypeAndVersion(assignmentPacket, PACKET_TYPE_SEND_ASSIGNMENT);
    
    while (true) {
        if (serverSocket.receive((sockaddr*) &senderSocket, &senderData, &receivedBytes)) {
            
            int numHeaderBytes = numBytesForPacketHeader(senderData);
            
            if (senderData[0] == PACKET_TYPE_REQUEST_ASSIGNMENT) {                
                // grab the FI assignment in the queue, if it exists
                if (assignmentQueue.size() > 0) {
                    Assignment firstAssignment = assignmentQueue.front();
                    assignmentQueue.pop();
                    
                    printf("Pulled an assignment with index %d\n", firstAssignment.birdIndex);
                    
                    assignmentPacket[numHeaderBytes] = firstAssignment.birdIndex;
                    
                    // send the assignment
                    serverSocket.send((sockaddr*) &senderSocket, assignmentPacket, sizeof(assignmentPacket));
                }
            } else if (senderData[0] == PACKET_TYPE_SEND_ASSIGNMENT) {
                Assignment newAssignment;
                newAssignment.birdIndex = *(senderData + numHeaderBytes);
                
                printf("Created an assignment in the queue with %d\n", newAssignment.birdIndex);
                
                // add this assignment to the queue
                assignmentQueue.push(newAssignment);
            }
        }
    }    
}
