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
#include "Log.h"
#include "PacketHeaders.h"
#include "OctalCode.h"
#include "GeometryUtil.h"
#include "VoxelTree.h"
#include "VoxelNodeBag.h"
#include "ViewFrustum.h"
#include <fstream> // to load voxels from file
#include "VoxelConstants.h"

#include <glm/gtc/noise.hpp>

int boundaryDistanceForRenderLevel(unsigned int renderLevel) {
    float voxelSizeScale = 50000.0f;
    return voxelSizeScale / powf(2, renderLevel);
}

VoxelTree::VoxelTree(bool shouldReaverage) :
    voxelsCreated(0),
    voxelsColored(0),
    voxelsBytesRead(0),
    voxelsCreatedStats(100),
    voxelsColoredStats(100),
    voxelsBytesReadStats(100),
    _isDirty(true),
    _shouldReaverage(shouldReaverage) {
    rootNode = new VoxelNode();
}

VoxelTree::~VoxelTree() {
    // delete the children of the root node
    // this recursively deletes the tree
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        delete rootNode->getChildAtIndex(i);
    }
}

// Recurses voxel tree calling the RecurseVoxelTreeOperation function for each node.
// stops recursion if operation function returns false.
void VoxelTree::recurseTreeWithOperation(RecurseVoxelTreeOperation operation, void* extraData) {
    recurseNodeWithOperation(rootNode, operation,extraData);
}

// Recurses voxel node with an operation function
void VoxelTree::recurseNodeWithOperation(VoxelNode* node,RecurseVoxelTreeOperation operation, void* extraData) {
    if (operation(node, extraData)) {
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            VoxelNode* child = node->getChildAtIndex(i);
            if (child) {
                recurseNodeWithOperation(child, operation, extraData);
            }
        }
    }
}

