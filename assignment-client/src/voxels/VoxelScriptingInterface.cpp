//
//  VoxelScriptingInterface.cpp
//  hifi
//
//  Created by Stephen Birarda on 9/17/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "VoxelScriptingInterface.h"

void VoxelScriptingInterface::queueVoxelAdd(PACKET_TYPE addPacketType, VoxelDetail& addVoxelDetails) {
    _voxelPacketSender.sendVoxelEditMessage(addPacketType, addVoxelDetails);
}

void VoxelScriptingInterface::queueVoxelAdd(float x, float y, float z, float scale, uchar red, uchar green, uchar blue) {
    // setup a VoxelDetail struct with the data
    VoxelDetail addVoxelDetail = {x, y, z, scale, red, green, blue};
    
    // queue the packet
    queueVoxelAdd(PACKET_TYPE_SET_VOXEL, addVoxelDetail);
}

void VoxelScriptingInterface::queueDestructiveVoxelAdd(float x, float y, float z, float scale,
                                                        uchar red, uchar green, uchar blue) {
    // setup a VoxelDetail struct with the data
    VoxelDetail addVoxelDetail = {x, y, z, scale, red, green, blue};
    
    // queue the destructive add
    queueVoxelAdd(PACKET_TYPE_SET_VOXEL_DESTRUCTIVE, addVoxelDetail);
}

void VoxelScriptingInterface::queueVoxelDelete(float x, float y, float z, float scale) {
    
    // setup a VoxelDetail struct with data
    VoxelDetail deleteVoxelDetail = {x, y, z, scale, 0, 0, 0};
    
    _voxelPacketSender.sendVoxelEditMessage(PACKET_TYPE_ERASE_VOXEL, deleteVoxelDetail);
}
