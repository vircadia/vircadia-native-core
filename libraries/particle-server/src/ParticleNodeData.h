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

#include <OctreeQueryNode.h>
#include <PacketHeaders.h>

class ParticleNodeData : public OctreeQueryNode {
public:
    ParticleNodeData() :
        OctreeQueryNode(),
        _lastDeletedParticlesSentAt(0) {  };

    virtual PACKET_TYPE getMyPacketType() const { return PACKET_TYPE_PARTICLE_DATA; }

    uint64_t getLastDeletedParticlesSentAt() const { return _lastDeletedParticlesSentAt; }
    void setLastDeletedParticlesSentAt(uint64_t sentAt) { _lastDeletedParticlesSentAt = sentAt; }

private:
    uint64_t _lastDeletedParticlesSentAt;
};

#endif /* defined(__hifi__ParticleNodeData__) */
