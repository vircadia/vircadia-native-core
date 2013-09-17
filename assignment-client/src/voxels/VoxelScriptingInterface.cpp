//
//  VoxelScriptingInterface.cpp
//  hifi
//
//  Created by Stephen Birarda on 9/17/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "VoxelScriptingInterface.h"

void VoxelScriptingInterface::queueVoxelAdd(float x, float y, float z, float scale, uchar red, uchar green, uchar blue) {
    // setup a VoxelDetail struct with the data
    VoxelDetail addVoxelDetail = {x, y, z, scale, red, green, blue};
    _voxelPacketSender.sendVoxelEditMessage(PACKET_TYPE_SET_VOXEL, addVoxelDetail);
}
