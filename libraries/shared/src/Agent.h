//
//  Agent.h
//  hifi
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__Agent__
#define __hifi__Agent__

#include <stdint.h>
#include <ostream>

#ifdef _WIN32
#include "Syssocket.h"
#else
#include <sys/socket.h>
#endif

#include "SimpleMovingAverage.h"
#include "AgentData.h"

class Agent {    
public:
    Agent(sockaddr *agentPublicSocket, sockaddr *agentLocalSocket, char agentType, uint16_t thisAgentId);
    Agent(const Agent &otherAgent);
    ~Agent();
    Agent& operator=(Agent otherAgent);
    bool operator==(const Agent& otherAgent);
    
    bool matches(sockaddr *otherPublicSocket, sockaddr *otherLocalSocket, char otherAgentType);
    
    char getType() const;
    const char* getTypeName() const;
    void setType(char newType);
    
    uint16_t getAgentId();
    void setAgentId(uint16_t thisAgentId);
    
    double getWakeMicrostamp() const { return _wakeMicrostamp; }
    void setWakeMicrostamp(double wakeMicrostamp) { _wakeMicrostamp = wakeMicrostamp; }
    
    double getLastHeardMicrostamp() const { return _lastHeardMicrostamp; }
    void setLastHeardMicrostamp(double lastHeardMicrostamp) { _lastHeardMicrostamp = lastHeardMicrostamp; }
    
    sockaddr* getPublicSocket();
    void setPublicSocket(sockaddr *newSocket);
    sockaddr* getLocalSocket();
    void setLocalSocket(sockaddr *newSocket);
    sockaddr* getActiveSocket();
    
    void activatePublicSocket();
    void activateLocalSocket();
    
    AgentData* getLinkedData();
    void setLinkedData(AgentData *newData);
    
    bool isAlive() const { return _isAlive; };
    void setAlive(bool isAlive) { _isAlive = isAlive; };
    
    void  recordBytesReceived(int bytesReceived);
    float getAverageKilobitsPerSecond();
    float getAveragePacketsPerSecond();

    static void printLog(Agent const&);
private:
    void swap(Agent &first, Agent &second);
    
    sockaddr *publicSocket, *localSocket, *activeSocket;
    char type;
    uint16_t agentId;
    double _wakeMicrostamp;
    double _lastHeardMicrostamp;
    SimpleMovingAverage* _bytesReceivedMovingAverage;
    AgentData* linkedData;
    bool _isAlive;
};


int unpackAgentId(unsigned char *packedData, uint16_t *agentId);
int packAgentId(unsigned char *packStore, uint16_t agentId);

#endif /* defined(__hifi__Agent__) */
