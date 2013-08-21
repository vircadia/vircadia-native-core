//
//  VoxelServerPacketProcessor.h
//  voxel-server
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded network packet processor for the voxel-server
//

#ifndef __voxel_server__VoxelServerPacketProcessor__
#define __voxel_server__VoxelServerPacketProcessor__

#include <ReceivedPacketProcessor.h>

/// Handles processing of incoming network packets for the voxel-server. As with other ReceivedPacketProcessor classes 
/// the user is responsible for reading inbound packets and adding them to the processing queue by calling queueReceivedPacket()
class VoxelServerPacketProcessor : public ReceivedPacketProcessor {
protected:
    virtual void processPacket(sockaddr& senderAddress, unsigned char*  packetData, ssize_t packetLength);
};
#endif // __voxel_server__VoxelServerPacketProcessor__
