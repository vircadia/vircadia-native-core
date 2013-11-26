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
    EditPacketBuffer() : _nodeUUID(), _currentType(PACKET_TYPE_UNKNOWN), _currentSize(0)  { }
    EditPacketBuffer(PACKET_TYPE type, unsigned char* codeColorBuffer, ssize_t length, const QUuid nodeUUID = QUuid());
    QUuid _nodeUUID;
    PACKET_TYPE _currentType;
    unsigned char _currentBuffer[MAX_PACKET_SIZE];
    ssize_t _currentSize;
};

/// Utility for processing, packing, queueing and sending of outbound edit voxel messages.
class VoxelEditPacketSender :  public virtual PacketSender {
public:
    VoxelEditPacketSender(PacketSenderNotify* notify = NULL);
    ~VoxelEditPacketSender();
    
    /// Send voxel edit message immediately
    void sendVoxelEditMessage(PACKET_TYPE type, VoxelDetail& detail);

    /// Queues a single voxel edit message. Will potentially send a pending multi-command packet. Determines which voxel-server
    /// node or nodes the packet should be sent to. Can be called even before voxel servers are known, in which case up to 
    /// MaxPendingMessages will be buffered and processed when voxel servers are known.
    void queueVoxelEditMessage(PACKET_TYPE type, unsigned char* codeColorBuffer, ssize_t length);

    /// Queues an array of several voxel edit messages. Will potentially send a pending multi-command packet. Determines 
    /// which voxel-server node or nodes the packet should be sent to. Can be called even before voxel servers are known, in 
    /// which case up to MaxPendingMessages will be buffered and processed when voxel servers are known.
    void queueVoxelEditMessages(PACKET_TYPE type, int numberOfDetails, VoxelDetail* details);

    /// Releases all queued messages even if those messages haven't filled an MTU packet. This will move the packed message 
    /// packets onto the send queue. If running in threaded mode, the caller does not need to do any further processing to
    /// have these packets get sent. If running in non-threaded mode, the caller must still call process() on a regular
    /// interval to ensure that the packets are actually sent. Can be called even before voxel servers are known, in 
    /// which case  up to MaxPendingMessages of the released messages will be buffered and actually released when 
    /// voxel servers are known.
    void releaseQueuedMessages();

    /// are we in sending mode. If we're not in sending mode then all packets and messages will be ignored and
    /// not queued and not sent
    bool getShouldSend() const { return _shouldSend; }

    /// set sending mode. By default we are set to shouldSend=TRUE and packets will be sent. If shouldSend=FALSE, then we'll
    /// switch to not sending mode, and all packets and messages will be ignored, not queued, and not sent. This might be used
    /// in an application like interface when all voxel features are disabled.
    void setShouldSend(bool shouldSend) { _shouldSend = shouldSend; }

    /// call this to inform the VoxelEditPacketSender of the voxel server jurisdictions. This is required for normal operation.
    /// The internal contents of the jurisdiction map may change throughout the lifetime of the VoxelEditPacketSender. This map
    /// can be set prior to voxel servers being present, so long as the contents of the map accurately reflect the current
    /// known jurisdictions.
    void setVoxelServerJurisdictions(NodeToJurisdictionMap* voxelServerJurisdictions) { 
        _voxelServerJurisdictions = voxelServerJurisdictions;
    }

    /// if you're running in non-threaded mode, you must call this method regularly
    virtual bool process();

    /// Set the desired number of pending messages that the VoxelEditPacketSender should attempt to queue even if voxel
    /// servers are not present. This only applies to how the VoxelEditPacketSender will manage messages when no voxel
    /// servers are present. By default, this value is the same as the default packets that will be sent in one second.
    /// Which means the VoxelEditPacketSender will not buffer all messages given to it if no voxel servers are present. 
    /// This is the maximum number of queued messages and single messages.
    void setMaxPendingMessages(int maxPendingMessages) { _maxPendingMessages = maxPendingMessages; }

    // the default number of pending messages we will store if no voxel servers are available
    static const int DEFAULT_MAX_PENDING_MESSAGES;

    // is there a voxel server available to send packets to    
    bool voxelServersExist() const;

    /// Set the desired max packet size in bytes that the VoxelEditPacketSender should create
    void setMaxPacketSize(int maxPacketSize) { _maxPacketSize = maxPacketSize; }

    /// returns the current desired max packet size in bytes that the VoxelEditPacketSender will create
    int getMaxPacketSize() const { return _maxPacketSize; }

private:
    bool _shouldSend;
    void queuePacketToNode(const QUuid& nodeID, unsigned char* buffer, ssize_t length);
    void queuePacketToNodes(unsigned char* buffer, ssize_t length);
    void initializePacket(EditPacketBuffer& packetBuffer, PACKET_TYPE type);
    void releaseQueuedPacket(EditPacketBuffer& packetBuffer); // releases specific queued packet
    
    void processPreServerExistsPackets();

    // These are packets which are destined from know servers but haven't been released because they're still too small
    std::map<QUuid, EditPacketBuffer> _pendingEditPackets;
    
    // These are packets that are waiting to be processed because we don't yet know if there are voxel servers
    int _maxPendingMessages;
    bool _releaseQueuedMessagesPending;
    std::vector<EditPacketBuffer*> _preServerPackets; // these will get packed into other larger packets
    std::vector<EditPacketBuffer*> _preServerSingleMessagePackets; // these will go out as is

    NodeToJurisdictionMap* _voxelServerJurisdictions;
    
    unsigned short int _sequenceNumber;
    int _maxPacketSize;
};
#endif // __shared__VoxelEditPacketSender__
