//
//  VoxelPacketData.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 11/19/2013.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <PerfStat.h>
#include "VoxelPacketData.h"

// currently just an alias for OctreePacketData

VoxelPacketData::VoxelPacketData(bool enableCompression, int maxFinalizedSize) : 
    OctreePacketData(enableCompression, maxFinalizedSize) {
};
