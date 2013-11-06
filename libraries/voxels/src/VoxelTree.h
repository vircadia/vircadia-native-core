//
//  VoxelTree.h
//  hifi
//
//  Created by Stephen Birarda on 3/13/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__VoxelTree__
#define __hifi__VoxelTree__

#include <set>
#include <SimpleMovingAverage.h>

#include "CoverageMap.h"
#include "JurisdictionMap.h"
#include "ViewFrustum.h"
#include "VoxelNode.h"
#include "VoxelNodeBag.h"
#include "VoxelSceneStats.h"
#include "VoxelEditPacketSender.h"

#include <QObject>
#include <QReadWriteLock>

// Callback function, for recuseTreeWithOperation
typedef bool (*RecurseVoxelTreeOperation)(VoxelNode* node, void* extraData);
typedef enum {GRADIENT, RANDOM, NATURAL} creationMode;

const bool NO_EXISTS_BITS         = false;
const bool WANT_EXISTS_BITS       = true;
const bool NO_COLOR               = false;
const bool WANT_COLOR             = true;
const bool COLLAPSE_EMPTY_TREE    = true;
const bool DONT_COLLAPSE          = false;
const bool NO_OCCLUSION_CULLING   = false;
const bool WANT_OCCLUSION_CULLING = true;

const int DONT_CHOP              = 0;
const int NO_BOUNDARY_ADJUST     = 0;
const int LOW_RES_MOVING_ADJUST  = 1;
const uint64_t IGNORE_LAST_SENT  = 0;

#define IGNORE_SCENE_STATS       NULL
#define IGNORE_VIEW_FRUSTUM      NULL
#define IGNORE_COVERAGE_MAP      NULL
#define IGNORE_JURISDICTION_MAP  NULL

class EncodeBitstreamParams {
public:
    int maxEncodeLevel;
    int maxLevelReached;
    const ViewFrustum* viewFrustum;
    bool includeColor;
    bool includeExistsBits;
    int chopLevels;
    bool deltaViewFrustum;
    const ViewFrustum* lastViewFrustum;
    bool wantOcclusionCulling;
    int boundaryLevelAdjust;
    float voxelSizeScale;
    uint64_t lastViewFrustumSent;
    bool forceSendScene;
    VoxelSceneStats* stats;
    CoverageMap* map;
    JurisdictionMap* jurisdictionMap;
    
    EncodeBitstreamParams(
        int maxEncodeLevel = INT_MAX, 
        const ViewFrustum* viewFrustum = IGNORE_VIEW_FRUSTUM,
        bool includeColor = WANT_COLOR, 
        bool includeExistsBits = WANT_EXISTS_BITS,
        int  chopLevels = 0, 
        bool deltaViewFrustum = false, 
        const ViewFrustum* lastViewFrustum = IGNORE_VIEW_FRUSTUM,
        bool wantOcclusionCulling = NO_OCCLUSION_CULLING,
        CoverageMap* map = IGNORE_COVERAGE_MAP,
        int boundaryLevelAdjust = NO_BOUNDARY_ADJUST,
        float voxelSizeScale = DEFAULT_VOXEL_SIZE_SCALE,
        uint64_t lastViewFrustumSent = IGNORE_LAST_SENT,
        bool forceSendScene = true,
        VoxelSceneStats* stats = IGNORE_SCENE_STATS,
        JurisdictionMap* jurisdictionMap = IGNORE_JURISDICTION_MAP) :
            maxEncodeLevel(maxEncodeLevel),
            maxLevelReached(0),
            viewFrustum(viewFrustum),
            includeColor(includeColor),
            includeExistsBits(includeExistsBits),
            chopLevels(chopLevels),
            deltaViewFrustum(deltaViewFrustum),
            lastViewFrustum(lastViewFrustum),
            wantOcclusionCulling(wantOcclusionCulling),
            boundaryLevelAdjust(boundaryLevelAdjust),
            voxelSizeScale(voxelSizeScale),
            lastViewFrustumSent(lastViewFrustumSent),
            forceSendScene(forceSendScene),
            stats(stats),
            map(map),
            jurisdictionMap(jurisdictionMap)
    {}
};

