//
//  AgentList.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <pthread.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "AgentList.h"
#include "AgentTypes.h"
#include "PacketHeaders.h"
#include "SharedUtil.h"
#include "shared_Log.h"

#ifdef _WIN32
#include "Syssocket.h"
#else
#include <arpa/inet.h>
#endif

using shared_lib::printLog;

const char SOLO_AGENT_TYPES_STRING[] = {
    AGENT_TYPE_AVATAR_MIXER,
    AGENT_TYPE_AUDIO_MIXER,
    AGENT_TYPE_VOXEL
};

char DOMAIN_HOSTNAME[] = "highfidelity.below92.com";
char DOMAIN_IP[100] = "";    //  IP Address will be re-set by lookup on startup
const int DOMAINSERVER_PORT = 40102;

bool silentAgentThreadStopFlag = false;
bool domainServerCheckinStopFlag = false;
bool pingUnknownAgentThreadStopFlag = false;
pthread_mutex_t vectorChangeMutex = PTHREAD_MUTEX_INITIALIZER;

AgentList* AgentList::_sharedInstance = NULL;

AgentList* AgentList::createInstance(char ownerType, unsigned int socketListenPort) {
    if (_sharedInstance == NULL) {
        _sharedInstance = new AgentList(ownerType, socketListenPort);
    } else {
        printLog("AgentList createInstance called with existing instance.\n");
    }
    
    return _sharedInstance;
}

AgentList* AgentList::getInstance() {
    if (_sharedInstance == NULL) {
        printLog("AgentList getInstance called before call to createInstance. Returning NULL pointer.\n");
    }
    
    return _sharedInstance;
}


AgentList::AgentList(char newOwnerType, unsigned int newSocketListenPort) : agentSocket(newSocketListenPort) {
    ownerType = newOwnerType;
    socketListenPort = newSocketListenPort;
    lastAgentId = 0;
}

AgentList::~AgentList() {
    // stop the spawned threads, if they were started
    stopSilentAgentRemovalThread();
    stopDomainServerCheckInThread();
    stopPingUnknownAgentsThread();
}

std::vector<Agent>& AgentList::getAgents() {
    return agents;
}

UDPSocket& AgentList::getAgentSocket() {
    return agentSocket;
}

char AgentList::getOwnerType() {
    return ownerType;
}

unsigned int AgentList::getSocketListenPort() {
    return socketListenPort;
}

void AgentList::processAgentData(sockaddr *senderAddress, unsigned char *packetData, size_t dataBytes) {
    switch (((char *)packetData)[0]) {
        case PACKET_HEADER_DOMAIN: {
            updateList(packetData, dataBytes);
            break;
        }
        case PACKET_HEADER_PING: {
            agentSocket.send(senderAddress, &PACKET_HEADER_PING_REPLY, 1);
            break;
        }
        case PACKET_HEADER_PING_REPLY: {
            handlePingReply(senderAddress);
            break;
        }
    }
}

void AgentList::processBulkAgentData(sockaddr *senderAddress, unsigned char *packetData, int numTotalBytes, int numBytesPerAgent) {
    // find the avatar mixer in our agent list and update the lastRecvTime from it
    int bulkSendAgentIndex = indexOfMatchingAgent(senderAddress);

    if (bulkSendAgentIndex >= 0) {
        Agent *bulkSendAgent = &agents[bulkSendAgentIndex];
        bulkSendAgent->setLastRecvTimeUsecs(usecTimestampNow());
        bulkSendAgent->recordBytesReceived(numTotalBytes);
    }

    unsigned char *startPosition = packetData;
    unsigned char *currentPosition = startPosition + 1;
    unsigned char packetHolder[numBytesPerAgent + 1];
    
    packetHolder[0] = PACKET_HEADER_HEAD_DATA;
    
    uint16_t agentID = -1;
    
    while ((currentPosition - startPosition) < numTotalBytes) {
        currentPosition += unpackAgentId(currentPosition, &agentID);
        memcpy(packetHolder + 1, currentPosition, numBytesPerAgent);
        
        int matchingAgentIndex = indexOfMatchingAgent(agentID);
        
        if (matchingAgentIndex >= 0) {
            
            updateAgentWithData(&agents[matchingAgentIndex], packetHolder, numBytesPerAgent + 1);
        }
        
        currentPosition += numBytesPerAgent;
    }
}

