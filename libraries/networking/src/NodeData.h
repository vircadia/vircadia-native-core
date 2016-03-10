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
#include <QtCore/QSharedPointer>

#include "NLPacket.h"
#include "ReceivedMessage.h"

class Node;

class NodeData : public QObject {
    Q_OBJECT
public:
    NodeData(const QUuid& nodeID = QUuid());
    virtual ~NodeData() = 0;
    virtual int parseData(ReceivedMessage& message) { return 0; }

    const QUuid& getNodeID() const { return _nodeID; }
    
    QMutex& getMutex() { return _mutex; }

private:
    QMutex _mutex;
    QUuid _nodeID;
};

#endif // hifi_NodeData_h
