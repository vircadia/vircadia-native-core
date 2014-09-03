//
//  VoxelShapeManager.h
//  interface/src/avatar
//
//  Created by Andrew Meadows on 2014.09.02
//  Copyright 2012 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_VoxelShapeManager_h
#define hifi_VoxelShapeManager_h

#include <AACube.h>
#include <PhysicsEntity.h>
#include <Octree.h>

#include "VoxelShapeManager.h"

class AACubeShape;

class VoxelInfo{
public:
    bool operator<(const VoxelInfo& otherVoxel) const { return _cube < otherVoxel._cube; }
    AACube _cube;
    AACubeShape* _shape;
};

class VoxelShapeManager : public PhysicsEntity {
public:
	VoxelShapeManager();
    ~VoxelShapeManager();

    void stepForward(float deltaTime);
    void buildShapes();
    void clearShapes();

    /// \param cubes list of AACubes representing all of the voxels that should be in this VoxelShapeManager
    void updateVoxels(CubeList& cubes);


private:
    glm::vec3 _lastSimulationTranslation;
    QVector<VoxelInfo> _voxels;
};

#endif // hifi_VoxelShapeManager_h
