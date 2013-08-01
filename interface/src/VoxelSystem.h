//
//  Cube.h
//  interface
//
//  Created by Philip on 12/31/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Cube__
#define __interface__Cube__

#include "InterfaceConfig.h"
#include <glm/glm.hpp>

#include <SharedUtil.h>
#include <UDPSocket.h>

#include <CoverageMapV2.h>
#include <NodeData.h>
#include <ViewFrustum.h>
#include <VoxelTree.h>

#include "Camera.h"
#include "Util.h"
#include "world.h"

class ProgramObject;

const int NUM_CHILDREN = 8;

class VoxelSystem : public NodeData, public VoxelNodeDeleteHook {
public:
    VoxelSystem(float treeScale = TREE_SCALE, int maxVoxels = MAX_VOXELS_PER_SYSTEM);
    ~VoxelSystem();

    int parseData(unsigned char* sourceBuffer, int numBytes);
    
    virtual void init();
    void simulate(float deltaTime) { };
    void render(bool texture);

    unsigned long  getVoxelsUpdated() const {return _voxelsUpdated;};
    unsigned long  getVoxelsRendered() const {return _voxelsInReadArrays;};

    void loadVoxelsFile(const char* fileName,bool wantColorRandomizer);
    void writeToSVOFile(const char* filename, VoxelNode* node) const;
    bool readFromSVOFile(const char* filename);

    long int getVoxelsCreated();
    long int getVoxelsColored();
    long int getVoxelsBytesRead();
    float getVoxelsCreatedPerSecondAverage();
    float getVoxelsColoredPerSecondAverage();
    float getVoxelsBytesReadPerSecondAverage();

    // Methods that recurse tree
    void randomizeVoxelColors();
    void falseColorizeRandom();
    void trueColorize();
    void falseColorizeInView(ViewFrustum* viewFrustum);
    void falseColorizeDistanceFromView(ViewFrustum* viewFrustum);
    void falseColorizeRandomEveryOther();
    void falseColorizeOccluded();
    void falseColorizeOccludedV2();

    void killLocalVoxels();
    void setRenderPipelineWarnings(bool on) { _renderWarningsOn = on; };
    bool getRenderPipelineWarnings() const { return _renderWarningsOn; };

    virtual void removeOutOfView();
    bool hasViewChanged();
    bool isViewChanging();
    
    bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                             VoxelDetail& detail, float& distance, BoxFace& face);
    
    bool findSpherePenetration(const glm::vec3& center, float radius, glm::vec3& penetration);
    bool findCapsulePenetration(const glm::vec3& start, const glm::vec3& end, float radius, glm::vec3& penetration);
    
    void collectStatsForTreesAndVBOs();

    void deleteVoxelAt(float x, float y, float z, float s);
    VoxelNode* getVoxelAt(float x, float y, float z, float s) const;
    void createVoxel(float x, float y, float z, float s, 
                     unsigned char red, unsigned char green, unsigned char blue, bool destructive = false);
    void createLine(glm::vec3 point1, glm::vec3 point2, float unitSize, rgbColor color, bool destructive = false);
    void createSphere(float r,float xc, float yc, float zc, float s, bool solid, 
                      creationMode mode, bool destructive = false, bool debug = false);

    void copySubTreeIntoNewTree(VoxelNode* startNode, VoxelTree* destinationTree, bool rebaseToRoot);
    void copyFromTreeIntoSubTree(VoxelTree* sourceTree, VoxelNode* destinationNode);

    CoverageMapV2 myCoverageMapV2;
    CoverageMap   myCoverageMap;

    virtual void nodeDeleted(VoxelNode* node);
    
