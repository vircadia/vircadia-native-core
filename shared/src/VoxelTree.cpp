//
//  VoxelTree.cpp
//  hifi
//
//  Created by Stephen Birarda on 3/13/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <cstring>
#include "SharedUtil.h"
#include "OctalCode.h"
#include "VoxelTree.h"

const int MAX_TREE_SLICE_BYTES = 26;

VoxelTree::VoxelTree() {
    rootNode = new VoxelNode();
    rootNode->octalCode = new unsigned char[1];
    *rootNode->octalCode = (char)0;
}

VoxelTree::~VoxelTree() {
    // delete the children of the root node
    // this recursively deletes the tree
    for (int i = 0; i < 8; i++) {
        delete rootNode->children[i];
    }
}

VoxelNode * VoxelTree::nodeForOctalCode(VoxelNode *ancestorNode, unsigned char * needleCode) {
    // find the appropriate branch index based on this ancestorNode
    if (*needleCode == 0) {
        return ancestorNode;
    } else {
        int branchForNeedle = branchIndexWithDescendant(ancestorNode->octalCode, needleCode);
        VoxelNode *childNode = ancestorNode->children[branchForNeedle];
        
        if (childNode != NULL) {
            if (*childNode->octalCode == *needleCode) {
                // the fact that the number of sections is equivalent does not always guarantee
                // that this is the same node, however due to the recursive traversal
                // we know that this is our node
                return childNode;
            } else {
                // we need to go deeper
                return nodeForOctalCode(childNode, needleCode);
            }
        }
    }
    
    // we've been given a code we don't have a node for
    // return this node as the last created parent
    return ancestorNode;
}

VoxelNode * VoxelTree::createMissingNode(VoxelNode *lastParentNode, unsigned char *codeToReach) {
    int indexOfNewChild = branchIndexWithDescendant(lastParentNode->octalCode, codeToReach);
    lastParentNode->addChildAtIndex(indexOfNewChild);
    
    if (*lastParentNode->children[indexOfNewChild]->octalCode == *codeToReach) {
        return lastParentNode;
    } else {
        return createMissingNode(lastParentNode->children[indexOfNewChild], codeToReach);
    }
}

int VoxelTree::readNodeData(VoxelNode *destinationNode, unsigned char * nodeData, int bytesLeftToRead) {
    
    // instantiate variable for bytes already read
    int bytesRead = 1;
    int colorArray[4] = {};
    
    for (int i = 0; i < 8; i++) {
        // check the colors mask to see if we have a child to color in
        if (oneAtBit(*nodeData, i)) {
            
            // create the child if it doesn't exist
            if (destinationNode->children[i] == NULL) {
                destinationNode->addChildAtIndex(i);
            }
            
            // pull the color for this child
            memcpy(destinationNode->children[i]->color, nodeData + bytesRead, 3);
            destinationNode->children[i]->color[3] = 1;
           
            for (int j = 0; j < 3; j++) {
                colorArray[j] += destinationNode->children[i]->color[j];
            }
            
            bytesRead += 3;
            colorArray[3]++;
        }
    }
    
    // average node's color based on color of children
    destinationNode->setColorFromAverageOfChildren(colorArray);
    
    // give this destination node the child mask from the packet
    destinationNode->childMask = *(nodeData + bytesRead);
    
    int childIndex = 0;
    bytesRead++;
    
    while (bytesLeftToRead - bytesRead > 0 && childIndex < 8) {
        // check the exists mask to see if we have a child to traverse into
        
        if (oneAtBit(destinationNode->childMask, childIndex)) {            
            if (destinationNode->children[childIndex] == NULL) {
                // add a child at that index, if it doesn't exist
                destinationNode->addChildAtIndex(childIndex);
            }
            
            // tell the child to read the subsequent data
            bytesRead += readNodeData(destinationNode->children[childIndex], nodeData + bytesRead, bytesLeftToRead - bytesRead);
        }
        
        childIndex++;
    }
    
    return bytesRead;
}

void VoxelTree::readBitstreamToTree(unsigned char * bitstream, int bufferSizeBytes) {
    VoxelNode *bitstreamRootNode = nodeForOctalCode(rootNode, (unsigned char *)bitstream);
    
    if (*bitstream != *bitstreamRootNode->octalCode) {
        // if the octal code returned is not on the same level as
        // the code being searched for, we have VoxelNodes to create
        bitstreamRootNode = createMissingNode(bitstreamRootNode, (unsigned char *)bitstream);
    }
    
    int octalCodeBytes = bytesRequiredForCodeLength(*bitstream);
    readNodeData(bitstreamRootNode, bitstream + octalCodeBytes, bufferSizeBytes - octalCodeBytes);
}

unsigned char * VoxelTree::loadBitstreamBuffer(unsigned char *& bitstreamBuffer,
                                               unsigned char * stopOctalCode,
                                               VoxelNode *currentVoxelNode)
{
    static unsigned char *initialBitstreamPos = bitstreamBuffer;
    
    int firstIndexToCheck = 0;
    
    // we'll only be writing data if we're lower than
    // or at the same level as the stopOctalCode
    if (*currentVoxelNode->octalCode >= *stopOctalCode) {
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
        unsigned char *colorPointer = bitstreamBuffer + 1;
        
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
                if (oneAtBit(currentVoxelNode->childMask, i)) {
                    childStopOctalCode = loadBitstreamBuffer(bitstreamBuffer, stopOctalCode, currentVoxelNode->children[i]);
                } else {
                    childStopOctalCode = NULL;
                }
            }
        }
        
        if (childStopOctalCode != NULL) {
            break;
        }
    }
    
    return childStopOctalCode;
}

void VoxelTree::printTreeForDebugging(VoxelNode *startNode) {
    int colorMask = 0;
    
    // create the color mask
    for (int i = 0; i < 8; i++) {
        if (startNode->children[i] != NULL && startNode->children[i]->color[3] != 0) {
            colorMask += (1 << (7 - i));
        }
    }
    
    outputBits(colorMask);
    
    // output the colors we have
    for (int j = 0; j < 8; j++) {
        if (startNode->children[j] != NULL && startNode->children[j]->color[3] != 0) {
            for (int c = 0; c < 3; c++) {
                outputBits(startNode->children[j]->color[c]);
            }
        }
    }
    
    outputBits(startNode->childMask);
    
    // ask children to recursively output their trees
    // if they aren't a leaf
    for (int k = 0; k < 8; k++) {
        if (startNode->children[k] != NULL && oneAtBit(startNode->childMask, k)) {
            printTreeForDebugging(startNode->children[k]);
        }
    }
}