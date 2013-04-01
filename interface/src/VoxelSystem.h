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
#include <iostream>
#include <UDPSocket.h>
#include <AgentData.h>
#include <VoxelTree.h>
#include "Head.h"
#include "Util.h"
#include "world.h"

const int NUM_CHILDREN = 8;

class VoxelSystem : public AgentData {
public:
    VoxelSystem();
    ~VoxelSystem();
    
    void parseData(void *data, int size);
    VoxelSystem* clone() const;
    
    void init();
    void simulate(float deltaTime);
    void render();
    void setVoxelsRendered(int v) {voxelsRendered = v;};
    int getVoxelsRendered() {return voxelsRendered;};
    void setViewerHead(Head *newViewerHead);
    void loadVoxelsFile(const char* fileName,bool wantColorRandomizer);
	void createSphere(float r,float xc, float yc, float zc, float s, bool solid, bool wantColorRandomizer);
private:
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
    GLuint vboColorsID;
    GLuint vboIndicesID;
    pthread_mutex_t bufferWriteLock;

    int treeToArrays(VoxelNode *currentNode, float nodePosition[3]);
    void setupNewVoxelsForDrawing();
    void copyWrittenDataToReadArrays();
};

#endif
