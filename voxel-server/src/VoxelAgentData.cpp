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
    memcpy(&_bodyPosition, &otherAgentData._bodyPosition, sizeof(_bodyPosition));
    rootMarkerNode = new MarkerNode();
}

VoxelAgentData* VoxelAgentData::clone() const {
    return new VoxelAgentData(*this);
}
