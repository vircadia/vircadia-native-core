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
public:
    VoxelNode();
    ~VoxelNode();
    
    void addChildAtIndex(int childIndex);
    void setColorFromAverageOfChildren();
    void setRandomColor(int minimumBrightness);
    bool collapseIdenticalLeaves();
    
    unsigned char *octalCode;
    VoxelNode *children[8];
    
    bool isColored() const { return (_trueColor[3]==1); }; 
    bool isInView(const ViewFrustum& viewFrustum) const; 
    float distanceToCamera(const ViewFrustum& viewFrustum) const; 
    bool isLeaf() const;
    void getAABox(AABox& box) const;
    void printDebugDetails(const char* label) const;
    bool isDirty() const { return _isDirty; };
    void clearDirtyBit() { _isDirty = false; };
    glBufferIndex getBufferIndex() const { return _glBufferIndex; };
    bool isKnownBufferIndex() const { return (_glBufferIndex != GLBUFFER_INDEX_UNKNOWN); };
    void setBufferIndex(glBufferIndex index) { _glBufferIndex = index; };
    void setShouldRender(bool shouldRender);
    bool getShouldRender() const { return _shouldRender; }

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
