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

int VoxelTree::levelForViewerPosition(float *position) {
    // get the distance to the viewer
    // for now the voxel tree starts at 0,0,0
    float distance = sqrtf(powf(position[0] + 30, 2) + powf(position[2] + 30, 2));
    
    // go through the if else branch to return the right level
    // this is a gross way to do this for now for a friday demo
    if (distance >= 50) {
        return 3;
    } else if (distance >= 20) {
        return 4;
    } else {
        return 5;
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

/// If node has color & <= 4 children then prune children
void VoxelTree::pruneTree(VoxelNode* pruneAt) {
	int childCount = 0;
    int childMask = 0;
	
	// determine how many children we have
	for (int i = 0; i < 8; i++) {
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
                                               unsigned char * stopOctalCode,
                                               VoxelNode *currentVoxelNode,
                                               int deepestLevel)
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
        *(bitstreamBuffer++) = *currentVoxelNode->octalCode < deepestLevel - 1
            ? currentVoxelNode->childMask
            : 0;
    } else {
        firstIndexToCheck = *stopOctalCode > 0
            ? branchIndexWithDescendant(currentVoxelNode->octalCode, stopOctalCode)
            : 0;
    }
    
    unsigned char * childStopOctalCode = NULL;
    
    if (currentVoxelNode->childMask == 0) {
        leavesWrittenToBitstream++;
    }
    
    if (*currentVoxelNode->octalCode < deepestLevel - 1) {
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
                        childStopOctalCode = loadBitstreamBuffer(bitstreamBuffer, stopOctalCode, currentVoxelNode->children[i], deepestLevel);
                    } else {
                        childStopOctalCode = NULL;
                    }
                }
            }
            
            if (childStopOctalCode != NULL) {
                break;
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
    
    outputBits(startNode->childMask);
    
    // ask children to recursively output their trees
    // if they aren't a leaf
    for (int k = 0; k < 8; k++) {
        if (startNode->children[k] != NULL) {
            printTreeForDebugging(startNode->children[k]);
        }
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// Method:      VoxelTree::loadVoxelsFile()
// Description: Loads HiFidelity encoded Voxels from a binary file. The current file
//              format is a stream of single voxels with color data.
// Complaints:  Brad :)
void VoxelTree::loadVoxelsFile(const char* fileName, bool wantColorRandomizer) {
    int vCount = 0;

    std::ifstream file(fileName, std::ios::in|std::ios::binary);

    char octets;
    unsigned int lengthInBytes;
    
    int totalBytesRead = 0;
    if(file.is_open()) {
		printf("loading file...\n");    
        bool bail = false;
        while (!file.eof() && !bail) {
            file.get(octets);
			//printf("octets=%d...\n",octets);    
            totalBytesRead++;
            lengthInBytes = bytesRequiredForCodeLength(octets)-1; //(octets*3/8)+1;
            unsigned char * voxelData = new unsigned char[lengthInBytes+1+3];
            voxelData[0]=octets;
            char byte;

            for (size_t i = 0; i < lengthInBytes; i++) {
                file.get(byte);
                totalBytesRead++;
                voxelData[i+1] = byte;
            }
            // read color data
            char colorRead;
            unsigned char red,green,blue;
            file.get(colorRead);
            red = (unsigned char)colorRead;
            file.get(colorRead);
            green = (unsigned char)colorRead;
            file.get(colorRead);
            blue = (unsigned char)colorRead;

            printf("voxel color from file  red:%d\, green:%d, blue:%d \n",red,green,blue);
            vCount++;
            //printf("vCount:%d\n",vCount);

            int colorRandomizer = wantColorRandomizer ? randIntInRange (-5, 5) : 0;
            voxelData[lengthInBytes+1] = std::max(0,std::min(255,red + colorRandomizer));
            voxelData[lengthInBytes+2] = std::max(0,std::min(255,green + colorRandomizer));
            voxelData[lengthInBytes+3] = std::max(0,std::min(255,blue + colorRandomizer));
            printf("voxel color after rand red:%d\, green:%d, blue:%d\n",voxelData[lengthInBytes+1],voxelData[lengthInBytes+2],voxelData[lengthInBytes+3]);

            //printVoxelCode(voxelData);
            this->readCodeColorBufferToTree(voxelData);
            delete voxelData;
        }
        file.close();
    }
    
    this->pruneTree(this->rootNode);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Method:      VoxelTree::createSphere()
// Description: Creates a sphere of voxels in the local system at a given location/radius
// To Do:       Move this function someplace better? Do we really need to call pruneTree()??
// Complaints:  Brad :)
void VoxelTree::createSphere(float r,float xc, float yc, float zc, float s, bool solid) {
    // About the color of the sphere... we're going to make this sphere be a gradient
    // between two RGB colors. We will do the gradient along the phi spectrum
    unsigned char r1 = randomColorValue(165);
    unsigned char g1 = randomColorValue(165);
    unsigned char b1 = randomColorValue(165);
    unsigned char r2 = randomColorValue(65);
    unsigned char g2 = randomColorValue(65);
    unsigned char b2 = randomColorValue(65);
    
    // we don't want them to match!!
    if (r1==r2 && g1==g2 && b1==b2) {
        r2=r1/2;
        g2=g1/2;
        b2=b1/2;
    }

    // Psuedocode for creating a sphere: 
    //
    // for (theta from 0 to 2pi):
    //     for (phi from 0 to pi):
	//          x = xc+r*cos(theta)*sin(phi)
	//          y = yc+r*sin(theta)*sin(phi)
	//          z = zc+r*cos(phi)
	
	int t=0; // total points

    // We want to make sure that as we "sweep" through our angles we use a delta angle that's small enough to not skip any 
    // voxels we can calculate theta from our desired arc length
	//      lenArc = ndeg/360deg * 2pi*R  --->  lenArc = theta/2pi * 2pi*R
	//      lenArc = theta*R ---> theta = lenArc/R ---> theta = g/r	
	float angleDelta = (s/r);

	// assume solid for now
	float ri = 0.0;
	if (!solid) {
		ri=r; // just the outer surface
	}
	// If you also iterate form the interior of the sphere to the radius, makeing
	// larger and larger sphere's you'd end up with a solid sphere. And lots of voxels!
	for (; ri <= r; ri+=s) {
		for (float theta=0.0; theta <= 2*M_PI; theta += angleDelta) {
			for (float phi=0.0; phi <= M_PI; phi += angleDelta) {
				t++; // total voxels
				float x = xc+r*cos(theta)*sin(phi);
				float y = yc+r*sin(theta)*sin(phi);
				float z = zc+r*cos(phi);
                
                // gradient color data
                float gradient = (phi/M_PI);
                unsigned char red   = r1+((r2-r1)*gradient);
                unsigned char green = g1+((g2-g1)*gradient);
                unsigned char blue  = b1+((b2-b1)*gradient);
				
				unsigned char* voxelData = pointToVoxel(x,y,z,s,red,green,blue);
                this->readCodeColorBufferToTree(voxelData);
                delete voxelData;
			}
		}
	}
    this->pruneTree(this->rootNode); // XXXBHG Hack: Need to call this for now???
}
