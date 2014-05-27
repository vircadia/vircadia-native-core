//
//  LocalVoxels.cpp
//  libraries/script-engine/src
//
//  Created by Clément Brisset on 2/24/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
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

VoxelDetail LocalVoxels::getVoxelAt(float x, float y, float z, float scale) {
    // setup a VoxelDetail struct with the data
    VoxelDetail result = {0,0,0,0,0,0,0};
    if (_tree) {
        _tree->lockForRead();
        
        VoxelTreeElement* voxel = static_cast<VoxelTreeElement*>(_tree->getOctreeElementAt(x / (float)TREE_SCALE,
                                                                                           y / (float)TREE_SCALE,
                                                                                           z / (float)TREE_SCALE,
                                                                                           scale / (float)TREE_SCALE));
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

void LocalVoxels::setVoxelNonDestructive(float x, float y, float z, float scale,
                                         uchar red, uchar green, uchar blue) {
    if (_name == DOMAIN_TREE_NAME) {
        qDebug() << "LocalVoxels::setVoxelNonDestructive(): Please use the \"Voxels\" interface to modify the domain tree.";
        return;
    }
    if (_tree ) {
        _tree->createVoxel(x / (float)TREE_SCALE,
                           y / (float)TREE_SCALE,
                           z / (float)TREE_SCALE,
                           scale / (float)TREE_SCALE,
                           red, green, blue, false);
    }
}

void LocalVoxels::setVoxel(float x, float y, float z, float scale,
                           uchar red, uchar green, uchar blue) {
    if (_name == DOMAIN_TREE_NAME) {
        qDebug() << "LocalVoxels::setVoxel(): Please use the \"Voxels\" interface to modify the domain tree.";
        return;
    }
    if (_tree ) {
        _tree->createVoxel(x / (float)TREE_SCALE,
                           y / (float)TREE_SCALE,
                           z / (float)TREE_SCALE,
                           scale / (float)TREE_SCALE,
                           red, green, blue, true);
    }
}

void LocalVoxels::eraseVoxel(float x, float y, float z, float scale) {
    if (_name == DOMAIN_TREE_NAME) {
        qDebug() << "LocalVoxels::eraseVoxel(): Please use the \"Voxels\" interface to modify the domain tree.";
        return;
    }
    if (_tree ) {
        _tree->deleteVoxelAt(x / (float)TREE_SCALE,
                                 y / (float)TREE_SCALE,
                                 z / (float)TREE_SCALE,
                                 scale / (float)TREE_SCALE);
    }
}

void LocalVoxels::copyTo(float x, float y, float z, float scale, const QString destination) {
    if (destination == DOMAIN_TREE_NAME) {
        qDebug() << "LocalVoxels::copyTo(): Please use the \"Voxels\" interface to modify the domain tree.";
        return;
    }
    StrongVoxelTreePointer destinationTree = LocalVoxelsList::getInstance()->getTree(destination);
    VoxelTreeElement* destinationNode = destinationTree->getVoxelAt(x / (float)TREE_SCALE,
                                                                    y / (float)TREE_SCALE,
                                                                    z / (float)TREE_SCALE,
                                                                    scale / (float)TREE_SCALE);
    destinationTree->copyFromTreeIntoSubTree(_tree.data(), destinationNode);
}

void LocalVoxels::pasteFrom(float x, float y, float z, float scale, const QString source) {
    if (_name == DOMAIN_TREE_NAME) {
        qDebug() << "LocalVoxels::pasteFrom(): Please use the \"Voxels\" interface to modify the domain tree.";
        return;
    }
    StrongVoxelTreePointer sourceTree = LocalVoxelsList::getInstance()->getTree(source);
    VoxelTreeElement* sourceNode = _tree->getVoxelAt(x / (float)TREE_SCALE,
                                                     y / (float)TREE_SCALE,
                                                     z / (float)TREE_SCALE,
                                                     scale / (float)TREE_SCALE);
    _tree->copySubTreeIntoNewTree(sourceNode, sourceTree.data(), true);
}

RayToVoxelIntersectionResult LocalVoxels::findRayIntersection(const PickRay& ray) {
    return findRayIntersectionWorker(ray, Octree::TryLock);
}

RayToVoxelIntersectionResult LocalVoxels::findRayIntersectionBlocking(const PickRay& ray) {
    return findRayIntersectionWorker(ray, Octree::Lock);
}

RayToVoxelIntersectionResult LocalVoxels::findRayIntersectionWorker(const PickRay& ray, Octree::lockType lockType) {
    RayToVoxelIntersectionResult result;
    if (_tree) {
        OctreeElement* element;
        result.intersects = _tree->findRayIntersection(ray.origin, ray.direction, element, result.distance, result.face, NULL,
                                                                lockType, &result.accurate);
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
