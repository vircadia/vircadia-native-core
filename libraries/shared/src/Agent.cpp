//
//  Agent.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <pthread.h>
#include "Agent.h"
#include "AgentTypes.h"
#include <cstring>
#include "shared_Log.h"
#include "UDPSocket.h"
#include "SharedUtil.h"

#ifdef _WIN32
#include "Syssocket.h"
#else
#include <arpa/inet.h>
#endif

using shared_lib::printLog;

int unpackAgentId(unsigned char* packedData, uint16_t* agentId) {
    memcpy(agentId, packedData, sizeof(uint16_t));
    return sizeof(uint16_t);
}

int packAgentId(unsigned char* packStore, uint16_t agentId) {
    memcpy(packStore, &agentId, sizeof(uint16_t));
    return sizeof(uint16_t);
}

Agent::Agent(sockaddr* publicSocket, sockaddr* localSocket, char type, uint16_t agentID) :
    _type(type),
    _agentID(agentID),
    _wakeMicrostamp(usecTimestampNow()),
    _lastHeardMicrostamp(usecTimestampNow()),
    _activeSocket(NULL),
    _bytesReceivedMovingAverage(NULL),
    _linkedData(NULL),
    _isAlive(true)
{
    if (publicSocket) {
        _publicSocket = new sockaddr(*publicSocket);
    } else {
        _publicSocket = NULL;
    }
    
    if (localSocket) {
        _localSocket = new sockaddr(*localSocket);
    } else {
        _localSocket = NULL;
    }
}

Agent::Agent(const Agent &otherAgent) :
    _type(otherAgent._type),
    _agentID(otherAgent._agentID),
    _wakeMicrostamp(otherAgent._wakeMicrostamp),
    _lastHeardMicrostamp(otherAgent._lastHeardMicrostamp),
    _isAlive(otherAgent._isAlive)
{
    if (otherAgent._publicSocket) {
        _publicSocket = new sockaddr(*otherAgent._localSocket);
    } else {
        _publicSocket = NULL;
    }
    
    if (otherAgent._localSocket) {
        _localSocket = new sockaddr(*otherAgent._localSocket);
    } else {
        _localSocket = NULL;
    }
    
    
    if (otherAgent._activeSocket == otherAgent._publicSocket) {
        _activeSocket = _publicSocket;
    } else if (otherAgent._activeSocket == otherAgent._localSocket) {
        _activeSocket = _localSocket;
    } else {
        _activeSocket = NULL;
    }
    
    if (otherAgent._linkedData) {
        _linkedData = otherAgent._linkedData->clone();
    } else {
        _linkedData = NULL;
    }
    
    if (otherAgent._bytesReceivedMovingAverage != NULL) {
        _bytesReceivedMovingAverage = new SimpleMovingAverage(100);
        memcpy(_bytesReceivedMovingAverage, otherAgent._bytesReceivedMovingAverage, sizeof(SimpleMovingAverage));
    } else {
        _bytesReceivedMovingAverage = NULL;
    }
}

Agent& Agent::operator=(Agent otherAgent) {
    swap(*this, otherAgent);
    return *this;
}

void Agent::swap(Agent &first, Agent &second) {
    using std::swap;
    
    swap(first._isAlive, second._isAlive);
    swap(first._publicSocket, second._publicSocket);
    swap(first._localSocket, second._localSocket);
    swap(first._activeSocket, second._activeSocket);
    swap(first._type, second._type);
    swap(first._linkedData, second._linkedData);
    swap(first._agentID, second._agentID);
    swap(first._wakeMicrostamp, second._wakeMicrostamp);
    swap(first._lastHeardMicrostamp, second._lastHeardMicrostamp);
    swap(first._bytesReceivedMovingAverage, second._bytesReceivedMovingAverage);
}

Agent::~Agent() {
    delete _publicSocket;
    delete _localSocket;
    delete _linkedData;
    delete _bytesReceivedMovingAverage;
}

// Names of Agent Types
const char* AGENT_TYPE_NAME_DOMAIN = "Domain";
const char* AGENT_TYPE_NAME_VOXEL = "Voxel Server";
const char* AGENT_TYPE_NAME_INTERFACE = "Client Interface";
const char* AGENT_TYPE_NAME_AUDIO_MIXER = "Audio Mixer";
const char* AGENT_TYPE_NAME_AVATAR_MIXER = "Avatar Mixer";
const char* AGENT_TYPE_NAME_AUDIO_INJECTOR = "Audio Injector";
const char* AGENT_TYPE_NAME_UNKNOWN = "Unknown";

const char* Agent::getTypeName() const {
	switch (this->_type) {
		case AGENT_TYPE_DOMAIN:
			return AGENT_TYPE_NAME_DOMAIN;
		case AGENT_TYPE_VOXEL:
			return AGENT_TYPE_NAME_VOXEL;
		case AGENT_TYPE_AVATAR:
			return AGENT_TYPE_NAME_INTERFACE;
		case AGENT_TYPE_AUDIO_MIXER:
			return AGENT_TYPE_NAME_AUDIO_MIXER;
        case AGENT_TYPE_AVATAR_MIXER:
            return AGENT_TYPE_NAME_AVATAR_MIXER;
        case AGENT_TYPE_AUDIO_INJECTOR:
            return AGENT_TYPE_NAME_AUDIO_INJECTOR;
        default:
            return AGENT_TYPE_NAME_UNKNOWN;
	}
}

void Agent::activateLocalSocket() {
    _activeSocket = _localSocket;
}

void Agent::activatePublicSocket() {
    _activeSocket = _publicSocket;
}

bool Agent::operator==(const Agent& otherAgent) {
    return matches(otherAgent._publicSocket, otherAgent._localSocket, otherAgent._type);
}

bool Agent::matches(sockaddr *otherPublicSocket, sockaddr *otherLocalSocket, char otherAgentType) {
    // checks if two agent objects are the same agent (same type + local + public address)
    return _type == otherAgentType
        && socketMatch(_publicSocket, otherPublicSocket)
        && socketMatch(_localSocket, otherLocalSocket);
}

void Agent::recordBytesReceived(int bytesReceived) {
    if (_bytesReceivedMovingAverage == NULL) {
        _bytesReceivedMovingAverage = new SimpleMovingAverage(100);
    }
    
    _bytesReceivedMovingAverage->updateAverage((float) bytesReceived);
}

float Agent::getAveragePacketsPerSecond() {
    if (_bytesReceivedMovingAverage != NULL) {
        return (1 / _bytesReceivedMovingAverage->getEventDeltaAverage());
    } else {
        return 0;
    }
}

float Agent::getAverageKilobitsPerSecond() {
    if (_bytesReceivedMovingAverage != NULL) {
        return (_bytesReceivedMovingAverage->getAverageSampleValuePerSecond() * (8.0f / 1000));
    } else {
        return 0;
    }
}

void Agent::printLog(Agent const& agent) {
    
    char publicAddressBuffer[16] = {'\0'};
    unsigned short publicAddressPort = loadBufferWithSocketInfo(publicAddressBuffer, agent._publicSocket);
    
    //char localAddressBuffer[16] = {'\0'};
    //unsigned short localAddressPort = loadBufferWithSocketInfo(localAddressBuffer, agent.localSocket);
    
    ::printLog("# %d %s (%c) @ %s:%d\n",
               agent._agentID,
               agent.getTypeName(),
               agent._type,
               publicAddressBuffer,
               publicAddressPort);
}