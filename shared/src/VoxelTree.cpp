//
//  VoxelTree.cpp
//  hifi
//
//  Created by Stephen Birarda on 3/13/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "OctalCode.h"
#include "VoxelTree.h"

const int MAX_TREE_SLICE_BYTES = 26;

VoxelTree::VoxelTree() {
    rootNode = new VoxelNode();
}

char * VoxelTree::loadBitstream(char * bitstreamBuffer,
                                VoxelNode *bitstreamRootNode,
                                char ** startSplitPtr) {
    static char **packetSplitPtr = startSplitPtr;
    
    if (bitstreamRootNode->childMask != 0) {
        
        if (bitstreamRootNode == rootNode || (bitstreamBuffer + MAX_TREE_SLICE_BYTES - *packetSplitPtr) > MAX_VOXEL_PACKET_SIZE) {
            // we need to add a packet split here
            
            
            if (bitstreamRootNode == rootNode) {
                // set the packetSplitPtr and increment bitstream buffer by 1 for leading V
                *packetSplitPtr = bitstreamBuffer;
                
                // lead packet with a V
                *(bitstreamBuffer++) = 'V';
                // root node octal code length byte is 0
                *(bitstreamBuffer++) = 0;
            } else {
                // increment packetSplitPtr and set the pointer it points to
                // to the beginning of this tree section
                
                // bistreamBuffer is incremented for leading V
                *(++packetSplitPtr) = bitstreamBuffer;
                
                // lead packet with a V
                *(bitstreamBuffer++) = 'V';
                
                // add the octal code for the current root
                int bytesForOctalCode = bytesRequiredForCodeLength(*bitstreamRootNode->octalCode);
                memcpy(bitstreamBuffer, bitstreamRootNode->octalCode, bytesForOctalCode);
                
                // push the bitstreamBuffer forwards by the number of bytes used for the octal code
                bitstreamBuffer += bytesForOctalCode;
            }            
        
            // set the color bit for this node
            *(bitstreamBuffer++) = (int)(bitstreamRootNode->color[3] != 0);
            
            // copy this node's color into the bitstreamBuffer, if required
            if (bitstreamRootNode->color[3] != 0) {
                memcpy(bitstreamBuffer, bitstreamRootNode->color, 3);
                bitstreamBuffer += 3;
            }
        }
        
        // default color mask is 0, increment pointer for colors
        *(bitstreamBuffer++) = 0;
        
        // keep a colorPointer so we can check how many colors were added
        char *colorPointer = bitstreamBuffer;
        
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
        // copy the childMask to the current position of the bitstreamBuffer
        // and push the buffer pointer forwards
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