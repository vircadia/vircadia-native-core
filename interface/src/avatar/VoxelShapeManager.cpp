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
            int numVoxels = _voxels.size();
            for (int i = 0; i < numVoxels; ++i) {
                // the shape's position is stored in the simulation-frame
                glm::vec3 cubeCenter = _voxels[i]._cube.calcCenter();
                _voxels[i]._shape->setTranslation(cubeCenter - simulationOrigin);
            }
            _lastSimulationTranslation = simulationOrigin;
        }
    }
}

void VoxelShapeManager::buildShapes() {
    // the shapes are owned by the elements of _voxels, 
    // so _shapes is constructed by harvesting them from _voxels
    _shapes.clear();
    int numVoxels = _voxels.size();
    for (int i = 0; i < numVoxels; ++i) {
        _shapes.push_back(_voxels[i]._shape);
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
    // sort incoming cubes
    qSort(cubes);

    // Some of the cubes are new, others already exist, and some old voxels no longer exist.
    // Since both lists are sorted we walk them simultaneously looking for matches.
    QVector<int> cubesToAdd;
    QVector<int> voxelsToRemove;
    int numCubes = cubes.size();
    int numVoxels = _voxels.size();
    int j = 0;
    for (int i = 0; i < numCubes; ++i) {
        while (j < numVoxels && _voxels[j]._cube < cubes[i]) {
            // remove non-matching voxels not found in cubes
            voxelsToRemove.push_back(j++);
        }
        if (j < numVoxels) {
            if (glm::distance2(cubes[i].getCorner(), _voxels[j]._cube.getCorner()) < EPSILON) {
                if (cubes[i].getScale() != _voxels[j]._cube.getScale()) {
                    // the voxel changed scale so we replace
                    voxelsToRemove.push_back(j++);
                    cubesToAdd.push_back(i);
                } else {
                    // the voxel already exists
                    ++j;
                }
            } else {
                // the voxel doesn't exist yet
                cubesToAdd.push_back(i);
            }
        } else {
            // all existing voxels have already been processed, so this one is new
            cubesToAdd.push_back(i);
        }
    }
    while (j < numVoxels) {
        // remove non-matching voxels at the end
        voxelsToRemove.push_back(j++);
    }

    // remove voxels identified as old, from back to front
    for (int i = voxelsToRemove.size() - 1; i >= 0; --i) {
        int k = voxelsToRemove[i];
        simulation->removeShape(_voxels[k]._shape);
        if (k < numVoxels - 1) {
            // copy the last voxel into this spot
            _voxels[k] = _voxels[numVoxels - 1];
        } 
        _voxels.pop_back();
        --numVoxels;
    }

    // add new voxels
    glm::vec3 simulationOrigin = simulation->getTranslation();
    for (int i = 0; i < cubesToAdd.size(); ++i) {
        const AACube& cube = cubes[cubesToAdd[i]];
        // NOTE: shape's position is in simulation frame
        AACubeShape* shape = new AACubeShape(cube.getScale(), cube.calcCenter() - simulationOrigin);
        shape->setEntity(this);
        VoxelInfo voxel = { cube, shape };
        _voxels.push_back(voxel);
        ++numVoxels;
    }

    if (cubesToAdd.size() > 0 || voxelsToRemove.size() > 0) {
        // keep _voxels sorted
        qSort(_voxels);
        buildShapes();
    }
}
