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


RayToVoxelIntersectionResult VoxelsScriptingInterface::findRayIntersection(const PickRay& ray) {
    RayToVoxelIntersectionResult result;
    if (_tree) {
        if (_tree->tryLockForRead()) {
            OctreeElement* element;
            result.intersects = _tree->findRayIntersection(ray.origin, ray.direction, element, result.distance, result.face);
            if (result.intersects) {
                VoxelTreeElement* voxel = (VoxelTreeElement*)element;
                result.voxel.x = voxel->getCorner().x;
                result.voxel.y = voxel->getCorner().y;
                result.voxel.z = voxel->getCorner().z;
                result.voxel.s = voxel->getScale();
                result.voxel.red = voxel->getColor()[0];
                result.voxel.green = voxel->getColor()[1];
                result.voxel.blue = voxel->getColor()[2];
                result.intersection = ray.origin + (ray.direction * result.distance);
            }
            _tree->unlock();
        }
    }
    return result;
}
