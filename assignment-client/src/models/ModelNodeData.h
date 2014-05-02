//
//  ModelNodeData.h
//  assignment-client/src/models
//
//  Created by Brad Hefta-Gaub on 4/29/14
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelNodeData_h
#define hifi_ModelNodeData_h

#include <PacketHeaders.h>

#include "../octree/OctreeQueryNode.h"

class ModelNodeData : public OctreeQueryNode {
public:
    ModelNodeData() :
        OctreeQueryNode(),
        _lastDeletedModelsSentAt(0) {  };

    virtual PacketType getMyPacketType() const { return PacketTypeModelData; }

    quint64 getLastDeletedModelsSentAt() const { return _lastDeletedModelsSentAt; }
    void setLastDeletedModelsSentAt(quint64 sentAt) { _lastDeletedModelsSentAt = sentAt; }

private:
    quint64 _lastDeletedModelsSentAt;
};

#endif // hifi_ModelNodeData_h
