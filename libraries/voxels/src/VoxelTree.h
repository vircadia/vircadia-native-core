//
//  VoxelTree.h
//  hifi
//
//  Created by Stephen Birarda on 3/13/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__VoxelTree__
#define __hifi__VoxelTree__

#include "SimpleMovingAverage.h"

#include "ViewFrustum.h"
#include "VoxelNode.h"
#include "VoxelNodeBag.h"

// Callback function, for recuseTreeWithOperation
typedef bool (*RecurseVoxelTreeOperation)(VoxelNode* node, void* extraData);

class VoxelTree {
public:
    // when a voxel is created in the tree (object new'd)
	long voxelsCreated;
    // when a voxel is colored/set in the tree (object may have already existed)
	long voxelsColored;
	long voxelsBytesRead;
    
    SimpleMovingAverage voxelsCreatedStats;
	SimpleMovingAverage voxelsColoredStats;
	SimpleMovingAverage voxelsBytesReadStats;

    VoxelTree();
    ~VoxelTree();
    
    VoxelNode *rootNode;
    int leavesWrittenToBitstream;

	void eraseAllVoxels();

	void processRemoveVoxelBitstream(unsigned char * bitstream, int bufferSizeBytes);
    void readBitstreamToTree(unsigned char * bitstream, int bufferSizeBytes);
    void readCodeColorBufferToTree(unsigned char *codeColorBuffer);
	void deleteVoxelCodeFromTree(unsigned char *codeBuffer);
    void printTreeForDebugging(VoxelNode *startNode);
    void reaverageVoxelColors(VoxelNode *startNode);
	void loadVoxelsFile(const char* fileName, bool wantColorRandomizer);
	void createSphere(float r,float xc, float yc, float zc, float s, bool solid, bool wantColorRandomizer);
	
    void recurseTreeWithOperation(RecurseVoxelTreeOperation operation, void* extraData=NULL);

    int encodeTreeBitstream(int maxEncodeLevel, VoxelNode* node, const ViewFrustum& viewFrustum,
                            unsigned char* outputBuffer, int availableBytes,
                            VoxelNodeBag& bag);

    int searchForColoredNodes(int maxSearchLevel, VoxelNode* node, const ViewFrustum& viewFrustum, VoxelNodeBag& bag);
    
private:
    int encodeTreeBitstreamRecursion(int maxEncodeLevel, int& currentEncodeLevel,
                                     VoxelNode* node, const ViewFrustum& viewFrustum,
                                     unsigned char* outputBuffer, int availableBytes,
                                     VoxelNodeBag& bag) const;

    int searchForColoredNodesRecursion(int maxSearchLevel, int& currentSearchLevel, 
                                       VoxelNode* node, const ViewFrustum& viewFrustum, VoxelNodeBag& bag);

    void recurseNodeWithOperation(VoxelNode* node, RecurseVoxelTreeOperation operation, void* extraData);
    VoxelNode* nodeForOctalCode(VoxelNode* ancestorNode, unsigned char* needleCode, VoxelNode** parentOfFoundNode);
    VoxelNode* createMissingNode(VoxelNode* lastParentNode, unsigned char* deepestCodeToCreate);
    int readNodeData(VoxelNode *destinationNode, unsigned char* nodeData, int bufferSizeBytes);
};

int boundaryDistanceForRenderLevel(unsigned int renderLevel);

#endif /* defined(__hifi__VoxelTree__) */