class ReadBitstreamToTreeParams {
public:
    bool includeColor;
    bool includeExistsBits;
    VoxelNode* destinationNode;
    QUuid sourceUUID;
    bool wantImportProgress;
    
    ReadBitstreamToTreeParams(
        bool includeColor = WANT_COLOR, 
        bool includeExistsBits = WANT_EXISTS_BITS,
        VoxelNode* destinationNode = NULL,
        QUuid sourceUUID = QUuid(),
        bool wantImportProgress = false) :
            includeColor(includeColor),
            includeExistsBits(includeExistsBits),
            destinationNode(destinationNode),
            sourceUUID(sourceUUID),
            wantImportProgress(wantImportProgress)
    {}
};

class VoxelTree : public QObject {
    Q_OBJECT
public:
    // when a voxel is created in the tree (object new'd)
    long voxelsCreated;
    // when a voxel is colored/set in the tree (object may have already existed)
    long voxelsColored;
    long voxelsBytesRead;

    SimpleMovingAverage voxelsCreatedStats;
    SimpleMovingAverage voxelsColoredStats;
    SimpleMovingAverage voxelsBytesReadStats;

    VoxelTree(bool shouldReaverage = false);
    ~VoxelTree();

    VoxelNode* rootNode;
    int leavesWrittenToBitstream;

    void eraseAllVoxels();

    void processRemoveVoxelBitstream(unsigned char* bitstream, int bufferSizeBytes);
    void readBitstreamToTree(unsigned char* bitstream,  unsigned long int bufferSizeBytes, ReadBitstreamToTreeParams& args);
    void readCodeColorBufferToTree(unsigned char* codeColorBuffer, bool destructive = false);
    void deleteVoxelCodeFromTree(unsigned char* codeBuffer, bool collapseEmptyTrees = DONT_COLLAPSE);
    void printTreeForDebugging(VoxelNode* startNode);
    void reaverageVoxelColors(VoxelNode* startNode);

    void deleteVoxelAt(float x, float y, float z, float s);
    VoxelNode* getVoxelAt(float x, float y, float z, float s) const;
    void createVoxel(float x, float y, float z, float s, 
                     unsigned char red, unsigned char green, unsigned char blue, bool destructive = false);
    void createLine(glm::vec3 point1, glm::vec3 point2, float unitSize, rgbColor color, bool destructive = false);
    void createSphere(float r,float xc, float yc, float zc, float s, bool solid, 
                      creationMode mode, bool destructive = false, bool debug = false);

    void recurseTreeWithOperation(RecurseVoxelTreeOperation operation, void* extraData=NULL);
                                    
    void recurseTreeWithOperationDistanceSorted(RecurseVoxelTreeOperation operation, 
                                                const glm::vec3& point, void* extraData=NULL);

    int encodeTreeBitstream(VoxelNode* node, unsigned char* outputBuffer, int availableBytes, VoxelNodeBag& bag, 
                            EncodeBitstreamParams& params) ;

    bool isDirty() const { return _isDirty; }
    void clearDirtyBit() { _isDirty = false; }
    void setDirtyBit() { _isDirty = true; }
    unsigned long int getNodesChangedFromBitstream() const { return _nodesChangedFromBitstream; }

    bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                             VoxelNode*& node, float& distance, BoxFace& face);

    bool findSpherePenetration(const glm::vec3& center, float radius, glm::vec3& penetration);
    bool findCapsulePenetration(const glm::vec3& start, const glm::vec3& end, float radius, glm::vec3& penetration);

    // Note: this assumes the fileFormat is the HIO individual voxels code files
    void loadVoxelsFile(const char* fileName, bool wantColorRandomizer);

    // these will read/write files that match the wireformat, excluding the 'V' leading
    void writeToSVOFile(const char* filename, VoxelNode* node = NULL);
    bool readFromSVOFile(const char* filename);
    // reads voxels from square image with alpha as a Y-axis
    bool readFromSquareARGB32Pixels(const char *filename);
    bool readFromSchematicFile(const char* filename);
    
    // VoxelTree does not currently handle its own locking, caller must use these to lock/unlock
    void lockForRead() { lock.lockForRead(); }
    void lockForWrite() { lock.lockForWrite(); }
    void unlock() { lock.unlock(); }

    unsigned long getVoxelCount();

    void copySubTreeIntoNewTree(VoxelNode* startNode, VoxelTree* destinationTree, bool rebaseToRoot);
    void copyFromTreeIntoSubTree(VoxelTree* sourceTree, VoxelNode* destinationNode);
    
    bool getShouldReaverage() const { return _shouldReaverage; }

    void recurseNodeWithOperation(VoxelNode* node, RecurseVoxelTreeOperation operation, 
                void* extraData, int recursionCount = 0);
            
    void recurseNodeWithOperationDistanceSorted(VoxelNode* node, RecurseVoxelTreeOperation operation, 
                const glm::vec3& point, void* extraData, int recursionCount = 0);

    void nudgeSubTree(VoxelNode* nodeToNudge, const glm::vec3& nudgeAmount, VoxelEditPacketSender& voxelEditSender);

