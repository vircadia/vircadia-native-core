//
//  VoxelScriptingInterface.cpp
//  hifi
//
//  Created by Stephen Birarda on 9/17/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "VoxelScriptingInterface.h"

VoxelScriptingInterface::VoxelScriptingInterface() {
    _jurisdictionListener.initialize(true);
    _voxelPacketSender.setVoxelServerJurisdictions(_jurisdictionListener.getJurisdictions());
}

void VoxelScriptingInterface::queueVoxelAdd(PACKET_TYPE addPacketType, VoxelDetail& addVoxelDetails) {
    _voxelPacketSender.queueVoxelEditMessages(addPacketType, 1, &addVoxelDetails);
}

void VoxelScriptingInterface::queueVoxelAdd(float x, float y, float z, float scale, uchar red, uchar green, uchar blue) {
    // setup a VoxelDetail struct with the data
    VoxelDetail addVoxelDetail = {x, y, z, scale, red, green, blue};
    
    // queue the packet
    queueVoxelAdd(PACKET_TYPE_VOXEL_SET, addVoxelDetail);
}

void VoxelScriptingInterface::queueDestructiveVoxelAdd(float x, float y, float z, float scale,
                                                        uchar red, uchar green, uchar blue) {
    // setup a VoxelDetail struct with the data
    VoxelDetail addVoxelDetail = {x, y, z, scale, red, green, blue};
    
    // queue the destructive add
    queueVoxelAdd(PACKET_TYPE_VOXEL_SET_DESTRUCTIVE, addVoxelDetail);
}

void VoxelScriptingInterface::queueVoxelDelete(float x, float y, float z, float scale) {
    
    // setup a VoxelDetail struct with data
    VoxelDetail deleteVoxelDetail = {x, y, z, scale, 0, 0, 0};
    
    _voxelPacketSender.queueVoxelEditMessages(PACKET_TYPE_VOXEL_ERASE, 1, &deleteVoxelDetail);
}

