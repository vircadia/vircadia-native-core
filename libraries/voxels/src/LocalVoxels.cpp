//
//  LocalVoxels.cpp
//  hifi
//
//  Created by Clément Brisset on 2/24/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include "LocalVoxels.h"

LocalVoxels::LocalVoxels(QString name) :
    QObject(NULL),
    _name(name),
    _tree(new VoxelTree(true))
{
    LocalVoxelsList::getInstance()->insert(_name, _tree);
}

LocalVoxels::~LocalVoxels() {
    _tree.clear();
    LocalVoxelsList::getInstance()->remove(_name);
}

void LocalVoxels::setVoxelNonDestructive(float x, float y, float z, float scale,
                                         uchar red, uchar green, uchar blue) {if (_tree ) {
    if (_tree->tryLockForWrite()) {
        _tree->createVoxel(x, y, z, scale, red, green, blue, false);
        _tree->unlock();
    }
}
}

void LocalVoxels::setVoxel(float x, float y, float z, float scale,
                           uchar red, uchar green, uchar blue) {
    if (_tree ) {
        if (_tree->tryLockForWrite()) {
            _tree->createVoxel(x, y, z, scale, red, green, blue, true);
            _tree->unlock();
        }
    }
}

void LocalVoxels::eraseVoxel(float x, float y, float z, float scale) {
    if (_tree ) {
        if (_tree->tryLockForWrite()) {
            _tree->deleteVoxelAt(x, y, z, scale);
            _tree->unlock();
        }
    }
}

RayToVoxelIntersectionResult LocalVoxels::findRayIntersection(const PickRay& ray) {
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

glm::vec3 LocalVoxels::getFaceVector(const QString& face) {
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
