//
//  AgentList.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "AgentList.h"
#include <pthread.h>
#include "SharedUtil.h"

#ifdef _WIN32
#include "Syssocket.h"
#else
#include <arpa/inet.h>
#endif

const char * SOLO_AGENT_TYPES_STRING = "MV";

bool stopAgentRemovalThread = false;
pthread_mutex_t vectorChangeMutex = PTHREAD_MUTEX_INITIALIZER;

AgentList::AgentList() : agentSocket(AGENT_SOCKET_LISTEN_PORT) {
    linkedDataCreateCallback = NULL;
    audioMixerSocketUpdate = NULL;
    lastAgentId = 1;
}

AgentList::AgentList(int socketListenPort) : agentSocket(socketListenPort) {
    linkedDataCreateCallback = NULL;
    audioMixerSocketUpdate = NULL;
    lastAgentId = 1;
    
}

AgentList::~AgentList() {
    stopSilentAgentRemovalThread();
}

std::vector<Agent>& AgentList::getAgents() {
    return agents;
}

UDPSocket& AgentList::getAgentSocket() {
    return agentSocket;
}

void AgentList::processAgentData(sockaddr *senderAddress, void *packetData, size_t dataBytes) {
    switch (((char *)packetData)[0]) {
        case 'D':
        {
            // list of agents from domain server
            updateList((unsigned char *)packetData, dataBytes);
            break;
        }
        case 'H':
        {
            // head data from another agent
            updateAgentWithData(senderAddress, packetData, dataBytes);
            break;
        }
        case 'P':
        {
            // ping from another agent
            //std::cout << "Got ping from " << inet_ntoa(((sockaddr_in *)senderAddress)->sin_addr) << "\n";
            char reply[] = "R";
            agentSocket.send(senderAddress, reply, 1);  
            break;
        }
        case 'R':
        {
            // ping reply from another agent
            //std::cout << "Got ping reply from " << inet_ntoa(((sockaddr_in *)senderAddress)->sin_addr) << "\n";
            handlePingReply(senderAddress);
            break;
        }
    }
}

void AgentList::updateAgentWithData(sockaddr *senderAddress, void *packetData, size_t dataBytes) {
    // find the agent by the sockaddr
    int agentIndex = indexOfMatchingAgent(senderAddress);
    
    if (agentIndex != -1) {
        Agent *matchingAgent = &agents[agentIndex];
        
        matchingAgent->setLastRecvTimeUsecs(usecTimestampNow());
        
        if (matchingAgent->getLinkedData() == NULL) {
            if (linkedDataCreateCallback != NULL) {
                linkedDataCreateCallback(matchingAgent);
            }
        }
        
        matchingAgent->getLinkedData()->parseData(packetData, dataBytes);
    }

}

int AgentList::indexOfMatchingAgent(sockaddr *senderAddress) {
    for(std::vector<Agent>::iterator agent = agents.begin(); agent != agents.end(); agent++) {
        if (agent->getActiveSocket() != NULL && socketMatch(agent->getActiveSocket(), senderAddress)) {
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

bool AgentList::addOrUpdateAgent(sockaddr *publicSocket, sockaddr *localSocket, char agentType, uint16_t agentId = 0) {
    std::vector<Agent>::iterator agent;
    uint16_t thisAgentId;
    
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
            // likely debugging scenario with DS + agent on local network
            // set the agent active right away
            newAgent.activatePublicSocket();
        }
        
        if (newAgent.getType() == 'M' && audioMixerSocketUpdate != NULL) {
            // this is an audio mixer
            // for now that means we need to tell the audio class
            // to use the local socket information the domain server gave us
            sockaddr_in *localSocketIn = (sockaddr_in *)localSocket;
            audioMixerSocketUpdate(localSocketIn->sin_addr.s_addr, localSocketIn->sin_port);
        } else if (newAgent.getType() == 'V') {
            newAgent.activateLocalSocket();
        }
        
        std::cout << "Added agent - " << &newAgent << "\n";
        
        pthread_mutex_lock(&vectorChangeMutex);
        agents.push_back(newAgent);
        pthread_mutex_unlock(&vectorChangeMutex);
        
        return true;
    } else {
        
        if (agent->getType() == 'M' || agent->getType() == 'V') {
            // until the Audio class also uses our agentList, we need to update
            // the lastRecvTimeUsecs for the audio mixer so it doesn't get killed and re-added continously
            agent->setLastRecvTimeUsecs(usecTimestampNow());
        }
        
        // we had this agent already, do nothing for now
        return false;
    }    
}

void AgentList::broadcastToAgents(char *broadcastData, size_t dataBytes) {
    for(std::vector<Agent>::iterator agent = agents.begin(); agent != agents.end(); agent++) {
        // for now assume we only want to send to other interface clients
        // until the Audio class uses the AgentList
        if (agent->getActiveSocket() != NULL && (agent->getType() == 'I' || agent->getType() == 'V')) {
            // we know which socket is good for this agent, send there
            agentSocket.send(agent->getActiveSocket(), broadcastData, dataBytes);
        }
    }
}

void AgentList::pingAgents() {
    char payload[] = "P";
    
    for(std::vector<Agent>::iterator agent = agents.begin(); agent != agents.end(); agent++) {
        if (agent->getType() == 'I') {
            if (agent->getActiveSocket() != NULL) {
                // we know which socket is good for this agent, send there
                agentSocket.send(agent->getActiveSocket(), payload, 1);
            } else {
                // ping both of the sockets for the agent so we can figure out
                // which socket we can use
                agentSocket.send(agent->getPublicSocket(), payload, 1);
                agentSocket.send(agent->getLocalSocket(), payload, 1);
            }
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

void *removeSilentAgents(void *args) {
    std::vector<Agent> *agents = (std::vector<Agent> *)args;
    double checkTimeUSecs, sleepTime;
    
    while (!stopAgentRemovalThread) {
        checkTimeUSecs = usecTimestampNow();
        
        for(std::vector<Agent>::iterator agent = agents->begin(); agent != agents->end();) {
            if ((checkTimeUSecs - agent->getLastRecvTimeUsecs()) > AGENT_SILENCE_THRESHOLD_USECS && agent->getType() != 'V') {
                std::cout << "Killing agent " << &(*agent)  << "\n";
                pthread_mutex_lock(&vectorChangeMutex);
                agent = agents->erase(agent);
                pthread_mutex_unlock(&vectorChangeMutex);
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
    stopAgentRemovalThread = true;
    pthread_join(removeSilentAgentsThread, NULL);
}

int unpackAgentId(unsigned char *packedData, uint16_t *agentId) {
    memcpy(packedData, agentId, sizeof(uint16_t));
    return sizeof(uint16_t);
}

int packAgentId(unsigned char *packStore, uint16_t agentId) {
    memcpy(&agentId, packStore, sizeof(uint16_t));
    return sizeof(uint16_t);
}