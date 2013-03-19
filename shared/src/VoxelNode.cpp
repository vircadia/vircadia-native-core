//
//  VoxelNode.cpp
//  hifi
//
//  Created by Stephen Birarda on 3/13/13.
//
//

#include "SharedUtil.h"
#include "VoxelNode.h"
#include "OctalCode.h"

VoxelNode::VoxelNode() {
    
    childMask = 0;
    octalCode = NULL;
    
    // default pointers to child nodes to NULL
    for (int i = 0; i < 8; i++) {
        children[i] = NULL;
    }
}

VoxelNode::~VoxelNode() {
    delete[] octalCode;
    
    // delete all of this node's children
    for (int i = 0; i < 8; i++) {
        delete children[i];
    }
}

void VoxelNode::addChildAtIndex(int8_t childIndex) {
    children[childIndex] = new VoxelNode();
    
    // give this child its octal code
    children[childIndex]->octalCode = childOctalCode(octalCode, childIndex);
}

void VoxelNode::setColorFromAverageOfChildren(int * colorArray) {
    if (colorArray == NULL) {
        colorArray = new int[4];
        memset(colorArray, 0, 4);
        
        for (int i = 0; i < 8; i++) {
            if (children[i] != NULL && children[i]->color[3] == 1) {
                for (int j = 0; j < 3; j++) {
                    colorArray[j] += children[i]->color[j];
                }
                
                colorArray[3]++;
            }
        }
    }
    
    if (colorArray[3] > 4) {
        // we need at least 4 colored children to have an average color value
        // or if we have none we generate random values
        
        for (int c = 0; c < 3; c++) {
            // set the average color value
            color[c] = colorArray[c] / colorArray[3];
        }
        
        // set the alpha to 1 to indicate that this isn't transparent
        color[3] = 1;
    } else {
        // some children, but not enough
        // set this node's alpha to 0
        color[3] = 0;
    }
}

void VoxelNode::setRandomColor(int minimumBrightness) {
    for (int c = 0; c < 3; c++) {
        color[c] = randomColorValue(minimumBrightness);
    }
    
    color[3] = 1;
}