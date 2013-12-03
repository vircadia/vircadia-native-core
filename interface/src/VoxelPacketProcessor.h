//
//  VoxelPacketProcessor.h
//  interface
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Voxel Packet Receiver
//

#ifndef __shared__VoxelPacketProcessor__
#define __shared__VoxelPacketProcessor__

#include <ReceivedPacketProcessor.h>

/// Handles processing of incoming voxel packets for the interface application. As with other ReceivedPacketProcessor classes 
/// the user is responsible for reading inbound packets and adding them to the processing queue by calling queueReceivedPacket()
class VoxelPacketProcessor : public ReceivedPacketProcessor {
protected:
    virtual void processPacket(const HifiSockAddr& senderSockAddr, unsigned char*  packetData, ssize_t packetLength);
};
#endif // __shared__VoxelPacketProcessor__
