//
//  Octree.cpp
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

#include <QtCore/QDebug>
#include <QImage>
#include <QRgb>

#include "CoverageMap.h"
#include <GeometryUtil.h>
#include "OctalCode.h"
#include <PacketHeaders.h>
#include <SharedUtil.h>

//#include "Tags.h"

#include "ViewFrustum.h"
#include "OctreeConstants.h"
#include "OctreeElementBag.h"
#include "Octree.h"

float boundaryDistanceForRenderLevel(unsigned int renderLevel, float voxelSizeScale) {
    return voxelSizeScale / powf(2, renderLevel);
}

Octree::Octree(bool shouldReaverage) :
    _isDirty(true),
    _shouldReaverage(shouldReaverage),
    _stopImport(false) {
    _rootNode = NULL;
    
    pthread_mutex_init(&_encodeSetLock, NULL);
    pthread_mutex_init(&_deleteSetLock, NULL);
    pthread_mutex_init(&_deletePendingSetLock, NULL);
}

Octree::~Octree() {
    // delete the children of the root node
    // this recursively deletes the tree
    delete _rootNode;

    pthread_mutex_destroy(&_encodeSetLock);
    pthread_mutex_destroy(&_deleteSetLock);
    pthread_mutex_destroy(&_deletePendingSetLock);
}

// Recurses voxel tree calling the RecurseOctreeOperation function for each node.
// stops recursion if operation function returns false.
void Octree::recurseTreeWithOperation(RecurseOctreeOperation operation, void* extraData) {
    recurseNodeWithOperation(_rootNode, operation, extraData);
}

// Recurses voxel node with an operation function
void Octree::recurseNodeWithOperation(OctreeElement* node, RecurseOctreeOperation operation, void* extraData, 
                        int recursionCount) {
    if (recursionCount > DANGEROUSLY_DEEP_RECURSION) {
        qDebug() << "Octree::recurseNodeWithOperation() reached DANGEROUSLY_DEEP_RECURSION, bailing!\n";
        return;
    }
        
    if (operation(node, extraData)) {
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            OctreeElement* child = node->getChildAtIndex(i);
            if (child) {
                recurseNodeWithOperation(child, operation, extraData, recursionCount+1);
            }
        }
    }
}

// Recurses voxel tree calling the RecurseOctreeOperation function for each node.
// stops recursion if operation function returns false.
void Octree::recurseTreeWithOperationDistanceSorted(RecurseOctreeOperation operation,
                                                       const glm::vec3& point, void* extraData) {

    recurseNodeWithOperationDistanceSorted(_rootNode, operation, point, extraData);
}

// Recurses voxel node with an operation function
void Octree::recurseNodeWithOperationDistanceSorted(OctreeElement* node, RecurseOctreeOperation operation, 
                                                       const glm::vec3& point, void* extraData, int recursionCount) {

    if (recursionCount > DANGEROUSLY_DEEP_RECURSION) {
        qDebug() << "Octree::recurseNodeWithOperationDistanceSorted() reached DANGEROUSLY_DEEP_RECURSION, bailing!\n";
        return;
    }

    if (operation(node, extraData)) {
        // determine the distance sorted order of our children
        OctreeElement* sortedChildren[NUMBER_OF_CHILDREN] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
        float distancesToChildren[NUMBER_OF_CHILDREN] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        int indexOfChildren[NUMBER_OF_CHILDREN] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        int currentCount = 0;

        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            OctreeElement* childNode = node->getChildAtIndex(i);
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
            OctreeElement* childNode = sortedChildren[i];
            if (childNode) {
                //qDebug("recurseNodeWithOperationDistanceSorted() PROCESSING child[%d] distance=%f...\n", i, distancesToChildren[i]);
                //childNode->printDebugDetails("");
                recurseNodeWithOperationDistanceSorted(childNode, operation, point, extraData);
            }
        }
    }
}


OctreeElement* Octree::nodeForOctalCode(OctreeElement* ancestorNode,
                                       const unsigned char* needleCode, OctreeElement** parentOfFoundNode) const {
    // special case for NULL octcode
    if (needleCode == NULL) {
        return _rootNode;
    }
    
    // find the appropriate branch index based on this ancestorNode
    if (*needleCode > 0) {
        int branchForNeedle = branchIndexWithDescendant(ancestorNode->getOctalCode(), needleCode);
        OctreeElement* childNode = ancestorNode->getChildAtIndex(branchForNeedle);

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
                return nodeForOctalCode(childNode, needleCode, parentOfFoundNode);
            }
        }
    }

    // we've been given a code we don't have a node for
    // return this node as the last created parent
    return ancestorNode;
}

