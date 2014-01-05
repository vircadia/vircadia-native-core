//
//  VoxelsScriptingInterface.cpp
//  hifi
//
//  Created by Stephen Birarda on 9/17/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "VoxelsScriptingInterface.h"

void VoxelsScriptingInterface::queueVoxelAdd(PACKET_TYPE addPacketType, VoxelDetail& addVoxelDetails) {
    getVoxelPacketSender()->queueVoxelEditMessages(addPacketType, 1, &addVoxelDetails);
}

void VoxelsScriptingInterface::setVoxel(float x, float y, float z, float scale, uchar red, uchar green, uchar blue) {
    // setup a VoxelDetail struct with the data
    VoxelDetail addVoxelDetail = {x, y, z, scale, red, green, blue};

    // queue the packet
    queueVoxelAdd(PACKET_TYPE_VOXEL_SET, addVoxelDetail);
}

void VoxelsScriptingInterface::destructiveSetVoxel(float x, float y, float z, float scale,
                                                        uchar red, uchar green, uchar blue) {
    // setup a VoxelDetail struct with the data
    VoxelDetail addVoxelDetail = {x, y, z, scale, red, green, blue};

    // queue the destructive add
    queueVoxelAdd(PACKET_TYPE_VOXEL_SET_DESTRUCTIVE, addVoxelDetail);
}

void VoxelsScriptingInterface::eraseVoxel(float x, float y, float z, float scale) {

    // setup a VoxelDetail struct with data
    VoxelDetail deleteVoxelDetail = {x, y, z, scale, 0, 0, 0};

    getVoxelPacketSender()->queueVoxelEditMessages(PACKET_TYPE_VOXEL_ERASE, 1, &deleteVoxelDetail);
}

