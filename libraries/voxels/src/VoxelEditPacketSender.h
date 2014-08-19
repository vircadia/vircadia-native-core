//
//  VoxelEditPacketSender.h
//  libraries/voxels/src
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Voxel Packet Sender
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_VoxelEditPacketSender_h
#define hifi_VoxelEditPacketSender_h

#include <OctreeEditPacketSender.h>

#include "VoxelDetail.h"

/// Utility for processing, packing, queueing and sending of outbound edit voxel messages.
class VoxelEditPacketSender :  public OctreeEditPacketSender {
    Q_OBJECT
public:
    /// Send voxel edit message immediately
    void sendVoxelEditMessage(PacketType type, const VoxelDetail& detail);

    /// Queues a single voxel edit message. Will potentially send a pending multi-command packet. Determines which voxel-server
    /// node or nodes the packet should be sent to. Can be called even before voxel servers are known, in which case up to 
    /// MaxPendingMessages will be buffered and processed when voxel servers are known.
    void queueVoxelEditMessage(PacketType type, unsigned char* codeColorBuffer, size_t length) {
        queueOctreeEditMessage(type, codeColorBuffer, length);
    }

    /// Queues an array of several voxel edit messages. Will potentially send a pending multi-command packet. Determines 
    /// which voxel-server node or nodes the packet should be sent to. Can be called even before voxel servers are known, in 
    /// which case up to MaxPendingMessages will be buffered and processed when voxel servers are known.
    void queueVoxelEditMessages(PacketType type, int numberOfDetails, VoxelDetail* details);

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
    virtual char getMyNodeType() const { return NodeType::VoxelServer; }
    
    void setSatoshisPerVoxel(qint64 satoshisPerVoxel) { _satoshisPerVoxel = satoshisPerVoxel; }
    void setSatoshisPerMeterCubed(qint64 satoshisPerMeterCubed) { _satoshisPerMeterCubed = satoshisPerMeterCubed; }
    
    qint64 satoshiCostForMessage(const VoxelDetail& details);
    
private:
    qint64 _satoshisPerVoxel;
    qint64 _satoshisPerMeterCubed;
};
#endif // hifi_VoxelEditPacketSender_h
