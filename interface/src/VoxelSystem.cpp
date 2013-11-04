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
#include <pthread.h>

#include <OctalCode.h>
#include <PacketHeaders.h>
#include <PerfStat.h>
#include <SharedUtil.h>
#include <NodeList.h>
#include <NodeTypes.h>

#include "Application.h"
#include "CoverageMap.h"
#include "CoverageMapV2.h"
#include "InterfaceConfig.h"
#include "Menu.h"
#include "renderer/ProgramObject.h"
#include "VoxelConstants.h"
#include "VoxelSystem.h"

const bool VoxelSystem::DONT_BAIL_EARLY = false;


float identityVerticesGlobalNormals[] = { 0,0,0, 1,0,0, 1,1,0, 0,1,0, 0,0,1, 1,0,1, 1,1,1, 0,1,1 };

float identityVertices[] = { 0,0,0, 1,0,0, 1,1,0, 0,1,0, 0,0,1, 1,0,1, 1,1,1, 0,1,1, //0-7
                             0,0,0, 1,0,0, 1,1,0, 0,1,0, 0,0,1, 1,0,1, 1,1,1, 0,1,1, //8-15
                             0,0,0, 1,0,0, 1,1,0, 0,1,0, 0,0,1, 1,0,1, 1,1,1, 0,1,1 }; // 16-23

GLfloat identityNormals[] = { 0,0,-1, 0,0,-1, 0,0,-1, 0,0,-1,
                              0,0,+1, 0,0,+1, 0,0,+1, 0,0,+1,
                              0,-1,0, 0,-1,0, 0,+1,0, 0,+1,0,
                              0,-1,0, 0,-1,0, 0,+1,0, 0,+1,0,
                              -1,0,0, +1,0,0, +1,0,0, -1,0,0,
                              -1,0,0, +1,0,0, +1,0,0, -1,0,0 };

GLubyte identityIndices[] = { 0,2,1,    0,3,2,    // Z-
                              8,9,13,   8,13,12,  // Y-
                              16,23,19, 16,20,23, // X-
                              17,18,22, 17,22,21, // X+
                              10,11,15, 10,15,14, // Y+
                              4,5,6,    4,6,7 };  // Z+

GLubyte identityIndicesTop[]    = {  2, 3, 7,  2, 7, 6 };
GLubyte identityIndicesBottom[] = {  0, 1, 5,  0, 5, 4 };
GLubyte identityIndicesLeft[]   = {  0, 7, 3,  0, 4, 7 };
GLubyte identityIndicesRight[]  = {  1, 2, 6,  1, 6, 5 };
GLubyte identityIndicesFront[]  = {  0, 2, 1,  0, 3, 2 };
GLubyte identityIndicesBack[]   = {  4, 5, 6,  4, 6, 7 };

VoxelSystem::VoxelSystem(float treeScale, int maxVoxels)
    : NodeData(NULL),
      _treeScale(treeScale),
      _maxVoxels(maxVoxels),
      _initialized(false) {

    _voxelsInReadArrays = _voxelsInWriteArrays = _voxelsUpdated = 0;
    _writeRenderFullVBO = true;
    _readRenderFullVBO = true;
    _tree = new VoxelTree();

    _tree->rootNode->setVoxelSystem(this);
    pthread_mutex_init(&_bufferWriteLock, NULL);
    pthread_mutex_init(&_treeLock, NULL);
    pthread_mutex_init(&_freeIndexLock, NULL);

    VoxelNode::addDeleteHook(this);
    VoxelNode::addUpdateHook(this);
    _abandonedVBOSlots = 0;
    _falseColorizeBySource = false;
    _dataSourceUUID = QUuid();
    _voxelServerCount = 0;

    _viewFrustum = Application::getInstance()->getViewFrustum();

    connect(_tree, SIGNAL(importSize(float,float,float)), SIGNAL(importSize(float,float,float)));
    connect(_tree, SIGNAL(importProgress(int)), SIGNAL(importProgress(int)));

    _useVoxelShader = false;
    _voxelsAsPoints = false;
    _voxelShaderModeWhenVoxelsAsPointsEnabled = false;

    _writeVoxelShaderData = NULL;
    _readVoxelShaderData = NULL;

    _readVerticesArray = NULL;
    _writeVerticesArray = NULL;
    _readColorsArray = NULL;
    _writeColorsArray = NULL;
    _writeVoxelDirtyArray = NULL;
    _readVoxelDirtyArray = NULL;

    _inSetupNewVoxelsForDrawing = false;
    _useFastVoxelPipeline = false;
    
    _culledOnce = false;
    _inhideOutOfView = false;
    _treeIsBusy = false;
}

void VoxelSystem::voxelDeleted(VoxelNode* node) {
    if (node->getVoxelSystem() == this) {
        if (_voxelsInWriteArrays != 0) {
            forceRemoveNodeFromArrays(node);
        } else {
            if (Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings)) {
                printf("VoxelSystem::voxelDeleted() while _voxelsInWriteArrays==0, is that expected? \n");
            }
        }
    }
}

void VoxelSystem::setDisableFastVoxelPipeline(bool disableFastVoxelPipeline) {
    _useFastVoxelPipeline = !disableFastVoxelPipeline;
    printf("setDisableFastVoxelPipeline() disableFastVoxelPipeline=%s _useFastVoxelPipeline=%s\n", 
        debug::valueOf(disableFastVoxelPipeline), debug::valueOf(_useFastVoxelPipeline));
    setupNewVoxelsForDrawing();
}

void VoxelSystem::voxelUpdated(VoxelNode* node) {
    // If we're in SetupNewVoxelsForDrawing() or _writeRenderFullVBO then bail..
    if (!_useFastVoxelPipeline || _inSetupNewVoxelsForDrawing || _writeRenderFullVBO) {
        return;
    }

    if (node->getVoxelSystem() == this) {
        bool shouldRender = false; // assume we don't need to render it
        // if it's colored, we might need to render it!
        float voxelSizeScale = Menu::getInstance()->getVoxelSizeScale();
        int boundaryLevelAdjust = Menu::getInstance()->getBoundaryLevelAdjust();
        shouldRender = node->calculateShouldRender(_viewFrustum, voxelSizeScale, boundaryLevelAdjust);

        if (node->getShouldRender() != shouldRender) {
            node->setShouldRender(shouldRender);
        }
        
        if (!node->isLeaf()) {
    
            // As we check our children, see if any of them went from shouldRender to NOT shouldRender
            // then we probably dropped LOD and if we don't have color, we want to average our children 
            // for a new color.
            int childrenGotHiddenCount = 0;
            for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
                VoxelNode* childNode = node->getChildAtIndex(i);
                if (childNode) {
                    bool wasShouldRender = childNode->getShouldRender();
                    bool isShouldRender = childNode->calculateShouldRender(_viewFrustum, voxelSizeScale, boundaryLevelAdjust);
                    if (wasShouldRender && !isShouldRender) {
                        childrenGotHiddenCount++;
                    }
                }
            }
            if (childrenGotHiddenCount > 0) {
                node->setColorFromAverageOfChildren();
            }
        }

        const bool REUSE_INDEX = true;
        const bool DONT_FORCE_REDRAW = false;
        updateNodeInArrays(node, REUSE_INDEX, DONT_FORCE_REDRAW);
        _voxelsUpdated++;

        node->clearDirtyBit(); // clear the dirty bit, do this before we potentially delete things.
        
        setupNewVoxelsForDrawingSingleNode();
    }
}

// returns an available index, starts by reusing a previously freed index, but if there isn't one available
// it will use the end of the VBO array and grow our accounting of that array.
// and makes the index available for some other node to use
glBufferIndex VoxelSystem::getNextBufferIndex() {
    glBufferIndex output = GLBUFFER_INDEX_UNKNOWN;
    // if there's a free index, use it...
    if (_freeIndexes.size() > 0) {
        pthread_mutex_lock(&_freeIndexLock);
        output = _freeIndexes.back();
        _freeIndexes.pop_back();
        pthread_mutex_unlock(&_freeIndexLock);
    } else {
        output = _voxelsInWriteArrays;
        _voxelsInWriteArrays++;
    }
    return output;
}

// Release responsibility of the buffer/vbo index from the VoxelNode, and makes the index available for some other node to use
// will also "clean up" the index data for the buffer/vbo slot, so that if it's in the middle of the draw range, the triangles
// will be "invisible"
void VoxelSystem::freeBufferIndex(glBufferIndex index) {
    if (_voxelsInWriteArrays == 0) {
        qDebug() << "freeBufferIndex() called when _voxelsInWriteArrays == 0!!!!\n";
    }

    // if the "freed" index was our max index, then just drop the _voxelsInWriteArrays down one...
    bool inList = false;

    // make sure the index isn't already in the free list..., this is a debugging measure only done if you've enabled audits
    if (Menu::getInstance()->isOptionChecked(MenuOption::AutomaticallyAuditTree)) {
        for (long i = 0; i < _freeIndexes.size(); i++) {
            if (_freeIndexes[i] == index) {
                printf("freeBufferIndex(glBufferIndex index)... index=%ld already in free list!\n", index);
                inList = true;
                break;
            }
        }
    }    
    if (!inList) {
        // make the index available for next node that needs to be drawn
        pthread_mutex_lock(&_freeIndexLock);
        _freeIndexes.push_back(index);
        pthread_mutex_unlock(&_freeIndexLock);

        // make the VBO slot "invisible" in case this slot is not used
        const glm::vec3 startVertex(FLT_MAX, FLT_MAX, FLT_MAX);
        const float voxelScale = 0;
        const nodeColor BLACK = {0, 0, 0, 0};
        updateArraysDetails(index, startVertex, voxelScale, BLACK);
    }
}

// This will run through the list of _freeIndexes and reset their VBO array values to be "invisible".
void VoxelSystem::clearFreeBufferIndexes() {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "clearFreeBufferIndexes()");
    _voxelsInWriteArrays = 0; // reset our VBO
    _abandonedVBOSlots = 0;

    // clear out freeIndexes
    {
        PerformanceWarning warn(showWarnings,"clearFreeBufferIndexes() : pthread_mutex_lock(&_freeIndexLock)");
        pthread_mutex_lock(&_freeIndexLock);
    }
    {
        PerformanceWarning warn(showWarnings,"clearFreeBufferIndexes() : _freeIndexes.clear()");
        _freeIndexes.clear();
    }
    pthread_mutex_unlock(&_freeIndexLock);
}

VoxelSystem::~VoxelSystem() {
    cleanupVoxelMemory();
    delete _tree;
    pthread_mutex_destroy(&_bufferWriteLock);
    pthread_mutex_destroy(&_treeLock);
    pthread_mutex_destroy(&_freeIndexLock);

    VoxelNode::removeDeleteHook(this);
    VoxelNode::removeUpdateHook(this);
}

void VoxelSystem::setMaxVoxels(int maxVoxels) {
    if (maxVoxels == _maxVoxels) {
        return;
    }
    bool wasInitialized = _initialized;
    if (wasInitialized) {
        clearAllNodesBufferIndex();
        cleanupVoxelMemory();
    }
    _maxVoxels = maxVoxels;
    if (wasInitialized) {
        initVoxelMemory();
    }
    if (wasInitialized) {
        forceRedrawEntireTree();
    }
}

void VoxelSystem::setUseVoxelShader(bool useVoxelShader) {
    if (_useVoxelShader == useVoxelShader) {
        return;
    }

    bool wasInitialized = _initialized;
    if (wasInitialized) {
        clearAllNodesBufferIndex();
        cleanupVoxelMemory();
    }
    _useVoxelShader = useVoxelShader;
    if (wasInitialized) {
        initVoxelMemory();
    }

    if (wasInitialized) {
        forceRedrawEntireTree();
    }
}

void VoxelSystem::setVoxelsAsPoints(bool voxelsAsPoints) {
    if (_voxelsAsPoints == voxelsAsPoints) {
        return;
    }

    bool wasInitialized = _initialized;
    
    // If we're "turning on" Voxels as points, we need to double check that we're in voxel shader mode.
    // Voxels as points uses the VoxelShader memory model, so if we're not in voxel shader mode,
    // then set it to voxel shader mode.
    if (voxelsAsPoints) {
        Menu::getInstance()->getUseVoxelShader()->setEnabled(false);

        // If enabling this... then do it before checking voxel shader status, that way, if voxel
        // shader is already enabled, we just start drawing as points.
        _voxelsAsPoints = true;
    
        if (!_useVoxelShader) {
            setUseVoxelShader(true);
            _voxelShaderModeWhenVoxelsAsPointsEnabled = false;
        } else {
            _voxelShaderModeWhenVoxelsAsPointsEnabled = true;
        }
    } else {
        Menu::getInstance()->getUseVoxelShader()->setEnabled(true);
        // if we're turning OFF voxels as point mode, then we check what the state of voxel shader was when we enabled
        // voxels as points, if it was OFF, then we return it to that value.
        if (_voxelShaderModeWhenVoxelsAsPointsEnabled == false) {
            setUseVoxelShader(false);
        }
        // If disabling this... then do it AFTER checking previous voxel shader status, that way, if voxel
        // shader is was not enabled, we switch back to normal mode before turning off points.
        _voxelsAsPoints = false;
    }

    // Set our voxels as points
    if (wasInitialized) {
        forceRedrawEntireTree();
    }
}

