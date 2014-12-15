//
//  VoxelSystem.cpp
//  interface/src/voxels
//
//  Created by Philip on 12/31/12.
//  Copyright 2012 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <cstring>
#include <cmath>
#include <iostream> // to load voxels from file
#include <fstream> // to load voxels from file

#include <OctalCode.h>
#include <PacketHeaders.h>
#include <PerfStat.h>
#include <SharedUtil.h>
#include <NodeList.h>

#include <glm/gtc/type_ptr.hpp>

#include "Application.h"
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

static glm::vec3 grayColor = glm::vec3(0.3f, 0.3f, 0.3f);

VoxelSystem::VoxelSystem(float treeScale, int maxVoxels, VoxelTree* tree)
    : NodeData(),
    _treeScale(treeScale),
    _maxVoxels(maxVoxels),
    _initialized(false),
    _writeArraysLock(QReadWriteLock::Recursive),
    _readArraysLock(QReadWriteLock::Recursive),
    _drawHaze(false),
    _farHazeDistance(300.0f),
    _hazeColor(grayColor)
{

    _voxelsInReadArrays = _voxelsInWriteArrays = _voxelsUpdated = 0;
    _writeRenderFullVBO = true;
    _readRenderFullVBO = true;
    _tree = (tree) ? tree : new VoxelTree();

    _tree->getRoot()->setVoxelSystem(this);

    VoxelTreeElement::addDeleteHook(this);
    VoxelTreeElement::addUpdateHook(this);
    _falseColorizeBySource = false;
    _dataSourceUUID = QUuid();
    _voxelServerCount = 0;

    _viewFrustum = Application::getInstance()->getViewFrustum();

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

    _lastKnownVoxelSizeScale = DEFAULT_OCTREE_SIZE_SCALE;
    _lastKnownBoundaryLevelAdjust = 0;
}

void VoxelSystem::elementDeleted(OctreeElement* element) {
    VoxelTreeElement* voxel = (VoxelTreeElement*)element;
    if (voxel->getVoxelSystem() == this) {
        if ((_voxelsInWriteArrays != 0)) {
            forceRemoveNodeFromArrays(voxel);
        } else {
            if (Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings)) {
                qDebug("VoxelSystem::elementDeleted() while _voxelsInWriteArrays==0, is that expected? ");
            }
        }
    }
}

void VoxelSystem::setDisableFastVoxelPipeline(bool disableFastVoxelPipeline) {
    _useFastVoxelPipeline = !disableFastVoxelPipeline;
    setupNewVoxelsForDrawing();
}

void VoxelSystem::elementUpdated(OctreeElement* element) {
    VoxelTreeElement* voxel = (VoxelTreeElement*)element;

    // If we're in SetupNewVoxelsForDrawing() or _writeRenderFullVBO then bail..
    if (!_useFastVoxelPipeline || _inSetupNewVoxelsForDrawing || _writeRenderFullVBO) {
        return;
    }

    if (voxel->getVoxelSystem() == this) {
        bool shouldRender = false; // assume we don't need to render it
        // if it's colored, we might need to render it!
        float voxelSizeScale = Menu::getInstance()->getVoxelSizeScale();
        int boundaryLevelAdjust = Menu::getInstance()->getBoundaryLevelAdjust();
        shouldRender = voxel->calculateShouldRender(_viewFrustum, voxelSizeScale, boundaryLevelAdjust);

        if (voxel->getShouldRender() != shouldRender) {
            voxel->setShouldRender(shouldRender);
        }

        if (!voxel->isLeaf()) {

            // As we check our children, see if any of them went from shouldRender to NOT shouldRender
            // then we probably dropped LOD and if we don't have color, we want to average our children
            // for a new color.
            int childrenGotHiddenCount = 0;
            for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
                VoxelTreeElement* childVoxel = voxel->getChildAtIndex(i);
                if (childVoxel) {
                    bool wasShouldRender = childVoxel->getShouldRender();
                    bool isShouldRender = childVoxel->calculateShouldRender(_viewFrustum, voxelSizeScale, boundaryLevelAdjust);
                    if (wasShouldRender && !isShouldRender) {
                        childrenGotHiddenCount++;
                    }
                }
            }
            if (childrenGotHiddenCount > 0) {
                voxel->calculateAverageFromChildren();
            }
        }

        const bool REUSE_INDEX = true;
        const bool DONT_FORCE_REDRAW = false;
        updateNodeInArrays(voxel, REUSE_INDEX, DONT_FORCE_REDRAW);
        _voxelsUpdated++;

        voxel->clearDirtyBit(); // clear the dirty bit, do this before we potentially delete things.

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
        _freeIndexLock.lock();
        output = _freeIndexes.back();
        _freeIndexes.pop_back();
        _freeIndexLock.unlock();
    } else {
        output = _voxelsInWriteArrays;
        _voxelsInWriteArrays++;
    }
    return output;
}

