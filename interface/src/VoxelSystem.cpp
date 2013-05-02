//
//  Cube.cpp
//  interface
//
//  Created by Philip on 12/31/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//
#ifdef _WIN32
#define _timeval_
#define _USE_MATH_DEFINES
#endif
#include <cstring>
#include <cmath>
#include <iostream> // to load voxels from file
#include <fstream> // to load voxels from file
#include <SharedUtil.h>
#include <PacketHeaders.h>
#include <OctalCode.h>
#include <pthread.h>
#include "Log.h"
#include "VoxelConstants.h"

#include "VoxelSystem.h"

float identityVertices[] = { 0,0,0, 1,0,0, 1,1,0, 0,1,0, 0,0,1, 1,0,1, 1,1,1, 0,1,1,
                             0,0,0, 1,0,0, 1,1,0, 0,1,0, 0,0,1, 1,0,1, 1,1,1, 0,1,1,
                             0,0,0, 1,0,0, 1,1,0, 0,1,0, 0,0,1, 1,0,1, 1,1,1, 0,1,1 };

GLfloat identityNormals[] = { 0,0,-1, 0,0,-1, 0,0,-1, 0,0,-1,
                              0,0,+1, 0,0,+1, 0,0,+1, 0,0,+1,
                              0,-1,0, 0,-1,0, 0,+1,0, 0,+1,0,
                              0,-1,0, 0,-1,0, 0,+1,0, 0,+1,0,
                              -1,0,0, +1,0,0, +1,0,0, -1,0,0,
                              -1,0,0, +1,0,0, +1,0,0, -1,0,0 };

GLubyte identityIndices[] = { 0,2,1,    0,3,2,    // Z- .
                              8,9,13,   8,13,12,  // Y-
                              16,23,19, 16,20,23, // X-
                              17,18,22, 17,22,21, // X+
                              10,11,15, 10,15,14, // Y+
                              4,5,6,    4,6,7 };  // Z+ .

VoxelSystem::VoxelSystem() {
    _voxelsInArrays = _voxelsUpdated = 0;
    _tree = new VoxelTree();
    pthread_mutex_init(&_bufferWriteLock, NULL);
}

VoxelSystem::~VoxelSystem() {
    delete[] _readVerticesArray;
    delete[] _writeVerticesArray;
    delete[] _readColorsArray;
    delete[] _writeColorsArray;
    delete[] _voxelDirtyArray;
    delete _tree;
    pthread_mutex_destroy(&_bufferWriteLock);
}

void VoxelSystem::loadVoxelsFile(const char* fileName, bool wantColorRandomizer) {
    _tree->loadVoxelsFile(fileName, wantColorRandomizer);
    copyWrittenDataToReadArrays();
}

void VoxelSystem::createSphere(float r,float xc, float yc, float zc, float s, bool solid, bool wantColorRandomizer) {
    _tree->createSphere(r, xc, yc, zc, s, solid, wantColorRandomizer);
    setupNewVoxelsForDrawing();
}

long int VoxelSystem::getVoxelsCreated() {
    return _tree->voxelsCreated;
}

float VoxelSystem::getVoxelsCreatedPerSecondAverage() {
    return (1 / _tree->voxelsCreatedStats.getEventDeltaAverage());
}

long int VoxelSystem::getVoxelsColored() {
    return _tree->voxelsColored;
}

float VoxelSystem::getVoxelsColoredPerSecondAverage() {
    return (1 / _tree->voxelsColoredStats.getEventDeltaAverage());
}

long int VoxelSystem::getVoxelsBytesRead() {
    return _tree->voxelsBytesRead;
}

float VoxelSystem::getVoxelsBytesReadPerSecondAverage() {
    return _tree->voxelsBytesReadStats.getAverageSampleValuePerSecond();
}

