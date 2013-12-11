//
//  ParticleEditPacketSender.h
//  shared
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Voxel Packet Sender
//

#ifndef __shared__ParticleEditPacketSender__
#define __shared__ParticleEditPacketSender__

#include <OctreeEditPacketSender.h>
#include "Particle.h"

/// Utility for processing, packing, queueing and sending of outbound edit voxel messages.
class ParticleEditPacketSender :  public virtual OctreeEditPacketSender {
public:
    ParticleEditPacketSender(PacketSenderNotify* notify = NULL) : OctreeEditPacketSender(notify) { }
    ~ParticleEditPacketSender() { }
    
    /// Send particle add message immediately
    void sendEditParticleMessage(PACKET_TYPE type, const ParticleDetail& detail);

    /// Queues an array of several voxel edit messages. Will potentially send a pending multi-command packet. Determines 
    /// which voxel-server node or nodes the packet should be sent to. Can be called even before voxel servers are known, in 
    /// which case up to MaxPendingMessages will be buffered and processed when voxel servers are known.
    void queueParticleEditMessages(PACKET_TYPE type, int numberOfDetails, ParticleDetail* details);

    // My server type is the voxel server
    virtual unsigned char getMyNodeType() const { return NODE_TYPE_PARTICLE_SERVER; }
};
#endif // __shared__ParticleEditPacketSender__
