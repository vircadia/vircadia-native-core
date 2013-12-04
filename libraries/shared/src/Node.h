//
//  Node.h
//  hifi
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__Node__
#define __hifi__Node__

#include <ostream>
#include <stdint.h>

#ifdef _WIN32
#include "Syssocket.h"
#else
#include <sys/socket.h>
#endif

#include <QtCore/QDebug>
#include <QtCore/QUuid>

#include "HifiSockAddr.h"
#include "NodeData.h"
#include "SimpleMovingAverage.h"

class Node {
public:
    Node(const QUuid& uuid, char type, const HifiSockAddr& publicSocket, const HifiSockAddr& localSocket);
    ~Node();
    
    bool operator==(const Node& otherNode) const { return _uuid == otherNode._uuid; }
    bool operator!=(const Node& otherNode) const { return !(*this == otherNode); }
    
    char getType() const { return _type; }
    void setType(char type) { _type = type; }
    const char* getTypeName() const;
    
    const QUuid& getUUID() const { return _uuid; }
    void setUUID(const QUuid& uuid) { _uuid = uuid; }
    
    uint64_t getWakeMicrostamp() const { return _wakeMicrostamp; }
    void setWakeMicrostamp(uint64_t wakeMicrostamp) { _wakeMicrostamp = wakeMicrostamp; }
    
    uint64_t getLastHeardMicrostamp() const { return _lastHeardMicrostamp; }
    void setLastHeardMicrostamp(uint64_t lastHeardMicrostamp) { _lastHeardMicrostamp = lastHeardMicrostamp; }
    
    const HifiSockAddr& getPublicSocket() const { return _publicSocket; }
    void setPublicSocket(const HifiSockAddr& publicSocket);
    const HifiSockAddr& getLocalSocket() const { return _localSocket; }
    void setLocalSocket(const HifiSockAddr& localSocket);
    
    const HifiSockAddr* getActiveSocket() const { return _activeSocket; }
    
    void activatePublicSocket();
    void activateLocalSocket();
    
    NodeData* getLinkedData() const { return _linkedData; }
    void setLinkedData(NodeData* linkedData) { _linkedData = linkedData; }
    
    bool isAlive() const { return _isAlive; }
    void setAlive(bool isAlive) { _isAlive = isAlive; }
    
    void  recordBytesReceived(int bytesReceived);
    float getAverageKilobitsPerSecond();
    float getAveragePacketsPerSecond();

    int getPingMs() const { return _pingMs; }
    void setPingMs(int pingMs) { _pingMs = pingMs; }
    
    void lock() { pthread_mutex_lock(&_mutex); }
    
    /// returns false if lock failed, true if you got the lock
    bool trylock() { return (pthread_mutex_trylock(&_mutex) == 0); }
    void unlock() { pthread_mutex_unlock(&_mutex); }

    static void printLog(Node const&);
    
private:
    // privatize copy and assignment operator to disallow Node copying
    Node(const Node &otherNode);
    Node& operator=(Node otherNode);
    
    char _type;
    QUuid _uuid;
    uint64_t _wakeMicrostamp;
    uint64_t _lastHeardMicrostamp;
    HifiSockAddr _publicSocket;
    HifiSockAddr _localSocket;
    HifiSockAddr* _activeSocket;
    SimpleMovingAverage* _bytesReceivedMovingAverage;
    NodeData* _linkedData;
    bool _isAlive;
    int _pingMs;
    pthread_mutex_t _mutex;
};

int unpackNodeId(unsigned char *packedData, uint16_t *nodeId);
int packNodeId(unsigned char *packStore, uint16_t nodeId);

QDebug operator<<(QDebug debug, const Node &message);

#endif /* defined(__hifi__Node__) */
