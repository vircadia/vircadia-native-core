//
//  UDPSocket.cpp
//  interface
//
//  Created by Stephen Birarda on 1/28/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "UDPSocket.h"
#include <fcntl.h>
#include <cstdio>
#include <errno.h>
#include <string.h>

#ifdef _WIN32
#include "Syssocket.h"
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#endif

sockaddr_in destSockaddr, senderAddress;

bool socketMatch(sockaddr *first, sockaddr *second) {
    // utility function that indicates if two sockets are equivalent
   
    // currently only compares two IPv4 addresses
    // expandable to IPv6 by adding else if for AF_INET6
    
    if (first->sa_family != second->sa_family) {
        // not the same family, can't be equal
        return false;
    } else if (first->sa_family == AF_INET) {
        sockaddr_in *firstIn = (sockaddr_in *) first;
        sockaddr_in *secondIn = (sockaddr_in *) second;
        
        return firstIn->sin_addr.s_addr == secondIn->sin_addr.s_addr
            && firstIn->sin_port == secondIn->sin_port;
    } else {
        return false;
    }
}

int packSocket(unsigned char *packStore, in_addr_t inAddress, in_port_t networkOrderPort) {
    packStore[0] = inAddress >> 24;
    packStore[1] = inAddress >> 16;
    packStore[2] = inAddress >> 8;
    packStore[3] = inAddress;
    
    packStore[4] = networkOrderPort >> 8;
    packStore[5] = networkOrderPort;
    
    return 6; // could be dynamically more if we need IPv6
}

int packSocket(unsigned char *packStore, sockaddr *socketToPack) {
    return packSocket(packStore, ((sockaddr_in *) socketToPack)->sin_addr.s_addr, ((sockaddr_in *) socketToPack)->sin_port);
}

int unpackSocket(unsigned char *packedData, sockaddr *unpackDestSocket) {
    sockaddr_in *destinationSocket = (sockaddr_in *) unpackDestSocket;
    destinationSocket->sin_addr.s_addr = (packedData[0] << 24) + (packedData[1] << 16) + (packedData[2] << 8) + packedData[3];
    destinationSocket->sin_port = (packedData[4] << 8) + packedData[5];
    return 6; // this could be more if we ever need IPv6
}

int getLocalAddress() {
    // get this agent's local address so we can pass that to DS
    struct ifaddrs * ifAddrStruct = NULL;
    struct ifaddrs * ifa = NULL;
    
    int family;
    int localAddress = 0;
 
#ifndef _WIN32
    getifaddrs(&ifAddrStruct); 
        
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        family = ifa->ifa_addr->sa_family;
        if (family == AF_INET) {
            localAddress = ((sockaddr_in *)ifa->ifa_addr)->sin_addr.s_addr;
        }
    }
    
    freeifaddrs(ifAddrStruct);
#else 
    // Get the local hostname
    char szHostName[255];
    gethostname(szHostName, 255);
    struct hostent *host_entry;
    host_entry = gethostbyname(szHostName);
    char * szLocalIP;
    szLocalIP = inet_ntoa (*(struct in_addr *)*host_entry->h_addr_list);
    localAddress = inet_addr(szLocalIP);
#endif
    
    return localAddress;
}

UDPSocket::UDPSocket(int listeningPort) {
    init();
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
    
    // set timeout on socket recieve to 0.5 seconds
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000;
    setsockopt(handle, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof tv);
    
    printf("Created UDP socket listening on port %d.\n", listeningPort);
}

UDPSocket::~UDPSocket() {
#ifdef _WIN32
    closesocket(handle);
#else
    close(handle);
#endif
}

bool UDPSocket::init() { 
#ifdef _WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
 
    wVersionRequested = MAKEWORD( 2, 2 );
 
    err = WSAStartup( wVersionRequested, &wsaData );
    if ( err != 0 ) {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        return false;
    }
 
    /* Confirm that the WinSock DLL supports 2.2.*/
    /* Note that if the DLL supports versions later    */
    /* than 2.2 in addition to 2.2, it will still return */
    /* 2.2 in wVersion since that is the version we      */
    /* requested.                                        */
 
    if ( LOBYTE( wsaData.wVersion ) != 2 ||
            HIBYTE( wsaData.wVersion ) != 2 ) {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        WSACleanup();
        return false; 
    }
#endif
    return true;
}

//  Receive data on this socket with retrieving address of sender
bool UDPSocket::receive(void *receivedData, ssize_t *receivedBytes) {
    
    return receive((sockaddr *)&senderAddress, receivedData, receivedBytes);
}

//  Receive data on this socket with the address of the sender 
bool UDPSocket::receive(sockaddr *recvAddress, void *receivedData, ssize_t *receivedBytes) {
    
#ifdef _WIN32
    int addressSize = sizeof(*recvAddress);
#else
    socklen_t addressSize = sizeof(&recvAddress);
#endif    
    *receivedBytes = recvfrom(handle, static_cast<char*>(receivedData), MAX_BUFFER_LENGTH_BYTES,
                              0, recvAddress, &addressSize);
    
    return (*receivedBytes > 0);
}

int UDPSocket::send(sockaddr *destAddress, const void *data, size_t byteLength) {
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
    
    return send((sockaddr *)&destSockaddr, data, byteLength);
}
