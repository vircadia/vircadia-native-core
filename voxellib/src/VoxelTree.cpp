//
//  VoxelTree.cpp
//  hifi
//
//  Created by Stephen Birarda on 3/13/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif
#include <cstring>
#include <cstdio>
#include <cmath>
#include "SharedUtil.h"
#include "PacketHeaders.h"
#include "CounterStats.h"
#include "OctalCode.h"
#include "VoxelTree.h"
#include <iostream> // to load voxels from file
#include <fstream> // to load voxels from file


int boundaryDistanceForRenderLevel(unsigned int renderLevel) {
    switch (renderLevel) {
        case 1:
        case 2:
        case 3:
            return 100;
        case 4:
            return 75;
            break;
        case 5:
            return 50;
            break;
        case 6:
            return 25;
            break;
        case 7:
            return 12;
            break;
        default:
            return 6;
            break;
    }
}

VoxelTree::VoxelTree() {
    rootNode = new VoxelNode();
    rootNode->octalCode = new unsigned char[1];
    *rootNode->octalCode = 0;

	// Some stats tracking    
	this->voxelsCreated = 0; // when a voxel is created in the tree (object new'd)
	this->voxelsColored = 0; // when a voxel is colored/set in the tree (object may have already existed)
	this->voxelsBytesRead = 0;
	voxelsCreatedStats.name = "voxelsCreated";
	voxelsColoredStats.name = "voxelsColored";
	voxelsBytesReadStats.name = "voxelsBytesRead";

}

VoxelTree::~VoxelTree() {
    // delete the children of the root node
    // this recursively deletes the tree
    for (int i = 0; i < 8; i++) {
        delete rootNode->children[i];
    }
}

