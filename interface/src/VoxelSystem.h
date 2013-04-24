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
#include <UDPSocket.h>
#include <AgentData.h>
#include <VoxelTree.h>
#include <ViewFrustum.h>
#include "Head.h"
#include "Util.h"
#include "world.h"

const int NUM_CHILDREN = 8;

class VoxelSystem : public AgentData {
public:
    VoxelSystem();
    ~VoxelSystem();

    int parseData(unsigned char* sourceBuffer, int numBytes);
    VoxelSystem* clone() const;

    void init();
    void simulate(float deltaTime);
    void render();
    void setVoxelsRendered(int v) {voxelsRendered = v;};
    int getVoxelsRendered() {return voxelsRendered;};
    void setViewerHead(Head *newViewerHead);
    void loadVoxelsFile(const char* fileName,bool wantColorRandomizer);
	void createSphere(float r,float xc, float yc, float zc, float s, bool solid, bool wantColorRandomizer);

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

private:
    // Operation functions for tree recursion methods
    static int _nodeCount;
    static bool randomColorOperation(VoxelNode* node, bool down, void* extraData);
    static bool falseColorizeRandomOperation(VoxelNode* node, bool down, void* extraData);
    static bool trueColorizeOperation(VoxelNode* node, bool down, void* extraData);
    static bool falseColorizeInViewOperation(VoxelNode* node, bool down, void* extraData);
    static bool falseColorizeDistanceFromViewOperation(VoxelNode* node, bool down, void* extraData);
    static bool getDistanceFromViewRangeOperation(VoxelNode* node, bool down, void* extraData);

    // these are kinda hacks, used by getDistanceFromViewRangeOperation() probably shouldn't be here
    static float _maxDistance;
    static float _minDistance;

    int voxelsRendered;
    Head *viewerHead;
    VoxelTree *tree;
    GLfloat *readVerticesArray;
    GLubyte *readColorsArray;
    GLfloat *readVerticesEndPointer;
    GLfloat *writeVerticesArray;
    GLubyte *writeColorsArray;
    GLfloat *writeVerticesEndPointer;
    GLuint vboVerticesID;
    GLuint vboNormalsID;
    GLuint vboColorsID;
    GLuint vboIndicesID;
    pthread_mutex_t bufferWriteLock;

    int treeToArrays(VoxelNode *currentNode, const glm::vec3& nodePosition);
    void setupNewVoxelsForDrawing();
    void copyWrittenDataToReadArrays();
};

#endif