void AgentList::updateAgentWithData(sockaddr *senderAddress, unsigned char *packetData, size_t dataBytes) {
    // find the agent by the sockaddr
    int agentIndex = indexOfMatchingAgent(senderAddress);
    
    if (agentIndex != -1) {
        updateAgentWithData(&agents[agentIndex], packetData, dataBytes);
    }
}

void AgentList::updateAgentWithData(Agent *agent, unsigned char *packetData, int dataBytes) {
    agent->setLastRecvTimeUsecs(usecTimestampNow());
    agent->recordBytesReceived(dataBytes);
    
    if (agent->getLinkedData() == NULL) {
        if (linkedDataCreateCallback != NULL) {
            linkedDataCreateCallback(agent);
        }
    }

    agent->getLinkedData()->parseData(packetData, dataBytes);
}

int AgentList::indexOfMatchingAgent(sockaddr *senderAddress) {
    for(std::vector<Agent>::iterator agent = agents.begin(); agent != agents.end(); agent++) {
        if (agent->getActiveSocket() != NULL && socketMatch(agent->getActiveSocket(), senderAddress)) {
            return agent - agents.begin();
        }
    }
    
    return -1;
}

int AgentList::indexOfMatchingAgent(uint16_t agentID) {
    for(std::vector<Agent>::iterator agent = agents.begin(); agent != agents.end(); agent++) {
        if (agent->getActiveSocket() != NULL && agent->getAgentId() == agentID) {
            return agent - agents.begin();
        }
    }

    return -1;
}

uint16_t AgentList::getLastAgentId() {
    return lastAgentId;
}

void AgentList::increaseAgentId() {
    ++lastAgentId;
}

int AgentList::updateList(unsigned char *packetData, size_t dataBytes) {
    int readAgents = 0;

    char agentType;
    uint16_t agentId;
    
    // assumes only IPv4 addresses
    sockaddr_in agentPublicSocket;
    agentPublicSocket.sin_family = AF_INET;
    sockaddr_in agentLocalSocket;
    agentLocalSocket.sin_family = AF_INET;
    
    unsigned char *readPtr = packetData + 1;
    unsigned char *startPtr = packetData;
    
    while((readPtr - startPtr) < dataBytes) {
        agentType = *readPtr++;
        readPtr += unpackAgentId(readPtr, (uint16_t *)&agentId);
        readPtr += unpackSocket(readPtr, (sockaddr *)&agentPublicSocket);
        readPtr += unpackSocket(readPtr, (sockaddr *)&agentLocalSocket);
        
        addOrUpdateAgent((sockaddr *)&agentPublicSocket, (sockaddr *)&agentLocalSocket, agentType, agentId);
    }  

    return readAgents;
}

bool AgentList::addOrUpdateAgent(sockaddr *publicSocket, sockaddr *localSocket, char agentType, uint16_t agentId) {
    std::vector<Agent>::iterator agent;
    
    for (agent = agents.begin(); agent != agents.end(); agent++) {
        if (agent->matches(publicSocket, localSocket, agentType)) {
            // we already have this agent, stop checking
            break;
        }
    }
    
    if (agent == agents.end()) {
        // we didn't have this agent, so add them
        Agent newAgent = Agent(publicSocket, localSocket, agentType, agentId);
        
        if (socketMatch(publicSocket, localSocket)) {
            // likely debugging scenario with two agents on local network
            // set the agent active right away
            newAgent.activatePublicSocket();
        }
        
        if (newAgent.getType() == AGENT_TYPE_AUDIO_MIXER && audioMixerSocketUpdate != NULL) {
            // this is an audio mixer
            // for now that means we need to tell the audio class
            // to use the local socket information the domain server gave us
            sockaddr_in *publicSocketIn = (sockaddr_in *)publicSocket;
            audioMixerSocketUpdate(publicSocketIn->sin_addr.s_addr, publicSocketIn->sin_port);
        } else if (newAgent.getType() == AGENT_TYPE_VOXEL) {
            newAgent.activatePublicSocket();
        }
        
        printLog("Added agent - ");
        Agent::printLog(newAgent);
        
        pthread_mutex_lock(&vectorChangeMutex);
        agents.push_back(newAgent);
        pthread_mutex_unlock(&vectorChangeMutex);
        
        return true;
    } else {
        
        if (agent->getType() == AGENT_TYPE_AUDIO_MIXER || agent->getType() == AGENT_TYPE_VOXEL) {
            // until the Audio class also uses our agentList, we need to update
            // the lastRecvTimeUsecs for the audio mixer so it doesn't get killed and re-added continously
            agent->setLastRecvTimeUsecs(usecTimestampNow());
        }
        
        // we had this agent already, do nothing for now
        return false;
    }    
}