// returns the node created!
OctreeElement* Octree::createMissingNode(OctreeElement* lastParentNode, const unsigned char* codeToReach) {
    int indexOfNewChild = branchIndexWithDescendant(lastParentNode->getOctalCode(), codeToReach);
    // If this parent node is a leaf, then you know the child path doesn't exist, so deal with
    // breaking up the leaf first, which will also create a child path
    if (lastParentNode->requiresSplit()) {
        lastParentNode->splitChildren();
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

int Octree::readNodeData(OctreeElement* destinationNode, const unsigned char* nodeData, int bytesLeftToRead,
                            ReadBitstreamToTreeParams& args) {
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
                }
            }

            OctreeElement* childNodeAt = destinationNode->getChildAtIndex(i);
            bool nodeWasDirty = false;
            bool nodeIsDirty = false;
            if (childNodeAt) {
                nodeWasDirty = childNodeAt->isDirty();
                bytesRead += childNodeAt->readElementDataFromBuffer(nodeData + bytesRead, bytesLeftToRead, args);
                childNodeAt->setSourceUUID(args.sourceUUID);
                
                // if we had a local version of the node already, it's possible that we have it already but
                // with the same color data, so this won't count as a change. To address this we check the following
                if (!childNodeAt->isDirty() && childNodeAt->getShouldRender() && !childNodeAt->isRendered()) {
                    childNodeAt->setDirtyBit(); // force dirty!
                }
                
                nodeIsDirty = childNodeAt->isDirty();
            }
            if (nodeIsDirty) {
                _isDirty = true;
            }
        }
    }

    // give this destination node the child mask from the packet
    unsigned char childrenInTreeMask = args.includeExistsBits ? *(nodeData + bytesRead) : ALL_CHILDREN_ASSUMED_TO_EXIST;
    unsigned char childMask = *(nodeData + bytesRead + (args.includeExistsBits ? sizeof(childrenInTreeMask) : 0));

    int childIndex = 0;
    bytesRead += args.includeExistsBits ? sizeof(childrenInTreeMask) + sizeof(childMask) : sizeof(childMask);

    while (bytesLeftToRead - bytesRead > 0 && childIndex < NUMBER_OF_CHILDREN) {
        // check the exists mask to see if we have a child to traverse into

        if (oneAtBit(childMask, childIndex)) {
            if (!destinationNode->getChildAtIndex(childIndex)) {
                // add a child at that index, if it doesn't exist
                destinationNode->addChildAtIndex(childIndex);
                bool nodeIsDirty = destinationNode->isDirty();
                if (nodeIsDirty) {
                    _isDirty = true;
                }
            }

            // tell the child to read the subsequent data
            bytesRead += readNodeData(destinationNode->getChildAtIndex(childIndex),
                                      nodeData + bytesRead, bytesLeftToRead - bytesRead, args);
        }
        childIndex++;
    }

    if (args.includeExistsBits) {
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

void Octree::readBitstreamToTree(const unsigned char * bitstream, unsigned long int bufferSizeBytes,
                                    ReadBitstreamToTreeParams& args) {
    int bytesRead = 0;
    const unsigned char* bitstreamAt = bitstream;

    // If destination node is not included, set it to root
    if (!args.destinationNode) {
        args.destinationNode = _rootNode;
    }

    // Keep looping through the buffer calling readNodeData() this allows us to pack multiple root-relative Octal codes
    // into a single network packet. readNodeData() basically goes down a tree from the root, and fills things in from there
    // if there are more bytes after that, it's assumed to be another root relative tree

    while (bitstreamAt < bitstream + bufferSizeBytes) {
        OctreeElement* bitstreamRootNode = nodeForOctalCode(args.destinationNode, (unsigned char *)bitstreamAt, NULL);
        if (*bitstreamAt != *bitstreamRootNode->getOctalCode()) {
            // if the octal code returned is not on the same level as
            // the code being searched for, we have OctreeElements to create

            // Note: we need to create this node relative to root, because we're assuming that the bitstream for the initial
            // octal code is always relative to root!
            bitstreamRootNode = createMissingNode(args.destinationNode, (unsigned char*) bitstreamAt);
            if (bitstreamRootNode->isDirty()) {
                _isDirty = true;
            }
        }

        int octalCodeBytes = bytesRequiredForCodeLength(*bitstreamAt);
        int theseBytesRead = 0;
        theseBytesRead += octalCodeBytes;
        
        theseBytesRead += readNodeData(bitstreamRootNode, bitstreamAt + octalCodeBytes, 
                                       bufferSizeBytes - (bytesRead + octalCodeBytes), args);

        // skip bitstream to new startPoint
        bitstreamAt += theseBytesRead;
        bytesRead +=  theseBytesRead;

        if (args.wantImportProgress) {
            emit importProgress((100 * (bitstreamAt - bitstream)) / bufferSizeBytes);
        }
    }
}

void Octree::deleteOctreeElementAt(float x, float y, float z, float s) {
    unsigned char* octalCode = pointToOctalCode(x,y,z,s);
    deleteOctalCodeFromTree(octalCode);
    delete[] octalCode; // cleanup memory
}

class DeleteOctalCodeFromTreeArgs {
public:
    bool collapseEmptyTrees;
    const unsigned char* codeBuffer;
    int lengthOfCode;
    bool deleteLastChild;
    bool pathChanged;
};

// Note: uses the codeColorBuffer format, but the color's are ignored, because
// this only finds and deletes the node from the tree.
void Octree::deleteOctalCodeFromTree(const unsigned char* codeBuffer, bool collapseEmptyTrees) {
    // recurse the tree while decoding the codeBuffer, once you find the node in question, recurse
    // back and implement color reaveraging, and marking of lastChanged
    DeleteOctalCodeFromTreeArgs args;
    args.collapseEmptyTrees = collapseEmptyTrees;
    args.codeBuffer         = codeBuffer;
    args.lengthOfCode       = numberOfThreeBitSectionsInCode(codeBuffer);
    args.deleteLastChild    = false;
    args.pathChanged        = false;

    OctreeElement* node = _rootNode;
    
    // We can't encode and delete nodes at the same time, so we guard against deleting any node that is actively
    // being encoded. And we stick that code on our pendingDelete list.
    if (isEncoding(codeBuffer)) {
        queueForLaterDelete(codeBuffer);
    } else {
        startDeleting(codeBuffer);
        deleteOctalCodeFromTreeRecursion(node, &args);
        doneDeleting(codeBuffer);
    }
}

void Octree::deleteOctalCodeFromTreeRecursion(OctreeElement* node, void* extraData) {
    DeleteOctalCodeFromTreeArgs* args = (DeleteOctalCodeFromTreeArgs*)extraData;

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
    OctreeElement* childNode = node->getChildAtIndex(childIndex);

    // If there is no child at the target location, and the current parent node is a colored leaf,
    // then it means we were asked to delete a child out of a larger leaf voxel.
    // We support this by breaking up the parent voxel into smaller pieces.
    if (!childNode && node->requiresSplit()) {
        // we need to break up ancestors until we get to the right level
        OctreeElement* ancestorNode = node;
        while (true) {
            int index = branchIndexWithDescendant(ancestorNode->getOctalCode(), args->codeBuffer);
            
            // we end up with all the children, even the one we want to delete
            ancestorNode->splitChildren();
            
            int lengthOfAncestorNode = numberOfThreeBitSectionsInCode(ancestorNode->getOctalCode());

            // If we've reached the parent of the target, then stop breaking up children
            if (lengthOfAncestorNode == (args->lengthOfCode - 1)) {
                
                // since we created all the children when we split, we need to delete this target one
                ancestorNode->deleteChildAtIndex(index);
                break;
            }
            ancestorNode = ancestorNode->getChildAtIndex(index);
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
        return;
    }

    // If we got this far then we have a child for the branch we're looking for, but we're not there yet
    // recurse till we get there
    deleteOctalCodeFromTreeRecursion(childNode, args);

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
            if (node == _rootNode) {
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

void Octree::eraseAllOctreeElements() {
    delete _rootNode; // this will recurse and delete all children
    _rootNode = createNewElement();
    _isDirty = true;
}

void Octree::processRemoveOctreeElementsBitstream(const unsigned char* bitstream, int bufferSizeBytes) {
    //unsigned short int itemNumber = (*((unsigned short int*)&bitstream[sizeof(PACKET_HEADER)]));

    int numBytesPacketHeader = numBytesForPacketHeader(bitstream);
    unsigned short int sequence = (*((unsigned short int*)(bitstream + numBytesPacketHeader)));
    uint64_t sentAt = (*((uint64_t*)(bitstream + numBytesPacketHeader + sizeof(sequence))));
    
    int atByte = numBytesPacketHeader + sizeof(sequence) + sizeof(sentAt);
    
    unsigned char* voxelCode = (unsigned char*)&bitstream[atByte];
    while (atByte < bufferSizeBytes) {
        int maxSize = bufferSizeBytes - atByte;
        int codeLength = numberOfThreeBitSectionsInCode(voxelCode, maxSize);
        
        if (codeLength == OVERFLOWED_OCTCODE_BUFFER) {
            printf("WARNING! Got remove voxel bitstream that would overflow buffer in numberOfThreeBitSectionsInCode(), ");
            printf("bailing processing of packet!\n");
            break;
        }
        int voxelDataSize = bytesRequiredForCodeLength(codeLength) + SIZE_OF_COLOR_DATA;
        
        if (atByte + voxelDataSize <= bufferSizeBytes) {
            deleteOctalCodeFromTree(voxelCode, COLLAPSE_EMPTY_TREE);
            voxelCode += voxelDataSize;
            atByte += voxelDataSize;
        } else {
            printf("WARNING! Got remove voxel bitstream that would overflow buffer, bailing processing!\n");
            break;
        }
    }
}

// Note: this is an expensive call. Don't call it unless you really need to reaverage the entire tree (from startNode)
void Octree::reaverageOctreeElements(OctreeElement* startNode) {
    if (startNode == NULL) {
        startNode = getRoot();
    }
    // if our tree is a reaveraging tree, then we do this, otherwise we don't do anything
    if (_shouldReaverage) {
        static int recursionCount;
        if (startNode == _rootNode) {
            recursionCount = 0;
        } else {
            recursionCount++;
        }
        if (recursionCount > UNREASONABLY_DEEP_RECURSION) {
            qDebug("Octree::reaverageOctreeElements()... bailing out of UNREASONABLY_DEEP_RECURSION\n");
            recursionCount--;
            return;
        }

        bool hasChildren = false;

        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            if (startNode->getChildAtIndex(i)) {
                reaverageOctreeElements(startNode->getChildAtIndex(i));
                hasChildren = true;
            }
        }

        // collapseIdenticalLeaves() returns true if it collapses the leaves
        // in which case we don't need to set the average color
        if (hasChildren && !startNode->collapseChildren()) {
            startNode->calculateAverageFromChildren();
        }
        recursionCount--;
    }
}

OctreeElement* Octree::getOctreeElementAt(float x, float y, float z, float s) const {
    unsigned char* octalCode = pointToOctalCode(x,y,z,s);
    OctreeElement* node = nodeForOctalCode(_rootNode, octalCode, NULL);
    if (*node->getOctalCode() != *octalCode) {
        node = NULL;
    }
    delete[] octalCode; // cleanup memory
#ifdef HAS_AUDIT_CHILDREN
    if (node) {
        node->auditChildren("Octree::getOctreeElementAt()");
    }
#endif // def HAS_AUDIT_CHILDREN
    return node;
}


OctreeElement* Octree::getOrCreateChildElementAt(float x, float y, float z, float s) {
    return getRoot()->getOrCreateChildElementAt(x, y, z, s);
}


// combines the ray cast arguments into a single object
class RayArgs {
public:
    glm::vec3 origin;
    glm::vec3 direction;
    OctreeElement*& node;
    float& distance;
    BoxFace& face;
    bool found;
};

bool findRayIntersectionOp(OctreeElement* node, void* extraData) {
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
    if (node->hasContent() && (!args->found || distance < args->distance)) {
        args->node = node;
        args->distance = distance;
        args->face = face;
        args->found = true;
    }
    return false;
}

bool Octree::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                    OctreeElement*& node, float& distance, BoxFace& face) {
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
    OctreeElement* penetratedElement;
};

bool findSpherePenetrationOp(OctreeElement* element, void* extraData) {
    SphereArgs* args = static_cast<SphereArgs*>(extraData);

    // coarse check against bounds
    const AABox& box = element->getAABox();
    if (!box.expandedContains(args->center, args->radius)) {
        return false;
    }
    if (!element->isLeaf()) {
        return true; // recurse on children
    }
    if (element->hasContent()) {
        glm::vec3 elementPenetration;
        if (element->findSpherePenetration(args->center, args->radius, elementPenetration)) {
            args->penetration = addPenetrations(args->penetration, elementPenetration * (float)TREE_SCALE);
            args->found = true;
            args->penetratedElement = element;
        }
    }
    return false;
}

bool Octree::findSpherePenetration(const glm::vec3& center, float radius, glm::vec3& penetration, 
                    OctreeElement** penetratedElement) {
                    
    SphereArgs args = { center / (float)TREE_SCALE, radius / TREE_SCALE, penetration, false, NULL };
    penetration = glm::vec3(0.0f, 0.0f, 0.0f);
    recurseTreeWithOperation(findSpherePenetrationOp, &args);
    if (penetratedElement) {
        *penetratedElement = args.penetratedElement;
    }
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

bool findCapsulePenetrationOp(OctreeElement* node, void* extraData) {
    CapsuleArgs* args = static_cast<CapsuleArgs*>(extraData);

    // coarse check against bounds
    const AABox& box = node->getAABox();
    if (!box.expandedIntersectsSegment(args->start, args->end, args->radius)) {
        return false;
    }
    if (!node->isLeaf()) {
        return true; // recurse on children
    }
    if (node->hasContent()) {
        glm::vec3 nodePenetration;
        if (box.findCapsulePenetration(args->start, args->end, args->radius, nodePenetration)) {
            args->penetration = addPenetrations(args->penetration, nodePenetration * (float)TREE_SCALE);
            args->found = true;
        }
    }
    return false;
}

bool Octree::findCapsulePenetration(const glm::vec3& start, const glm::vec3& end, float radius, glm::vec3& penetration) {
    CapsuleArgs args = { start / (float)TREE_SCALE, end / (float)TREE_SCALE, radius / TREE_SCALE, penetration };
    penetration = glm::vec3(0.0f, 0.0f, 0.0f);
    recurseTreeWithOperation(findCapsulePenetrationOp, &args);
    return args.found;
}

int Octree::encodeTreeBitstream(OctreeElement* node, 
                        OctreePacketData* packetData, OctreeElementBag& bag,
                        EncodeBitstreamParams& params) {

    // How many bytes have we written so far at this level;
    int bytesWritten = 0;

    // you can't call this without a valid node
    if (!node) {
        qDebug("WARNING! encodeTreeBitstream() called with node=NULL\n");
        params.stopReason = EncodeBitstreamParams::NULL_NODE;
        return bytesWritten;
    }

    startEncoding(node);
    
    // If we're at a node that is out of view, then we can return, because no nodes below us will be in view!
    if (params.viewFrustum && !node->isInView(*params.viewFrustum)) {
        doneEncoding(node);
        params.stopReason = EncodeBitstreamParams::OUT_OF_VIEW;
        return bytesWritten;
    }

    // write the octal code
    bool roomForOctalCode = false; // assume the worst
    int codeLength;
    if (params.chopLevels) {
        unsigned char* newCode = chopOctalCode(node->getOctalCode(), params.chopLevels);
        roomForOctalCode = packetData->startSubTree(newCode);

        if (newCode) {
            delete newCode;            
        } else {
            codeLength = 1;
        }
    } else {
        roomForOctalCode = packetData->startSubTree(node->getOctalCode());
        codeLength = bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(node->getOctalCode()));
    }

    // If the octalcode couldn't fit, then we can return, because no nodes below us will fit...
    if (!roomForOctalCode) {
        doneEncoding(node);
        bag.insert(node); // add the node back to the bag so it will eventually get included
        params.stopReason = EncodeBitstreamParams::DIDNT_FIT;
        return bytesWritten;
    }
    
    bytesWritten += codeLength; // keep track of byte count
    
    int currentEncodeLevel = 0;
    
    // record some stats, this is the one node that we won't record below in the recursion function, so we need to 
    // track it here
    if (params.stats) {
        params.stats->traversed(node);
    }
    
    int childBytesWritten = encodeTreeBitstreamRecursion(node, packetData, bag, params, currentEncodeLevel);

    // if childBytesWritten == 1 then something went wrong... that's not possible
    assert(childBytesWritten != 1);

    // if includeColor and childBytesWritten == 2, then it can only mean that the lower level trees don't exist or for some reason
    // couldn't be written... so reset them here... This isn't true for the non-color included case
    if (params.includeColor && childBytesWritten == 2) {
        childBytesWritten = 0;
        //params.stopReason = EncodeBitstreamParams::UNKNOWN; // possibly should be DIDNT_FIT...
    }

    // if we wrote child bytes, then return our result of all bytes written
    if (childBytesWritten) {
        bytesWritten += childBytesWritten;
    } else {
        // otherwise... if we didn't write any child bytes, then pretend like we also didn't write our octal code
        bytesWritten = 0;
        //params.stopReason = EncodeBitstreamParams::DIDNT_FIT;
    }
    
    if (bytesWritten == 0) {
        packetData->discardSubTree();
    } else {
        packetData->endSubTree();
    }
    
    doneEncoding(node);
    
    return bytesWritten;
}

int Octree::encodeTreeBitstreamRecursion(OctreeElement* node, 
                                            OctreePacketData* packetData, OctreeElementBag& bag,
                                            EncodeBitstreamParams& params, int& currentEncodeLevel) const {
    // How many bytes have we written so far at this level;
    int bytesAtThisLevel = 0;

    // you can't call this without a valid node
    if (!node) {
        qDebug("WARNING! encodeTreeBitstreamRecursion() called with node=NULL\n");
        params.stopReason = EncodeBitstreamParams::NULL_NODE;
        return bytesAtThisLevel;
    }

    // Keep track of how deep we've encoded.
    currentEncodeLevel++;
    
    params.maxLevelReached = std::max(currentEncodeLevel,params.maxLevelReached);

    // If we've reached our max Search Level, then stop searching.
    if (currentEncodeLevel >= params.maxEncodeLevel) {
        params.stopReason = EncodeBitstreamParams::TOO_DEEP;
        return bytesAtThisLevel;
    }

    // If we've been provided a jurisdiction map, then we need to honor it.
    if (params.jurisdictionMap) {
        // here's how it works... if we're currently above our root jurisdiction, then we proceed normally.
        // but once we're in our own jurisdiction, then we need to make sure we're not below it.
        if (JurisdictionMap::BELOW == params.jurisdictionMap->isMyJurisdiction(node->getOctalCode(), CHECK_NODE_ONLY)) {
            params.stopReason = EncodeBitstreamParams::OUT_OF_JURISDICTION;
            return bytesAtThisLevel;
        }
    }
    
    // caller can pass NULL as viewFrustum if they want everything
    if (params.viewFrustum) {
        float distance = node->distanceToCamera(*params.viewFrustum);
        float boundaryDistance = boundaryDistanceForRenderLevel(node->getLevel() + params.boundaryLevelAdjust, 
                                        params.octreeElementSizeScale);

        // If we're too far away for our render level, then just return
        if (distance >= boundaryDistance) {
            if (params.stats) {
                params.stats->skippedDistance(node);
            }
            params.stopReason = EncodeBitstreamParams::LOD_SKIP;
            return bytesAtThisLevel;
        }

        // If we're at a node that is out of view, then we can return, because no nodes below us will be in view!
        // although technically, we really shouldn't ever be here, because our callers shouldn't be calling us if
        // we're out of view
        if (!node->isInView(*params.viewFrustum)) {
            if (params.stats) {
                params.stats->skippedOutOfView(node);
            }
            params.stopReason = EncodeBitstreamParams::OUT_OF_VIEW;
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
            
            // If we were in view, double check that we didn't switch LOD visibility... namely, the was in view doesn't
            // tell us if it was so small we wouldn't have rendered it. Which may be the case. And we may have moved closer
            // to it, and so therefore it may now be visible from an LOD perspective, in which case we don't consider it
            // as "was in view"...
            if (wasInView) {
                float distance = node->distanceToCamera(*params.lastViewFrustum);
                float boundaryDistance = boundaryDistanceForRenderLevel(node->getLevel() + params.boundaryLevelAdjust, 
                                                                            params.octreeElementSizeScale);
                if (distance >= boundaryDistance) {
                    // This would have been invisible... but now should be visible (we wouldn't be here otherwise)...
                    wasInView = false;
                }
            }
        }

        // If we were previously in the view, then we normally will return out of here and stop recursing. But
        // if we're in deltaViewFrustum mode, and this node has changed since it was last sent, then we do
        // need to send it.
        if (wasInView && !(params.deltaViewFrustum && node->hasChangedSince(params.lastViewFrustumSent - CHANGE_FUDGE))) {
            if (params.stats) {
                params.stats->skippedWasInView(node);
            }
            params.stopReason = EncodeBitstreamParams::WAS_IN_VIEW;
            return bytesAtThisLevel;
        }

        // If we're not in delta sending mode, and we weren't asked to do a force send, and the voxel hasn't changed, 
        // then we can also bail early and save bits
        if (!params.forceSendScene && !params.deltaViewFrustum && 
            !node->hasChangedSince(params.lastViewFrustumSent - CHANGE_FUDGE)) {
            if (params.stats) {
                params.stats->skippedNoChange(node);
            }
            params.stopReason = EncodeBitstreamParams::NO_CHANGE;
            return bytesAtThisLevel;
        }

        // If the user also asked for occlusion culling, check if this node is occluded, but only if it's not a leaf.
        // leaf occlusion is handled down below when we check child nodes
        if (params.wantOcclusionCulling && !node->isLeaf()) {
            AABox voxelBox = node->getAABox();
            voxelBox.scale(TREE_SCALE);
            OctreeProjectedPolygon* voxelPolygon = new OctreeProjectedPolygon(params.viewFrustum->getProjectedPolygon(voxelBox));

            // In order to check occlusion culling, the shadow has to be "all in view" otherwise, we will ignore occlusion
            // culling and proceed as normal
            if (voxelPolygon->getAllInView()) {
                CoverageMapStorageResult result = params.map->checkMap(voxelPolygon, false);
                delete voxelPolygon; // cleanup
                if (result == OCCLUDED) {
                    if (params.stats) {
                        params.stats->skippedOccluded(node);
                    }
                    params.stopReason = EncodeBitstreamParams::OCCLUDED;
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

    // Make our local buffer large enough to handle writing at this level in case we need to.
    LevelDetails thisLevelKey = packetData->startLevel();

    int inViewCount = 0;
    int inViewNotLeafCount = 0;
    int inViewWithColorCount = 0;

    OctreeElement* sortedChildren[NUMBER_OF_CHILDREN] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    float distancesToChildren[NUMBER_OF_CHILDREN] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    int indexOfChildren[NUMBER_OF_CHILDREN] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    int currentCount = 0;

    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        OctreeElement* childNode = node->getChildAtIndex(i);

        // if the caller wants to include childExistsBits, then include them even if not in view, if however,
        // we're in a portion of the tree that's not our responsibility, then we assume the child nodes exist
        // even if they don't in our local tree
        bool notMyJurisdiction = false;
        if (params.jurisdictionMap) {
            notMyJurisdiction = (JurisdictionMap::WITHIN != params.jurisdictionMap->isMyJurisdiction(node->getOctalCode(), i));
        }
        if (params.includeExistsBits) {
            // If the child is known to exist, OR, it's not my jurisdiction, then we mark the bit as existing
            if (childNode || notMyJurisdiction) {
                childrenExistInTreeBits += (1 << (7 - i));
            }
        }

        if (params.wantOcclusionCulling) {
            if (childNode) {
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
        OctreeElement* childNode = sortedChildren[i];
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
                                     boundaryDistanceForRenderLevel(childNode->getLevel() + params.boundaryLevelAdjust, 
                                            params.octreeElementSizeScale);

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
                    OctreeProjectedPolygon* voxelPolygon = new OctreeProjectedPolygon(
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
                                    : childNode->calculateShouldRender(params.viewFrustum, 
                                                    params.octreeElementSizeScale, params.boundaryLevelAdjust);
                     
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
    
    bool continueThisLevel = true;    
    continueThisLevel = packetData->appendBitMask(childrenColoredBits);
    
    if (continueThisLevel) {
        bytesAtThisLevel += sizeof(childrenColoredBits); // keep track of byte count
        if (params.stats) {
            params.stats->colorBitsWritten();
        }
    }

    // write the color data...
    if (continueThisLevel && params.includeColor) {
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            if (oneAtBit(childrenColoredBits, i)) {
                OctreeElement* childNode = node->getChildAtIndex(i);
                if (childNode) {
                    int bytesBeforeChild = packetData->getUncompressedSize();
                    continueThisLevel = childNode->appendElementData(packetData);
                    int bytesAfterChild = packetData->getUncompressedSize();
                    
                    if (!continueThisLevel) {
                        break; // no point in continuing
                    }
                    
                    bytesAtThisLevel += (bytesAfterChild - bytesBeforeChild); // keep track of byte count for this child

                    // don't need to check childNode here, because we can't get here with no childNode
                    if (params.stats) {
                        params.stats->colorSent(childNode);
                    }
                }
            }
        }
    }

    // if the caller wants to include childExistsBits, then include them even if not in view, put them before the
    // childrenExistInPacketBits, so that the lower code can properly repair the packet exists bits
    if (continueThisLevel && params.includeExistsBits) {
        continueThisLevel = packetData->appendBitMask(childrenExistInTreeBits);
        if (continueThisLevel) {
            bytesAtThisLevel += sizeof(childrenExistInTreeBits); // keep track of byte count
            if (params.stats) {
                params.stats->existsBitsWritten();
            }
        }
    }

    // write the child exist bits
    if (continueThisLevel) {
        continueThisLevel = packetData->appendBitMask(childrenExistInPacketBits);
        if (continueThisLevel) {
            bytesAtThisLevel += sizeof(childrenExistInPacketBits); // keep track of byte count
            if (params.stats) {
                params.stats->existsInPacketBitsWritten();
            }
        }
    }
    
    // We only need to keep digging, if there is at least one child that is inView, and not a leaf.
    keepDiggingDeeper = (inViewNotLeafCount > 0);

    if (continueThisLevel && keepDiggingDeeper) {
        // at this point, we need to iterate the children who are in view, even if not colored
        // and we need to determine if there's a deeper tree below them that we care about.
        //
        // Since this recursive function assumes we're already writing, we know we've already written our
        // childrenExistInPacketBits. But... we don't really know how big the child tree will be. And we don't know if
        // we'll have room in our buffer to actually write all these child trees. What we kinda would like to do is
        // write our childExistsBits as a place holder. Then let each potential tree have a go at it. If they
        // write something, we keep them in the bits, if they don't, we take them out.
        //
        // we know the last thing we wrote to the packet was our childrenExistInPacketBits. Let's remember where that was!
        int childExistsPlaceHolder = packetData->getUncompressedByteOffset(sizeof(childrenExistInPacketBits));

        // we are also going to recurse these child trees in "distance" sorted order, but we need to pack them in the
        // final packet in standard order. So what we're going to do is keep track of how big each subtree was in bytes,
        // and then later reshuffle these sections of our output buffer back into normal order. This allows us to make
        // a single recursive pass in distance sorted order, but retain standard order in our encoded packet
        int recursiveSliceSizes[NUMBER_OF_CHILDREN];
        const unsigned char* recursiveSliceStarts[NUMBER_OF_CHILDREN];
        int firstRecursiveSliceOffset = packetData->getUncompressedByteOffset();
        int allSlicesSize = 0;

        // for each child node in Distance sorted order..., check to see if they exist, are colored, and in view, and if so
        // add them to our distance ordered array of children
        for (int indexByDistance = 0; indexByDistance < currentCount; indexByDistance++) {
            OctreeElement* childNode = sortedChildren[indexByDistance];
            int originalIndex = indexOfChildren[indexByDistance];

            if (oneAtBit(childrenExistInPacketBits, originalIndex)) {

                int thisLevel = currentEncodeLevel;
                // remember this for reshuffling
                recursiveSliceStarts[originalIndex] = packetData->getUncompressedData() + packetData->getUncompressedSize();

                int childTreeBytesOut = 0;

                // XXXBHG - Note, this seems like the correct logic here, if we included the color in this packet, then
                // the LOD logic determined that the child nodes would not be visible... and if so, we shouldn't recurse
                // them further. But... for some time now the code has included and recursed into these child nodes, which
                // would likely still send the child content, even though the client wouldn't render it. This change is 
                // a major savings (~30%) and it seems to work correctly. But I want us to discuss as a group when we do
                // a voxel protocol review.
                //
                // This only applies in the view frustum case, in other cases, like file save and copy/past where
                // no viewFrustum was requested, we still want to recurse the child tree.
                if (!params.viewFrustum || !oneAtBit(childrenColoredBits, originalIndex)) {
                    childTreeBytesOut = encodeTreeBitstreamRecursion(childNode, packetData, bag, params, thisLevel);
                }

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

                // If we had previously started writing, and if the child DIDN'T write any bytes,
                // then we want to remove their bit from the childExistsPlaceHolder bitmask
                if (childTreeBytesOut == 0) {
                    // remove this child's bit...
                    childrenExistInPacketBits -= (1 << (7 - originalIndex));

                    // repair the child exists mask
                    continueThisLevel = packetData->updatePriorBitMask(childExistsPlaceHolder, childrenExistInPacketBits);
                    
                    // If this is the last of the child exists bits, then we're actually be rolling out the entire tree
                    if (params.stats && childrenExistInPacketBits == 0) {
                        params.stats->childBitsRemoved(params.includeExistsBits, params.includeColor);
                    }

                    if (!continueThisLevel) {
                        break; // can't continue...
                    }
                    
                    // Note: no need to move the pointer, cause we already stored this
                } // end if (childTreeBytesOut == 0)
            } // end if (oneAtBit(childrenExistInPacketBits, originalIndex))
        } // end for

        // reshuffle here...
        if (continueThisLevel && params.wantOcclusionCulling) {
            unsigned char tempReshuffleBuffer[MAX_OCTREE_UNCOMRESSED_PACKET_SIZE];

            unsigned char* tempBufferTo = &tempReshuffleBuffer[0]; // this is our temporary destination

            // iterate through our childrenExistInPacketBits, these will be the sections of the packet that we copied subTree
            // details into. Unfortunately, they're in distance sorted order, not original index order. we need to put them
            // back into original distance order
            for (int originalIndex = 0; originalIndex < NUMBER_OF_CHILDREN; originalIndex++) {
                if (oneAtBit(childrenExistInPacketBits, originalIndex)) {
                    int thisSliceSize = recursiveSliceSizes[originalIndex];
                    const unsigned char* thisSliceStarts = recursiveSliceStarts[originalIndex];

                    memcpy(tempBufferTo, thisSliceStarts, thisSliceSize);
                    tempBufferTo += thisSliceSize;
                }
            }

            // now that all slices are back in the correct order, copy them to the correct output buffer
            continueThisLevel = packetData->updatePriorBytes(firstRecursiveSliceOffset, &tempReshuffleBuffer[0], allSlicesSize);
        }
    } // end keepDiggingDeeper

    // At this point all our BitMasks are complete... so let's output them to see how they compare...
    /**
    printf("This Level's BitMasks: childInTree:");
    outputBits(childrenExistInTreeBits, false, true);
    printf(" childInPacket:");
    outputBits(childrenExistInPacketBits, false, true);
    printf(" childrenColored:");
    outputBits(childrenColoredBits, false, true);
    printf("\n");
    **/

    // if we were unable to fit this level in our packet, then rewind and add it to the node bag for 
    // sending later...
    if (continueThisLevel) {
        continueThisLevel = packetData->endLevel(thisLevelKey);
    } else {
        packetData->discardLevel(thisLevelKey);
    }
    
    if (!continueThisLevel) {
        bag.insert(node);

        // don't need to check node here, because we can't get here with no node
        if (params.stats) {
            params.stats->didntFit(node);
        }

        params.stopReason = EncodeBitstreamParams::DIDNT_FIT;
        bytesAtThisLevel = 0; // didn't fit
    }

    return bytesAtThisLevel;
}

bool Octree::readFromSVOFile(const char* fileName) {
    std::ifstream file(fileName, std::ios::in|std::ios::binary|std::ios::ate);
    if(file.is_open()) {
        emit importSize(1.0f, 1.0f, 1.0f);
        emit importProgress(0);

        qDebug("loading file %s...\n", fileName);

        // get file length....
        unsigned long fileLength = file.tellg();
        file.seekg( 0, std::ios::beg );

        // read the entire file into a buffer, WHAT!? Why not.
        unsigned char* entireFile = new unsigned char[fileLength];
        file.read((char*)entireFile, fileLength);
        bool wantImportProgress = true;
        ReadBitstreamToTreeParams args(WANT_COLOR, NO_EXISTS_BITS, NULL, 0, wantImportProgress);
        readBitstreamToTree(entireFile, fileLength, args);
        delete[] entireFile;

        emit importProgress(100);

        file.close();
        return true;
    }
    return false;
}

void Octree::writeToSVOFile(const char* fileName, OctreeElement* node) {

    std::ofstream file(fileName, std::ios::out|std::ios::binary);

    if(file.is_open()) {
        qDebug("saving to file %s...\n", fileName);

        OctreeElementBag nodeBag;
        // If we were given a specific node, start from there, otherwise start from root
        if (node) {
            nodeBag.insert(node);
        } else {
            nodeBag.insert(_rootNode);
        }

        static OctreePacketData packetData;
        int bytesWritten = 0;
        bool lastPacketWritten = false;

        while (!nodeBag.isEmpty()) {
            OctreeElement* subTree = nodeBag.extract();
            
            lockForRead(); // do tree locking down here so that we have shorter slices and less thread contention
            EncodeBitstreamParams params(INT_MAX, IGNORE_VIEW_FRUSTUM, WANT_COLOR, NO_EXISTS_BITS);
            bytesWritten = encodeTreeBitstream(subTree, &packetData, nodeBag, params);
            unlock();

            // if bytesWritten == 0, then it means that the subTree couldn't fit, and so we should reset the packet
            // and reinsert the node in our bag and try again...
            if (bytesWritten == 0) {
                if (packetData.hasContent()) {
                    file.write((const char*)packetData.getFinalizedData(), packetData.getFinalizedSize());
                    lastPacketWritten = true;
                }
                packetData.reset(); // is there a better way to do this? could we fit more?
                nodeBag.insert(subTree);
            } else {
                lastPacketWritten = false;
            }
        }

        if (!lastPacketWritten) {
            file.write((const char*)packetData.getFinalizedData(), packetData.getFinalizedSize());
        }

    }
    file.close();
}

unsigned long Octree::getOctreeElementsCount() {
    unsigned long nodeCount = 0;
    recurseTreeWithOperation(countOctreeElementsOperation, &nodeCount);
    return nodeCount;
}

bool Octree::countOctreeElementsOperation(OctreeElement* node, void* extraData) {
    (*(unsigned long*)extraData)++;
    return true; // keep going
}

void Octree::copySubTreeIntoNewTree(OctreeElement* startNode, Octree* destinationTree, bool rebaseToRoot) {
    OctreeElementBag nodeBag;
    nodeBag.insert(startNode);
    int chopLevels = 0;
    if (rebaseToRoot) {
        chopLevels = numberOfThreeBitSectionsInCode(startNode->getOctalCode());
    }

    static OctreePacketData packetData;
    int bytesWritten = 0;

    while (!nodeBag.isEmpty()) {
        OctreeElement* subTree = nodeBag.extract();
        packetData.reset(); // reset the packet between usage
        // ask our tree to write a bitsteam
        EncodeBitstreamParams params(INT_MAX, IGNORE_VIEW_FRUSTUM, WANT_COLOR, NO_EXISTS_BITS, chopLevels);
        bytesWritten = encodeTreeBitstream(subTree, &packetData, nodeBag, params);
        // ask destination tree to read the bitstream
        ReadBitstreamToTreeParams args(WANT_COLOR, NO_EXISTS_BITS);
        destinationTree->readBitstreamToTree(packetData.getUncompressedData(), packetData.getUncompressedSize(), args);
    }

    // XXXBHG - what is this trying to do?
    //      This code appears to be trying to set the color of the destination root
    //      of a copy operation. But that shouldn't be necessary. I think this code might
    //      have been a hack that Mark added when he was trying to solve the copy of a single
    //      voxel bug. But this won't solve that problem, and doesn't appear to be needed for
    //      a normal copy operation. I'm leaving this in for a little bit until we see if anything
    //      about copy/paste is broken.
    //
    //OctreeElement* destinationStartNode;
    //if (rebaseToRoot) {
    //    destinationStartNode = destinationTree->_rootNode;
    //} else {
    //    destinationStartNode = nodeForOctalCode(destinationTree->_rootNode, startNode->getOctalCode(), NULL);
    //}
    //destinationStartNode->setColor(startNode->getColor());
}

void Octree::copyFromTreeIntoSubTree(Octree* sourceTree, OctreeElement* destinationNode) {
    OctreeElementBag nodeBag;
    // If we were given a specific node, start from there, otherwise start from root
    nodeBag.insert(sourceTree->_rootNode);

    static OctreePacketData packetData;
    int bytesWritten = 0;

    while (!nodeBag.isEmpty()) {
        OctreeElement* subTree = nodeBag.extract();
        
        packetData.reset(); // reset between usage

        // ask our tree to write a bitsteam
        EncodeBitstreamParams params(INT_MAX, IGNORE_VIEW_FRUSTUM, WANT_COLOR, NO_EXISTS_BITS);
        bytesWritten = sourceTree->encodeTreeBitstream(subTree, &packetData, nodeBag, params);

        // ask destination tree to read the bitstream
        bool wantImportProgress = true;
        ReadBitstreamToTreeParams args(WANT_COLOR, NO_EXISTS_BITS, destinationNode, 0, wantImportProgress);
        readBitstreamToTree(packetData.getUncompressedData(), packetData.getUncompressedSize(), args);
    }
}

void dumpSetContents(const char* name, std::set<unsigned char*> set) {
    printf("set %s has %ld elements\n", name, set.size());
    /*
    for (std::set<unsigned char*>::iterator i = set.begin(); i != set.end(); ++i) {
        printOctalCode(*i);
    }
    */
}

void Octree::startEncoding(OctreeElement* node) {
    pthread_mutex_lock(&_encodeSetLock);
    _codesBeingEncoded.insert(node->getOctalCode());
    pthread_mutex_unlock(&_encodeSetLock);
}

void Octree::doneEncoding(OctreeElement* node) {
    pthread_mutex_lock(&_encodeSetLock);
    _codesBeingEncoded.erase(node->getOctalCode());
    pthread_mutex_unlock(&_encodeSetLock);
    
    // if we have any pending delete codes, then delete them now.
    emptyDeleteQueue();
}

void Octree::startDeleting(const unsigned char* code) {
    pthread_mutex_lock(&_deleteSetLock);
    _codesBeingDeleted.insert(code);
    pthread_mutex_unlock(&_deleteSetLock);
}

void Octree::doneDeleting(const unsigned char* code) {
    pthread_mutex_lock(&_deleteSetLock);
    _codesBeingDeleted.erase(code);
    pthread_mutex_unlock(&_deleteSetLock);
}

bool Octree::isEncoding(const unsigned char* codeBuffer) {
    pthread_mutex_lock(&_encodeSetLock);
    bool isEncoding = (_codesBeingEncoded.find(codeBuffer) != _codesBeingEncoded.end());
    pthread_mutex_unlock(&_encodeSetLock);
    return isEncoding;
}

void Octree::queueForLaterDelete(const unsigned char* codeBuffer) {
    pthread_mutex_lock(&_deletePendingSetLock);
    _codesPendingDelete.insert(codeBuffer);
    pthread_mutex_unlock(&_deletePendingSetLock);
}

void Octree::emptyDeleteQueue() {
    pthread_mutex_lock(&_deletePendingSetLock);
    for (std::set<const unsigned char*>::iterator i = _codesPendingDelete.begin(); i != _codesPendingDelete.end(); ++i) {
        const unsigned char* codeToDelete = *i;
        _codesBeingDeleted.erase(codeToDelete);
        deleteOctalCodeFromTree(codeToDelete, COLLAPSE_EMPTY_TREE);
    }
    pthread_mutex_unlock(&_deletePendingSetLock);
}

void Octree::cancelImport() {
    _stopImport = true;
}

