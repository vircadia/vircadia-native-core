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
    memcpy(position, otherAgentData.position, sizeof(float) * 3);
    rootMarkerNode = new MarkerNode();
}

VoxelAgentData* VoxelAgentData::clone() const {
    return new VoxelAgentData(*this);
}

void VoxelAgentData::parseData(void *data, int size) {
    // pull the position from the interface agent data packet
    sscanf((char *)data,
           "H%*f,%*f,%*f,%f,%f,%f",
           &position[0], &position[1], &position[2]);
}