VoxelNode * VoxelTree::nodeForOctalCode(VoxelNode* ancestorNode, unsigned char* needleCode, VoxelNode** parentOfFoundNode) const {
    // find the appropriate branch index based on this ancestorNode
    if (*needleCode > 0) {
        int branchForNeedle = branchIndexWithDescendant(ancestorNode->getOctalCode(), needleCode);
        VoxelNode* childNode = ancestorNode->getChildAtIndex(branchForNeedle);
        
        if (childNode) {
            if (*childNode->getOctalCode() == *needleCode) {
            
            	// If the caller asked for the parent, then give them that too...
            	if (parentOfFoundNode) {
					*parentOfFoundNode = ancestorNode;
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
    int indexOfNewChild = branchIndexWithDescendant(lastParentNode->getOctalCode(), codeToReach);
    // If this parent node is a leaf, then you know the child path doesn't exist, so deal with
    // breaking up the leaf first, which will also create a child path
    if (lastParentNode->isLeaf() && lastParentNode->isColored()) {
        // for colored leaves, we must add *all* the children
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            lastParentNode->addChildAtIndex(i);
            lastParentNode->getChildAtIndex(i)->setColor(lastParentNode->getColor());
        }
    } else if (!lastParentNode->getChildAtIndex(indexOfNewChild)) {
        // we could be coming down a branch that was already created, so don't stomp on it.
        lastParentNode->addChildAtIndex(indexOfNewChild);
    }

    // This works because we know we traversed down the same tree so if the length is the same, then the whole code is the same
    if (*lastParentNode->getChildAtIndex(indexOfNewChild)->getOctalCode() == *codeToReach) {
        return lastParentNode->getChildAtIndex(indexOfNewChild);
    } else {
        return createMissingNode(lastParentNode->getChildAtIndex(indexOfNewChild), codeToReach);
    }
}

int VoxelTree::readNodeData(VoxelNode* destinationNode, unsigned char* nodeData, int bytesLeftToRead, 
                            bool includeColor, bool includeExistsBits) {
    // give this destination node the child mask from the packet
    const unsigned char ALL_CHILDREN_ASSUMED_TO_EXIST = 0xFF;
    unsigned char colorInPacketMask = *nodeData;

    // instantiate variable for bytes already read
    int bytesRead = sizeof(colorInPacketMask);
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        // check the colors mask to see if we have a child to color in
        if (oneAtBit(colorInPacketMask, i)) {
            // create the child if it doesn't exist
            if (!destinationNode->getChildAtIndex(i)) {
                destinationNode->addChildAtIndex(i);
                if (destinationNode->isDirty()) {
                    _isDirty = true;
                    _nodesChangedFromBitstream++;
                }
                voxelsCreated++;
                voxelsCreatedStats.updateAverage(1);
            }

            // pull the color for this child
            nodeColor newColor = { 128, 128, 128, 1};
            if (includeColor) {
                memcpy(newColor, nodeData + bytesRead, 3);
                bytesRead += 3;
            }
            bool nodeWasDirty = destinationNode->getChildAtIndex(i)->isDirty();
            destinationNode->getChildAtIndex(i)->setColor(newColor);
            bool nodeIsDirty = destinationNode->getChildAtIndex(i)->isDirty();
            if (nodeIsDirty) {
                _isDirty = true;
            }
            if (!nodeWasDirty && nodeIsDirty) {
                _nodesChangedFromBitstream++;
            }
			this->voxelsColored++;
			this->voxelsColoredStats.updateAverage(1);
        }
    }

    // give this destination node the child mask from the packet
    unsigned char childrenInTreeMask = includeExistsBits ? *(nodeData + bytesRead) : ALL_CHILDREN_ASSUMED_TO_EXIST; 
    unsigned char childMask = *(nodeData + bytesRead + (includeExistsBits ? sizeof(childrenInTreeMask) : 0));

    int childIndex = 0;
    bytesRead += includeExistsBits ? sizeof(childrenInTreeMask) + sizeof(childMask) : sizeof(childMask);
    
    while (bytesLeftToRead - bytesRead > 0 && childIndex < NUMBER_OF_CHILDREN) {
        // check the exists mask to see if we have a child to traverse into
        
        if (oneAtBit(childMask, childIndex)) {            
            if (!destinationNode->getChildAtIndex(childIndex)) {
                // add a child at that index, if it doesn't exist
                bool nodeWasDirty = destinationNode->isDirty();
                destinationNode->addChildAtIndex(childIndex);
                bool nodeIsDirty = destinationNode->isDirty();
                if (nodeIsDirty) {
                    _isDirty = true;
                }
                if (!nodeWasDirty && nodeIsDirty) {
                    _nodesChangedFromBitstream++;
                }
                this->voxelsCreated++;
                this->voxelsCreatedStats.updateAverage(this->voxelsCreated);
            }
            
            // tell the child to read the subsequent data
            bytesRead += readNodeData(destinationNode->getChildAtIndex(childIndex),
                                      nodeData + bytesRead, bytesLeftToRead - bytesRead, includeColor, includeExistsBits);
        }
        childIndex++;
    }
    

    if (includeExistsBits) {
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            // now also check the childrenInTreeMask, if the mask is missing the bit, then it means we need to delete this child
            // subtree/node, because it shouldn't actually exist in the tree.
            if (!oneAtBit(childrenInTreeMask, i) && destinationNode->getChildAtIndex(i)) {
                bool stagedForDeletion = false; // assume staging is not needed
                destinationNode->safeDeepDeleteChildAtIndex(i, stagedForDeletion);
                _isDirty = true; // by definition!
            }
        }        
    }
    return bytesRead;
}

void VoxelTree::readBitstreamToTree(unsigned char * bitstream, unsigned long int bufferSizeBytes, 
                                    bool includeColor, bool includeExistsBits) {
    int bytesRead = 0;
    unsigned char* bitstreamAt = bitstream;
    
    _nodesChangedFromBitstream = 0;

    // Keep looping through the buffer calling readNodeData() this allows us to pack multiple root-relative Octal codes
    // into a single network packet. readNodeData() basically goes down a tree from the root, and fills things in from there
    // if there are more bytes after that, it's assumed to be another root relative tree

    while (bitstreamAt < bitstream + bufferSizeBytes) {
        VoxelNode* bitstreamRootNode = nodeForOctalCode(rootNode, (unsigned char *)bitstreamAt, NULL);
        if (*bitstreamAt != *bitstreamRootNode->getOctalCode()) {
            // if the octal code returned is not on the same level as
            // the code being searched for, we have VoxelNodes to create

            // Note: we need to create this node relative to root, because we're assuming that the bitstream for the initial
            // octal code is always relative to root!
            bitstreamRootNode = createMissingNode(rootNode, (unsigned char*) bitstreamAt);
            if (bitstreamRootNode->isDirty()) {
                _isDirty = true;
                _nodesChangedFromBitstream++;
            }
        }

        int octalCodeBytes = bytesRequiredForCodeLength(*bitstreamAt);
        int theseBytesRead = 0;
        theseBytesRead += octalCodeBytes;
        theseBytesRead += readNodeData(bitstreamRootNode, bitstreamAt + octalCodeBytes, 
                                bufferSizeBytes - (bytesRead + octalCodeBytes), includeColor, includeExistsBits);

        // skip bitstream to new startPoint
        bitstreamAt += theseBytesRead;
        bytesRead +=  theseBytesRead;
    }

    this->voxelsBytesRead += bufferSizeBytes;
    this->voxelsBytesReadStats.updateAverage(bufferSizeBytes);
}

void VoxelTree::deleteVoxelAt(float x, float y, float z, float s, bool stage) {
    unsigned char* octalCode = pointToVoxel(x,y,z,s,0,0,0);
    deleteVoxelCodeFromTree(octalCode, stage);
    delete octalCode; // cleanup memory
    reaverageVoxelColors(rootNode); 
}


// Note: uses the codeColorBuffer format, but the color's are ignored, because
// this only finds and deletes the node from the tree.
void VoxelTree::deleteVoxelCodeFromTree(unsigned char* codeBuffer, bool stage, bool collapseEmptyTrees) {
    VoxelNode* parentNode = NULL;
    VoxelNode* nodeToDelete = nodeForOctalCode(rootNode, codeBuffer, &parentNode);
    // If the node exists...
    int lengthInBytes = bytesRequiredForCodeLength(*codeBuffer); // includes octet count, not color!

    // if the code we got back matches our target, then we know we can actually delete it
    if (memcmp(nodeToDelete->getOctalCode(), codeBuffer, lengthInBytes) == 0) {
        if (parentNode) {
            int childIndex = branchIndexWithDescendant(parentNode->getOctalCode(), codeBuffer);
            if (stage) {
                nodeToDelete->stageForDeletion();
            } else {
                parentNode->deleteChildAtIndex(childIndex);
                if (_shouldReaverage) {
                    parentNode->setColorFromAverageOfChildren();
                }
            }

            // If we're not a colored leaf, and we have no children, then delete ourselves
            // This will collapse the empty tree above us.
            if (collapseEmptyTrees && parentNode->getChildCount() == 0 && !parentNode->isColored()) {
                // Can't delete the root this way.
                if (parentNode != rootNode) {
                    deleteVoxelCodeFromTree(parentNode->getOctalCode(), stage, collapseEmptyTrees);
                }
            }
            _isDirty = true;
        }
    } else if (nodeToDelete->isLeaf()) {
        // we need to break up ancestors until we get to the right level
        VoxelNode* ancestorNode = nodeToDelete;
        while (true) {
            int index = branchIndexWithDescendant(ancestorNode->getOctalCode(), codeBuffer);
            for (int i = 0; i < 8; i++) {
                if (i != index) {
                    ancestorNode->addChildAtIndex(i);
                    if (nodeToDelete->isColored()) {
                        ancestorNode->getChildAtIndex(i)->setColor(nodeToDelete->getColor());
                    }
                }
            }
            if (*ancestorNode->getOctalCode() == *codeBuffer - 1) {
                break;
            }
            ancestorNode->addChildAtIndex(index);
            ancestorNode = ancestorNode->getChildAtIndex(index);
            if (nodeToDelete->isColored()) {
                ancestorNode->setColor(nodeToDelete->getColor());
            }
        }
        _isDirty = true;
    }
}

void VoxelTree::eraseAllVoxels() {
    // XXXBHG Hack attack - is there a better way to erase the voxel tree?
    delete rootNode; // this will recurse and delete all children
    rootNode = new VoxelNode();
    _isDirty = true;
}

void VoxelTree::readCodeColorBufferToTree(unsigned char *codeColorBuffer, bool destructive) {
    VoxelNode* lastCreatedNode = nodeForOctalCode(rootNode, codeColorBuffer, NULL);
    // create the node if it does not exist
    if (*lastCreatedNode->getOctalCode() != *codeColorBuffer) {
        lastCreatedNode = createMissingNode(lastCreatedNode, codeColorBuffer);
        _isDirty = true;
    } else {
        // if it does exist, make sure it has no children
        for (int i = 0; i < 8; i++) {
            if (lastCreatedNode->getChildAtIndex(i)) {
                if (destructive) {
                    lastCreatedNode->deleteChildAtIndex(i);
                } else {
                    printLog("WARNING! operation would require deleting child at index %d, add Voxel ignored!\n ", i);
                }
            }
        }
    }

    if (lastCreatedNode->isLeaf()) {
        // give this node its color
        int octalCodeBytes = bytesRequiredForCodeLength(*codeColorBuffer);

        nodeColor newColor;
        memcpy(newColor, codeColorBuffer + octalCodeBytes, 3);
        newColor[3] = 1;
        lastCreatedNode->setColor(newColor);
        if (lastCreatedNode->isDirty()) {
            _isDirty = true;
        }
    }
}

void VoxelTree::processRemoveVoxelBitstream(unsigned char * bitstream, int bufferSizeBytes) {
	//unsigned short int itemNumber = (*((unsigned short int*)&bitstream[sizeof(PACKET_HEADER)]));
	int atByte = sizeof(short int) + sizeof(PACKET_HEADER);
	unsigned char* pVoxelData = (unsigned char*)&bitstream[atByte];
	while (atByte < bufferSizeBytes) {
		unsigned char octets = (unsigned char)*pVoxelData;
		int voxelDataSize = bytesRequiredForCodeLength(octets)+3; // 3 for color!

		deleteVoxelCodeFromTree(pVoxelData, ACTUALLY_DELETE, COLLAPSE_EMPTY_TREE);

		pVoxelData+=voxelDataSize;
		atByte+=voxelDataSize;
	}
    reaverageVoxelColors(rootNode); // Fix our colors!! Need to call it on rootNode
}

void VoxelTree::printTreeForDebugging(VoxelNode *startNode) {
    int colorMask = 0;

    // create the color mask
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        if (startNode->getChildAtIndex(i) && startNode->getChildAtIndex(i)->isColored()) {
            colorMask += (1 << (7 - i));
        }
    }

    printLog("color mask: ");
    outputBits(colorMask);

    // output the colors we have
    for (int j = 0; j < NUMBER_OF_CHILDREN; j++) {
        if (startNode->getChildAtIndex(j) && startNode->getChildAtIndex(j)->isColored()) {
            printLog("color %d : ",j);
            for (int c = 0; c < 3; c++) {
                outputBits(startNode->getChildAtIndex(j)->getTrueColor()[c],false);
            }
            startNode->getChildAtIndex(j)->printDebugDetails("");
        }
    }

    unsigned char childMask = 0;

    for (int k = 0; k < NUMBER_OF_CHILDREN; k++) {
        if (startNode->getChildAtIndex(k)) {
            childMask += (1 << (7 - k));
        }
    }

    printLog("child mask: ");
    outputBits(childMask);

    if (childMask > 0) {
        // ask children to recursively output their trees
        // if they aren't a leaf
        for (int l = 0; l < NUMBER_OF_CHILDREN; l++) {
            if (startNode->getChildAtIndex(l)) {
                printTreeForDebugging(startNode->getChildAtIndex(l));
            }
        }
    }   
}

