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
#include <arpa/inet.h>

const int PAIRING_SERVER_LISTEN_PORT = 52934;
const int MAX_PACKET_SIZE_BYTES = 1400;

struct PairableDevice {
    char identifier[64];
    char name[64];
    sockaddr_in sendingSocket;
    sockaddr_in localSocket;
};

int indexOfFirstOccurenceOfCharacter(char* haystack, char needle) {
    int currentIndex = 0;
    
    while (haystack[currentIndex] != '\0' && haystack[currentIndex] != needle) {
        currentIndex++;
    }
    
    return currentIndex;
}

int main(int argc, const char* argv[]) {
    UDPSocket serverSocket(PAIRING_SERVER_LISTEN_PORT);
    
    sockaddr_in senderSocket;
    char deviceData[MAX_PACKET_SIZE_BYTES] = {};
    ssize_t receivedBytes = 0;
    
    std::vector<PairableDevice> devices;
    
    while (true) {
        if (serverSocket.receive((sockaddr *)&senderSocket, &deviceData, &receivedBytes)) {
            
            if (deviceData[0] == 'A') {
                // this is a device reporting itself as available
                
                // create a new PairableDevice
                PairableDevice newDevice = {};
                
                int addressBytes[4];
                int socketPort = 0;
                
                int numMatches = sscanf(deviceData, "A %s %d.%d.%d.%d:%d %s",
                                        newDevice.identifier,
                                        &addressBytes[3],
                                        &addressBytes[2],
                                        &addressBytes[1],
                                        &addressBytes[0],
                                        &socketPort,
                                        newDevice.name);
                
                if (numMatches >= 6) {
                    // if we have fewer than 6 matches the packet wasn't properly formatted
                    
                    // setup the localSocket for the pairing device
                    newDevice.localSocket.sin_family = AF_INET;
                    newDevice.localSocket.sin_addr.s_addr = (addressBytes[3] |
                                                             addressBytes[2] << 8 |
                                                             addressBytes[1] << 16 |
                                                             addressBytes[0] << 24);
                    newDevice.localSocket.sin_port = socketPort;
                    
                    // store this device's sending socket so we can talk back to it
                    newDevice.sendingSocket = senderSocket;
                    
                    // push this new device into the vector
                    printf("Adding device %s (%s) to list\n", newDevice.identifier, newDevice.name);
                    devices.push_back(newDevice);
                }
            } else if (deviceData[0] == 'P') {
                // this is a client looking to pair with a device
                // send the most recent device this address so it can attempt to pair
                
                char requestorSocketString[INET_ADDRSTRLEN + 6] = {};
                char requestorAddress[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(senderSocket.sin_addr), requestorAddress, INET_ADDRSTRLEN);
                
                sprintf(requestorSocketString, "%s:%d", requestorAddress, ntohs(senderSocket.sin_port));
                
                PairableDevice lastDevice = devices[devices.size() - 1];
                serverSocket.send((sockaddr*) &lastDevice.sendingSocket, requestorSocketString, strlen(requestorSocketString));
            }
        }
    }
        
}