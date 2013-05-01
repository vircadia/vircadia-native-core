//
//  main.cpp
//  pairing-server
//
//  Created by Stephen Birarda on 5/1/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <iostream>
#include <vector>

#include <UDPSocket.h>

const int PAIRING_SERVER_LISTEN_PORT = 52934;
const int MAX_PACKET_SIZE_BYTES = 1400;

struct PairableDevice {
    char identifier[64];
    char name[64];
    sockaddr localSocket;
};

int main(int argc, const char* argv[]) {
    UDPSocket serverSocket(PAIRING_SERVER_LISTEN_PORT);
    
    sockaddr deviceAddress;
    char deviceData[MAX_PACKET_SIZE_BYTES] = {};
    ssize_t receivedBytes = 0;
    
    std::vector<PairableDevice> devices;
    
    while (true) {
        if (serverSocket.receive(&deviceAddress, &deviceData, &receivedBytes)) {
            
        }
    }
}


