//
//  VoxelsScriptingInterface.cpp
//  hifi
//
//  Created by Stephen Birarda on 9/17/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "VoxelsScriptingInterface.h"

VoxelsScriptingInterface::VoxelsScriptingInterface() {
    NodeList::getInstance()->addNodeTypeToInterestSet(NODE_TYPE_VOXEL_SERVER);
}

void VoxelsScriptingInterface::queueVoxelAdd(PACKET_TYPE addPacketType, VoxelDetail& addVoxelDetails) {
    getVoxelPacketSender()->queueVoxelEditMessages(addPacketType, 1, &addVoxelDetails);
}

void VoxelsScriptingInterface::setVoxelNonDestructive(float x, float y, float z, float scale, 
                                                        uchar red, uchar green, uchar blue) {
    // setup a VoxelDetail struct with the data
    VoxelDetail addVoxelDetail = {x / (float)TREE_SCALE, y / (float)TREE_SCALE, z / (float)TREE_SCALE, 
                                    scale / (float)TREE_SCALE, red, green, blue};

    // queue the packet
    queueVoxelAdd(PACKET_TYPE_VOXEL_SET, addVoxelDetail);
}

void VoxelsScriptingInterface::setVoxel(float x, float y, float z, float scale,
                                                        uchar red, uchar green, uchar blue) {
    // setup a VoxelDetail struct with the data
    VoxelDetail addVoxelDetail = {x / (float)TREE_SCALE, y / (float)TREE_SCALE, z / (float)TREE_SCALE, 
                                    scale / (float)TREE_SCALE, red, green, blue};

    // queue the destructive add
    queueVoxelAdd(PACKET_TYPE_VOXEL_SET_DESTRUCTIVE, addVoxelDetail);
}

void VoxelsScriptingInterface::eraseVoxel(float x, float y, float z, float scale) {

    // setup a VoxelDetail struct with data
    VoxelDetail deleteVoxelDetail = {x / (float)TREE_SCALE, y / (float)TREE_SCALE, z / (float)TREE_SCALE, 
                                        scale / (float)TREE_SCALE, 0, 0, 0};

    getVoxelPacketSender()->queueVoxelEditMessages(PACKET_TYPE_VOXEL_ERASE, 1, &deleteVoxelDetail);
}

