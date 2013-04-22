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

void VoxelAgentData::parseData(unsigned char* sourceBuffer, int numBytes) {
    // call base class to parse the data
    AvatarData::parseData(sourceBuffer,numBytes);
    
    // make sure our class knows it's position
    memcpy(&position, &_bodyPosition, sizeof(_bodyPosition));
}

