//
//  Node.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <cstring>
#include <pthread.h>
#include <stdio.h>

#ifdef _WIN32
#include "Syssocket.h"
#else
#include <arpa/inet.h>
#endif

#include "Node.h"
#include "NodeTypes.h"
#include "SharedUtil.h"
#include "UDPSocket.h"

#include <QDebug>

int unpackNodeId(unsigned char* packedData, uint16_t* nodeId) {
    memcpy(nodeId, packedData, sizeof(uint16_t));
    return sizeof(uint16_t);
}

int packNodeId(unsigned char* packStore, uint16_t nodeId) {
    memcpy(packStore, &nodeId, sizeof(uint16_t));
    return sizeof(uint16_t);
}

Node::Node(sockaddr* publicSocket, sockaddr* localSocket, char type, uint16_t nodeID) :
    _type(type),
    _nodeID(nodeID),
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
    
    pthread_mutex_init(&_mutex, 0);
}

Node::~Node() {
    delete _publicSocket;
    delete _localSocket;
    delete _linkedData;
    delete _bytesReceivedMovingAverage;
    
    pthread_mutex_destroy(&_mutex);
}

// Names of Node Types
const char* NODE_TYPE_NAME_DOMAIN = "Domain";
const char* NODE_TYPE_NAME_VOXEL_SERVER = "Voxel Server";
const char* NODE_TYPE_NAME_INTERFACE = "Client Interface";
const char* NODE_TYPE_NAME_AUDIO_MIXER = "Audio Mixer";
const char* NODE_TYPE_NAME_AVATAR_MIXER = "Avatar Mixer";
const char* NODE_TYPE_NAME_AUDIO_INJECTOR = "Audio Injector";
const char* NODE_TYPE_NAME_ANIMATION_SERVER = "Animation Server";
const char* NODE_TYPE_NAME_UNKNOWN = "Unknown";

const char* Node::getTypeName() const {
	switch (this->_type) {
		case NODE_TYPE_DOMAIN:
			return NODE_TYPE_NAME_DOMAIN;
		case NODE_TYPE_VOXEL_SERVER:
			return NODE_TYPE_NAME_VOXEL_SERVER;
		case NODE_TYPE_AGENT:
			return NODE_TYPE_NAME_INTERFACE;
		case NODE_TYPE_AUDIO_MIXER:
			return NODE_TYPE_NAME_AUDIO_MIXER;
        case NODE_TYPE_AVATAR_MIXER:
            return NODE_TYPE_NAME_AVATAR_MIXER;
        case NODE_TYPE_AUDIO_INJECTOR:
            return NODE_TYPE_NAME_AUDIO_INJECTOR;
        case NODE_TYPE_ANIMATION_SERVER:
            return NODE_TYPE_NAME_ANIMATION_SERVER;
        default:
            return NODE_TYPE_NAME_UNKNOWN;
	}
}

void Node::activateLocalSocket() {
    _activeSocket = _localSocket;
}

void Node::activatePublicSocket() {
    _activeSocket = _publicSocket;
}

bool Node::operator==(const Node& otherNode) {
    return matches(otherNode._publicSocket, otherNode._localSocket, otherNode._type);
}

bool Node::matches(sockaddr* otherPublicSocket, sockaddr* otherLocalSocket, char otherNodeType) {
    // checks if two node objects are the same node (same type + local + public address)
    return _type == otherNodeType
        && socketMatch(_publicSocket, otherPublicSocket)
        && socketMatch(_localSocket, otherLocalSocket);
}

void Node::recordBytesReceived(int bytesReceived) {
    if (_bytesReceivedMovingAverage == NULL) {
        _bytesReceivedMovingAverage = new SimpleMovingAverage(100);
    }
    
    _bytesReceivedMovingAverage->updateAverage((float) bytesReceived);
}

float Node::getAveragePacketsPerSecond() {
    if (_bytesReceivedMovingAverage) {
        return (1 / _bytesReceivedMovingAverage->getEventDeltaAverage());
    } else {
        return 0;
    }
}

float Node::getAverageKilobitsPerSecond() {
    if (_bytesReceivedMovingAverage) {
        return (_bytesReceivedMovingAverage->getAverageSampleValuePerSecond() * (8.0f / 1000));
    } else {
        return 0;
    }
}

QDebug operator<<(QDebug debug, const Node &node) {
    char publicAddressBuffer[16] = {'\0'};
    unsigned short publicAddressPort = loadBufferWithSocketInfo(publicAddressBuffer, node.getPublicSocket());
    
    //char localAddressBuffer[16] = {'\0'};
    //unsigned short localAddressPort = loadBufferWithSocketInfo(localAddressBuffer, node.localSocket);
    
    debug << "#" << node.getNodeID() << node.getTypeName() << node.getType();
    debug.nospace() << publicAddressBuffer << ":" << publicAddressPort;
    return debug.space();
}
