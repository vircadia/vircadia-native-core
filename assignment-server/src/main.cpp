//
//  main.cpp
//  assignment-server
//
//  Created by Stephen Birarda on 7/1/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <arpa/inet.h>
#include <fstream>

#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <UDPSocket.h>


const int ASSIGNMENT_SERVER_LISTEN_PORT = 7007;
const int MAX_PACKET_SIZE_BYTES = 1400;

int main(int argc, const char* argv[]) {
    
    sockaddr_in senderSocket;
    char senderData[MAX_PACKET_SIZE_BYTES] = {};
    ssize_t receivedBytes = 0;
    
    UDPSocket serverSocket(ASSIGNMENT_SERVER_LISTEN_PORT);
    
    int numHeaderBytes = numBytesForPacketHeader((unsigned char*) &PACKET_TYPE_SEND_ASSIGNMENT);
    unsigned char assignmentPacket[numHeaderBytes + sizeof(char)];
    populateTypeAndVersion(assignmentPacket, PACKET_TYPE_SEND_ASSIGNMENT);
    
    int birdIndex = 0;
    
    while (true) {
        if (serverSocket.receive((sockaddr*) &senderSocket, &senderData, &receivedBytes)) {
            if (senderData[0] == PACKET_TYPE_REQUEST_ASSIGNMENT) {
                // for now just send back a dummy assignment packet
                
                // give this assignee the next bird index
                assignmentPacket[numHeaderBytes] = birdIndex++;
            
                // wrap back to 0 if the bird index is 6 now
                if (birdIndex == 6) {
                    birdIndex = 0;
                }
                
                // send the assignment
                serverSocket.send((sockaddr*) &senderSocket, assignmentPacket, sizeof(assignmentPacket));
            }
        }
    }    
}
