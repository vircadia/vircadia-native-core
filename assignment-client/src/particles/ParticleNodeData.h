//
//  ParticleNodeData.h
//  assignment-client/src/particles
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ParticleNodeData_h
#define hifi_ParticleNodeData_h

#include <PacketHeaders.h>

#include "../octree/OctreeQueryNode.h"

class ParticleNodeData : public OctreeQueryNode {
public:
    ParticleNodeData() :
        OctreeQueryNode(),
        _lastDeletedParticlesSentAt(0) { }

    virtual PacketType getMyPacketType() const { return PacketTypeParticleData; }

    quint64 getLastDeletedParticlesSentAt() const { return _lastDeletedParticlesSentAt; }
    void setLastDeletedParticlesSentAt(quint64 sentAt) { _lastDeletedParticlesSentAt = sentAt; }

private:
    quint64 _lastDeletedParticlesSentAt;
};

#endif // hifi_ParticleNodeData_h
