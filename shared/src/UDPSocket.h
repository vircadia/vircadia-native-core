//
//  UDPSocket.h
//  interface
//
//  Created by Stephen Birarda on 1/28/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__UDPSocket__
#define __interface__UDPSocket__

#include <iostream>

#ifdef _WIN32
#include "Syssocket.h"
#else
#include <netinet/in.h>
#include <netdb.h>
#endif

#define MAX_BUFFER_LENGTH_BYTES 1500

class UDPSocket {    
    public:
        UDPSocket(int listening_port);
        ~UDPSocket();
        bool init();
        int send(sockaddr *destAddress, const void *data, size_t byteLength);
        int send(char *destAddress, int destPort, const void *data, size_t byteLength);
        bool receive(void *receivedData, ssize_t *receivedBytes);
        bool receive(sockaddr *recvAddress, void *receivedData, ssize_t *receivedBytes);
    private:
        int handle;
};

bool socketMatch(sockaddr *first, sockaddr *second);
int packSocket(unsigned char *packStore, in_addr_t inAddress, in_port_t networkOrderPort);
int packSocket(unsigned char *packStore, sockaddr *socketToPack);
int unpackSocket(unsigned char *packedData, sockaddr *unpackDestSocket);
int getLocalAddress();

#endif /* defined(__interface__UDPSocket__) */