int VoxelSystem::parseData(unsigned char* sourceBuffer, int numBytes) {

    unsigned char command = *sourceBuffer;
    unsigned char *voxelData = sourceBuffer + 1;

    switch(command) {
        case PACKET_HEADER_VOXEL_DATA:
            // ask the VoxelTree to read the bitstream into the tree
            _tree->readBitstreamToTree(voxelData, numBytes - 1);
        break;
        case PACKET_HEADER_ERASE_VOXEL:
            // ask the tree to read the "remove" bitstream
            _tree->processRemoveVoxelBitstream(sourceBuffer, numBytes);
        break;
        case PACKET_HEADER_Z_COMMAND:

            // the Z command is a special command that allows the sender to send high level semantic
            // requests, like erase all, or add sphere scene, different receivers may handle these
            // messages differently
            char* packetData = (char *)sourceBuffer;
            char* command = &packetData[1]; // start of the command
            int commandLength = strlen(command); // commands are null terminated strings
            int totalLength = 1+commandLength+1;

            printLog("got Z message len(%d)= %s\n", numBytes, command);

            while (totalLength <= numBytes) {
                if (0==strcmp(command,(char*)"erase all")) {
                    printLog("got Z message == erase all\n");
                    _tree->eraseAllVoxels();
                    _voxelsInArrays = 0; // better way to do this??
                }
                if (0==strcmp(command,(char*)"add scene")) {
                    printLog("got Z message == add scene - NOT SUPPORTED ON INTERFACE\n");
                }
                totalLength += commandLength+1;
            }
        break;
    }

    setupNewVoxelsForDrawing();
    return numBytes;
}

void VoxelSystem::setupNewVoxelsForDrawing() {
    _voxelsUpdated = newTreeToArrays(_tree->rootNode);
    if (_voxelsUpdated) {
        _voxelsDirty=true;
    }

    // copy the newly written data to the arrays designated for reading
    copyWrittenDataToReadArrays();
}

void VoxelSystem::copyWrittenDataToReadArrays() {
    if (_voxelsDirty) {
        // lock on the buffer write lock so we can't modify the data when the GPU is reading it
        pthread_mutex_lock(&_bufferWriteLock);
        int bytesOfVertices = (_voxelsInArrays * VERTEX_POINTS_PER_VOXEL) * sizeof(GLfloat);
        int bytesOfColors   = (_voxelsInArrays * VERTEX_POINTS_PER_VOXEL) * sizeof(GLubyte);
        memcpy(_readVerticesArray, _writeVerticesArray, bytesOfVertices);
        memcpy(_readColorsArray,   _writeColorsArray,   bytesOfColors  );
        pthread_mutex_unlock(&_bufferWriteLock);
    }
}

int VoxelSystem::newTreeToArrays(VoxelNode* node) {
    assert(_viewFrustum); // you must set up _viewFrustum before calling this
    int   voxelsUpdated   = 0;
    float distanceToNode  = node->distanceToCamera(*_viewFrustum);
    float boundary        = boundaryDistanceForRenderLevel(*node->octalCode + 1);
    float childBoundary   = boundaryDistanceForRenderLevel(*node->octalCode + 2);
    bool  inBoundary      = (distanceToNode <= boundary);
    bool  inChildBoundary = (distanceToNode <= childBoundary);
    bool shouldRender     = node->isColored() && ((node->isLeaf() && inChildBoundary) || (inBoundary && !inChildBoundary));

    node->setShouldRender(shouldRender);
    // let children figure out their renderness
    for (int i = 0; i < 8; i++) {
        if (node->children[i]) {
            voxelsUpdated += newTreeToArrays(node->children[i]);
        }
    }
    
    // Now, if we've changed any attributes (our renderness, our color, etc) then update the Arrays... for us
    if (node->isDirty() && (shouldRender || node->isKnownBufferIndex())) {
        glm::vec3 startVertex;
        float voxelScale = 0;
        
        // If we're should render, use our legit location and scale, 
        if (node->getShouldRender()) {
            copyFirstVertexForCode(node->octalCode, (float*)&startVertex);
            voxelScale = (1 / powf(2, *node->octalCode));
        } else {
            // if we shouldn't render then set out location to some infinitely distant location, 
            // and our scale as infinitely small
            startVertex[0] = startVertex[1] = startVertex[2] = FLT_MAX;
            voxelScale = 0;
        }

        // If this node has not yet been written to the array, then add it to the end of the array.
        glBufferIndex nodeIndex;
        if (node->isKnownBufferIndex()) {
            nodeIndex = node->getBufferIndex();
        } else {
            nodeIndex = _voxelsInArrays;
        }
        
        _voxelDirtyArray[nodeIndex] = true;

        // populate the array with points for the 8 vertices
        // and RGB color for each added vertex
        for (int j = 0; j < VERTEX_POINTS_PER_VOXEL; j++ ) {
            GLfloat* writeVerticesAt = _writeVerticesArray + (nodeIndex * VERTEX_POINTS_PER_VOXEL);
            GLubyte* writeColorsAt   = _writeColorsArray   + (nodeIndex * VERTEX_POINTS_PER_VOXEL);
            *(writeVerticesAt+j) = startVertex[j % 3] + (identityVertices[j] * voxelScale);
            *(writeColorsAt  +j) = node->getColor()[j % 3];
        }
        if (!node->isKnownBufferIndex()) {
            node->setBufferIndex(nodeIndex);
            _voxelsInArrays++; // our know vertices in the arrays
        }
        voxelsUpdated++;
        node->clearDirtyBit();
    }
    return voxelsUpdated;
}