void VoxelSystem::cleanupVoxelMemory() {
    if (_initialized) {
        pthread_mutex_lock(&_bufferWriteLock);
        _initialized = false; // no longer initialized
        if (_useVoxelShader) {
            // these are used when in VoxelShader mode.
            glDeleteBuffers(1, &_vboVoxelsID);
            glDeleteBuffers(1, &_vboVoxelsIndicesID);

            delete[] _writeVoxelShaderData;
            delete[] _readVoxelShaderData;
            
            _writeVoxelShaderData = _readVoxelShaderData = NULL;

        } else {
            // Destroy  glBuffers
            glDeleteBuffers(1, &_vboVerticesID);
            glDeleteBuffers(1, &_vboColorsID);

            glDeleteBuffers(1, &_vboIndicesTop);
            glDeleteBuffers(1, &_vboIndicesBottom);
            glDeleteBuffers(1, &_vboIndicesLeft);
            glDeleteBuffers(1, &_vboIndicesRight);
            glDeleteBuffers(1, &_vboIndicesFront);
            glDeleteBuffers(1, &_vboIndicesBack);
        
            delete[] _readVerticesArray;
            delete[] _writeVerticesArray;
            delete[] _readColorsArray;
            delete[] _writeColorsArray;

            _readVerticesArray = NULL;
            _writeVerticesArray = NULL;
            _readColorsArray = NULL;
            _writeColorsArray = NULL;

        }
        delete[] _writeVoxelDirtyArray;
        delete[] _readVoxelDirtyArray;
        _writeVoxelDirtyArray = _readVoxelDirtyArray = NULL;
        pthread_mutex_unlock(&_bufferWriteLock);
    }
}

void VoxelSystem::setupFaceIndices(GLuint& faceVBOID, GLubyte faceIdentityIndices[]) {
    GLuint* indicesArray = new GLuint[INDICES_PER_FACE * _maxVoxels];

    // populate the indicesArray
    // this will not change given new voxels, so we can set it all up now
    for (int n = 0; n < _maxVoxels; n++) {
        // fill the indices array
        int voxelIndexOffset = n * INDICES_PER_FACE;
        GLuint* currentIndicesPos = indicesArray + voxelIndexOffset;
        int startIndex = (n * GLOBAL_NORMALS_VERTICES_PER_VOXEL);

        for (int i = 0; i < INDICES_PER_FACE; i++) {
            // add indices for this side of the cube
            currentIndicesPos[i] = startIndex + faceIdentityIndices[i];
        }
    }

    glGenBuffers(1, &faceVBOID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, faceVBOID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 INDICES_PER_FACE * sizeof(GLuint) * _maxVoxels,
                 indicesArray, GL_STATIC_DRAW);
    _memoryUsageVBO += INDICES_PER_FACE * sizeof(GLuint) * _maxVoxels;

    // delete the indices and normals arrays that are no longer needed
    delete[] indicesArray;
}

void VoxelSystem::initVoxelMemory() {
    pthread_mutex_lock(&_bufferWriteLock);

    _memoryUsageRAM = 0;
    _memoryUsageVBO = 0; // our VBO allocations as we know them
    
    // if _voxelsAsPoints then we must have _useVoxelShader
    if (_voxelsAsPoints && !_useVoxelShader) {
        _useVoxelShader = true;
    }
    
    if (_useVoxelShader) {
        GLuint* indicesArray = new GLuint[_maxVoxels];

        // populate the indicesArray
        // this will not change given new voxels, so we can set it all up now
        for (int n = 0; n < _maxVoxels; n++) {
            indicesArray[n] = n;
        }

        // bind the indices VBO to the actual indices array
        glGenBuffers(1, &_vboVoxelsIndicesID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vboVoxelsIndicesID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * _maxVoxels, indicesArray, GL_STATIC_DRAW);
        _memoryUsageVBO += sizeof(GLuint) * _maxVoxels;

        glGenBuffers(1, &_vboVoxelsID);
        glBindBuffer(GL_ARRAY_BUFFER, _vboVoxelsID);
        glBufferData(GL_ARRAY_BUFFER, _maxVoxels * sizeof(VoxelShaderVBOData), NULL, GL_DYNAMIC_DRAW);
        _memoryUsageVBO += _maxVoxels * sizeof(VoxelShaderVBOData);
    
        // delete the indices and normals arrays that are no longer needed
        delete[] indicesArray;

        // we will track individual dirty sections with these arrays of bools
        _writeVoxelDirtyArray = new bool[_maxVoxels];
        memset(_writeVoxelDirtyArray, false, _maxVoxels * sizeof(bool));
        _memoryUsageRAM += (_maxVoxels * sizeof(bool));

        _readVoxelDirtyArray = new bool[_maxVoxels];
        memset(_readVoxelDirtyArray, false, _maxVoxels * sizeof(bool));
        _memoryUsageRAM += (_maxVoxels * sizeof(bool));

        // prep the data structures for incoming voxel data
        _writeVoxelShaderData = new VoxelShaderVBOData[_maxVoxels];
        _memoryUsageRAM += (sizeof(VoxelShaderVBOData) * _maxVoxels);

        _readVoxelShaderData = new VoxelShaderVBOData[_maxVoxels];
        _memoryUsageRAM += (sizeof(VoxelShaderVBOData) * _maxVoxels);
    } else {

        // Global Normals mode uses a technique of not including normals on any voxel vertices, and instead
        // rendering the voxel faces in 6 passes that use a global call to glNormal3f()
        setupFaceIndices(_vboIndicesTop,    identityIndicesTop);
        setupFaceIndices(_vboIndicesBottom, identityIndicesBottom);
        setupFaceIndices(_vboIndicesLeft,   identityIndicesLeft);
        setupFaceIndices(_vboIndicesRight,  identityIndicesRight);
        setupFaceIndices(_vboIndicesFront,  identityIndicesFront);
        setupFaceIndices(_vboIndicesBack,   identityIndicesBack);
        
        // Depending on if we're using per vertex normals, we will need more or less vertex points per voxel
        int vertexPointsPerVoxel = GLOBAL_NORMALS_VERTEX_POINTS_PER_VOXEL;
        glGenBuffers(1, &_vboVerticesID);
        glBindBuffer(GL_ARRAY_BUFFER, _vboVerticesID);
        glBufferData(GL_ARRAY_BUFFER, vertexPointsPerVoxel * sizeof(GLfloat) * _maxVoxels, NULL, GL_DYNAMIC_DRAW);
        _memoryUsageVBO += vertexPointsPerVoxel * sizeof(GLfloat) * _maxVoxels;

        // VBO for colorsArray
        glGenBuffers(1, &_vboColorsID);
        glBindBuffer(GL_ARRAY_BUFFER, _vboColorsID);
        glBufferData(GL_ARRAY_BUFFER, vertexPointsPerVoxel * sizeof(GLubyte) * _maxVoxels, NULL, GL_DYNAMIC_DRAW);
        _memoryUsageVBO += vertexPointsPerVoxel * sizeof(GLubyte) * _maxVoxels;

        // we will track individual dirty sections with these arrays of bools
        _writeVoxelDirtyArray = new bool[_maxVoxels];
        memset(_writeVoxelDirtyArray, false, _maxVoxels * sizeof(bool));
        _memoryUsageRAM += (sizeof(bool) * _maxVoxels);

        _readVoxelDirtyArray = new bool[_maxVoxels];
        memset(_readVoxelDirtyArray, false, _maxVoxels * sizeof(bool));
        _memoryUsageRAM += (sizeof(bool) * _maxVoxels);

        // prep the data structures for incoming voxel data
        _writeVerticesArray = new GLfloat[vertexPointsPerVoxel * _maxVoxels];
        _memoryUsageRAM += (sizeof(GLfloat) * vertexPointsPerVoxel * _maxVoxels);
        _readVerticesArray = new GLfloat[vertexPointsPerVoxel * _maxVoxels];
        _memoryUsageRAM += (sizeof(GLfloat) * vertexPointsPerVoxel * _maxVoxels);

        _writeColorsArray = new GLubyte[vertexPointsPerVoxel * _maxVoxels];
        _memoryUsageRAM += (sizeof(GLubyte) * vertexPointsPerVoxel * _maxVoxels);
        _readColorsArray = new GLubyte[vertexPointsPerVoxel * _maxVoxels];
        _memoryUsageRAM += (sizeof(GLubyte) * vertexPointsPerVoxel * _maxVoxels);


        // create our simple fragment shader if we're the first system to init
        if (!_perlinModulateProgram.isLinked()) {
            switchToResourcesParentIfRequired();
            _perlinModulateProgram.addShaderFromSourceFile(QGLShader::Vertex, "resources/shaders/perlin_modulate.vert");
            _perlinModulateProgram.addShaderFromSourceFile(QGLShader::Fragment, "resources/shaders/perlin_modulate.frag");
            _perlinModulateProgram.link();

            _perlinModulateProgram.bind();
            _perlinModulateProgram.setUniformValue("permutationNormalTexture", 0);
            _perlinModulateProgram.release();
        }
    }

    _initialized = true;

    pthread_mutex_unlock(&_bufferWriteLock);
}

void VoxelSystem::loadVoxelsFile(const char* fileName, bool wantColorRandomizer) {
    _tree->loadVoxelsFile(fileName, wantColorRandomizer);
    setupNewVoxelsForDrawing();
}

void VoxelSystem::writeToSVOFile(const char* filename, VoxelNode* node) const {
    _tree->writeToSVOFile(filename, node);
}

bool VoxelSystem::readFromSVOFile(const char* filename) {
    bool result = _tree->readFromSVOFile(filename);
    if (result) {
        setupNewVoxelsForDrawing();
    }
    return result;
}

bool VoxelSystem::readFromSquareARGB32Pixels(const char *filename) {
    bool result = _tree->readFromSquareARGB32Pixels(filename);
    if (result) {
        setupNewVoxelsForDrawing();
    }
    return result;
}

bool VoxelSystem::readFromSchematicFile(const char* filename) {
    bool result = _tree->readFromSchematicFile(filename);
    if (result) {
        setupNewVoxelsForDrawing();
    }
    return result;
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
    int numBytesPacketHeader = numBytesForPacketHeader(sourceBuffer);
    unsigned char* voxelData = sourceBuffer + numBytesPacketHeader;

    switch(command) {
        case PACKET_TYPE_VOXEL_DATA: {
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                                    "readBitstreamToTree()");
            // ask the VoxelTree to read the bitstream into the tree
            ReadBitstreamToTreeParams args(WANT_COLOR, WANT_EXISTS_BITS, NULL, getDataSourceUUID());
            lockTree();
            _tree->readBitstreamToTree(voxelData, numBytes - numBytesPacketHeader, args);
            unlockTree();
        }
            break;
        case PACKET_TYPE_VOXEL_DATA_MONOCHROME: {
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                                    "readBitstreamToTree()");
            // ask the VoxelTree to read the MONOCHROME bitstream into the tree
            ReadBitstreamToTreeParams args(NO_COLOR, WANT_EXISTS_BITS, NULL, getDataSourceUUID());
            lockTree();
            _tree->readBitstreamToTree(voxelData, numBytes - numBytesPacketHeader, args);
            unlockTree();
        }
            break;
        case PACKET_TYPE_Z_COMMAND:

            // the Z command is a special command that allows the sender to send high level semantic
            // requests, like erase all, or add sphere scene, different receivers may handle these
            // messages differently
            char* packetData = (char *)sourceBuffer;
            char* command = &packetData[numBytesPacketHeader]; // start of the command
            int commandLength = strlen(command); // commands are null terminated strings
            int totalLength = 1+commandLength+1;

            qDebug("got Z message len(%d)= %s\n", numBytes, command);

            while (totalLength <= numBytes) {
                if (0==strcmp(command,(char*)"erase all")) {
                    qDebug("got Z message == erase all - NOT SUPPORTED ON INTERFACE\n");
                }
                if (0==strcmp(command,(char*)"add scene")) {
                    qDebug("got Z message == add scene - NOT SUPPORTED ON INTERFACE\n");
                }
                totalLength += commandLength+1;
            }
        break;
    }


    if (!_useFastVoxelPipeline || _writeRenderFullVBO) {
        setupNewVoxelsForDrawing();
    } else {
        checkForCulling();
        setupNewVoxelsForDrawingSingleNode(DONT_BAIL_EARLY);
    }
    
    Application::getInstance()->getBandwidthMeter()->inputStream(BandwidthMeter::VOXELS).updateValue(numBytes);
 
    return numBytes;
}

void VoxelSystem::setupNewVoxelsForDrawing() {
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                            "setupNewVoxelsForDrawing()");

    if (!_initialized) {
        return; // bail early if we're not initialized
    }

    uint64_t start = usecTimestampNow();
    uint64_t sinceLastTime = (start - _setupNewVoxelsForDrawingLastFinished) / 1000;
    
    bool iAmDebugging = false;  // if you're debugging set this to true, so you won't get skipped for slow debugging
    if (!iAmDebugging && sinceLastTime <= std::max((float) _setupNewVoxelsForDrawingLastElapsed, SIXTY_FPS_IN_MILLISECONDS)) {
        return; // bail early, it hasn't been long enough since the last time we ran
    }

    _inSetupNewVoxelsForDrawing = true;

    checkForCulling(); // check for out of view and deleted voxels...
    
    bool didWriteFullVBO = _writeRenderFullVBO;
    if (_tree->isDirty()) {
        static char buffer[64] = { 0 };
        if (Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings)) {
            sprintf(buffer, "newTreeToArrays() _writeRenderFullVBO=%s", debug::valueOf(_writeRenderFullVBO)); 
        };
        PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), buffer);
        _callsToTreesToArrays++;
        if (_writeRenderFullVBO) {
            clearFreeBufferIndexes();
        }
        _voxelsUpdated = newTreeToArrays(_tree->rootNode);
        _tree->clearDirtyBit(); // after we pull the trees into the array, we can consider the tree clean

        if (_writeRenderFullVBO) {
            _abandonedVBOSlots = 0; // reset the count of our abandoned slots, why is this here and not earlier????
        }
        
        // since we called treeToArrays, we can assume that our VBO is in sync, and so partial updates to the VBOs are
        // ok again, until/unless we call removeOutOfView() 
        _writeRenderFullVBO = false; 
    } else {
        _voxelsUpdated = 0;
    }
    
    // lock on the buffer write lock so we can't modify the data when the GPU is reading it
    pthread_mutex_lock(&_bufferWriteLock);
    
    if (_voxelsUpdated) {
        _voxelsDirty=true;
    }

    // copy the newly written data to the arrays designated for reading, only does something if _voxelsDirty && _voxelsUpdated
    copyWrittenDataToReadArrays(didWriteFullVBO);

    pthread_mutex_unlock(&_bufferWriteLock);

    uint64_t end = usecTimestampNow();
    int elapsedmsec = (end - start) / 1000;
    _setupNewVoxelsForDrawingLastFinished = end;
    _setupNewVoxelsForDrawingLastElapsed = elapsedmsec;
    _inSetupNewVoxelsForDrawing = false;
}

