//
//  VoxelEditPacketSender.h
//  interface
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

class Application;

class EditPacketBuffer {
public:
    EditPacketBuffer() { _currentSize = 0; _currentType = PACKET_TYPE_UNKNOWN; _nodeID = UNKNOWN_NODE_ID; }
    uint16_t        _nodeID;
    PACKET_TYPE     _currentType;
    unsigned char   _currentBuffer[MAX_PACKET_SIZE];
    ssize_t         _currentSize;
};

class VoxelEditPacketSender : public PacketSender {
public:
    VoxelEditPacketSender(Application* app);
    
    // Some ways you can send voxel edit messages...
    void sendVoxelEditMessage(PACKET_TYPE type, VoxelDetail& detail); // sends it right away
    void queueVoxelEditMessage(PACKET_TYPE type, unsigned char* codeColorBuffer, ssize_t length); // queues it into a multi-command packet
    void flushQueue(); // flushes all queued packets

private:
    void actuallySendMessage(uint16_t nodeID, unsigned char* bufferOut, ssize_t sizeOut);
    void initializePacket(EditPacketBuffer& packetBuffer, PACKET_TYPE type);
    void flushQueue(EditPacketBuffer& packetBuffer); // flushes specific queued packet

    Application*    _app;
    std::map<uint16_t,EditPacketBuffer> _pendingEditPackets;
};
#endif // __shared__VoxelEditPacketSender__