void VoxelTree::reaverageVoxelColors(VoxelNode *startNode) {
    // if our tree is a reaveraging tree, then we do this, otherwise we don't do anything
    if (_shouldReaverage) {
        bool hasChildren = false;

        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            if (startNode->getChildAtIndex(i)) {
                reaverageVoxelColors(startNode->getChildAtIndex(i));
                hasChildren = true;
            }
        }

        // collapseIdenticalLeaves() returns true if it collapses the leaves
        // in which case we don't need to set the average color
        if (hasChildren && !startNode->collapseIdenticalLeaves()) {
            startNode->setColorFromAverageOfChildren();
        }
    }
}

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
            totalBytesRead++;
            lengthInBytes = bytesRequiredForCodeLength(octets) - 1; 
            unsigned char * voxelData = new unsigned char[lengthInBytes + 1 + 3];
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

VoxelNode* VoxelTree::getVoxelAt(float x, float y, float z, float s) const {
    unsigned char* octalCode = pointToVoxel(x,y,z,s,0,0,0);
    VoxelNode* node = nodeForOctalCode(rootNode, octalCode, NULL);
    if (*node->getOctalCode() != *octalCode) {
        node = NULL;
    }
    delete octalCode; // cleanup memory
    return node;
}

void VoxelTree::createVoxel(float x, float y, float z, float s, 
                            unsigned char red, unsigned char green, unsigned char blue, bool destructive) {
    unsigned char* voxelData = pointToVoxel(x,y,z,s,red,green,blue);
    this->readCodeColorBufferToTree(voxelData, destructive);
    delete voxelData;
}