void VoxelSystem::setupNewVoxelsForDrawingSingleNode(bool allowBailEarly) {

    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                            "setupNewVoxelsForDrawingSingleNode() xxxxx");

    uint64_t start = usecTimestampNow();
    uint64_t sinceLastTime = (start - _setupNewVoxelsForDrawingLastFinished) / 1000;

    bool iAmDebugging = false;  // if you're debugging set this to true, so you won't get skipped for slow debugging
    if (allowBailEarly && !iAmDebugging && 
        sinceLastTime <= std::max((float) _setupNewVoxelsForDrawingLastElapsed, SIXTY_FPS_IN_MILLISECONDS)) {
        return; // bail early, it hasn't been long enough since the last time we ran
    }

    // lock on the buffer write lock so we can't modify the data when the GPU is reading it
    {
        PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                            "setupNewVoxelsForDrawingSingleNode()... pthread_mutex_lock(&_bufferWriteLock);");
        pthread_mutex_lock(&_bufferWriteLock);
    }

    _voxelsDirty = true; // if we got this far, then we can assume some voxels are dirty

    // copy the newly written data to the arrays designated for reading, only does something if _voxelsDirty && _voxelsUpdated
    copyWrittenDataToReadArrays(_writeRenderFullVBO);

    // after...
    _voxelsUpdated = 0;

    pthread_mutex_unlock(&_bufferWriteLock);

    uint64_t end = usecTimestampNow();
    int elapsedmsec = (end - start) / 1000;
    _setupNewVoxelsForDrawingLastFinished = end;
    _setupNewVoxelsForDrawingLastElapsed = elapsedmsec;
}

void VoxelSystem::checkForCulling() {

    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), "checkForCulling()");
    uint64_t start = usecTimestampNow();
    uint64_t sinceLastViewCulling = (start - _lastViewCulling) / 1000;
    
    bool constantCulling = !Menu::getInstance()->isOptionChecked(MenuOption::DisableConstantCulling);
    
    // If the view frustum is no longer changing, but has changed, since last time, then remove nodes that are out of view
    if (constantCulling || (
            (sinceLastViewCulling >= std::max((float) _lastViewCullingElapsed, VIEW_CULLING_RATE_IN_MILLISECONDS))
            && !isViewChanging()
        )
    ) {
        // When we call removeOutOfView() voxels, we don't actually remove the voxels from the VBOs, but we do remove
        // them from tree, this makes our tree caclulations faster, but doesn't require us to fully rebuild the VBOs (which
        // can be expensive).
        if (!Menu::getInstance()->isOptionChecked(MenuOption::DisableHideOutOfView)) {

            // track how long its been since we were last moving. If we have recently moved then only use delta frustums, if
            // it's been a long time since we last moved, then go ahead and do a full frustum cull.
            if (isViewChanging()) {
                _lastViewIsChanging = start;
            }
            uint64_t sinceLastMoving = (start - _lastViewIsChanging) / 1000;

            bool enoughTime = (sinceLastMoving >= std::max((float) _lastViewCullingElapsed, VIEW_CULLING_RATE_IN_MILLISECONDS));

            // These has changed events will occur before we stop. So we need to remember this for when we finally have stopped
            // moving long enough to be enoughTime            
            if (hasViewChanged()) {
                _hasRecentlyChanged = true;
            }
        
            // If we have recently changed, but it's been enough time since we last moved, then we will do a full frustum
            // hide/show culling pass
            bool forceFullFrustum = enoughTime && _hasRecentlyChanged;

            // in hide mode, we only track the full frustum culls, because we don't care about the partials.
            if (forceFullFrustum) {          
                _lastViewCulling = start;
                _hasRecentlyChanged = false;
            }
            
            hideOutOfView(forceFullFrustum);

            if (forceFullFrustum) {          
                uint64_t endViewCulling = usecTimestampNow();
                _lastViewCullingElapsed = (endViewCulling - start) / 1000;
            }

        } else if (Menu::getInstance()->isOptionChecked(MenuOption::RemoveOutOfView)) {
            _lastViewCulling = start;
            removeOutOfView();
            uint64_t endViewCulling = usecTimestampNow();
            _lastViewCullingElapsed = (endViewCulling - start) / 1000;
        }
        
        // Once we call cleanupRemovedVoxels() we do need to rebuild our VBOs (if anything was actually removed). So,
        // we should consider putting this someplace else... as this might be able to occur less frequently, and save us on
        // VBO reubuilding. Possibly we should do this only if our actual VBO usage crosses some lower boundary.
        cleanupRemovedVoxels();
    }

    uint64_t sinceLastAudit = (start - _lastAudit) / 1000;
    
    if (Menu::getInstance()->isOptionChecked(MenuOption::AutomaticallyAuditTree)) {
        if (sinceLastAudit >= std::max((float) _lastViewCullingElapsed, VIEW_CULLING_RATE_IN_MILLISECONDS)) {
            _lastAudit = start;
            collectStatsForTreesAndVBOs();
        }
    }
}

void VoxelSystem::cleanupRemovedVoxels() {
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), "cleanupRemovedVoxels()");
    // This handles cleanup of voxels that were culled as part of our regular out of view culling operation
    if (!_removedVoxels.isEmpty()) {
        if (Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings)) {
            qDebug() << "cleanupRemovedVoxels().. _removedVoxels=" << _removedVoxels.count() << "\n";
        }
        while (!_removedVoxels.isEmpty()) {
            delete _removedVoxels.extract();
        }
        _writeRenderFullVBO = true; // if we remove voxels, we must update our full VBOs
    }

    // we also might have VBO slots that have been abandoned, if too many of our VBO slots
    // are abandonded we want to rerender our full VBOs
    const float TOO_MANY_ABANDONED_RATIO = 0.5f;
    if (!_writeRenderFullVBO && (_abandonedVBOSlots > (_voxelsInWriteArrays * TOO_MANY_ABANDONED_RATIO))) {
        if (Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings)) {
            qDebug() << "cleanupRemovedVoxels().. _abandonedVBOSlots [" 
                << _abandonedVBOSlots << "] > TOO_MANY_ABANDONED_RATIO \n";
        }
        _writeRenderFullVBO = true;
    }
}

void VoxelSystem::copyWrittenDataToReadArraysFullVBOs() {
    copyWrittenDataSegmentToReadArrays(0, _voxelsInWriteArrays - 1);

    _voxelsInReadArrays = _voxelsInWriteArrays;
    
    // clear our dirty flags
    memset(_writeVoxelDirtyArray, false, _voxelsInWriteArrays * sizeof(bool));
    
    // let the reader know to get the full array
    _readRenderFullVBO = true;
}

void VoxelSystem::copyWrittenDataToReadArraysPartialVBOs() {
    glBufferIndex segmentStart = 0;
    bool inSegment = false;
    for (glBufferIndex i = 0; i < _voxelsInWriteArrays; i++) {
        bool thisVoxelDirty = _writeVoxelDirtyArray[i];
        _readVoxelDirtyArray[i] |= thisVoxelDirty;
        _writeVoxelDirtyArray[i] = false;
        if (!inSegment) {
            if (thisVoxelDirty) {
                segmentStart = i;
                inSegment = true;
            }
        } else {
            if (!thisVoxelDirty) {
                // If we got here because because this voxel is NOT dirty, so the last dirty voxel was the one before
                // this one and so that's where the "segment" ends
                copyWrittenDataSegmentToReadArrays(segmentStart, i - 1);
                inSegment = false;
            }
        }
    }
    
    // if we got to the end of the array, and we're in an active dirty segment...
    if (inSegment) {
        copyWrittenDataSegmentToReadArrays(segmentStart, _voxelsInWriteArrays - 1);
    }

    // update our length
    _voxelsInReadArrays = _voxelsInWriteArrays;
}

void VoxelSystem::copyWrittenDataSegmentToReadArrays(glBufferIndex segmentStart, glBufferIndex segmentEnd) {
    int segmentLength = (segmentEnd - segmentStart) + 1;
    if (_useVoxelShader) {
        GLsizeiptr segmentSizeBytes = segmentLength * sizeof(VoxelShaderVBOData);
        void* readDataAt = &_readVoxelShaderData[segmentStart];
        void* writeDataAt = &_writeVoxelShaderData[segmentStart];
        memcpy(readDataAt, writeDataAt, segmentSizeBytes);
    } else {
        // Depending on if we're using per vertex normals, we will need more or less vertex points per voxel
        int vertexPointsPerVoxel = GLOBAL_NORMALS_VERTEX_POINTS_PER_VOXEL;

        GLintptr   segmentStartAt   = segmentStart * vertexPointsPerVoxel * sizeof(GLfloat);
        GLsizeiptr segmentSizeBytes = segmentLength * vertexPointsPerVoxel * sizeof(GLfloat);
        GLfloat* readVerticesAt     = _readVerticesArray  + (segmentStart * vertexPointsPerVoxel);
        GLfloat* writeVerticesAt    = _writeVerticesArray + (segmentStart * vertexPointsPerVoxel);
        memcpy(readVerticesAt, writeVerticesAt, segmentSizeBytes);

        segmentStartAt          = segmentStart * vertexPointsPerVoxel * sizeof(GLubyte);
        segmentSizeBytes        = segmentLength * vertexPointsPerVoxel * sizeof(GLubyte);
        GLubyte* readColorsAt   = _readColorsArray   + (segmentStart * vertexPointsPerVoxel);
        GLubyte* writeColorsAt  = _writeColorsArray  + (segmentStart * vertexPointsPerVoxel);
        memcpy(readColorsAt, writeColorsAt, segmentSizeBytes);
    }
}

void VoxelSystem::copyWrittenDataToReadArrays(bool fullVBOs) {
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                            "copyWrittenDataToReadArrays()");

    if (_voxelsDirty && _voxelsUpdated) {
        if (fullVBOs) {
            copyWrittenDataToReadArraysFullVBOs();
        } else {
            copyWrittenDataToReadArraysPartialVBOs();
        }
    }
}

int VoxelSystem::newTreeToArrays(VoxelNode* node) {
    int   voxelsUpdated   = 0;
    bool  shouldRender    = false; // assume we don't need to render it
    // if it's colored, we might need to render it!
    float voxelSizeScale = Menu::getInstance()->getVoxelSizeScale();;
    int boundaryLevelAdjust = Menu::getInstance()->getBoundaryLevelAdjust();
    shouldRender = node->calculateShouldRender(_viewFrustum, voxelSizeScale, boundaryLevelAdjust);

    node->setShouldRender(shouldRender);
    // let children figure out their renderness
    if (!node->isLeaf()) {
    
        // As we check our children, see if any of them went from shouldRender to NOT shouldRender
        // then we probably dropped LOD and if we don't have color, we want to average our children 
        // for a new color.
        int childrenGotHiddenCount = 0;
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            VoxelNode* childNode = node->getChildAtIndex(i);
            if (childNode) {
                bool wasShouldRender = childNode->getShouldRender();
                voxelsUpdated += newTreeToArrays(childNode);
                bool isShouldRender = childNode->getShouldRender();
                if (wasShouldRender && !isShouldRender) {
                    childrenGotHiddenCount++;
                }
            }
        }
        if (childrenGotHiddenCount > 0) {
            node->setColorFromAverageOfChildren();
        }
    }
    if (_writeRenderFullVBO) {
        const bool DONT_REUSE_INDEX = false;
        const bool FORCE_REDRAW = true;
        voxelsUpdated += updateNodeInArrays(node, DONT_REUSE_INDEX, FORCE_REDRAW);
    } else {
        const bool REUSE_INDEX = true;
        const bool DONT_FORCE_REDRAW = false;
        voxelsUpdated += updateNodeInArrays(node, REUSE_INDEX, DONT_FORCE_REDRAW);
    }
    node->clearDirtyBit(); // clear the dirty bit, do this before we potentially delete things.
    
    return voxelsUpdated;
}

// called as response to voxelDeleted() in fast pipeline case. The node
// is being deleted, but it's state is such that it thinks it should render
// and therefore we can't use the normal render calculations. This method
// will forcibly remove it from the VBOs because we know better!!!
int VoxelSystem::forceRemoveNodeFromArrays(VoxelNode* node) {

    if (!_initialized) {
        return 0;
    }

    // if the node is not in the VBOs then we have nothing to do!
    if (node->isKnownBufferIndex()) {
        // If this node has not yet been written to the array, then add it to the end of the array.
        glBufferIndex nodeIndex = node->getBufferIndex();
        node->setBufferIndex(GLBUFFER_INDEX_UNKNOWN);
        freeBufferIndex(nodeIndex); // NOTE: This is make the node invisible!
        return 1; // updated!
    }
    return 0; // not-updated
}

