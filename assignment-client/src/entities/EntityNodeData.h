//
//  EntityNodeData.h
//  assignment-client/src/models
//
//  Created by Brad Hefta-Gaub on 4/29/14
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityNodeData_h
#define hifi_EntityNodeData_h

#include <PacketHeaders.h>

#include "../octree/OctreeQueryNode.h"

class EntityNodeData : public OctreeQueryNode {
public:
    EntityNodeData() :
        OctreeQueryNode(),
        _lastDeletedEntitysSentAt(0) {  };

    virtual PacketType getMyPacketType() const { return PacketTypeEntityData; }

    quint64 getLastDeletedEntitysSentAt() const { return _lastDeletedEntitysSentAt; }
    void setLastDeletedEntitysSentAt(quint64 sentAt) { _lastDeletedEntitysSentAt = sentAt; }

private:
    quint64 _lastDeletedEntitysSentAt;
};

#endif // hifi_EntityNodeData_h
