//
//  VoxelTree.cpp
//  hifi
//
//  Created by Stephen Birarda on 3/13/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "VoxelTree.h"

const int MAX_VOXEL_PACKET_SIZE = 1492;
const int MAX_TREE_SIZE_BYTES = 26;

VoxelTree::VoxelTree() {
    rootNode = new VoxelNode();
}

unsigned char * VoxelTree::loadBitstream(unsigned char * bitstreamBuffer, VoxelNode *bitstreamRootNode, unsigned char ** startSplitPtr) {
    static unsigned char **packetSplitPtr = startSplitPtr;
    
    if (bitstreamRootNode->childMask != 0) {
        
        if ((bitstreamBuffer + MAX_TREE_SIZE_BYTES - *packetSplitPtr) > MAX_VOXEL_PACKET_SIZE) {
            // we need to add a packet split here
            
            // add the octal code for the current root
            
            // increment packetSplitPtr and set the pointer it points to
            // to the beginning of the next tree section
            *(++packetSplitPtr) = bitstreamBuffer;
        }
        
        // default color mask is 0, increment pointer for colors
        *(bitstreamBuffer++) = 0;
        
        // keep a colorPointer so we can check how many colors were added
        unsigned char *colorPointer = bitstreamBuffer;
        
        for (int i = 0; i < 8; i++) {
            // grab this child
            VoxelNode *child = bitstreamRootNode->children[i];
            
            // check if the child exists and is not transparent
            if (child != NULL && child->color[3] != 0) {
                
                // copy in the childs color to bitstreamBuffer
                memcpy(colorPointer, child->color, 3);
                colorPointer += 3;
                
                // set the colorMask by bitshifting the value of childExists
                *(bitstreamBuffer - 1) += (1 << (7 - i));
            }
        }
        
        // push the bitstreamBuffer forwards for the number of added colors
        bitstreamBuffer += (colorPointer - bitstreamBuffer);
        // copy the childMask to the current position of the bitstreamBuffer, and increment it
        *(bitstreamBuffer++) = bitstreamRootNode->childMask;
        
        for (int j = 0; j < 8; j++) {
            // make sure we have a child to visit
            if (bitstreamRootNode->children[j] != NULL) {
                bitstreamBuffer = loadBitstream(bitstreamBuffer,
                                                bitstreamRootNode->children[j]);
            }
        }
    }
    
    return bitstreamBuffer;
}