// Release responsibility of the buffer/vbo index from the VoxelTreeElement, and makes the index available for some other node to use
// will also "clean up" the index data for the buffer/vbo slot, so that if it's in the middle of the draw range, the triangles
// will be "invisible"
void VoxelSystem::freeBufferIndex(glBufferIndex index) {
    if (_voxelsInWriteArrays == 0) {
        qDebug() << "freeBufferIndex() called when _voxelsInWriteArrays == 0!";
    }

    // make the index available for next node that needs to be drawn
    _freeIndexLock.lock();
    _freeIndexes.push_back(index);
    _freeIndexLock.unlock();

    // make the VBO slot "invisible" in case this slot is not used
    const glm::vec3 startVertex(FLT_MAX, FLT_MAX, FLT_MAX);
    const float voxelScale = 0;
    const nodeColor BLACK = {0, 0, 0, 0};
    updateArraysDetails(index, startVertex, voxelScale, BLACK);
}

// This will run through the list of _freeIndexes and reset their VBO array values to be "invisible".
void VoxelSystem::clearFreeBufferIndexes() {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "clearFreeBufferIndexes()");
    _voxelsInWriteArrays = 0; // reset our VBO

    // clear out freeIndexes
    {
        PerformanceWarning warn(showWarnings,"clearFreeBufferIndexes() : _freeIndexLock.lock()");
        _freeIndexLock.lock();
    }
    {
        PerformanceWarning warn(showWarnings,"clearFreeBufferIndexes() : _freeIndexes.clear()");
        _freeIndexes.clear();
    }
    _freeIndexLock.unlock();
}

VoxelSystem::~VoxelSystem() {
    VoxelTreeElement::removeDeleteHook(this);
    VoxelTreeElement::removeUpdateHook(this);

    cleanupVoxelMemory();
    delete _tree;
}


