//
//  VoxelSystem.h
//  interface
//
//  Created by Philip on 12/31/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__VoxelSystem__
#define __interface__VoxelSystem__

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
#include "renderer/VoxelShader.h"

class ProgramObject;

const int NUM_CHILDREN = 8;

struct VoxelShaderVBOData
{
    float x, y, z; // position
    float s; // size
    unsigned char r,g,b; // color
};


class VoxelSystem : public NodeData, public VoxelNodeDeleteHook, public VoxelNodeUpdateHook, 
                    public NodeListHook, public DomainChangeListener {
    Q_OBJECT
public:
    VoxelSystem(float treeScale = TREE_SCALE, int maxVoxels = DEFAULT_MAX_VOXELS_PER_SYSTEM);
    ~VoxelSystem();

    void setDataSourceUUID(const QUuid& dataSourceUUID) { _dataSourceUUID = dataSourceUUID; }
    const QUuid&  getDataSourceUUID() const { return _dataSourceUUID; }
    
    int parseData(unsigned char* sourceBuffer, int numBytes);
    
    virtual void init();
    void simulate(float deltaTime) { }
    void render(bool texture);

    void changeTree(VoxelTree* newTree);
    VoxelTree* getTree() const { return _tree; }
    ViewFrustum* getViewFrustum() const { return _viewFrustum; }
    void setViewFrustum(ViewFrustum* viewFrustum) { _viewFrustum = viewFrustum; }
    unsigned long  getVoxelsUpdated() const { return _voxelsUpdated; }
    unsigned long  getVoxelsRendered() const { return _voxelsInReadArrays; }
    unsigned long  getVoxelsWritten() const { return _voxelsInWriteArrays; }
    unsigned long  getAbandonedVoxels() const { return _freeIndexes.size(); }

    ViewFrustum* getLastCulledViewFrustum() { return &_lastCulledViewFrustum; }

    void loadVoxelsFile(const char* fileName,bool wantColorRandomizer);
    void writeToSVOFile(const char* filename, VoxelNode* node) const;
    bool readFromSVOFile(const char* filename);
    bool readFromSquareARGB32Pixels(const char* filename);
    bool readFromSchematicFile(const char* filename);

    void setMaxVoxels(int maxVoxels);
    long int getMaxVoxels() const { return _maxVoxels; }
    unsigned long getVoxelMemoryUsageRAM() const { return _memoryUsageRAM; }
    unsigned long getVoxelMemoryUsageVBO() const { return _memoryUsageVBO; }
    bool hasVoxelMemoryUsageGPU() const { return _hasMemoryUsageGPU; }
    unsigned long getVoxelMemoryUsageGPU();
    long int getVoxelsCreated();
    long int getVoxelsColored();
    long int getVoxelsBytesRead();
    float getVoxelsCreatedPerSecondAverage();
    float getVoxelsColoredPerSecondAverage();
    float getVoxelsBytesReadPerSecondAverage();

    void killLocalVoxels();
    void redrawInViewVoxels();

    virtual void removeOutOfView();
    virtual void hideOutOfView(bool forceFullFrustum = false);
    bool hasViewChanged();
    bool isViewChanging();
    
    bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                             VoxelDetail& detail, float& distance, BoxFace& face);
    
    bool findSpherePenetration(const glm::vec3& center, float radius, glm::vec3& penetration);
    bool findCapsulePenetration(const glm::vec3& start, const glm::vec3& end, float radius, glm::vec3& penetration);

    void deleteVoxelAt(float x, float y, float z, float s);
    VoxelNode* getVoxelAt(float x, float y, float z, float s) const;
    void createVoxel(float x, float y, float z, float s, 
                     unsigned char red, unsigned char green, unsigned char blue, bool destructive = false);
    void createLine(glm::vec3 point1, glm::vec3 point2, float unitSize, rgbColor color, bool destructive = false);
    void createSphere(float r,float xc, float yc, float zc, float s, bool solid, 
                      creationMode mode, bool destructive = false, bool debug = false);

    void copySubTreeIntoNewTree(VoxelNode* startNode, VoxelSystem* destinationTree, bool rebaseToRoot);
    void copySubTreeIntoNewTree(VoxelNode* startNode, VoxelTree* destinationTree, bool rebaseToRoot);
    void copyFromTreeIntoSubTree(VoxelTree* sourceTree, VoxelNode* destinationNode);

    void recurseTreeWithOperation(RecurseVoxelTreeOperation operation, void* extraData=NULL);

    CoverageMapV2 myCoverageMapV2;
    CoverageMap   myCoverageMap;

    virtual void voxelDeleted(VoxelNode* node);
    virtual void voxelUpdated(VoxelNode* node);
    virtual void nodeAdded(Node* node);
    virtual void nodeKilled(Node* node);
    virtual void domainChanged(QString domain);
    
    bool treeIsBusy() const { return _treeIsBusy; }
    
signals:
    void importSize(float x, float y, float z);
    void importProgress(int progress);