int VoxelSystem::updateNodeInArrays(VoxelNode* node, bool reuseIndex, bool forceDraw) {
    // If we've run out of room, then just bail...
    if (_voxelsInWriteArrays >= _maxVoxels && (_freeIndexes.size() == 0)) {
        // We need to think about what else we can do in this case. This basically means that all of our available
        // VBO slots are used up, but we're trying to render more voxels. At this point, if this happens we'll just
        // not render these Voxels. We need to think about ways to keep the entire scene intact but maybe lower quality
        // possibly shifting down to lower LOD or something. This debug message is to help identify, if/when/how this
        // state actually occurs.
        if (Menu::getInstance()->isOptionChecked(MenuOption::ExtraDebugging)) {
            qDebug("OHHHH NOOOOOO!!!! updateNodeInArrays() BAILING (_voxelsInWriteArrays >= _maxVoxels)\n");
        }
        return 0;
    }

    if (!_initialized) {
        return 0;
    }
    
    // If we've changed any attributes (our renderness, our color, etc), or we've been told to force a redraw
    // then update the Arrays...
    if (forceDraw || node->isDirty()) {
        // If we're should render, use our legit location and scale, 
        if (node->getShouldRender()) {
            glm::vec3 startVertex = node->getCorner();
            float voxelScale = node->getScale();

            glBufferIndex nodeIndex = GLBUFFER_INDEX_UNKNOWN;
            if (reuseIndex && node->isKnownBufferIndex()) {
                nodeIndex = node->getBufferIndex();
            } else {
                nodeIndex = getNextBufferIndex();
                node->setBufferIndex(nodeIndex);
                node->setVoxelSystem(this);
            }
            // populate the array with points for the 8 vertices and RGB color for each added vertex
            updateArraysDetails(nodeIndex, startVertex, voxelScale, node->getColor());
            return 1; // updated!
        } else {
            // If we shouldn't render, and we're in reuseIndex mode, then free our index, this only operates
            // on nodes with known index values, so it's safe to call for any node.
            if (reuseIndex) {
                return forceRemoveNodeFromArrays(node);
            }
        }
    }
    return 0; // not-updated
}

void VoxelSystem::updateArraysDetails(glBufferIndex nodeIndex, const glm::vec3& startVertex,
                                     float voxelScale, const nodeColor& color) {

    if (_initialized) {
        _writeVoxelDirtyArray[nodeIndex] = true;
                                     
        if (_useVoxelShader) {
            if (_writeVoxelShaderData) {
                VoxelShaderVBOData* writeVerticesAt = &_writeVoxelShaderData[nodeIndex];
                writeVerticesAt->x = startVertex.x * TREE_SCALE;
                writeVerticesAt->y = startVertex.y * TREE_SCALE;
                writeVerticesAt->z = startVertex.z * TREE_SCALE;
                writeVerticesAt->s = voxelScale * TREE_SCALE;
                writeVerticesAt->r = color[RED_INDEX];
                writeVerticesAt->g = color[GREEN_INDEX];
                writeVerticesAt->b = color[BLUE_INDEX];
            }
        } else {
            if (_writeVerticesArray && _writeColorsArray) {
                int vertexPointsPerVoxel = GLOBAL_NORMALS_VERTEX_POINTS_PER_VOXEL;
                for (int j = 0; j < vertexPointsPerVoxel; j++ ) {
                    GLfloat* writeVerticesAt = _writeVerticesArray + (nodeIndex * vertexPointsPerVoxel);
                    GLubyte* writeColorsAt   = _writeColorsArray   + (nodeIndex * vertexPointsPerVoxel);
                    *(writeVerticesAt+j) = startVertex[j % 3] + (identityVerticesGlobalNormals[j] * voxelScale);
                    *(writeColorsAt  +j) = color[j % 3];
                }
            }
        }
    }
}

glm::vec3 VoxelSystem::computeVoxelVertex(const glm::vec3& startVertex, float voxelScale, int index) const {
    const float* identityVertex = identityVertices + index * 3;
    return startVertex + glm::vec3(identityVertex[0], identityVertex[1], identityVertex[2]) * voxelScale;
}

ProgramObject VoxelSystem::_perlinModulateProgram;

void VoxelSystem::init() {
    if (_initialized) {
        qDebug("[ERROR] VoxelSystem is already initialized.\n");
        return;
    }

    _callsToTreesToArrays = 0;
    _setupNewVoxelsForDrawingLastFinished = 0;
    _setupNewVoxelsForDrawingLastElapsed = 0;
    _lastViewCullingElapsed = _lastViewCulling = _lastAudit = _lastViewIsChanging = 0;
    _hasRecentlyChanged = false;

    _voxelsDirty = false;
    _voxelsInWriteArrays = 0;
    _voxelsInReadArrays = 0;

    // VBO for the verticesArray
    _initialMemoryUsageGPU = getFreeMemoryGPU();
    initVoxelMemory();
    
    // our own _removedVoxels doesn't need to be notified of voxel deletes
    VoxelNode::removeDeleteHook(&_removedVoxels);
}

void VoxelSystem::changeTree(VoxelTree* newTree) {
    disconnect(_tree, 0, this, 0);

    _tree = newTree;
    _tree->setDirtyBit();
    _tree->rootNode->setVoxelSystem(this);

    connect(_tree, SIGNAL(importSize(float,float,float)), SIGNAL(importSize(float,float,float)));
    connect(_tree, SIGNAL(importProgress(int)), SIGNAL(importProgress(int)));

    setupNewVoxelsForDrawing();
}

void VoxelSystem::updateFullVBOs() {
    bool outputWarning = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(outputWarning, "updateFullVBOs()");

    {
        static char buffer[128] = { 0 };
        if (outputWarning) {
            sprintf(buffer, "updateFullVBOs() : updateVBOSegment(0, _voxelsInReadArrays=%lu);", _voxelsInReadArrays);
        };

        PerformanceWarning warn(outputWarning,buffer);
        updateVBOSegment(0, _voxelsInReadArrays);
    }
    
    {
        PerformanceWarning warn(outputWarning,"updateFullVBOs() : memset(_readVoxelDirtyArray...)");
        // consider the _readVoxelDirtyArray[] clean!
        memset(_readVoxelDirtyArray, false, _voxelsInReadArrays * sizeof(bool));
    }
}

void VoxelSystem::updatePartialVBOs() {
    glBufferIndex segmentStart = 0;
    bool inSegment = false;
    for (glBufferIndex i = 0; i < _voxelsInReadArrays; i++) {
        bool thisVoxelDirty = _readVoxelDirtyArray[i];
        if (!inSegment) {
            if (thisVoxelDirty) {
                segmentStart = i;
                inSegment = true;
                _readVoxelDirtyArray[i] = false; // consider us clean!
            }
        } else {
            if (!thisVoxelDirty) {
                // If we got here because because this voxel is NOT dirty, so the last dirty voxel was the one before
                // this one and so that's where the "segment" ends
                updateVBOSegment(segmentStart, i - 1);
                inSegment = false;
            }
            _readVoxelDirtyArray[i] = false; // consider us clean!
        }
    }
    
    // if we got to the end of the array, and we're in an active dirty segment...
    if (inSegment) {
        updateVBOSegment(segmentStart, _voxelsInReadArrays - 1);
        inSegment = false;
    }
}

void VoxelSystem::updateVBOs() {
    static char buffer[40] = { 0 };
    if (Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings)) {
        sprintf(buffer, "updateVBOs() _readRenderFullVBO=%s", debug::valueOf(_readRenderFullVBO));
    };
    // would like to include _callsToTreesToArrays
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), buffer);
    if (_voxelsDirty) {
        if (_readRenderFullVBO) {
            updateFullVBOs();
        } else {
            updatePartialVBOs();
        }
        _voxelsDirty = false;
        _readRenderFullVBO = false;
    }
    _callsToTreesToArrays = 0; // clear it
}

void VoxelSystem::updateVBOSegment(glBufferIndex segmentStart, glBufferIndex segmentEnd) {
    bool showWarning = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarning, "updateVBOSegment()");

    if (_useVoxelShader) {
        int segmentLength = (segmentEnd - segmentStart) + 1;
        GLintptr segmentStartAt = segmentStart * sizeof(VoxelShaderVBOData);
        GLsizeiptr segmentSizeBytes = segmentLength * sizeof(VoxelShaderVBOData);
        void* readVerticesFrom = &_readVoxelShaderData[segmentStart];

        glBindBuffer(GL_ARRAY_BUFFER, _vboVoxelsID);
        glBufferSubData(GL_ARRAY_BUFFER, segmentStartAt, segmentSizeBytes, readVerticesFrom);
    } else {
        int vertexPointsPerVoxel = GLOBAL_NORMALS_VERTEX_POINTS_PER_VOXEL;
        int segmentLength = (segmentEnd - segmentStart) + 1;
        GLintptr   segmentStartAt   = segmentStart * vertexPointsPerVoxel * sizeof(GLfloat);
        GLsizeiptr segmentSizeBytes = segmentLength * vertexPointsPerVoxel * sizeof(GLfloat);
        GLfloat* readVerticesFrom   = _readVerticesArray + (segmentStart * vertexPointsPerVoxel);

        {
            PerformanceWarning warn(showWarning, "updateVBOSegment() : glBindBuffer(GL_ARRAY_BUFFER, _vboVerticesID);");
            glBindBuffer(GL_ARRAY_BUFFER, _vboVerticesID);
        }

        {
            PerformanceWarning warn(showWarning, "updateVBOSegment() : glBufferSubData() _vboVerticesID);");
            glBufferSubData(GL_ARRAY_BUFFER, segmentStartAt, segmentSizeBytes, readVerticesFrom);
        }

        segmentStartAt          = segmentStart * vertexPointsPerVoxel * sizeof(GLubyte);
        segmentSizeBytes        = segmentLength * vertexPointsPerVoxel * sizeof(GLubyte);
        GLubyte* readColorsFrom = _readColorsArray   + (segmentStart * vertexPointsPerVoxel);

        {
            PerformanceWarning warn(showWarning, "updateVBOSegment() : glBindBuffer(GL_ARRAY_BUFFER, _vboColorsID);");
            glBindBuffer(GL_ARRAY_BUFFER, _vboColorsID);
        }
        
        {
            PerformanceWarning warn(showWarning, "updateVBOSegment() : glBufferSubData() _vboColorsID);");
            glBufferSubData(GL_ARRAY_BUFFER, segmentStartAt, segmentSizeBytes, readColorsFrom);
        }
    }
}