// This is called by the main application thread on both the initialization of the application and when
// the preferences dialog box is called/saved
void VoxelSystem::setMaxVoxels(unsigned long maxVoxels) {
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

void VoxelSystem::cleanupVoxelMemory() {
    if (_initialized) {
        _readArraysLock.lockForWrite();
        _initialized = false; // no longer initialized
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

        delete[] _writeVoxelDirtyArray;
        delete[] _readVoxelDirtyArray;
        _writeVoxelDirtyArray = _readVoxelDirtyArray = NULL;
        _readArraysLock.unlock();
    
    }
}

void VoxelSystem::setupFaceIndices(GLuint& faceVBOID, GLubyte faceIdentityIndices[]) {
    GLuint* indicesArray = new GLuint[INDICES_PER_FACE * _maxVoxels];

    // populate the indicesArray
    // this will not change given new voxels, so we can set it all up now
    for (unsigned long n = 0; n < _maxVoxels; n++) {
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
    _readArraysLock.lockForWrite();
    _writeArraysLock.lockForWrite();

    _memoryUsageRAM = 0;
    _memoryUsageVBO = 0; // our VBO allocations as we know them

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
    if (!_program.isLinked()) {
        _program.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/voxel.vert");
        _program.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/voxel.frag");
        _program.link();
    }
    _initialized = true;
    
    _writeArraysLock.unlock();
    _readArraysLock.unlock();
    
    // fog for haze
    if (_drawHaze) {
        GLfloat fogColor[] = {_hazeColor.x, _hazeColor.y, _hazeColor.z, 1.0f};
        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogfv(GL_FOG_COLOR, fogColor);
        glFogf(GL_FOG_START, 0.0f);
        glFogf(GL_FOG_END, _farHazeDistance);
    }
}

int VoxelSystem::parseData(const QByteArray& packet) {
    bool showTimingDetails = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showTimingDetails, "VoxelSystem::parseData()",showTimingDetails);

    PacketType command = packetTypeForPacket(packet);
    int numBytesPacketHeader = numBytesForPacketHeader(packet);
    switch(command) {
        case PacketTypeVoxelData: {
            PerformanceWarning warn(showTimingDetails, "VoxelSystem::parseData() PacketType_VOXEL_DATA part...",showTimingDetails);
            
            const unsigned char* dataAt = reinterpret_cast<const unsigned char*>(packet.data()) + numBytesPacketHeader;

            OCTREE_PACKET_FLAGS flags = (*(OCTREE_PACKET_FLAGS*)(dataAt));
            dataAt += sizeof(OCTREE_PACKET_FLAGS);
            OCTREE_PACKET_SEQUENCE sequence = (*(OCTREE_PACKET_SEQUENCE*)dataAt);
            dataAt += sizeof(OCTREE_PACKET_SEQUENCE);

            OCTREE_PACKET_SENT_TIME sentAt = (*(OCTREE_PACKET_SENT_TIME*)dataAt);
            dataAt += sizeof(OCTREE_PACKET_SENT_TIME);

            bool packetIsColored = oneAtBit(flags, PACKET_IS_COLOR_BIT);
            bool packetIsCompressed = oneAtBit(flags, PACKET_IS_COMPRESSED_BIT);

            OCTREE_PACKET_SENT_TIME arrivedAt = usecTimestampNow();
            int flightTime = arrivedAt - sentAt;

            OCTREE_PACKET_INTERNAL_SECTION_SIZE sectionLength = 0;
            unsigned int dataBytes = packet.size() - (numBytesPacketHeader + OCTREE_PACKET_EXTRA_HEADERS_SIZE);

            int subsection = 1;
            while (dataBytes > 0) {
                if (packetIsCompressed) {
                    if (dataBytes > sizeof(OCTREE_PACKET_INTERNAL_SECTION_SIZE)) {
                        sectionLength = (*(OCTREE_PACKET_INTERNAL_SECTION_SIZE*)dataAt);
                        dataAt += sizeof(OCTREE_PACKET_INTERNAL_SECTION_SIZE);
                        dataBytes -= sizeof(OCTREE_PACKET_INTERNAL_SECTION_SIZE);
                    } else {
                        sectionLength = 0;
                        dataBytes = 0; // stop looping something is wrong
                    }
                } else {
                    sectionLength = dataBytes;
                }

                if (sectionLength) {
                    PerformanceWarning warn(showTimingDetails, "VoxelSystem::parseData() section");
                    // ask the VoxelTree to read the bitstream into the tree
                    ReadBitstreamToTreeParams args(packetIsColored ? WANT_COLOR : NO_COLOR, WANT_EXISTS_BITS, NULL, getDataSourceUUID());
                    _tree->lockForWrite();
                    OctreePacketData packetData(packetIsCompressed);
                    packetData.loadFinalizedContent(dataAt, sectionLength);
                    if (Application::getInstance()->getLogger()->extraDebugging()) {
                        qDebug("VoxelSystem::parseData() ... Got Packet Section"
                               " color:%s compressed:%s sequence: %u flight:%d usec size:%d data:%u"
                               " subsection:%d sectionLength:%d uncompressed:%d",
                            debug::valueOf(packetIsColored), debug::valueOf(packetIsCompressed),
                            sequence, flightTime, packet.size(), dataBytes, subsection, sectionLength,
                               packetData.getUncompressedSize());
                    }
                    _tree->readBitstreamToTree(packetData.getUncompressedData(), packetData.getUncompressedSize(), args);
                    _tree->unlock();

                    dataBytes -= sectionLength;
                    dataAt += sectionLength;
                }
            }
            subsection++;
        }
        default:
            break;
    }
    if (!_useFastVoxelPipeline || _writeRenderFullVBO) {
        setupNewVoxelsForDrawing();
    } else {
        setupNewVoxelsForDrawingSingleNode(DONT_BAIL_EARLY);
    }

    Application::getInstance()->getBandwidthMeter()->inputStream(BandwidthMeter::VOXELS).updateValue(packet.size());

    return packet.size();
}

void VoxelSystem::setupNewVoxelsForDrawing() {
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                            "setupNewVoxelsForDrawing()");

    if (!_initialized) {
        return; // bail early if we're not initialized
    }

    quint64 start = usecTimestampNow();
    quint64 sinceLastTime = (start - _setupNewVoxelsForDrawingLastFinished) / 1000;

    bool iAmDebugging = false;  // if you're debugging set this to true, so you won't get skipped for slow debugging
    if (!iAmDebugging && sinceLastTime <= std::max((float) _setupNewVoxelsForDrawingLastElapsed, SIXTY_FPS_IN_MILLISECONDS)) {
        return; // bail early, it hasn't been long enough since the last time we ran
    }

    _inSetupNewVoxelsForDrawing = true;
    
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
        _voxelsUpdated = newTreeToArrays(_tree->getRoot());
        _tree->clearDirtyBit(); // after we pull the trees into the array, we can consider the tree clean

        _writeRenderFullVBO = false;
    } else {
        _voxelsUpdated = 0;
    }

    // lock on the buffer write lock so we can't modify the data when the GPU is reading it
    _readArraysLock.lockForWrite();

    if (_voxelsUpdated) {
        _voxelsDirty=true;
    }

    // copy the newly written data to the arrays designated for reading, only does something if _voxelsDirty && _voxelsUpdated
    copyWrittenDataToReadArrays(didWriteFullVBO);
    _readArraysLock.unlock();

    quint64 end = usecTimestampNow();
    int elapsedmsec = (end - start) / 1000;
    _setupNewVoxelsForDrawingLastFinished = end;
    _setupNewVoxelsForDrawingLastElapsed = elapsedmsec;
    _inSetupNewVoxelsForDrawing = false;

    bool extraDebugging = Application::getInstance()->getLogger()->extraDebugging();
    if (extraDebugging) {
        qDebug("setupNewVoxelsForDrawing()... _voxelsUpdated=%lu...",_voxelsUpdated);
        _viewFrustum->printDebugDetails();
    }
}

