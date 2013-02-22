//
//  AgentList.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "AgentList.h"
#include <arpa/inet.h>
#include <pthread.h>
#include "SharedUtil.h"

bool stopAgentRemovalThread = false;
pthread_mutex_t vectorChangeMutex = PTHREAD_MUTEX_INITIALIZER;

AgentList::AgentList() : agentSocket(AGENT_SOCKET_LISTEN_PORT) {
    linkedDataCreateCallback = NULL;
    audioMixerSocketUpdate = NULL;
}

AgentList::AgentList(int socketListenPort) : agentSocket(socketListenPort) {
    linkedDataCreateCallback = NULL;
    audioMixerSocketUpdate = NULL;
}

AgentList::~AgentList() {
    stopSilentAgentRemovalThread();
}

UDPSocket * AgentList::getAgentSocket() {
    return &agentSocket;
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
            char reply[] = "R";
            agentSocket.send(senderAddress, reply, 1);
            break;
        }
        case 'R':
        {
            // ping reply from another agent
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
        
        matchingAgent->lastRecvTimeUsecs = usecTimestampNow();
        
        if (matchingAgent->linkedData == NULL) {
            if (linkedDataCreateCallback != NULL) {
                linkedDataCreateCallback(matchingAgent);
            }
        }
        
        matchingAgent->linkedData->parseData(packetData, dataBytes);
    }

}

int AgentList::indexOfMatchingAgent(sockaddr *senderAddress) {
    for(std::vector<Agent>::iterator agent = agents.begin(); agent != agents.end(); agent++) {
        if (agent->activeSocket != NULL && socketMatch(agent->activeSocket, senderAddress)) {
            return agent - agents.begin();
        }
    }
    
    return -1;
}

int AgentList::updateList(unsigned char *packetData, size_t dataBytes) {
    int readAgents = 0;

    char agentType;
    
    // assumes only IPv4 addresses
    sockaddr_in agentPublicSocket;
    agentPublicSocket.sin_family = AF_INET;
    sockaddr_in agentLocalSocket;
    agentLocalSocket.sin_family = AF_INET;
    
    unsigned char *readPtr = packetData + 1;
    unsigned char *startPtr = packetData;
    
    while((readPtr - startPtr) < dataBytes) {
        agentType = *readPtr++;
        readPtr += unpackSocket(readPtr, (sockaddr *)&agentPublicSocket);
        readPtr += unpackSocket(readPtr, (sockaddr *)&agentLocalSocket);
        
        addOrUpdateAgent((sockaddr *)&agentPublicSocket, (sockaddr *)&agentLocalSocket, agentType);
    }  

    return readAgents;
}

bool AgentList::addOrUpdateAgent(sockaddr *publicSocket, sockaddr *localSocket, char agentType) {
    std::vector<Agent>::iterator agent;
    
    for(agent = agents.begin(); agent != agents.end(); agent++) {
        if (agent->matches(publicSocket, localSocket, agentType)) {
            // we already have this agent, stop checking
            break;
        }
    }
    
    if (agent == agents.end()) {
        // we didn't have this agent, so add them
        Agent newAgent = Agent(publicSocket, localSocket, agentType);
        
        if (socketMatch(publicSocket, localSocket)) {
            // likely debugging scenario with DS + agent on local network
            // set the agent active right away
            newAgent.activeSocket = newAgent.localSocket;
        }
        
        if (newAgent.type == 'M' && audioMixerSocketUpdate != NULL) {
            // this is an audio mixer
            // for now that means we need to tell the audio class
            // to use the local socket information the domain server gave us
            sockaddr_in *localSocketIn = (sockaddr_in *)localSocket;
            audioMixerSocketUpdate(localSocketIn->sin_addr.s_addr, localSocketIn->sin_port);
        }
        
        std::cout << "Added agent - " << &newAgent << "\n";
        
        pthread_mutex_lock(&vectorChangeMutex);
        agents.push_back(newAgent);
        pthread_mutex_unlock(&vectorChangeMutex);
        
        return true;
    } else {
        
        if (agent->type == 'M') {
            // until the Audio class also uses our agentList, we need to update
            // the lastRecvTimeUsecs for the audio mixer so it doesn't get killed and re-added continously
            agent->lastRecvTimeUsecs = usecTimestampNow();
        }
        
        // we had this agent already, do nothing for now
        return false;
    }    
}

void AgentList::broadcastToAgents(char *broadcastData, size_t dataBytes) {
    for(std::vector<Agent>::iterator agent = agents.begin(); agent != agents.end(); agent++) {
        // for now assume we only want to send to other interface clients
        // until the Audio class uses the AgentList
        if (agent->activeSocket != NULL && agent->type == 'I') {
            // we know which socket is good for this agent, send there
            agentSocket.send(agent->activeSocket, broadcastData, dataBytes);
        }
    }
}

void AgentList::pingAgents() {
    char payload[] = "P";
    
    for(std::vector<Agent>::iterator agent = agents.begin(); agent != agents.end(); agent++) {
        if (agent->type == 'I') {
            if (agent->activeSocket != NULL) {
                // we know which socket is good for this agent, send there
                agentSocket.send(agent->activeSocket, payload, 1);
            } else {
                // ping both of the sockets for the agent so we can figure out
                // which socket we can use
                agentSocket.send(agent->publicSocket, payload, 1);
                agentSocket.send(agent->localSocket, payload, 1);
            }
        }
    }
}

void AgentList::handlePingReply(sockaddr *agentAddress) {
    for(std::vector<Agent>::iterator agent = agents.begin(); agent != agents.end(); agent++) {
        // check both the public and local addresses for each agent to see if we find a match
        // prioritize the private address so that we prune erroneous local matches
        sockaddr *matchedSocket = NULL;
        
        if (socketMatch(agent->publicSocket, agentAddress)) {
            matchedSocket = agent->publicSocket;
        } else if (socketMatch(agent->localSocket, agentAddress)) {
            matchedSocket = agent->localSocket;
        }
        
        if (matchedSocket != NULL) {
            // matched agent, stop checking
            // update the agent's ping
            agent->activeSocket = matchedSocket;
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
            if ((checkTimeUSecs - agent->lastRecvTimeUsecs) > AGENT_SILENCE_THRESHOLD_USECS) {
                std::cout << "Killing agent " << &(*agent)  << "\n";
                pthread_mutex_lock(&vectorChangeMutex);
                agent = agents->erase(agent);
                pthread_mutex_unlock(&vectorChangeMutex);
            } else {
                agent++;
            }
        }
        
        
        sleepTime = AGENT_SILENCE_THRESHOLD_USECS - (usecTimestampNow() - checkTimeUSecs);
        usleep(sleepTime);
    }
    
    pthread_exit(0);
}

void AgentList::startSilentAgentRemovalThread() {
    pthread_create(&removeSilentAgentsThread, NULL, removeSilentAgents, (void *)&agents);
}

void AgentList::stopSilentAgentRemovalThread() {
    stopAgentRemovalThread = true;
    pthread_join(removeSilentAgentsThread, NULL);
}