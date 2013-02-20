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
#include "UDPSocket.h"

const unsigned short AGENT_SOCKET_LISTEN_PORT = 40103;

class AgentList {
    public:
        AgentList();
        std::vector<Agent> agents;
        int updateList(char *packetData);
        void processAgentData(sockaddr *senderAddress, char *packetData, size_t dataBytes);
    
        int broadcastToAgents(char *broadcastData, size_t dataBytes, bool sendToSelf);
        void pingAgents();
        UDPSocket* getAgentSocket();
    private:
        UDPSocket agentSocket;
        int addAgent(sockaddr *publicSocket, sockaddr *localSocket, char agentType);
        void handlePingReply(sockaddr *agentAddress);
};

#endif /* defined(__hifi__AgentList__) */