void VoxelSystem::setupNewVoxelsForDrawingSingleNode(bool allowBailEarly) {
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                            "setupNewVoxelsForDrawingSingleNode() xxxxx");

    quint64 start = usecTimestampNow();
    quint64 sinceLastTime = (start - _setupNewVoxelsForDrawingLastFinished) / 1000;

    bool iAmDebugging = false;  // if you're debugging set this to true, so you won't get skipped for slow debugging
    if (allowBailEarly && !iAmDebugging &&
        sinceLastTime <= std::max((float) _setupNewVoxelsForDrawingLastElapsed, SIXTY_FPS_IN_MILLISECONDS)) {
        return; // bail early, it hasn't been long enough since the last time we ran
    }

    // lock on the buffer write lock so we can't modify the data when the GPU is reading it
    {
        PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                                "setupNewVoxelsForDrawingSingleNode()... _bufferWriteLock.lock();" );
        _readArraysLock.lockForWrite();
    }

    _voxelsDirty = true; // if we got this far, then we can assume some voxels are dirty

    // copy the newly written data to the arrays designated for reading, only does something if _voxelsDirty && _voxelsUpdated
    copyWrittenDataToReadArrays(_writeRenderFullVBO);

    // after...
    _voxelsUpdated = 0;
    _readArraysLock.unlock();

    quint64 end = usecTimestampNow();
    int elapsedmsec = (end - start) / 1000;
    _setupNewVoxelsForDrawingLastFinished = end;
    _setupNewVoxelsForDrawingLastElapsed = elapsedmsec;
}



class recreateVoxelGeometryInViewArgs {
public:
    VoxelSystem* thisVoxelSystem;
    ViewFrustum thisViewFrustum;
    unsigned long nodesScanned;
    float voxelSizeScale;
    int boundaryLevelAdjust;

    recreateVoxelGeometryInViewArgs(VoxelSystem* voxelSystem) :
        thisVoxelSystem(voxelSystem),
        thisViewFrustum(*voxelSystem->getViewFrustum()),
        nodesScanned(0),
        voxelSizeScale(Menu::getInstance()->getVoxelSizeScale()),
        boundaryLevelAdjust(Menu::getInstance()->getBoundaryLevelAdjust())
    {
    }
};

// The goal of this operation is to remove any old references to old geometry, and if the voxel
// should be visible, create new geometry for it.
bool VoxelSystem::recreateVoxelGeometryInViewOperation(OctreeElement* element, void* extraData) {
    VoxelTreeElement* voxel = (VoxelTreeElement*)element;
    recreateVoxelGeometryInViewArgs* args = (recreateVoxelGeometryInViewArgs*)extraData;

    args->nodesScanned++;
    
    // reset the old geometry...
    // note: this doesn't "mark the voxel as changed", so it only releases the old buffer index thereby forgetting the
    // old geometry
    voxel->setBufferIndex(GLBUFFER_INDEX_UNKNOWN); 

    bool shouldRender = voxel->calculateShouldRender(&args->thisViewFrustum, args->voxelSizeScale, args->boundaryLevelAdjust);
    bool inView = voxel->isInView(args->thisViewFrustum);
    voxel->setShouldRender(inView && shouldRender);
    if (shouldRender && inView) {
        // recreate the geometry
        args->thisVoxelSystem->updateNodeInArrays(voxel, false, true); // DONT_REUSE_INDEX, FORCE_REDRAW
    }

    return true; // keep recursing!
}


// TODO: other than cleanupRemovedVoxels() is there anyplace we attempt to detect too many abandoned slots???
void VoxelSystem::recreateVoxelGeometryInView() {

    qDebug() << "recreateVoxelGeometryInView()...";

    recreateVoxelGeometryInViewArgs args(this);
    _writeArraysLock.lockForWrite(); // don't let anyone read or write our write arrays until we're done
    _tree->lockForRead(); // don't let anyone change our tree structure until we're run
    
    // reset our write arrays bookkeeping to think we've got no voxels in it
    clearFreeBufferIndexes();

    // do we need to reset out _writeVoxelDirtyArray arrays??
    memset(_writeVoxelDirtyArray, false, _maxVoxels * sizeof(bool));
    
    _tree->recurseTreeWithOperation(recreateVoxelGeometryInViewOperation,(void*)&args);
    _tree->unlock();
    _writeArraysLock.unlock();
}

void VoxelSystem::checkForCulling() {
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), "checkForCulling()");
    quint64 start = usecTimestampNow();

    // track how long its been since we were last moving. If we have recently moved then only use delta frustums, if
    // it's been a long time since we last moved, then go ahead and do a full frustum cull.
    if (isViewChanging()) {
        _lastViewIsChanging = start;
    }
    quint64 sinceLastMoving = (start - _lastViewIsChanging) / 1000;
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

    // This would be a good place to do a special processing pass, for example, switching the LOD of the scene
    bool fullRedraw = (_lastKnownVoxelSizeScale != Menu::getInstance()->getVoxelSizeScale() || 
                        _lastKnownBoundaryLevelAdjust != Menu::getInstance()->getBoundaryLevelAdjust());

    _lastKnownVoxelSizeScale = Menu::getInstance()->getVoxelSizeScale();
    _lastKnownBoundaryLevelAdjust = Menu::getInstance()->getBoundaryLevelAdjust();

    if (fullRedraw) {
        // this will remove all old geometry and recreate the correct geometry for all in view voxels
        recreateVoxelGeometryInView();
    } else {
        hideOutOfView(forceFullFrustum);
    }

    if (forceFullFrustum) {
        quint64 endViewCulling = usecTimestampNow();
        _lastViewCullingElapsed = (endViewCulling - start) / 1000;
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
    // Depending on if we're using per vertex normals, we will need more or less vertex points per voxel
    int vertexPointsPerVoxel = GLOBAL_NORMALS_VERTEX_POINTS_PER_VOXEL;

    GLsizeiptr segmentSizeBytes = segmentLength * vertexPointsPerVoxel * sizeof(GLfloat);
    GLfloat* readVerticesAt     = _readVerticesArray  + (segmentStart * vertexPointsPerVoxel);
    GLfloat* writeVerticesAt    = _writeVerticesArray + (segmentStart * vertexPointsPerVoxel);
    memcpy(readVerticesAt, writeVerticesAt, segmentSizeBytes);

    segmentSizeBytes        = segmentLength * vertexPointsPerVoxel * sizeof(GLubyte);
    GLubyte* readColorsAt   = _readColorsArray   + (segmentStart * vertexPointsPerVoxel);
    GLubyte* writeColorsAt  = _writeColorsArray  + (segmentStart * vertexPointsPerVoxel);
    memcpy(readColorsAt, writeColorsAt, segmentSizeBytes);
}

