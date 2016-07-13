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

#include <memory>
#include <ostream>
#include <stdint.h>

#include <QtCore/QDebug>
#include <QtCore/QMutex>
#include <QtCore/QSharedPointer>
#include <QtCore/QUuid>

#include <UUIDHasher.h>

#include <tbb/concurrent_unordered_set.h>

#include "HifiSockAddr.h"
#include "NetworkPeer.h"
#include "NodeData.h"
#include "NodeType.h"
#include "SimpleMovingAverage.h"
#include "MovingPercentile.h"
#include "NodePermissions.h"

class Node : public NetworkPeer {
    Q_OBJECT
public:
    Node(const QUuid& uuid, NodeType_t type,
         const HifiSockAddr& publicSocket, const HifiSockAddr& localSocket,
         const NodePermissions& permissions, const QUuid& connectionSecret = QUuid(),
         QObject* parent = 0);

    bool operator==(const Node& otherNode) const { return _uuid == otherNode._uuid; }
    bool operator!=(const Node& otherNode) const { return !(*this == otherNode); }

    char getType() const { return _type; }
    void setType(char type);

    const QUuid& getConnectionSecret() const { return _connectionSecret; }
    void setConnectionSecret(const QUuid& connectionSecret) { _connectionSecret = connectionSecret; }

    NodeData* getLinkedData() const { return _linkedData.get(); }
    void setLinkedData(std::unique_ptr<NodeData> linkedData) { _linkedData = std::move(linkedData); }

    bool isAlive() const { return _isAlive; }
    void setAlive(bool isAlive) { _isAlive = isAlive; }

    int getPingMs() const { return _pingMs; }
    void setPingMs(int pingMs) { _pingMs = pingMs; }

    qint64 getClockSkewUsec() const { return _clockSkewUsec; }
    void updateClockSkewUsec(qint64 clockSkewSample);
    QMutex& getMutex() { return _mutex; }

    void setPermissions(const NodePermissions& newPermissions) { _permissions = newPermissions; }
    NodePermissions getPermissions() const { return _permissions; }
    bool isAllowedEditor() const { return _permissions.canAdjustLocks; }
    bool getCanRez() const { return _permissions.canRezPermanentEntities; }
    bool getCanRezTmp() const { return _permissions.canRezTemporaryEntities; }
    bool getCanWriteToAssetServer() const { return _permissions.canWriteToAssetServer; }

    void parseIgnoreRequestMessage(QSharedPointer<ReceivedMessage> message);
    void addIgnoredNode(const QUuid& otherNodeID);
    bool isIgnoringNodeWithID(const QUuid& nodeID) const { return _ignoredNodeIDSet.find(nodeID) != _ignoredNodeIDSet.cend(); }

    friend QDataStream& operator<<(QDataStream& out, const Node& node);
    friend QDataStream& operator>>(QDataStream& in, Node& node);

private:
    // privatize copy and assignment operator to disallow Node copying
    Node(const Node &otherNode);
    Node& operator=(Node otherNode);

    NodeType_t _type;

    QUuid _connectionSecret;
    std::unique_ptr<NodeData> _linkedData;
    bool _isAlive;
    int _pingMs;
    qint64 _clockSkewUsec;
    QMutex _mutex;
    MovingPercentile _clockSkewMovingPercentile;
    NodePermissions _permissions;
    tbb::concurrent_unordered_set<QUuid, UUIDHasher> _ignoredNodeIDSet;
};

Q_DECLARE_METATYPE(Node*)

typedef QSharedPointer<Node> SharedNodePointer;
Q_DECLARE_METATYPE(SharedNodePointer)

namespace std {
    template<>
    struct hash<SharedNodePointer> {
        size_t operator()(const SharedNodePointer& p) const {
            // Return the hash of the pointer
            return hash<Node*>()(p.data());
        }
    };
}

QDebug operator<<(QDebug debug, const Node& node);

#endif // hifi_Node_h
