//
//  Agent.h
//  interface
//
//  Created by Philip Rosedale on 11/20/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Agent__
#define __interface__Agent__

#include <glm/glm.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include "UDPSocket.h"


int update_agents(char * data, int length);
int add_agent(char * address, unsigned short port);
int broadcast_to_agents(UDPSocket * handle, char * data, int length);
void update_agent(char * address, unsigned short port, char * data, int length);
void render_agents();

#endif
