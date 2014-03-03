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

VoxelDetail VoxelsScriptingInterface::getVoxelAt(float x, float y, float z, float scale) {
    // setup a VoxelDetail struct with the data
    VoxelDetail result = {0,0,0,0,0,0,0};

    if (_tree) {
        _tree->lockForRead();

        VoxelTreeElement* voxel = static_cast<VoxelTreeElement*>(_tree->getOctreeElementAt(x / (float)TREE_SCALE, y / (float)TREE_SCALE, 
                                                    z / (float)TREE_SCALE, scale / (float)TREE_SCALE));
        _tree->unlock();
        if (voxel) {
             // Note: these need to be in voxel space because the VoxelDetail -> js converter will upscale
            result.x = voxel->getCorner().x;
            result.y = voxel->getCorner().y;
            result.z = voxel->getCorner().z;
            result.s = voxel->getScale();
            result.red = voxel->getColor()[RED_INDEX];
            result.green = voxel->getColor()[GREEN_INDEX];
            result.blue = voxel->getColor()[BLUE_INDEX];
        }
    }
    return result;
}

void VoxelsScriptingInterface::setVoxelNonDestructive(float x, float y, float z, float scale, 
                                                        uchar red, uchar green, uchar blue) {
    // setup a VoxelDetail struct with the data
    VoxelDetail addVoxelDetail = {x / (float)TREE_SCALE, y / (float)TREE_SCALE, z / (float)TREE_SCALE, 
                                    scale / (float)TREE_SCALE, red, green, blue};

    // queue the add packet
    queueVoxelAdd(PacketTypeVoxelSet, addVoxelDetail);
    
    // handle the local tree also...
    if (_tree) {
        _tree->createVoxel(addVoxelDetail.x, addVoxelDetail.y, addVoxelDetail.z, addVoxelDetail.s, red, green, blue, false);
    }
}

void VoxelsScriptingInterface::setVoxel(float x, float y, float z, float scale,
                                                        uchar red, uchar green, uchar blue) {
    // setup a VoxelDetail struct with the data
    VoxelDetail addVoxelDetail = {x / (float)TREE_SCALE, y / (float)TREE_SCALE, z / (float)TREE_SCALE, 
                                    scale / (float)TREE_SCALE, red, green, blue};

    // queue the destructive add
    queueVoxelAdd(PacketTypeVoxelSetDestructive, addVoxelDetail);

    // handle the local tree also...
    if (_tree) {
        _tree->createVoxel(addVoxelDetail.x, addVoxelDetail.y, addVoxelDetail.z, addVoxelDetail.s, red, green, blue, true);
    }
}

void VoxelsScriptingInterface::eraseVoxel(float x, float y, float z, float scale) {

    // setup a VoxelDetail struct with data
    VoxelDetail deleteVoxelDetail = {x / (float)TREE_SCALE, y / (float)TREE_SCALE, z / (float)TREE_SCALE, 
                                        scale / (float)TREE_SCALE, 0, 0, 0};

    getVoxelPacketSender()->queueVoxelEditMessages(PacketTypeVoxelErase, 1, &deleteVoxelDetail);

    // handle the local tree also...
    if (_tree) {
        _tree->deleteVoxelAt(deleteVoxelDetail.x, deleteVoxelDetail.y, deleteVoxelDetail.z, deleteVoxelDetail.s);
    }
}


RayToVoxelIntersectionResult VoxelsScriptingInterface::findRayIntersection(const PickRay& ray) {
    RayToVoxelIntersectionResult result;
    if (_tree) {
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
    }
    return result;
}

glm::vec3 VoxelsScriptingInterface::getFaceVector(const QString& face) {
    if (face == "MIN_X_FACE") {
        return glm::vec3(-1, 0, 0);
    } else if (face == "MAX_X_FACE") {
        return glm::vec3(1, 0, 0);
    } else if (face == "MIN_Y_FACE") {
        return glm::vec3(0, -1, 0);
    } else if (face == "MAX_Y_FACE") {
        return glm::vec3(0, 1, 0);
    } else if (face == "MIN_Z_FACE") {
        return glm::vec3(0, 0, -1);
    } else if (face == "MAX_Z_FACE") {
        return glm::vec3(0, 0, 1);
    }
    return glm::vec3(0, 0, 0); //error case
}

VoxelDetail VoxelsScriptingInterface::getVoxelEnclosingPoint(const glm::vec3& point) {
    VoxelDetail result = { 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0 };
    if (_tree) {
        OctreeElement* element = _tree->getElementEnclosingPoint(point / (float)TREE_SCALE);
        if (element) {
            VoxelTreeElement* voxel = static_cast<VoxelTreeElement*>(element);
            result.x = voxel->getCorner().x;
            result.y = voxel->getCorner().y;
            result.z = voxel->getCorner().z;
            result.s = voxel->getScale();
            result.red = voxel->getColor()[0];
            result.green = voxel->getColor()[1];
            result.blue = voxel->getColor()[2];
        }
    }
    return result;
}


