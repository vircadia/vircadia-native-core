//
//  OctreeElement.h
//  libraries/octree/src
//
//  Created by Stephen Birarda on 3/13/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreeElement_h
#define hifi_OctreeElement_h

//#define HAS_AUDIT_CHILDREN
//#define SIMPLE_CHILD_ARRAY
#define SIMPLE_EXTERNAL_CHILDREN

#include <QReadWriteLock>

#include <OctalCode.h>
#include <SharedUtil.h>

#include "AACube.h"
#include "ViewFrustum.h"
#include "OctreeConstants.h"

class CollisionList;
class EncodeBitstreamParams;
class Octree;
class OctreeElement;
class OctreeElementBag;
class OctreeElementDeleteHook;
class OctreePacketData;
class ReadBitstreamToTreeParams;
class Shape;
class VoxelSystem;

const float SMALLEST_REASONABLE_OCTREE_ELEMENT_SCALE = (1.0f / TREE_SCALE) / 10000.0f; // 1/10,000th of a meter

// Callers who want delete hook callbacks should implement this class
class OctreeElementDeleteHook {
public:
    virtual void elementDeleted(OctreeElement* element) = 0;
};

// Callers who want update hook callbacks should implement this class
class OctreeElementUpdateHook {
public:
    virtual void elementUpdated(OctreeElement* element) = 0;
};


class OctreeElement {

protected:
    // can only be constructed by derived implementation
    OctreeElement();

    virtual OctreeElement* createNewElement(unsigned char * octalCode = NULL) = 0;
    
public:
    virtual void init(unsigned char * octalCode); /// Your subclass must call init on construction.
    virtual ~OctreeElement();

    // methods you can and should override to implement your tree functionality
    
    /// Adds a child to the current element. Override this if there is additional child initialization your class needs.
    virtual OctreeElement* addChildAtIndex(int childIndex);

    /// Override this to implement LOD averaging on changes to the tree. 
    virtual void calculateAverageFromChildren() { }

    /// Override this to implement LOD collapsing and identical child pruning on changes to the tree. 
    virtual bool collapseChildren() { return false; }

    /// Should this element be considered to have content in it. This will be used in collision and ray casting methods.
    /// By default we assume that only leaves are actual content, but some octrees may have different semantics.
    virtual bool hasContent() const { return isLeaf(); }

    /// Should this element be considered to have detailed content in it. Specifically should it be rendered.
    /// By default we assume that only leaves have detailed content, but some octrees may have different semantics.
    virtual bool hasDetailedContent() const { return isLeaf(); }
    
    /// Override this to break up large octree elements when an edit operation is performed on a smaller octree element.
    /// For example, if the octrees represent solid cubes and a delete of a smaller octree element is done then the 
    /// meaningful split would be to break the larger cube into smaller cubes of the same color/texture.
    virtual void splitChildren() { }
    
    /// Override to indicate that this element requires a split before editing lower elements in the octree
    virtual bool requiresSplit() const { return false; }

    /// The state of the call to appendElementData
    typedef enum { COMPLETED, PARTIAL, NONE } AppendState;

    virtual void debugExtraEncodeData(EncodeBitstreamParams& params) const { }
    virtual void initializeExtraEncodeData(EncodeBitstreamParams& params) const { }
    virtual bool shouldIncludeChildData(int childIndex, EncodeBitstreamParams& params) const { return true; }
    virtual bool shouldRecurseChildTree(int childIndex, EncodeBitstreamParams& params) const { return true; }
    
    virtual void updateEncodedData(int childIndex, AppendState childAppendState, EncodeBitstreamParams& params) const { }
    virtual void elementEncodeComplete(EncodeBitstreamParams& params, OctreeElementBag* bag) const { }

    /// Override to serialize the state of this element. This is used for persistance and for transmission across the network.
    virtual AppendState appendElementData(OctreePacketData* packetData, EncodeBitstreamParams& params) const 
                                { return COMPLETED; }
    
