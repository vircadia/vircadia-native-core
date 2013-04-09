//
//  VoxelNode.h
//  hifi
//
//  Created by Stephen Birarda on 3/13/13.
//
//

#ifndef __hifi__VoxelNode__
#define __hifi__VoxelNode__

#include <iostream>

class VoxelNode {
public:
    VoxelNode();
    ~VoxelNode();
    
    void addChildAtIndex(int childIndex);
    void setColorFromAverageOfChildren();
    void setRandomColor(int minimumBrightness);
    bool collapseIdenticalLeaves();
    
    unsigned char *octalCode;
    unsigned char color[4];
    VoxelNode *children[8];
};

#endif /* defined(__hifi__VoxelNode__) */
