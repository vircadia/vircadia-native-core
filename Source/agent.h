//
//  agent.h
//  interface
//
//  Created by Philip Rosedale on 11/20/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef interface_agent_h
#define interface_agent_h

#include "glm.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include "network.h"

void update_agents(char * data, int length);
int add_agent(std::string * IP);
int broadcast_to_agents(int handle, char * data, int length);
void update_agent(in_addr addr, char * data, int length);
void render_agents();

#endif