    /// Override to deserialize the state of this element. This is used for loading from a persisted file or from reading
    /// from the network.
    virtual int readElementDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args) 
                    { return 0; }

    /// Override to indicate that the item is currently rendered in the rendering engine. By default we assume that if
    /// the element should be rendered, then your rendering engine is rendering. But some rendering engines my have cases
    /// where an element is not actually rendering all should render elements. If the isRendered() state doesn't match the
    /// shouldRender() state, the tree will remark elements as changed even in cases there the elements have not changed.
    virtual bool isRendered() const { return getShouldRender(); }
    
    virtual bool deleteApproved() const { return true; }

    virtual bool canRayIntersect() const { return isLeaf(); }
    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                             bool& keepSearching, OctreeElement*& node, float& distance, BoxFace& face, 
                             void** intersectedObject = NULL);

    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face, 
                         void** intersectedObject);

    virtual bool findSpherePenetration(const glm::vec3& center, float radius, 
                        glm::vec3& penetration, void** penetratedObject) const;

    virtual bool findShapeCollisions(const Shape* shape, CollisionList& collisions) const;

    // Base class methods you don't need to implement
    const unsigned char* getOctalCode() const { return (_octcodePointer) ? _octalCode.pointer : &_octalCode.buffer[0]; }
    OctreeElement* getChildAtIndex(int childIndex) const;
    void deleteChildAtIndex(int childIndex);
    OctreeElement* removeChildAtIndex(int childIndex);
    bool isParentOf(OctreeElement* possibleChild) const;

    /// handles deletion of all descendants, returns false if delete not approved
    bool safeDeepDeleteChildAtIndex(int childIndex, int recursionCount = 0); 


    const AACube& getAACube() const { return _cube; }
    const glm::vec3& getCorner() const { return _cube.getCorner(); }
    float getScale() const { return _cube.getScale(); }
    int getLevel() const { return numberOfThreeBitSectionsInCode(getOctalCode()) + 1; }
    
    float getEnclosingRadius() const;
    bool isInView(const ViewFrustum& viewFrustum) const { return inFrustum(viewFrustum) != ViewFrustum::OUTSIDE; }
    ViewFrustum::location inFrustum(const ViewFrustum& viewFrustum) const;
    float distanceToCamera(const ViewFrustum& viewFrustum) const; 
    float furthestDistanceToCamera(const ViewFrustum& viewFrustum) const;

    bool calculateShouldRender(const ViewFrustum* viewFrustum, 
                float voxelSizeScale = DEFAULT_OCTREE_SIZE_SCALE, int boundaryLevelAdjust = 0) const;
    
    // points are assumed to be in Voxel Coordinates (not TREE_SCALE'd)
    float distanceSquareToPoint(const glm::vec3& point) const; // when you don't need the actual distance, use this.
    float distanceToPoint(const glm::vec3& point) const;

    bool isLeaf() const { return _childBitmask == 0; }
    int getChildCount() const { return numberOfOnes(_childBitmask); }
    void printDebugDetails(const char* label) const;
    bool isDirty() const { return _isDirty; }
    void clearDirtyBit() { _isDirty = false; }
    void setDirtyBit() { _isDirty = true; }
    bool hasChangedSince(quint64 time) const { return (_lastChanged > time); }
    void markWithChangedTime();
    quint64 getLastChanged() const { return _lastChanged; }
    void handleSubtreeChanged(Octree* myTree);
    
    // Used by VoxelSystem for rendering in/out of view and LOD
    void setShouldRender(bool shouldRender);
    bool getShouldRender() const { return _shouldRender; }
    
    
    void setSourceUUID(const QUuid& sourceID);
    QUuid getSourceUUID() const;
    uint16_t getSourceUUIDKey() const { return _sourceUUIDKey; }
    bool matchesSourceUUID(const QUuid& sourceUUID) const;
    static uint16_t getSourceNodeUUIDKey(const QUuid& sourceUUID);

    static void addDeleteHook(OctreeElementDeleteHook* hook);
    static void removeDeleteHook(OctreeElementDeleteHook* hook);

    static void addUpdateHook(OctreeElementUpdateHook* hook);
    static void removeUpdateHook(OctreeElementUpdateHook* hook);
    
    static void resetPopulationStatistics();
    static unsigned long getNodeCount() { return _voxelNodeCount; }
    static unsigned long getInternalNodeCount() { return _voxelNodeCount - _voxelNodeLeafCount; }
    static unsigned long getLeafNodeCount() { return _voxelNodeLeafCount; }

    static quint64 getVoxelMemoryUsage() { return _voxelMemoryUsage; }
    static quint64 getOctcodeMemoryUsage() { return _octcodeMemoryUsage; }
    static quint64 getExternalChildrenMemoryUsage() { return _externalChildrenMemoryUsage; }
    static quint64 getTotalMemoryUsage() { return _voxelMemoryUsage + _octcodeMemoryUsage + _externalChildrenMemoryUsage; }

    static quint64 getGetChildAtIndexTime() { return _getChildAtIndexTime; }
    static quint64 getGetChildAtIndexCalls() { return _getChildAtIndexCalls; }
    static quint64 getSetChildAtIndexTime() { return _setChildAtIndexTime; }
    static quint64 getSetChildAtIndexCalls() { return _setChildAtIndexCalls; }

