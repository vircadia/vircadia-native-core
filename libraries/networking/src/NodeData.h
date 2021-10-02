//
//  NodeData.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2/19/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_NodeData_h
#define hifi_NodeData_h

#include <QtCore/QMutex>
#include <QtCore/QObject>

#include "NetworkPeer.h"
#include "NLPacket.h"
#include "ReceivedMessage.h"

class Node;

class NodeData : public QObject {
    Q_OBJECT
public:
    NodeData(const QUuid& nodeID = QUuid(), NetworkPeer::LocalID localID = NetworkPeer::NULL_LOCAL_ID);
    virtual ~NodeData() = default;
    virtual int parseData(ReceivedMessage& message) { return 0; }

    const QUuid& getNodeID() const { return _nodeID; }
    NetworkPeer::LocalID getNodeLocalID() const { return _nodeLocalID; }
    
    QMutex& getMutex() { return _mutex; }

private:
    QMutex _mutex;
    QUuid _nodeID;
    NetworkPeer::LocalID _nodeLocalID;
};

#endif // hifi_NodeData_h
