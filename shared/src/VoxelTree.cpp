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
    rootNode->octalCode = new unsigned char[1];
    *rootNode->octalCode = (char)0;
}

unsigned char * VoxelTree::loadBitstreamBuffer(char *& bitstreamBuffer,
                                               unsigned char * stopOctalCode,
                                               VoxelNode *currentVoxelNode)
{
    static char *initialBitstreamPos = bitstreamBuffer;
    
    char firstIndexToCheck = 0;
    
    // we'll only be writing data if we're lower than
    // or at the same level as the stopOctalCode
    if (*currentVoxelNode->octalCode >= *stopOctalCode) {
        if (currentVoxelNode->childMask != 0) {
            if ((bitstreamBuffer - initialBitstreamPos) + MAX_TREE_SLICE_BYTES > MAX_VOXEL_PACKET_SIZE) {
                // we can't send this packet, not enough room
                // return our octal code as the stop
                return currentVoxelNode->octalCode;
            }
            
            if (strcmp((char *)stopOctalCode, (char *)currentVoxelNode->octalCode) == 0) {
                // this is is the root node for this packet
                // add the leading V
                *(bitstreamBuffer++) = 'V';
                
                // add its octal code to the packet
                int octalCodeBytes = bytesRequiredForCodeLength(*currentVoxelNode->octalCode);
                
                memcpy(bitstreamBuffer, currentVoxelNode->octalCode, octalCodeBytes);
                bitstreamBuffer += octalCodeBytes;
            }
            
            // default color mask is 0, increment pointer for colors
            *bitstreamBuffer = 0;
            
            // keep a colorPointer so we can check how many colors were added
            char *colorPointer = bitstreamBuffer + 1;
            
            for (int i = 0; i < 8; i++) {
                
                // check if the child exists and is not transparent
                if (currentVoxelNode->children[i] != NULL
                    && currentVoxelNode->children[i]->color[3] != 0) {
                    
                    // copy in the childs color to bitstreamBuffer
                    memcpy(colorPointer, currentVoxelNode->children[i]->color, 3);
                    colorPointer += 3;
                    
                    // set the colorMask by bitshifting the value of childExists
                    *bitstreamBuffer += (1 << (7 - i));
                }
            }
            
            // push the bitstreamBuffer forwards for the number of added colors
            bitstreamBuffer += (colorPointer - bitstreamBuffer);
            
            // copy the childMask to the current position of the bitstreamBuffer
            // and push the buffer pointer forwards
            
            *(bitstreamBuffer++) = currentVoxelNode->childMask;
        } else {
            // if this node is a leaf, return a NULL stop code
            // it has been visited
            return NULL;
        }
    } else {
        firstIndexToCheck = *stopOctalCode > 0
            ? branchIndexWithDescendant(currentVoxelNode->octalCode, stopOctalCode)
            : 0;
    }
    
    unsigned char * childStopOctalCode = NULL;
    
    for (int i = firstIndexToCheck; i < 8; i ++) {
        
        // ask the child to load this bitstream buffer
        // if they or their descendants fill the MTU we will receive the childStopOctalCode back
        if (currentVoxelNode->children[i] != NULL) {
            if (*currentVoxelNode->octalCode < *stopOctalCode
                && i > firstIndexToCheck
                && childStopOctalCode == NULL) {
                return currentVoxelNode->children[i]->octalCode;
            } else {
                childStopOctalCode = loadBitstreamBuffer(bitstreamBuffer, stopOctalCode, currentVoxelNode->children[i]);
            }
        }
        
        if (childStopOctalCode != NULL) {
            break;
        }
    }
    
    return childStopOctalCode;    
}