void VoxelSystem::render(bool texture) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "render()");
    
    // If we got here and we're not initialized then bail!
    if (!_initialized) {
        return;
    }
    
    updateVBOs();
    
    bool dontCallOpenGLDraw = Menu::getInstance()->isOptionChecked(MenuOption::DontCallOpenGLForVoxels);
    // if not don't... then do...
    if (_useVoxelShader) {
        PerformanceWarning warn(showWarnings,"render().. _useVoxelShader openGL..");


        //Define this somewhere in your header file
        #define BUFFER_OFFSET(i) ((void*)(i))

        glBindBuffer(GL_ARRAY_BUFFER, _vboVoxelsID);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, sizeof(VoxelShaderVBOData), BUFFER_OFFSET(0));   //The starting point of the VBO, for the vertices

        int attributeLocation;

        if (!_voxelsAsPoints) {
            Application::getInstance()->getVoxelShader().begin();
            attributeLocation = Application::getInstance()->getVoxelShader().attributeLocation("voxelSizeIn");
            glEnableVertexAttribArray(attributeLocation);
            glVertexAttribPointer(attributeLocation, 1, GL_FLOAT, false, sizeof(VoxelShaderVBOData), BUFFER_OFFSET(3*sizeof(float)));
        } else {
            glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

            glm::vec2 viewDimensions = Application::getInstance()->getViewportDimensions();
            float viewportWidth = viewDimensions.x;
            float viewportHeight = viewDimensions.y;
            glm::vec3 cameraPosition = Application::getInstance()->getViewFrustum()->getPosition();
            PointShader& pointShader = Application::getInstance()->getPointShader();

            pointShader.begin();

            pointShader.setUniformValue(pointShader.uniformLocation("viewportWidth"), viewportWidth);
            pointShader.setUniformValue(pointShader.uniformLocation("viewportHeight"), viewportHeight);
            pointShader.setUniformValue(pointShader.uniformLocation("cameraPosition"), cameraPosition);

            attributeLocation = pointShader.attributeLocation("voxelSizeIn");
            glEnableVertexAttribArray(attributeLocation);
            glVertexAttribPointer(attributeLocation, 1, GL_FLOAT, false, sizeof(VoxelShaderVBOData), BUFFER_OFFSET(3*sizeof(float)));
        }
        
        
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(3, GL_UNSIGNED_BYTE, sizeof(VoxelShaderVBOData), BUFFER_OFFSET(4*sizeof(float)));//The starting point of colors

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vboVoxelsIndicesID);

        if (!dontCallOpenGLDraw) {
            glDrawElements(GL_POINTS, _voxelsInReadArrays, GL_UNSIGNED_INT, BUFFER_OFFSET(0)); //The starting point of the IBO
        }

        // deactivate vertex and color arrays after drawing
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);

        // bind with 0 to switch back to normal operation
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        if (!_voxelsAsPoints) {
            Application::getInstance()->getVoxelShader().end();
            glDisableVertexAttribArray(attributeLocation);
        } else {
            Application::getInstance()->getPointShader().end();
            glDisableVertexAttribArray(attributeLocation);
            glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
        }
    } else {
        PerformanceWarning warn(showWarnings, "render().. TRIANGLES...");

        {
            PerformanceWarning warn(showWarnings,"render().. setup before glDrawRangeElementsEXT()...");
                
            // tell OpenGL where to find vertex and color information
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_COLOR_ARRAY);

            glBindBuffer(GL_ARRAY_BUFFER, _vboVerticesID);
            glVertexPointer(3, GL_FLOAT, 0, 0);

            glBindBuffer(GL_ARRAY_BUFFER, _vboColorsID);
            glColorPointer(3, GL_UNSIGNED_BYTE, 0, 0);

            applyScaleAndBindProgram(texture);

            // for performance, enable backface culling
            glEnable(GL_CULL_FACE);
        }

        // draw voxels in 6 passes

        if (!dontCallOpenGLDraw) {
            PerformanceWarning warn(showWarnings, "render().. glDrawRangeElementsEXT()...");
                
            glNormal3f(0,1.0f,0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vboIndicesTop);
            glDrawRangeElementsEXT(GL_TRIANGLES, 0, GLOBAL_NORMALS_VERTICES_PER_VOXEL * _voxelsInReadArrays - 1,
                INDICES_PER_FACE * _voxelsInReadArrays, GL_UNSIGNED_INT, 0);

            glNormal3f(0,-1.0f,0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vboIndicesBottom);
            glDrawRangeElementsEXT(GL_TRIANGLES, 0, GLOBAL_NORMALS_VERTICES_PER_VOXEL * _voxelsInReadArrays - 1,
                INDICES_PER_FACE * _voxelsInReadArrays, GL_UNSIGNED_INT, 0);

            glNormal3f(-1.0f,0,0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vboIndicesLeft);
            glDrawRangeElementsEXT(GL_TRIANGLES, 0, GLOBAL_NORMALS_VERTICES_PER_VOXEL * _voxelsInReadArrays - 1,
                INDICES_PER_FACE * _voxelsInReadArrays, GL_UNSIGNED_INT, 0);

            glNormal3f(1.0f,0,0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vboIndicesRight);
            glDrawRangeElementsEXT(GL_TRIANGLES, 0, GLOBAL_NORMALS_VERTICES_PER_VOXEL * _voxelsInReadArrays - 1,
                INDICES_PER_FACE * _voxelsInReadArrays, GL_UNSIGNED_INT, 0);

            glNormal3f(0,0,-1.0f);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vboIndicesFront);
            glDrawRangeElementsEXT(GL_TRIANGLES, 0, GLOBAL_NORMALS_VERTICES_PER_VOXEL * _voxelsInReadArrays - 1,
                INDICES_PER_FACE * _voxelsInReadArrays, GL_UNSIGNED_INT, 0);

            glNormal3f(0,0,1.0f);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vboIndicesBack);
            glDrawRangeElementsEXT(GL_TRIANGLES, 0, GLOBAL_NORMALS_VERTICES_PER_VOXEL * _voxelsInReadArrays - 1,
                INDICES_PER_FACE * _voxelsInReadArrays, GL_UNSIGNED_INT, 0);
        }

        {
            PerformanceWarning warn(showWarnings, "render().. cleanup after glDrawRangeElementsEXT()...");
        
            glDisable(GL_CULL_FACE);

            removeScaleAndReleaseProgram(texture);

            // deactivate vertex and color arrays after drawing
            glDisableClientState(GL_VERTEX_ARRAY);
            glDisableClientState(GL_COLOR_ARRAY);

            // bind with 0 to switch back to normal operation
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }
    }
}

void VoxelSystem::applyScaleAndBindProgram(bool texture) {
    glPushMatrix();
    glScalef(_treeScale, _treeScale, _treeScale);

    if (texture) {
        _perlinModulateProgram.bind();
        glBindTexture(GL_TEXTURE_2D, Application::getInstance()->getTextureCache()->getPermutationNormalTextureID());
    }
}

void VoxelSystem::removeScaleAndReleaseProgram(bool texture) {
    // scale back down to 1 so heads aren't massive
    glPopMatrix();
    
    if (texture) {
        _perlinModulateProgram.release();
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

int VoxelSystem::_nodeCount = 0;

void VoxelSystem::killLocalVoxels() {
    lockTree();
    _tree->eraseAllVoxels();
    unlockTree();
    clearFreeBufferIndexes();    
    _voxelsInReadArrays = 0; // do we need to do this?
    setupNewVoxelsForDrawing();
}

void VoxelSystem::redrawInViewVoxels() {
    hideOutOfView(true);
}


bool VoxelSystem::clearAllNodesBufferIndexOperation(VoxelNode* node, void* extraData) {
    _nodeCount++;
    node->setBufferIndex(GLBUFFER_INDEX_UNKNOWN);
    return true;
}

void VoxelSystem::clearAllNodesBufferIndex() {
    _nodeCount = 0;
    lockTree();                                  
    _tree->recurseTreeWithOperation(clearAllNodesBufferIndexOperation);
    unlockTree();
    if (Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings)) {
        qDebug("clearing buffer index of %d nodes\n", _nodeCount);
    }
}

bool VoxelSystem::forceRedrawEntireTreeOperation(VoxelNode* node, void* extraData) {
    _nodeCount++;
    node->setDirtyBit();
    return true;
}

void VoxelSystem::forceRedrawEntireTree() {
    _nodeCount = 0;
    _tree->recurseTreeWithOperation(forceRedrawEntireTreeOperation);
    qDebug("forcing redraw of %d nodes\n", _nodeCount);
    _tree->setDirtyBit();
    setupNewVoxelsForDrawing();
}

bool VoxelSystem::randomColorOperation(VoxelNode* node, void* extraData) {
    _nodeCount++;
    if (node->isColored()) {
        nodeColor newColor = { 255, randomColorValue(150), randomColorValue(150), 1 };
        node->setColor(newColor);
    }
    return true;
}

void VoxelSystem::randomizeVoxelColors() {
    _nodeCount = 0;
    _tree->recurseTreeWithOperation(randomColorOperation);
    qDebug("setting randomized true color for %d nodes\n", _nodeCount);
    _tree->setDirtyBit();
    setupNewVoxelsForDrawing();
}

bool VoxelSystem::falseColorizeRandomOperation(VoxelNode* node, void* extraData) {
    _nodeCount++;
    // always false colorize
    node->setFalseColor(255, randomColorValue(150), randomColorValue(150));
    return true; // keep going!
}

void VoxelSystem::falseColorizeRandom() {
    _nodeCount = 0;
    _tree->recurseTreeWithOperation(falseColorizeRandomOperation);
    qDebug("setting randomized false color for %d nodes\n", _nodeCount);
    _tree->setDirtyBit();
    setupNewVoxelsForDrawing();
}

bool VoxelSystem::trueColorizeOperation(VoxelNode* node, void* extraData) {
    _nodeCount++;
    node->setFalseColored(false);
    return true;
}

void VoxelSystem::trueColorize() {
    PerformanceWarning warn(true, "trueColorize()",true);
    _nodeCount = 0;
    _tree->recurseTreeWithOperation(trueColorizeOperation);
    qDebug("setting true color for %d nodes\n", _nodeCount);
    _tree->setDirtyBit();
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

void VoxelSystem::falseColorizeInView() {
    _nodeCount = 0;
    _tree->recurseTreeWithOperation(falseColorizeInViewOperation,(void*)_viewFrustum);
    qDebug("setting in view false color for %d nodes\n", _nodeCount);
    _tree->setDirtyBit();
    setupNewVoxelsForDrawing();
}

// helper classes and args for falseColorizeBySource
class groupColor {
public:
    unsigned char red, green, blue;
    groupColor(unsigned char red, unsigned char green, unsigned char blue) :
        red(red), green(green), blue(blue) { }

    groupColor() :
        red(0), green(0), blue(0) { }
};

class colorizeBySourceArgs {
public:
    std::map<uint16_t, groupColor> colors;
};

// Will false colorize voxels that are not in view
bool VoxelSystem::falseColorizeBySourceOperation(VoxelNode* node, void* extraData) {
    colorizeBySourceArgs* args = (colorizeBySourceArgs*)extraData;
    _nodeCount++;
    if (node->isColored()) {
        // pick a color based on the source - we want each source to be obviously different
        uint16_t nodeIDKey = node->getSourceUUIDKey();
        node->setFalseColor(args->colors[nodeIDKey].red, args->colors[nodeIDKey].green,  args->colors[nodeIDKey].blue);
    }
    return true; // keep going!
}

void VoxelSystem::falseColorizeBySource() {
    _nodeCount = 0;
    colorizeBySourceArgs args;
    const int NUMBER_OF_COLOR_GROUPS = 6;
    const unsigned char MIN_COLOR = 128;
    int voxelServerCount = 0;
    groupColor groupColors[NUMBER_OF_COLOR_GROUPS] = { 
        groupColor(255,   0,   0), 
        groupColor(  0, 255,   0), 
        groupColor(  0,   0, 255),
        groupColor(255,   0, 255),
        groupColor(  0, 255, 255),
        groupColor(255, 255, 255)
    };

    // create a bunch of colors we'll use during colorization
    NodeList* nodeList = NodeList::getInstance();
    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
        if (node->getType() == NODE_TYPE_VOXEL_SERVER) {
            uint16_t nodeID = VoxelNode::getSourceNodeUUIDKey(node->getUUID());
            int groupColor = voxelServerCount % NUMBER_OF_COLOR_GROUPS;
            args.colors[nodeID] = groupColors[groupColor];

            if (groupColors[groupColor].red > 0) {
                groupColors[groupColor].red = ((groupColors[groupColor].red - MIN_COLOR)/2) + MIN_COLOR;
            }
            if (groupColors[groupColor].green > 0) {
                groupColors[groupColor].green = ((groupColors[groupColor].green - MIN_COLOR)/2) + MIN_COLOR;
            }
            if (groupColors[groupColor].blue > 0) {
                groupColors[groupColor].blue = ((groupColors[groupColor].blue - MIN_COLOR)/2) + MIN_COLOR;
            }

            voxelServerCount++;
        }
    }
    
    _tree->recurseTreeWithOperation(falseColorizeBySourceOperation, &args);
    qDebug("setting false color by source for %d nodes\n", _nodeCount);
    _tree->setDirtyBit();
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

void VoxelSystem::falseColorizeDistanceFromView() {
    _nodeCount = 0;
    _maxDistance = 0.0;
    _minDistance = FLT_MAX;
    _tree->recurseTreeWithOperation(getDistanceFromViewRangeOperation, (void*) _viewFrustum);
    qDebug("determining distance range for %d nodes\n", _nodeCount);
    _nodeCount = 0;
    _tree->recurseTreeWithOperation(falseColorizeDistanceFromViewOperation, (void*) _viewFrustum);
    qDebug("setting in distance false color for %d nodes\n", _nodeCount);
    _tree->setDirtyBit();
    setupNewVoxelsForDrawing();
}

// combines the removeOutOfView args into a single class
class removeOutOfViewArgs {
public:
    VoxelSystem*    thisVoxelSystem;
    ViewFrustum     thisViewFrustum;
    VoxelNodeBag    dontRecurseBag;
    unsigned long nodesScanned;
    unsigned long nodesRemoved;
    unsigned long nodesInside;
    unsigned long nodesIntersect;
    unsigned long nodesOutside;
    VoxelNode*      insideRoot;
    VoxelNode*      outsideRoot;
    
    removeOutOfViewArgs(VoxelSystem* voxelSystem, bool widenViewFrustum = true) :
        thisVoxelSystem(voxelSystem),
        thisViewFrustum(*voxelSystem->getViewFrustum()),
        dontRecurseBag(),
        nodesScanned(0),
        nodesRemoved(0),
        nodesInside(0),
        nodesIntersect(0),
        nodesOutside(0),
        insideRoot(NULL),
        outsideRoot(NULL)
    {
        // Widen the FOV for trimming
        if (widenViewFrustum) {
            float originalFOV = thisViewFrustum.getFieldOfView();
            float wideFOV = originalFOV + VIEW_FRUSTUM_FOV_OVERSEND;
            thisViewFrustum.setFieldOfView(wideFOV);
            thisViewFrustum.calculate();
        }
    }
};

void VoxelSystem::cancelImport() {
    _tree->cancelImport();
}

// "Remove" voxels from the tree that are not in view. We don't actually delete them,
// we remove them from the tree and place them into a holding area for later deletion
bool VoxelSystem::removeOutOfViewOperation(VoxelNode* node, void* extraData) {
    removeOutOfViewArgs* args = (removeOutOfViewArgs*)extraData;

    // If our node was previously added to the don't recurse bag, then return false to
    // stop the further recursion. This means that the whole node and it's children are
    // known to be in view, so don't recurse them
    if (args->dontRecurseBag.contains(node)) {
        args->dontRecurseBag.remove(node);
        return false; // stop recursion
    }
    
    VoxelSystem* thisVoxelSystem = args->thisVoxelSystem;
    args->nodesScanned++;
    // Need to operate on our child nodes, so we can remove them
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        VoxelNode* childNode = node->getChildAtIndex(i);
        if (childNode) {
            ViewFrustum::location inFrustum = childNode->inFrustum(args->thisViewFrustum);
            switch (inFrustum) {
                case ViewFrustum::OUTSIDE: {
                    args->nodesOutside++;
                    args->nodesRemoved++;
                    node->removeChildAtIndex(i);
                    thisVoxelSystem->_removedVoxels.insert(childNode);
                    // by removing the child, it will not get recursed!
                } break;
                case ViewFrustum::INSIDE: {
                    // if the child node is fully INSIDE the view, then there's no need to recurse it
                    // because we know all it's children will also be in the view, so we want to 
                    // tell the caller to NOT recurse this child
                    args->nodesInside++;
                    args->dontRecurseBag.insert(childNode);
                } break;
                case ViewFrustum::INTERSECT: {
                    // if the child node INTERSECTs the view, then we don't want to remove it because
                    // it is at least partially in view. But we DO want to recurse the children because
                    // some of them may not be in view... nothing specifically to do, just keep iterating
                    // the children
                    args->nodesIntersect++;
                } break;
            }
        }
    }
    return true; // keep going!
}

bool VoxelSystem::isViewChanging() {
    bool result = false; // assume the best

    // If our viewFrustum has changed since our _lastKnownViewFrustum
    if (!_lastKnownViewFrustum.isVerySimilar(_viewFrustum)) {
        result = true;
        _lastKnownViewFrustum = *_viewFrustum; // save last known
    }
    return result;
}

bool VoxelSystem::hasViewChanged() {
    bool result = false; // assume the best
    
    // If we're still changing, report no change yet.
    if (isViewChanging()) {
        return false;
    }
    
    // If our viewFrustum has changed since our _lastKnownViewFrustum
    if (!_lastStableViewFrustum.isVerySimilar(_viewFrustum)) {
        result = true;
        _lastStableViewFrustum = *_viewFrustum; // save last stable
    }
    return result;
}

void VoxelSystem::removeOutOfView() {
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), "removeOutOfView()");
    removeOutOfViewArgs args(this);
    _tree->recurseTreeWithOperation(removeOutOfViewOperation,(void*)&args);

    if (args.nodesRemoved) {
        _tree->setDirtyBit();
    }
    bool showRemoveDebugDetails = false;
    if (showRemoveDebugDetails) {
        qDebug("removeOutOfView() scanned=%ld removed=%ld inside=%ld intersect=%ld outside=%ld _removedVoxels.count()=%d \n", 
                args.nodesScanned, args.nodesRemoved, args.nodesInside, 
                args.nodesIntersect, args.nodesOutside, _removedVoxels.count()
            );
    }
}

