//
//  VoxelTree.cpp
//  hifi
//
//  Created by Stephen Birarda on 3/13/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <cstring>
#include <cmath>
#include "SharedUtil.h"
#include "OctalCode.h"
#include "VoxelTree.h"
#include <iostream> // to load voxels from file
#include <fstream> // to load voxels from file

const int TREE_SCALE = 10;

int boundaryDistanceForRenderLevel(unsigned int renderLevel) {
    switch (renderLevel) {
        case 1:
            return 100;
            break;
        case 2:
            return 50;
            break;
        case 3:
            return 25;
            break;
        case 4:
            return 12;
            break;
        case 5:
            return 6;
            break;
        default:
            return 3;
            break;
    }
}

VoxelTree::VoxelTree() {
    rootNode = new VoxelNode();
    rootNode->octalCode = new unsigned char[1];
    *rootNode->octalCode = 0;
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
    if (*needleCode > 0) {
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

// if node has color & <= 4 children then prune children
void VoxelTree::pruneTree(VoxelNode* pruneAt) {
	int childCount = 0;
    int childMask = 0;
	
	// determine how many children we have
	for (int i = 0; i < 8; i++)	{
		if (pruneAt->children[i]) {
			childCount++;
		}
	}
    
	// if appropriate, prune them
	if (pruneAt->color[3] && childCount <= 4) {
		for (int i = 0; i < 8; i++) {
			if (pruneAt->children[i]) {
				delete pruneAt->children[i];
				pruneAt->children[i]=NULL;
			}
		}
	} else {
		// Otherwise, iterate the children and recursively call
		// prune on all of them
		for (int i = 0; i < 8; i++) {
			if (pruneAt->children[i]) {
				this->pruneTree(pruneAt->children[i]);
			}
		}
	}
	// repair our child mask
	// determine how many children we have
	pruneAt->childMask = 0;
	for (int i = 0; i < 8; i++) {
		if (pruneAt->children[i]) {
			pruneAt->childMask += (1 << (7 - i));
		}
	}
}

int VoxelTree::readNodeData(VoxelNode *destinationNode,
                            unsigned char * nodeData,
                            int bytesLeftToRead) {
    
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
    unsigned char childMask = *(nodeData + bytesRead);
    
    int childIndex = 0;
    bytesRead++;
    
    while (bytesLeftToRead - bytesRead > 0 && childIndex < 8) {
        // check the exists mask to see if we have a child to traverse into
        
        if (oneAtBit(childMask, childIndex)) {            
            if (destinationNode->children[childIndex] == NULL) {
                // add a child at that index, if it doesn't exist
                destinationNode->addChildAtIndex(childIndex);
            }
            
            // tell the child to read the subsequent data
            bytesRead += readNodeData(destinationNode->children[childIndex],
                                      nodeData + bytesRead,
                                      bytesLeftToRead - bytesRead);
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

void VoxelTree::readCodeColorBufferToTree(unsigned char *codeColorBuffer) {
    int octalCodeBytes = bytesRequiredForCodeLength(*codeColorBuffer);
    VoxelNode *lastCreatedNode = nodeForOctalCode(rootNode, codeColorBuffer);
    
    // create the node if it does not exist
    if (*lastCreatedNode->octalCode != *codeColorBuffer) {
        VoxelNode *parentNode = createMissingNode(lastCreatedNode, codeColorBuffer);
        lastCreatedNode = parentNode->children[branchIndexWithDescendant(parentNode->octalCode, codeColorBuffer)];
    }
    
    // give this node its color
    memcpy(lastCreatedNode->color, codeColorBuffer + octalCodeBytes, 3);
    lastCreatedNode->color[3] = 1;
}

unsigned char * VoxelTree::loadBitstreamBuffer(unsigned char *& bitstreamBuffer,
                                               VoxelNode *currentVoxelNode,
                                               MarkerNode *currentMarkerNode,
                                               float * agentPosition,
                                               float thisNodePosition[3],
                                               unsigned char * stopOctalCode)
{
    static unsigned char *initialBitstreamPos = bitstreamBuffer;
    
    unsigned char * childStopOctalCode = NULL;
    
    if (stopOctalCode == NULL) {
        stopOctalCode = rootNode->octalCode;
    }
    
    // check if we have any children
    bool hasAtLeastOneChild;
    
    for (int i = 0; i < 8; i++) {
        if (currentVoxelNode->children[i] != NULL) {
            hasAtLeastOneChild = true;
        }
    }
    
    // if we have at least one child, check if it will be worth recursing into our children
    if (hasAtLeastOneChild) {
        
        int firstIndexToCheck = 0;
        unsigned char * childMaskPointer = NULL;
        
        float halfUnitForVoxel = powf(0.5, *currentVoxelNode->octalCode) * (0.5 * TREE_SCALE);
        
        float distanceToVoxelCenter = sqrtf(powf(agentPosition[0] - thisNodePosition[0] - halfUnitForVoxel, 2) +
                                            powf(agentPosition[1] - thisNodePosition[1] - halfUnitForVoxel, 2) +
                                            powf(agentPosition[2] - thisNodePosition[2] - halfUnitForVoxel, 2));
        
        // if the distance to this voxel's center is less than the threshold
        // distance for its children, we should send the children
        if (distanceToVoxelCenter < boundaryDistanceForRenderLevel(*currentVoxelNode->octalCode + 1)) {
            
            // write this voxel's data if we're below or at
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
                
                // maintain a pointer to this spot in the buffer so we can set our child mask
                // depending on the results of the recursion below
                childMaskPointer = bitstreamBuffer++;
                
                // reset the childMaskPointer for this node to 0
                *childMaskPointer = 0;
            } else {
                firstIndexToCheck = *stopOctalCode > 0
                ? branchIndexWithDescendant(currentVoxelNode->octalCode, stopOctalCode)
                : 0;
            }            
            
            unsigned char  * arrBufferBeforeChild = bitstreamBuffer;
            
            for (int i = firstIndexToCheck; i < 8; i ++) {
                
                // ask the child to load this bitstream buffer
                // if they or their descendants fill the MTU we will receive the childStopOctalCode back
                if (currentVoxelNode->children[i] != NULL) {
                    
                    if (!oneAtBit(currentMarkerNode->childrenVisitedMask, i)) {
                        
                        // create the marker node for this child if it does not yet exist
                        if (currentMarkerNode->children[i] == NULL) {
                            currentMarkerNode->children[i] = new MarkerNode();
                        }
                        
                        // calculate the child's position based on the parent position
                        float childNodePosition[3];
                        
                        for (int j = 0; j < 3; j++) {
                            childNodePosition[j] = thisNodePosition[j];
                            
                            if (oneAtBit(branchIndexWithDescendant(currentVoxelNode->octalCode,
                                                                   currentVoxelNode->children[i]->octalCode),
                                         (7 - j))) {
                                childNodePosition[j] -= (powf(0.5, *currentVoxelNode->children[i]->octalCode) * TREE_SCALE);
                            }
                        }
                        
                        // ask the child to load the bitstream buffer with their data
                        childStopOctalCode = loadBitstreamBuffer(bitstreamBuffer,
                                                                 currentVoxelNode->children[i],
                                                                 currentMarkerNode->children[i],
                                                                 agentPosition,
                                                                 childNodePosition,
                                                                 stopOctalCode);
                        
                        if (bitstreamBuffer - arrBufferBeforeChild > 0) {
                            // this child added data to the packet - add it to our child mask
                            if (childMaskPointer != NULL) {
                                *childMaskPointer += (1 << (7 - i));
                            }
                            
                            arrBufferBeforeChild = bitstreamBuffer;
                        }
                    }
                }
                
                if (childStopOctalCode != NULL) {
                    break;
                } else {
                    // this child node has been covered
                    // add the appropriate bit to the childrenVisitedMask for the current marker node
                    currentMarkerNode->childrenVisitedMask += 1 << (7 - i);
                    
                    // if we are above the stopOctal and we got a NULL code
                    // we cannot go to the next child
                    // so break and return the NULL stop code
                    if (*currentVoxelNode->octalCode < *stopOctalCode) {
                        break;
                    }
                }
            }
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
    
    unsigned char childMask = 0;
    
    for (int k = 0; k < 8; k++) {
        if (startNode->children[k] != NULL) {
            childMask += (1 << (7 - k));
        }
    }
    
    outputBits(childMask);
    
    if (childMask > 0) {
        // ask children to recursively output their trees
        // if they aren't a leaf
        for (int l = 0; l < 8; l++) {
            if (startNode->children[l] != NULL) {
                printTreeForDebugging(startNode->children[l]);
            }
        }
    }   
}
