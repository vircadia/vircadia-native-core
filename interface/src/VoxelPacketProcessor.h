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

class Application;

class VoxelPacketProcessor : public ReceivedPacketProcessor {
public:
    VoxelPacketProcessor(Application* app);
    virtual void processPacket(sockaddr& senderAddress, unsigned char*  packetData, ssize_t packetLength);

private:
    Application* _app;
};
#endif // __shared__VoxelPacketProcessor__