protected:
    float _treeScale; 
    int _maxVoxels;      
    VoxelTree* _tree;
    
    glm::vec3 computeVoxelVertex(const glm::vec3& startVertex, float voxelScale, int index) const;
    
    void setupNewVoxelsForDrawing();
    
    virtual void updateNodeInArrays(glBufferIndex nodeIndex, const glm::vec3& startVertex,
                                    float voxelScale, const nodeColor& color);
    virtual void copyWrittenDataSegmentToReadArrays(glBufferIndex segmentStart, glBufferIndex segmentEnd);
    virtual void updateVBOSegment(glBufferIndex segmentStart, glBufferIndex segmentEnd);
    virtual void applyScaleAndBindProgram(bool texture);
    virtual void removeScaleAndReleaseProgram(bool texture);

private:
    // disallow copying of VoxelSystem objects
    VoxelSystem(const VoxelSystem&);
    VoxelSystem& operator= (const VoxelSystem&);
    
    int  _callsToTreesToArrays;
    VoxelNodeBag _removedVoxels;

    bool _renderWarningsOn;
    // Operation functions for tree recursion methods
    static int _nodeCount;
    static bool randomColorOperation(VoxelNode* node, void* extraData);
    static bool falseColorizeRandomOperation(VoxelNode* node, void* extraData);
    static bool trueColorizeOperation(VoxelNode* node, void* extraData);
    static bool falseColorizeInViewOperation(VoxelNode* node, void* extraData);
    static bool falseColorizeDistanceFromViewOperation(VoxelNode* node, void* extraData);
    static bool getDistanceFromViewRangeOperation(VoxelNode* node, void* extraData);
    static bool removeOutOfViewOperation(VoxelNode* node, void* extraData);
    static bool falseColorizeRandomEveryOtherOperation(VoxelNode* node, void* extraData);
    static bool collectStatsForTreesAndVBOsOperation(VoxelNode* node, void* extraData);
    static bool falseColorizeOccludedOperation(VoxelNode* node, void* extraData);
    static bool falseColorizeSubTreeOperation(VoxelNode* node, void* extraData);
    static bool falseColorizeOccludedV2Operation(VoxelNode* node, void* extraData);


    int updateNodeInArraysAsFullVBO(VoxelNode* node);
    int updateNodeInArraysAsPartialVBO(VoxelNode* node);

    void copyWrittenDataToReadArraysFullVBOs();
    void copyWrittenDataToReadArraysPartialVBOs();

    void updateVBOs();

    // these are kinda hacks, used by getDistanceFromViewRangeOperation() probably shouldn't be here
    static float _maxDistance;
    static float _minDistance;

    GLfloat* _readVerticesArray;
    GLubyte* _readColorsArray;
    GLfloat* _writeVerticesArray;
    GLubyte* _writeColorsArray;
    bool* _writeVoxelDirtyArray;
    bool* _readVoxelDirtyArray;
    unsigned long _voxelsUpdated;
    unsigned long _voxelsInReadArrays;
    unsigned long _voxelsInWriteArrays;
    unsigned long _abandonedVBOSlots;
    
    bool _writeRenderFullVBO;
    bool _readRenderFullVBO;
    
    int _setupNewVoxelsForDrawingLastElapsed;
    uint64_t _setupNewVoxelsForDrawingLastFinished;
    uint64_t _lastViewCulling;
    int _lastViewCullingElapsed;
    
    GLuint _vboVerticesID;
    GLuint _vboNormalsID;
    GLuint _vboColorsID;
    GLuint _vboIndicesID;
    pthread_mutex_t _bufferWriteLock;
    pthread_mutex_t _treeLock;

    ViewFrustum _lastKnowViewFrustum;
    ViewFrustum _lastStableViewFrustum;

    int newTreeToArrays(VoxelNode *currentNode);
    void cleanupRemovedVoxels();

    void copyWrittenDataToReadArrays(bool fullVBOs);
    
    void updateFullVBOs(); // all voxels in the VBO
    void updatePartialVBOs(); // multiple segments, only dirty voxels

    bool _voxelsDirty;
    
    static ProgramObject* _perlinModulateProgram;
    static GLuint _permutationNormalTextureID;
    
    int _hookID;
    std::vector<glBufferIndex> _freeIndexes;

    void freeBufferIndex(glBufferIndex index);
    void clearFreeBufferIndexes();
};

#endif
