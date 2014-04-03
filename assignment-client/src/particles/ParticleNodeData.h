//
//  ParticleNodeData.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/4/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __hifi__ParticleNodeData__
#define __hifi__ParticleNodeData__

#include <PacketHeaders.h>

#include "../octree/OctreeQueryNode.h"

class ParticleNodeData : public OctreeQueryNode {
public:
    ParticleNodeData() :
        OctreeQueryNode(),
        _lastDeletedParticlesSentAt(0) {  };

    virtual PacketType getMyPacketType() const { return PacketTypeParticleData; }

    quint64 getLastDeletedParticlesSentAt() const { return _lastDeletedParticlesSentAt; }
    void setLastDeletedParticlesSentAt(quint64 sentAt) { _lastDeletedParticlesSentAt = sentAt; }

private:
    quint64 _lastDeletedParticlesSentAt;
};

#endif /* defined(__hifi__ParticleNodeData__) */
