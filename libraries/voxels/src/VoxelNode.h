//
//  VoxelNode.h
//  hifi
//
//  Created by Stephen Birarda on 3/13/13.
//
//

#ifndef __hifi__VoxelNode__
#define __hifi__VoxelNode__

#include "AABox.h"
#include "ViewFrustum.h"
#include "VoxelConstants.h"

typedef unsigned char colorPart;
typedef unsigned char nodeColor[4];
typedef unsigned char rgbColor[3];

class VoxelNode {
private:
    nodeColor _trueColor;
#ifndef NO_FALSE_COLOR // !NO_FALSE_COLOR means, does have false color
    nodeColor _currentColor;
    bool      _falseColored;
#endif
    glBufferIndex _glBufferIndex;
    bool _isDirty;
    bool _shouldRender;
    bool _isStagedForDeletion;
    AABox _box;
    unsigned char* _octalCode;
    VoxelNode* _children[8];

    void calculateAABox();

    void init(unsigned char * octalCode);

public:
    VoxelNode(); // root node constructor
    VoxelNode(unsigned char * octalCode); // regular constructor
    ~VoxelNode();
    
    unsigned char* getOctalCode() const { return _octalCode; };
    VoxelNode* getChildAtIndex(int i) const { return _children[i]; };
    void deleteChildAtIndex(int childIndex);
    VoxelNode* removeChildAtIndex(int childIndex);
    void addChildAtIndex(int childIndex);
    void setColorFromAverageOfChildren();
    void setRandomColor(int minimumBrightness);
    bool collapseIdenticalLeaves();

    const AABox& getAABox() const { return _box; };
    const glm::vec3& getCenter() const { return _box.getCenter(); };
    const glm::vec3& getCorner() const { return _box.getCorner(); };
    float getScale() const { return _box.getSize().x;  /* voxelScale = (1 / powf(2, *node->getOctalCode())); */ };
    int getLevel() const { return *_octalCode + 1; /* one based or zero based? */ };
    
    bool isColored() const { return (_trueColor[3]==1); }; 
    bool isInView(const ViewFrustum& viewFrustum) const; 
    ViewFrustum::location inFrustum(const ViewFrustum& viewFrustum) const;
    float distanceToCamera(const ViewFrustum& viewFrustum) const; 
    bool isLeaf() const;
    void printDebugDetails(const char* label) const;
    bool isDirty() const { return _isDirty; };
    void clearDirtyBit() { _isDirty = false; };
    glBufferIndex getBufferIndex() const { return _glBufferIndex; };
    bool isKnownBufferIndex() const { return (_glBufferIndex != GLBUFFER_INDEX_UNKNOWN); };
    void setBufferIndex(glBufferIndex index) { _glBufferIndex = index; };

    // Used by VoxelSystem for rendering in/out of view and LOD
    void setShouldRender(bool shouldRender);
    bool getShouldRender() const { return _shouldRender; }

    // Used by VoxelSystem to mark a node as to be deleted on next render pass
    void stageForDeletion() { _isStagedForDeletion = true; };
    bool isStagedForDeletion() const { return _isStagedForDeletion; }

#ifndef NO_FALSE_COLOR // !NO_FALSE_COLOR means, does have false color
    void setFalseColor(colorPart red, colorPart green, colorPart blue);
    void setFalseColored(bool isFalseColored);
    bool getFalseColored() { return _falseColored; };
    void setColor(const nodeColor& color);
    const nodeColor& getTrueColor() const { return _trueColor; };
    const nodeColor& getColor() const { return _currentColor; };
#else
    void setFalseColor(colorPart red, colorPart green, colorPart blue) { /* no op */ };
    void setFalseColored(bool isFalseColored) { /* no op */ };
    bool getFalseColored() { return false; };
    void setColor(const nodeColor& color) { memcpy(_trueColor,color,sizeof(nodeColor)); };
    const nodeColor& getTrueColor() const { return _trueColor; };
    const nodeColor& getColor() const { return _trueColor; };
#endif
};

#endif /* defined(__hifi__VoxelNode__) */
