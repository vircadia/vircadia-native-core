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
#include <fstream> // to load voxels from file

#include <glm/gtc/noise.hpp>

#include <QDebug>

#include "CoverageMap.h"
#include "GeometryUtil.h"
#include "OctalCode.h"
#include "PacketHeaders.h"
#include "SharedUtil.h"
#include "SquarePixelMap.h"
#include "Tags.h"
#include "ViewFrustum.h"
#include "VoxelConstants.h"
#include "VoxelNodeBag.h"
#include "VoxelTree.h"

float boundaryDistanceForRenderLevel(unsigned int renderLevel) {
    return ::VOXEL_SIZE_SCALE / powf(2, renderLevel);
}

float boundaryDistanceSquaredForRenderLevel(unsigned int renderLevel) {
    const float voxelSizeScale = (::VOXEL_SIZE_SCALE/TREE_SCALE) * (::VOXEL_SIZE_SCALE/TREE_SCALE);
    return voxelSizeScale / powf(2, (2 * renderLevel));
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


void VoxelTree::recurseTreeWithOperationDistanceSortedTimed(PointerStack* stackOfNodes, long allowedTime,
                                                            RecurseVoxelTreeOperation operation, 
                                                            const glm::vec3& point, void* extraData) {

    long long start = usecTimestampNow();
    
    // start case, stack empty, so start with root...
    if (stackOfNodes->empty()) {
        stackOfNodes->push(rootNode);
    }
    while (!stackOfNodes->empty()) {
        VoxelNode* node = (VoxelNode*)stackOfNodes->top();
        stackOfNodes->pop();
    
        if (operation(node, extraData)) {

            //sortChildren... CLOSEST to FURTHEST
            // determine the distance sorted order of our children
            VoxelNode*  sortedChildren[NUMBER_OF_CHILDREN];
            float       distancesToChildren[NUMBER_OF_CHILDREN];
            int         indexOfChildren[NUMBER_OF_CHILDREN]; // not really needed
            int         currentCount = 0;

            for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
                VoxelNode* childNode = node->getChildAtIndex(i);
                if (childNode) {
                    // chance to optimize, doesn't need to be actual distance!! Could be distance squared
                    float distanceSquared = childNode->distanceSquareToPoint(point); 
                    currentCount = insertIntoSortedArrays((void*)childNode, distanceSquared, i,
                                                          (void**)&sortedChildren, (float*)&distancesToChildren, 
                                                          (int*)&indexOfChildren, currentCount, NUMBER_OF_CHILDREN);
                }
            }

            //iterate sorted children FURTHEST to CLOSEST
            for (int i = currentCount-1; i >= 0; i--) {
                VoxelNode* child = sortedChildren[i];
                stackOfNodes->push(child);
            }
        }

        // at this point, we can check to see if we should bail for timing reasons
        // because if we bail at this point, then reenter the while, we will basically
        // be back to processing the stack from same place we left off, and all can proceed normally
        long long now = usecTimestampNow();
        long elapsedTime = now - start;

        if (elapsedTime > allowedTime) {
            return; // caller responsible for calling us again to finish the job!
        }
    }
}


// Recurses voxel tree calling the RecurseVoxelTreeOperation function for each node.
// stops recursion if operation function returns false.
void VoxelTree::recurseTreeWithOperation(RecurseVoxelTreeOperation operation, void* extraData) {
    recurseNodeWithOperation(rootNode, operation, extraData);
}

// Recurses voxel node with an operation function
void VoxelTree::recurseNodeWithOperation(VoxelNode* node, RecurseVoxelTreeOperation operation, void* extraData) {
    if (operation(node, extraData)) {
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            VoxelNode* child = node->getChildAtIndex(i);
            if (child) {
                recurseNodeWithOperation(child, operation, extraData);
            }
        }
    }
}

// Recurses voxel tree calling the RecurseVoxelTreeOperation function for each node.
// stops recursion if operation function returns false.
void VoxelTree::recurseTreeWithOperationDistanceSorted(RecurseVoxelTreeOperation operation,
                                                       const glm::vec3& point, void* extraData) {

    recurseNodeWithOperationDistanceSorted(rootNode, operation, point, extraData);
}

// Recurses voxel node with an operation function
void VoxelTree::recurseNodeWithOperationDistanceSorted(VoxelNode* node, RecurseVoxelTreeOperation operation, 
                                                       const glm::vec3& point, void* extraData) {
    if (operation(node, extraData)) {
        // determine the distance sorted order of our children
        VoxelNode*  sortedChildren[NUMBER_OF_CHILDREN];
        float       distancesToChildren[NUMBER_OF_CHILDREN];
        int         indexOfChildren[NUMBER_OF_CHILDREN]; // not really needed
        int         currentCount = 0;

        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            VoxelNode* childNode = node->getChildAtIndex(i);
            if (childNode) {
                // chance to optimize, doesn't need to be actual distance!! Could be distance squared
                float distanceSquared = childNode->distanceSquareToPoint(point);
                //qDebug("recurseNodeWithOperationDistanceSorted() CHECKING child[%d] point=%f,%f center=%f,%f distance=%f...\n", i, point.x, point.y, center.x, center.y, distance);
                //childNode->printDebugDetails("");
                currentCount = insertIntoSortedArrays((void*)childNode, distanceSquared, i,
                                                      (void**)&sortedChildren, (float*)&distancesToChildren,
                                                      (int*)&indexOfChildren, currentCount, NUMBER_OF_CHILDREN);
            }
        }

        for (int i = 0; i < currentCount; i++) {
            VoxelNode* childNode = sortedChildren[i];
            if (childNode) {
                //qDebug("recurseNodeWithOperationDistanceSorted() PROCESSING child[%d] distance=%f...\n", i, distancesToChildren[i]);
                //childNode->printDebugDetails("");
                recurseNodeWithOperationDistanceSorted(childNode, operation, point, extraData);
            }
        }
    }
}


VoxelNode* VoxelTree::nodeForOctalCode(VoxelNode* ancestorNode,
                                       unsigned char* needleCode, VoxelNode** parentOfFoundNode) const {
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
                destinationNode->safeDeepDeleteChildAtIndex(i);
                _isDirty = true; // by definition!
            }
        }
    }
    return bytesRead;
}

