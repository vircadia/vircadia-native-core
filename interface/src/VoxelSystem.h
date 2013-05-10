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
#include <AgentData.h>
#include <VoxelTree.h>
#include <ViewFrustum.h>
#include "Avatar.h"
#include "Camera.h"
#include "Util.h"
#include "world.h"

const int NUM_CHILDREN = 8;

class VoxelSystem : public AgentData {
public:
    VoxelSystem();
    ~VoxelSystem();

    int parseData(unsigned char* sourceBuffer, int numBytes);
    VoxelSystem* clone() const;
    
    void setViewFrustum(ViewFrustum* viewFrustum) { _viewFrustum = viewFrustum; };

    void init();
    void simulate(float deltaTime) { };
    void render();

    unsigned long  getVoxelsUpdated() const {return _voxelsUpdated;};
    unsigned long  getVoxelsRendered() const {return _voxelsInReadArrays;};

    void setViewerAvatar(Avatar *newViewerAvatar) { _viewerAvatar = newViewerAvatar; };
    void setCamera(Camera* newCamera) { _camera = newCamera; };
    void loadVoxelsFile(const char* fileName,bool wantColorRandomizer);

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

    void killLocalVoxels();
    void setRenderPipelineWarnings(bool on) { _renderWarningsOn = on; };
    bool getRenderPipelineWarnings() const { return _renderWarningsOn; };

    void removeOutOfView();
    bool hasViewChanged();
    
    bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                             VoxelDetail& detail, float& distance, BoxFace& face);

    void collectStatsForTreesAndVBOs();
    
private:
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

    int updateNodeInArraysAsFullVBO(VoxelNode* node);
    int updateNodeInArraysAsPartialVBO(VoxelNode* node);

    // these are kinda hacks, used by getDistanceFromViewRangeOperation() probably shouldn't be here
    static float _maxDistance;
    static float _minDistance;

    Avatar* _viewerAvatar;
    Camera* _camera;
    VoxelTree* _tree;
    GLfloat* _readVerticesArray;
    GLubyte* _readColorsArray;
    GLfloat* _writeVerticesArray;
    GLubyte* _writeColorsArray;
    bool* _voxelDirtyArray;
    unsigned long _voxelsUpdated;
    unsigned long _voxelsInWriteArrays;
    unsigned long _voxelsInReadArrays;
    unsigned long _unusedArraySpace;
    
    bool _renderFullVBO;
    
    double _setupNewVoxelsForDrawingLastElapsed;
    double _setupNewVoxelsForDrawingLastFinished;
    double _lastViewCulling;
    double _lastViewCullingElapsed;
    
    GLuint _vboVerticesID;
    GLuint _vboNormalsID;
    GLuint _vboColorsID;
    GLuint _vboIndicesID;
    pthread_mutex_t _bufferWriteLock;

    ViewFrustum* _viewFrustum;
    ViewFrustum _lastKnowViewFrustum;

    int newTreeToArrays(VoxelNode *currentNode);
    void cleanupRemovedVoxels();

    void setupNewVoxelsForDrawing();
    void copyWrittenDataToReadArrays();

    bool _voxelsDirty;

public:
    void updateVBOs();
    void updateFullVBOs(); // all voxels in the VBO
    void updatePartialVBOs(); // multiple segments, only dirty voxels
    
};

#endif
