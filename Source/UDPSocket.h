//
//  UDPSocket.h
//  interface
//
//  Created by Stephen Birarda on 1/28/13.
//  Copyright (c) 2013 Rosedale Lab. All rights reserved.
//

#ifndef __interface__UDPSocket__
#define __interface__UDPSocket__

#include <iostream>
#include <netinet/in.h>

class UDPSocket {    
    public:
        static struct sockaddr_in sockaddr_util(char *address, int port);
    
    UDPSocket(int listening_port);
        int send(char * dest_address, int dest_port, const void *data, int length_in_bytes);
    private:
        UDPSocket(); // private default constructor
        int handle;
    
        struct AgentData {
            char * address;
            int listening_port;
        };

        AgentData *known_agents;
};

#endif /* defined(__interface__UDPSocket__) */
