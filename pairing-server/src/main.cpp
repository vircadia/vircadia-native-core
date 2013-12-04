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

#include <arpa/inet.h>

#include <QtNetwork/QUdpSocket>

#include <HifiSockAddr.h>

const int PAIRING_SERVER_LISTEN_PORT = 7247;
const int MAX_PACKET_SIZE_BYTES = 1400;

struct PairableDevice {
    char identifier[64];
    char name[64];
    HifiSockAddr sendingSocket;
    HifiSockAddr localSocket;
};

struct RequestingClient {
    char address[INET_ADDRSTRLEN];
    int port;
};

QUdpSocket serverSocket;
PairableDevice* lastDevice = NULL;
RequestingClient* lastClient = NULL;

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
    
    ::serverSocket.writeDatagram(pairData, bytesWritten,
                                 ::lastDevice->sendingSocket.getAddress(), ::lastDevice->sendingSocket.getPort());
}

int main(int argc, const char* argv[]) {
    
    serverSocket.bind(QHostAddress::LocalHost, PAIRING_SERVER_LISTEN_PORT);
    
    HifiSockAddr senderSockAddr;
    char senderData[MAX_PACKET_SIZE_BYTES] = {};
    
    while (true) {
        if (::serverSocket.hasPendingDatagrams()
            && ::serverSocket.readDatagram(senderData, MAX_PACKET_SIZE_BYTES,
                                           senderSockAddr.getAddressPointer(), senderSockAddr.getPortPointer())) {
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
                    tempDevice.localSocket.setAddress(QHostAddress(deviceAddress));
                    tempDevice.localSocket.setPort(socketPort);
                    
                    // store this device's sending socket so we can talk back to it
                    tempDevice.sendingSocket = senderSockAddr;
                    
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