void VoxelTree::createLine(glm::vec3 point1, glm::vec3 point2, float unitSize, rgbColor color, bool destructive) {
    glm::vec3 distance = point2 - point1;
    glm::vec3 items = distance / unitSize;
    int maxItems = std::max(items.x, std::max(items.y, items.z));
    glm::vec3 increment = distance * (1.0f/ maxItems);
    glm::vec3 pointAt = point1;
    for (int i = 0; i <= maxItems; i++ ) {
        pointAt += increment;
        createVoxel(pointAt.x, pointAt.y, pointAt.z, unitSize, color[0], color[1], color[2], destructive);
    }
}

void VoxelTree::createSphere(float radius, float xc, float yc, float zc, float voxelSize, 
        bool solid, creationMode mode, bool destructive, bool debug) {
        
    bool wantColorRandomizer = (mode == RANDOM);
    bool wantNaturalSurface  = (mode == NATURAL);
    bool wantNaturalColor    = (mode == NATURAL);
    
    // About the color of the sphere... we're going to make this sphere be a mixture of two colors
    // in NATURAL mode, those colors will be green dominant and blue dominant. In GRADIENT mode we
    // will randomly pick which color family red, green, or blue to be dominant. In RANDOM mode we
    // ignore these dominant colors and make every voxel a completely random color.
    unsigned char r1, g1, b1, r2, g2, b2;

    if (wantNaturalColor) {
        r1 = r2 = b2 = g1 = 0;
        b1 = g2 = 255;
    } else if (!wantColorRandomizer) {
        unsigned char dominantColor1 = randIntInRange(1, 3); //1=r, 2=g, 3=b dominant
        unsigned char dominantColor2 = randIntInRange(1, 3);

        if (dominantColor1 == dominantColor2) {
            dominantColor2 = dominantColor1 + 1 % 3;
        }

        r1 = (dominantColor1 == 1) ? randIntInRange(200, 255) : randIntInRange(40, 100);
        g1 = (dominantColor1 == 2) ? randIntInRange(200, 255) : randIntInRange(40, 100);
        b1 = (dominantColor1 == 3) ? randIntInRange(200, 255) : randIntInRange(40, 100);
        r2 = (dominantColor2 == 1) ? randIntInRange(200, 255) : randIntInRange(40, 100);
        g2 = (dominantColor2 == 2) ? randIntInRange(200, 255) : randIntInRange(40, 100);
        b2 = (dominantColor2 == 3) ? randIntInRange(200, 255) : randIntInRange(40, 100);
    }
    
    // We initialize our rgb to be either "grey" in case of randomized surface, or
    // the average of the gradient, in the case of the gradient sphere.
    unsigned char red   = wantColorRandomizer ? 128 : (r1 + r2) / 2; // average of the colors
    unsigned char green = wantColorRandomizer ? 128 : (g1 + g2) / 2;
    unsigned char blue  = wantColorRandomizer ? 128 : (b1 + b2) / 2;

    // I want to do something smart like make these inside circles with bigger voxels, but this doesn't seem to work.
    float thisVoxelSize = voxelSize; // radius / 2.0f; 
    float thisRadius = 0.0;
    if (!solid) {
        thisRadius = radius; // just the outer surface
        thisVoxelSize = voxelSize;
    }

    // If you also iterate form the interior of the sphere to the radius, making
    // larger and larger spheres you'd end up with a solid sphere. And lots of voxels!
    bool lastLayer = false;
    while (!lastLayer) {
        lastLayer = (thisRadius + (voxelSize * 2.0) >= radius);

        // We want to make sure that as we "sweep" through our angles we use a delta angle that voxelSize 
        // small enough to not skip any voxels we can calculate theta from our desired arc length
        //      lenArc = ndeg/360deg * 2pi*R  --->  lenArc = theta/2pi * 2pi*R
        //      lenArc = theta*R ---> theta = lenArc/R ---> theta = g/r	
        float angleDelta = (thisVoxelSize / thisRadius);

        if (debug) {
            int percentComplete = 100 * (thisRadius/radius);
            printLog("percentComplete=%d\n",percentComplete); 
        }
    
        for (float theta=0.0; theta <= 2 * M_PI; theta += angleDelta) {
            for (float phi=0.0; phi <= M_PI; phi += angleDelta) {
                bool naturalSurfaceRendered = false;
                float x = xc + thisRadius * cos(theta) * sin(phi);
                float y = yc + thisRadius * sin(theta) * sin(phi);
                float z = zc + thisRadius * cos(phi);
            
                // if we're on the outer radius, then we do a couple of things differently. 
                // 1) If we're in NATURAL mode we will actually draw voxels from our surface outward (from the surface) up 
                //    some random height. This will give our sphere some contours.
                // 2) In all modes, we will use our "outer" color to draw the voxels. Otherwise we will use the average color
                if (lastLayer) {
                    if (false && debug) { 
                        printLog("adding candy shell: theta=%f phi=%f thisRadius=%f radius=%f\n",
                                theta, phi, thisRadius,radius); 
                    }
                    switch (mode) {
                        case RANDOM: {
                            red   = randomColorValue(165);
                            green = randomColorValue(165);
                            blue  = randomColorValue(165);
                        } break;
                        case GRADIENT: {
                            float gradient = (phi / M_PI);
                            red   = r1 + ((r2 - r1) * gradient);
                            green = g1 + ((g2 - g1) * gradient);
                            blue  = b1 + ((b2 - b1) * gradient);
                        } break;
                        case NATURAL: {
                            glm::vec3 position = glm::vec3(theta,phi,radius);
                            float perlin = glm::perlin(position) + .25f * glm::perlin(position * 4.f) 
                                            + .125f * glm::perlin(position * 16.f);
                            float gradient = (1.0f + perlin)/ 2.0f;
                            red   = (unsigned char)std::min(255, std::max(0, (int)(r1 + ((r2 - r1) * gradient))));
                            green = (unsigned char)std::min(255, std::max(0, (int)(g1 + ((g2 - g1) * gradient))));
                            blue  = (unsigned char)std::min(255, std::max(0, (int)(b1 + ((b2 - b1) * gradient))));
                            if (debug) { 
                                printLog("perlin=%f gradient=%f color=(%d,%d,%d)\n",perlin, gradient, red, green, blue); 
                            }
                        } break;
                    }
                    if (wantNaturalSurface) {
                        // for natural surfaces, we will render up to 16 voxel's above the surface of the sphere
                        glm::vec3 position = glm::vec3(theta,phi,radius);
                        float perlin = glm::perlin(position) + .25f * glm::perlin(position * 4.f) 
                                        + .125f * glm::perlin(position * 16.f);
                        float gradient = (1.0f + perlin)/ 2.0f;

                        int height = (4 * gradient)+1; // make it at least 4 thick, so we get some averaging
                        float subVoxelScale = thisVoxelSize;
                        for (int i = 0; i < height; i++) {
                            x = xc + (thisRadius + i * subVoxelScale) * cos(theta) * sin(phi);
                            y = yc + (thisRadius + i * subVoxelScale) * sin(theta) * sin(phi);
                            z = zc + (thisRadius + i * subVoxelScale) * cos(phi);
                            this->createVoxel(x, y, z, subVoxelScale, red, green, blue, destructive);
                        }
                        naturalSurfaceRendered = true;
                    }
                }
                if (!naturalSurfaceRendered) {
                    this->createVoxel(x, y, z, thisVoxelSize, red, green, blue, destructive);
                }
            }
        }
        thisRadius += thisVoxelSize;
        thisVoxelSize = std::max(voxelSize, thisVoxelSize / 2.0f);
    }
    this->reaverageVoxelColors(this->rootNode);
}

