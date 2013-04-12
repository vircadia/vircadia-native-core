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
#include <stdint.h>
#include "Agent.h"
#include "UDPSocket.h"

#ifdef _WIN32
#include "pthread.h"
#endif

const int MAX_PACKET_SIZE = 1500;
const unsigned int AGENT_SOCKET_LISTEN_PORT = 40103;
const int AGENT_SILENCE_THRESHOLD_USECS = 2 * 1000000;
extern const char *SOLO_AGENT_TYPES_STRING;

extern char DOMAIN_HOSTNAME[];
extern char DOMAIN_IP[100];    //  IP Address will be re-set by lookup on startup
extern const int DOMAINSERVER_PORT;

class AgentList {
public:
    static AgentList* createInstance(char ownerType, unsigned int socketListenPort = AGENT_SOCKET_LISTEN_PORT);
    static AgentList* getInstance();
    
    void(*linkedDataCreateCallback)(Agent *);
    void(*audioMixerSocketUpdate)(in_addr_t, in_port_t);
    
    std::vector<Agent>& getAgents();
    UDPSocket& getAgentSocket();
    
    uint16_t getLastAgentId();
    void increaseAgentId();
    
    int updateList(unsigned char *packetData, size_t dataBytes);
    
    int indexOfMatchingAgent(sockaddr *senderAddress);
    int indexOfMatchingAgent(uint16_t agentID);
    
    bool addOrUpdateAgent(sockaddr *publicSocket, sockaddr *localSocket, char agentType, uint16_t agentId);
    
    void processAgentData(sockaddr *senderAddress, void *packetData, size_t dataBytes);
    void processBulkAgentData(sockaddr *senderAddress, void *packetData, int numTotalBytes, int numBytesPerAgent);
   
    void updateAgentWithData(sockaddr *senderAddress, void *packetData, size_t dataBytes);
    void updateAgentWithData(Agent *agent, void *packetData, int dataBytes);
    
    void broadcastToAgents(char *broadcastData, size_t dataBytes, const char* agentTypes, int numAgentTypes);
    char getOwnerType();
    unsigned int getSocketListenPort();
    
    void startSilentAgentRemovalThread();
    void stopSilentAgentRemovalThread();
    void startDomainServerCheckInThread();
    void stopDomainServerCheckInThread();
    void startPingUnknownAgentsThread();
    void stopPingUnknownAgentsThread();
private:
    static AgentList* _sharedInstance;
    
    AgentList(char ownerType, unsigned int socketListenPort);
    ~AgentList();
    AgentList(AgentList const&); // Don't implement, needed to avoid copies of singleton
    void operator=(AgentList const&); // Don't implement, needed to avoid copies of singleton
    
    UDPSocket agentSocket;
    char ownerType;
    unsigned int socketListenPort;
    std::vector<Agent> agents;
    uint16_t lastAgentId;
    pthread_t removeSilentAgentsThread;
    pthread_t checkInWithDomainServerThread;
    pthread_t pingUnknownAgentsThread;
    
    void handlePingReply(sockaddr *agentAddress);
};

int unpackAgentId(unsigned char *packedData, uint16_t *agentId);
int packAgentId(unsigned char *packStore, uint16_t agentId);

#endif /* defined(__hifi__AgentList__) */