VoxelSystem* VoxelSystem::clone() const {
    // this still needs to be implemented, will need to be used if VoxelSystem is attached to agent
    return NULL;
}

void VoxelSystem::init() {

    // When we change voxels representations in the arrays, we'll update this
    _voxelsDirty = false;
    _voxelsInArrays = 0;

    // we will track individual dirty sections with this array of bools
    _voxelDirtyArray = new bool[MAX_VOXELS_PER_SYSTEM];
    memset(_voxelDirtyArray, false, MAX_VOXELS_PER_SYSTEM * sizeof(bool));

    // prep the data structures for incoming voxel data
    _writeVerticesArray = new GLfloat[VERTEX_POINTS_PER_VOXEL * MAX_VOXELS_PER_SYSTEM];
    _readVerticesArray = new GLfloat[VERTEX_POINTS_PER_VOXEL * MAX_VOXELS_PER_SYSTEM];

    _writeColorsArray = new GLubyte[VERTEX_POINTS_PER_VOXEL * MAX_VOXELS_PER_SYSTEM];
    _readColorsArray = new GLubyte[VERTEX_POINTS_PER_VOXEL * MAX_VOXELS_PER_SYSTEM];

    GLuint* indicesArray = new GLuint[INDICES_PER_VOXEL * MAX_VOXELS_PER_SYSTEM];

    // populate the indicesArray
    // this will not change given new voxels, so we can set it all up now
    for (int n = 0; n < MAX_VOXELS_PER_SYSTEM; n++) {
        // fill the indices array
        int voxelIndexOffset = n * INDICES_PER_VOXEL;
        GLuint* currentIndicesPos = indicesArray + voxelIndexOffset;
        int startIndex = (n * VERTICES_PER_VOXEL);

        for (int i = 0; i < INDICES_PER_VOXEL; i++) {
            // add indices for this side of the cube
            currentIndicesPos[i] = startIndex + identityIndices[i];
        }
    }

    GLfloat* normalsArray = new GLfloat[VERTEX_POINTS_PER_VOXEL * MAX_VOXELS_PER_SYSTEM];
    GLfloat* normalsArrayEndPointer = normalsArray;

    // populate the normalsArray
    for (int n = 0; n < MAX_VOXELS_PER_SYSTEM; n++) {
        for (int i = 0; i < VERTEX_POINTS_PER_VOXEL; i++) {
            *(normalsArrayEndPointer++) = identityNormals[i];
        }
    }

    // VBO for the verticesArray
    glGenBuffers(1, &_vboVerticesID);
    glBindBuffer(GL_ARRAY_BUFFER, _vboVerticesID);
    glBufferData(GL_ARRAY_BUFFER, VERTEX_POINTS_PER_VOXEL * sizeof(GLfloat) * MAX_VOXELS_PER_SYSTEM, NULL, GL_DYNAMIC_DRAW);

    // VBO for the normalsArray
    glGenBuffers(1, &_vboNormalsID);
    glBindBuffer(GL_ARRAY_BUFFER, _vboNormalsID);
    glBufferData(GL_ARRAY_BUFFER,
                 VERTEX_POINTS_PER_VOXEL * sizeof(GLfloat) * MAX_VOXELS_PER_SYSTEM,
                 normalsArray, GL_STATIC_DRAW);

    // VBO for colorsArray
    glGenBuffers(1, &_vboColorsID);
    glBindBuffer(GL_ARRAY_BUFFER, _vboColorsID);
    glBufferData(GL_ARRAY_BUFFER, VERTEX_POINTS_PER_VOXEL * sizeof(GLubyte) * MAX_VOXELS_PER_SYSTEM, NULL, GL_DYNAMIC_DRAW);

    // VBO for the indicesArray
    glGenBuffers(1, &_vboIndicesID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vboIndicesID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 INDICES_PER_VOXEL * sizeof(GLuint) * MAX_VOXELS_PER_SYSTEM,
                 indicesArray, GL_STATIC_DRAW);

    // delete the indices and normals arrays that are no longer needed
    delete[] indicesArray;
    delete[] normalsArray;
}

