//
//  VoxelTree.cpp
//  hifi
//
//  Created by Stephen Birarda on 3/13/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//
//
// Oct-Tree in bits, and child ordinals
//
// Location             Decimal  Bit  Child in Array
// -------------------  -------  ---  ---------------
// Bottom Right Near    128      8th      0
// Bottom Right Far     64       7th      1
// Top    Right Near    32       6th      2
// Top    Right Far     16       5th      3
// Bottom Left  Near    8        4th      4
// Bottom Left  Far     4        3rd      5
// Top    Left  Near    2        2nd      6
// Top    Left  Far     1        1st      7
// 
//                        ^
//                +-------|------+ + Y
//               /      / |     /|
//              /   1  /  | 16 / |
//             /------/-------+  | 
//            /      /    |  /|16| 
//           /   2  /  32 | / |  |
//          /      /      |/  | /|     / +Z
//         +--------------+ 32|/ |    / 
//         |      |       |   /  |   /
//         |      |       |  /|  |  /
//         |   2  |   32  | / |64| /  
//    4----|      |       |/  |  |/
//         +------|------ +   |  /
//         |      |       |   | /
//         |      |       |128|/
//         |   8  |  128  |   /
//         |      |       |  /
//         |      |       | /
// < - - - +------+-------+/ - - - - - - >
//   +X                (0,0,0)       -X         
//                       /|
//                      / |
//                     /  |
//                 -Z /   V -Y
//
//

#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif
#include <cstring>
#include <cstdio>
#include <cmath>
#include "SharedUtil.h"
#include "voxels_Log.h"
#include "PacketHeaders.h"
#include "OctalCode.h"
#include "VoxelTree.h"
#include "VoxelNodeBag.h"
#include "ViewFrustum.h"
#include <fstream> // to load voxels from file
#include "VoxelConstants.h"

using voxels_lib::printLog;

int boundaryDistanceForRenderLevel(unsigned int renderLevel) {
    float voxelSizeScale = 5000.0;
    return voxelSizeScale / powf(2, renderLevel);
}