void AgentList::broadcastToAgents(unsigned char *broadcastData, size_t dataBytes, const char* agentTypes, int numAgentTypes) {
    for(std::vector<Agent>::iterator agent = agents.begin(); agent != agents.end(); agent++) {
        // only send to the AgentTypes we are asked to send to.
        if (agent->getActiveSocket() != NULL && memchr(agentTypes, agent->getType(), numAgentTypes)) {
            // we know which socket is good for this agent, send there
            agentSocket.send(agent->getActiveSocket(), broadcastData, dataBytes);
        }
    }
}

void AgentList::handlePingReply(sockaddr *agentAddress) {
    for(std::vector<Agent>::iterator agent = agents.begin(); agent != agents.end(); agent++) {
        // check both the public and local addresses for each agent to see if we find a match
        // prioritize the private address so that we prune erroneous local matches
        if (socketMatch(agent->getPublicSocket(), agentAddress)) {
            agent->activatePublicSocket();
            break;
        } else if (socketMatch(agent->getLocalSocket(), agentAddress)) {
            agent->activateLocalSocket();
            break;
        }
    }
}

Agent* AgentList::soloAgentOfType(char agentType) {    
    if (memchr(SOLO_AGENT_TYPES_STRING, agentType, 1)) {
        for(std::vector<Agent>::iterator agent = agents.begin(); agent != agents.end(); agent++) {
            if (agent->getType() == agentType) {
                return &*agent;
            }
        }
    }
    
    return NULL;
}

void *pingUnknownAgents(void *args) {
    
    AgentList *agentList = (AgentList *)args;
    const int PING_INTERVAL_USECS = 1 * 1000000;
    
    timeval lastSend;
    
    while (!pingUnknownAgentThreadStopFlag) {
        gettimeofday(&lastSend, NULL);
        
        for(std::vector<Agent>::iterator agent = agentList->getAgents().begin();
            agent != agentList->getAgents().end();
            agent++) {
            if (agent->getActiveSocket() == NULL) {
                // ping both of the sockets for the agent so we can figure out
                // which socket we can use
                agentList->getAgentSocket().send(agent->getPublicSocket(), &PACKET_HEADER_PING, 1);
                agentList->getAgentSocket().send(agent->getLocalSocket(), &PACKET_HEADER_PING, 1);
            }
        }
        
        double usecToSleep = PING_INTERVAL_USECS - (usecTimestampNow() - usecTimestamp(&lastSend));
        
        if (usecToSleep > 0) {
            usleep(usecToSleep);
        }
    }
    
    return NULL;
}

void AgentList::startPingUnknownAgentsThread() {
    pthread_create(&pingUnknownAgentsThread, NULL, pingUnknownAgents, (void *)this);
}

void AgentList::stopPingUnknownAgentsThread() {
    pingUnknownAgentThreadStopFlag = true;
    pthread_join(pingUnknownAgentsThread, NULL);
}

