//
//  main.cpp
//  pairing-server
//
//  Created by Stephen Birarda on 5/1/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <iostream>
#include <vector>
#include <cstdio>
#include <cstring>

#include <UDPSocket.h>
#include <arpa/inet.h>

const int PAIRING_SERVER_LISTEN_PORT = 7247;
const int MAX_PACKET_SIZE_BYTES = 1400;

struct PairableDevice {
    char identifier[64];
    char name[64];
    sockaddr_in sendingSocket;
    sockaddr_in localSocket;
};

struct RequestingClient {
    char address[INET_ADDRSTRLEN];
    int port;
};

UDPSocket serverSocket(PAIRING_SERVER_LISTEN_PORT);
PairableDevice *lastDevice = NULL;
RequestingClient *lastClient = NULL;

int indexOfFirstOccurenceOfCharacter(char* haystack, char needle) {
    int currentIndex = 0;
    
    while (haystack[currentIndex] != '\0' && haystack[currentIndex] != needle) {
        currentIndex++;
    }
    
    return currentIndex;
}

void sendLastClientToLastDevice() {
    char pairData[INET_ADDRSTRLEN + 6] = {};
    int bytesWritten = sprintf(pairData, "%s:%d", ::lastClient->address, ::lastClient->port);
    
    ::serverSocket.send((sockaddr*) &::lastDevice->sendingSocket, pairData, bytesWritten);
}

int main(int argc, const char* argv[]) {
    
    sockaddr_in senderSocket;
    char senderData[MAX_PACKET_SIZE_BYTES] = {};
    ssize_t receivedBytes = 0;
    
    while (true) {
        if (::serverSocket.receive((sockaddr *)&senderSocket, &senderData, &receivedBytes)) {
            if (senderData[0] == 'A') {
                // this is a device reporting itself as available
                
                PairableDevice tempDevice = {};
                
                char deviceAddress[INET_ADDRSTRLEN] = {};
                int socketPort = 0;
                
                int numMatches = sscanf(senderData, "Available %s %[^:]:%d %s",
                                        tempDevice.identifier,
                                        deviceAddress,
                                        &socketPort,
                                        tempDevice.name);
                
                if (numMatches >= 3) {
                    // if we have fewer than 3 matches the packet wasn't properly formatted
                    
                    // setup the localSocket for the pairing device
                    tempDevice.localSocket.sin_family = AF_INET;
                    inet_pton(AF_INET, deviceAddress, &::lastDevice);
                    tempDevice.localSocket.sin_port = socketPort;
                    
                    // store this device's sending socket so we can talk back to it
                    tempDevice.sendingSocket = senderSocket;
                    
                    // push this new device into the vector
                    printf("New last device is %s (%s) at %s:%d\n",
                           tempDevice.identifier,
                           tempDevice.name,
                           deviceAddress,
                           socketPort);
                
                    // copy the tempDevice to the persisting lastDevice                    
                    ::lastDevice = new PairableDevice(tempDevice);
                    
                    if (::lastClient) {
                        sendLastClientToLastDevice();
                    }
                }
            } else if (senderData[0] == 'F') {
                // this is a client looking to pair with a device
                // send the most recent device this address so it can attempt to pair
                
                RequestingClient tempClient = {};
                
                int requestorMatches = sscanf(senderData, "Find %[^:]:%d",
                                              tempClient.address,
                                              &tempClient.port);
                
                if (requestorMatches == 2) {
                    // good data, copy the tempClient to the persisting lastInterfaceClient                    
                    ::lastClient = new RequestingClient(tempClient);
                    
                    printf("New last client is at %s:%d\n",
                           ::lastClient->address,
                           ::lastClient->port);
                    
                    if (::lastDevice) {
                        sendLastClientToLastDevice();
                    }
                }
            }
        }
    }
}