// combines the removeOutOfView args into a single class
class showAllLocalVoxelsArgs {
public:
    VoxelSystem* thisVoxelSystem;
    ViewFrustum thisViewFrustum;
    unsigned long nodesScanned;
    
    showAllLocalVoxelsArgs(VoxelSystem* voxelSystem) :
        thisVoxelSystem(voxelSystem), 
        thisViewFrustum(*voxelSystem->getViewFrustum()),
        nodesScanned(0)
    {
    }
};

void VoxelSystem::showAllLocalVoxels() {
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), "showAllLocalVoxels()");
    showAllLocalVoxelsArgs args(this);
    _tree->recurseTreeWithOperation(showAllLocalVoxelsOperation,(void*)&args);

    bool showRemoveDebugDetails = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    if (showRemoveDebugDetails) {
        qDebug("showAllLocalVoxels() scanned=%ld \n",args.nodesScanned );
    }
}

bool VoxelSystem::showAllLocalVoxelsOperation(VoxelNode* node, void* extraData) {
    showAllLocalVoxelsArgs* args = (showAllLocalVoxelsArgs*)extraData;

    args->nodesScanned++;

    float voxelSizeScale = Menu::getInstance()->getVoxelSizeScale();;
    int boundaryLevelAdjust = Menu::getInstance()->getBoundaryLevelAdjust();
    bool shouldRender = node->calculateShouldRender(&args->thisViewFrustum, voxelSizeScale, boundaryLevelAdjust);
    node->setShouldRender(shouldRender);

    if (shouldRender) {
        bool falseColorize = false;
        if (falseColorize) {
            node->setFalseColor(0,0,255); // false colorize
        }
        // These are both needed to force redraw...
        node->setDirtyBit();
        node->markWithChangedTime();
    }

    return true; // keep recursing!
}


// combines the removeOutOfView args into a single class
class hideOutOfViewArgs {
public:
    VoxelSystem* thisVoxelSystem;
    VoxelTree* tree;
    ViewFrustum thisViewFrustum;
    ViewFrustum lastViewFrustum;
    bool culledOnce;
    bool wantDeltaFrustums;
    unsigned long nodesScanned;
    unsigned long nodesRemoved;
    unsigned long nodesInside;
    unsigned long nodesIntersect;
    unsigned long nodesOutside;
    unsigned long nodesInsideInside;
    unsigned long nodesIntersectInside;
    unsigned long nodesOutsideInside;
    unsigned long nodesInsideOutside;
    unsigned long nodesOutsideOutside;
    
    hideOutOfViewArgs(VoxelSystem* voxelSystem, VoxelTree* tree, 
                        bool culledOnce, bool widenViewFrustum, bool wantDeltaFrustums) :
        thisVoxelSystem(voxelSystem),
        tree(tree),
        thisViewFrustum(*voxelSystem->getViewFrustum()),
        lastViewFrustum(*voxelSystem->getLastCulledViewFrustum()),
        culledOnce(culledOnce),
        wantDeltaFrustums(wantDeltaFrustums),
        nodesScanned(0),
        nodesRemoved(0),
        nodesInside(0),
        nodesIntersect(0),
        nodesOutside(0),
        nodesInsideInside(0),
        nodesIntersectInside(0),
        nodesOutsideInside(0),
        nodesInsideOutside(0),
        nodesOutsideOutside(0)
    {
        // Widen the FOV for trimming
        if (widenViewFrustum) {
            float originalFOV = thisViewFrustum.getFieldOfView();
            float wideFOV = originalFOV + VIEW_FRUSTUM_FOV_OVERSEND;
            thisViewFrustum.setFieldOfView(wideFOV);
            thisViewFrustum.calculate();
        }
    }
};

void VoxelSystem::hideOutOfView(bool forceFullFrustum) {

    // don't re-enter...
    if (_inhideOutOfView) {
        return;
    }
    
    _inhideOutOfView = true;

    bool showDebugDetails = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showDebugDetails, "hideOutOfView()");
    bool widenFrustum = true;

    // When using "delta" view frustums and only hide/show items that are in the difference
    // between the two view frustums. There are some potential problems with this mode.
    //
    // 1) This work well for rotating, but what about moving forward?
    //    In the move forward case, you'll get new voxel details, but those
    //    new voxels will be in the last view.
    //
    // 2) Also, voxels will arrive from the network that are OUTSIDE of the view
    //    frustum... these won't get hidden... and so we can't assume they are correctly
    //    hidden... 
    //
    // Both these problems are solved by intermittently calling this with forceFullFrustum set
    // to true. This will essentially clean up the improperly hidden or shown voxels.
    //
    bool wantDeltaFrustums = !forceFullFrustum && !Menu::getInstance()->isOptionChecked(MenuOption::UseFullFrustumInHide);
    hideOutOfViewArgs args(this, this->_tree, _culledOnce, widenFrustum, wantDeltaFrustums);

    const bool wantViewFrustumDebugging = false; // change to true for additional debugging
    if (wantViewFrustumDebugging) {
        args.thisViewFrustum.printDebugDetails();
        if (_culledOnce) {
            args.lastViewFrustum.printDebugDetails();
        }
    }
    
    if (!forceFullFrustum && _culledOnce && args.lastViewFrustum.isVerySimilar(args.thisViewFrustum)) {
        //printf("view frustum hasn't changed BAIL!!!\n");
        _inhideOutOfView = false;
        return;
    }

    lockTree();                                  
    _tree->recurseTreeWithOperation(hideOutOfViewOperation,(void*)&args);
    unlockTree();
    _lastCulledViewFrustum = args.thisViewFrustum; // save last stable
    _culledOnce = true;

    if (args.nodesRemoved) {
        _tree->setDirtyBit();
        setupNewVoxelsForDrawingSingleNode(DONT_BAIL_EARLY);
    }
    
    bool extraDebugDetails = Menu::getInstance()->isOptionChecked(MenuOption::ExtraDebugging);
    if (extraDebugDetails) {
        qDebug("hideOutOfView() scanned=%ld removed=%ld inside=%ld intersect=%ld outside=%ld\n", 
                args.nodesScanned, args.nodesRemoved, args.nodesInside, 
                args.nodesIntersect, args.nodesOutside
            );
        qDebug("    inside/inside=%ld intersect/inside=%ld outside/outside=%ld\n", 
                args.nodesInsideInside, args.nodesIntersectInside, args.nodesOutsideOutside
            );
    }
    _inhideOutOfView = false;
}

bool VoxelSystem::hideAllSubTreeOperation(VoxelNode* node, void* extraData) {
    hideOutOfViewArgs* args = (hideOutOfViewArgs*)extraData;

    // If we've culled at least once, then we will use the status of this voxel in the last culled frustum to determine
    // how to proceed. If we've never culled, then we just consider all these voxels to be UNKNOWN so that we will not
    // consider that case.
    ViewFrustum::location inLastCulledFrustum;
    
    if (args->culledOnce && args->wantDeltaFrustums) {
        inLastCulledFrustum = node->inFrustum(args->lastViewFrustum);
        
        // if this node is fully OUTSIDE our last culled view frustum, then we don't need to recurse further
        if (inLastCulledFrustum == ViewFrustum::OUTSIDE) {
            args->nodesOutsideOutside++;
            return false;
        }
    }

    args->nodesOutside++;
    if (node->isKnownBufferIndex()) {
        args->nodesRemoved++;
        bool falseColorize = false;
        if (falseColorize) {
            node->setFalseColor(255,0,0); // false colorize
        } else {
            VoxelSystem* thisVoxelSystem = args->thisVoxelSystem;
            thisVoxelSystem->_voxelsUpdated += thisVoxelSystem->forceRemoveNodeFromArrays(node);
            thisVoxelSystem->setupNewVoxelsForDrawingSingleNode();
        }

    }
    
    return true;
}

bool VoxelSystem::showAllSubTreeOperation(VoxelNode* node, void* extraData) {
    hideOutOfViewArgs* args = (hideOutOfViewArgs*)extraData;

    // If we've culled at least once, then we will use the status of this voxel in the last culled frustum to determine
    // how to proceed. If we've never culled, then we just consider all these voxels to be UNKNOWN so that we will not
    // consider that case.
    ViewFrustum::location inLastCulledFrustum;
    
    if (args->culledOnce && args->wantDeltaFrustums) {
        inLastCulledFrustum = node->inFrustum(args->lastViewFrustum);
        
        // if this node is fully inside our last culled view frustum, then we don't need to recurse further
        if (inLastCulledFrustum == ViewFrustum::INSIDE) {
            args->nodesInsideInside++;
            return false;
        }
    }

    args->nodesInside++;

    float voxelSizeScale = Menu::getInstance()->getVoxelSizeScale();
    int boundaryLevelAdjust = Menu::getInstance()->getBoundaryLevelAdjust();
    bool shouldRender = node->calculateShouldRender(&args->thisViewFrustum, voxelSizeScale, boundaryLevelAdjust);
    node->setShouldRender(shouldRender);

    if (shouldRender && !node->isKnownBufferIndex()) {
        bool falseColorize = false;
        if (falseColorize) {
            node->setFalseColor(0,0,255); // false colorize
        }
        // These are both needed to force redraw...
        node->setDirtyBit();
        node->markWithChangedTime();
    }

    return true; // keep recursing!
}

// "hide" voxels in the VBOs that are still in the tree that but not in view. 
// We don't remove them from the tree, we don't delete them, we do remove them
// from the VBOs and mark them as such in the tree.
bool VoxelSystem::hideOutOfViewOperation(VoxelNode* node, void* extraData) {
    hideOutOfViewArgs* args = (hideOutOfViewArgs*)extraData;

    // If we're still recursing the tree using this operator, then we don't know if we're inside or outside... 
    // so before we move forward we need to determine our frustum location
    ViewFrustum::location inFrustum = node->inFrustum(args->thisViewFrustum);
    
    // If we've culled at least once, then we will use the status of this voxel in the last culled frustum to determine
    // how to proceed. If we've never culled, then we just consider all these voxels to be UNKNOWN so that we will not
    // consider that case.
    ViewFrustum::location inLastCulledFrustum;
    
    if (args->culledOnce && args->wantDeltaFrustums) {
        inLastCulledFrustum = node->inFrustum(args->lastViewFrustum);
    }
        
    // ok, now do some processing for this node...
    switch (inFrustum) {
        case ViewFrustum::OUTSIDE: {
        
            // If this node is outside the current view, then we might want to hide it... unless it was previously OUTSIDE,
            // if it was previously outside, then we can safely assume it's already hidden, and we can also safely assume
            // that all of it's children are outside both of our views, in which case we can just stop recursing...
            if (args->culledOnce && args->wantDeltaFrustums && inLastCulledFrustum == ViewFrustum::OUTSIDE) {
                args->nodesScanned++;
                args->nodesOutsideOutside++;
                return false; // stop recursing this branch!
            }
            
            // if this node is fully OUTSIDE the view, but previously intersected and/or was inside the last view, then
            // we need to hide it. Additionally we know that ALL of it's children are also fully OUTSIDE so we can recurse 
            // the children and simply mark them as hidden
            args->tree->recurseNodeWithOperation(node, hideAllSubTreeOperation, args );
            
            return false;
            
        } break;
        case ViewFrustum::INSIDE: {
        
            // If this node is INSIDE the current view, then we might want to show it... unless it was previously INSIDE,
            // if it was previously INSIDE, then we can safely assume it's already shown, and we can also safely assume
            // that all of it's children are INSIDE both of our views, in which case we can just stop recursing...
            if (args->culledOnce && args->wantDeltaFrustums && inLastCulledFrustum == ViewFrustum::INSIDE) {
                args->nodesScanned++;
                args->nodesInsideInside++;
                return false; // stop recursing this branch!
            }
        
            // if this node is fully INSIDE the view, but previously INTERSECTED and/or was OUTSIDE the last view, then
            // we need to show it. Additionally we know that ALL of it's children are also fully INSIDE so we can recurse 
            // the children and simply mark them as visible (as appropriate based on LOD)
            args->tree->recurseNodeWithOperation(node, showAllSubTreeOperation, args);

            return false;    
        } break;
        case ViewFrustum::INTERSECT: {
            args->nodesScanned++;

            // If this node INTERSECTS the current view, then we might want to show it... unless it was previously INSIDE 
            // the last known view, in which case it will already be visible, and we know that all it's children are also
            // previously INSIDE and visible. So in this case stop recursing
            if (args->culledOnce && args->wantDeltaFrustums && inLastCulledFrustum == ViewFrustum::INSIDE) {
                args->nodesIntersectInside++;
                return false; // stop recursing this branch!
            }

            args->nodesIntersect++;

            // if the child node INTERSECTs the view, then we want to check to see if it thinks it should render
            // if it should render but is missing it's VBO index, then we want to flip it on, and we can stop recursing from 
            // here because we know will block any children anyway
            if (node->getShouldRender() && !node->isKnownBufferIndex()) {
                node->setDirtyBit(); // will this make it draw?
                return false;
            }

            // If it INTERSECTS but shouldn't be displayed, then it's probably a parent and it is at least partially in view.
            // So we DO want to recurse the children because some of them may not be in view... nothing specifically to do, 
            // just keep iterating the children
            return true;            

        } break;
    } // switch
    

    return true; // keep going!
}