signals:
    void importSize(float x, float y, float z);
    void importProgress(int progress);

public slots:
    void cancelImport();


private:
    void deleteVoxelCodeFromTreeRecursion(VoxelNode* node, void* extraData);
    void readCodeColorBufferToTreeRecursion(VoxelNode* node, void* extraData);

    int encodeTreeBitstreamRecursion(VoxelNode* node, unsigned char* outputBuffer, int availableBytes, VoxelNodeBag& bag, 
                                     EncodeBitstreamParams& params, int& currentEncodeLevel) const;

    static bool countVoxelsOperation(VoxelNode* node, void* extraData);

    VoxelNode* nodeForOctalCode(VoxelNode* ancestorNode, const unsigned char* needleCode, VoxelNode** parentOfFoundNode) const;
    VoxelNode* createMissingNode(VoxelNode* lastParentNode, unsigned char* deepestCodeToCreate);
    int readNodeData(VoxelNode *destinationNode, unsigned char* nodeData, int bufferSizeBytes, ReadBitstreamToTreeParams& args);
    
    bool _isDirty;
    unsigned long int _nodesChangedFromBitstream;
    bool _shouldReaverage;
    bool _stopImport;

    /// Octal Codes of any subtrees currently being encoded. While any of these codes is being encoded, ancestors and 
    /// descendants of them can not be deleted.
    std::set<const unsigned char*>  _codesBeingEncoded;
    /// mutex lock to protect the encoding set
    pthread_mutex_t _encodeSetLock;

    /// Called to indicate that a VoxelNode is in the process of being encoded.
    void startEncoding(VoxelNode* node);
    /// Called to indicate that a VoxelNode is done being encoded.
    void doneEncoding(VoxelNode* node);
    /// Is the Octal Code currently being deleted?
    bool isEncoding(unsigned char* codeBuffer);

    /// Octal Codes of any subtrees currently being deleted. While any of these codes is being deleted, ancestors and 
    /// descendants of them can not be encoded.
    std::set<unsigned char*>  _codesBeingDeleted;
    /// mutex lock to protect the deleting set
    pthread_mutex_t _deleteSetLock;

    /// Called to indicate that an octal code is in the process of being deleted.
    void startDeleting(unsigned char* code);
    /// Called to indicate that an octal code is done being deleted.
    void doneDeleting(unsigned char* code);
    /// Octal Codes that were attempted to be deleted but couldn't be because they were actively being encoded, and were
    /// instead queued for later delete
    std::set<unsigned char*>  _codesPendingDelete;
    /// mutex lock to protect the deleting set
    pthread_mutex_t _deletePendingSetLock;

    /// Adds an Octal Code to the set of codes that needs to be deleted
    void queueForLaterDelete(unsigned char* codeBuffer);
    /// flushes out any Octal Codes that had to be queued
    void emptyDeleteQueue();

    // helper functions for nudgeSubTree
    void recurseNodeForNudge(VoxelNode* node, RecurseVoxelTreeOperation operation, void* extraData);
    static bool nudgeCheck(VoxelNode* node, void* extraData);
    void nudgeLeaf(VoxelNode* node, void* extraData);
    void chunkifyLeaf(VoxelNode* node);
    
    QReadWriteLock lock;
};

float boundaryDistanceForRenderLevel(unsigned int renderLevel, float voxelSizeScale);

#endif /* defined(__hifi__VoxelTree__) */
