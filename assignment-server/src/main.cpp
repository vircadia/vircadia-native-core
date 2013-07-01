//
//  main.cpp
//  assignment-server
//
//  Created by Stephen Birarda on 7/1/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <arpa/inet.h>

#include <PacketHeaders.h>
#include <UDPSocket.h>

const int ASSIGNMENT_SERVER_LISTEN_PORT = 7007;
const int MAX_PACKET_SIZE_BYTES = 1400;

int main(int argc, const char* argv[]) {
    
    sockaddr_in senderSocket;
    char senderData[MAX_PACKET_SIZE_BYTES] = {};
    ssize_t receivedBytes = 0;
    
    UDPSocket serverSocket(ASSIGNMENT_SERVER_LISTEN_PORT);
    
    while (true) {
        if (serverSocket.receive((sockaddr*) &senderSocket, &senderData, &receivedBytes)) {
            if (senderData[0] == PACKET_HEADER_REQUEST_ASSIGNMENT) {
                // for now just send back a dummy assignment packet
                serverSocket.send((sockaddr*) &senderSocket, &PACKET_HEADER_SEND_ASSIGNMENT, 1);
            }
        }
    }    
}
