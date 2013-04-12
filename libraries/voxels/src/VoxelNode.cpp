//
//  VoxelNode.cpp
//  hifi
//
//  Created by Stephen Birarda on 3/13/13.
//
//

#include <cstring>
#include "SharedUtil.h"
#include "VoxelNode.h"
#include "OctalCode.h"

VoxelNode::VoxelNode() {
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

void VoxelNode::addChildAtIndex(int childIndex) {
    children[childIndex] = new VoxelNode();
    
    // give this child its octal code
    children[childIndex]->octalCode = childOctalCode(octalCode, childIndex);
}

// will average the child colors...
void VoxelNode::setColorFromAverageOfChildren() {
	int colorArray[4] = {0,0,0,0};
	for (int i = 0; i < 8; i++) {
		if (children[i] != NULL && children[i]->color[3] == 1) {
			for (int j = 0; j < 3; j++) {
				colorArray[j] += children[i]->color[j];
			}
			colorArray[3]++;
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
		if (children[i] == NULL || children[i]->color[3] != 1) {
			allChildrenMatch=false;
			//printf("SADNESS child missing or not colored! i=%d\n",i);
			break;
		} else {
			if (i==0) {
				red   = children[i]->color[0];
				green = children[i]->color[1];
				blue  = children[i]->color[2];
			} else if (red != children[i]->color[0] || green != children[i]->color[1] || blue != children[i]->color[2]) {
				allChildrenMatch=false;
				break;
			}
		}
	}
	
	
	if (allChildrenMatch) {
		//printf("allChildrenMatch: pruning tree\n");
		for (int i = 0; i < 8; i++) {
			delete children[i]; // delete all the child nodes
			children[i]=NULL; // set it to NULL
		}
		color[0]=red;		
		color[1]=green;		
		color[2]=blue;		
		color[3]=1;	// color is set
	}
	return allChildrenMatch;
}

void VoxelNode::setRandomColor(int minimumBrightness) {
    for (int c = 0; c < 3; c++) {
        color[c] = randomColorValue(minimumBrightness);
    }
    
    color[3] = 1;
}