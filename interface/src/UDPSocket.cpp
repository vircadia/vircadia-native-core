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

sockaddr_in destSockaddr, senderAddress;
socklen_t addLength = sizeof(senderAddress);

UDPSocket::UDPSocket(int listeningPort) {
    // create the socket
    handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if (handle <= 0) {
        printf("Failed to create socket.\n");
        return;
    }
    
    // instantiate the re-usable dest_sockaddr with a dummy IP and port
    sockaddr_in dest_sockaddr;
    dest_sockaddr.sin_family = AF_INET;
    dest_sockaddr.sin_addr.s_addr = inet_addr("1.0.0.0");
    dest_sockaddr.sin_port = htons((uint16_t) 1);
    
    // bind the socket to the passed listeningPort
    sockaddr_in bind_address;
    bind_address.sin_family = AF_INET;
    bind_address.sin_addr.s_addr = INADDR_ANY;
    bind_address.sin_port = htons((uint16_t) listeningPort);
    
    if (bind(handle, (const sockaddr*) &bind_address, sizeof(sockaddr_in)) < 0) {
        printf("Failed to bind socket to port %d.\n", listeningPort);
        return;
    }
    
    //  set socket as non-blocking
    int nonBlocking = 1;
    if (fcntl(handle, F_SETFL, O_NONBLOCK, nonBlocking) == -1) {
        printf("Failed to set non-blocking socket\n");
        return;
    }
    
    printf("Created UDP socket listening on port %d.\n", listeningPort);
}

//  Receive data on this socket with retrieving address of sender
bool UDPSocket::receive(void *receivedData, int *receivedBytes) {
    
    *receivedBytes = recvfrom(handle, receivedData, MAX_BUFFER_LENGTH_BYTES,
                              0, (sockaddr *)&senderAddress, &addLength);
    
    return (*receivedBytes > 0);
}

//  Receive data on this socket with the address of the sender 
bool UDPSocket::receive(sockaddr_in *senderAddress, void *receivedData, int *receivedBytes) {
    
    *receivedBytes = recvfrom(handle, receivedData, MAX_BUFFER_LENGTH_BYTES,
                              0, (sockaddr *)senderAddress, &addLength);
    
    return (*receivedBytes > 0);
}

int UDPSocket::send(char * destAddress, int destPort, const void *data, int byteLength) {
    
    // change address and port on reusable global to passed variables
    destSockaddr.sin_addr.s_addr = inet_addr(destAddress);
    destSockaddr.sin_port = htons((uint16_t)destPort);
    
    // send data via UDP
    int sent_bytes = sendto(handle, (const char*)data, byteLength,
                        0, (sockaddr *)&destSockaddr, sizeof(sockaddr_in));
    
    if (sent_bytes != byteLength) {
        printf("Failed to send packet: return value = %d\n", sent_bytes);
        return false;
    }
    
    return sent_bytes;
}
