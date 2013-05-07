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
    float voxelSizeScale = 500.0*TREE_SCALE;
    return voxelSizeScale / powf(2, renderLevel);
}

VoxelTree::VoxelTree() :
    voxelsCreated(0),
    voxelsColored(0),
    voxelsBytesRead(0),
    voxelsCreatedStats(100),
    voxelsColoredStats(100),
    voxelsBytesReadStats(100),
    _isDirty(true) {
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

VoxelNode * VoxelTree::nodeForOctalCode(VoxelNode *ancestorNode, unsigned char * needleCode, VoxelNode** parentOfFoundNode) const {
    // find the appropriate branch index based on this ancestorNode
    if (*needleCode > 0) {
        int branchForNeedle = branchIndexWithDescendant(ancestorNode->getOctalCode(), needleCode);
        VoxelNode *childNode = ancestorNode->getChildAtIndex(branchForNeedle);
        
        if (childNode) {
            if (*childNode->getOctalCode() == *needleCode) {
            
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

    int indexOfNewChild = branchIndexWithDescendant(lastParentNode->getOctalCode(), codeToReach);
    
    // we could be coming down a branch that was already created, so don't stomp on it.
    if (!lastParentNode->getChildAtIndex(indexOfNewChild)) {
        lastParentNode->addChildAtIndex(indexOfNewChild);
    }

    // This works because we know we traversed down the same tree so if the length is the same, then the whole code is the same
    if (*lastParentNode->getChildAtIndex(indexOfNewChild)->getOctalCode() == *codeToReach) {
        return lastParentNode->getChildAtIndex(indexOfNewChild);
    } else {
        return createMissingNode(lastParentNode->getChildAtIndex(indexOfNewChild), codeToReach);
    }
}

int VoxelTree::readNodeData(VoxelNode* destinationNode,
                            unsigned char* nodeData,
                            int bytesLeftToRead) {
    // instantiate variable for bytes already read
    int bytesRead = 1;
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        // check the colors mask to see if we have a child to color in
        if (oneAtBit(*nodeData, i)) {
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
            nodeColor newColor;
            memcpy(newColor, nodeData + bytesRead, 3);
            newColor[3] = 1;
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
           
            bytesRead += 3;
        }
    }

    // give this destination node the child mask from the packet
    unsigned char childMask = *(nodeData + bytesRead);
    
    int childIndex = 0;
    bytesRead++;
    
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
                                      nodeData + bytesRead,
                                      bytesLeftToRead - bytesRead);
        }
        
        childIndex++;
    }
    
    return bytesRead;
}

void VoxelTree::readBitstreamToTree(unsigned char * bitstream, unsigned long int bufferSizeBytes) {
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
                                bufferSizeBytes - (bytesRead + octalCodeBytes));

        // skip bitstream to new startPoint
        bitstreamAt += theseBytesRead;
        bytesRead +=  theseBytesRead;
    }

    this->voxelsBytesRead += bufferSizeBytes;
    this->voxelsBytesReadStats.updateAverage(bufferSizeBytes);
}

void VoxelTree::deleteVoxelAt(float x, float y, float z, float s) {
    unsigned char* octalCode = pointToVoxel(x,y,z,s,0,0,0);
    deleteVoxelCodeFromTree(octalCode);
    delete octalCode; // cleanup memory
}


// Note: uses the codeColorBuffer format, but the color's are ignored, because
// this only finds and deletes the node from the tree.
void VoxelTree::deleteVoxelCodeFromTree(unsigned char *codeBuffer) {
    VoxelNode* parentNode = NULL;
    VoxelNode* nodeToDelete = nodeForOctalCode(rootNode, codeBuffer, &parentNode);

    // If the node exists...
    int lengthInBytes = bytesRequiredForCodeLength(*codeBuffer); // includes octet count, not color!

    if (0 == memcmp(nodeToDelete->getOctalCode(),codeBuffer,lengthInBytes)) {
        if (parentNode) {
            int childIndex = branchIndexWithDescendant(parentNode->getOctalCode(), codeBuffer);

            parentNode->deleteChildAtIndex(childIndex);

            reaverageVoxelColors(rootNode); // Fix our colors!! Need to call it on rootNode
            _isDirty = true;
        }
    }
}