void VoxelSystem::copyWrittenDataToReadArrays(bool fullVBOs) {
    static unsigned int lockForReadAttempt = 0;
    static unsigned int lockForWriteAttempt = 0;
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                            "copyWrittenDataToReadArrays()");

    // attempt to get the writeArraysLock for reading and the readArraysLock for writing 
    // so we can copy from the write to the read...  if we fail, that's ok, we'll get it the next
    // time around, the only side effect is the VBOs won't be updated this frame
    const int WAIT_FOR_LOCK_IN_MS = 5;
    if (_readArraysLock.tryLockForWrite(WAIT_FOR_LOCK_IN_MS)) {
        lockForWriteAttempt = 0;
        if (_writeArraysLock.tryLockForRead(WAIT_FOR_LOCK_IN_MS)) {
            lockForReadAttempt = 0;
            if (_voxelsDirty && _voxelsUpdated) {
                if (fullVBOs) {
                    copyWrittenDataToReadArraysFullVBOs();
                } else {
                    copyWrittenDataToReadArraysPartialVBOs();
                }
            }
            _writeArraysLock.unlock();
        } else {
            lockForReadAttempt++;
            // only report error of first failure
            if (lockForReadAttempt == 1) {
                qDebug() << "couldn't get _writeArraysLock.LockForRead()...";
            }
        }
        _readArraysLock.unlock();
    } else {
        lockForWriteAttempt++;
        // only report error of first failure
        if (lockForWriteAttempt == 1) {
            qDebug() << "couldn't get _readArraysLock.LockForWrite()...";
        }
    }
}

int VoxelSystem::newTreeToArrays(VoxelTreeElement* voxel) {
    int   voxelsUpdated   = 0;
    bool  shouldRender    = false; // assume we don't need to render it
    // if it's colored, we might need to render it!
    float voxelSizeScale = Menu::getInstance()->getVoxelSizeScale();;
    int boundaryLevelAdjust = Menu::getInstance()->getBoundaryLevelAdjust();
    shouldRender = voxel->calculateShouldRender(_viewFrustum, voxelSizeScale, boundaryLevelAdjust);

    voxel->setShouldRender(shouldRender);
    // let children figure out their renderness
    if (!voxel->isLeaf()) {

        // As we check our children, see if any of them went from shouldRender to NOT shouldRender
        // then we probably dropped LOD and if we don't have color, we want to average our children
        // for a new color.
        int childrenGotHiddenCount = 0;
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            VoxelTreeElement* childVoxel = voxel->getChildAtIndex(i);
            if (childVoxel) {
                bool wasShouldRender = childVoxel->getShouldRender();
                voxelsUpdated += newTreeToArrays(childVoxel);
                bool isShouldRender = childVoxel->getShouldRender();
                if (wasShouldRender && !isShouldRender) {
                    childrenGotHiddenCount++;
                }
            }
        }
        if (childrenGotHiddenCount > 0) {
            voxel->calculateAverageFromChildren();
        }
    }

    // update their geometry in the array. depending on our over all mode (fullVBO or not) we will reuse or not reuse the index
    if (_writeRenderFullVBO) {
        const bool DONT_REUSE_INDEX = false;
        const bool FORCE_REDRAW = true;
        voxelsUpdated += updateNodeInArrays(voxel, DONT_REUSE_INDEX, FORCE_REDRAW);
    } else {
        const bool REUSE_INDEX = true;
        const bool DONT_FORCE_REDRAW = false;
        voxelsUpdated += updateNodeInArrays(voxel, REUSE_INDEX, DONT_FORCE_REDRAW);
    }
    voxel->clearDirtyBit(); // clear the dirty bit, do this before we potentially delete things.

    return voxelsUpdated;
}

// called as response to elementDeleted() in fast pipeline case. The node
// is being deleted, but it's state is such that it thinks it should render
// and therefore we can't use the normal render calculations. This method
// will forcibly remove it from the VBOs because we know better!!!
int VoxelSystem::forceRemoveNodeFromArrays(VoxelTreeElement* node) {

    if (!_initialized) {
        return 0;
    }

    // if the node is not in the VBOs then we have nothing to do!
    if (node->isKnownBufferIndex()) {
        // If this node has not yet been written to the array, then add it to the end of the array.
        glBufferIndex nodeIndex = node->getBufferIndex();
        node->setBufferIndex(GLBUFFER_INDEX_UNKNOWN);
        freeBufferIndex(nodeIndex); // NOTE: This will make the node invisible!
        return 1; // updated!
    }
    return 0; // not-updated
}