#ifdef BLENDED_UNION_CHILDREN
    static quint64 getSingleChildrenCount() { return _singleChildrenCount; }
    static quint64 getTwoChildrenOffsetCount() { return _twoChildrenOffsetCount; }
    static quint64 getTwoChildrenExternalCount() { return _twoChildrenExternalCount; }
    static quint64 getThreeChildrenOffsetCount() { return _threeChildrenOffsetCount; }
    static quint64 getThreeChildrenExternalCount() { return _threeChildrenExternalCount; }
    static quint64 getCouldStoreFourChildrenInternally() { return _couldStoreFourChildrenInternally; }
    static quint64 getCouldNotStoreFourChildrenInternally() { return _couldNotStoreFourChildrenInternally; }
#endif

    static quint64 getExternalChildrenCount() { return _externalChildrenCount; }
    static quint64 getChildrenCount(int childCount) { return _childrenCount[childCount]; }
    
#ifdef BLENDED_UNION_CHILDREN
#ifdef HAS_AUDIT_CHILDREN
    void auditChildren(const char* label) const;
#endif // def HAS_AUDIT_CHILDREN
#endif // def BLENDED_UNION_CHILDREN

    enum ChildIndex {
        CHILD_BOTTOM_RIGHT_NEAR = 0,
        CHILD_BOTTOM_RIGHT_FAR = 1,
        CHILD_TOP_RIGHT_NEAR = 2,
        CHILD_TOP_RIGHT_FAR = 3,
        CHILD_BOTTOM_LEFT_NEAR = 4,
        CHILD_BOTTOM_LEFT_FAR = 5,
        CHILD_TOP_LEFT_NEAR = 6,
        CHILD_TOP_LEFT_FAR = 7,
        CHILD_UNKNOWN = -1
    };

    struct HalfSpace {
        enum {
            None    = 0x00,
            Bottom  = 0x01,
            Top     = 0x02,
            Right   = 0x04,
            Left    = 0x08,
            Near    = 0x10,
            Far     = 0x20,
            All     = 0x3f,
        };
    };


    OctreeElement* getOrCreateChildElementAt(float x, float y, float z, float s);
    OctreeElement* getOrCreateChildElementContaining(const AACube& box);
    OctreeElement* getOrCreateChildElementContaining(const AABox& box);
    int getMyChildContaining(const AACube& cube) const;
    int getMyChildContaining(const AABox& box) const;
    int getMyChildContainingPoint(const glm::vec3& point) const;

protected:

    void deleteAllChildren();
    void setChildAtIndex(int childIndex, OctreeElement* child);

#ifdef BLENDED_UNION_CHILDREN
    void storeTwoChildren(OctreeElement* childOne, OctreeElement* childTwo);
    void retrieveTwoChildren(OctreeElement*& childOne, OctreeElement*& childTwo);
    void storeThreeChildren(OctreeElement* childOne, OctreeElement* childTwo, OctreeElement* childThree);
    void retrieveThreeChildren(OctreeElement*& childOne, OctreeElement*& childTwo, OctreeElement*& childThree);
    void decodeThreeOffsets(int64_t& offsetOne, int64_t& offsetTwo, int64_t& offsetThree) const;
    void encodeThreeOffsets(int64_t offsetOne, int64_t offsetTwo, int64_t offsetThree);
    void checkStoreFourChildren(OctreeElement* childOne, OctreeElement* childTwo, OctreeElement* childThree, OctreeElement* childFour);
