//
//  VoxelAgentData.h
//  hifi
//
//  Created by Stephen Birarda on 3/21/13.
//
//

#ifndef __hifi__VoxelAgentData__
#define __hifi__VoxelAgentData__

#include <iostream>
#include <AgentData.h>
#include "MarkerNode.h"

class VoxelAgentData : public AgentData {
public:
    float position[3];
    MarkerNode *rootMarkerNode;

    VoxelAgentData();
    ~VoxelAgentData();
    VoxelAgentData(const VoxelAgentData &otherAgentData);
    
    void parseData(void *data, int size);
    VoxelAgentData* clone() const;
};

#endif /* defined(__hifi__VoxelAgentData__) */