void *removeSilentAgents(void *args) {
    std::vector<Agent> *agents = (std::vector<Agent> *)args;
    double checkTimeUSecs, sleepTime;
    
    while (!silentAgentThreadStopFlag) {
        checkTimeUSecs = usecTimestampNow();
        
        for(std::vector<Agent>::iterator agent = agents->begin(); agent != agents->end();) {
            
            pthread_mutex_t* agentDeleteMutex = agent->deleteMutex;
            
            if ((checkTimeUSecs - agent->getLastRecvTimeUsecs()) > AGENT_SILENCE_THRESHOLD_USECS 
            	&& agent->getType() != AGENT_TYPE_VOXEL
                && pthread_mutex_trylock(agentDeleteMutex) == 0) {
                
                printLog("Killing agent - ");
                Agent::printLog(*agent);
                
                // make sure the vector isn't currently adding an agent
                pthread_mutex_lock(&vectorChangeMutex);
                agent = agents->erase(agent);
                pthread_mutex_unlock(&vectorChangeMutex);
                
                // release the delete mutex and destroy it
                pthread_mutex_unlock(agentDeleteMutex);
                pthread_mutex_destroy(agentDeleteMutex);
            } else {
                agent++;
            }
        }
        
        sleepTime = AGENT_SILENCE_THRESHOLD_USECS - (usecTimestampNow() - checkTimeUSecs);
        #ifdef _WIN32
        Sleep( static_cast<int>(1000.0f*sleepTime) );
        #else
        usleep(sleepTime);
        #endif
    }
    
    pthread_exit(0);
    return NULL;
}

void AgentList::startSilentAgentRemovalThread() {
    pthread_create(&removeSilentAgentsThread, NULL, removeSilentAgents, (void *)&agents);
}

void AgentList::stopSilentAgentRemovalThread() {
    silentAgentThreadStopFlag = true;
    pthread_join(removeSilentAgentsThread, NULL);
}

void *checkInWithDomainServer(void *args) {
    
    const int DOMAIN_SERVER_CHECK_IN_USECS = 1 * 1000000;
    
    AgentList *parentAgentList = (AgentList *)args;
    
    timeval lastSend;
    unsigned char output[7];
    
    in_addr_t localAddress = getLocalAddress();
    
    //  Lookup the IP address of the domain server if we need to
    if (atoi(DOMAIN_IP) == 0) {
        struct hostent* pHostInfo;
        if ((pHostInfo = gethostbyname(DOMAIN_HOSTNAME)) != NULL) {
            sockaddr_in tempAddress;
            memcpy(&tempAddress.sin_addr, pHostInfo->h_addr_list[0], pHostInfo->h_length);
            strcpy(DOMAIN_IP, inet_ntoa(tempAddress.sin_addr));
            printLog("Domain server: %s \n", DOMAIN_HOSTNAME);
            
        } else {
            printLog("Failed lookup domainserver\n");
        }
    } else printLog("Using static domainserver IP: %s\n", DOMAIN_IP);
    
    
    while (!domainServerCheckinStopFlag) {
        gettimeofday(&lastSend, NULL);
        
        output[0] = parentAgentList->getOwnerType();
        packSocket(output + 1, localAddress, htons(parentAgentList->getSocketListenPort()));
        
        parentAgentList->getAgentSocket().send(DOMAIN_IP, DOMAINSERVER_PORT, output, 7);
        
        double usecToSleep = DOMAIN_SERVER_CHECK_IN_USECS - (usecTimestampNow() - usecTimestamp(&lastSend));
        
        if (usecToSleep > 0) {
            usleep(usecToSleep);
        }
    }
    
    pthread_exit(0);
    return NULL;
}

void AgentList::startDomainServerCheckInThread() {
    pthread_create(&checkInWithDomainServerThread, NULL, checkInWithDomainServer, (void *)this);
}

void AgentList::stopDomainServerCheckInThread() {
    domainServerCheckinStopFlag = true;
    pthread_join(checkInWithDomainServerThread, NULL);
}

int unpackAgentId(unsigned char *packedData, uint16_t *agentId) {
    memcpy(agentId, packedData, sizeof(uint16_t));
    return sizeof(uint16_t);
}

int packAgentId(unsigned char *packStore, uint16_t agentId) {
    memcpy(packStore, &agentId, sizeof(uint16_t));
    return sizeof(uint16_t);
}