int VoxelTree::searchForColoredNodes(int maxSearchLevel, VoxelNode* node, const ViewFrustum& viewFrustum, VoxelNodeBag& bag,
                                     bool deltaViewFrustum, const ViewFrustum* lastViewFrustum) {

    // call the recursive version, this will add all found colored node roots to the bag
    int currentSearchLevel = 0;
    
    int levelReached = searchForColoredNodesRecursion(maxSearchLevel, currentSearchLevel, rootNode, 
                                                      viewFrustum, bag, deltaViewFrustum, lastViewFrustum);
    return levelReached;
}

// combines the ray cast arguments into a single object
class RayArgs {
public:
    glm::vec3 origin;
    glm::vec3 direction;
    VoxelNode*& node;
    float& distance;
    BoxFace& face;
    bool found;
};

bool findRayIntersectionOp(VoxelNode* node, void* extraData) {
    RayArgs* args = static_cast<RayArgs*>(extraData);
    AABox box = node->getAABox();
    float distance;
    BoxFace face;
    if (!box.findRayIntersection(args->origin, args->direction, distance, face)) {
        return false;
    }
    if (!node->isLeaf()) {
        return true; // recurse on children
    }
    distance *= TREE_SCALE;
    if (node->isColored() && (!args->found || distance < args->distance)) {
        args->node = node;
        args->distance = distance;
        args->face = face;
        args->found = true;
    }
    return false;
}

bool VoxelTree::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                    VoxelNode*& node, float& distance, BoxFace& face) {
    RayArgs args = { origin / (float)TREE_SCALE, direction, node, distance, face };
    recurseTreeWithOperation(findRayIntersectionOp, &args);
    return args.found;
}

class SphereArgs {
public:
    glm::vec3 center;
    float radius;
    glm::vec3& penetration;
    bool found;
};

