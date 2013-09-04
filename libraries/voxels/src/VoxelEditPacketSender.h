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

#include <PacketSender.h>
#include <SharedUtil.h> // for VoxelDetail
#include "JurisdictionMap.h"

/// Used for construction of edit voxel packets
class EditPacketBuffer {
public:
    EditPacketBuffer() { _currentSize = 0; _currentType = PACKET_TYPE_UNKNOWN; _nodeID = UNKNOWN_NODE_ID; }
    uint16_t _nodeID;
    PACKET_TYPE _currentType;
    unsigned char _currentBuffer[MAX_PACKET_SIZE];
    ssize_t _currentSize;
};

/// Threaded processor for queueing and sending of outbound edit voxel packets. 
class VoxelEditPacketSender : public PacketSender {
public:
    VoxelEditPacketSender(PacketSenderNotify* notify = NULL);
    
    /// Send voxel edit message immediately
    void sendVoxelEditMessage(PACKET_TYPE type, VoxelDetail& detail);

    /// Queues a voxel edit message. Will potentially sends a pending multi-command packet. Determines which voxel-server
    /// node or nodes the packet should be sent to.
    void queueVoxelEditMessage(PACKET_TYPE type, unsigned char* codeColorBuffer, ssize_t length);

    /// Queues an array of several voxel edit messages. Will potentially send a pending multi-command packet. Determines 
    /// which voxel-server node or nodes the packet should be sent to.
    void queueVoxelEditMessages(PACKET_TYPE type, int numberOfDetails, VoxelDetail* details);

    /// flushes all queued packets for all nodes
    void flushQueue();

    bool getShouldSend() const { return _shouldSend; }
    void setShouldSend(bool shouldSend) { _shouldSend = shouldSend; }

    void setVoxelServerJurisdictions(NodeToJurisdictionMap* voxelServerJurisdictions) { 
            _voxelServerJurisdictions = voxelServerJurisdictions;
    }

private:
    bool _shouldSend;
    void actuallySendMessage(uint16_t nodeID, unsigned char* bufferOut, ssize_t sizeOut);
    void initializePacket(EditPacketBuffer& packetBuffer, PACKET_TYPE type);
    void flushQueue(EditPacketBuffer& packetBuffer); // flushes specific queued packet

    std::map<uint16_t,EditPacketBuffer> _pendingEditPackets;

    NodeToJurisdictionMap* _voxelServerJurisdictions;
};
#endif // __shared__VoxelEditPacketSender__