void VoxelSystem::updateVBOs() {
    if (_voxelsDirty) {
        glBufferIndex segmentStart = 0;
        glBufferIndex segmentEnd = 0;
        bool inSegment = false;
        for (glBufferIndex i = 0; i < _voxelsInArrays; i++) {
            if (!inSegment) {
                if (_voxelDirtyArray[i]) {
                    segmentStart = i;
                    inSegment = true;
                    _voxelDirtyArray[i] = false; // consider us clean!
                }
            } else {
                if (!_voxelDirtyArray[i] || (i == (_voxelsInArrays - 1)) ) {
                    segmentEnd = i;
                    inSegment = false;
                    int segmentLength = (segmentEnd - segmentStart) + 1;
                    GLintptr   segmentStartAt   = segmentStart * VERTEX_POINTS_PER_VOXEL * sizeof(GLfloat);
                    GLsizeiptr segmentSizeBytes = segmentLength * VERTEX_POINTS_PER_VOXEL * sizeof(GLfloat);
                    GLfloat* readVerticesFrom   = _readVerticesArray + (segmentStart * VERTEX_POINTS_PER_VOXEL);
                    glBindBuffer(GL_ARRAY_BUFFER, _vboVerticesID);
                    glBufferSubData(GL_ARRAY_BUFFER, segmentStartAt, segmentSizeBytes, readVerticesFrom);
                    segmentStartAt          = segmentStart * VERTEX_POINTS_PER_VOXEL * sizeof(GLubyte);
                    segmentSizeBytes        = segmentLength * VERTEX_POINTS_PER_VOXEL * sizeof(GLubyte);
                    GLubyte* readColorsFrom = _readColorsArray   + (segmentStart * VERTEX_POINTS_PER_VOXEL);
                    glBindBuffer(GL_ARRAY_BUFFER, _vboColorsID);
                    glBufferSubData(GL_ARRAY_BUFFER, segmentStartAt, segmentSizeBytes, readColorsFrom);
                }
            }
        }
        _voxelsDirty = false;
    }
}

void VoxelSystem::render() {
    glPushMatrix();
    updateVBOs();
    // tell OpenGL where to find vertex and color information
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glBindBuffer(GL_ARRAY_BUFFER, _vboVerticesID);
    glVertexPointer(3, GL_FLOAT, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, _vboNormalsID);
    glNormalPointer(GL_FLOAT, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, _vboColorsID);
    glColorPointer(3, GL_UNSIGNED_BYTE, 0, 0);

    // draw the number of voxels we have
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vboIndicesID);
    glScalef(10, 10, 10);
    glDrawElements(GL_TRIANGLES, 36 * _voxelsInArrays, GL_UNSIGNED_INT, 0);

    // deactivate vertex and color arrays after drawing
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    // bind with 0 to switch back to normal operation
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // scale back down to 1 so heads aren't massive
    glPopMatrix();
}

int VoxelSystem::_nodeCount = 0;

bool VoxelSystem::randomColorOperation(VoxelNode* node, void* extraData) {
    _nodeCount++;
    if (node->isColored()) {
        nodeColor newColor = { randomColorValue(150), randomColorValue(150), randomColorValue(150), 1 };
        node->setColor(newColor);
    }
    return true;
}

void VoxelSystem::randomizeVoxelColors() {
    _nodeCount = 0;
    _tree->recurseTreeWithOperation(randomColorOperation);
    printLog("setting randomized true color for %d nodes\n", _nodeCount);
    setupNewVoxelsForDrawing();
}