void VoxelTree::eraseAllVoxels() {
    // XXXBHG Hack attack - is there a better way to erase the voxel tree?
    delete rootNode; // this will recurse and delete all children
    rootNode = new VoxelNode();
    _isDirty = true;
}

void VoxelTree::readCodeColorBufferToTree(unsigned char *codeColorBuffer) {
    VoxelNode* lastCreatedNode = nodeForOctalCode(rootNode, codeColorBuffer, NULL);

    // create the node if it does not exist
    if (*lastCreatedNode->getOctalCode() != *codeColorBuffer) {
        lastCreatedNode = createMissingNode(lastCreatedNode, codeColorBuffer);
        _isDirty = true;
    }

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
    bool hasChildren = false;

    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        if (startNode->getChildAtIndex(i)) {
            reaverageVoxelColors(startNode->getChildAtIndex(i));
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

void VoxelTree::createVoxel(float x, float y, float z, float s, unsigned char red, unsigned char green, unsigned char blue) {
    unsigned char* voxelData = pointToVoxel(x,y,z,s,red,green,blue);
    this->readCodeColorBufferToTree(voxelData);
    delete voxelData;
}


void VoxelTree::createLine(glm::vec3 point1, glm::vec3 point2, float unitSize, rgbColor color) {
    glm::vec3 distance = point2 - point1;
    glm::vec3 items = distance / unitSize;
    int maxItems = std::max(items.x, std::max(items.y, items.z));
    glm::vec3 increment = distance * (1.0f/ maxItems);
    glm::vec3 pointAt = point1;
    for (int i = 0; i <= maxItems; i++ ) {
        pointAt += increment;
        unsigned char* voxelData = pointToVoxel(pointAt.x,pointAt.y,pointAt.z,unitSize,color[0],color[1],color[2]);
        readCodeColorBufferToTree(voxelData);
        delete voxelData;
    }
}

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

int VoxelTree::searchForColoredNodes(int maxSearchLevel, VoxelNode* node, const ViewFrustum& viewFrustum, VoxelNodeBag& bag) {

    // call the recursive version, this will add all found colored node roots to the bag
    int currentSearchLevel = 0;
    
    int levelReached = searchForColoredNodesRecursion(maxSearchLevel, currentSearchLevel, rootNode, viewFrustum, bag);
    return levelReached;
}

// combines the ray cast arguments into a single object
class RayArgs {
public:
    glm::vec3 origin;
    glm::vec3 direction;
    VoxelNode*& node;
    float& distance;
    bool found;
};

bool findRayOperation(VoxelNode* node, void* extraData) {
    RayArgs* args = static_cast<RayArgs*>(extraData);
    AABox box = node->getAABox();
    float distance;
    if (!box.findRayIntersection(args->origin, args->direction, distance)) {
        return false;
    }
    if (!node->isLeaf()) {
        return true; // recurse on children
    }
    if (!args->found || distance < args->distance) {
        args->node = node;
        args->distance = distance;
        args->found = true;
    }
    return false;
}

bool VoxelTree::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, VoxelNode*& node, float& distance)
{
    RayArgs args = { origin / (float)TREE_SCALE, direction, node, distance };
    recurseTreeWithOperation(findRayOperation, &args);
    return args.found;
}

int VoxelTree::searchForColoredNodesRecursion(int maxSearchLevel, int& currentSearchLevel, 
                                              VoxelNode* node, const ViewFrustum& viewFrustum, VoxelNodeBag& bag) {

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
            int childLevelReached = searchForColoredNodesRecursion(maxSearchLevel, thisLevel, childNode, viewFrustum, bag);
            maxChildLevel = std::max(maxChildLevel, childLevelReached);
        }
    }
    return maxChildLevel;
}