bool VoxelSystem::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                      VoxelDetail& detail, float& distance, BoxFace& face) {
    lockTree();                                  
    VoxelNode* node;
    if (!_tree->findRayIntersection(origin, direction, node, distance, face)) {
        unlockTree();
        return false;
    }
    detail.x = node->getCorner().x;
    detail.y = node->getCorner().y;
    detail.z = node->getCorner().z;
    detail.s = node->getScale();
    detail.red = node->getColor()[0];
    detail.green = node->getColor()[1];
    detail.blue = node->getColor()[2];
    unlockTree();
    return true;
}

bool VoxelSystem::findSpherePenetration(const glm::vec3& center, float radius, glm::vec3& penetration) {
    lockTree();
    bool result = _tree->findSpherePenetration(center, radius, penetration);
    unlockTree();
    return result;
}

bool VoxelSystem::findCapsulePenetration(const glm::vec3& start, const glm::vec3& end, float radius, glm::vec3& penetration) {
    lockTree();
    bool result = _tree->findCapsulePenetration(start, end, radius, penetration);
    unlockTree();
    return result;
}

class falseColorizeRandomEveryOtherArgs {
public:
    falseColorizeRandomEveryOtherArgs() : totalNodes(0), colorableNodes(0), coloredNodes(0), colorThis(true) {};
    unsigned long totalNodes;
    unsigned long colorableNodes;
    unsigned long coloredNodes;
    bool colorThis;
};

bool VoxelSystem::falseColorizeRandomEveryOtherOperation(VoxelNode* node, void* extraData) {
    falseColorizeRandomEveryOtherArgs* args = (falseColorizeRandomEveryOtherArgs*)extraData;
    args->totalNodes++;
    if (node->isColored()) {
        args->colorableNodes++;
        if (args->colorThis) {
            args->coloredNodes++;
            node->setFalseColor(255, randomColorValue(150), randomColorValue(150));
        }
        args->colorThis = !args->colorThis;
    }
    return true; // keep going!
}

void VoxelSystem::falseColorizeRandomEveryOther() {
    falseColorizeRandomEveryOtherArgs args;
    _tree->recurseTreeWithOperation(falseColorizeRandomEveryOtherOperation,&args);
    qDebug("randomized false color for every other node: total %ld, colorable %ld, colored %ld\n", 
        args.totalNodes, args.colorableNodes, args.coloredNodes);
    _tree->setDirtyBit();
    setupNewVoxelsForDrawing();
}

class collectStatsForTreesAndVBOsArgs {
public:
    collectStatsForTreesAndVBOsArgs(int maxVoxels) : 
        totalNodes(0), 
        dirtyNodes(0), 
        shouldRenderNodes(0),
        coloredNodes(0),
        nodesInVBO(0),
        nodesInVBONotShouldRender(0),
        nodesInVBOOverExpectedMax(0),
        duplicateVBOIndex(0),
        leafNodes(0)
        {
            hasIndexFound = new bool[maxVoxels];
            memset(hasIndexFound, false, maxVoxels * sizeof(bool));
        };
        
    ~collectStatsForTreesAndVBOsArgs() {
        delete[] hasIndexFound;
    }

    unsigned long totalNodes;
    unsigned long dirtyNodes;
    unsigned long shouldRenderNodes;
    unsigned long coloredNodes;
    unsigned long nodesInVBO;
    unsigned long nodesInVBONotShouldRender;
    unsigned long nodesInVBOOverExpectedMax;
    unsigned long duplicateVBOIndex;
    unsigned long leafNodes;

    unsigned long expectedMax;
    
    bool* hasIndexFound;
};

bool VoxelSystem::collectStatsForTreesAndVBOsOperation(VoxelNode* node, void* extraData) {
    
    collectStatsForTreesAndVBOsArgs* args = (collectStatsForTreesAndVBOsArgs*)extraData;
    args->totalNodes++;

    if (node->isLeaf()) {
        args->leafNodes++;
    }

    if (node->isColored()) {
        args->coloredNodes++;
    }

    if (node->getShouldRender()) {
        args->shouldRenderNodes++;
    }

    if (node->isDirty()) {
        args->dirtyNodes++;
    }

    if (node->isKnownBufferIndex()) {
        args->nodesInVBO++;
        unsigned long nodeIndex = node->getBufferIndex();
        
        const bool extraDebugging = false; // enable for extra debugging
        if (extraDebugging) {
            qDebug("node In VBO... [%f,%f,%f] %f ... index=%ld, isDirty=%s, shouldRender=%s \n", 
                    node->getCorner().x, node->getCorner().y, node->getCorner().z, node->getScale(), 
                    nodeIndex, debug::valueOf(node->isDirty()), debug::valueOf(node->getShouldRender()));
        }

        if (args->hasIndexFound[nodeIndex]) {
            args->duplicateVBOIndex++;
            qDebug("duplicateVBO found... index=%ld, isDirty=%s, shouldRender=%s \n", nodeIndex, 
                    debug::valueOf(node->isDirty()), debug::valueOf(node->getShouldRender()));
        } else {
            args->hasIndexFound[nodeIndex] = true;
        }
        if (nodeIndex > args->expectedMax) {
            args->nodesInVBOOverExpectedMax++;
        }
        
        // if it's in VBO but not-shouldRender, track that also...
        if (!node->getShouldRender()) {
            args->nodesInVBONotShouldRender++;
        }
    }

    return true; // keep going!
}

void VoxelSystem::collectStatsForTreesAndVBOs() {
    PerformanceWarning warn(true, "collectStatsForTreesAndVBOs()", true);

    glBufferIndex minDirty = GLBUFFER_INDEX_UNKNOWN;
    glBufferIndex maxDirty = 0;

    for (glBufferIndex i = 0; i < _voxelsInWriteArrays; i++) {
        if (_writeVoxelDirtyArray[i]) {
            minDirty = std::min(minDirty,i);
            maxDirty = std::max(maxDirty,i);
        }
    }

    collectStatsForTreesAndVBOsArgs args(_maxVoxels);
    args.expectedMax = _voxelsInWriteArrays;
    
    qDebug("CALCULATING Local Voxel Tree Statistics >>>>>>>>>>>>\n");
    
    _tree->recurseTreeWithOperation(collectStatsForTreesAndVBOsOperation,&args);

    qDebug("Local Voxel Tree Statistics:\n total nodes %ld \n leaves %ld \n dirty %ld \n colored %ld \n shouldRender %ld \n",
        args.totalNodes, args.leafNodes, args.dirtyNodes, args.coloredNodes, args.shouldRenderNodes);

    qDebug(" _voxelsDirty=%s \n _voxelsInWriteArrays=%ld \n minDirty=%ld \n maxDirty=%ld \n", debug::valueOf(_voxelsDirty),
        _voxelsInWriteArrays, minDirty, maxDirty);

    qDebug(" inVBO %ld \n nodesInVBOOverExpectedMax %ld \n duplicateVBOIndex %ld \n nodesInVBONotShouldRender %ld \n", 
        args.nodesInVBO, args.nodesInVBOOverExpectedMax, args.duplicateVBOIndex, args.nodesInVBONotShouldRender);

    glBufferIndex minInVBO = GLBUFFER_INDEX_UNKNOWN;
    glBufferIndex maxInVBO = 0;

    for (glBufferIndex i = 0; i < _maxVoxels; i++) {
        if (args.hasIndexFound[i]) {
            minInVBO = std::min(minInVBO,i);
            maxInVBO = std::max(maxInVBO,i);
        }
    }

    qDebug(" minInVBO=%ld \n maxInVBO=%ld \n _voxelsInWriteArrays=%ld \n _voxelsInReadArrays=%ld \n", 
            minInVBO, maxInVBO, _voxelsInWriteArrays, _voxelsInReadArrays);

    qDebug(" _freeIndexes.size()=%ld \n", 
            _freeIndexes.size());

    qDebug("DONE WITH Local Voxel Tree Statistics >>>>>>>>>>>>\n");
}


void VoxelSystem::deleteVoxelAt(float x, float y, float z, float s) {
    lockTree();
    _tree->deleteVoxelAt(x, y, z, s);
    unlockTree();
    
    // redraw!
    setupNewVoxelsForDrawing();  // do we even need to do this? Or will the next network receive kick in?
    
};

VoxelNode* VoxelSystem::getVoxelAt(float x, float y, float z, float s) const { 
    return _tree->getVoxelAt(x, y, z, s); 
};

void VoxelSystem::createVoxel(float x, float y, float z, float s, 
                              unsigned char red, unsigned char green, unsigned char blue, bool destructive) {
    
    //qDebug("VoxelSystem::createVoxel(%f,%f,%f,%f)\n",x,y,z,s);
    lockTree();
    _tree->createVoxel(x, y, z, s, red, green, blue, destructive); 
    unlockTree();

    setupNewVoxelsForDrawing(); 
};

void VoxelSystem::createLine(glm::vec3 point1, glm::vec3 point2, float unitSize, rgbColor color, bool destructive) { 
    _tree->createLine(point1, point2, unitSize, color, destructive); 
    setupNewVoxelsForDrawing(); 
};

void VoxelSystem::createSphere(float r,float xc, float yc, float zc, float s, bool solid, 
                               creationMode mode, bool destructive, bool debug) { 
    _tree->createSphere(r, xc, yc, zc, s, solid, mode, destructive, debug); 
    setupNewVoxelsForDrawing(); 
};

void VoxelSystem::copySubTreeIntoNewTree(VoxelNode* startNode, VoxelSystem* destination, bool rebaseToRoot) {
    _tree->copySubTreeIntoNewTree(startNode, destination->_tree, rebaseToRoot);
    destination->setupNewVoxelsForDrawing();
}

void VoxelSystem::copySubTreeIntoNewTree(VoxelNode* startNode, VoxelTree* destination, bool rebaseToRoot) {
    _tree->copySubTreeIntoNewTree(startNode, destination, rebaseToRoot);
}

void VoxelSystem::copyFromTreeIntoSubTree(VoxelTree* sourceTree, VoxelNode* destinationNode) {
    _tree->copyFromTreeIntoSubTree(sourceTree, destinationNode);
}

void VoxelSystem::recurseTreeWithOperation(RecurseVoxelTreeOperation operation, void* extraData) {
    _tree->recurseTreeWithOperation(operation, extraData);
}

struct FalseColorizeOccludedArgs {
    ViewFrustum* viewFrustum;
    CoverageMap* map;
    CoverageMapV2* mapV2;
    VoxelTree* tree;
    long totalVoxels;
    long coloredVoxels;
    long occludedVoxels;
    long notOccludedVoxels;
    long outOfView;
    long subtreeVoxelsSkipped;
    long nonLeaves;
    long nonLeavesOutOfView;
    long nonLeavesOccluded;
};

struct FalseColorizeSubTreeOperationArgs {
    unsigned char color[NUMBER_OF_COLORS];
    long voxelsTouched;
};

bool VoxelSystem::falseColorizeSubTreeOperation(VoxelNode* node, void* extraData) {
    if (node->getShouldRender()) {
        FalseColorizeSubTreeOperationArgs* args = (FalseColorizeSubTreeOperationArgs*) extraData;
        node->setFalseColor(args->color[0], args->color[1], args->color[2]);
        args->voxelsTouched++;
    }
    return true;    
}

