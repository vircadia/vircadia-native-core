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

typedef unsigned char colorPart;
typedef unsigned char nodeColor[4];

class VoxelNode {
private:
    nodeColor _trueColor;
    nodeColor _currentColor;
    bool      _falseColored;
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
    void setFalseColor(colorPart red, colorPart green, colorPart blue);
    void setFalseColored(bool isFalseColored);
    bool getFalseColored() { return _falseColored; };
    
    void setColor(const nodeColor& color);
    const nodeColor& getTrueColor() const { return _trueColor; };
    const nodeColor& getColor() const { return _currentColor; };
    
    void getAABox(AABox& box) const;
};

#endif /* defined(__hifi__VoxelNode__) */
