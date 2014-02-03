//
//  VoxelsScriptingInterface.cpp
//  hifi
//
//  Created by Stephen Birarda on 9/17/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "VoxelsScriptingInterface.h"

void VoxelsScriptingInterface::queueVoxelAdd(PacketType addPacketType, VoxelDetail& addVoxelDetails) {
    getVoxelPacketSender()->queueVoxelEditMessages(addPacketType, 1, &addVoxelDetails);
}

void VoxelsScriptingInterface::setVoxelNonDestructive(float x, float y, float z, float scale, 
                                                        uchar red, uchar green, uchar blue) {
    // setup a VoxelDetail struct with the data
    VoxelDetail addVoxelDetail = {x / (float)TREE_SCALE, y / (float)TREE_SCALE, z / (float)TREE_SCALE, 
                                    scale / (float)TREE_SCALE, red, green, blue};

    // queue the packet
    queueVoxelAdd(PacketTypeVoxelSet, addVoxelDetail);
}

void VoxelsScriptingInterface::setVoxel(float x, float y, float z, float scale,
                                                        uchar red, uchar green, uchar blue) {
    // setup a VoxelDetail struct with the data
    VoxelDetail addVoxelDetail = {x / (float)TREE_SCALE, y / (float)TREE_SCALE, z / (float)TREE_SCALE, 
                                    scale / (float)TREE_SCALE, red, green, blue};

    // queue the destructive add
    queueVoxelAdd(PacketTypeVoxelSetDestructive, addVoxelDetail);
}

void VoxelsScriptingInterface::eraseVoxel(float x, float y, float z, float scale) {

    // setup a VoxelDetail struct with data
    VoxelDetail deleteVoxelDetail = {x / (float)TREE_SCALE, y / (float)TREE_SCALE, z / (float)TREE_SCALE, 
                                        scale / (float)TREE_SCALE, 0, 0, 0};

    getVoxelPacketSender()->queueVoxelEditMessages(PacketTypeVoxelErase, 1, &deleteVoxelDetail);
}

