//
//  VoxelEditPacketSender.h
//  shared
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Voxel Packet Sender
//

#ifndef __shared__VoxelEditPacketSender__
#define __shared__VoxelEditPacketSender__

#include <OctreeEditPacketSender.h>

/// Utility for processing, packing, queueing and sending of outbound edit voxel messages.
class VoxelEditPacketSender :  public virtual OctreeEditPacketSender {
public:
    VoxelEditPacketSender(PacketSenderNotify* notify = NULL) : OctreeEditPacketSender(notify) { }
    ~VoxelEditPacketSender() { }
    
    /// Send voxel edit message immediately
    void sendVoxelEditMessage(PACKET_TYPE type, VoxelDetail& detail);

    /// Queues a single voxel edit message. Will potentially send a pending multi-command packet. Determines which voxel-server
    /// node or nodes the packet should be sent to. Can be called even before voxel servers are known, in which case up to 
    /// MaxPendingMessages will be buffered and processed when voxel servers are known.
    void queueVoxelEditMessage(PACKET_TYPE type, unsigned char* codeColorBuffer, ssize_t length) {
        queueOctreeEditMessage(type, codeColorBuffer, length);
    }

    /// Queues an array of several voxel edit messages. Will potentially send a pending multi-command packet. Determines 
    /// which voxel-server node or nodes the packet should be sent to. Can be called even before voxel servers are known, in 
    /// which case up to MaxPendingMessages will be buffered and processed when voxel servers are known.
    void queueVoxelEditMessages(PACKET_TYPE type, int numberOfDetails, VoxelDetail* details);

    /// call this to inform the VoxelEditPacketSender of the voxel server jurisdictions. This is required for normal operation.
    /// The internal contents of the jurisdiction map may change throughout the lifetime of the VoxelEditPacketSender. This map
    /// can be set prior to voxel servers being present, so long as the contents of the map accurately reflect the current
    /// known jurisdictions.
    void setVoxelServerJurisdictions(NodeToJurisdictionMap* voxelServerJurisdictions) { 
        setServerJurisdictions(voxelServerJurisdictions);
    }

    // is there a voxel server available to send packets to    
    bool voxelServersExist() const { return serversExist(); }

    // My server type is the voxel server
    virtual unsigned char getMyNodeType() const { return NODE_TYPE_VOXEL_SERVER; }
};
#endif // __shared__VoxelEditPacketSender__
