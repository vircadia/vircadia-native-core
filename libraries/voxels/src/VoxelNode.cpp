//
//  VoxelNode.cpp
//  hifi
//
//  Created by Stephen Birarda on 3/13/13.
//
//

#include <stdio.h>
#include <cstring>
#include "SharedUtil.h"
//#include "voxels_Log.h"
#include "VoxelNode.h"
#include "OctalCode.h"
#include "AABox.h"

// using voxels_lib::printLog;

VoxelNode::VoxelNode() {
    octalCode = NULL;
    
#ifdef HAS_FALSE_COLOR
    _falseColored = false; // assume true color
#endif
    
    // default pointers to child nodes to NULL
    for (int i = 0; i < 8; i++) {
        children[i] = NULL;
    }
}

VoxelNode::~VoxelNode() {
    delete[] octalCode;
    
    // delete all of this node's children
    for (int i = 0; i < 8; i++) {
        if (children[i]) {
            delete children[i];
        }
    }
}

void VoxelNode::getAABox(AABox& box) const {
    // copy corner into box
    copyFirstVertexForCode(octalCode,(float*)&box.corner);
    
    // this tells you the "size" of the voxel
    float voxelScale = 1 / powf(2, *octalCode);
    box.x = box.y = box.z = voxelScale;
}

void VoxelNode::addChildAtIndex(int childIndex) {
    children[childIndex] = new VoxelNode();
    
    // give this child its octal code
    children[childIndex]->octalCode = childOctalCode(octalCode, childIndex);
}

// will average the child colors...
void VoxelNode::setColorFromAverageOfChildren() {
	int colorArray[4] = {0,0,0,0};
	for (int i = 0; i < 8; i++) {
		if (children[i] != NULL && children[i]->isColored()) {
			for (int j = 0; j < 3; j++) {
				colorArray[j] += children[i]->getTrueColor()[j]; // color averaging should always be based on true colors
			}
			colorArray[3]++;
		}
	}
    
    nodeColor newColor = { 0, 0, 0, 0};
    if (colorArray[3] > 4) {
        // we need at least 4 colored children to have an average color value
        // or if we have none we generate random values
        for (int c = 0; c < 3; c++) {
            // set the average color value
            newColor[c] = colorArray[c] / colorArray[3];
        }
        // set the alpha to 1 to indicate that this isn't transparent
        newColor[3] = 1;
    }
    // actually set our color, note, if we didn't have enough children
    // this will be the default value all zeros, and therefore be marked as
    // transparent with a 4th element of 0
    setColor(newColor);
}

// Note: !NO_FALSE_COLOR implementations of setFalseColor(), setFalseColored(), and setColor() here.
//       the actual NO_FALSE_COLOR version are inline in the VoxelNode.h
#ifndef NO_FALSE_COLOR // !NO_FALSE_COLOR means, does have false color
void VoxelNode::setFalseColor(colorPart red, colorPart green, colorPart blue) {
    _falseColored=true;
    _currentColor[0] = red;
    _currentColor[1] = green;
    _currentColor[2] = blue;
    _currentColor[3] = 1; // XXXBHG - False colors are always considered set
}

void VoxelNode::setFalseColored(bool isFalseColored) {
    // if we were false colored, and are no longer false colored, then swap back
    if (_falseColored && !isFalseColored) {
        memcpy(&_currentColor,&_trueColor,sizeof(nodeColor));
    }
    _falseColored = isFalseColored; 
};


void VoxelNode::setColor(const nodeColor& color) {
    //printf("VoxelNode::setColor() isFalseColored=%s\n",_falseColored ? "Yes" : "No");
    memcpy(&_trueColor,&color,sizeof(nodeColor));
    if (!_falseColored) {
        memcpy(&_currentColor,&color,sizeof(nodeColor));
    }
}
#endif

// will detect if children are leaves AND the same color
// and in that case will delete the children and make this node
// a leaf, returns TRUE if all the leaves are collapsed into a 
// single node
bool VoxelNode::collapseIdenticalLeaves() {
	// scan children, verify that they are ALL present and accounted for
	bool allChildrenMatch = true; // assume the best (ottimista)
	int red,green,blue;
	for (int i = 0; i < 8; i++) {
		// if no child, or child doesn't have a color
		if (children[i] == NULL || !children[i]->isColored()) {
			allChildrenMatch=false;
			//printLog("SADNESS child missing or not colored! i=%d\n",i);
			break;
		} else {
			if (i==0) {
				red   = children[i]->getColor()[0];
				green = children[i]->getColor()[1];
				blue  = children[i]->getColor()[2];
			} else if (red != children[i]->getColor()[0] || 
                    green != children[i]->getColor()[1] || blue != children[i]->getColor()[2]) {
				allChildrenMatch=false;
				break;
			}
		}
	}
	
	
	if (allChildrenMatch) {
		//printLog("allChildrenMatch: pruning tree\n");
		for (int i = 0; i < 8; i++) {
			delete children[i]; // delete all the child nodes
			children[i]=NULL; // set it to NULL
		}
		nodeColor collapsedColor;
		collapsedColor[0]=red;		
		collapsedColor[1]=green;		
		collapsedColor[2]=blue;		
		collapsedColor[3]=1;	// color is set
		setColor(collapsedColor);
	}
	return allChildrenMatch;
}

void VoxelNode::setRandomColor(int minimumBrightness) {
    nodeColor newColor;
    for (int c = 0; c < 3; c++) {
        newColor[c] = randomColorValue(minimumBrightness);
    }
    
    newColor[3] = 1;
    setColor(newColor);
}