bool findSpherePenetrationOp(VoxelNode* node, void* extraData) {
    SphereArgs* args = static_cast<SphereArgs*>(extraData);
    
    // coarse check against bounds
    const AABox& box = node->getAABox();
    if (!box.expandedContains(args->center, args->radius)) {
        return false;
    }
    if (!node->isLeaf()) {
        return true; // recurse on children
    }
    if (node->isColored()) {
        glm::vec3 nodePenetration;
        if (box.findSpherePenetration(args->center, args->radius, nodePenetration)) {
            args->penetration = addPenetrations(args->penetration, nodePenetration * (float)TREE_SCALE);
            args->found = true;
        }        
    }
    return false;
}

bool VoxelTree::findSpherePenetration(const glm::vec3& center, float radius, glm::vec3& penetration) {
    SphereArgs args = { center / (float)TREE_SCALE, radius / TREE_SCALE, penetration };
    penetration = glm::vec3(0.0f, 0.0f, 0.0f);
    recurseTreeWithOperation(findSpherePenetrationOp, &args);
    return args.found;
}

class CapsuleArgs {
public:
    glm::vec3 start;
    glm::vec3 end;
    float radius;
    glm::vec3& penetration;
    bool found;
};

bool findCapsulePenetrationOp(VoxelNode* node, void* extraData) {
    CapsuleArgs* args = static_cast<CapsuleArgs*>(extraData);
    
    // coarse check against bounds
    const AABox& box = node->getAABox();
    if (!box.expandedIntersectsSegment(args->start, args->end, args->radius)) {
        return false;
    }
    if (!node->isLeaf()) {
        return true; // recurse on children
    }
    if (node->isColored()) {
        glm::vec3 nodePenetration;
        if (box.findCapsulePenetration(args->start, args->end, args->radius, nodePenetration)) {
            args->penetration = addPenetrations(args->penetration, nodePenetration * (float)TREE_SCALE);
            args->found = true;
        }        
    }
    return false;
}

bool VoxelTree::findCapsulePenetration(const glm::vec3& start, const glm::vec3& end, float radius, glm::vec3& penetration) {
    CapsuleArgs args = { start / (float)TREE_SCALE, end / (float)TREE_SCALE, radius / TREE_SCALE, penetration };
    penetration = glm::vec3(0.0f, 0.0f, 0.0f);
    recurseTreeWithOperation(findCapsulePenetrationOp, &args);
    return args.found;
}

int VoxelTree::searchForColoredNodesRecursion(int maxSearchLevel, int& currentSearchLevel, 
                                              VoxelNode* node, const ViewFrustum& viewFrustum, VoxelNodeBag& bag,
                                              bool deltaViewFrustum, const ViewFrustum* lastViewFrustum) {

    // Keep track of how deep we've searched.    
    currentSearchLevel++;

    // If we've passed our max Search Level, then stop searching. return last level searched
    if (currentSearchLevel > maxSearchLevel) {
        return currentSearchLevel-1;
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
    
    VoxelNode* inViewChildren[NUMBER_OF_CHILDREN];
    float      distancesToChildren[NUMBER_OF_CHILDREN];
    int        positionOfChildren[NUMBER_OF_CHILDREN];
    int        inViewCount = 0;
    int        inViewNotLeafCount = 0;
    int        inViewWithColorCount = 0;
    
    // for each child node, check to see if they exist, are colored, and in view, and if so
    // add them to our distance ordered array of children
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        VoxelNode* childNode = node->getChildAtIndex(i);
        bool childIsColored = (childNode && childNode->isColored());
        bool childIsInView  = (childNode && childNode->isInView(viewFrustum));
        bool childIsLeaf    = (childNode && childNode->isLeaf());

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
            
            if (distance < boundaryDistanceForRenderLevel(*childNode->getOctalCode() + 1)) {
                inViewCount = insertIntoSortedArrays((void*)childNode, distance, i, 
                                                     (void**)&inViewChildren, (float*)&distancesToChildren, 
                                                     (int*)&positionOfChildren, inViewCount, NUMBER_OF_CHILDREN);
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
            int childLevelReached = searchForColoredNodesRecursion(maxSearchLevel, thisLevel, childNode, viewFrustum, bag,
                                                                   deltaViewFrustum, lastViewFrustum);
            maxChildLevel = std::max(maxChildLevel, childLevelReached);
        }
    }
    return maxChildLevel;
}