void VoxelTree::readBitstreamToTree(unsigned char * bitstream, unsigned long int bufferSizeBytes,
                                    bool includeColor, bool includeExistsBits, VoxelNode* destinationNode) {
    int bytesRead = 0;
    unsigned char* bitstreamAt = bitstream;

    // If destination node is not included, set it to root
    if (!destinationNode) {
        destinationNode = rootNode;
    }

    _nodesChangedFromBitstream = 0;

    // Keep looping through the buffer calling readNodeData() this allows us to pack multiple root-relative Octal codes
    // into a single network packet. readNodeData() basically goes down a tree from the root, and fills things in from there
    // if there are more bytes after that, it's assumed to be another root relative tree

    while (bitstreamAt < bitstream + bufferSizeBytes) {
        VoxelNode* bitstreamRootNode = nodeForOctalCode(destinationNode, (unsigned char *)bitstreamAt, NULL);
        if (*bitstreamAt != *bitstreamRootNode->getOctalCode()) {
            // if the octal code returned is not on the same level as
            // the code being searched for, we have VoxelNodes to create

            // Note: we need to create this node relative to root, because we're assuming that the bitstream for the initial
            // octal code is always relative to root!
            bitstreamRootNode = createMissingNode(destinationNode, (unsigned char*) bitstreamAt);
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

void VoxelTree::deleteVoxelAt(float x, float y, float z, float s) {
    unsigned char* octalCode = pointToVoxel(x,y,z,s,0,0,0);
    deleteVoxelCodeFromTree(octalCode);
    delete[] octalCode; // cleanup memory
}

class DeleteVoxelCodeFromTreeArgs {
public:
    bool            collapseEmptyTrees;
    unsigned char*  codeBuffer;
    int             lengthOfCode;
    bool            deleteLastChild;
    bool            pathChanged;
};

// Note: uses the codeColorBuffer format, but the color's are ignored, because
// this only finds and deletes the node from the tree.
void VoxelTree::deleteVoxelCodeFromTree(unsigned char* codeBuffer, bool collapseEmptyTrees) {
    // recurse the tree while decoding the codeBuffer, once you find the node in question, recurse
    // back and implement color reaveraging, and marking of lastChanged
    DeleteVoxelCodeFromTreeArgs args;
    args.collapseEmptyTrees = collapseEmptyTrees;
    args.codeBuffer         = codeBuffer;
    args.lengthOfCode       = numberOfThreeBitSectionsInCode(codeBuffer);
    args.deleteLastChild    = false;
    args.pathChanged        = false;

    VoxelNode* node = rootNode;
    deleteVoxelCodeFromTreeRecursion(node, &args);
}

void VoxelTree::deleteVoxelCodeFromTreeRecursion(VoxelNode* node, void* extraData) {
    DeleteVoxelCodeFromTreeArgs* args = (DeleteVoxelCodeFromTreeArgs*)extraData;

    int lengthOfNodeCode = numberOfThreeBitSectionsInCode(node->getOctalCode());

    // Since we traverse the tree in code order, we know that if our code
    // matches, then we've reached  our target node.
    if (lengthOfNodeCode == args->lengthOfCode) {
        // we've reached our target, depending on how we're called we may be able to operate on it
        // it here, we need to recurse up, and delete it there. So we handle these cases the same to keep
        // the logic consistent.
        args->deleteLastChild = true;
        return;
    }

    // Ok, we know we haven't reached our target node yet, so keep looking
    int childIndex = branchIndexWithDescendant(node->getOctalCode(), args->codeBuffer);
    VoxelNode* childNode = node->getChildAtIndex(childIndex);

    // If there is no child at the target location, and the current parent node is a colored leaf,
    // then it means we were asked to delete a child out of a larger leaf voxel.
    // We support this by breaking up the parent voxel into smaller pieces.
    if (!childNode && node->isLeaf() && node->isColored()) {
        // we need to break up ancestors until we get to the right level
        VoxelNode* ancestorNode = node;
        while (true) {
            int index = branchIndexWithDescendant(ancestorNode->getOctalCode(), args->codeBuffer);
            for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
                if (i != index) {
                    ancestorNode->addChildAtIndex(i);
                    if (node->isColored()) {
                        ancestorNode->getChildAtIndex(i)->setColor(node->getColor());
                    }
                }
            }
            int lengthOfancestorNode = numberOfThreeBitSectionsInCode(ancestorNode->getOctalCode());

            // If we've reached the parent of the target, then stop breaking up children
            if (lengthOfancestorNode == (args->lengthOfCode - 1)) {
                break;
            }
            ancestorNode->addChildAtIndex(index);
            ancestorNode = ancestorNode->getChildAtIndex(index);
            if (node->isColored()) {
                ancestorNode->setColor(node->getColor());
            }
        }
        _isDirty = true;
        args->pathChanged = true;

        // ends recursion, unwinds up stack
        return;
    }

    // if we don't have a child and we reach this point, then we actually know that the parent
    // isn't a colored leaf, and the child branch doesn't exist, so there's nothing to do below and
    // we can safely return, ending the recursion and unwinding
    if (!childNode) {
        //qDebug("new___deleteVoxelCodeFromTree() child branch doesn't exist, but parent is not a leaf, just unwind\n");
        return;
    }

    // If we got this far then we have a child for the branch we're looking for, but we're not there yet
    // recurse till we get there
    deleteVoxelCodeFromTreeRecursion(childNode, args);

    // If the lower level determined it needs to be deleted, then we should delete now.
    if (args->deleteLastChild) {
        node->deleteChildAtIndex(childIndex); // note: this will track dirtiness and lastChanged for this node

        // track our tree dirtiness
        _isDirty = true;

        // track that path has changed
        args->pathChanged = true;

        // If we're in collapseEmptyTrees mode, and this was the last child of this node, then we also want
        // to delete this node.  This will collapse the empty tree above us.
        if (args->collapseEmptyTrees && node->getChildCount() == 0) {
            // Can't delete the root this way.
            if (node == rootNode) {
                args->deleteLastChild = false; // reset so that further up the unwinding chain we don't do anything
            }
        } else {
            args->deleteLastChild = false; // reset so that further up the unwinding chain we don't do anything
        }
    }

    // If the lower level did some work, then we need to let this node know, so it can
    // do any bookkeeping it wants to, like color re-averaging, time stamp marking, etc
    if (args->pathChanged) {
        node->handleSubtreeChanged(this);
    }
}

void VoxelTree::eraseAllVoxels() {
    // XXXBHG Hack attack - is there a better way to erase the voxel tree?
    delete rootNode; // this will recurse and delete all children
    rootNode = new VoxelNode();
    _isDirty = true;
}

class ReadCodeColorBufferToTreeArgs {
public:
    unsigned char*  codeColorBuffer;
    int             lengthOfCode;
    bool            destructive;
    bool            pathChanged;
};

void VoxelTree::readCodeColorBufferToTree(unsigned char* codeColorBuffer, bool destructive) {
    ReadCodeColorBufferToTreeArgs args;
    args.codeColorBuffer = codeColorBuffer;
    args.lengthOfCode    = numberOfThreeBitSectionsInCode(codeColorBuffer);
    args.destructive     = destructive;
    args.pathChanged     = false;


    VoxelNode* node = rootNode;

    readCodeColorBufferToTreeRecursion(node, &args);
}


void VoxelTree::readCodeColorBufferToTreeRecursion(VoxelNode* node, void* extraData) {
    ReadCodeColorBufferToTreeArgs* args = (ReadCodeColorBufferToTreeArgs*)extraData;

    int lengthOfNodeCode = numberOfThreeBitSectionsInCode(node->getOctalCode());

    // Since we traverse the tree in code order, we know that if our code
    // matches, then we've reached  our target node.
    if (lengthOfNodeCode == args->lengthOfCode) {
        // we've reached our target -- we might have found our node, but that node might have children.
        // in this case, we only allow you to set the color if you explicitly asked for a destructive
        // write.
        if (!node->isLeaf() && args->destructive) {
            // if it does exist, make sure it has no children
            for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
                node->deleteChildAtIndex(i);
            }
        } else {
            if (!node->isLeaf()) {
                qDebug("WARNING! operation would require deleting children, add Voxel ignored!\n ");
            }
        }

        // If we get here, then it means, we either had a true leaf to begin with, or we were in
        // destructive mode and we deleted all the child trees. So we can color.
        if (node->isLeaf()) {
            // give this node its color
            int octalCodeBytes = bytesRequiredForCodeLength(args->lengthOfCode);

            nodeColor newColor;
            memcpy(newColor, args->codeColorBuffer + octalCodeBytes, SIZE_OF_COLOR_DATA);
            newColor[SIZE_OF_COLOR_DATA] = 1;
            node->setColor(newColor);

            // It's possible we just reset the node to it's exact same color, in
            // which case we don't consider this to be dirty...
            if (node->isDirty()) {
                // track our tree dirtiness
                _isDirty = true;
                // track that path has changed
                args->pathChanged = true;
            }
        }
        return;
    }

    // Ok, we know we haven't reached our target node yet, so keep looking
    int childIndex = branchIndexWithDescendant(node->getOctalCode(), args->codeColorBuffer);
    VoxelNode* childNode = node->getChildAtIndex(childIndex);

    // If the branch we need to traverse does not exist, then create it on the way down...
    if (!childNode) {
        childNode = node->addChildAtIndex(childIndex);
    }

    // recurse...
    readCodeColorBufferToTreeRecursion(childNode, args);

    // Unwinding...

    // If the lower level did some work, then we need to let this node know, so it can
    // do any bookkeeping it wants to, like color re-averaging, time stamp marking, etc
    if (args->pathChanged) {
        node->handleSubtreeChanged(this);
    }
}

void VoxelTree::processRemoveVoxelBitstream(unsigned char * bitstream, int bufferSizeBytes) {
    //unsigned short int itemNumber = (*((unsigned short int*)&bitstream[sizeof(PACKET_HEADER)]));
    int atByte = sizeof(short int) + numBytesForPacketHeader(bitstream);
    unsigned char* voxelCode = (unsigned char*)&bitstream[atByte];
    while (atByte < bufferSizeBytes) {
        int codeLength = numberOfThreeBitSectionsInCode(voxelCode);
        int voxelDataSize = bytesRequiredForCodeLength(codeLength) + SIZE_OF_COLOR_DATA;

        deleteVoxelCodeFromTree(voxelCode, COLLAPSE_EMPTY_TREE);

        voxelCode+=voxelDataSize;
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

    qDebug("color mask: ");
    outputBits(colorMask);

    // output the colors we have
    for (int j = 0; j < NUMBER_OF_CHILDREN; j++) {
        if (startNode->getChildAtIndex(j) && startNode->getChildAtIndex(j)->isColored()) {
            qDebug("color %d : ",j);
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

    qDebug("child mask: ");
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

// Note: this is an expensive call. Don't call it unless you really need to reaverage the entire tree (from startNode)
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
        
        // this is also a good time to recalculateSubTreeNodeCount()
        startNode->recalculateSubTreeNodeCount();
    }
}

void VoxelTree::loadVoxelsFile(const char* fileName, bool wantColorRandomizer) {
    int vCount = 0;

    std::ifstream file(fileName, std::ios::in|std::ios::binary);

    char octets;
    unsigned int lengthInBytes;

    int totalBytesRead = 0;
    if(file.is_open()) {
        qDebug("loading file...\n");
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

            qDebug("voxel color from file  red:%d, green:%d, blue:%d \n",red,green,blue);
            vCount++;

            int colorRandomizer = wantColorRandomizer ? randIntInRange (-5, 5) : 0;
            voxelData[lengthInBytes+1] = std::max(0,std::min(255,red + colorRandomizer));
            voxelData[lengthInBytes+2] = std::max(0,std::min(255,green + colorRandomizer));
            voxelData[lengthInBytes+3] = std::max(0,std::min(255,blue + colorRandomizer));
            qDebug("voxel color after rand red:%d, green:%d, blue:%d\n",
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
    delete[] octalCode; // cleanup memory
    return node;
}

void VoxelTree::createVoxel(float x, float y, float z, float s,
                            unsigned char red, unsigned char green, unsigned char blue, bool destructive) {
    unsigned char* voxelData = pointToVoxel(x,y,z,s,red,green,blue);
    this->readCodeColorBufferToTree(voxelData, destructive);
    delete[] voxelData;
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
            qDebug("percentComplete=%d\n",percentComplete);
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
                        qDebug("adding candy shell: theta=%f phi=%f thisRadius=%f radius=%f\n",
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
                            qDebug("perlin=%f gradient=%f color=(%d,%d,%d)\n",perlin, gradient, red, green, blue);
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


int VoxelTree::encodeTreeBitstream(VoxelNode* node, unsigned char* outputBuffer, int availableBytes, VoxelNodeBag& bag,
                                   EncodeBitstreamParams& params) const {

    // How many bytes have we written so far at this level;
    int bytesWritten = 0;
    
    // If we're at a node that is out of view, then we can return, because no nodes below us will be in view!
    if (params.viewFrustum && !node->isInView(*params.viewFrustum)) {
        return bytesWritten;
    }
    
    // write the octal code
    int codeLength;
    if (params.chopLevels) {
        unsigned char* newCode = chopOctalCode(node->getOctalCode(), params.chopLevels);
        if (newCode) {
            codeLength = bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(newCode));
            memcpy(outputBuffer, newCode, codeLength);
            delete newCode;
        } else {
            codeLength = 1; // chopped to root!
            *outputBuffer = 0; // root
        }
    } else {
        codeLength = bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(node->getOctalCode()));
        memcpy(outputBuffer, node->getOctalCode(), codeLength);
    }

    outputBuffer += codeLength; // move the pointer
    bytesWritten += codeLength; // keep track of byte count
    availableBytes -= codeLength; // keep track or remaining space

    int currentEncodeLevel = 0;
    
    // record some stats, this is the one node that we won't record below in the recursion function, so we need to 
    // track it here
    if (params.stats) {
        params.stats->traversed(node);
    }
    
    int childBytesWritten = encodeTreeBitstreamRecursion(node, outputBuffer, availableBytes, bag, params, currentEncodeLevel);

    // if childBytesWritten == 1 then something went wrong... that's not possible
    assert(childBytesWritten != 1);

    // if includeColor and childBytesWritten == 2, then it can only mean that the lower level trees don't exist or for some reason
    // couldn't be written... so reset them here... This isn't true for the non-color included case
    if (params.includeColor && childBytesWritten == 2) {
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

int VoxelTree::encodeTreeBitstreamRecursion(VoxelNode* node, unsigned char* outputBuffer, int availableBytes, VoxelNodeBag& bag,
                                            EncodeBitstreamParams& params, int& currentEncodeLevel) const {

    // you can't call this without a valid node
    assert(node);
    
    // How many bytes have we written so far at this level;
    int bytesAtThisLevel = 0;

    // Keep track of how deep we've encoded.
    currentEncodeLevel++;
    
    params.maxLevelReached = std::max(currentEncodeLevel,params.maxLevelReached);

    // If we've reached our max Search Level, then stop searching.
    if (currentEncodeLevel >= params.maxEncodeLevel) {
        return bytesAtThisLevel;
    }
    
    // caller can pass NULL as viewFrustum if they want everything
    if (params.viewFrustum) {
        float distance = node->distanceToCamera(*params.viewFrustum);
        float boundaryDistance = boundaryDistanceForRenderLevel(node->getLevel() + params.boundaryLevelAdjust);

        // If we're too far away for our render level, then just return
        if (distance >= boundaryDistance) {
            if (params.stats) {
                params.stats->skippedDistance(node);
            }
            return bytesAtThisLevel;
        }

        // If we're at a node that is out of view, then we can return, because no nodes below us will be in view!
        // although technically, we really shouldn't ever be here, because our callers shouldn't be calling us if
        // we're out of view
        if (!node->isInView(*params.viewFrustum)) {
            if (params.stats) {
                params.stats->skippedOutOfView(node);
            }
            return bytesAtThisLevel;
        }
        
        // Ok, we are in view, but if we're in delta mode, then we also want to make sure we weren't already in view
        // because we don't send nodes from the previously know in view frustum.
        bool wasInView = false;
        
        if (params.deltaViewFrustum && params.lastViewFrustum) {
            ViewFrustum::location location = node->inFrustum(*params.lastViewFrustum);
            
            // If we're a leaf, then either intersect or inside is considered "formerly in view"
            if (node->isLeaf()) {
                wasInView = location != ViewFrustum::OUTSIDE;
            } else {
                wasInView = location == ViewFrustum::INSIDE;
            }
        }

        // If we were previously in the view, then we normally will return out of here and stop recursing. But
        // if we're in deltaViewFrustum mode, and this node has changed since it was last sent, then we do
        // need to send it.
        if (wasInView && !(params.deltaViewFrustum && node->hasChangedSince(params.lastViewFrustumSent - CHANGE_FUDGE))) {
            if (params.stats) {
                params.stats->skippedWasInView(node);
            }
            return bytesAtThisLevel;
        }

        // If we're not in delta sending mode, and we weren't asked to do a force send, and the voxel hasn't changed, 
        // then we can also bail early and save bits
        if (!params.forceSendScene && !params.deltaViewFrustum && 
            !node->hasChangedSince(params.lastViewFrustumSent - CHANGE_FUDGE)) {
            if (params.stats) {
                params.stats->skippedNoChange(node);
            }
            return bytesAtThisLevel;
        }

        // If the user also asked for occlusion culling, check if this node is occluded, but only if it's not a leaf.
        // leaf occlusion is handled down below when we check child nodes
        if (params.wantOcclusionCulling && !node->isLeaf()) {
            //node->printDebugDetails("upper section, params.wantOcclusionCulling...  node=");
            AABox voxelBox = node->getAABox();
            voxelBox.scale(TREE_SCALE);
            VoxelProjectedPolygon* voxelPolygon = new VoxelProjectedPolygon(params.viewFrustum->getProjectedPolygon(voxelBox));

            // In order to check occlusion culling, the shadow has to be "all in view" otherwise, we will ignore occlusion
            // culling and proceed as normal
            if (voxelPolygon->getAllInView()) {
                //node->printDebugDetails("upper section, voxelPolygon->getAllInView() node=");

                CoverageMapStorageResult result = params.map->checkMap(voxelPolygon, false);
                delete voxelPolygon; // cleanup
                if (result == OCCLUDED) {
                    if (params.stats) {
                        params.stats->skippedOccluded(node);
                    }
                    return bytesAtThisLevel;
                }
            } else {
                // If this shadow wasn't "all in view" then we ignored it for occlusion culling, but
                // we do need to clean up memory and proceed as normal...
                delete voxelPolygon;
            }
        }
    }

    bool keepDiggingDeeper = true; // Assuming we're in view we have a great work ethic, we're always ready for more!

    // At any given point in writing the bitstream, the largest minimum we might need to flesh out the current level
    // is 1 byte for child colors + 3*NUMBER_OF_CHILDREN bytes for the actual colors + 1 byte for child trees. There could be sub trees
    // below this point, which might take many more bytes, but that's ok, because we can always mark our subtrees as
    // not existing and stop the packet at this point, then start up with a new packet for the remaining sub trees.
    unsigned char childrenExistInTreeBits = 0;
    unsigned char childrenExistInPacketBits = 0;
    unsigned char childrenColoredBits = 0;

    const int CHILD_COLOR_MASK_BYTES = sizeof(childrenColoredBits);
    const int BYTES_PER_COLOR = 3;
    const int CHILD_TREE_EXISTS_BYTES = sizeof(childrenExistInTreeBits) + sizeof(childrenExistInPacketBits);
    const int MAX_LEVEL_BYTES = CHILD_COLOR_MASK_BYTES + NUMBER_OF_CHILDREN * BYTES_PER_COLOR + CHILD_TREE_EXISTS_BYTES;

    // Make our local buffer large enough to handle writing at this level in case we need to.
    unsigned char thisLevelBuffer[MAX_LEVEL_BYTES];
    unsigned char* writeToThisLevelBuffer = &thisLevelBuffer[0];

    int inViewCount = 0;
    int inViewNotLeafCount = 0;
    int inViewWithColorCount = 0;

    VoxelNode*  sortedChildren[NUMBER_OF_CHILDREN];
    float       distancesToChildren[NUMBER_OF_CHILDREN];
    int         indexOfChildren[NUMBER_OF_CHILDREN]; // not really needed
    int         currentCount = 0;

    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        VoxelNode* childNode = node->getChildAtIndex(i);

        // if the caller wants to include childExistsBits, then include them even if not in view
        if (params.includeExistsBits && childNode) {
            childrenExistInTreeBits += (1 << (7 - i));
        }

        if (params.wantOcclusionCulling) {
            if (childNode) {
                // chance to optimize, doesn't need to be actual distance!! Could be distance squared
                //float distanceSquared = childNode->distanceSquareToPoint(point);
                //qDebug("recurseNodeWithOperationDistanceSorted() CHECKING child[%d] point=%f,%f center=%f,%f distance=%f...\n", i, point.x, point.y, center.x, center.y, distance);
                //childNode->printDebugDetails("");

                float distance = params.viewFrustum ? childNode->distanceToCamera(*params.viewFrustum) : 0;

                currentCount = insertIntoSortedArrays((void*)childNode, distance, i,
                                                      (void**)&sortedChildren, (float*)&distancesToChildren,
                                                      (int*)&indexOfChildren, currentCount, NUMBER_OF_CHILDREN);
            }
        } else {
            sortedChildren[i] = childNode;
            indexOfChildren[i] = i;
            distancesToChildren[i] = 0.0f;
            currentCount++;
        }

        // track stats
        // must check childNode here, because it could be we got here with no childNode
        if (params.stats && childNode) {
            params.stats->traversed(childNode);
        }
    
    }

    // for each child node in Distance sorted order..., check to see if they exist, are colored, and in view, and if so
    // add them to our distance ordered array of children
    for (int i = 0; i < currentCount; i++) {
        VoxelNode* childNode = sortedChildren[i];
        int originalIndex = indexOfChildren[i];

        bool childIsInView  = (childNode && (!params.viewFrustum || childNode->isInView(*params.viewFrustum)));

        if (!childIsInView) {
            // must check childNode here, because it could be we got here because there was no childNode
            if (params.stats && childNode) {
                params.stats->skippedOutOfView(childNode);
            }
        } else {
            // Before we determine consider this further, let's see if it's in our LOD scope...
            float distance = distancesToChildren[i]; // params.viewFrustum ? childNode->distanceToCamera(*params.viewFrustum) : 0;
            float boundaryDistance = !params.viewFrustum ? 1 :
                                     boundaryDistanceForRenderLevel(childNode->getLevel() + params.boundaryLevelAdjust);

            if (!(distance < boundaryDistance)) {
                // don't need to check childNode here, because we can't get here with no childNode
                if (params.stats) {
                    params.stats->skippedDistance(childNode);
                }
            } else {
                inViewCount++;

                // track children in view as existing and not a leaf, if they're a leaf,
                // we don't care about recursing deeper on them, and we don't consider their
                // subtree to exist
                if (!(childNode && childNode->isLeaf())) {
                    childrenExistInPacketBits += (1 << (7 - originalIndex));
                    inViewNotLeafCount++;
                }

                bool childIsOccluded = false; // assume it's not occluded

                // If the user also asked for occlusion culling, check if this node is occluded
                if (params.wantOcclusionCulling && childNode->isLeaf()) {
                    // Don't check occlusion here, just add them to our distance ordered array...

                    AABox voxelBox = childNode->getAABox();
                    voxelBox.scale(TREE_SCALE);
                    VoxelProjectedPolygon* voxelPolygon = new VoxelProjectedPolygon(
                                params.viewFrustum->getProjectedPolygon(voxelBox));

                    // In order to check occlusion culling, the shadow has to be "all in view" otherwise, we will ignore occlusion
                    // culling and proceed as normal
                    if (voxelPolygon->getAllInView()) {
                        CoverageMapStorageResult result = params.map->checkMap(voxelPolygon, true);

                        // In all cases where the shadow wasn't stored, we need to free our own memory.
                        // In the case where it is stored, the CoverageMap will free memory for us later.
                        if (result != STORED) {
                            delete voxelPolygon;
                        }

                        // If while attempting to add this voxel's shadow, we determined it was occluded, then
                        // we don't need to process it further and we can exit early.
                        if (result == OCCLUDED) {
                            childIsOccluded = true;
                        }
                    } else {
                        delete voxelPolygon;
                    }
                } // wants occlusion culling & isLeaf()


                bool shouldRender = !params.viewFrustum 
                                    ? true 
                                    : childNode->calculateShouldRender(params.viewFrustum, params.boundaryLevelAdjust);
                     
                // track some stats               
                if (params.stats) {
                    // don't need to check childNode here, because we can't get here with no childNode
                    if (!shouldRender && childNode->isLeaf()) {
                        params.stats->skippedDistance(childNode);
                    }
                    // don't need to check childNode here, because we can't get here with no childNode
                    if (childIsOccluded) {
                        params.stats->skippedOccluded(childNode);
                    }
                }
                
                // track children with actual color, only if the child wasn't previously in view!
                if (shouldRender && !childIsOccluded) {
                    bool childWasInView = false;
                    
                    if (childNode && params.deltaViewFrustum && params.lastViewFrustum) {
                        ViewFrustum::location location = childNode->inFrustum(*params.lastViewFrustum);
                        
                        // If we're a leaf, then either intersect or inside is considered "formerly in view"
                        if (childNode->isLeaf()) {
                            childWasInView = location != ViewFrustum::OUTSIDE;
                        } else {
                            childWasInView = location == ViewFrustum::INSIDE;
                        }
                    }         

                    // If our child wasn't in view (or we're ignoring wasInView) then we add it to our sending items.
                    // Or if we were previously in the view, but this node has changed since it was last sent, then we do
                    // need to send it.
                    if (!childWasInView || 
                        (params.deltaViewFrustum && 
                         childNode->hasChangedSince(params.lastViewFrustumSent - CHANGE_FUDGE))){
                        childrenColoredBits += (1 << (7 - originalIndex));
                        inViewWithColorCount++;
                    } else {
                        // otherwise just track stats of the items we discarded
                        // don't need to check childNode here, because we can't get here with no childNode
                        if (params.stats) {
                            if (childWasInView) {
                                params.stats->skippedWasInView(childNode);
                            } else {
                                params.stats->skippedNoChange(childNode);
                            }
                        }
                        
                    }
                }
            }
        }
    }
    *writeToThisLevelBuffer = childrenColoredBits;
    writeToThisLevelBuffer += sizeof(childrenColoredBits); // move the pointer
    bytesAtThisLevel += sizeof(childrenColoredBits); // keep track of byte count
    if (params.stats) {
        params.stats->colorBitsWritten();
    }

    // write the color data...
    if (params.includeColor) {
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            if (oneAtBit(childrenColoredBits, i)) {
                VoxelNode* childNode = node->getChildAtIndex(i);
                memcpy(writeToThisLevelBuffer, &childNode->getColor(), BYTES_PER_COLOR);
                writeToThisLevelBuffer += BYTES_PER_COLOR; // move the pointer for color
                bytesAtThisLevel += BYTES_PER_COLOR; // keep track of byte count for color

                // don't need to check childNode here, because we can't get here with no childNode
                if (params.stats) {
                    params.stats->colorSent(childNode);
                }

            }
        }
    }

    // if the caller wants to include childExistsBits, then include them even if not in view, put them before the
    // childrenExistInPacketBits, so that the lower code can properly repair the packet exists bits
    if (params.includeExistsBits) {
        *writeToThisLevelBuffer = childrenExistInTreeBits;
        writeToThisLevelBuffer += sizeof(childrenExistInTreeBits); // move the pointer
        bytesAtThisLevel += sizeof(childrenExistInTreeBits); // keep track of byte count
        if (params.stats) {
            params.stats->existsBitsWritten();
        }
    }

    // write the child exist bits
    *writeToThisLevelBuffer = childrenExistInPacketBits;
    writeToThisLevelBuffer += sizeof(childrenExistInPacketBits); // move the pointer
    bytesAtThisLevel += sizeof(childrenExistInPacketBits); // keep track of byte count
    if (params.stats) {
        params.stats->existsInPacketBitsWritten();
    }

    // We only need to keep digging, if there is at least one child that is inView, and not a leaf.
    keepDiggingDeeper = (inViewNotLeafCount > 0);

    // If we have enough room to copy our local results into the buffer, then do so...
    if (availableBytes >= bytesAtThisLevel) {
        memcpy(outputBuffer, &thisLevelBuffer[0], bytesAtThisLevel);

        outputBuffer   += bytesAtThisLevel;
        availableBytes -= bytesAtThisLevel;
    } else {
        bag.insert(node);

        // don't need to check node here, because we can't get here with no node
        if (params.stats) {
            params.stats->didntFit(node);
        }

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

        // we are also going to recurse these child trees in "distance" sorted order, but we need to pack them in the
        // final packet in standard order. So what we're going to do is keep track of how big each subtree was in bytes,
        // and then later reshuffle these sections of our output buffer back into normal order. This allows us to make
        // a single recursive pass in distance sorted order, but retain standard order in our encoded packet
        int recursiveSliceSizes[NUMBER_OF_CHILDREN];
        unsigned char* recursiveSliceStarts[NUMBER_OF_CHILDREN];
        unsigned char* firstRecursiveSlice = outputBuffer;
        int allSlicesSize = 0;

        // for each child node in Distance sorted order..., check to see if they exist, are colored, and in view, and if so
        // add them to our distance ordered array of children
        for (int indexByDistance = 0; indexByDistance < currentCount; indexByDistance++) {
            VoxelNode* childNode = sortedChildren[indexByDistance];
            int originalIndex = indexOfChildren[indexByDistance];

            if (oneAtBit(childrenExistInPacketBits, originalIndex)) {

                int thisLevel = currentEncodeLevel;
                // remember this for reshuffling
                recursiveSliceStarts[originalIndex] = outputBuffer;

                int childTreeBytesOut = encodeTreeBitstreamRecursion(childNode, outputBuffer, availableBytes, bag,
                                                                     params, thisLevel);

                // remember this for reshuffling
                recursiveSliceSizes[originalIndex] = childTreeBytesOut;
                allSlicesSize += childTreeBytesOut;

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
                if (params.includeColor && !params.includeExistsBits && childTreeBytesOut == 2) {
                    childTreeBytesOut = 0; // this is the degenerate case of a tree with no colors and no child trees
                }
                // We used to try to collapse trees that didn't contain any data, but this does appear to create a problem
                // in detecting node deletion. So, I've commented this out but left it in here as a warning to anyone else
                // about not attempting to add this optimization back in, without solving the node deletion case.
                // We need to send these bitMasks in case the exists in tree bitmask is indicating the deletion of a tree
                //if (params.includeColor && params.includeExistsBits && childTreeBytesOut == 3) {
                //    childTreeBytesOut = 0; // this is the degenerate case of a tree with no colors and no child trees
                //}

                bytesAtThisLevel += childTreeBytesOut;
                availableBytes -= childTreeBytesOut;
                outputBuffer += childTreeBytesOut;

                // If we had previously started writing, and if the child DIDN'T write any bytes,
                // then we want to remove their bit from the childExistsPlaceHolder bitmask
                if (childTreeBytesOut == 0) {
                    // remove this child's bit...
                    childrenExistInPacketBits -= (1 << (7 - originalIndex));
                    // repair the child exists mask
                    *childExistsPlaceHolder = childrenExistInPacketBits;

                    // If this is the last of the child exists bits, then we're actually be rolling out the entire tree
                    if (params.stats && childrenExistInPacketBits == 0) {
                        params.stats->childBitsRemoved(params.includeExistsBits, params.includeColor);
                    }
                    
                    // Note: no need to move the pointer, cause we already stored this
                } // end if (childTreeBytesOut == 0)
            } // end if (oneAtBit(childrenExistInPacketBits, originalIndex))
        } // end for

        // reshuffle here...
        if (params.wantOcclusionCulling) {
            unsigned char tempReshuffleBuffer[MAX_VOXEL_PACKET_SIZE];

            unsigned char* tempBufferTo = &tempReshuffleBuffer[0]; // this is our temporary destination

            // iterate through our childrenExistInPacketBits, these will be the sections of the packet that we copied subTree
            // details into. Unfortunately, they're in distance sorted order, not original index order. we need to put them
            // back into original distance order
            for (int originalIndex = 0; originalIndex < NUMBER_OF_CHILDREN; originalIndex++) {
                if (oneAtBit(childrenExistInPacketBits, originalIndex)) {
                    int thisSliceSize = recursiveSliceSizes[originalIndex];
                    unsigned char* thisSliceStarts = recursiveSliceStarts[originalIndex];

                    memcpy(tempBufferTo, thisSliceStarts, thisSliceSize);
                    tempBufferTo += thisSliceSize;
                }
            }

            // now that all slices are back in the correct order, copy them to the correct output buffer
            memcpy(firstRecursiveSlice, &tempReshuffleBuffer[0], allSlicesSize);
        }


    } // end keepDiggingDeeper

    return bytesAtThisLevel;
}

bool VoxelTree::readFromSVOFile(const char* fileName) {
    std::ifstream file(fileName, std::ios::in|std::ios::binary|std::ios::ate);
    if(file.is_open()) {
        qDebug("loading file %s...\n", fileName);

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

bool VoxelTree::readFromSquareARGB32Pixels(const uint32_t* pixels, int dimension) {
    SquarePixelMap pixelMap = SquarePixelMap(pixels, dimension);
    pixelMap.addVoxelsToVoxelTree(this);
    return true;
}

bool VoxelTree::readFromSchematicFile(const char *fileName) {
    std::stringstream ss;
    int err = retrieveData(fileName, ss);
    if (err && ss.get() != TAG_Compound) {
        qDebug("[ERROR] Invalid schematic file.\n");
        return false;
    }

    ss.get();
    TagCompound schematics(ss);
    if (!schematics.getBlocksId() || !schematics.getBlocksData()) {
        qDebug("[ERROR] Invalid schematic file.\n");
        return false;
    }

    int max = (schematics.getWidth() > schematics.getLength()) ? schematics.getWidth() : schematics.getLength();
    max = (max > schematics.getHeight()) ? max : schematics.getHeight();

    int scale = 1;
    while (max > scale) {scale *= 2;}
    float size = 1.0f / scale;

    int create = 1;
    int red = 128, green = 128, blue = 128;
    int count = 0;

    for (int y = 0; y < schematics.getHeight(); ++y) {
        for (int z = 0; z < schematics.getLength(); ++z) {
            for (int x = 0; x < schematics.getWidth(); ++x) {
                int pos  = ((y * schematics.getLength()) + z) * schematics.getWidth() + x;
                int id   = schematics.getBlocksId()[pos];
                int data = schematics.getBlocksData()[pos];

                create = 1;
                computeBlockColor(id, data, red, green, blue, create);

                switch (create) {
                    case 1:
                        createVoxel(size * x, size * y, size * z, size, red, green, blue, true);
                        ++count;
                        break;
                    case 2:
                        switch (data) {
                            case 0:
                                createVoxel(size * x + size / 2, size * y + size / 2, size * z           , size / 2, red, green, blue, true);
                                createVoxel(size * x + size / 2, size * y + size / 2, size * z + size / 2, size / 2, red, green, blue, true);
                                break;
                            case 1:
                                createVoxel(size * x           , size * y + size / 2, size * z           , size / 2, red, green, blue, true);
                                createVoxel(size * x           , size * y + size / 2, size * z + size / 2, size / 2, red, green, blue, true);
                                break;
                            case 2:
                                createVoxel(size * x           , size * y + size / 2, size * z + size / 2, size / 2, red, green, blue, true);
                                createVoxel(size * x + size / 2, size * y + size / 2, size * z + size / 2, size / 2, red, green, blue, true);
                                break;
                            case 3:
                                createVoxel(size * x           , size * y + size / 2, size * z           , size / 2, red, green, blue, true);
                                createVoxel(size * x + size / 2, size * y + size / 2, size * z           , size / 2, red, green, blue, true);
                                break;
                        }
                        count += 2;
                        // There's no break on purpose.
                    case 3:
                        createVoxel(size * x           , size * y, size * z           , size / 2, red, green, blue, true);
                        createVoxel(size * x + size / 2, size * y, size * z           , size / 2, red, green, blue, true);
                        createVoxel(size * x           , size * y, size * z + size / 2, size / 2, red, green, blue, true);
                        createVoxel(size * x + size / 2, size * y, size * z + size / 2, size / 2, red, green, blue, true);
                        count += 4;
                        break;
                }
            }
        }
    }

    qDebug("Created %d voxels from minecraft import.\n", count);

    return true;
}

void VoxelTree::writeToSVOFile(const char* fileName, VoxelNode* node) const {

    std::ofstream file(fileName, std::ios::out|std::ios::binary);

    if(file.is_open()) {
        qDebug("saving to file %s...\n", fileName);

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

            EncodeBitstreamParams params(INT_MAX, IGNORE_VIEW_FRUSTUM, WANT_COLOR, NO_EXISTS_BITS);
            bytesWritten = encodeTreeBitstream(subTree, &outputBuffer[0], MAX_VOXEL_PACKET_SIZE - 1, nodeBag, params);

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

void VoxelTree::copySubTreeIntoNewTree(VoxelNode* startNode, VoxelTree* destinationTree, bool rebaseToRoot) {
    VoxelNodeBag nodeBag;
    nodeBag.insert(startNode);
    int chopLevels = 0;
    if (rebaseToRoot) {
        chopLevels = numberOfThreeBitSectionsInCode(startNode->getOctalCode());
    }

    static unsigned char outputBuffer[MAX_VOXEL_PACKET_SIZE - 1]; // save on allocs by making this static
    int bytesWritten = 0;

    while (!nodeBag.isEmpty()) {
        VoxelNode* subTree = nodeBag.extract();

        // ask our tree to write a bitsteam
        EncodeBitstreamParams params(INT_MAX, IGNORE_VIEW_FRUSTUM, WANT_COLOR, NO_EXISTS_BITS, chopLevels);
        bytesWritten = encodeTreeBitstream(subTree, &outputBuffer[0], MAX_VOXEL_PACKET_SIZE - 1, nodeBag, params);

        // ask destination tree to read the bitstream
        destinationTree->readBitstreamToTree(&outputBuffer[0], bytesWritten, WANT_COLOR, NO_EXISTS_BITS);
    }
}

void VoxelTree::copyFromTreeIntoSubTree(VoxelTree* sourceTree, VoxelNode* destinationNode) {
    VoxelNodeBag nodeBag;
    // If we were given a specific node, start from there, otherwise start from root
    nodeBag.insert(sourceTree->rootNode);

    static unsigned char outputBuffer[MAX_VOXEL_PACKET_SIZE - 1]; // save on allocs by making this static
    int bytesWritten = 0;

    while (!nodeBag.isEmpty()) {
        VoxelNode* subTree = nodeBag.extract();

        // ask our tree to write a bitsteam
        EncodeBitstreamParams params(INT_MAX, IGNORE_VIEW_FRUSTUM, WANT_COLOR, NO_EXISTS_BITS);
        bytesWritten = sourceTree->encodeTreeBitstream(subTree, &outputBuffer[0], MAX_VOXEL_PACKET_SIZE - 1, nodeBag, params);

        // ask destination tree to read the bitstream
        readBitstreamToTree(&outputBuffer[0], bytesWritten, WANT_COLOR, NO_EXISTS_BITS, destinationNode);
    }
}

void VoxelTree::computeBlockColor(int id, int data, int& red, int& green, int& blue, int& create) {

    switch (id) {
        case   1:
        case  14:
        case  15:
        case  16:
        case  21:
        case  56:
        case  73:
        case  74:
        case  97:
        case 129: red = 128; green = 128; blue = 128; break;
        case   2: red =  77; green = 117; blue =  66; break;
        case   3:
        case  60: red = 116; green =  83; blue =  56; break;
        case   4: red =  71; green =  71; blue =  71; break;
        case   5:
        case 125: red = 133; green =  94; blue =  62; break;
        case   7: red =  35; green =  35; blue =  35; break;
        case   8:
        case   9: red = 100; green = 109; blue = 185; break;
        case  10:
        case  11: red = 192; green =  64; blue =   8; break;
        case  12: red = 209; green = 199; blue = 155; break;
        case  13: red =  96; green =  94; blue =  93; break;
        case  17: red =  71; green =  56; blue =  35; break;
        case  18: red =  76; green = 104; blue =  64; break;
        case  19: red = 119; green = 119; blue =  37; break;
        case  22: red =  22; green =  44; blue =  86; break;
        case  23:
        case  29:
        case  33:
        case  61:
        case  62:
        case 158: red =  61; green =  61; blue =  61; break;
        case  24: red = 209; green = 202; blue = 156; break;
        case  25:
        case  58:
        case  84:
        case 137: red =  57; green =  38; blue =  25; break;
        case  35:
            switch (data) {
                case  0: red = 234; green = 234; blue = 234; break;
                case  1: red = 224; green = 140; blue =  84; break;
                case  2: red = 185; green =  90; blue = 194; break;
                case  3: red = 124; green = 152; blue = 208; break;
                case  4: red = 165; green = 154; blue =  35; break;
                case  5: red =  70; green = 187; blue =  61; break;
                case  6: red = 206; green = 124; blue = 145; break;
                case  7: red =  66; green =  66; blue =  66; break;
                case  8: red = 170; green = 176; blue = 176; break;
                case  9: red =  45; green = 108; blue =  35; break;
                case 10: red = 130; green =  62; blue =   8; break;
                case 11: red =  43; green =  51; blue =  29; break;
                case 12: red =  73; green =  47; blue =  29; break;
                case 13: red =  57; green =  76; blue =  36; break;
                case 14: red = 165; green =  58; blue =  53; break;
                case 15: red =  24; green =  24; blue =  24; break;
                default:
                    create = 0;
                    break;
            }
            break;
        case  41: red = 239; green = 238; blue = 105; break;
        case  42: red = 146; green = 146; blue = 146; break;
        case  43:
        case  98: red = 161; green = 161; blue = 161; break;
        case  44:
            create = 3;

            switch (data) {
                case 0: red = 161; green = 161; blue = 161; break;
                case 1: red = 209; green = 202; blue = 156; break;
                case 2: red = 133; green =  94; blue =  62; break;
                case 3: red =  71; green =  71; blue =  71; break;
                case 4: red = 121; green =  67; blue =  53; break;
                case 5: red = 161; green = 161; blue = 161; break;
                case 6: red =  45; green =  22; blue =  26; break;
                case 7: red = 195; green = 192; blue = 185; break;
                default:
                    create = 0;
                    break;
            }
            break;
        case  45: red = 121; green =  67; blue =  53; break;
        case  46: red = 118; green =  36; blue =  13; break;
        case  47: red = 155; green = 127; blue =  76; break;
        case  48: red =  61; green =  79; blue =  61; break;
        case  49: red =  52; green =  41; blue =  74; break;
        case  52: red =  12; green =  66; blue =  71; break;
        case  53:
        case  67:
        case 108:
        case 109:
        case 114:
        case 128:
        case 134:
        case 135:
        case 136:
        case 156:
            create = 2;

            switch (id) {
                case  53:
                case 134:
                case 135:
                case 136: red = 133; green =  94; blue =  62; break;
                case  67: red =  71; green =  71; blue =  71; break;
                case 108: red = 121; green =  67; blue =  53; break;
                case 109: red = 161; green = 161; blue = 161; break;
                case 114: red =  45; green =  22; blue =  26; break;
                case 128: red = 209; green = 202; blue = 156; break;
                case 156: red = 195; green = 192; blue = 185; break;
                default:
                    create = 0;
                    break;
            }
            break;
        case  54:
        case  95:
        case 146: red = 155; green = 105; blue =  32; break;
        case  57: red = 145; green = 219; blue = 215; break;
        case  79: red = 142; green = 162; blue = 195; break;
        case  80: red = 255; green = 255; blue = 255; break;
        case  81: red =   8; green =  64; blue =  15; break;
        case  82: red = 150; green = 155; blue = 166; break;
        case  86:
        case  91: red = 179; green = 108; blue =  17; break;
        case  87:
        case 153: red =  91; green =  31; blue =  30; break;
        case  88: red =  68; green =  49; blue =  38; break;
        case  89: red = 180; green = 134; blue =  65; break;
        case 103: red = 141; green = 143; blue =  36; break;
        case 110: red = 103; green =  92; blue =  95; break;
        case 112: red =  45; green =  22; blue =  26; break;
        case 121: red = 183; green = 178; blue = 129; break;
        case 123: red = 101; green =  59; blue =  31; break;
        case 124: red = 213; green = 178; blue = 123; break;
        case 130: red =  38; green =  54; blue =  56; break;
        case 133: red =  53; green =  84; blue =  85; break;
        case 152: red = 131; green =  22; blue =   7; break;
        case 155: red = 195; green = 192; blue = 185; break;
        case 159: red = 195; green = 165; blue = 150; break;
        case 170: red = 168; green = 139; blue =  15; break;
        case 172: red = 140; green =  86; blue =  61; break;
        case 173: red =   9; green =   9; blue =   9; break;
        default:
            create = 0;
            break;
    }
}