int VoxelSystem::updateNodeInArrays(VoxelTreeElement* node, bool reuseIndex, bool forceDraw) {
    // If we've run out of room, then just bail...
    if (_voxelsInWriteArrays >= _maxVoxels && (_freeIndexes.size() == 0)) {
        // We need to think about what else we can do in this case. This basically means that all of our available
        // VBO slots are used up, but we're trying to render more voxels. At this point, if this happens we'll just
        // not render these Voxels. We need to think about ways to keep the entire scene intact but maybe lower quality
        // possibly shifting down to lower LOD or something. This debug message is to help identify, if/when/how this
        // state actually occurs.
        if (Application::getInstance()->getLogger()->extraDebugging()) {
            qDebug("OH NO! updateNodeInArrays() BAILING (_voxelsInWriteArrays >= _maxVoxels)");
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
    
    if (_initialized && nodeIndex <= _maxVoxels) {
        _writeVoxelDirtyArray[nodeIndex] = true;
        
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

glm::vec3 VoxelSystem::computeVoxelVertex(const glm::vec3& startVertex, float voxelScale, int index) const {
    const float* identityVertex = identityVertices + index * 3;
    return startVertex + glm::vec3(identityVertex[0], identityVertex[1], identityVertex[2]) * voxelScale;
}

ProgramObject VoxelSystem::_program;
ProgramObject VoxelSystem::_perlinModulateProgram;

void VoxelSystem::init() {
    if (_initialized) {
        qDebug("[ERROR] VoxelSystem is already initialized.");
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

}

void VoxelSystem::changeTree(VoxelTree* newTree) {
    _tree = newTree;

    _tree->setDirtyBit();
    _tree->getRoot()->setVoxelSystem(this);

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

        // attempt to lock the read arrays, to for copying from them to the actual GPU VBOs.
        // if we fail to get the lock, that's ok, our VBOs will update on the next frame...
        const int WAIT_FOR_LOCK_IN_MS = 5;
        if (_readArraysLock.tryLockForRead(WAIT_FOR_LOCK_IN_MS)) {
            if (_readRenderFullVBO) {
                updateFullVBOs();
            } else {
                updatePartialVBOs();
            }
            _voxelsDirty = false;
            _readRenderFullVBO = false;
            _readArraysLock.unlock();
        } else {
            qDebug() << "updateVBOs().... couldn't get _readArraysLock.tryLockForRead()";
        }
    }
    _callsToTreesToArrays = 0; // clear it
}

// this should only be called on the main application thread during render
void VoxelSystem::updateVBOSegment(glBufferIndex segmentStart, glBufferIndex segmentEnd) {
    bool showWarning = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarning, "updateVBOSegment()");

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

void VoxelSystem::render() {
    bool texture = Menu::getInstance()->isOptionChecked(MenuOption::VoxelTextures);
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "render()");

    // If we got here and we're not initialized then bail!
    if (!_initialized) {
        return;
    }

    updateVBOs();

    if (_drawHaze) {
        glEnable(GL_FOG);
    }

    {
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

            // for performance, enable backface culling and disable blending
            glEnable(GL_CULL_FACE);
            glDisable(GL_BLEND);

            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        }

        // draw voxels in 6 passes

        {
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
            glEnable(GL_BLEND);

            removeScaleAndReleaseProgram(texture);

            // deactivate vertex and color arrays after drawing
            glDisableClientState(GL_VERTEX_ARRAY);
            glDisableClientState(GL_COLOR_ARRAY);

            // bind with 0 to switch back to normal operation
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }
    }
    
    if (_drawHaze) {
        glDisable(GL_FOG);
    }
}

void VoxelSystem::applyScaleAndBindProgram(bool texture) {
    if (texture) {
        bindPerlinModulateProgram();
        glBindTexture(GL_TEXTURE_2D, TextureCache::getInstance()->getPermutationNormalTextureID());
    } else {
        _program.bind();
    }

    glPushMatrix();
    glScalef(_treeScale, _treeScale, _treeScale);
    
    TextureCache::getInstance()->setPrimaryDrawBuffers(true, true);
}

void VoxelSystem::removeScaleAndReleaseProgram(bool texture) {
    // scale back down to 1 so heads aren't massive
    glPopMatrix();

    if (texture) {
        _perlinModulateProgram.release();
        glBindTexture(GL_TEXTURE_2D, 0);
    } else {
        _program.release();
    }
    
    TextureCache::getInstance()->setPrimaryDrawBuffers(true, false);
}

int VoxelSystem::_nodeCount = 0;

void VoxelSystem::killLocalVoxels() {
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), 
                            "VoxelSystem::killLocalVoxels()");
    _tree->lockForWrite();
    VoxelSystem* voxelSystem = _tree->getRoot()->getVoxelSystem();
    _tree->eraseAllOctreeElements();
    _tree->getRoot()->setVoxelSystem(voxelSystem);
    _tree->unlock();
    clearFreeBufferIndexes();
    _voxelsInReadArrays = 0; // do we need to do this?
    setupNewVoxelsForDrawing();
}

