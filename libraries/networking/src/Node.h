//
//  Node.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Node_h
#define hifi_Node_h

#include <ostream>
#include <stdint.h>

#include <QtCore/QDebug>
#include <QtCore/QMutex>
#include <QtCore/QUuid>
#include <QMutex>

#include "HifiSockAddr.h"
#include "NodeData.h"
#include "SimpleMovingAverage.h"
#include "MovingPercentile.h"

typedef quint8 NodeType_t;

namespace NodeType {
    const NodeType_t DomainServer = 'D';
    const NodeType_t VoxelServer = 'V';
    const NodeType_t ParticleServer = 'P';
    const NodeType_t ModelServer = 'o';
    const NodeType_t MetavoxelServer = 'm';
    const NodeType_t EnvironmentServer = 'E';
    const NodeType_t Agent = 'I';
    const NodeType_t AudioMixer = 'M';
    const NodeType_t AvatarMixer = 'W';
    const NodeType_t AnimationServer = 'a';
    const NodeType_t Unassigned = 1;
    
    void init();
    const QString& getNodeTypeName(NodeType_t nodeType);
}

class Node : public QObject {
    Q_OBJECT
public:
    Node(const QUuid& uuid, NodeType_t type, const HifiSockAddr& publicSocket, const HifiSockAddr& localSocket);
    ~Node();

    bool operator==(const Node& otherNode) const { return _uuid == otherNode._uuid; }
    bool operator!=(const Node& otherNode) const { return !(*this == otherNode); }

    char getType() const { return _type; }
    void setType(char type) { _type = type; }

    const QUuid& getUUID() const { return _uuid; }
    void setUUID(const QUuid& uuid) { _uuid = uuid; }

    quint64 getWakeTimestamp() const { return _wakeTimestamp; }
    void setWakeTimestamp(quint64 wakeTimestamp) { _wakeTimestamp = wakeTimestamp; }

    quint64 getLastHeardMicrostamp() const { return _lastHeardMicrostamp; }
    void setLastHeardMicrostamp(quint64 lastHeardMicrostamp) { _lastHeardMicrostamp = lastHeardMicrostamp; }

    const HifiSockAddr& getPublicSocket() const { return _publicSocket; }
    void setPublicSocket(const HifiSockAddr& publicSocket);
    const HifiSockAddr& getLocalSocket() const { return _localSocket; }
    void setLocalSocket(const HifiSockAddr& localSocket);
    const HifiSockAddr& getSymmetricSocket() const { return _symmetricSocket; }
    void setSymmetricSocket(const HifiSockAddr& symmetricSocket);
    
    const HifiSockAddr* getActiveSocket() const { return _activeSocket; }

    void activatePublicSocket();
    void activateLocalSocket();
    void activateSymmetricSocket();
    
    const QUuid& getConnectionSecret() const { return _connectionSecret; }
    void setConnectionSecret(const QUuid& connectionSecret) { _connectionSecret = connectionSecret; }

    NodeData* getLinkedData() const { return _linkedData; }
    void setLinkedData(NodeData* linkedData) { _linkedData = linkedData; }

    bool isAlive() const { return _isAlive; }
    void setAlive(bool isAlive) { _isAlive = isAlive; }

    void  recordBytesReceived(int bytesReceived);
    float getAverageKilobitsPerSecond();
    float getAveragePacketsPerSecond();

    int getPingMs() const { return _pingMs; }
    void setPingMs(int pingMs) { _pingMs = pingMs; }

    int getClockSkewUsec() const { return _clockSkewUsec; }
    void updateClockSkewUsec(int clockSkewSample);
    QMutex& getMutex() { return _mutex; }
    
    friend QDataStream& operator<<(QDataStream& out, const Node& node);
    friend QDataStream& operator>>(QDataStream& in, Node& node);

private:
    // privatize copy and assignment operator to disallow Node copying
    Node(const Node &otherNode);
    Node& operator=(Node otherNode);

    NodeType_t _type;
    QUuid _uuid;
    quint64 _wakeTimestamp;
    quint64 _lastHeardMicrostamp;
    HifiSockAddr _publicSocket;
    HifiSockAddr _localSocket;
    HifiSockAddr _symmetricSocket;
    HifiSockAddr* _activeSocket;
    QUuid _connectionSecret;
    SimpleMovingAverage* _bytesReceivedMovingAverage;
    NodeData* _linkedData;
    bool _isAlive;
    int _pingMs;
    int _clockSkewUsec;
    QMutex _mutex;
    MovingPercentile _clockSkewMovingPercentile;
};

QDebug operator<<(QDebug debug, const Node &message);

#endif // hifi_Node_h