#endif
    void calculateAACube();
    void notifyDeleteHooks();
    void notifyUpdateHooks();

    AACube _cube; /// Client and server, axis aligned box for bounds of this voxel, 48 bytes

    /// Client and server, buffer containing the octal code or a pointer to octal code for this node, 8 bytes
    union octalCode_t {
      unsigned char buffer[8];
      unsigned char* pointer;
    } _octalCode;  

    quint64 _lastChanged; /// Client and server, timestamp this node was last changed, 8 bytes

    /// Client and server, pointers to child nodes, various encodings
#ifdef SIMPLE_CHILD_ARRAY
    OctreeElement* _simpleChildArray[8]; /// Only used when SIMPLE_CHILD_ARRAY is enabled
#endif

#ifdef SIMPLE_EXTERNAL_CHILDREN
    union children_t {
      OctreeElement* single;
      OctreeElement** external;
    } _children;
#endif
    
#ifdef BLENDED_UNION_CHILDREN
    union children_t {
      OctreeElement* single;
      int32_t offsetsTwoChildren[2];
      quint64 offsetsThreeChildrenEncoded;
      OctreeElement** external;
    } _children;
#ifdef HAS_AUDIT_CHILDREN
    OctreeElement* _childrenArray[8]; /// Only used when HAS_AUDIT_CHILDREN is enabled to help debug children encoding
#endif // def HAS_AUDIT_CHILDREN

#endif //def BLENDED_UNION_CHILDREN

    uint16_t _sourceUUIDKey; /// Client only, stores node id of voxel server that sent his voxel, 2 bytes

    // Support for _sourceUUID, we use these static member variables to track the UUIDs that are
    // in use by various voxel server nodes. We map the UUID strings into an 16 bit key, this limits us to at
    // most 65k voxel servers in use at a time within the client. Which is far more than we need.
    static uint16_t _nextUUIDKey; // start at 1, 0 is reserved for NULL
    static std::map<QString, uint16_t> _mapSourceUUIDsToKeys;
    static std::map<uint16_t, QString> _mapKeysToSourceUUIDs;

    unsigned char _childBitmask;     // 1 byte 

    bool _falseColored : 1, /// Client only, is this voxel false colored, 1 bit
         _isDirty : 1, /// Client only, has this voxel changed since being rendered, 1 bit
         _shouldRender : 1, /// Client only, should this voxel render at this time, 1 bit
         _octcodePointer : 1, /// Client and Server only, is this voxel's octal code a pointer or buffer, 1 bit
         _unknownBufferIndex : 1,
         _childrenExternal : 1; /// Client only, is this voxel's VBO buffer the unknown buffer index, 1 bit

    static QReadWriteLock _deleteHooksLock;
    static std::vector<OctreeElementDeleteHook*> _deleteHooks;

    //static QReadWriteLock _updateHooksLock;
    static std::vector<OctreeElementUpdateHook*> _updateHooks;

    static quint64 _voxelNodeCount;
    static quint64 _voxelNodeLeafCount;

    static quint64 _voxelMemoryUsage;
    static quint64 _octcodeMemoryUsage;
    static quint64 _externalChildrenMemoryUsage;

    static quint64 _getChildAtIndexTime;
    static quint64 _getChildAtIndexCalls;
    static quint64 _setChildAtIndexTime;
    static quint64 _setChildAtIndexCalls;

#ifdef BLENDED_UNION_CHILDREN
    static quint64 _singleChildrenCount;
    static quint64 _twoChildrenOffsetCount;
    static quint64 _twoChildrenExternalCount;
    static quint64 _threeChildrenOffsetCount;
    static quint64 _threeChildrenExternalCount;
    static quint64 _couldStoreFourChildrenInternally;
    static quint64 _couldNotStoreFourChildrenInternally;
#endif
    static quint64 _externalChildrenCount;
    static quint64 _childrenCount[NUMBER_OF_CHILDREN + 1];
};

#endif // hifi_OctreeElement_h