// only called on main thread
bool VoxelSystem::clearAllNodesBufferIndexOperation(OctreeElement* element, void* extraData) {
    _nodeCount++;
    VoxelTreeElement* voxel = (VoxelTreeElement*)element;
    voxel->setBufferIndex(GLBUFFER_INDEX_UNKNOWN);
    return true;
}

// only called on main thread, and also always followed by a call to cleanupVoxelMemory()
// you shouldn't be calling this on any other thread or without also cleaning up voxel memory
void VoxelSystem::clearAllNodesBufferIndex() {
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), 
                            "VoxelSystem::clearAllNodesBufferIndex()");
    _nodeCount = 0;
    _tree->lockForRead(); // we won't change the tree so it's ok to treat this as a read
    _tree->recurseTreeWithOperation(clearAllNodesBufferIndexOperation);
    clearFreeBufferIndexes(); // this should be called too
    _tree->unlock();
    if (Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings)) {
        qDebug("clearing buffer index of %d nodes", _nodeCount);
    }
}

bool VoxelSystem::forceRedrawEntireTreeOperation(OctreeElement* element, void* extraData) {
    _nodeCount++;
    element->setDirtyBit();
    return true;
}

void VoxelSystem::forceRedrawEntireTree() {
    _nodeCount = 0;
    _tree->recurseTreeWithOperation(forceRedrawEntireTreeOperation);
    qDebug("forcing redraw of %d nodes", _nodeCount);
    _tree->setDirtyBit();
    setupNewVoxelsForDrawing();
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
    unsigned long nodesShown;

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
        nodesOutsideOutside(0),
        nodesShown(0)
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
    bool wantDeltaFrustums = !forceFullFrustum;
    hideOutOfViewArgs args(this, this->_tree, _culledOnce, widenFrustum, wantDeltaFrustums);

    const bool wantViewFrustumDebugging = false; // change to true for additional debugging
    if (wantViewFrustumDebugging) {
        args.thisViewFrustum.printDebugDetails();
        if (_culledOnce) {
            args.lastViewFrustum.printDebugDetails();
        }
    }

    if (!forceFullFrustum && _culledOnce && args.lastViewFrustum.isVerySimilar(args.thisViewFrustum)) {
        _inhideOutOfView = false;
        return;
    }

    {
        PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), 
                            "VoxelSystem::... recurseTreeWithOperation(hideOutOfViewOperation)");
        _tree->lockForRead();
        _tree->recurseTreeWithOperation(hideOutOfViewOperation,(void*)&args);
        _tree->unlock();
    }
    _lastCulledViewFrustum = args.thisViewFrustum; // save last stable
    _culledOnce = true;

    if (args.nodesRemoved) {
        _tree->setDirtyBit();
        setupNewVoxelsForDrawingSingleNode(DONT_BAIL_EARLY);
    }

    bool extraDebugDetails = false; // Application::getInstance()->getLogger()->extraDebugging();
    if (extraDebugDetails) {
        qDebug("hideOutOfView() scanned=%ld removed=%ld show=%ld inside=%ld intersect=%ld outside=%ld",
                args.nodesScanned, args.nodesRemoved, args.nodesShown, args.nodesInside,
                args.nodesIntersect, args.nodesOutside
            );
        qDebug("inside/inside=%ld intersect/inside=%ld outside/outside=%ld",
                args.nodesInsideInside, args.nodesIntersectInside, args.nodesOutsideOutside
            );

        qDebug() << "args.thisViewFrustum....";
        args.thisViewFrustum.printDebugDetails();
    }
    _inhideOutOfView = false;
}

bool VoxelSystem::hideAllSubTreeOperation(OctreeElement* element, void* extraData) {
    VoxelTreeElement* voxel = (VoxelTreeElement*)element;
    hideOutOfViewArgs* args = (hideOutOfViewArgs*)extraData;
    
    // If we've culled at least once, then we will use the status of this voxel in the last culled frustum to determine
    // how to proceed. If we've never culled, then we just consider all these voxels to be UNKNOWN so that we will not
    // consider that case.
    ViewFrustum::location inLastCulledFrustum;

    if (args->culledOnce && args->wantDeltaFrustums) {
        inLastCulledFrustum = voxel->inFrustum(args->lastViewFrustum);

        // if this node is fully OUTSIDE our last culled view frustum, then we don't need to recurse further
        if (inLastCulledFrustum == ViewFrustum::OUTSIDE) {
            args->nodesOutsideOutside++;
            return false;
        }
    }

    args->nodesOutside++;
    if (voxel->isKnownBufferIndex()) {
        args->nodesRemoved++;
        VoxelSystem* thisVoxelSystem = args->thisVoxelSystem;
        thisVoxelSystem->_voxelsUpdated += thisVoxelSystem->forceRemoveNodeFromArrays(voxel);
        thisVoxelSystem->setupNewVoxelsForDrawingSingleNode();
    }

    return true;
}

