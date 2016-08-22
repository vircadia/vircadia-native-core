//
//  EntityNodeData.h
//  assignment-client/src/entities
//
//  Created by Brad Hefta-Gaub on 4/29/14
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityNodeData_h
#define hifi_EntityNodeData_h

#include <udt/PacketHeaders.h>

#include "../octree/OctreeQueryNode.h"

class EntityNodeData : public OctreeQueryNode {
public:
    virtual PacketType getMyPacketType() const override { return PacketType::EntityData; }

    quint64 getLastDeletedEntitiesSentAt() const { return _lastDeletedEntitiesSentAt; }
    void setLastDeletedEntitiesSentAt(quint64 sentAt) { _lastDeletedEntitiesSentAt = sentAt; }

private:
    quint64 _lastDeletedEntitiesSentAt { usecTimestampNow() };
};

#endif // hifi_EntityNodeData_h