VoxelTree::VoxelTree() :
    voxelsCreated(0),
    voxelsColored(0),
    voxelsBytesRead(0),
    voxelsCreatedStats(100),
    voxelsColoredStats(100),
    voxelsBytesReadStats(100) {
        
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

// Recurses voxel tree calling the RecurseVoxelTreeOperation function for each node.
// stops recursion if operation function returns false.
void VoxelTree::recurseTreeWithOperation(RecurseVoxelTreeOperation operation, void* extraData) {
    recurseNodeWithOperation(rootNode, operation,extraData);
}

// Recurses voxel node with an operation function
void VoxelTree::recurseNodeWithOperation(VoxelNode* node,RecurseVoxelTreeOperation operation, void* extraData) {
    // call the operation function going "down" first, stop deeper recursion if function returns false
    if (operation(node,true,extraData)) {
        for (int i = 0; i < sizeof(node->children)/sizeof(node->children[0]); i++) {
            VoxelNode* child = node->children[i];
            if (child) {
                recurseNodeWithOperation(child,operation,extraData);
            }
        }
        // call operation on way back up
        operation(node,false,extraData);
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

// returns the node created!
VoxelNode* VoxelTree::createMissingNode(VoxelNode* lastParentNode, unsigned char* codeToReach) {

    int indexOfNewChild = branchIndexWithDescendant(lastParentNode->octalCode, codeToReach);
    
    // we could be coming down a branch that was already created, so don't stomp on it.
    if (lastParentNode->children[indexOfNewChild] == NULL) {
        lastParentNode->addChildAtIndex(indexOfNewChild);
    }

    // This works because we know we traversed down the same tree so if the length is the same, then the whole code is the same
    if (*lastParentNode->children[indexOfNewChild]->octalCode == *codeToReach) {
        return lastParentNode->children[indexOfNewChild];
    } else {
        return createMissingNode(lastParentNode->children[indexOfNewChild], codeToReach);
    }
}

// BHG Notes: We appear to call this function for every Voxel Node getting created.
// This is recursive in nature. So, for example, if we are given an octal code for
// a 1/256th size voxel, we appear to call this function 8 times. Maybe??
int VoxelTree::readNodeData(VoxelNode* destinationNode,
                            unsigned char* nodeData,
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
                this->voxelsCreatedStats.updateAverage(1);
            }

            // pull the color for this child
            nodeColor newColor;
            memcpy(newColor, nodeData + bytesRead, 3);
            newColor[3] = 1;
            destinationNode->children[i]->setColor(newColor);
			this->voxelsColored++;
			this->voxelsColoredStats.updateAverage(1);
           
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
                this->voxelsCreatedStats.updateAverage(this->voxelsCreated);
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
    int bytesRead = 0;
    unsigned char* bitstreamAt = bitstream;

    // Keep looping through the buffer calling readNodeData() this allows us to pack multiple root-relative Octal codes
    // into a single network packet. readNodeData() basically goes down a tree from the root, and fills things in from there
    // if there are more bytes after that, it's assumed to be another root relative tree

    while (bitstreamAt < bitstream+bufferSizeBytes) {
        VoxelNode* bitstreamRootNode = nodeForOctalCode(rootNode, (unsigned char *)bitstreamAt, NULL);
        if (*bitstreamAt != *bitstreamRootNode->octalCode) {
            // if the octal code returned is not on the same level as
            // the code being searched for, we have VoxelNodes to create

            // Note: we need to create this node relative to root, because we're assuming that the bitstream for the initial
            // octal code is always relative to root!
            bitstreamRootNode = createMissingNode(rootNode, (unsigned char *)bitstreamAt);
        }

        int octalCodeBytes = bytesRequiredForCodeLength(*bitstreamAt);
        int theseBytesRead = 0;
        theseBytesRead += octalCodeBytes;
        theseBytesRead += readNodeData(bitstreamRootNode, bitstreamAt + octalCodeBytes, bufferSizeBytes - (bytesRead+octalCodeBytes));

        // skip bitstream to new startPoint
        bitstreamAt += theseBytesRead;
        bytesRead +=  theseBytesRead;
    }

    this->voxelsBytesRead += bufferSizeBytes;
    this->voxelsBytesReadStats.updateAverage(bufferSizeBytes);
}

// Note: uses the codeColorBuffer format, but the color's are ignored, because
// this only finds and deletes the node from the tree.
void VoxelTree::deleteVoxelCodeFromTree(unsigned char *codeBuffer) {
	VoxelNode* parentNode = NULL;
    VoxelNode* nodeToDelete = nodeForOctalCode(rootNode, codeBuffer, &parentNode);
    
    // If the node exists...
	int lengthInBytes = bytesRequiredForCodeLength(*codeBuffer); // includes octet count, not color!

    if (0 == memcmp(nodeToDelete->octalCode,codeBuffer,lengthInBytes)) {

		float* vertices = firstVertexForCode(nodeToDelete->octalCode);
		delete []vertices;

		if (parentNode) {
			float* vertices = firstVertexForCode(parentNode->octalCode);
			delete []vertices;
			
			int childNDX = branchIndexWithDescendant(parentNode->octalCode, codeBuffer);

            delete parentNode->children[childNDX]; // delete the child nodes
            parentNode->children[childNDX]=NULL; // set it to NULL

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
    VoxelNode* lastCreatedNode = nodeForOctalCode(rootNode, codeColorBuffer, NULL);

    // create the node if it does not exist
    if (*lastCreatedNode->octalCode != *codeColorBuffer) {
        lastCreatedNode = createMissingNode(lastCreatedNode, codeColorBuffer);
    }

    // give this node its color
    int octalCodeBytes = bytesRequiredForCodeLength(*codeColorBuffer);

    nodeColor newColor;
    memcpy(newColor, codeColorBuffer + octalCodeBytes, 3);
    newColor[3] = 1;
    lastCreatedNode->setColor(newColor);
}

unsigned char * VoxelTree::loadBitstreamBuffer(unsigned char *& bitstreamBuffer,
                                               VoxelNode *currentVoxelNode,
                                               MarkerNode *currentMarkerNode,
                                               const glm::vec3& agentPosition,
                                               float thisNodePosition[3],
                                               const ViewFrustum& viewFrustum,
                                               bool viewFrustumCulling,
                                               unsigned char * stopOctalCode)
{
    unsigned char * childStopOctalCode = NULL;
    static unsigned char *initialBitstreamPos = bitstreamBuffer;

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
                    // Rules for including a child:
                    // 1) child must exists
                    if ((currentVoxelNode->children[i] != NULL)) {
                        // 2) child must have a color...
                        if (currentVoxelNode->children[i]->isColored()) {
                            unsigned char* childOctalCode = currentVoxelNode->children[i]->octalCode;

                            float childPosition[3];
                            copyFirstVertexForCode(childOctalCode,(float*)&childPosition);
                            childPosition[0] *= TREE_SCALE; // scale it up
                            childPosition[1] *= TREE_SCALE; // scale it up
                            childPosition[2] *= TREE_SCALE; // scale it up
                    
                            float halfChildVoxel = powf(0.5, *childOctalCode) * (0.5 * TREE_SCALE);
                            float fullChildVoxel = halfChildVoxel * 2.0f;
                            AABox childBox;
                            childBox.setBox(glm::vec3(childPosition[0], childPosition[1], childPosition[2]),
                                fullChildVoxel, fullChildVoxel, fullChildVoxel);
    
                            bool childInView  = !viewFrustumCulling || 
                                (ViewFrustum::OUTSIDE != viewFrustum.boxInFrustum(childBox));

                            /// XXXBHG - debug code, switch this to true, and we'll send everything but include false coloring
                            // on voxels based on whether or not they match these rules.
                            bool falseColorInsteadOfCulling = false;
                            bool sendChild =  childInView || falseColorInsteadOfCulling; 
                        
                            // if we sendAnyway, we'll do false coloring of the voxels based on childInView
                            if (sendChild) {
                    
                                // copy in the childs color to bitstreamBuffer
                                if (childInView) {
                                    // true color
                                    memcpy(colorPointer, currentVoxelNode->children[i]->getTrueColor(), 3);
                                } else {
                                    unsigned char red[3]   = {255,0,0};
                                    if (!childInView) {
                                        // If not in view, color them red
                                        memcpy(colorPointer, red, 3);
                                    }
                                }
                                colorPointer += 3;
                    
                                // set the colorMask by bitshifting the value of childExists
                                *bitstreamBuffer += (1 << (7 - i));
                            }
                        }
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
                    
                        float childNodePosition[3];
                        copyFirstVertexForCode(currentVoxelNode->children[i]->octalCode,(float*)&childNodePosition);
                        childNodePosition[0] *= TREE_SCALE; // scale it up
                        childNodePosition[1] *= TREE_SCALE; // scale it up
                        childNodePosition[2] *= TREE_SCALE; // scale it up
            
                        // ask the child to load the bitstream buffer with their data
                        childStopOctalCode = loadBitstreamBuffer(bitstreamBuffer,
                                                                 currentVoxelNode->children[i],
                                                                 currentMarkerNode->children[i],
                                                                 agentPosition,
                                                                 childNodePosition,
                                                                 viewFrustum,
                                                                 viewFrustumCulling,
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
	printLog("processRemoveVoxelBitstream() receivedBytes=%d itemNumber=%d\n",bufferSizeBytes,itemNumber);
	int atByte = 3;
	unsigned char* pVoxelData = (unsigned char*)&bitstream[3];
	while (atByte < bufferSizeBytes) {
		unsigned char octets = (unsigned char)*pVoxelData;
		int voxelDataSize = bytesRequiredForCodeLength(octets)+3; // 3 for color!

		float* vertices = firstVertexForCode(pVoxelData);
		printLog("deleting voxel at: %f,%f,%f\n",vertices[0],vertices[1],vertices[2]);
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
        if (startNode->children[i] != NULL && startNode->children[i]->isColored()) {
            colorMask += (1 << (7 - i));
        }
    }

    printLog("color mask: ");
    outputBits(colorMask);

    // output the colors we have
    for (int j = 0; j < 8; j++) {
        if (startNode->children[j] != NULL && startNode->children[j]->isColored()) {
            printLog("color %d : ",j);
            for (int c = 0; c < 3; c++) {
                outputBits(startNode->children[j]->getTrueColor()[c],false);
            }
            startNode->children[j]->printDebugDetails("");
        }
    }

    unsigned char childMask = 0;

    for (int k = 0; k < 8; k++) {
        if (startNode->children[k] != NULL) {
            childMask += (1 << (7 - k));
        }
    }

    printLog("child mask: ");
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
		printLog("loading file...\n");    
        bool bail = false;
        while (!file.eof() && !bail) {
            file.get(octets);
			//printLog("octets=%d...\n",octets);    
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

            printLog("voxel color from file  red:%d, green:%d, blue:%d \n",red,green,blue);
            vCount++;

            int colorRandomizer = wantColorRandomizer ? randIntInRange (-5, 5) : 0;
            voxelData[lengthInBytes+1] = std::max(0,std::min(255,red + colorRandomizer));
            voxelData[lengthInBytes+2] = std::max(0,std::min(255,green + colorRandomizer));
            voxelData[lengthInBytes+3] = std::max(0,std::min(255,blue + colorRandomizer));
            printLog("voxel color after rand red:%d, green:%d, blue:%d\n",
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
		//printLog("radius: ri=%f ri+s=%f (r+(s/2.0))=%f\n",ri,ri+s,(r+(s/2.0)));
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
					//printLog("painting candy shell radius: ri=%f r=%f\n",ri,r);
					red   = wantColorRandomizer ? randomColorValue(165) : r1+((r2-r1)*gradient);
					green = wantColorRandomizer ? randomColorValue(165) : g1+((g2-g1)*gradient);
					blue  = wantColorRandomizer ? randomColorValue(165) : b1+((b2-b1)*gradient);
				}				
				
				unsigned char* voxelData = pointToVoxel(x,y,z,s,red,green,blue);
                this->readCodeColorBufferToTree(voxelData);
                delete voxelData;
			}
		}
	}
	this->reaverageVoxelColors(this->rootNode);
}

// This will encode a larger tree into multiple subtree bitstreams. Given a node it will search for deeper subtrees that 
// have color. It will search for sub trees, and upon finding a subTree, it will stick the node in the bag to for later
// endcoding. It returns the maximum level that we reached during our search. That might be the maximum level we were asked
// to search (if the tree is deeper than that max level), or it will be the maximum level of the tree (if we asked to search
// deeper than the tree exists).
int VoxelTree::searchForColoredNodes(int maxSearchLevel, VoxelNode* node, const ViewFrustum& viewFrustum, VoxelNodeBag& bag) {

    // call the recursive version, this will add all found colored node roots to the bag
    int currentSearchLevel = 0;
    
    // levelReached will be the maximum level reached. If we made it to the maxSearchLevel, then it will be that.
    // but if the tree is shallower than the maxSearchLevel, then we will return the deepest level of the tree that
    // exists.
    int levelReached = searchForColoredNodesRecursion(maxSearchLevel, currentSearchLevel, rootNode, viewFrustum, bag);
    return levelReached;
}



int VoxelTree::searchForColoredNodesRecursion(int maxSearchLevel, int& currentSearchLevel, 
    VoxelNode* node, const ViewFrustum& viewFrustum, VoxelNodeBag& bag) {

    // Keep track of how deep we've searched.    
    currentSearchLevel++;

    // If we've reached our max Search Level, then stop searching.
    if (currentSearchLevel >= maxSearchLevel) {
        return currentSearchLevel;
    }

    // If we're at a node that is out of view, then we can return, because no nodes below us will be in view!
    if (!node->isInView(viewFrustum)) {
        return currentSearchLevel;
    }
    
    // Ok, this is a little tricky, each child may have been deeper than the others, so we need to track
    // how deep each child went. And we actually return the maximum of each child. We use these variables below
    // when we recurse the children.
    int thisLevel = currentSearchLevel;
    int maxChildLevel = thisLevel;
    
    const int  MAX_CHILDREN = 8;
    VoxelNode* inViewChildren[MAX_CHILDREN];
    float      distancesToChildren[MAX_CHILDREN];
    int        positionOfChildren[MAX_CHILDREN];
    int        inViewCount = 0;
    int        inViewNotLeafCount = 0;
    int        inViewWithColorCount = 0;
    
    // for each child node, check to see if they exist, are colored, and in view, and if so
    // add them to our distance ordered array of children
    for (int i = 0; i < MAX_CHILDREN; i++) {
        VoxelNode* childNode = node->children[i];
        bool childExists = (childNode != NULL);
        bool childIsColored = (childExists && childNode->isColored());
        bool childIsInView  = (childExists && childNode->isInView(viewFrustum));
        bool childIsLeaf    = (childExists && childNode->isLeaf());
        
        if (childIsInView) {
            
            // track children in view as existing and not a leaf
            if (!childIsLeaf) {
                inViewNotLeafCount++;
            }
            
            // track children with actual color
            if (childIsColored) {
                inViewWithColorCount++;
            }
        
            float distance = childNode->distanceToCamera(viewFrustum);
            
            if (distance < boundaryDistanceForRenderLevel(*childNode->octalCode + 1)) {
                inViewCount = insertIntoSortedArrays((void*)childNode, distance, i, 
                                    (void**)&inViewChildren, (float*)&distancesToChildren, (int*)&positionOfChildren,
                                    inViewCount, MAX_CHILDREN);
            }
        }
    }

    // If we have children with color, then we do want to add this node (and it's descendants) to the bag to be written
    // we don't need to dig deeper.
    //
    // XXXBHG - this might be a good time to look at colors and add them to a dictionary? But we're not planning
    // on scanning the whole tree, so we won't actually see all the colors, so maybe no point in that.
    if (inViewWithColorCount) {
        bag.insert(node);
    } else {
        // at this point, we need to iterate the children who are in view, even if not colored
        // and we need to determine if there's a deeper tree below them that we care about. We will iterate
        // these based on which tree is closer. 
        for (int i = 0; i < inViewCount; i++) {
            VoxelNode* childNode = inViewChildren[i];
            thisLevel = currentSearchLevel; // reset this, since the children will munge it up
            int childLevelReached = searchForColoredNodesRecursion(maxSearchLevel, thisLevel, childNode, viewFrustum, bag);
            maxChildLevel = std::max(maxChildLevel, childLevelReached);
        }
    }
    return maxChildLevel;
}

// This will encode a tree bitstream, given a node it will encode the full tree from that point onward. 
// It will ignore any branches that are not in view. But other than that, it will not (can not) do any
// prioritization of branches or deeper searching of the branches for optimization.
//
// NOTE: This STUB function DOES add the octcode to the buffer, then it calls the recursive helper to
// actually encode the tree
//
// extraTrees is assumed to me an allocated array of VoxelNode*, if we're unable to fully encode the tree
// because we run out of room on the outputBuffer, then we will add VoxelNode*'s of the trees that need
// to be encoded to that array. If the array
int VoxelTree::encodeTreeBitstream(int maxEncodeLevel, VoxelNode* node, const ViewFrustum& viewFrustum,
                                    unsigned char* outputBuffer, int availableBytes, 
                                    VoxelNodeBag& bag) {

    // How many bytes have we written so far at this level;
    int bytesWritten = 0;

    // If we're at a node that is out of view, then we can return, because no nodes below us will be in view!
    if (!node->isInView(viewFrustum)) {
        return bytesWritten;
    }

    // write the octal code
    int codeLength = bytesRequiredForCodeLength(*node->octalCode);
    memcpy(outputBuffer,node->octalCode,codeLength);

    outputBuffer += codeLength; // move the pointer
    bytesWritten += codeLength; // keep track of byte count
    availableBytes -= codeLength; // keep track or remaining space 

    int currentEncodeLevel = 0;
    int childBytesWritten = encodeTreeBitstreamRecursion(maxEncodeLevel, currentEncodeLevel, 
                                    node, viewFrustum, 
                                    outputBuffer, availableBytes, bag);

    // if childBytesWritten == 1 then something went wrong... that's not possible
    assert(childBytesWritten != 1);

    // if childBytesWritten == 2, then it can only mean that the lower level trees don't exist or for some reason
    // couldn't be written... so reset them here...
    if (childBytesWritten == 2) {
        childBytesWritten = 0;
    }

    // if we wrote child bytes, then return our result of all bytes written
    if (childBytesWritten) {
        bytesWritten += childBytesWritten; 
    } else {
        // otherwise... if we didn't write any child bytes, then pretend like we also didn't write our octal code
        bytesWritten = 0;
    }
    return bytesWritten;
}

// This will encode a tree bitstream, given a node it will encode the full tree from that point onward. 
// It will ignore any branches that are not in view. But other than that, it will not (can not) do any
// prioritization of branches or deeper searching of the branches for optimization.
//
// NOTE: This recursive function DOES NOT add the octcode to the buffer. It's assumed that the caller has
// already done that.
int VoxelTree::encodeTreeBitstreamRecursion(int maxEncodeLevel, int& currentEncodeLevel,
                                    VoxelNode* node, const ViewFrustum& viewFrustum,
                                    unsigned char* outputBuffer, int availableBytes,
                                    VoxelNodeBag& bag) const {
    // How many bytes have we written so far at this level;
    int bytesAtThisLevel = 0;

    // Keep track of how deep we've encoded.
    currentEncodeLevel++;

    // If we've reached our max Search Level, then stop searching.
    if (currentEncodeLevel >= maxEncodeLevel) {
        return bytesAtThisLevel;
    }

    float distance = node->distanceToCamera(viewFrustum);
    float boundaryDistance = boundaryDistanceForRenderLevel(*node->octalCode + 1);

    // If we're too far away for our render level, then just return
    if (distance >= boundaryDistance) {
        return bytesAtThisLevel;
    }

    // If we're at a node that is out of view, then we can return, because no nodes below us will be in view!
    // although technically, we really shouldn't ever be here, because our callers shouldn't be calling us if
    // we're out of view
    if (!node->isInView(viewFrustum)) {
        return bytesAtThisLevel;
    }
    
    bool keepDiggingDeeper = true; // Assuming we're in view we have a great work ethic, we're always ready for more!

    // At any given point in writing the bitstream, the largest minimum we might need to flesh out the current level
    // is 1 byte for child colors + 3*8 bytes for the actual colors + 1 byte for child trees. There could be sub trees
    // below this point, which might take many more bytes, but that's ok, because we can always mark our subtrees as
    // not existing and stop the packet at this point, then start up with a new packet for the remaining sub trees.
    const int CHILD_COLOR_MASK_BYTES = 1;
    const int MAX_CHILDREN = 8;
    const int BYTES_PER_COLOR = 3;
    const int CHILD_TREE_EXISTS_BYTES = 1;
    const int MAX_LEVEL_BYTES = CHILD_COLOR_MASK_BYTES + MAX_CHILDREN * BYTES_PER_COLOR + CHILD_TREE_EXISTS_BYTES;
    
    // Make our local buffer large enough to handle writing at this level in case we need to.
    unsigned char thisLevelBuffer[MAX_LEVEL_BYTES];
    unsigned char* writeToThisLevelBuffer = &thisLevelBuffer[0];

    unsigned char childrenExistBits = 0;
    unsigned char childrenColoredBits = 0;
    int inViewCount = 0;
    int inViewNotLeafCount = 0;
    int inViewWithColorCount = 0;

    // for each child node, check to see if they exist, are colored, and in view, and if so
    // add them to our distance ordered array of children
    for (int i = 0; i < MAX_CHILDREN; i++) {
        VoxelNode* childNode = node->children[i];
        bool childExists = (childNode != NULL);
        bool childIsInView  = (childExists && childNode->isInView(viewFrustum));
        if (childIsInView) {
            // Before we determine consider this further, let's see if it's in our LOD scope...
            float distance = childNode->distanceToCamera(viewFrustum);
            float boundaryDistance = boundaryDistanceForRenderLevel(*childNode->octalCode + 1);

            if (distance < boundaryDistance) {
                inViewCount++;
            
                // track children in view as existing and not a leaf, if they're a leaf,
                // we don't care about recursing deeper on them, and we don't consider their
                // subtree to exist
                if (!(childExists && childNode->isLeaf())) {
                    childrenExistBits += (1 << (7 - i));
                    inViewNotLeafCount++;
                }
            
                // track children with actual color
                if (childExists && childNode->isColored()) {
                    childrenColoredBits += (1 << (7 - i));
                    inViewWithColorCount++;
                }
            }
        }
    }
    *writeToThisLevelBuffer = childrenColoredBits;
    writeToThisLevelBuffer += sizeof(childrenColoredBits); // move the pointer
    bytesAtThisLevel += sizeof(childrenColoredBits); // keep track of byte count
    
    // write the color data...
    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (oneAtBit(childrenColoredBits, i)) {
            memcpy(writeToThisLevelBuffer,&node->children[i]->getColor(),BYTES_PER_COLOR);
            writeToThisLevelBuffer += BYTES_PER_COLOR; // move the pointer for color
            bytesAtThisLevel += BYTES_PER_COLOR; // keep track of byte count for color
        }
    }
    // write the child exist bits
    *writeToThisLevelBuffer = childrenExistBits;

    writeToThisLevelBuffer += sizeof(childrenExistBits); // move the pointer
    bytesAtThisLevel += sizeof(childrenExistBits); // keep track of byte count
    
    // We only need to keep digging, if there is at least one child that is inView, and not a leaf.
    keepDiggingDeeper = (inViewNotLeafCount > 0);
        
    // at this point we need to do a gut check. If we're writing, then we need to check how many bytes we've
    // written at this level, and how many bytes we have available in the outputBuffer. If we can fit what we've got so far
    // and room for the no more children terminator, then let's write what we got so far.

    // If we have enough room to copy our local results into the buffer, then do so...
    if (availableBytes >= bytesAtThisLevel) {
        memcpy(outputBuffer,&thisLevelBuffer[0],bytesAtThisLevel);
        
        outputBuffer   += bytesAtThisLevel;
        availableBytes -= bytesAtThisLevel;
    } else {
        // we've run out of room!!! What do we do!!!
        // 1) return 0 for bytes written so upper levels do the right thing, namely prune us from their trees
        // 2) add our node to the list of extra nodes for later output...
        // Note: we don't do any termination for this level, we just return 0, then the upper level is in charge
        // of handling things. For example, in case of child iteration, it needs to unset the child exist bit for
        // this child.
        // add our node the the list of extra nodes to output later... 
        bag.insert(node);
        return 0;
    }
    
    if (keepDiggingDeeper) {
        // at this point, we need to iterate the children who are in view, even if not colored
        // and we need to determine if there's a deeper tree below them that we care about. 
        //
        // Since this recursive function assumes we're already writing, we know we've already written our 
        // childrenExistBits. But... we don't really know how big the child tree will be. And we don't know if
        // we'll have room in our buffer to actually write all these child trees. What we kinda would like to do is
        // write our childExistsBits as a place holder. Then let each potential tree have a go at it. If they 
        // write something, we keep them in the bits, if they don't, we take them out.
        //
        // we know the last thing we wrote to the outputBuffer was our childrenExistBits. Let's remember where that was!
        unsigned char* childExistsPlaceHolder = outputBuffer-sizeof(childrenExistBits);

        for (int i = 0; i < MAX_CHILDREN; i++) {
        
            if (oneAtBit(childrenExistBits, i)) {
                VoxelNode* childNode = node->children[i];
                
                int thisLevel = currentEncodeLevel;
                int childTreeBytesOut = encodeTreeBitstreamRecursion(maxEncodeLevel, thisLevel, 
                        childNode, viewFrustum, outputBuffer, availableBytes, bag);
                
                // if the child wrote 0 bytes, it means that nothing below exists or was in view, or we ran out of space,
                // basically, the children below don't contain any info.

                // if the child tree wrote 1 byte??? something must have gone wrong... because it must have at least the color
                // byte and the child exist byte.
                //
                assert(childTreeBytesOut != 1);
                
                // if the child tree wrote just 2 bytes, then it means: it had no colors and no child nodes, because...
                //    if it had colors it would write 1 byte for the color mask, 
                //          and at least a color's worth of bytes for the node of colors.
                //   if it had child trees (with something in them) then it would have the 1 byte for child mask
                //         and some number of bytes of lower children...
                // so, if the child returns 2 bytes out, we can actually consider that an empty tree also!!
                //
                // we can make this act like no bytes out, by just resetting the bytes out in this case
                if (2 == childTreeBytesOut) {
                    childTreeBytesOut = 0; // this is the degenerate case of a tree with no colors and no child trees
                }

                bytesAtThisLevel += childTreeBytesOut;
                availableBytes -= childTreeBytesOut;
                outputBuffer += childTreeBytesOut;

                // If we had previously started writing, and if the child DIDN'T write any bytes,
                // then we want to remove their bit from the childExistsPlaceHolder bitmask
                if (0 == childTreeBytesOut) {
                    // remove this child's bit...
                    childrenExistBits -= (1 << (7 - i));
                
                    // repair the child exists mask
                    *childExistsPlaceHolder = childrenExistBits;
                    // Note: no need to move the pointer, cause we already stored this
                } // end if (0 == childTreeBytesOut)
            } // end if (oneAtBit(childrenExistBits, i))
        } // end for
    } // end keepDiggingDeeper
    return bytesAtThisLevel;
}

