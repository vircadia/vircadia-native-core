//
//  UDPSocket.cpp
//  interface
//
//  Created by Stephen Birarda on 1/28/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "UDPSocket.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cstdio>
#include <errno.h>
#include <err.h>

sockaddr_in destSockaddr, senderAddress;

UDPSocket::UDPSocket(int listeningPort) {
    // create the socket
    handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if (handle <= 0) {
        printf("Failed to create socket.\n");
        return;
    }
    
    destSockaddr.sin_family = AF_INET;
    
    // bind the socket to the passed listeningPort
    sockaddr_in bind_address;
    bind_address.sin_family = AF_INET;
    bind_address.sin_addr.s_addr = INADDR_ANY;
    bind_address.sin_port = htons((uint16_t) listeningPort);
    
    if (bind(handle, (const sockaddr*) &bind_address, sizeof(sockaddr_in)) < 0) {
        printf("Failed to bind socket to port %d.\n", listeningPort);
        return;
    }
    
    printf("Created UDP socket listening on port %d.\n", listeningPort);
}

UDPSocket::~UDPSocket() {
    close(handle);
}

//  Receive data on this socket with retrieving address of sender
bool UDPSocket::receive(void *receivedData, ssize_t *receivedBytes) {
    
    return receive(&senderAddress, receivedData, receivedBytes);
}

//  Receive data on this socket with the address of the sender 
bool UDPSocket::receive(sockaddr_in *recvAddress, void *receivedData, ssize_t *receivedBytes) {
    
    socklen_t addressSize = sizeof(&recvAddress);
    
    *receivedBytes = recvfrom(handle, receivedData, MAX_BUFFER_LENGTH_BYTES,
                              0, (sockaddr *) recvAddress, &addressSize);
    
    return (*receivedBytes > 0);
}

int UDPSocket::send(sockaddr_in *destAddress, const void *data, size_t byteLength) {
    // send data via UDP
    
    int sent_bytes = sendto(handle, (const char*)data, byteLength,
                            0, (sockaddr *) destAddress, sizeof(sockaddr_in));
    
    if (sent_bytes != byteLength) {
        printf("Failed to send packet: %s\n", strerror(errno));
        return false;
    }
    
    return sent_bytes;
}

int UDPSocket::send(char * destAddress, int destPort, const void *data, size_t byteLength) {
    
    // change address and port on reusable global to passed variables
    destSockaddr.sin_addr.s_addr = inet_addr(destAddress);
    destSockaddr.sin_port = htons((uint16_t)destPort);
    
    return send(&destSockaddr, data, byteLength);
}