int VoxelTree::encodeTreeBitstream(int maxEncodeLevel, VoxelNode* node, unsigned char* outputBuffer, int availableBytes, 
                                   VoxelNodeBag& bag, const ViewFrustum* viewFrustum) const {

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
    int childBytesWritten = encodeTreeBitstreamRecursion(maxEncodeLevel, currentEncodeLevel, 
                                                         node, outputBuffer, availableBytes, bag, viewFrustum);

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

int VoxelTree::encodeTreeBitstreamRecursion(int maxEncodeLevel, int& currentEncodeLevel,
                                            VoxelNode* node, unsigned char* outputBuffer, int availableBytes,
                                            VoxelNodeBag& bag, const ViewFrustum* viewFrustum) const {
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

    unsigned char childrenExistBits = 0;
    unsigned char childrenColoredBits = 0;
    int inViewCount = 0;
    int inViewNotLeafCount = 0;
    int inViewWithColorCount = 0;

    // for each child node, check to see if they exist, are colored, and in view, and if so
    // add them to our distance ordered array of children
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        VoxelNode* childNode = node->getChildAtIndex(i);
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
                    childrenExistBits += (1 << (7 - i));
                    inViewNotLeafCount++;
                }
            
                // track children with actual color
                if (childNode && childNode->isColored()) {
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
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        if (oneAtBit(childrenColoredBits, i)) {
            memcpy(writeToThisLevelBuffer, &node->getChildAtIndex(i)->getColor(), BYTES_PER_COLOR);
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
        // childrenExistBits. But... we don't really know how big the child tree will be. And we don't know if
        // we'll have room in our buffer to actually write all these child trees. What we kinda would like to do is
        // write our childExistsBits as a place holder. Then let each potential tree have a go at it. If they 
        // write something, we keep them in the bits, if they don't, we take them out.
        //
        // we know the last thing we wrote to the outputBuffer was our childrenExistBits. Let's remember where that was!
        unsigned char* childExistsPlaceHolder = outputBuffer-sizeof(childrenExistBits);

        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        
            if (oneAtBit(childrenExistBits, i)) {
                VoxelNode* childNode = node->getChildAtIndex(i);
                
                int thisLevel = currentEncodeLevel;
                int childTreeBytesOut = encodeTreeBitstreamRecursion(maxEncodeLevel, thisLevel, childNode, 
                                                                     outputBuffer, availableBytes, bag, viewFrustum);
                
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
                if (childTreeBytesOut == 2) {
                    childTreeBytesOut = 0; // this is the degenerate case of a tree with no colors and no child trees
                }

                bytesAtThisLevel += childTreeBytesOut;
                availableBytes -= childTreeBytesOut;
                outputBuffer += childTreeBytesOut;

                // If we had previously started writing, and if the child DIDN'T write any bytes,
                // then we want to remove their bit from the childExistsPlaceHolder bitmask
                if (childTreeBytesOut == 0) {
                    // remove this child's bit...
                    childrenExistBits -= (1 << (7 - i));
                    // repair the child exists mask
                    *childExistsPlaceHolder = childrenExistBits;
                    // Note: no need to move the pointer, cause we already stored this
                } // end if (childTreeBytesOut == 0)
            } // end if (oneAtBit(childrenExistBits, i))
        } // end for
    } // end keepDiggingDeeper
    return bytesAtThisLevel;
}

bool VoxelTree::readFromFileV2(const char* fileName) {
    std::ifstream file(fileName, std::ios::in|std::ios::binary|std::ios::ate);
    if(file.is_open()) {
		printLog("loading file...\n");
		
		// get file length....
        unsigned long fileLength = file.tellg();
        file.seekg( 0, std::ios::beg );
        
        // read the entire file into a buffer, WHAT!? Why not.
        unsigned char* entireFile = new unsigned char[fileLength];
        file.read((char*)entireFile, fileLength);
        readBitstreamToTree(entireFile, fileLength);
        delete[] entireFile;
    		
        file.close();
        return true;
    }
    return false;
}

void VoxelTree::writeToFileV2(const char* fileName) const {

    std::ofstream file(fileName, std::ios::out|std::ios::binary);

    if(file.is_open()) {
        VoxelNodeBag nodeBag;
        nodeBag.insert(rootNode);

        static unsigned char outputBuffer[MAX_VOXEL_PACKET_SIZE - 1]; // save on allocs by making this static
        int bytesWritten = 0;
        
        while (!nodeBag.isEmpty()) {
            VoxelNode* subTree = nodeBag.extract();
            bytesWritten = encodeTreeBitstream(INT_MAX, subTree, &outputBuffer[0], MAX_VOXEL_PACKET_SIZE - 1, nodeBag, NULL);
                                                      
            file.write((const char*)&outputBuffer[0], bytesWritten);
        }
    }
    file.close();
}

