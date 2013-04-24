//
//  VoxelAgentData.cpp
//  hifi
//
//  Created by Stephen Birarda on 3/21/13.
//
//

#include "VoxelAgentData.h"
#include <cstring>
#include <cstdio>

VoxelAgentData::VoxelAgentData() {
    rootMarkerNode = new MarkerNode();
}

VoxelAgentData::~VoxelAgentData() {
    delete rootMarkerNode;
}

VoxelAgentData::VoxelAgentData(const VoxelAgentData &otherAgentData) {
    memcpy(&_Position, &otherAgentData._Position, sizeof(_Position));
    rootMarkerNode = new MarkerNode();
}

VoxelAgentData* VoxelAgentData::clone() const {
    return new VoxelAgentData(*this);
}
