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
    voxelsRendered = 0;
    tree = new VoxelTree();
    pthread_mutex_init(&bufferWriteLock, NULL);
}

VoxelSystem::~VoxelSystem() {
    delete[] readVerticesArray;
    delete[] writeVerticesArray;
    delete[] readColorsArray;
    delete[] writeColorsArray;
    delete tree;
    pthread_mutex_destroy(&bufferWriteLock);
}

void VoxelSystem::setViewerAvatar(Avatar *newViewerAvatar) {
    viewerAvatar = newViewerAvatar;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Method:      VoxelSystem::loadVoxelsFile()
// Description: Loads HiFidelity encoded Voxels from a binary file. The current file
//              format is a stream of single voxels with NO color data. Currently
//              colors are set randomly
// Complaints:  Brad :)
// To Do:       Need to add color data to the file.
void VoxelSystem::loadVoxelsFile(const char* fileName, bool wantColorRandomizer) {

    tree->loadVoxelsFile(fileName,wantColorRandomizer);

    copyWrittenDataToReadArrays();
}

//////////////////////////////////////////////////////////////////////////////////////////
// Method:      VoxelSystem::createSphere()
// Description: Creates a sphere of voxels in the local system at a given location/radius
// To Do:       Move this function someplace better? I put it here because we need a
//              mechanism to tell the system to redraw it's arrays after voxels are done
//              being added. This is a concept mostly only understood by VoxelSystem.
// Complaints:  Brad :)
void VoxelSystem::createSphere(float r,float xc, float yc, float zc, float s, bool solid, bool wantColorRandomizer) {

    tree->createSphere(r,xc,yc,zc,s,solid,wantColorRandomizer);
    setupNewVoxelsForDrawing();
}

long int VoxelSystem::getVoxelsCreated() {
    return tree->voxelsCreated;
}

float VoxelSystem::getVoxelsCreatedPerSecondAverage() {
    return (1 / tree->voxelsCreatedStats.getEventDeltaAverage());
}

long int VoxelSystem::getVoxelsColored() {
    return tree->voxelsColored;
}

float VoxelSystem::getVoxelsColoredPerSecondAverage() {
    return (1 / tree->voxelsColoredStats.getEventDeltaAverage());
}

long int VoxelSystem::getVoxelsBytesRead() {
    return tree->voxelsBytesRead;
}

float VoxelSystem::getVoxelsBytesReadPerSecondAverage() {
    return tree->voxelsBytesReadStats.getAverageSampleValuePerSecond();
}

int VoxelSystem::parseData(unsigned char* sourceBuffer, int numBytes) {

    unsigned char command = *sourceBuffer;
    unsigned char *voxelData = sourceBuffer + 1;

    switch(command) {
        case PACKET_HEADER_VOXEL_DATA:
            // ask the VoxelTree to read the bitstream into the tree
            tree->readBitstreamToTree(voxelData, numBytes - 1);
        break;
        case PACKET_HEADER_ERASE_VOXEL:
            // ask the tree to read the "remove" bitstream
            tree->processRemoveVoxelBitstream(sourceBuffer, numBytes);
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
                    tree->eraseAllVoxels();
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
    // reset the verticesEndPointer so we're writing to the beginning of the array
    writeVerticesEndPointer = writeVerticesArray;
    // call recursive function to populate in memory arrays
    // it will return the number of voxels added
    glm::vec3 treeRoot = glm::vec3(0,0,0);
    voxelsRendered = treeToArrays(tree->rootNode, treeRoot);
    // copy the newly written data to the arrays designated for reading
    copyWrittenDataToReadArrays();
}

void VoxelSystem::copyWrittenDataToReadArrays() {
    // lock on the buffer write lock so we can't modify the data when the GPU is reading it
    pthread_mutex_lock(&bufferWriteLock);
    // store a pointer to the current end so it doesn't change during copy
    GLfloat *endOfCurrentVerticesData = writeVerticesEndPointer;
    // copy the vertices and colors
    memcpy(readVerticesArray, writeVerticesArray, (endOfCurrentVerticesData - writeVerticesArray) * sizeof(GLfloat));
    memcpy(readColorsArray, writeColorsArray, (endOfCurrentVerticesData - writeVerticesArray) * sizeof(GLubyte));
    // set the read vertices end pointer to the correct spot so the GPU knows how much to pull
    readVerticesEndPointer = readVerticesArray + (endOfCurrentVerticesData - writeVerticesArray);
    pthread_mutex_unlock(&bufferWriteLock);
}

int VoxelSystem::treeToArrays(VoxelNode* currentNode, const glm::vec3&  nodePosition) {
    int voxelsAdded = 0;
    float halfUnitForVoxel = powf(0.5, *currentNode->octalCode) * (0.5 * TREE_SCALE);
    glm::vec3 viewerPosition = viewerAvatar->getPosition();

    // debug LOD code
    glm::vec3 debugNodePosition;
    copyFirstVertexForCode(currentNode->octalCode,(float*)&debugNodePosition);

    float distanceToVoxelCenter = sqrtf(powf(viewerPosition.x - nodePosition[0] - halfUnitForVoxel, 2) +
                                        powf(viewerPosition.y - nodePosition[1] - halfUnitForVoxel, 2) +
                                        powf(viewerPosition.z - nodePosition[2] - halfUnitForVoxel, 2));

    int renderLevel = *currentNode->octalCode + 1;
    int boundaryPosition = boundaryDistanceForRenderLevel(renderLevel);
    bool alwaysDraw = false; // XXXBHG - temporary debug code. Flip this to true to disable LOD blurring

    if (alwaysDraw || distanceToVoxelCenter < boundaryPosition) {
        for (int i = 0; i < 8; i++) {
            // check if there is a child here
            if (currentNode->children[i] != NULL) {

                glm::vec3 childNodePosition;
                copyFirstVertexForCode(currentNode->children[i]->octalCode,(float*)&childNodePosition);
                childNodePosition *= (float)TREE_SCALE; // scale it up
                voxelsAdded += treeToArrays(currentNode->children[i], childNodePosition);
            }
        }
    }

    // if we didn't get any voxels added then we're a leaf (XXXBHG - Stephen can you explain this to me????)
    // add our vertex and color information to the interleaved array
    if (voxelsAdded == 0 && currentNode->isColored()) {
        float startVertex[3];
        copyFirstVertexForCode(currentNode->octalCode,(float*)&startVertex);
        float voxelScale = 1 / powf(2, *currentNode->octalCode);

        // populate the array with points for the 8 vertices
        // and RGB color for each added vertex
        for (int j = 0; j < VERTEX_POINTS_PER_VOXEL; j++ ) {
            *writeVerticesEndPointer = startVertex[j % 3] + (identityVertices[j] * voxelScale);
            *(writeColorsArray + (writeVerticesEndPointer - writeVerticesArray)) = currentNode->getColor()[j % 3];

            writeVerticesEndPointer++;
        }
        voxelsAdded++;
    }

    return voxelsAdded;
}

VoxelSystem* VoxelSystem::clone() const {
    // this still needs to be implemented, will need to be used if VoxelSystem is attached to agent
    return NULL;
}

void VoxelSystem::init() {
    // prep the data structures for incoming voxel data
    writeVerticesEndPointer = writeVerticesArray = new GLfloat[VERTEX_POINTS_PER_VOXEL * MAX_VOXELS_PER_SYSTEM];
    readVerticesEndPointer = readVerticesArray = new GLfloat[VERTEX_POINTS_PER_VOXEL * MAX_VOXELS_PER_SYSTEM];
    writeColorsArray = new GLubyte[VERTEX_POINTS_PER_VOXEL * MAX_VOXELS_PER_SYSTEM];
    readColorsArray = new GLubyte[VERTEX_POINTS_PER_VOXEL * MAX_VOXELS_PER_SYSTEM];

    GLuint *indicesArray = new GLuint[INDICES_PER_VOXEL * MAX_VOXELS_PER_SYSTEM];

    // populate the indicesArray
    // this will not change given new voxels, so we can set it all up now
    for (int n = 0; n < MAX_VOXELS_PER_SYSTEM; n++) {
        // fill the indices array
        int voxelIndexOffset = n * INDICES_PER_VOXEL;
        GLuint *currentIndicesPos = indicesArray + voxelIndexOffset;
        int startIndex = (n * VERTICES_PER_VOXEL);

        for (int i = 0; i < INDICES_PER_VOXEL; i++) {
            // add indices for this side of the cube
            currentIndicesPos[i] = startIndex + identityIndices[i];
        }
    }

    GLfloat *normalsArray = new GLfloat[VERTEX_POINTS_PER_VOXEL * MAX_VOXELS_PER_SYSTEM];
    GLfloat *normalsArrayEndPointer = normalsArray;

    // populate the normalsArray
    for (int n = 0; n < MAX_VOXELS_PER_SYSTEM; n++) {
        for (int i = 0; i < VERTEX_POINTS_PER_VOXEL; i++) {
            *(normalsArrayEndPointer++) = identityNormals[i];
        }
    }

    // VBO for the verticesArray
    glGenBuffers(1, &vboVerticesID);
    glBindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
    glBufferData(GL_ARRAY_BUFFER, VERTEX_POINTS_PER_VOXEL * sizeof(GLfloat) * MAX_VOXELS_PER_SYSTEM, NULL, GL_DYNAMIC_DRAW);

    // VBO for the normalsArray
    glGenBuffers(1, &vboNormalsID);
    glBindBuffer(GL_ARRAY_BUFFER, vboNormalsID);
    glBufferData(GL_ARRAY_BUFFER,
                 VERTEX_POINTS_PER_VOXEL * sizeof(GLfloat) * MAX_VOXELS_PER_SYSTEM,
                 normalsArray, GL_STATIC_DRAW);

    // VBO for colorsArray
    glGenBuffers(1, &vboColorsID);
    glBindBuffer(GL_ARRAY_BUFFER, vboColorsID);
    glBufferData(GL_ARRAY_BUFFER, VERTEX_POINTS_PER_VOXEL * sizeof(GLubyte) * MAX_VOXELS_PER_SYSTEM, NULL, GL_DYNAMIC_DRAW);

    // VBO for the indicesArray
    glGenBuffers(1, &vboIndicesID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndicesID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 INDICES_PER_VOXEL * sizeof(GLuint) * MAX_VOXELS_PER_SYSTEM,
                 indicesArray, GL_STATIC_DRAW);

    // delete the indices and normals arrays that are no longer needed
    delete[] indicesArray;
    delete[] normalsArray;
}

void VoxelSystem::render() {

    glPushMatrix();

    if (readVerticesEndPointer != readVerticesArray) {
        // try to lock on the buffer write
        // just avoid pulling new data if it is currently being written
        if (pthread_mutex_trylock(&bufferWriteLock) == 0) {

            glBindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
            glBufferSubData(GL_ARRAY_BUFFER, 0, (readVerticesEndPointer - readVerticesArray) * sizeof(GLfloat), readVerticesArray);

            glBindBuffer(GL_ARRAY_BUFFER, vboColorsID);
            glBufferSubData(GL_ARRAY_BUFFER, 0, (readVerticesEndPointer - readVerticesArray) * sizeof(GLubyte), readColorsArray);

            readVerticesEndPointer = readVerticesArray;

            pthread_mutex_unlock(&bufferWriteLock);
        }
    }

    // tell OpenGL where to find vertex and color information
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glBindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
    glVertexPointer(3, GL_FLOAT, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, vboNormalsID);
    glNormalPointer(GL_FLOAT, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, vboColorsID);
    glColorPointer(3, GL_UNSIGNED_BYTE, 0, 0);

    // draw the number of voxels we have
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndicesID);
    glScalef(10, 10, 10);
    glDrawElements(GL_TRIANGLES, 36 * voxelsRendered, GL_UNSIGNED_INT, 0);

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

void VoxelSystem::simulate(float deltaTime) {

}

int VoxelSystem::_nodeCount = 0;

bool VoxelSystem::randomColorOperation(VoxelNode* node, bool down, void* extraData) {

    // we do our operations on the way up!
    if (down) {
        return true;
    }

    _nodeCount++;
    if (node->isColored()) {
        nodeColor newColor = { 0,0,0,1 };
        newColor[0] = randomColorValue(150);
        newColor[1] = randomColorValue(150);
        newColor[1] = randomColorValue(150);
        node->setColor(newColor);
    }
    return true;
}

void VoxelSystem::randomizeVoxelColors() {
    _nodeCount = 0;
    tree->recurseTreeWithOperation(randomColorOperation);
    printLog("setting randomized true color for %d nodes\n",_nodeCount);
    setupNewVoxelsForDrawing();
}

bool VoxelSystem::falseColorizeRandomOperation(VoxelNode* node, bool down, void* extraData) {

    // we do our operations on the way up!
    if (down) {
        return true;
    }

    _nodeCount++;

    // always false colorize
    unsigned char newR = randomColorValue(150);
    unsigned char newG = randomColorValue(150);
    unsigned char newB = randomColorValue(150);
    node->setFalseColor(newR,newG,newB);

    return true; // keep going!
}

void VoxelSystem::falseColorizeRandom() {
    _nodeCount = 0;
    tree->recurseTreeWithOperation(falseColorizeRandomOperation);
    printLog("setting randomized false color for %d nodes\n",_nodeCount);
    setupNewVoxelsForDrawing();
}

bool VoxelSystem::trueColorizeOperation(VoxelNode* node, bool down, void* extraData) {

    // we do our operations on the way up!
    if (down) {
        return true;
    }

    _nodeCount++;
    node->setFalseColored(false);
    return true;
}

void VoxelSystem::trueColorize() {
    _nodeCount = 0;
    tree->recurseTreeWithOperation(trueColorizeOperation);
    printLog("setting true color for %d nodes\n",_nodeCount);
    setupNewVoxelsForDrawing();
}

// Will false colorize voxels that are not in view
bool VoxelSystem::falseColorizeInViewOperation(VoxelNode* node, bool down, void* extraData) {

    // we do our operations on the way up!
    if (down) {
        return true;
    }

    const ViewFrustum* viewFrustum = (const ViewFrustum*) extraData;

    _nodeCount++;

    // only do this for truely colored voxels...
    if (node->isColored()) {
        // If the voxel is outside of the view frustum, then false color it red
        if (!node->isInView(*viewFrustum)) {
            // Out of view voxels are colored RED
            unsigned char newR = 255;
            unsigned char newG = 0;
            unsigned char newB = 0;
            node->setFalseColor(newR,newG,newB);
        }
    }

    return true; // keep going!
}

void VoxelSystem::falseColorizeInView(ViewFrustum* viewFrustum) {
    _nodeCount = 0;
    tree->recurseTreeWithOperation(falseColorizeInViewOperation,(void*)viewFrustum);
    printLog("setting in view false color for %d nodes\n",_nodeCount);
    setupNewVoxelsForDrawing();
}

// Will false colorize voxels based on distance from view
bool VoxelSystem::falseColorizeDistanceFromViewOperation(VoxelNode* node, bool down, void* extraData) {

    // we do our operations on the way up!
    if (down) {
        return true;
    }

    ViewFrustum* viewFrustum = (ViewFrustum*) extraData;

    // only do this for truly colored voxels...
    if (node->isColored()) {

        // We need our distance for both up and down
        glm::vec3 nodePosition;
        float* startVertex = firstVertexForCode(node->octalCode);
        nodePosition.x = startVertex[0];
        nodePosition.y = startVertex[1];
        nodePosition.z = startVertex[2];
        delete startVertex;

        // scale up the node position
        nodePosition = nodePosition*(float)TREE_SCALE;

        float halfUnitForVoxel = powf(0.5, *node->octalCode) * (0.5 * TREE_SCALE);
        glm::vec3 viewerPosition = viewFrustum->getPosition();

        float distance = sqrtf(powf(viewerPosition.x - nodePosition.x - halfUnitForVoxel, 2) +
                                            powf(viewerPosition.y - nodePosition.y - halfUnitForVoxel, 2) +
                                            powf(viewerPosition.z - nodePosition.z - halfUnitForVoxel, 2));

        // actually colorize
        _nodeCount++;

        float distanceRatio = (_minDistance==_maxDistance) ? 1 : (distance - _minDistance)/(_maxDistance - _minDistance);

        // We want to colorize this in 16 bug chunks of color
        const unsigned char maxColor = 255;
        const unsigned char colorBands = 16;
        const unsigned char gradientOver = 128;
        unsigned char colorBand = (colorBands*distanceRatio);
        unsigned char newR = (colorBand*(gradientOver/colorBands))+(maxColor-gradientOver);
        unsigned char newG = 0;
        unsigned char newB = 0;
        node->setFalseColor(newR,newG,newB);
    }
    return true; // keep going!
}

float VoxelSystem::_maxDistance = 0.0;
float VoxelSystem::_minDistance = FLT_MAX;

// Helper function will get the distance from view range, would be nice if you could just keep track
// of this as voxels are created and/or colored... seems like some transform math could do that so
// we wouldn't need to do two passes of the tree
bool VoxelSystem::getDistanceFromViewRangeOperation(VoxelNode* node, bool down, void* extraData) {

    // we do our operations on the way up!
    if (down) {
        return true;
    }

    ViewFrustum* viewFrustum = (ViewFrustum*) extraData;

    // only do this for truly colored voxels...
    if (node->isColored()) {

        // We need our distance for both up and down
        glm::vec3 nodePosition;
        float* startVertex = firstVertexForCode(node->octalCode);
        nodePosition.x = startVertex[0];
        nodePosition.y = startVertex[1];
        nodePosition.z = startVertex[2];
        delete startVertex;

        // scale up the node position
        nodePosition = nodePosition*(float)TREE_SCALE;

        float halfUnitForVoxel = powf(0.5, *node->octalCode) * (0.5 * TREE_SCALE);
        glm::vec3 viewerPosition = viewFrustum->getPosition();

        float distance = sqrtf(powf(viewerPosition.x - nodePosition.x - halfUnitForVoxel, 2) +
                                            powf(viewerPosition.y - nodePosition.y - halfUnitForVoxel, 2) +
                                            powf(viewerPosition.z - nodePosition.z - halfUnitForVoxel, 2));

        // on way down, calculate the range of distances
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
    tree->recurseTreeWithOperation(getDistanceFromViewRangeOperation,(void*)viewFrustum);
    printLog("determining distance range for %d nodes\n",_nodeCount);

    _nodeCount = 0;

    tree->recurseTreeWithOperation(falseColorizeDistanceFromViewOperation,(void*)viewFrustum);
    printLog("setting in distance false color for %d nodes\n",_nodeCount);
    setupNewVoxelsForDrawing();
}
