//
//  VoxelTree.h
//  hifi
//
//  Created by Stephen Birarda on 3/13/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__VoxelTree__
#define __hifi__VoxelTree__

#include "CounterStats.h"

#include "VoxelNode.h"
#include "MarkerNode.h"

const int MAX_VOXEL_PACKET_SIZE = 1492;
const int MAX_TREE_SLICE_BYTES = 26;
const int TREE_SCALE = 10;

class VoxelTree {
    VoxelNode * nodeForOctalCode(VoxelNode *ancestorNode, unsigned char * needleCode, VoxelNode** parentOfFoundNode);
    VoxelNode * createMissingNode(VoxelNode *lastParentNode, unsigned char *deepestCodeToCreate);
    int readNodeData(VoxelNode *destinationNode, unsigned char * nodeData, int bufferSizeBytes);


public:
	long int voxelsCreated;
	long int voxelsColored;
	long int voxelsBytesRead;
	
	CounterStatHistory voxelsCreatedStats;
	CounterStatHistory voxelsColoredStats;
	CounterStatHistory voxelsBytesReadStats;

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
                                        float * agentPosition,
                                        float thisNodePosition[3],
                                        unsigned char * octalCode = NULL);
    
	void loadVoxelsFile(const char* fileName, bool wantColorRandomizer);
	void createSphere(float r,float xc, float yc, float zc, float s, bool solid, bool wantColorRandomizer);
};

int boundaryDistanceForRenderLevel(unsigned int renderLevel);

#endif /* defined(__hifi__VoxelTree__) */
