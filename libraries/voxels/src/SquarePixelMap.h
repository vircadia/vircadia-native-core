//
//  SquarePixelMap.h
//  hifi
//
//  Created by Tomáš Horáček on 6/25/13.
//
//

#ifndef __hifi__SquarePixelMap__
#define __hifi__SquarePixelMap__

#include <stdint.h>
#include "VoxelTree.h"
#include "SharedUtil.h"

class PixelQuadTreeNode;

struct SquarePixelMapData {
    const uint32_t* pixels;
    int dimension;
    int reference_counter;
};

class SquarePixelMap {
public:
    SquarePixelMap(const uint32_t *pixels, int dimension);
    SquarePixelMap(const SquarePixelMap& other);
    ~SquarePixelMap();
    
    void addVoxelsToVoxelTree(VoxelTree *voxelTree);
    
    int dimension();
    uint32_t getPixelAt(unsigned int x, unsigned int y);
    uint8_t getAlphaAt(int x, int y);
private:
    SquarePixelMapData *_data;
    PixelQuadTreeNode *_rootPixelQuadTreeNode;
    
    void generateRootPixelQuadTreeNode();
    void createVoxelsFromPixelQuadTreeToVoxelTree(PixelQuadTreeNode *pixelQuadTreeNode, VoxelTree *voxelTree);
    VoxelDetail getVoxelDetail(PixelQuadTreeNode *pixelQuadTreeNode);
};

#endif /* defined(__hifi__SquarePixelMap__) */
