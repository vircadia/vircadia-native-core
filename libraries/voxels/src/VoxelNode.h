//
//  VoxelNode.h
//  hifi
//
//  Created by Stephen Birarda on 3/13/13.
//
//

#ifndef __hifi__VoxelNode__
#define __hifi__VoxelNode__

#include <SharedUtil.h>
#include "AABox.h"
#include "ViewFrustum.h"
#include "VoxelConstants.h"

class VoxelTree; // forward declaration
class VoxelNode; // forward declaration
class VoxelSystem; // forward declaration

typedef unsigned char colorPart;
typedef unsigned char nodeColor[4];
typedef unsigned char rgbColor[3];

// Callers who want delete hook callbacks should implement this class
class VoxelNodeDeleteHook {
public:
    virtual void voxelDeleted(VoxelNode* node) = 0;
};

// Callers who want update hook callbacks should implement this class
class VoxelNodeUpdateHook {
public:
    virtual void voxelUpdated(VoxelNode* node) = 0;
};


class VoxelNode {
public:
    VoxelNode(); // root node constructor
    VoxelNode(unsigned char * octalCode); // regular constructor
    ~VoxelNode();
    
    const unsigned char* getOctalCode() const { return (_octcodePointer) ? _octalCode.pointer : &_octalCode.buffer[0]; }
    VoxelNode* getChildAtIndex(int childIndex) const;
    void deleteChildAtIndex(int childIndex);
    VoxelNode* removeChildAtIndex(int childIndex);
    VoxelNode* addChildAtIndex(int childIndex);
    void safeDeepDeleteChildAtIndex(int childIndex); // handles deletion of all descendents

    void setColorFromAverageOfChildren();
    void setRandomColor(int minimumBrightness);
    bool collapseIdenticalLeaves();

    const AABox& getAABox() const { return _box; }
    const glm::vec3& getCorner() const { return _box.getCorner(); }
    float getScale() const { return _box.getScale(); }
    int getLevel() const { return numberOfThreeBitSectionsInCode(getOctalCode()) + 1; }
    
    float getEnclosingRadius() const;
    
    bool isColored() const { return _trueColor[3] == 1; }
    bool isInView(const ViewFrustum& viewFrustum) const; 
    ViewFrustum::location inFrustum(const ViewFrustum& viewFrustum) const;
    float distanceToCamera(const ViewFrustum& viewFrustum) const; 
    float furthestDistanceToCamera(const ViewFrustum& viewFrustum) const;

    bool calculateShouldRender(const ViewFrustum* viewFrustum, int boundaryLevelAdjust = 0) const;
    
    // points are assumed to be in Voxel Coordinates (not TREE_SCALE'd)
    float distanceSquareToPoint(const glm::vec3& point) const; // when you don't need the actual distance, use this.
    float distanceToPoint(const glm::vec3& point) const;

    bool isLeaf() const { return getChildCount() == 0; }
    int getChildCount() const { return numberOfOnes(_childBitmask); }
    void printDebugDetails(const char* label) const;
    bool isDirty() const { return _isDirty; }
    void clearDirtyBit() { _isDirty = false; }
    void setDirtyBit() { _isDirty = true; }
    bool hasChangedSince(uint64_t time) const { return (_lastChanged > time); }
    void markWithChangedTime();
    uint64_t getLastChanged() const { return _lastChanged; }
    void handleSubtreeChanged(VoxelTree* myTree);
    
    glBufferIndex getBufferIndex() const { return _glBufferIndex; }
    bool isKnownBufferIndex() const { return !_unknownBufferIndex; }
    void setBufferIndex(glBufferIndex index) { _glBufferIndex = index; _unknownBufferIndex =(index == GLBUFFER_INDEX_UNKNOWN);}
    VoxelSystem* getVoxelSystem() const;
    void setVoxelSystem(VoxelSystem* voxelSystem);

    // Used by VoxelSystem for rendering in/out of view and LOD
    void setShouldRender(bool shouldRender);
    bool getShouldRender() const { return _shouldRender; }

