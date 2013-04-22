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
#include "MarkerNode.h"

const int MAX_VOXEL_PACKET_SIZE = 1492;
const int MAX_TREE_SLICE_BYTES = 26;
const int TREE_SCALE = 10;

// Callback function, for recuseTreeWithOperation
typedef bool (*RecurseVoxelTreeOperation)(VoxelNode* node, bool down, void* extraData);

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
    unsigned char * loadBitstreamBuffer(unsigned char *& bitstreamBuffer,
                                        VoxelNode *currentVoxelNode,
                                        MarkerNode *currentMarkerNode,
                                        const glm::vec3& agentPosition,
                                        float thisNodePosition[3],
                                        const ViewFrustum& viewFrustum,
                                        bool viewFrustumCulling,
                                        unsigned char * octalCode = NULL);
    
	void loadVoxelsFile(const char* fileName, bool wantColorRandomizer);
	void createSphere(float r,float xc, float yc, float zc, float s, bool solid, bool wantColorRandomizer);
	
    void recurseTreeWithOperation(RecurseVoxelTreeOperation operation, void* extraData=NULL);
	
private:
    void recurseNodeWithOperation(VoxelNode* node,RecurseVoxelTreeOperation operation, void* extraData);
    VoxelNode * nodeForOctalCode(VoxelNode *ancestorNode, unsigned char * needleCode, VoxelNode** parentOfFoundNode);
    VoxelNode * createMissingNode(VoxelNode *lastParentNode, unsigned char *deepestCodeToCreate);
    int readNodeData(VoxelNode *destinationNode, unsigned char * nodeData, int bufferSizeBytes);
};

int boundaryDistanceForRenderLevel(unsigned int renderLevel);

#endif /* defined(__hifi__VoxelTree__) */