bool VoxelSystem::showAllSubTreeOperation(OctreeElement* element, void* extraData) {
    VoxelTreeElement* voxel = (VoxelTreeElement*)element;
    hideOutOfViewArgs* args = (hideOutOfViewArgs*)extraData;
    
    // If we've culled at least once, then we will use the status of this voxel in the last culled frustum to determine
    // how to proceed. If we've never culled, then we just consider all these voxels to be UNKNOWN so that we will not
    // consider that case.
    if (args->culledOnce && args->wantDeltaFrustums) {
        ViewFrustum::location inLastCulledFrustum = voxel->inFrustum(args->lastViewFrustum);

        // if this node is fully inside our last culled view frustum, then we don't need to recurse further
        if (inLastCulledFrustum == ViewFrustum::INSIDE) {
            args->nodesInsideInside++;
            return false;
        }
    }

    args->nodesInside++;

    float voxelSizeScale = Menu::getInstance()->getVoxelSizeScale();
    int boundaryLevelAdjust = Menu::getInstance()->getBoundaryLevelAdjust();
    bool shouldRender = voxel->calculateShouldRender(&args->thisViewFrustum, voxelSizeScale, boundaryLevelAdjust);
    voxel->setShouldRender(shouldRender);

    if (shouldRender && !voxel->isKnownBufferIndex()) {
        // These are both needed to force redraw...
        voxel->setDirtyBit();
        voxel->markWithChangedTime();
        args->nodesShown++;
    }

    return true; // keep recursing!
}

// "hide" voxels in the VBOs that are still in the tree that but not in view.
// We don't remove them from the tree, we don't delete them, we do remove them
// from the VBOs and mark them as such in the tree.
bool VoxelSystem::hideOutOfViewOperation(OctreeElement* element, void* extraData) {
    VoxelTreeElement* voxel = (VoxelTreeElement*)element;
    hideOutOfViewArgs* args = (hideOutOfViewArgs*)extraData;
    
    // If we're still recursing the tree using this operator, then we don't know if we're inside or outside...
    // so before we move forward we need to determine our frustum location
    ViewFrustum::location inFrustum = voxel->inFrustum(args->thisViewFrustum);

    // If we've culled at least once, then we will use the status of this voxel in the last culled frustum to determine
    // how to proceed. If we've never culled, then we just consider all these voxels to be UNKNOWN so that we will not
    // consider that case.
    ViewFrustum::location inLastCulledFrustum = ViewFrustum::OUTSIDE; // assume outside, but should get reset to actual value

    if (args->culledOnce && args->wantDeltaFrustums) {
        inLastCulledFrustum = voxel->inFrustum(args->lastViewFrustum);
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
            args->tree->recurseElementWithOperation(voxel, hideAllSubTreeOperation, args );
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
            args->tree->recurseElementWithOperation(voxel, showAllSubTreeOperation, args);
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
            
            float voxelSizeScale = Menu::getInstance()->getVoxelSizeScale();
            int boundaryLevelAdjust = Menu::getInstance()->getBoundaryLevelAdjust();
            bool shouldRender = voxel->calculateShouldRender(&args->thisViewFrustum, voxelSizeScale, boundaryLevelAdjust);
            voxel->setShouldRender(shouldRender);
            
            if (voxel->getShouldRender() && !voxel->isKnownBufferIndex()) {
                voxel->setDirtyBit(); // will this make it draw?
                voxel->markWithChangedTime(); // both are needed to force redraw
                args->nodesShown++;
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


void VoxelSystem::nodeAdded(SharedNodePointer node) {
    if (node->getType() == NodeType::VoxelServer) {
        qDebug("VoxelSystem... voxel server %s added...", node->getUUID().toString().toLocal8Bit().constData());
        _voxelServerCount++;
    }
}

bool VoxelSystem::killSourceVoxelsOperation(OctreeElement* element, void* extraData) {
    VoxelTreeElement* voxel = (VoxelTreeElement*)element;
    QUuid killedNodeID = *(QUuid*)extraData;
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        VoxelTreeElement* childNode = voxel->getChildAtIndex(i);
        if (childNode) {
            if (childNode->matchesSourceUUID(killedNodeID)) {
                voxel->safeDeepDeleteChildAtIndex(i);
            }
        }
    }
    return true;
}

void VoxelSystem::nodeKilled(SharedNodePointer node) {
    if (node->getType() == NodeType::VoxelServer) {
        _voxelServerCount--;
        QUuid nodeUUID = node->getUUID();
        qDebug("VoxelSystem... voxel server %s removed...", nodeUUID.toString().toLocal8Bit().constData());
    }
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

void VoxelSystem::bindPerlinModulateProgram() {
    if (!_perlinModulateProgram.isLinked()) {
        _perlinModulateProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/perlin_modulate.vert");
        _perlinModulateProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/perlin_modulate.frag");
        _perlinModulateProgram.link();

        _perlinModulateProgram.bind();
        _perlinModulateProgram.setUniformValue("permutationNormalTexture", 0);
    
    } else {
        _perlinModulateProgram.bind();
    }
}

