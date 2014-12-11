//
//  VoxelSystem.h
//  interface/src/voxels
//
//  Created by Philip on 12/31/12.
//  Copyright 2012 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_VoxelSystem_h
#define hifi_VoxelSystem_h

#include "InterfaceConfig.h"
#include <glm/glm.hpp>

#include <SharedUtil.h>

#include <NodeData.h>
#include <ViewFrustum.h>
#include <VoxelTree.h>
#include <OctreePersistThread.h>

#include "Camera.h"
#include "Util.h"
#include "world.h"
#include "PrimitiveRenderer.h"

class ProgramObject;

const int NUM_CHILDREN = 8;


class VoxelSystem : public NodeData, public OctreeElementDeleteHook, public OctreeElementUpdateHook {
    Q_OBJECT

    friend class VoxelHideShowThread;

public:
    VoxelSystem(float treeScale = TREE_SCALE, int maxVoxels = DEFAULT_MAX_VOXELS_PER_SYSTEM, VoxelTree* tree = NULL);
    ~VoxelSystem();

    void setDataSourceUUID(const QUuid& dataSourceUUID) { _dataSourceUUID = dataSourceUUID; }
    const QUuid&  getDataSourceUUID() const { return _dataSourceUUID; }

    int parseData(const QByteArray& packet);

    bool isInitialized() { return _initialized; }
    virtual void init();
    void render();

    void changeTree(VoxelTree* newTree);
    VoxelTree* getTree() const { return _tree; }
    ViewFrustum* getViewFrustum() const { return _viewFrustum; }
    void setViewFrustum(ViewFrustum* viewFrustum) { _viewFrustum = viewFrustum; }
    unsigned long  getVoxelsUpdated() const { return _voxelsUpdated; }
    unsigned long  getVoxelsRendered() const { return _voxelsInReadArrays; }
    unsigned long  getVoxelsWritten() const { return _voxelsInWriteArrays; }
    unsigned long  getAbandonedVoxels() const { return _freeIndexes.size(); }

    ViewFrustum* getLastCulledViewFrustum() { return &_lastCulledViewFrustum; }

    void setMaxVoxels(unsigned long maxVoxels);
    long int getMaxVoxels() const { return _maxVoxels; }
    unsigned long getVoxelMemoryUsageRAM() const { return _memoryUsageRAM; }
    unsigned long getVoxelMemoryUsageVBO() const { return _memoryUsageVBO; }
    bool hasVoxelMemoryUsageGPU() const { return _hasMemoryUsageGPU; }
    unsigned long getVoxelMemoryUsageGPU();

    void killLocalVoxels();

    virtual void hideOutOfView(bool forceFullFrustum = false);
    void inspectForOcclusions();
    bool hasViewChanged();
    bool isViewChanging();

    virtual void elementDeleted(OctreeElement* element);
    virtual void elementUpdated(OctreeElement* element);

public slots:
    void nodeAdded(SharedNodePointer node);
    void nodeKilled(SharedNodePointer node);
                        

    // Methods that recurse tree
    void forceRedrawEntireTree();
    void clearAllNodesBufferIndex();
 
    void setDisableFastVoxelPipeline(bool disableFastVoxelPipeline);

protected:
    float _treeScale;
    unsigned long _maxVoxels;
    VoxelTree* _tree;

    void setupNewVoxelsForDrawing();
    static const bool DONT_BAIL_EARLY; // by default we will bail early, if you want to force not bailing, then use this
    void setupNewVoxelsForDrawingSingleNode(bool allowBailEarly = true);

    /// called on the hide/show thread to hide any out of view voxels and show any newly in view voxels. 
    void checkForCulling();
    
    /// single pass to remove old VBO data and fill it with correct current view, used when switching LOD or needing to force
    /// a full redraw of everything in view
    void recreateVoxelGeometryInView();

    glm::vec3 computeVoxelVertex(const glm::vec3& startVertex, float voxelScale, int index) const;

    virtual void updateArraysDetails(glBufferIndex nodeIndex, const glm::vec3& startVertex,
                                    float voxelScale, const nodeColor& color);
    virtual void copyWrittenDataSegmentToReadArrays(glBufferIndex segmentStart, glBufferIndex segmentEnd);
    virtual void updateVBOSegment(glBufferIndex segmentStart, glBufferIndex segmentEnd);

    
    virtual void applyScaleAndBindProgram(bool texture); /// used in render() to apply shadows and textures
    virtual void removeScaleAndReleaseProgram(bool texture); /// stop the shaders for shadows and textures

private:
    // disallow copying of VoxelSystem objects
    VoxelSystem(const VoxelSystem&);
    VoxelSystem& operator= (const VoxelSystem&);

    bool _initialized;
    int  _callsToTreesToArrays;