bool VoxelSystem::falseColorizeOccludedOperation(VoxelNode* node, void* extraData) {

    FalseColorizeOccludedArgs* args = (FalseColorizeOccludedArgs*) extraData;
    args->totalVoxels++;

    // If we are a parent, let's see if we're completely occluded.
    if (!node->isLeaf()) {
        args->nonLeaves++;

        AABox voxelBox = node->getAABox();
        voxelBox.scale(TREE_SCALE);
        VoxelProjectedPolygon* voxelPolygon = new VoxelProjectedPolygon(args->viewFrustum->getProjectedPolygon(voxelBox));

        // If we're not all in view, then ignore it, and just return. But keep searching...
        if (!voxelPolygon->getAllInView()) {
            args->nonLeavesOutOfView++;
            delete voxelPolygon;
            return true;
        }

        CoverageMapStorageResult result = args->map->checkMap(voxelPolygon, false);
        if (result == OCCLUDED) {
            args->nonLeavesOccluded++;
            delete voxelPolygon;
            
            FalseColorizeSubTreeOperationArgs subArgs;
            subArgs.color[0] = 0;
            subArgs.color[1] = 255;
            subArgs.color[2] = 0;
            subArgs.voxelsTouched = 0;
            
            args->tree->recurseNodeWithOperation(node, falseColorizeSubTreeOperation, &subArgs );
            
            args->subtreeVoxelsSkipped += (subArgs.voxelsTouched - 1);
            args->totalVoxels += (subArgs.voxelsTouched - 1);
            
            return false;
        }

        delete voxelPolygon;
        return true; // keep looking...
    }

    if (node->isLeaf() && node->isColored() && node->getShouldRender()) {
        args->coloredVoxels++;

        AABox voxelBox = node->getAABox();
        voxelBox.scale(TREE_SCALE);
        VoxelProjectedPolygon* voxelPolygon = new VoxelProjectedPolygon(args->viewFrustum->getProjectedPolygon(voxelBox));

        // If we're not all in view, then ignore it, and just return. But keep searching...
        if (!voxelPolygon->getAllInView()) {
            args->outOfView++;
            delete voxelPolygon;
            return true;
        }

        CoverageMapStorageResult result = args->map->checkMap(voxelPolygon, true);
        if (result == OCCLUDED) {
            node->setFalseColor(255, 0, 0);
            args->occludedVoxels++;
        } else if (result == STORED) {
            args->notOccludedVoxels++;
            //qDebug("***** falseColorizeOccludedOperation() NODE is STORED *****\n");
        } else if (result == DOESNT_FIT) {
            //qDebug("***** falseColorizeOccludedOperation() NODE DOESNT_FIT???? *****\n");
        }
    }
    return true; // keep going!
}

void VoxelSystem::falseColorizeOccluded() {
    PerformanceWarning warn(true, "falseColorizeOccluded()",true);
    myCoverageMap.erase();
    
    FalseColorizeOccludedArgs args;
    args.viewFrustum = _viewFrustum;
    args.map = &myCoverageMap; 
    args.totalVoxels = 0;
    args.coloredVoxels = 0;
    args.occludedVoxels = 0;
    args.notOccludedVoxels = 0;
    args.outOfView = 0;
    args.subtreeVoxelsSkipped = 0;
    args.nonLeaves = 0;
    args.nonLeavesOutOfView = 0;
    args.nonLeavesOccluded = 0;
    args.tree = _tree;

    VoxelProjectedPolygon::pointInside_calls = 0;
    VoxelProjectedPolygon::occludes_calls = 0;
    VoxelProjectedPolygon::intersects_calls = 0;
    
    glm::vec3 position = args.viewFrustum->getPosition() * (1.0f/TREE_SCALE);

    _tree->recurseTreeWithOperationDistanceSorted(falseColorizeOccludedOperation, position, (void*)&args);

    qDebug("falseColorizeOccluded()\n    position=(%f,%f)\n    total=%ld\n    colored=%ld\n    occluded=%ld\n    notOccluded=%ld\n    outOfView=%ld\n    subtreeVoxelsSkipped=%ld\n    nonLeaves=%ld\n    nonLeavesOutOfView=%ld\n    nonLeavesOccluded=%ld\n    pointInside_calls=%ld\n    occludes_calls=%ld\n intersects_calls=%ld\n", 
        position.x, position.y,
        args.totalVoxels, args.coloredVoxels, args.occludedVoxels, 
        args.notOccludedVoxels, args.outOfView, args.subtreeVoxelsSkipped, 
        args.nonLeaves, args.nonLeavesOutOfView, args.nonLeavesOccluded,
        VoxelProjectedPolygon::pointInside_calls,
        VoxelProjectedPolygon::occludes_calls,
        VoxelProjectedPolygon::intersects_calls
    );


    //myCoverageMap.erase();

    _tree->setDirtyBit();
    setupNewVoxelsForDrawing();
}

bool VoxelSystem::falseColorizeOccludedV2Operation(VoxelNode* node, void* extraData) {

    FalseColorizeOccludedArgs* args = (FalseColorizeOccludedArgs*) extraData;
    args->totalVoxels++;

    // If we are a parent, let's see if we're completely occluded.
    if (!node->isLeaf()) {
        args->nonLeaves++;

        AABox voxelBox = node->getAABox();
        voxelBox.scale(TREE_SCALE);
        VoxelProjectedPolygon* voxelPolygon = new VoxelProjectedPolygon(args->viewFrustum->getProjectedPolygon(voxelBox));

        // If we're not all in view, then ignore it, and just return. But keep searching...
        if (!voxelPolygon->getAllInView()) {
            args->nonLeavesOutOfView++;
            delete voxelPolygon;
            return true;
        }

        CoverageMapV2StorageResult result = args->mapV2->checkMap(voxelPolygon, false);
        if (result == V2_OCCLUDED) {
            args->nonLeavesOccluded++;
            delete voxelPolygon;
            
            FalseColorizeSubTreeOperationArgs subArgs;
            subArgs.color[0] = 0;
            subArgs.color[1] = 255;
            subArgs.color[2] = 0;
            subArgs.voxelsTouched = 0;
            
            args->tree->recurseNodeWithOperation(node, falseColorizeSubTreeOperation, &subArgs );
            
            args->subtreeVoxelsSkipped += (subArgs.voxelsTouched - 1);
            args->totalVoxels += (subArgs.voxelsTouched - 1);
            
            return false;
        }

        delete voxelPolygon;
        return true; // keep looking...
    }

    if (node->isLeaf() && node->isColored() && node->getShouldRender()) {
        args->coloredVoxels++;

        AABox voxelBox = node->getAABox();
        voxelBox.scale(TREE_SCALE);
        VoxelProjectedPolygon* voxelPolygon = new VoxelProjectedPolygon(args->viewFrustum->getProjectedPolygon(voxelBox));

        // If we're not all in view, then ignore it, and just return. But keep searching...
        if (!voxelPolygon->getAllInView()) {
            args->outOfView++;
            delete voxelPolygon;
            return true;
        }

        CoverageMapV2StorageResult result = args->mapV2->checkMap(voxelPolygon, true);
        if (result == V2_OCCLUDED) {
            node->setFalseColor(255, 0, 0);
            args->occludedVoxels++;
        } else if (result == V2_STORED) {
            args->notOccludedVoxels++;
            //qDebug("***** falseColorizeOccludedOperation() NODE is STORED *****\n");
        } else if (result == V2_DOESNT_FIT) {
            //qDebug("***** falseColorizeOccludedOperation() NODE DOESNT_FIT???? *****\n");
        }
        delete voxelPolygon; // V2 maps don't store polygons, so we're always in charge of freeing
    }
    return true; // keep going!
}


void VoxelSystem::falseColorizeOccludedV2() {
    PerformanceWarning warn(true, "falseColorizeOccludedV2()",true);
    myCoverageMapV2.erase();

    CoverageMapV2::wantDebugging = true;

    VoxelProjectedPolygon::pointInside_calls = 0;
    VoxelProjectedPolygon::occludes_calls = 0;
    VoxelProjectedPolygon::intersects_calls = 0;
    
    FalseColorizeOccludedArgs args;
    args.viewFrustum = _viewFrustum;
    args.mapV2 = &myCoverageMapV2; 
    args.totalVoxels = 0;
    args.coloredVoxels = 0;
    args.occludedVoxels = 0;
    args.notOccludedVoxels = 0;
    args.outOfView = 0;
    args.subtreeVoxelsSkipped = 0;
    args.nonLeaves = 0;
    args.nonLeavesOutOfView = 0;
    args.nonLeavesOccluded = 0;
    args.tree = _tree;
    
    glm::vec3 position = args.viewFrustum->getPosition() * (1.0f/TREE_SCALE);

    _tree->recurseTreeWithOperationDistanceSorted(falseColorizeOccludedV2Operation, position, (void*)&args);

    qDebug("falseColorizeOccludedV2()\n    position=(%f,%f)\n    total=%ld\n    colored=%ld\n    occluded=%ld\n    notOccluded=%ld\n    outOfView=%ld\n    subtreeVoxelsSkipped=%ld\n    nonLeaves=%ld\n    nonLeavesOutOfView=%ld\n    nonLeavesOccluded=%ld\n    pointInside_calls=%ld\n    occludes_calls=%ld\n    intersects_calls=%ld\n", 
        position.x, position.y,
        args.totalVoxels, args.coloredVoxels, args.occludedVoxels, 
        args.notOccludedVoxels, args.outOfView, args.subtreeVoxelsSkipped, 
        args.nonLeaves, args.nonLeavesOutOfView, args.nonLeavesOccluded,
        VoxelProjectedPolygon::pointInside_calls,
        VoxelProjectedPolygon::occludes_calls,
        VoxelProjectedPolygon::intersects_calls
    );
    //myCoverageMapV2.erase();
    _tree->setDirtyBit();
    setupNewVoxelsForDrawing();
}

void VoxelSystem::nodeAdded(Node* node) {
    if (node->getType() == NODE_TYPE_VOXEL_SERVER) {
        qDebug("VoxelSystem... voxel server %s added...\n", node->getUUID().toString().toLocal8Bit().constData());
        _voxelServerCount++;
    }
}

bool VoxelSystem::killSourceVoxelsOperation(VoxelNode* node, void* extraData) {
    QUuid killedNodeID = *(QUuid*)extraData;
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        VoxelNode* childNode = node->getChildAtIndex(i);
        if (childNode) {
            if (childNode->matchesSourceUUID(killedNodeID)) {
                node->safeDeepDeleteChildAtIndex(i);
            }
        }
    }
    return true;
}

void VoxelSystem::nodeKilled(Node* node) {
    if (node->getType() == NODE_TYPE_VOXEL_SERVER) {
        _voxelServerCount--;

        QUuid nodeUUID = node->getUUID();

        qDebug("VoxelSystem... voxel server %s removed...\n", nodeUUID.toString().toLocal8Bit().constData());
        
        if (_voxelServerCount > 0) {
            // Kill any voxels from the local tree that match this nodeID
            // commenting out for removal of 16 bit node IDs
            lockTree();
            _tree->recurseTreeWithOperation(killSourceVoxelsOperation, &nodeUUID);
            unlockTree();
            _tree->setDirtyBit();
            setupNewVoxelsForDrawing();
        } else {
            // Last server, take the easy way and kill all the local voxels!
            killLocalVoxels();
        }
    }
}

void VoxelSystem::domainChanged(QString domain) {
    killLocalVoxels();
}


unsigned long VoxelSystem::getFreeMemoryGPU() {
    // We can't ask all GPUs how much memory they have in use, but we can ask them about how much is free.
    // So, we can record the free memory before we create our VBOs and the free memory after, and get a basic
    // idea how how much we're using.

    _hasMemoryUsageGPU = false; // assume the worst
    unsigned long freeMemory = 0;
    const int NUM_RESULTS = 4; // see notes, these APIs return up to 4 results
    GLint results[NUM_RESULTS] = { 0, 0, 0, 0};

    // ATI
    // http://www.opengl.org/registry/specs/ATI/meminfo.txt
    //
    // TEXTURE_FREE_MEMORY_ATI                 0x87FC
    // RENDERBUFFER_FREE_MEMORY_ATI            0x87FD
    const GLenum VBO_FREE_MEMORY_ATI = 0x87FB;
    glGetIntegerv(VBO_FREE_MEMORY_ATI, &results[0]);
    GLenum errorATI = glGetError();
    
    if (errorATI == GL_NO_ERROR) {
        _hasMemoryUsageGPU = true;
        freeMemory = results[0];
    } else {

        // NVIDIA
        // http://developer.download.nvidia.com/opengl/specs/GL_NVX_gpu_memory_info.txt
        //
        //const GLenum GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX = 0x9047;
        //const GLenum GPU_MEMORY_INFO_EVICTION_COUNT_NVX = 0x904A;
        //const GLenum GPU_MEMORY_INFO_EVICTED_MEMORY_NVX = 0x904B;
        //const GLenum GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX = 0x9048;
        
        const GLenum GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX = 0x9049;
        results[0] = 0;
        glGetIntegerv(GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &results[0]);
        freeMemory += results[0]; 
        GLenum errorNVIDIA = glGetError();
    
        if (errorNVIDIA == GL_NO_ERROR) {
            _hasMemoryUsageGPU = true;
            freeMemory = results[0];
        }
    }
    
    const unsigned long BYTES_PER_KBYTE = 1024;
    return freeMemory * BYTES_PER_KBYTE; // API results in KB, we want it in bytes
}

unsigned long VoxelSystem::getVoxelMemoryUsageGPU() {
    unsigned long currentFreeMemory = getFreeMemoryGPU();
    return (_initialMemoryUsageGPU - currentFreeMemory);
}

void VoxelSystem::lockTree() {
    pthread_mutex_lock(&_treeLock);
    _treeIsBusy = true;
}

void VoxelSystem::unlockTree() {
    _treeIsBusy = false;
    pthread_mutex_unlock(&_treeLock);
}



