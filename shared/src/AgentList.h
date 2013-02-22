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
const int AGENT_SILENCE_THRESHOLD_USECS = 2 * 1000000;

class AgentList {
    public:
        AgentList();
        AgentList(int socketListenPort);
        ~AgentList();
        std::vector<Agent> agents;
        void(*linkedDataCreateCallback)(Agent *);
        void(*audioMixerSocketUpdate)(in_addr_t, in_port_t);
    
        UDPSocket* getAgentSocket();
    
        int updateList(unsigned char *packetData, size_t dataBytes);
        bool addOrUpdateAgent(sockaddr *publicSocket, sockaddr *localSocket, char agentType);
        void processAgentData(sockaddr *senderAddress, void *packetData, size_t dataBytes);
        void broadcastToAgents(char *broadcastData, size_t dataBytes);
        void sendToAgent(Agent *destAgent, void *packetData, size_t dataBytes);
        void pingAgents();
        void startSilentAgentRemovalThread();
        void stopSilentAgentRemovalThread();
    private:
        int indexOfMatchingAgent(sockaddr *senderAddress);
        void updateAgentWithData(sockaddr *senderAddress, void *packetData, size_t dataBytes);
        void handlePingReply(sockaddr *agentAddress);
        UDPSocket agentSocket;
        pthread_t removeSilentAgentsThread;
};

#endif /* defined(__hifi__AgentList__) */
