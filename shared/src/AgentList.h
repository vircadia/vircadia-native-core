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

const int MAX_PACKET_SIZE = 1500;
const unsigned short AGENT_SOCKET_LISTEN_PORT = 40103;
const int AGENT_SILENCE_THRESHOLD_USECS = 2 * 1000000;
extern const char *SOLO_AGENT_TYPES_STRING;

class AgentList {
    public:
        AgentList();
        AgentList(int socketListenPort);
        ~AgentList();
        
        void(*linkedDataCreateCallback)(Agent *);
        void(*audioMixerSocketUpdate)(in_addr_t, in_port_t);
        void(*voxelServerAddCallback)(sockaddr *);

        std::vector<Agent>& getAgents();
        UDPSocket& getAgentSocket();
    
        int updateList(unsigned char *packetData, size_t dataBytes);
        bool addOrUpdateAgent(sockaddr *publicSocket, sockaddr *localSocket, char agentType);
        void processAgentData(sockaddr *senderAddress, void *packetData, size_t dataBytes);
        void updateAgentWithData(sockaddr *senderAddress, void *packetData, size_t dataBytes);
        void broadcastToAgents(char *broadcastData, size_t dataBytes);
        void sendToAgent(Agent *destAgent, void *packetData, size_t dataBytes);
        void pingAgents();
        void startSilentAgentRemovalThread();
        void stopSilentAgentRemovalThread();
    private:
        UDPSocket agentSocket;
        std::vector<Agent> agents;
        pthread_t removeSilentAgentsThread;
        int16_t lastAgentId;
        int indexOfMatchingAgent(sockaddr *senderAddress);
        void handlePingReply(sockaddr *agentAddress);
};

#endif /* defined(__hifi__AgentList__) */
