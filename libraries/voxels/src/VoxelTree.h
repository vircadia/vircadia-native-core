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
typedef enum {GRADIENT, RANDOM, NATURAL} creationMode;

#define NO_EXISTS_BITS   false
#define WANT_EXISTS_BITS true
#define NO_COLOR         false
#define WANT_COLOR       true

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
    void readBitstreamToTree(unsigned char * bitstream,  unsigned long int bufferSizeBytes, 
                             bool includeColor = WANT_COLOR, bool includeExistsBits = false);
    void readCodeColorBufferToTree(unsigned char *codeColorBuffer, bool destructive = false);
    void deleteVoxelCodeFromTree(unsigned char *codeBuffer, bool stage = false);
    void printTreeForDebugging(VoxelNode *startNode);
    void reaverageVoxelColors(VoxelNode *startNode);

    void deleteVoxelAt(float x, float y, float z, float s, bool stage = false);
    VoxelNode* getVoxelAt(float x, float y, float z, float s) const;
    void createVoxel(float x, float y, float z, float s, 
                     unsigned char red, unsigned char green, unsigned char blue, bool destructive = false);
    void createLine(glm::vec3 point1, glm::vec3 point2, float unitSize, rgbColor color, bool destructive = false);
    void createSphere(float r,float xc, float yc, float zc, float s, bool solid, 
                      creationMode mode, bool destructive = false, bool debug = false);

    void recurseTreeWithOperation(RecurseVoxelTreeOperation operation, void* extraData=NULL);

    int encodeTreeBitstream(int maxEncodeLevel, VoxelNode* node, unsigned char* outputBuffer, int availableBytes,
                            VoxelNodeBag& bag, const ViewFrustum* viewFrustum, 
                            bool includeColor = WANT_COLOR, bool includeExistsBits = false,
                            bool deltaViewFrustum = false, const ViewFrustum* lastViewFrustum = NULL) const;

    int searchForColoredNodes(int maxSearchLevel, VoxelNode* node, const ViewFrustum& viewFrustum, VoxelNodeBag& bag, 
            bool deltaViewFrustum = false, const ViewFrustum* lastViewFrustum = NULL);

    bool isDirty() const { return _isDirty; };
    void clearDirtyBit() { _isDirty = false; };
    void setDirtyBit() { _isDirty = true; };
    unsigned long int getNodesChangedFromBitstream() const { return _nodesChangedFromBitstream; };

    bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                             VoxelNode*& node, float& distance, BoxFace& face);

    // Note: this assumes the fileFormat is the HIO individual voxels code files
    void loadVoxelsFile(const char* fileName, bool wantColorRandomizer);

    // these will read/write files that match the wireformat, excluding the 'V' leading
    void writeToFileV2(const char* filename) const;
    bool readFromFileV2(const char* filename);

    unsigned long getVoxelCount();
    
private:
    int encodeTreeBitstreamRecursion(int maxEncodeLevel, int& currentEncodeLevel,
                                     VoxelNode* node, unsigned char* outputBuffer, int availableBytes, VoxelNodeBag& bag, 
                                     const ViewFrustum* viewFrustum, bool includeColor, bool includeExistsBits,
                                     bool deltaViewFrustum, const ViewFrustum* lastViewFrustum) const;

    int searchForColoredNodesRecursion(int maxSearchLevel, int& currentSearchLevel, 
                                       VoxelNode* node, const ViewFrustum& viewFrustum, VoxelNodeBag& bag,
                                       bool deltaViewFrustum, const ViewFrustum* lastViewFrustum);

    static bool countVoxelsOperation(VoxelNode* node, void* extraData);

    void recurseNodeWithOperation(VoxelNode* node, RecurseVoxelTreeOperation operation, void* extraData);
    VoxelNode* nodeForOctalCode(VoxelNode* ancestorNode, unsigned char* needleCode, VoxelNode** parentOfFoundNode) const;
    VoxelNode* createMissingNode(VoxelNode* lastParentNode, unsigned char* deepestCodeToCreate);
    int readNodeData(VoxelNode *destinationNode, unsigned char* nodeData, int bufferSizeBytes, 
                     bool includeColor = WANT_COLOR, bool includeExistsBits = false);
    
    bool _isDirty;
    unsigned long int _nodesChangedFromBitstream;
};

int boundaryDistanceForRenderLevel(unsigned int renderLevel);

#endif /* defined(__hifi__VoxelTree__) */
