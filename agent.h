//
//  agent.h
//  interface
//
//  Created by Philip Rosedale on 11/20/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef interface_agent_h
#define interface_agent_h

#include "glm/glm.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include "network.h"

void update_agents(char * data, int length);
int add_agent(std::string * IP);

#endif
