//
//  Octree.h
//  hifi
//
//  Created by Stephen Birarda on 3/13/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__Octree__
#define __hifi__Octree__

#include <set>
#include <SimpleMovingAverage.h>

//#include "CoverageMap.h"
class CoverageMap;
class ReadBitstreamToTreeParams;
class Octree;
class OctreeElement;
class OctreeElementBag;
class OctreePacketData;


#include "JurisdictionMap.h"
#include "ViewFrustum.h"
#include "OctreeElement.h"
#include "OctreeElementBag.h"
#include "OctreePacketData.h"
#include "OctreeSceneStats.h"

#include <QObject>
#include <QReadWriteLock>

// Callback function, for recuseTreeWithOperation
typedef bool (*RecurseOctreeOperation)(OctreeElement* node, void* extraData);
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
    float octreeElementSizeScale;
    uint64_t lastViewFrustumSent;
    bool forceSendScene;
    OctreeSceneStats* stats;
    CoverageMap* map;
    JurisdictionMap* jurisdictionMap;
    
    // output hints from the encode process
    typedef enum {
        UNKNOWN, 
        DIDNT_FIT, 
        NULL_NODE, 
        TOO_DEEP,
        OUT_OF_JURISDICTION,
        LOD_SKIP,
        OUT_OF_VIEW,
        WAS_IN_VIEW,
        NO_CHANGE,
        OCCLUDED
    } reason;
    reason stopReason;
    
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
        float octreeElementSizeScale = DEFAULT_OCTREE_SIZE_SCALE,
        uint64_t lastViewFrustumSent = IGNORE_LAST_SENT,
        bool forceSendScene = true,
        OctreeSceneStats* stats = IGNORE_SCENE_STATS,
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
            octreeElementSizeScale(octreeElementSizeScale),
            lastViewFrustumSent(lastViewFrustumSent),
            forceSendScene(forceSendScene),
            stats(stats),
            map(map),
            jurisdictionMap(jurisdictionMap),
            stopReason(UNKNOWN)
    {}
    
    void displayStopReason() {
        printf("StopReason: ");
        switch (stopReason) {
            default:
            case UNKNOWN: printf("UNKNOWN\n"); break;
            
            case DIDNT_FIT: printf("DIDNT_FIT\n"); break;
            case NULL_NODE: printf("NULL_NODE\n"); break;
            case TOO_DEEP: printf("TOO_DEEP\n"); break;
            case OUT_OF_JURISDICTION: printf("OUT_OF_JURISDICTION\n"); break;
            case LOD_SKIP: printf("LOD_SKIP\n"); break;
            case OUT_OF_VIEW: printf("OUT_OF_VIEW\n"); break;
            case WAS_IN_VIEW: printf("WAS_IN_VIEW\n"); break;
            case NO_CHANGE: printf("NO_CHANGE\n"); break;
            case OCCLUDED: printf("OCCLUDED\n"); break;
        }
    }
};

class ReadElementBufferToTreeArgs {
public:
    const unsigned char* buffer;
    int length;
    bool destructive;
    bool pathChanged;
};

class ReadBitstreamToTreeParams {
public:
    bool includeColor;
    bool includeExistsBits;
    OctreeElement* destinationNode;
    QUuid sourceUUID;
    bool wantImportProgress;
    
    ReadBitstreamToTreeParams(
        bool includeColor = WANT_COLOR, 
        bool includeExistsBits = WANT_EXISTS_BITS,
        OctreeElement* destinationNode = NULL,
        QUuid sourceUUID = QUuid(),
        bool wantImportProgress = false) :
            includeColor(includeColor),
            includeExistsBits(includeExistsBits),
            destinationNode(destinationNode),
            sourceUUID(sourceUUID),
            wantImportProgress(wantImportProgress)
    {}
};

class Octree : public QObject {
    Q_OBJECT
public:
    Octree(bool shouldReaverage = false);
    ~Octree();
    
    /// Your tree class must implement this to create the correct element type
    virtual OctreeElement* createNewElement(unsigned char * octalCode = NULL) const = 0;

    // These methods will allow the OctreeServer to send your tree inbound edit packets of your
    // own definition. Implement these to allow your octree based server to support editing
    virtual bool handlesEditPacketType(PACKET_TYPE packetType) const { return false; }
    virtual int processEditPacketData(PACKET_TYPE packetType, unsigned char* packetData, int packetLength,
                    unsigned char* editData, int maxLength, Node* senderNode) { return 0; }


    virtual void update() { }; // nothing to do by default

    OctreeElement* getRoot() { return _rootNode; }

    void eraseAllOctreeElements();

    void processRemoveOctreeElementsBitstream(const unsigned char* bitstream, int bufferSizeBytes);
    void readBitstreamToTree(const unsigned char* bitstream,  unsigned long int bufferSizeBytes, ReadBitstreamToTreeParams& args);
    void deleteOctalCodeFromTree(const unsigned char* codeBuffer, bool collapseEmptyTrees = DONT_COLLAPSE);
    void reaverageOctreeElements(OctreeElement* startNode = NULL);

    void deleteOctreeElementAt(float x, float y, float z, float s);
    OctreeElement* getOctreeElementAt(float x, float y, float z, float s) const;
    OctreeElement* getOrCreateChildElementAt(float x, float y, float z, float s);

