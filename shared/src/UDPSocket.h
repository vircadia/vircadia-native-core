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
#include <netinet/in.h>
#include <netdb.h>

#define MAX_BUFFER_LENGTH_BYTES 1024

class UDPSocket {    
    public:    
        UDPSocket(int listening_port);
        ~UDPSocket();
        int send(sockaddr_in *destAddress, const void *data, size_t byteLength);
        int send(char *destAddress, int destPort, const void *data, size_t byteLength);
        bool receive(void *receivedData, ssize_t *receivedBytes);
        bool receive(sockaddr_in *recvAddress, void *receivedData, ssize_t *receivedBytes);
    private:
        int handle;
    
        UDPSocket(); // private default constructor
    
        struct AgentData {
            char * address;
            int listening_port;
        };

        AgentData *known_agents;
};

#endif /* defined(__interface__UDPSocket__) */