    void setFalseColor(colorPart red, colorPart green, colorPart blue);
    void setFalseColored(bool isFalseColored);
    bool getFalseColored() { return _falseColored; }
    void setColor(const nodeColor& color);
    const nodeColor& getTrueColor() const { return _trueColor; }
    const nodeColor& getColor() const { return _currentColor; }

    void setDensity(float density) { _density = density; }
    float getDensity() const { return _density; }
    void setSourceID(uint16_t sourceID) { _sourceID = sourceID; }
    uint16_t getSourceID() const { return _sourceID; }

    static void addDeleteHook(VoxelNodeDeleteHook* hook);
    static void removeDeleteHook(VoxelNodeDeleteHook* hook);

    static void addUpdateHook(VoxelNodeUpdateHook* hook);
    static void removeUpdateHook(VoxelNodeUpdateHook* hook);
    
    static unsigned long getNodeCount() { return _voxelNodeCount; }
    static unsigned long getInternalNodeCount() { return _voxelNodeCount - _voxelNodeLeafCount; }
    static unsigned long getLeafNodeCount() { return _voxelNodeLeafCount; }

    static uint64_t getVoxelMemoryUsage() { return _voxelMemoryUsage; }
    static uint64_t getOctcodeMemoryUsage() { return _octcodeMemoryUsage; }
    
    static uint64_t _getChildAtIndexTime;
    static uint64_t _getChildAtIndexCalls;
    static uint64_t _setChildAtIndexTime;
    static uint64_t _setChildAtIndexCalls;

private:
    void setChildAtIndex(int childIndex, VoxelNode* child);
    void storeTwoChildren(VoxelNode* childOne, VoxelNode* childTwo);
    void retrieveTwoChildren(VoxelNode*& childOne, VoxelNode*& childTwo);
    VoxelNode* __new__getChildAtIndex(int childIndex) const;
    void __new__setChildAtIndex(int childIndex, VoxelNode* child);
    void auditChildren(const char* label) const;

    void calculateAABox();
    void init(unsigned char * octalCode);
    void notifyDeleteHooks();
    void notifyUpdateHooks();

    AABox _box; /// Client and server, axis aligned box for bounds of this voxel, 48 bytes

    /// Client and server, buffer containing the octal code or a pointer to octal code for this node, 8 bytes
    union octalCode_t {
      unsigned char buffer[8];
      unsigned char* pointer;
    } _octalCode;  

    uint64_t _lastChanged; /// Client and server, timestamp this node was last changed, 8 bytes

    /// Client and server, pointers to child nodes, various encodings
    union children_t {
      VoxelNode* single;
      int32_t offsetsTwoChildren[2];
      uint64_t offsetsThreeChildrenA : 21, offsetsThreeChildrenB : 21, offsetsThreeChildrenC : 21 ;
      VoxelNode** external;
    } _children;  

    static int64_t _offsetMax;
    static int64_t _offsetMin;
    
    //_offsetMax=4,184,812 
    //_offsetMin=-5,828,062
    
    VoxelNode* _childrenArray[8]; /// Client and server, pointers to child nodes, 64 bytes

    uint32_t _glBufferIndex : 24, /// Client only, vbo index for this voxel if being rendered, 3 bytes
             _voxelSystemIndex : 8; /// Client only, index to the VoxelSystem rendering this voxel, 1 bytes

    static uint8_t _nextIndex;
    static std::map<VoxelSystem*, uint8_t> _mapVoxelSystemPointersToIndex;
    static std::map<uint8_t, VoxelSystem*> _mapIndexToVoxelSystemPointers;

    float _density; /// Client and server, If leaf: density = 1, if internal node: 0-1 density of voxels inside, 4 bytes

    nodeColor _trueColor; /// Client and server, true color of this voxel, 4 bytes
    nodeColor _currentColor; /// Client only, false color of this voxel, 4 bytes

    uint16_t _sourceID; /// Client only, stores node id of voxel server that sent his voxel, 2 bytes

    unsigned char _childBitmask;     // 1 byte 