    void recurseTreeWithOperation(RecurseOctreeOperation operation, void* extraData=NULL);
                                    
    void recurseTreeWithOperationDistanceSorted(RecurseOctreeOperation operation, 
                                                const glm::vec3& point, void* extraData=NULL);

    int encodeTreeBitstream(OctreeElement* node, OctreePacketData* packetData, OctreeElementBag& bag,
                            EncodeBitstreamParams& params) ;

    bool isDirty() const { return _isDirty; }
    void clearDirtyBit() { _isDirty = false; }
    void setDirtyBit() { _isDirty = true; }

    bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                             OctreeElement*& node, float& distance, BoxFace& face);

    bool findSpherePenetration(const glm::vec3& center, float radius, glm::vec3& penetration, 
                                OctreeElement** penetratedElement = NULL);
                                
    bool findCapsulePenetration(const glm::vec3& start, const glm::vec3& end, float radius, glm::vec3& penetration);

    // Note: this assumes the fileFormat is the HIO individual voxels code files
    void loadOctreeFile(const char* fileName, bool wantColorRandomizer);

    // these will read/write files that match the wireformat, excluding the 'V' leading
    void writeToSVOFile(const char* filename, OctreeElement* node = NULL);
    bool readFromSVOFile(const char* filename);
    // reads voxels from square image with alpha as a Y-axis
    bool readFromSquareARGB32Pixels(const char *filename);
    bool readFromSchematicFile(const char* filename);
    
    // Octree does not currently handle its own locking, caller must use these to lock/unlock
    void lockForRead() { lock.lockForRead(); }
    void tryLockForRead() { lock.tryLockForRead(); }
    void lockForWrite() { lock.lockForWrite(); }
    void tryLockForWrite() { lock.tryLockForWrite(); }
    void unlock() { lock.unlock(); }

    unsigned long getOctreeElementsCount();

    void copySubTreeIntoNewTree(OctreeElement* startNode, Octree* destinationTree, bool rebaseToRoot);
    void copyFromTreeIntoSubTree(Octree* sourceTree, OctreeElement* destinationNode);
    
    bool getShouldReaverage() const { return _shouldReaverage; }

    void recurseNodeWithOperation(OctreeElement* node, RecurseOctreeOperation operation, 
                void* extraData, int recursionCount = 0);
            
    void recurseNodeWithOperationDistanceSorted(OctreeElement* node, RecurseOctreeOperation operation, 
                const glm::vec3& point, void* extraData, int recursionCount = 0);

signals:
    void importSize(float x, float y, float z);
    void importProgress(int progress);

public slots:
    void cancelImport();


protected:
    void deleteOctalCodeFromTreeRecursion(OctreeElement* node, void* extraData);

    int encodeTreeBitstreamRecursion(OctreeElement* node, 
                                     OctreePacketData* packetData, OctreeElementBag& bag,
                                     EncodeBitstreamParams& params, int& currentEncodeLevel) const;

    static bool countOctreeElementsOperation(OctreeElement* node, void* extraData);

    OctreeElement* nodeForOctalCode(OctreeElement* ancestorNode, const unsigned char* needleCode, OctreeElement** parentOfFoundNode) const;
    OctreeElement* createMissingNode(OctreeElement* lastParentNode, const unsigned char* codeToReach);
    int readNodeData(OctreeElement *destinationNode, const unsigned char* nodeData, 
                int bufferSizeBytes, ReadBitstreamToTreeParams& args);
    
    OctreeElement* _rootNode;
    
    bool _isDirty;
    bool _shouldReaverage;
    bool _stopImport;

    /// Octal Codes of any subtrees currently being encoded. While any of these codes is being encoded, ancestors and 
    /// descendants of them can not be deleted.
    std::set<const unsigned char*>  _codesBeingEncoded;
    /// mutex lock to protect the encoding set
    pthread_mutex_t _encodeSetLock;

    /// Called to indicate that a OctreeElement is in the process of being encoded.
    void startEncoding(OctreeElement* node);
    /// Called to indicate that a OctreeElement is done being encoded.
    void doneEncoding(OctreeElement* node);
    /// Is the Octal Code currently being deleted?
    bool isEncoding(const unsigned char* codeBuffer);

    /// Octal Codes of any subtrees currently being deleted. While any of these codes is being deleted, ancestors and 
    /// descendants of them can not be encoded.
    std::set<const unsigned char*> _codesBeingDeleted;
    /// mutex lock to protect the deleting set
    pthread_mutex_t _deleteSetLock;

    /// Called to indicate that an octal code is in the process of being deleted.
    void startDeleting(const unsigned char* code);
    /// Called to indicate that an octal code is done being deleted.
    void doneDeleting(const unsigned char* code);
    /// Octal Codes that were attempted to be deleted but couldn't be because they were actively being encoded, and were
    /// instead queued for later delete
    std::set<const unsigned char*> _codesPendingDelete;
    /// mutex lock to protect the deleting set
    pthread_mutex_t _deletePendingSetLock;

    /// Adds an Octal Code to the set of codes that needs to be deleted
    void queueForLaterDelete(const unsigned char* codeBuffer);
    /// flushes out any Octal Codes that had to be queued
    void emptyDeleteQueue();
    
    QReadWriteLock lock;
};

float boundaryDistanceForRenderLevel(unsigned int renderLevel, float voxelSizeScale);

#endif /* defined(__hifi__Octree__) */