public slots:
    void collectStatsForTreesAndVBOs();
    
    // Methods that recurse tree
    void showAllLocalVoxels();
    void randomizeVoxelColors();
    void falseColorizeRandom();
    void trueColorize();
    void falseColorizeInView();
    void falseColorizeDistanceFromView();
    void falseColorizeRandomEveryOther();
    void falseColorizeOccluded();
    void falseColorizeOccludedV2();
    void falseColorizeBySource();
    void forceRedrawEntireTree();
    void clearAllNodesBufferIndex();

    void cancelImport();
    
    void setDisableFastVoxelPipeline(bool disableFastVoxelPipeline);
    void setUseVoxelShader(bool useVoxelShader);
    void setVoxelsAsPoints(bool voxelsAsPoints);
        
protected:
    float _treeScale; 
    int _maxVoxels;      
    VoxelTree* _tree;

    void setupNewVoxelsForDrawing();
    static const bool DONT_BAIL_EARLY; // by default we will bail early, if you want to force not bailing, then use this
    void setupNewVoxelsForDrawingSingleNode(bool allowBailEarly = true);
    void checkForCulling();
    
    glm::vec3 computeVoxelVertex(const glm::vec3& startVertex, float voxelScale, int index) const;

    
    virtual void updateArraysDetails(glBufferIndex nodeIndex, const glm::vec3& startVertex,
                                    float voxelScale, const nodeColor& color);
    virtual void copyWrittenDataSegmentToReadArrays(glBufferIndex segmentStart, glBufferIndex segmentEnd);
    virtual void updateVBOSegment(glBufferIndex segmentStart, glBufferIndex segmentEnd);
    virtual void applyScaleAndBindProgram(bool texture);
    virtual void removeScaleAndReleaseProgram(bool texture);

private:
    // disallow copying of VoxelSystem objects
    VoxelSystem(const VoxelSystem&);
    VoxelSystem& operator= (const VoxelSystem&);
    
    bool _initialized;
    int  _callsToTreesToArrays;
    VoxelNodeBag _removedVoxels;

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
    static bool falseColorizeBySourceOperation(VoxelNode* node, void* extraData);
    static bool killSourceVoxelsOperation(VoxelNode* node, void* extraData);
    static bool forceRedrawEntireTreeOperation(VoxelNode* node, void* extraData);
    static bool clearAllNodesBufferIndexOperation(VoxelNode* node, void* extraData);
    static bool hideOutOfViewOperation(VoxelNode* node, void* extraData);
    static bool hideAllSubTreeOperation(VoxelNode* node, void* extraData);
    static bool showAllSubTreeOperation(VoxelNode* node, void* extraData);
    static bool showAllLocalVoxelsOperation(VoxelNode* node, void* extraData);

    int updateNodeInArrays(VoxelNode* node, bool reuseIndex, bool forceDraw);
    int forceRemoveNodeFromArrays(VoxelNode* node);

    void copyWrittenDataToReadArraysFullVBOs();
    void copyWrittenDataToReadArraysPartialVBOs();

    void updateVBOs();

    unsigned long getFreeMemoryGPU();

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
    uint64_t _lastViewIsChanging;
    uint64_t _lastAudit;
    int _lastViewCullingElapsed;
    bool _hasRecentlyChanged;
    
    void initVoxelMemory();
    void cleanupVoxelMemory();

    bool _useVoxelShader;
    bool _voxelsAsPoints;
    bool _voxelShaderModeWhenVoxelsAsPointsEnabled;

    GLuint _vboVoxelsID; /// when using voxel shader, we'll use this VBO
    GLuint _vboVoxelsIndicesID;  /// when using voxel shader, we'll use this VBO for our indexes
    VoxelShaderVBOData* _writeVoxelShaderData;
    VoxelShaderVBOData* _readVoxelShaderData;
    
    GLuint _vboVerticesID;
    GLuint _vboColorsID;

    GLuint _vboIndicesTop;
    GLuint _vboIndicesBottom;
    GLuint _vboIndicesLeft;
    GLuint _vboIndicesRight;
    GLuint _vboIndicesFront;
    GLuint _vboIndicesBack;

    pthread_mutex_t _bufferWriteLock;
    pthread_mutex_t _treeLock;

    ViewFrustum _lastKnownViewFrustum;
    ViewFrustum _lastStableViewFrustum;
    ViewFrustum* _viewFrustum;

    ViewFrustum _lastCulledViewFrustum; // used for hide/show visible passes
    bool _culledOnce;

    void setupFaceIndices(GLuint& faceVBOID, GLubyte faceIdentityIndices[]);

    int newTreeToArrays(VoxelNode *currentNode);
    void cleanupRemovedVoxels();

    void copyWrittenDataToReadArrays(bool fullVBOs);
    
    void updateFullVBOs(); // all voxels in the VBO
    void updatePartialVBOs(); // multiple segments, only dirty voxels

    bool _voxelsDirty;

    static ProgramObject _perlinModulateProgram;
    
    int _hookID;
    std::vector<glBufferIndex> _freeIndexes;
    pthread_mutex_t _freeIndexLock;

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
    bool _treeIsBusy; // is the tree mutex locked? if so, it's busy, and if you can avoid it, don't access the tree
    
    void lockTree();
    void unlockTree();
};

#endif
