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

struct sockaddr_in UDPSocket::sockaddr_util(char* hostname, int port) {
    sockaddr_in dest_address;
    
    dest_address.sin_family = AF_INET;
    dest_address.sin_addr.s_addr = inet_addr(hostname);
    dest_address.sin_port = htons((uint16_t)port);
    
    return dest_address;
}

struct sockaddr_in dest_sockaddr;

UDPSocket::UDPSocket(int listeningPort) {
    // create the socket
    handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if (handle <= 0) {
        printf("Failed to create socket.\n");
        return;
    }
    
    // instantiate the re-usable dest_sockaddr with a dummy IP and port
    dest_sockaddr = UDPSocket::sockaddr_util((char *) "1.0.0.0", 1);
    
    // bind the socket to the passed listeningPort
    sockaddr_in bind_address = UDPSocket::sockaddr_util(INADDR_ANY, listeningPort);
    
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
}

int UDPSocket::send(char * dest_address, int dest_port, const void *data, int length_in_bytes) {
    
    // change address and port on reusable global to passed variables
    dest_sockaddr.sin_addr.s_addr = inet_addr(dest_address);
    dest_sockaddr.sin_port = htons((uint16_t)dest_port);
    
    // send data via UDP
    int sent_bytes = sendto(handle, (const char*)data, length_in_bytes,
                        0, (sockaddr*)&dest_address, sizeof(sockaddr_in));
    
    if (sent_bytes != length_in_bytes) {
        printf("Failed to send packet: return value = %d\n", sent_bytes);
        return false;
    }
    
    return sent_bytes;
}