    // Operation functions for tree recursion methods
    static int _nodeCount;
    static bool killSourceVoxelsOperation(OctreeElement* element, void* extraData);
    static bool forceRedrawEntireTreeOperation(OctreeElement* element, void* extraData);
    static bool clearAllNodesBufferIndexOperation(OctreeElement* element, void* extraData);
    static bool inspectForExteriorOcclusionsOperation(OctreeElement* element, void* extraData);
    static bool inspectForInteriorOcclusionsOperation(OctreeElement* element, void* extraData);
    static bool hideOutOfViewOperation(OctreeElement* element, void* extraData);
    static bool hideAllSubTreeOperation(OctreeElement* element, void* extraData);
    static bool showAllSubTreeOperation(OctreeElement* element, void* extraData);
    static bool getVoxelEnclosingOperation(OctreeElement* element, void* extraData);
    static bool recreateVoxelGeometryInViewOperation(OctreeElement* element, void* extraData);

    int updateNodeInArrays(VoxelTreeElement* node, bool reuseIndex, bool forceDraw);
    int forceRemoveNodeFromArrays(VoxelTreeElement* node);

    void copyWrittenDataToReadArraysFullVBOs();
    void copyWrittenDataToReadArraysPartialVBOs();

    void updateVBOs();

    unsigned long getFreeMemoryGPU();

    // these are kinda hacks, used by getDistanceFromViewRangeOperation() probably shouldn't be here
    static float _maxDistance;
    static float _minDistance;

    GLfloat* _readVerticesArray;
    GLubyte* _readColorsArray;

    QReadWriteLock _writeArraysLock;
    QReadWriteLock _readArraysLock;


    GLfloat* _writeVerticesArray;
    GLubyte* _writeColorsArray;
    bool* _writeVoxelDirtyArray;
    bool* _readVoxelDirtyArray;
    unsigned long _voxelsUpdated;
    unsigned long _voxelsInReadArrays;
    unsigned long _voxelsInWriteArrays;

    bool _writeRenderFullVBO;
    bool _readRenderFullVBO;

    int _setupNewVoxelsForDrawingLastElapsed;
    quint64 _setupNewVoxelsForDrawingLastFinished;
    quint64 _lastViewCulling;
    quint64 _lastViewIsChanging;
    quint64 _lastAudit;
    int _lastViewCullingElapsed;
    bool _hasRecentlyChanged;

    void initVoxelMemory();
    void cleanupVoxelMemory();

    GLuint _vboVoxelsID; /// when using voxel shader, we'll use this VBO
    GLuint _vboVoxelsIndicesID;  /// when using voxel shader, we'll use this VBO for our indexes

    GLuint _vboVerticesID;
    GLuint _vboColorsID;

    GLuint _vboIndicesTop;
    GLuint _vboIndicesBottom;
    GLuint _vboIndicesLeft;
    GLuint _vboIndicesRight;
    GLuint _vboIndicesFront;
    GLuint _vboIndicesBack;

    ViewFrustum _lastKnownViewFrustum;
    ViewFrustum _lastStableViewFrustum;
    ViewFrustum* _viewFrustum;

    ViewFrustum _lastCulledViewFrustum; // used for hide/show visible passes
    bool _culledOnce;

    void setupFaceIndices(GLuint& faceVBOID, GLubyte faceIdentityIndices[]);

    int newTreeToArrays(VoxelTreeElement* currentNode);

    void copyWrittenDataToReadArrays(bool fullVBOs);

    void updateFullVBOs(); // all voxels in the VBO
    void updatePartialVBOs(); // multiple segments, only dirty voxels

    bool _voxelsDirty;

    static ProgramObject _program;
    static ProgramObject _perlinModulateProgram;

    static void bindPerlinModulateProgram();

    int _hookID;
    std::vector<glBufferIndex> _freeIndexes;
    QMutex _freeIndexLock;

    void freeBufferIndex(glBufferIndex index);
    void clearFreeBufferIndexes();
    glBufferIndex getNextBufferIndex();

    bool _falseColorizeBySource;
    QUuid _dataSourceUUID;

    int _voxelServerCount;
    unsigned long _memoryUsageRAM;
    unsigned long _memoryUsageVBO;
    unsigned long _initialMemoryUsageGPU;
    bool _hasMemoryUsageGPU;

    bool _inSetupNewVoxelsForDrawing;
    bool _useFastVoxelPipeline;

    bool _inhideOutOfView;

    float _lastKnownVoxelSizeScale;
    int _lastKnownBoundaryLevelAdjust;

    bool _inOcclusions;
    bool _showCulledSharedFaces;                ///< Flag visibility of culled faces
    bool _usePrimitiveRenderer;                 ///< Flag primitive renderer for use
    PrimitiveRenderer* _renderer;               ///< Voxel renderer

    static const unsigned int _sNumOctantsPerHemiVoxel = 4;
    static int _sCorrectedChildIndex[8];
    static unsigned short _sSwizzledOcclusionBits[64];          ///< Swizzle value of bit pairs of the value of index
    static unsigned char _sOctantIndexToBitMask[8];             ///< Map octant index to partition mask
    static unsigned char _sOctantIndexToSharedBitMask[8][8];    ///< Map octant indices to shared partition mask
    
    // haze
    bool _drawHaze;
    float _farHazeDistance;
    glm::vec3 _hazeColor;
};

#endif // hifi_VoxelSystem_h
