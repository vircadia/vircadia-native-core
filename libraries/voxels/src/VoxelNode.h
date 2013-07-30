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
    virtual void nodeDeleted(VoxelNode* node) = 0;
};

class VoxelNode {
public:
    VoxelNode(); // root node constructor
    VoxelNode(unsigned char * octalCode); // regular constructor
    ~VoxelNode();
    
    unsigned char* getOctalCode() const { return _octalCode; };
    VoxelNode* getChildAtIndex(int childIndex) const { return _children[childIndex]; };
    void deleteChildAtIndex(int childIndex);
    VoxelNode* removeChildAtIndex(int childIndex);
    VoxelNode* addChildAtIndex(int childIndex);
    void safeDeepDeleteChildAtIndex(int childIndex); // handles deletion of all descendents

    void setColorFromAverageOfChildren();
    void setRandomColor(int minimumBrightness);
    bool collapseIdenticalLeaves();

    const AABox& getAABox() const { return _box; };
    const glm::vec3& getCenter() const { return _box.getCenter(); };
    const glm::vec3& getCorner() const { return _box.getCorner(); };
    float getScale() const { return _box.getSize().x;  /* voxelScale = (1 / powf(2, *node->getOctalCode())); */ };
    int getLevel() const { return *_octalCode + 1; /* one based or zero based? this doesn't correctly handle 2 byte case */ };
    
    float getEnclosingRadius() const;
    
    bool isColored() const { return (_trueColor[3]==1); }; 
    bool isInView(const ViewFrustum& viewFrustum) const; 
    ViewFrustum::location inFrustum(const ViewFrustum& viewFrustum) const;
    float distanceToCamera(const ViewFrustum& viewFrustum) const; 
    float furthestDistanceToCamera(const ViewFrustum& viewFrustum) const;

    bool calculateShouldRender(const ViewFrustum* viewFrustum, int boundaryLevelAdjust = 0) const;
    
    // points are assumed to be in Voxel Coordinates (not TREE_SCALE'd)
    float distanceSquareToPoint(const glm::vec3& point) const; // when you don't need the actual distance, use this.
    float distanceToPoint(const glm::vec3& point) const;

    bool isLeaf() const { return _childCount == 0; }
    int getChildCount() const { return _childCount; }
    void printDebugDetails(const char* label) const;
    bool isDirty() const { return _isDirty; };
    void clearDirtyBit() { _isDirty = false; };
    bool hasChangedSince(uint64_t time) const { return (_lastChanged > time);  };
    void markWithChangedTime() { _lastChanged = usecTimestampNow();  };
    uint64_t getLastChanged() const { return _lastChanged; };
    void handleSubtreeChanged(VoxelTree* myTree);
    
    glBufferIndex getBufferIndex() const { return _glBufferIndex; };
    bool isKnownBufferIndex() const { return (_glBufferIndex != GLBUFFER_INDEX_UNKNOWN); };
    void setBufferIndex(glBufferIndex index) { _glBufferIndex = index; };
    VoxelSystem* getVoxelSystem() const { return _voxelSystem; };
    void setVoxelSystem(VoxelSystem* voxelSystem) { _voxelSystem = voxelSystem; };
    

    // Used by VoxelSystem for rendering in/out of view and LOD
    void setShouldRender(bool shouldRender);
    bool getShouldRender() const { return _shouldRender; }

#ifndef NO_FALSE_COLOR // !NO_FALSE_COLOR means, does have false color
    void setFalseColor(colorPart red, colorPart green, colorPart blue);
    void setFalseColored(bool isFalseColored);
    bool getFalseColored() { return _falseColored; };
    void setColor(const nodeColor& color);
    const nodeColor& getTrueColor() const { return _trueColor; };
    const nodeColor& getColor() const { return _currentColor; };
    void setDensity(float density) { _density = density; };
    float getDensity() const { return _density; };
#else
    void setFalseColor(colorPart red, colorPart green, colorPart blue) { /* no op */ };
    void setFalseColored(bool isFalseColored) { /* no op */ };
    bool getFalseColored() { return false; };
    void setColor(const nodeColor& color) { memcpy(_trueColor,color,sizeof(nodeColor)); };
    void setDensity(const float density) { _density = density; };
    const nodeColor& getTrueColor() const { return _trueColor; };
    const nodeColor& getColor() const { return _trueColor; };
#endif

    static void addDeleteHook(VoxelNodeDeleteHook* hook);
    static void removeDeleteHook(VoxelNodeDeleteHook* hook);
    
    void recalculateSubTreeNodeCount();
    unsigned long getSubTreeNodeCount()         const { return _subtreeNodeCount; };
    unsigned long getSubTreeInternalNodeCount() const { return _subtreeNodeCount - _subtreeLeafNodeCount; };
    unsigned long getSubTreeLeafNodeCount()     const { return _subtreeLeafNodeCount; };

private:
    void calculateAABox();
    void init(unsigned char * octalCode);
    void notifyDeleteHooks();

    nodeColor _trueColor;
#ifndef NO_FALSE_COLOR // !NO_FALSE_COLOR means, does have false color
    nodeColor _currentColor;
    bool      _falseColored;
#endif
    glBufferIndex   _glBufferIndex;
    VoxelSystem*    _voxelSystem;
    bool            _isDirty;
    uint64_t        _lastChanged;
    bool            _shouldRender;
    AABox           _box;
    unsigned char*  _octalCode;
    VoxelNode*      _children[8];
    int             _childCount;
    unsigned long   _subtreeNodeCount;
    unsigned long   _subtreeLeafNodeCount;
    float           _density;       // If leaf: density = 1, if internal node: 0-1 density of voxels inside

    static std::vector<VoxelNodeDeleteHook*> _hooks;
};

#endif /* defined(__hifi__VoxelNode__) */
