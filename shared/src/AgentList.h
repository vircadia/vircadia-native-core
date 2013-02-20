//
//  AgentList.h
//  hifi
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__AgentList__
#define __hifi__AgentList__

#include <iostream>
#include <vector>
#include "Agent.h"

const unsigned short AGENT_SOCKET_LISTEN_PORT = 40103;

class AgentList {
    public:
        std::vector<Agent> agents;
        int updateList(char *packetData);
        
        int processAgentData(char *address, unsigned short port, char *packetData, size_t dataBytes);
        int broadcastToAgents(char *broadcastData, size_t dataBytes, bool sendToSelf);
        void pingAgents();
    private:
        int addAgent(AgentSocket *publicSocket, AgentSocket *localSocket, char agentType);
        int indexOfMatchingAgent(AgentSocket *publicSocket, AgentSocket *privateSocket, char agentType);

};

#endif /* defined(__hifi__AgentList__) */