    bool _falseColored : 1, /// Client only, is this voxel false colored, 1 bit
         _isDirty : 1, /// Client only, has this voxel changed since being rendered, 1 bit
         _shouldRender : 1, /// Client only, should this voxel render at this time, 1 bit
         _octcodePointer : 1, /// Client and Server only, is this voxel's octal code a pointer or buffer, 1 bit
         _unknownBufferIndex : 1,
         _childrenExternal : 1; /// Client only, is this voxel's VBO buffer the unknown buffer index, 1 bit

    static std::vector<VoxelNodeDeleteHook*> _deleteHooks;
    static std::vector<VoxelNodeUpdateHook*> _updateHooks;

    static uint64_t _voxelNodeCount;
    static uint64_t _voxelNodeLeafCount;

    static uint64_t _voxelMemoryUsage;
    static uint64_t _octcodeMemoryUsage;
};


class oldVoxelNode {
public:

    VoxelNode* _children[8]; /// Client and server, pointers to child nodes, 64 bytes
    oldAABox _box; /// Client and server, axis aligned box for bounds of this voxel, 48 bytes
    unsigned char* _octalCode; /// Client and server, pointer to octal code for this node, 8 bytes

    uint64_t _lastChanged; /// Client and server, timestamp this node was last changed, 8 bytes
    unsigned long _subtreeNodeCount; /// Client and server, nodes below this node, 8 bytes
    unsigned long _subtreeLeafNodeCount; /// Client and server, leaves below this node, 8 bytes

    glBufferIndex _glBufferIndex; /// Client only, vbo index for this voxel if being rendered, 8 bytes
    VoxelSystem* _voxelSystem; /// Client only, pointer to VoxelSystem rendering this voxel, 8 bytes

    float _density; /// Client and server, If leaf: density = 1, if internal node: 0-1 density of voxels inside, 4 bytes
    int _childCount; /// Client and server, current child nodes set to non-null in _children, 4 bytes

    nodeColor _trueColor; /// Client and server, true color of this voxel, 4 bytes
    nodeColor _currentColor; /// Client only, false color of this voxel, 4 bytes
    bool _falseColored; /// Client only, is this voxel false colored, 1 bytes

    bool _isDirty; /// Client only, has this voxel changed since being rendered, 1 byte
    bool _shouldRender; /// Client only, should this voxel render at this time, 1 byte
    uint16_t _sourceID; /// Client only, stores node id of voxel server that sent his voxel, 2 bytes
};

class smallerVoxelNodeTest1 {
public:
    AABox _box;                          // 48 bytes - 4x glm::vec3, 3 floats x 4 bytes = 48 bytes
                                         // 16 bytes... 1x glm::vec3 + 1 float

    union octalCode_t {
      unsigned char _octalCodeBuffer[8];
      unsigned char* _octalCodePointer;
    } _octalCode;  /// Client and server, buffer containing the octal code if it's smaller than 8 bytes or a 
                   /// pointer to octal code for this node, 8 bytes
                                         

    uint32_t _glBufferIndex : 24, /// Client only, vbo index for this voxel if being rendered, 3 bytes
             _voxelSystemIndex : 8; /// Client only, index to the VoxelSystem rendering this voxel, 1 bytes


    uint64_t _lastChanged;               //  8 bytes, could be less?

    void* _children;                     //  8 bytes (for 0 or 1 children), or 8 bytes * count + 1


    nodeColor _trueColor;            // 4 bytes, could be 3 bytes + 1 bit
    nodeColor _currentColor;         // 4 bytes  ** CLIENT ONLY **


    float _density;                  //  4 bytes - If leaf: density = 1, if internal node: 0-1 density of voxels inside... 4 bytes? do we need this?
                                     // could make this 1 or 2 byte linear ratio...


    uint16_t _sourceID;              // 2 bytes - only used to colorize and kill sources? ** CLIENT ONLY **

    unsigned char _childBitmask;     // 1 byte 

    // Bitmask....                     // 1 byte... made up of 5 bits so far... room for 3 more bools...
    unsigned char _falseColored : 1,       //    1 bit
                  _shouldRender : 1,       //    1 bit
                  _isDirty : 1,            //    1 bit
                  _octcodePointer : 1,     //    1 bit
                  _unknownBufferIndex : 1; //    1 bit

};
#endif /* defined(__hifi__VoxelNode__) */