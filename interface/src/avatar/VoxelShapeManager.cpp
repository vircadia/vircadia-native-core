//
//  VoxelShapeManager.cpp
//  interface/src/avatar
//
//  Created by Andrew Meadows on 2014.09.02
//  Copyright 2012 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/norm.hpp>

#include <AACubeShape.h>
#include <PhysicsSimulation.h>
#include <SharedUtil.h>

#include "VoxelShapeManager.h"

VoxelShapeManager::VoxelShapeManager() : PhysicsEntity(), _lastSimulationTranslation(0.0f) {
}

VoxelShapeManager::~VoxelShapeManager() {
    clearShapes();
}

void VoxelShapeManager::stepForward(float deltaTime) {
    PhysicsSimulation* simulation = getSimulation();
    if (simulation) {
        glm::vec3 simulationOrigin = simulation->getTranslation();
        if (glm::distance2(_lastSimulationTranslation, simulationOrigin) > EPSILON) {
            VoxelPool::const_iterator voxelItr = _voxels.constBegin();
            while (voxelItr != _voxels.constEnd()) {
                // the shape's position is stored in the simulation-frame
                const VoxelInfo& voxel = voxelItr.value();
                voxel._shape->setTranslation(voxel._cube.calcCenter() - simulationOrigin);
                ++voxelItr;
            }
            _lastSimulationTranslation = simulationOrigin;
        }
    }
}

void VoxelShapeManager::buildShapes() {
    // the shapes are owned by the elements of _voxels, 
    // so _shapes is constructed by harvesting them from _voxels
    _shapes.clear();
    VoxelPool::const_iterator voxelItr = _voxels.constBegin();
    while (voxelItr != _voxels.constEnd()) {
        _shapes.push_back(voxelItr.value()._shape);
        ++voxelItr;
    }
}

void VoxelShapeManager::clearShapes() {
    PhysicsEntity::clearShapes();
    _voxels.clear();
}

void VoxelShapeManager::updateVoxels(CubeList& cubes) {
    PhysicsSimulation* simulation = getSimulation();
    if (!simulation) {
        return;
    }

    int numChanges = 0;
    VoxelPool::iterator voxelItr = _voxels.begin();
    while (voxelItr != _voxels.end()) {
        // look for this voxel in cubes
        CubeList::iterator cubeItr = cubes.find(voxelItr.key());
        if (cubeItr == cubes.end()) {
            // did not find it --> remove the voxel
            simulation->removeShape(voxelItr.value()._shape);
            voxelItr = _voxels.erase(voxelItr);
            ++numChanges;
        } else {
            // found it --> remove the cube
            cubes.erase(cubeItr);
            voxelItr++;
        }
    }

    // add remaining cubes to _voxels
    glm::vec3 simulationOrigin = simulation->getTranslation();
    CubeList::const_iterator cubeItr = cubes.constBegin();
    while (cubeItr != cubes.constEnd()) {
        AACube cube = cubeItr.value();
        AACubeShape* shape = new AACubeShape(cube.getScale(), cube.calcCenter() - simulationOrigin);
        shape->setEntity(this);
        VoxelInfo voxel = {cube, shape };
        _voxels.insert(cubeItr.key(), voxel);
        ++numChanges;
        ++cubeItr;
    }

    if (numChanges > 0) {
        buildShapes();
    }
}
