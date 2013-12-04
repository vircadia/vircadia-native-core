//
//  VoxelSceneStats.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 7/18/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __hifi__VoxelSceneStats__
#define __hifi__VoxelSceneStats__

#include <NodeList.h>
#include <OctreeSceneStats.h>

/// Collects statistics for calculating and sending a scene from a voxel server to an interface client
class VoxelSceneStats : public OctreeSceneStats {

    // currently an alias for OctreeSceneStats
    
};

/// Map between node IDs and their reported VoxelSceneStats. Typically used by classes that need to know which nodes sent
/// which voxel stats
typedef std::map<QUuid, VoxelSceneStats> NodeToVoxelSceneStats;
typedef std::map<QUuid, VoxelSceneStats>::iterator NodeToVoxelSceneStatsIterator;

#endif /* defined(__hifi__VoxelSceneStats__) */
