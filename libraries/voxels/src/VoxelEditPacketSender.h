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
#include <PacketHeaders.h>
#include <SharedUtil.h> // for VoxelDetail
#include "JurisdictionMap.h"

/// Used for construction of edit voxel packets
class EditPacketBuffer {
public:
    EditPacketBuffer() { _currentSize = 0; _currentType = PACKET_TYPE_UNKNOWN; _nodeID = UNKNOWN_NODE_ID; }
    EditPacketBuffer(PACKET_TYPE type, unsigned char* codeColorBuffer, ssize_t length, uint16_t nodeID = UNKNOWN_NODE_ID);
    uint16_t _nodeID;
    PACKET_TYPE _currentType;
    unsigned char _currentBuffer[MAX_PACKET_SIZE];
    ssize_t _currentSize;
};

/// Threaded processor for queueing and sending of outbound edit voxel packets. 
class VoxelEditPacketSender : public PacketSender {
public:
    VoxelEditPacketSender(PacketSenderNotify* notify = NULL);
    ~VoxelEditPacketSender();
    
    /// Send voxel edit message immediately
    void sendVoxelEditMessage(PACKET_TYPE type, VoxelDetail& detail);

    /// Queues a voxel edit message. Will potentially sends a pending multi-command packet. Determines which voxel-server
    /// node or nodes the packet should be sent to.
    void queueVoxelEditMessage(PACKET_TYPE type, unsigned char* codeColorBuffer, ssize_t length);

    /// Queues an array of several voxel edit messages. Will potentially send a pending multi-command packet. Determines 
    /// which voxel-server node or nodes the packet should be sent to.
    void queueVoxelEditMessages(PACKET_TYPE type, int numberOfDetails, VoxelDetail* details);

    /// releases all queued messages even if those messages haven't filled an MTU packet. This will move the packed message 
    /// packets onto the send queue. If running in threaded mode, the caller does not need to do any further processing to
    /// have these packets get sent. If running in non-threaded mode, the caller must still call process() on a regular
    /// interval to ensure that the packets are actually sent.
    void releaseQueuedMessages();

    /// are we in sending mode. If we're not in sending mode then all packets and messages will be ignored and
    /// not queued and not sent
    bool getShouldSend() const { return _shouldSend; }

    /// set sending mode. By default we are set to shouldSend=TRUE and packets will be sent. If shouldSend=FALSE, then we'll
    /// switch to not sending mode, and all packets and messages will be ignored, not queued, and not sent. This might be used
    /// in an application like interface when all voxel features are disabled.
    void setShouldSend(bool shouldSend) { _shouldSend = shouldSend; }

    /// call this to inform the VoxelEditPacketSender of the voxel server jurisdictions. This is required for normal operation.
    /// As initialized the VoxelEditPacketSender will wait until it knows of the jurisdictions before queuing and sending
    /// packets to voxel servers.
    void setVoxelServerJurisdictions(NodeToJurisdictionMap* voxelServerJurisdictions) { 
            _voxelServerJurisdictions = voxelServerJurisdictions;
    }

    /// if you're running in non-threaded mode, you must call this method regularly
    virtual bool process();

private:
    bool _shouldSend;
    void queuePacketToNode(uint16_t nodeID, unsigned char* bufferOut, ssize_t sizeOut);
    void initializePacket(EditPacketBuffer& packetBuffer, PACKET_TYPE type);
    void releaseQueuedPacket(EditPacketBuffer& packetBuffer); // releases specific queued packet
    bool voxelServersExist() const;
    void processPreJurisdictionPackets();

    std::map<uint16_t,EditPacketBuffer> _pendingEditPackets;
    std::vector<EditPacketBuffer*> _preJurisidictionPackets;

    NodeToJurisdictionMap* _voxelServerJurisdictions;
    bool _releaseQueuedMessagesPending;
};
#endif // __shared__VoxelEditPacketSender__