bool VoxelSystem::falseColorizeRandomOperation(VoxelNode* node, void* extraData) {
    _nodeCount++;
    // always false colorize
    node->setFalseColor(randomColorValue(150), randomColorValue(150), randomColorValue(150));
    return true; // keep going!
}

void VoxelSystem::falseColorizeRandom() {
    _nodeCount = 0;
    _tree->recurseTreeWithOperation(falseColorizeRandomOperation);
    printLog("setting randomized false color for %d nodes\n", _nodeCount);
    setupNewVoxelsForDrawing();
}

bool VoxelSystem::trueColorizeOperation(VoxelNode* node, void* extraData) {
    _nodeCount++;
    node->setFalseColored(false);
    return true;
}

void VoxelSystem::trueColorize() {
    _nodeCount = 0;
    _tree->recurseTreeWithOperation(trueColorizeOperation);
    printLog("setting true color for %d nodes\n", _nodeCount);
    setupNewVoxelsForDrawing();
}

// Will false colorize voxels that are not in view
bool VoxelSystem::falseColorizeInViewOperation(VoxelNode* node, void* extraData) {
    const ViewFrustum* viewFrustum = (const ViewFrustum*) extraData;
    _nodeCount++;
    if (node->isColored()) {
        if (!node->isInView(*viewFrustum)) {
            // Out of view voxels are colored RED
            node->setFalseColor(255, 0, 0);
        }
    }
    return true; // keep going!
}

void VoxelSystem::falseColorizeInView(ViewFrustum* viewFrustum) {
    _nodeCount = 0;
    _tree->recurseTreeWithOperation(falseColorizeInViewOperation,(void*)viewFrustum);
    printLog("setting in view false color for %d nodes\n", _nodeCount);
    setupNewVoxelsForDrawing();
}

// Will false colorize voxels based on distance from view
bool VoxelSystem::falseColorizeDistanceFromViewOperation(VoxelNode* node, void* extraData) {
    ViewFrustum* viewFrustum = (ViewFrustum*) extraData;
    if (node->isColored()) {
        float distance = node->distanceToCamera(*viewFrustum);
        _nodeCount++;
        float distanceRatio = (_minDistance == _maxDistance) ? 1 : (distance - _minDistance) / (_maxDistance - _minDistance);

        // We want to colorize this in 16 bug chunks of color
        const unsigned char maxColor = 255;
        const unsigned char colorBands = 16;
        const unsigned char gradientOver = 128;
        unsigned char colorBand = (colorBands * distanceRatio);
        node->setFalseColor((colorBand * (gradientOver / colorBands)) + (maxColor - gradientOver), 0, 0);
    }
    return true; // keep going!
}

float VoxelSystem::_maxDistance = 0.0;
float VoxelSystem::_minDistance = FLT_MAX;

// Helper function will get the distance from view range, would be nice if you could just keep track
// of this as voxels are created and/or colored... seems like some transform math could do that so
// we wouldn't need to do two passes of the tree
bool VoxelSystem::getDistanceFromViewRangeOperation(VoxelNode* node, void* extraData) {
    ViewFrustum* viewFrustum = (ViewFrustum*) extraData;
    // only do this for truly colored voxels...
    if (node->isColored()) {
        float distance = node->distanceToCamera(*viewFrustum);
        // calculate the range of distances
        if (distance > _maxDistance) {
            _maxDistance = distance;
        }
        if (distance < _minDistance) {
            _minDistance = distance;
        }
        _nodeCount++;
    }
    return true; // keep going!
}

void VoxelSystem::falseColorizeDistanceFromView(ViewFrustum* viewFrustum) {
    _nodeCount = 0;
    _maxDistance = 0.0;
    _minDistance = FLT_MAX;
    _tree->recurseTreeWithOperation(getDistanceFromViewRangeOperation,(void*)viewFrustum);
    printLog("determining distance range for %d nodes\n", _nodeCount);
    _nodeCount = 0;
    _tree->recurseTreeWithOperation(falseColorizeDistanceFromViewOperation,(void*)viewFrustum);
    printLog("setting in distance false color for %d nodes\n", _nodeCount);
    setupNewVoxelsForDrawing();
}


