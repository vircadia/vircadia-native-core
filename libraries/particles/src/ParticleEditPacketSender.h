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
class ParticleEditPacketSender :  public OctreeEditPacketSender {
    Q_OBJECT
public:
    /// Send particle add message immediately
    /// NOTE: ParticleProperties assumes that all distances are in meter units
    void sendEditParticleMessage(PacketType type, ParticleID particleID, const ParticleProperties& properties);

    /// Queues an array of several voxel edit messages. Will potentially send a pending multi-command packet. Determines
    /// which voxel-server node or nodes the packet should be sent to. Can be called even before voxel servers are known, in
    /// which case up to MaxPendingMessages will be buffered and processed when voxel servers are known.
    /// NOTE: ParticleProperties assumes that all distances are in meter units
    void queueParticleEditMessage(PacketType type, ParticleID particleID, const ParticleProperties& properties);

    // My server type is the particle server
    virtual unsigned char getMyNodeType() const { return NodeType::ParticleServer; }
    virtual void adjustEditPacketForClockSkew(unsigned char* codeColorBuffer, ssize_t length, int clockSkew);
};
#endif // __shared__ParticleEditPacketSender__