int VoxelTree::encodeTreeBitstream(int maxEncodeLevel, VoxelNode* node, unsigned char* outputBuffer, int availableBytes, 
                                   VoxelNodeBag& bag, const ViewFrustum* viewFrustum, bool includeColor, bool includeExistsBits,
                                   bool deltaViewFrustum, const ViewFrustum* lastViewFrustum) const {

    // How many bytes have we written so far at this level;
    int bytesWritten = 0;

    // If we're at a node that is out of view, then we can return, because no nodes below us will be in view!
    if (viewFrustum && !node->isInView(*viewFrustum)) {
        return bytesWritten;
    }

    // write the octal code
    int codeLength = bytesRequiredForCodeLength(*node->getOctalCode());
    memcpy(outputBuffer,node->getOctalCode(),codeLength);

    outputBuffer += codeLength; // move the pointer
    bytesWritten += codeLength; // keep track of byte count
    availableBytes -= codeLength; // keep track or remaining space 

    int currentEncodeLevel = 0;
    int childBytesWritten = encodeTreeBitstreamRecursion(maxEncodeLevel, currentEncodeLevel, node, outputBuffer, availableBytes, 
                                                         bag, viewFrustum, includeColor, includeExistsBits, 
                                                         deltaViewFrustum, lastViewFrustum);

    // if childBytesWritten == 1 then something went wrong... that's not possible
    assert(childBytesWritten != 1);

    // if includeColor and childBytesWritten == 2, then it can only mean that the lower level trees don't exist or for some reason
    // couldn't be written... so reset them here... This isn't true for the non-color included case
    if (includeColor && childBytesWritten == 2) {
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

int VoxelTree::encodeTreeBitstreamRecursion(int maxEncodeLevel, int& currentEncodeLevel, VoxelNode* node, 
                                            unsigned char* outputBuffer, int availableBytes, VoxelNodeBag& bag, 
                                            const ViewFrustum* viewFrustum, bool includeColor, bool includeExistsBits, 
                                            bool deltaViewFrustum, const ViewFrustum* lastViewFrustum) const {

    // How many bytes have we written so far at this level;
    int bytesAtThisLevel = 0;

    // Keep track of how deep we've encoded.
    currentEncodeLevel++;

    // If we've reached our max Search Level, then stop searching.
    if (currentEncodeLevel >= maxEncodeLevel) {
        return bytesAtThisLevel;
    }

    // caller can pass NULL as viewFrustum if they want everything
    if (viewFrustum) {
        float distance = node->distanceToCamera(*viewFrustum);
        float boundaryDistance = boundaryDistanceForRenderLevel(*node->getOctalCode() + 1);

        // If we're too far away for our render level, then just return
        if (distance >= boundaryDistance) {
            return bytesAtThisLevel;
        }

        // If we're at a node that is out of view, then we can return, because no nodes below us will be in view!
        // although technically, we really shouldn't ever be here, because our callers shouldn't be calling us if
        // we're out of view
        if (!node->isInView(*viewFrustum)) {
            return bytesAtThisLevel;
        }
    }
        
    bool keepDiggingDeeper = true; // Assuming we're in view we have a great work ethic, we're always ready for more!

    // At any given point in writing the bitstream, the largest minimum we might need to flesh out the current level
    // is 1 byte for child colors + 3*NUMBER_OF_CHILDREN bytes for the actual colors + 1 byte for child trees. There could be sub trees
    // below this point, which might take many more bytes, but that's ok, because we can always mark our subtrees as
    // not existing and stop the packet at this point, then start up with a new packet for the remaining sub trees.
    const int CHILD_COLOR_MASK_BYTES = 1;
    const int BYTES_PER_COLOR = 3;
    const int CHILD_TREE_EXISTS_BYTES = 1;
    const int MAX_LEVEL_BYTES = CHILD_COLOR_MASK_BYTES + NUMBER_OF_CHILDREN * BYTES_PER_COLOR + CHILD_TREE_EXISTS_BYTES;
    
    // Make our local buffer large enough to handle writing at this level in case we need to.
    unsigned char thisLevelBuffer[MAX_LEVEL_BYTES];
    unsigned char* writeToThisLevelBuffer = &thisLevelBuffer[0];

    unsigned char childrenExistInTreeBits = 0;
    unsigned char childrenExistInPacketBits = 0;
    unsigned char childrenColoredBits = 0;
    int inViewCount = 0;
    int inViewNotLeafCount = 0;
    int inViewWithColorCount = 0;

    // for each child node, check to see if they exist, are colored, and in view, and if so
    // add them to our distance ordered array of children
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        VoxelNode* childNode = node->getChildAtIndex(i);
        
        // if the caller wants to include childExistsBits, then include them even if not in view
        if (includeExistsBits && childNode) {
            childrenExistInTreeBits += (1 << (7 - i));
        }
        
        bool childIsInView  = (childNode && (!viewFrustum || childNode->isInView(*viewFrustum)));
        
        if (childIsInView) {
            // Before we determine consider this further, let's see if it's in our LOD scope...
            float distance = viewFrustum ? childNode->distanceToCamera(*viewFrustum) : 0;
            float boundaryDistance = viewFrustum ? boundaryDistanceForRenderLevel(*childNode->getOctalCode() + 1) : 1;

            if (distance < boundaryDistance) {
                inViewCount++;
            
                // track children in view as existing and not a leaf, if they're a leaf,
                // we don't care about recursing deeper on them, and we don't consider their
                // subtree to exist
                if (!(childNode && childNode->isLeaf())) {
                    childrenExistInPacketBits += (1 << (7 - i));
                    inViewNotLeafCount++;
                }

                bool childWasInView = (childNode && deltaViewFrustum && 
                                      (lastViewFrustum && ViewFrustum::INSIDE == childNode->inFrustum(*lastViewFrustum)));
            
                // track children with actual color, only if the child wasn't previously in view!
                if (childNode && childNode->isColored() && !childWasInView) {
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
    if (includeColor) {
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            if (oneAtBit(childrenColoredBits, i)) {
                memcpy(writeToThisLevelBuffer, &node->getChildAtIndex(i)->getColor(), BYTES_PER_COLOR);
                writeToThisLevelBuffer += BYTES_PER_COLOR; // move the pointer for color
                bytesAtThisLevel += BYTES_PER_COLOR; // keep track of byte count for color
            }
        }
    }

    // if the caller wants to include childExistsBits, then include them even if not in view, put them before the
    // childrenExistInPacketBits, so that the lower code can properly repair the packet exists bits
    if (includeExistsBits) {
        *writeToThisLevelBuffer = childrenExistInTreeBits;
        writeToThisLevelBuffer += sizeof(childrenExistInTreeBits); // move the pointer
        bytesAtThisLevel += sizeof(childrenExistInTreeBits); // keep track of byte count
    }

    // write the child exist bits
    *writeToThisLevelBuffer = childrenExistInPacketBits;
    writeToThisLevelBuffer += sizeof(childrenExistInPacketBits); // move the pointer
    bytesAtThisLevel += sizeof(childrenExistInPacketBits); // keep track of byte count

    // We only need to keep digging, if there is at least one child that is inView, and not a leaf.
    keepDiggingDeeper = (inViewNotLeafCount > 0);
        
    // If we have enough room to copy our local results into the buffer, then do so...
    if (availableBytes >= bytesAtThisLevel) {
        memcpy(outputBuffer, &thisLevelBuffer[0], bytesAtThisLevel);
        
        outputBuffer   += bytesAtThisLevel;
        availableBytes -= bytesAtThisLevel;
    } else {
        bag.insert(node);
        return 0;
    }
    
    if (keepDiggingDeeper) {
        // at this point, we need to iterate the children who are in view, even if not colored
        // and we need to determine if there's a deeper tree below them that we care about. 
        //
        // Since this recursive function assumes we're already writing, we know we've already written our 
        // childrenExistInPacketBits. But... we don't really know how big the child tree will be. And we don't know if
        // we'll have room in our buffer to actually write all these child trees. What we kinda would like to do is
        // write our childExistsBits as a place holder. Then let each potential tree have a go at it. If they 
        // write something, we keep them in the bits, if they don't, we take them out.
        //
        // we know the last thing we wrote to the outputBuffer was our childrenExistInPacketBits. Let's remember where that was!
        unsigned char* childExistsPlaceHolder = outputBuffer-sizeof(childrenExistInPacketBits);

        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        
            if (oneAtBit(childrenExistInPacketBits, i)) {
                VoxelNode* childNode = node->getChildAtIndex(i);
                
                int thisLevel = currentEncodeLevel;
                int childTreeBytesOut = encodeTreeBitstreamRecursion(maxEncodeLevel, thisLevel, childNode, 
                                                                     outputBuffer, availableBytes, bag, 
                                                                     viewFrustum, includeColor, includeExistsBits,
                                                                     deltaViewFrustum, lastViewFrustum);
                
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
                if (includeColor && childTreeBytesOut == 2) {
                    childTreeBytesOut = 0; // this is the degenerate case of a tree with no colors and no child trees
                }

                bytesAtThisLevel += childTreeBytesOut;
                availableBytes -= childTreeBytesOut;
                outputBuffer += childTreeBytesOut;

                // If we had previously started writing, and if the child DIDN'T write any bytes,
                // then we want to remove their bit from the childExistsPlaceHolder bitmask
                if (childTreeBytesOut == 0) {
                    // remove this child's bit...
                    childrenExistInPacketBits -= (1 << (7 - i));
                    // repair the child exists mask
                    *childExistsPlaceHolder = childrenExistInPacketBits;
                    // Note: no need to move the pointer, cause we already stored this
                } // end if (childTreeBytesOut == 0)
            } // end if (oneAtBit(childrenExistInPacketBits, i))
        } // end for
    } // end keepDiggingDeeper

    return bytesAtThisLevel;
}

bool VoxelTree::readFromFileV2(const char* fileName, VoxelNode* node) {
    std::ifstream file(fileName, std::ios::in|std::ios::binary|std::ios::ate);
    if(file.is_open()) {
		printLog("loading file %s...\n", fileName);
		
		// get file length....
        unsigned long fileLength = file.tellg();
        file.seekg( 0, std::ios::beg );
        
        // read the entire file into a buffer, WHAT!? Why not.
        unsigned char* entireFile = new unsigned char[fileLength];
        file.read((char*)entireFile, fileLength);
        readBitstreamToTree(entireFile, fileLength, WANT_COLOR, NO_EXISTS_BITS);
        delete[] entireFile;
    		
        file.close();
        return true;
    }
    return false;
}

void VoxelTree::writeToFileV2(const char* fileName, VoxelNode* node) const {

    std::ofstream file(fileName, std::ios::out|std::ios::binary);

    if(file.is_open()) {
		printLog("saving to file %s...\n", fileName);

        VoxelNodeBag nodeBag;
        // If we were given a specific node, start from there, otherwise start from root
        if (node) {
            nodeBag.insert(node);
        } else {
            nodeBag.insert(rootNode);
        }

        static unsigned char outputBuffer[MAX_VOXEL_PACKET_SIZE - 1]; // save on allocs by making this static
        int bytesWritten = 0;
        
        while (!nodeBag.isEmpty()) {
            VoxelNode* subTree = nodeBag.extract();
            bytesWritten = encodeTreeBitstream(INT_MAX, subTree, &outputBuffer[0], 
                    MAX_VOXEL_PACKET_SIZE - 1, nodeBag, IGNORE_VIEW_FRUSTUM, WANT_COLOR, NO_EXISTS_BITS);
                                                      
            file.write((const char*)&outputBuffer[0], bytesWritten);
        }
    }
    file.close();
}

unsigned long VoxelTree::getVoxelCount() {
    unsigned long nodeCount = 0;
    recurseTreeWithOperation(countVoxelsOperation, &nodeCount);
    return nodeCount;
}

bool VoxelTree::countVoxelsOperation(VoxelNode* node, void* extraData) {
    (*(unsigned long*)extraData)++;
    return true; // keep going
}