VoxelNode * VoxelTree::nodeForOctalCode(VoxelNode *ancestorNode, unsigned char * needleCode, VoxelNode** parentOfFoundNode) {
    // find the appropriate branch index based on this ancestorNode
    if (*needleCode > 0) {
        int branchForNeedle = branchIndexWithDescendant(ancestorNode->octalCode, needleCode);
        VoxelNode *childNode = ancestorNode->children[branchForNeedle];
        
        if (childNode != NULL) {
            if (*childNode->octalCode == *needleCode) {
            
            	// If the caller asked for the parent, then give them that too...
            	if (parentOfFoundNode) {
					*parentOfFoundNode=ancestorNode;
				}
                // the fact that the number of sections is equivalent does not always guarantee
                // that this is the same node, however due to the recursive traversal
                // we know that this is our node
                return childNode;
            } else {
                // we need to go deeper
                return nodeForOctalCode(childNode, needleCode,parentOfFoundNode);
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

// BHG Notes: We appear to call this function for every Voxel Node getting created.
// This is recursive in nature. So, for example, if we are given an octal code for
// a 1/256th size voxel, we appear to call this function 8 times. Maybe??
int VoxelTree::readNodeData(VoxelNode *destinationNode,
                            unsigned char * nodeData,
                            int bytesLeftToRead) {
    
    // instantiate variable for bytes already read
    int bytesRead = 1;
    for (int i = 0; i < 8; i++) {
        // check the colors mask to see if we have a child to color in
        if (oneAtBit(*nodeData, i)) {
            
            // create the child if it doesn't exist
            if (destinationNode->children[i] == NULL) {
                destinationNode->addChildAtIndex(i);
                this->voxelsCreated++;
                this->voxelsCreatedStats.recordSample(this->voxelsCreated);
            }
            
            // pull the color for this child
            memcpy(destinationNode->children[i]->color, nodeData + bytesRead, 3);
            destinationNode->children[i]->color[3] = 1;
			this->voxelsColored++;
			this->voxelsColoredStats.recordSample(this->voxelsColored);
           
            bytesRead += 3;
        }
    }
    
    // average node's color based on color of children
    destinationNode->setColorFromAverageOfChildren();
    
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
                this->voxelsCreated++;
                this->voxelsCreatedStats.recordSample(this->voxelsCreated);
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
    VoxelNode *bitstreamRootNode = nodeForOctalCode(rootNode, (unsigned char *)bitstream, NULL);
    
    if (*bitstream != *bitstreamRootNode->octalCode) {
        // if the octal code returned is not on the same level as
        // the code being searched for, we have VoxelNodes to create
        bitstreamRootNode = createMissingNode(bitstreamRootNode, (unsigned char *)bitstream);
    }
    
    int octalCodeBytes = bytesRequiredForCodeLength(*bitstream);
    readNodeData(bitstreamRootNode, bitstream + octalCodeBytes, bufferSizeBytes - octalCodeBytes);
    
    this->voxelsBytesRead += bufferSizeBytes;
	this->voxelsBytesReadStats.recordSample(this->voxelsBytesRead);
}

// Note: uses the codeColorBuffer format, but the color's are ignored, because
// this only finds and deletes the node from the tree.
void VoxelTree::deleteVoxelCodeFromTree(unsigned char *codeBuffer) {
	VoxelNode* parentNode = NULL;
    VoxelNode* nodeToDelete = nodeForOctalCode(rootNode, codeBuffer, &parentNode);
    
    printf("deleteVoxelCodeFromTree() looking [codeBuffer] for:\n");
    printOctalCode(codeBuffer);

    printf("deleteVoxelCodeFromTree() found [nodeToDelete->octalCode] for:\n");
    printOctalCode(nodeToDelete->octalCode);
    
    // If the node exists...
	int lengthInBytes = bytesRequiredForCodeLength(*codeBuffer); // includes octet count, not color!
	printf("compare octal codes of length %d\n",lengthInBytes);

    if (0==memcmp(nodeToDelete->octalCode,codeBuffer,lengthInBytes)) {
    	printf("found node to delete...\n");

		float* vertices = firstVertexForCode(nodeToDelete->octalCode);
		printf("deleting voxel at: %f,%f,%f\n",vertices[0],vertices[1],vertices[2]);
		delete []vertices;

		if (parentNode) {
			float* vertices = firstVertexForCode(parentNode->octalCode);
			printf("parent of deleting voxel at: %f,%f,%f\n",vertices[0],vertices[1],vertices[2]);
			delete []vertices;
			
			int childNDX = branchIndexWithDescendant(parentNode->octalCode, codeBuffer);
			printf("child INDEX=%d\n",childNDX);

			printf("deleting Node at parentNode->children[%d]\n",childNDX);
			delete parentNode->children[childNDX]; // delete the child nodes
			printf("setting parentNode->children[%d] to NULL\n",childNDX);
			parentNode->children[childNDX]=NULL; // set it to NULL

			printf("reaverageVoxelColors()\n");
			reaverageVoxelColors(rootNode); // Fix our colors!! Need to call it on rootNode
		}
    }
}

void VoxelTree::eraseAllVoxels() {

	// XXXBHG Hack attack - is there a better way to erase the voxel tree?

	delete rootNode; // this will recurse and delete all children
	rootNode = new VoxelNode();
	rootNode->octalCode = new unsigned char[1];
	*rootNode->octalCode = 0;
}

void VoxelTree::readCodeColorBufferToTree(unsigned char *codeColorBuffer) {
    VoxelNode *lastCreatedNode = nodeForOctalCode(rootNode, codeColorBuffer, NULL);
    
    // create the node if it does not exist
    if (*lastCreatedNode->octalCode != *codeColorBuffer) {
        VoxelNode *parentNode = createMissingNode(lastCreatedNode, codeColorBuffer);
        lastCreatedNode = parentNode->children[branchIndexWithDescendant(parentNode->octalCode, codeColorBuffer)];
    }
    
    // give this node its color
    int octalCodeBytes = bytesRequiredForCodeLength(*codeColorBuffer);
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

		// XXXBHG - Note: It appears as if the X and Z coordinates of Head or Agent are flip-flopped relative to the 
		// coords of the voxel space. This flip flop causes LOD behavior to be extremely odd. This is my temporary hack 
		// to fix this behavior. To disable this swap, set swapXandZ to false.
		// XXXBHG - 2013/04/11 - adding a note to my branch, I think this code is now broken.
        bool swapXandZ=true;
        float agentX = swapXandZ ? agentPosition[2] : agentPosition[0];
        float agentZ = swapXandZ ? agentPosition[0] : agentPosition[2];
        
        float distanceToVoxelCenter = sqrtf(powf(agentX - thisNodePosition[0] - halfUnitForVoxel, 2) +
                                            powf(agentPosition[1] - thisNodePosition[1] - halfUnitForVoxel, 2) +
                                            powf(agentZ - thisNodePosition[2] - halfUnitForVoxel, 2));
        
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
                    *(bitstreamBuffer++) = PACKET_HEADER_VOXEL_DATA;
                    
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

void VoxelTree::processRemoveVoxelBitstream(unsigned char * bitstream, int bufferSizeBytes) {
	// XXXBHG: validate buffer is at least 4 bytes long? other guards??
	unsigned short int itemNumber = (*((unsigned short int*)&bitstream[1]));
	printf("processRemoveVoxelBitstream() receivedBytes=%d itemNumber=%d\n",bufferSizeBytes,itemNumber);
	int atByte = 3;
	unsigned char* pVoxelData = (unsigned char*)&bitstream[3];
	while (atByte < bufferSizeBytes) {
		unsigned char octets = (unsigned char)*pVoxelData;
		int voxelDataSize = bytesRequiredForCodeLength(octets)+3; // 3 for color!

		float* vertices = firstVertexForCode(pVoxelData);
		printf("deleting voxel at: %f,%f,%f\n",vertices[0],vertices[1],vertices[2]);
		delete []vertices;

		deleteVoxelCodeFromTree(pVoxelData);

		pVoxelData+=voxelDataSize;
		atByte+=voxelDataSize;
	}
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

void VoxelTree::reaverageVoxelColors(VoxelNode *startNode) {
    bool hasChildren = false;
    
    for (int i = 0; i < 8; i++) {
        if (startNode->children[i] != NULL) {
            reaverageVoxelColors(startNode->children[i]);
            hasChildren = true;
        }
    }
    
    if (hasChildren) {
    	bool childrenCollapsed = startNode->collapseIdenticalLeaves();
        
    	if (!childrenCollapsed) {
	        startNode->setColorFromAverageOfChildren();
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

            printf("voxel color from file  red:%d, green:%d, blue:%d \n",red,green,blue);
            vCount++;

            int colorRandomizer = wantColorRandomizer ? randIntInRange (-5, 5) : 0;
            voxelData[lengthInBytes+1] = std::max(0,std::min(255,red + colorRandomizer));
            voxelData[lengthInBytes+2] = std::max(0,std::min(255,green + colorRandomizer));
            voxelData[lengthInBytes+3] = std::max(0,std::min(255,blue + colorRandomizer));
            printf("voxel color after rand red:%d, green:%d, blue:%d\n",
                   voxelData[lengthInBytes+1], voxelData[lengthInBytes+2], voxelData[lengthInBytes+3]);

            //printVoxelCode(voxelData);
            this->readCodeColorBufferToTree(voxelData);
            delete voxelData;
        }
        file.close();
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// Method:      VoxelTree::createSphere()
// Description: Creates a sphere of voxels in the local system at a given location/radius
// To Do:       Move this function someplace better?
// Complaints:  Brad :)
void VoxelTree::createSphere(float r,float xc, float yc, float zc, float s, bool solid, bool wantColorRandomizer) {
    // About the color of the sphere... we're going to make this sphere be a gradient
    // between two RGB colors. We will do the gradient along the phi spectrum
    unsigned char dominantColor1 = randIntInRange(1,3); //1=r, 2=g, 3=b dominant
    unsigned char dominantColor2 = randIntInRange(1,3);
    
    if (dominantColor1==dominantColor2) {
    	dominantColor2 = dominantColor1+1%3;
    }
    
    unsigned char r1 = (dominantColor1==1)?randIntInRange(200,255):randIntInRange(40,100);
    unsigned char g1 = (dominantColor1==2)?randIntInRange(200,255):randIntInRange(40,100);
    unsigned char b1 = (dominantColor1==3)?randIntInRange(200,255):randIntInRange(40,100);
    unsigned char r2 = (dominantColor2==1)?randIntInRange(200,255):randIntInRange(40,100);
    unsigned char g2 = (dominantColor2==2)?randIntInRange(200,255):randIntInRange(40,100);
    unsigned char b2 = (dominantColor2==3)?randIntInRange(200,255):randIntInRange(40,100);

	// We initialize our rgb to be either "grey" in case of randomized surface, or
	// the average of the gradient, in the case of the gradient sphere.
    unsigned char red   = wantColorRandomizer ? 128 : (r1+r2)/2; // average of the colors
    unsigned char green = wantColorRandomizer ? 128 : (g1+g2)/2;
    unsigned char blue  = wantColorRandomizer ? 128 : (b1+b2)/2;
    
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
	for (; ri <= (r+(s/2.0)); ri+=s) {
		//printf("radius: ri=%f ri+s=%f (r+(s/2.0))=%f\n",ri,ri+s,(r+(s/2.0)));
		for (float theta=0.0; theta <= 2*M_PI; theta += angleDelta) {
			for (float phi=0.0; phi <= M_PI; phi += angleDelta) {
				t++; // total voxels
				float x = xc+ri*cos(theta)*sin(phi);
				float y = yc+ri*sin(theta)*sin(phi);
				float z = zc+ri*cos(phi);
                
                // gradient color data
                float gradient = (phi/M_PI);
                
                // only use our actual desired color on the outer edge, otherwise
                // use our "average" color
                if (ri+(s*2.0)>=r) {
					//printf("painting candy shell radius: ri=%f r=%f\n",ri,r);
					red   = wantColorRandomizer ? randomColorValue(165) : r1+((r2-r1)*gradient);
					green = wantColorRandomizer ? randomColorValue(165) : g1+((g2-g1)*gradient);
					blue  = wantColorRandomizer ? randomColorValue(165) : b1+((b2-b1)*gradient);
				}				
				
				unsigned char* voxelData = pointToVoxel(x,y,z,s,red,green,blue);
                this->readCodeColorBufferToTree(voxelData);
				//printf("voxel data for x:%f y:%f z:%f s:%f\n",x,y,z,s);
                //printVoxelCode(voxelData);
                delete voxelData;
			}
		}
	}
	this->reaverageVoxelColors(this->rootNode);
}
