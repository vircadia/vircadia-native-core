//
//  Octree.cpp
//  libraries/octree/src
//
//  Created by Stephen Birarda on 3/13/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif

#include <cstring>
#include <cstdio>
#include <cmath>
#include <fstream> // to load voxels from file

#include <QDebug>

#include <GeometryUtil.h>
#include <OctalCode.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <Shape.h>
#include <ShapeCollider.h>

//#include "Tags.h"

#include "CoverageMap.h"
#include "OctreeConstants.h"
#include "OctreeElementBag.h"
#include "Octree.h"
#include "ViewFrustum.h"

float boundaryDistanceForRenderLevel(unsigned int renderLevel, float voxelSizeScale) {
    return voxelSizeScale / powf(2, renderLevel);
}

Octree::Octree(bool shouldReaverage) :
    _rootElement(NULL),
    _isDirty(true),
    _shouldReaverage(shouldReaverage),
    _stopImport(false),
    _lock(),
    _isViewing(false) 
{
}

Octree::~Octree() {
    // delete the children of the root element
    // this recursively deletes the tree
    delete _rootElement;
}

// Recurses voxel tree calling the RecurseOctreeOperation function for each element.
// stops recursion if operation function returns false.
void Octree::recurseTreeWithOperation(RecurseOctreeOperation operation, void* extraData) {
    recurseElementWithOperation(_rootElement, operation, extraData);
}

// Recurses voxel tree calling the RecurseOctreePostFixOperation function for each element in post-fix order.
void Octree::recurseTreeWithPostOperation(RecurseOctreeOperation operation, void* extraData) {
    recurseElementWithPostOperation(_rootElement, operation, extraData);
}

// Recurses voxel element with an operation function
void Octree::recurseElementWithOperation(OctreeElement* element, RecurseOctreeOperation operation, void* extraData,
                        int recursionCount) {
    if (recursionCount > DANGEROUSLY_DEEP_RECURSION) {
        qDebug() << "Octree::recurseElementWithOperation() reached DANGEROUSLY_DEEP_RECURSION, bailing!";
        return;
    }

    if (operation(element, extraData)) {
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            OctreeElement* child = element->getChildAtIndex(i);
            if (child) {
                recurseElementWithOperation(child, operation, extraData, recursionCount+1);
            }
        }
    }
}

// Recurses voxel element with an operation function
void Octree::recurseElementWithPostOperation(OctreeElement* element, RecurseOctreeOperation operation, void* extraData,
                        int recursionCount) {
    if (recursionCount > DANGEROUSLY_DEEP_RECURSION) {
        qDebug() << "Octree::recurseElementWithOperation() reached DANGEROUSLY_DEEP_RECURSION, bailing!\n";
        return;
    }

    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        OctreeElement* child = element->getChildAtIndex(i);
        if (child) {
            recurseElementWithPostOperation(child, operation, extraData, recursionCount+1);
        }
    }
	operation(element, extraData);
}

// Recurses voxel tree calling the RecurseOctreeOperation function for each element.
// stops recursion if operation function returns false.
void Octree::recurseTreeWithOperationDistanceSorted(RecurseOctreeOperation operation,
                                                       const glm::vec3& point, void* extraData) {

    recurseElementWithOperationDistanceSorted(_rootElement, operation, point, extraData);
}

// Recurses voxel element with an operation function
void Octree::recurseElementWithOperationDistanceSorted(OctreeElement* element, RecurseOctreeOperation operation,
                                                       const glm::vec3& point, void* extraData, int recursionCount) {

    if (recursionCount > DANGEROUSLY_DEEP_RECURSION) {
        qDebug() << "Octree::recurseElementWithOperationDistanceSorted() reached DANGEROUSLY_DEEP_RECURSION, bailing!";
        return;
    }

    if (operation(element, extraData)) {
        // determine the distance sorted order of our children
        OctreeElement* sortedChildren[NUMBER_OF_CHILDREN] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
        float distancesToChildren[NUMBER_OF_CHILDREN] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        int indexOfChildren[NUMBER_OF_CHILDREN] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        int currentCount = 0;

        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            OctreeElement* childElement = element->getChildAtIndex(i);
            if (childElement) {
                // chance to optimize, doesn't need to be actual distance!! Could be distance squared
                float distanceSquared = childElement->distanceSquareToPoint(point);
                currentCount = insertIntoSortedArrays((void*)childElement, distanceSquared, i,
                                                      (void**)&sortedChildren, (float*)&distancesToChildren,
                                                      (int*)&indexOfChildren, currentCount, NUMBER_OF_CHILDREN);
            }
        }

        for (int i = 0; i < currentCount; i++) {
            OctreeElement* childElement = sortedChildren[i];
            if (childElement) {
                recurseElementWithOperationDistanceSorted(childElement, operation, point, extraData);
            }
        }
    }
}

void Octree::recurseTreeWithOperator(RecurseOctreeOperator* operatorObject) {
    recurseElementWithOperator(_rootElement, operatorObject);
}

bool Octree::recurseElementWithOperator(OctreeElement* element, RecurseOctreeOperator* operatorObject, int recursionCount) {
    if (recursionCount > DANGEROUSLY_DEEP_RECURSION) {
        qDebug() << "Octree::recurseElementWithOperation() reached DANGEROUSLY_DEEP_RECURSION, bailing!";
        return false;
    }

    if (operatorObject->PreRecursion(element)) {
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            OctreeElement* child = element->getChildAtIndex(i);
            if (child) {
                if (!recurseElementWithOperator(child, operatorObject, recursionCount + 1)) {
                    break; // stop recursing if operator returns false...
                }
            }
        }
    }
    
    return operatorObject->PostRecursion(element);
}


OctreeElement* Octree::nodeForOctalCode(OctreeElement* ancestorElement,
                                       const unsigned char* needleCode, OctreeElement** parentOfFoundElement) const {
    // special case for NULL octcode
    if (!needleCode) {
        return _rootElement;
    }

    // find the appropriate branch index based on this ancestorElement
    if (*needleCode > 0) {
        int branchForNeedle = branchIndexWithDescendant(ancestorElement->getOctalCode(), needleCode);
        OctreeElement* childElement = ancestorElement->getChildAtIndex(branchForNeedle);

        if (childElement) {
            if (*childElement->getOctalCode() == *needleCode) {

                // If the caller asked for the parent, then give them that too...
                if (parentOfFoundElement) {
                    *parentOfFoundElement = ancestorElement;
                }
                // the fact that the number of sections is equivalent does not always guarantee
                // that this is the same element, however due to the recursive traversal
                // we know that this is our element
                return childElement;
            } else {
                // we need to go deeper
                return nodeForOctalCode(childElement, needleCode, parentOfFoundElement);
            }
        }
    }

    // we've been given a code we don't have a element for
    // return this element as the last created parent
    return ancestorElement;
}

// returns the element created!
OctreeElement* Octree::createMissingElement(OctreeElement* lastParentElement, const unsigned char* codeToReach) {
    int indexOfNewChild = branchIndexWithDescendant(lastParentElement->getOctalCode(), codeToReach);
    // If this parent element is a leaf, then you know the child path doesn't exist, so deal with
    // breaking up the leaf first, which will also create a child path
    if (lastParentElement->requiresSplit()) {
        lastParentElement->splitChildren();
    } else if (!lastParentElement->getChildAtIndex(indexOfNewChild)) {
        // we could be coming down a branch that was already created, so don't stomp on it.
        lastParentElement->addChildAtIndex(indexOfNewChild);
    }

    // This works because we know we traversed down the same tree so if the length is the same, then the whole code is the same
    if (*lastParentElement->getChildAtIndex(indexOfNewChild)->getOctalCode() == *codeToReach) {
        return lastParentElement->getChildAtIndex(indexOfNewChild);
    } else {
        return createMissingElement(lastParentElement->getChildAtIndex(indexOfNewChild), codeToReach);
    }
}

int Octree::readElementData(OctreeElement* destinationElement, const unsigned char* nodeData, int bytesLeftToRead,
                            ReadBitstreamToTreeParams& args) {
    // give this destination element the child mask from the packet
    const unsigned char ALL_CHILDREN_ASSUMED_TO_EXIST = 0xFF;
    unsigned char colorInPacketMask = *nodeData;

    // instantiate variable for bytes already read
    int bytesRead = sizeof(colorInPacketMask);
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        // check the colors mask to see if we have a child to color in
        if (oneAtBit(colorInPacketMask, i)) {
            // create the child if it doesn't exist
            if (!destinationElement->getChildAtIndex(i)) {
                destinationElement->addChildAtIndex(i);
                if (destinationElement->isDirty()) {
                    _isDirty = true;
                }
            }

            OctreeElement* childElementAt = destinationElement->getChildAtIndex(i);
            bool nodeIsDirty = false;
            if (childElementAt) {
                bytesRead += childElementAt->readElementDataFromBuffer(nodeData + bytesRead, bytesLeftToRead, args);
                childElementAt->setSourceUUID(args.sourceUUID);

                // if we had a local version of the element already, it's possible that we have it already but
                // with the same color data, so this won't count as a change. To address this we check the following
                if (!childElementAt->isDirty() && childElementAt->getShouldRender() && !childElementAt->isRendered()) {
                    childElementAt->setDirtyBit(); // force dirty!
                }

                nodeIsDirty = childElementAt->isDirty();
            }
            if (nodeIsDirty) {
                _isDirty = true;
            }
        }
    }

    // give this destination element the child mask from the packet
    unsigned char childrenInTreeMask = args.includeExistsBits ? *(nodeData + bytesRead) : ALL_CHILDREN_ASSUMED_TO_EXIST;
    unsigned char childMask = *(nodeData + bytesRead + (args.includeExistsBits ? sizeof(childrenInTreeMask) : 0));

    int childIndex = 0;
    bytesRead += args.includeExistsBits ? sizeof(childrenInTreeMask) + sizeof(childMask) : sizeof(childMask);

    while (bytesLeftToRead - bytesRead > 0 && childIndex < NUMBER_OF_CHILDREN) {
        // check the exists mask to see if we have a child to traverse into

        if (oneAtBit(childMask, childIndex)) {
            if (!destinationElement->getChildAtIndex(childIndex)) {
                // add a child at that index, if it doesn't exist
                destinationElement->addChildAtIndex(childIndex);
                bool nodeIsDirty = destinationElement->isDirty();
                if (nodeIsDirty) {
                    _isDirty = true;
                }
            }

            // tell the child to read the subsequent data
            bytesRead += readElementData(destinationElement->getChildAtIndex(childIndex),
                                      nodeData + bytesRead, bytesLeftToRead - bytesRead, args);
        }
        childIndex++;
    }

    if (args.includeExistsBits) {
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            // now also check the childrenInTreeMask, if the mask is missing the bit, then it means we need to delete this child
            // subtree/element, because it shouldn't actually exist in the tree.
            if (!oneAtBit(childrenInTreeMask, i) && destinationElement->getChildAtIndex(i)) {
                destinationElement->safeDeepDeleteChildAtIndex(i);
                _isDirty = true; // by definition!
            }
        }
    }
    
    // if this is the root, and there is more data to read, allow it to read it's element data...
    if (destinationElement == _rootElement  && rootElementHasData() && (bytesLeftToRead - bytesRead) > 0) {
        // tell the element to read the subsequent data
        bytesRead += _rootElement->readElementDataFromBuffer(nodeData + bytesRead, bytesLeftToRead - bytesRead, args);
    }
    
    return bytesRead;
}

void Octree::readBitstreamToTree(const unsigned char * bitstream, unsigned long int bufferSizeBytes,
                                    ReadBitstreamToTreeParams& args) {
    int bytesRead = 0;
    const unsigned char* bitstreamAt = bitstream;

    // If destination element is not included, set it to root
    if (!args.destinationElement) {
        args.destinationElement = _rootElement;
    }

    // Keep looping through the buffer calling readElementData() this allows us to pack multiple root-relative Octal codes
    // into a single network packet. readElementData() basically goes down a tree from the root, and fills things in from there
    // if there are more bytes after that, it's assumed to be another root relative tree

    while (bitstreamAt < bitstream + bufferSizeBytes) {
        OctreeElement* bitstreamRootElement = nodeForOctalCode(args.destinationElement, (unsigned char *)bitstreamAt, NULL);
        if (*bitstreamAt != *bitstreamRootElement->getOctalCode()) {
            // if the octal code returned is not on the same level as
            // the code being searched for, we have OctreeElements to create

            // Note: we need to create this element relative to root, because we're assuming that the bitstream for the initial
            // octal code is always relative to root!
            bitstreamRootElement = createMissingElement(args.destinationElement, (unsigned char*) bitstreamAt);
            if (bitstreamRootElement->isDirty()) {
                _isDirty = true;
            }
        }

        int octalCodeBytes = bytesRequiredForCodeLength(*bitstreamAt);
        int theseBytesRead = 0;
        theseBytesRead += octalCodeBytes;
        theseBytesRead += readElementData(bitstreamRootElement, bitstreamAt + octalCodeBytes,
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
    lockForWrite();
    deleteOctalCodeFromTree(octalCode);
    unlock();
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
// this only finds and deletes the element from the tree.
void Octree::deleteOctalCodeFromTree(const unsigned char* codeBuffer, bool collapseEmptyTrees) {
    // recurse the tree while decoding the codeBuffer, once you find the element in question, recurse
    // back and implement color reaveraging, and marking of lastChanged
    DeleteOctalCodeFromTreeArgs args;
    args.collapseEmptyTrees = collapseEmptyTrees;
    args.codeBuffer         = codeBuffer;
    args.lengthOfCode       = numberOfThreeBitSectionsInCode(codeBuffer);
    args.deleteLastChild    = false;
    args.pathChanged        = false;

    deleteOctalCodeFromTreeRecursion(_rootElement, &args);
}

void Octree::deleteOctalCodeFromTreeRecursion(OctreeElement* element, void* extraData) {
    DeleteOctalCodeFromTreeArgs* args = (DeleteOctalCodeFromTreeArgs*)extraData;

    int lengthOfElementCode = numberOfThreeBitSectionsInCode(element->getOctalCode());

    // Since we traverse the tree in code order, we know that if our code
    // matches, then we've reached  our target element.
    if (lengthOfElementCode == args->lengthOfCode) {
        // we've reached our target, depending on how we're called we may be able to operate on it
        // it here, we need to recurse up, and delete it there. So we handle these cases the same to keep
        // the logic consistent.
        args->deleteLastChild = true;
        return;
    }

    // Ok, we know we haven't reached our target element yet, so keep looking
    int childIndex = branchIndexWithDescendant(element->getOctalCode(), args->codeBuffer);
    OctreeElement* childElement = element->getChildAtIndex(childIndex);

    // If there is no child at the target location, and the current parent element is a colored leaf,
    // then it means we were asked to delete a child out of a larger leaf voxel.
    // We support this by breaking up the parent voxel into smaller pieces.
    if (!childElement && element->requiresSplit()) {
        // we need to break up ancestors until we get to the right level
        OctreeElement* ancestorElement = element;
        while (true) {
            int index = branchIndexWithDescendant(ancestorElement->getOctalCode(), args->codeBuffer);

            // we end up with all the children, even the one we want to delete
            ancestorElement->splitChildren();

            int lengthOfAncestorElement = numberOfThreeBitSectionsInCode(ancestorElement->getOctalCode());

            // If we've reached the parent of the target, then stop breaking up children
            if (lengthOfAncestorElement == (args->lengthOfCode - 1)) {

                // since we created all the children when we split, we need to delete this target one
                ancestorElement->deleteChildAtIndex(index);
                break;
            }
            ancestorElement = ancestorElement->getChildAtIndex(index);
        }
        _isDirty = true;
        args->pathChanged = true;

        // ends recursion, unwinds up stack
        return;
    }

    // if we don't have a child and we reach this point, then we actually know that the parent
    // isn't a colored leaf, and the child branch doesn't exist, so there's nothing to do below and
    // we can safely return, ending the recursion and unwinding
    if (!childElement) {
        return;
    }

    // If we got this far then we have a child for the branch we're looking for, but we're not there yet
    // recurse till we get there
    deleteOctalCodeFromTreeRecursion(childElement, args);

    // If the lower level determined it needs to be deleted, then we should delete now.
    if (args->deleteLastChild) {
        element->deleteChildAtIndex(childIndex); // note: this will track dirtiness and lastChanged for this element

        // track our tree dirtiness
        _isDirty = true;

        // track that path has changed
        args->pathChanged = true;

        // If we're in collapseEmptyTrees mode, and this was the last child of this element, then we also want
        // to delete this element.  This will collapse the empty tree above us.
        if (args->collapseEmptyTrees && element->getChildCount() == 0) {
            // Can't delete the root this way.
            if (element == _rootElement) {
                args->deleteLastChild = false; // reset so that further up the unwinding chain we don't do anything
            }
        } else {
            args->deleteLastChild = false; // reset so that further up the unwinding chain we don't do anything
        }
    }

    // If the lower level did some work, then we need to let this element know, so it can
    // do any bookkeeping it wants to, like color re-averaging, time stamp marking, etc
    if (args->pathChanged) {
        element->handleSubtreeChanged(this);
    }
}

void Octree::eraseAllOctreeElements() {
    delete _rootElement; // this will recurse and delete all children
    _rootElement = createNewElement();
    _isDirty = true;
}

void Octree::processRemoveOctreeElementsBitstream(const unsigned char* bitstream, int bufferSizeBytes) {
    //unsigned short int itemNumber = (*((unsigned short int*)&bitstream[sizeof(PACKET_HEADER)]));

    int numBytesPacketHeader = numBytesForPacketHeader(reinterpret_cast<const char*>(bitstream));
    unsigned short int sequence = (*((unsigned short int*)(bitstream + numBytesPacketHeader)));
    quint64 sentAt = (*((quint64*)(bitstream + numBytesPacketHeader + sizeof(sequence))));

    int atByte = numBytesPacketHeader + sizeof(sequence) + sizeof(sentAt);

    unsigned char* voxelCode = (unsigned char*)&bitstream[atByte];
    while (atByte < bufferSizeBytes) {
        int maxSize = bufferSizeBytes - atByte;
        int codeLength = numberOfThreeBitSectionsInCode(voxelCode, maxSize);

        if (codeLength == OVERFLOWED_OCTCODE_BUFFER) {
            qDebug("WARNING! Got remove voxel bitstream that would overflow buffer in numberOfThreeBitSectionsInCode(),"
                   " bailing processing of packet!");
            break;
        }
        int voxelDataSize = bytesRequiredForCodeLength(codeLength) + SIZE_OF_COLOR_DATA;

        if (atByte + voxelDataSize <= bufferSizeBytes) {
            deleteOctalCodeFromTree(voxelCode, COLLAPSE_EMPTY_TREE);
            voxelCode += voxelDataSize;
            atByte += voxelDataSize;
        } else {
            qDebug("WARNING! Got remove voxel bitstream that would overflow buffer, bailing processing!");
            break;
        }
    }
}

// Note: this is an expensive call. Don't call it unless you really need to reaverage the entire tree (from startElement)
void Octree::reaverageOctreeElements(OctreeElement* startElement) {
    if (!startElement) {
        startElement = getRoot();
    }
    // if our tree is a reaveraging tree, then we do this, otherwise we don't do anything
    if (_shouldReaverage) {
        static int recursionCount;
        if (startElement == _rootElement) {
            recursionCount = 0;
        } else {
            recursionCount++;
        }
        if (recursionCount > UNREASONABLY_DEEP_RECURSION) {
            qDebug("Octree::reaverageOctreeElements()... bailing out of UNREASONABLY_DEEP_RECURSION");
            recursionCount--;
            return;
        }

        bool hasChildren = false;

        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            if (startElement->getChildAtIndex(i)) {
                reaverageOctreeElements(startElement->getChildAtIndex(i));
                hasChildren = true;
            }
        }

        // collapseIdenticalLeaves() returns true if it collapses the leaves
        // in which case we don't need to set the average color
        if (hasChildren && !startElement->collapseChildren()) {
            startElement->calculateAverageFromChildren();
        }
        recursionCount--;
    }
}

OctreeElement* Octree::getOctreeElementAt(float x, float y, float z, float s) const {
    unsigned char* octalCode = pointToOctalCode(x,y,z,s);
    OctreeElement* element = nodeForOctalCode(_rootElement, octalCode, NULL);
    if (*element->getOctalCode() != *octalCode) {
        element = NULL;
    }
    delete[] octalCode; // cleanup memory
#ifdef HAS_AUDIT_CHILDREN
    if (element) {
        element->auditChildren("Octree::getOctreeElementAt()");
    }
#endif // def HAS_AUDIT_CHILDREN
    return element;
}

OctreeElement* Octree::getOctreeEnclosingElementAt(float x, float y, float z, float s) const {
    unsigned char* octalCode = pointToOctalCode(x,y,z,s);
    OctreeElement* element = nodeForOctalCode(_rootElement, octalCode, NULL);
    
    delete[] octalCode; // cleanup memory
#ifdef HAS_AUDIT_CHILDREN
    if (element) {
        element->auditChildren("Octree::getOctreeElementAt()");
    }
#endif // def HAS_AUDIT_CHILDREN
    return element;
}


OctreeElement* Octree::getOrCreateChildElementAt(float x, float y, float z, float s) {
    return getRoot()->getOrCreateChildElementAt(x, y, z, s);
}

OctreeElement* Octree::getOrCreateChildElementContaining(const AACube& box) {
    return getRoot()->getOrCreateChildElementContaining(box);
}


// combines the ray cast arguments into a single object
class RayArgs {
public:
    glm::vec3 origin;
    glm::vec3 direction;
    OctreeElement*& element;
    float& distance;
    BoxFace& face;
    void** intersectedObject;
    bool found;
};

bool findRayIntersectionOp(OctreeElement* element, void* extraData) {
    RayArgs* args = static_cast<RayArgs*>(extraData);
    bool keepSearching = true;
    if (element->findRayIntersection(args->origin, args->direction, keepSearching, 
                            args->element, args->distance, args->face, args->intersectedObject)) {
        args->found = true;
    }
    return keepSearching;
}

bool Octree::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                    OctreeElement*& element, float& distance, BoxFace& face, void** intersectedObject,
                                    Octree::lockType lockType, bool* accurateResult) {
    RayArgs args = { origin / (float)(TREE_SCALE), direction, element, distance, face, intersectedObject, false};
    distance = FLT_MAX;

    bool gotLock = false;
    if (lockType == Octree::Lock) {
        lockForRead();
        gotLock = true;
    } else if (lockType == Octree::TryLock) {
        gotLock = tryLockForRead();
        if (!gotLock) {
            if (accurateResult) {
                *accurateResult = false; // if user asked to accuracy or result, let them know this is inaccurate
            }
            return args.found; // if we wanted to tryLock, and we couldn't then just bail...
        }
    }

    recurseTreeWithOperation(findRayIntersectionOp, &args);

    if (gotLock) {
        unlock();
    }

    if (accurateResult) {
        *accurateResult = true; // if user asked to accuracy or result, let them know this is accurate
    }
    return args.found;
}

class SphereArgs {
public:
    glm::vec3 center;
    float radius;
    glm::vec3& penetration;
    bool found;
    void* penetratedObject; /// the type is defined by the type of Octree, the caller is assumed to know the type
};

bool findSpherePenetrationOp(OctreeElement* element, void* extraData) {
    SphereArgs* args = static_cast<SphereArgs*>(extraData);

    // coarse check against bounds
    const AACube& box = element->getAACube();
    if (!box.expandedContains(args->center, args->radius)) {
        return false;
    }
    if (!element->isLeaf()) {
        return true; // recurse on children
    }
    if (element->hasContent()) {
        glm::vec3 elementPenetration;
        if (element->findSpherePenetration(args->center, args->radius, elementPenetration, &args->penetratedObject)) {
            // NOTE: it is possible for this penetration accumulation algorithm to produce a 
            // final penetration vector with zero length.
            args->penetration = addPenetrations(args->penetration, elementPenetration * (float)(TREE_SCALE));
            args->found = true;
        }
    }
    return false;
}

bool Octree::findSpherePenetration(const glm::vec3& center, float radius, glm::vec3& penetration,
                    void** penetratedObject, Octree::lockType lockType, bool* accurateResult) {

    SphereArgs args = {
        center / (float)(TREE_SCALE),
        radius / (float)(TREE_SCALE),
        penetration,
        false,
        NULL };
    penetration = glm::vec3(0.0f, 0.0f, 0.0f);

    bool gotLock = false;
    if (lockType == Octree::Lock) {
        lockForRead();
        gotLock = true;
    } else if (lockType == Octree::TryLock) {
        gotLock = tryLockForRead();
        if (!gotLock) {
            if (accurateResult) {
                *accurateResult = false; // if user asked to accuracy or result, let them know this is inaccurate
            }
            return args.found; // if we wanted to tryLock, and we couldn't then just bail...
        }
    }

    recurseTreeWithOperation(findSpherePenetrationOp, &args);
    if (penetratedObject) {
        *penetratedObject = args.penetratedObject;
    }

    if (gotLock) {
        unlock();
    }
    
    if (accurateResult) {
        *accurateResult = true; // if user asked to accuracy or result, let them know this is accurate
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

class ShapeArgs {
public:
    const Shape* shape;
    CollisionList& collisions;
    bool found;
};

bool findCapsulePenetrationOp(OctreeElement* element, void* extraData) {
    CapsuleArgs* args = static_cast<CapsuleArgs*>(extraData);

    // coarse check against bounds
    const AACube& box = element->getAACube();
    if (!box.expandedIntersectsSegment(args->start, args->end, args->radius)) {
        return false;
    }
    if (!element->isLeaf()) {
        return true; // recurse on children
    }
    if (element->hasContent()) {
        glm::vec3 nodePenetration;
        if (box.findCapsulePenetration(args->start, args->end, args->radius, nodePenetration)) {
            args->penetration = addPenetrations(args->penetration, nodePenetration * (float)(TREE_SCALE));
            args->found = true;
        }
    }
    return false;
}

/* TODO: Andrew to reimplement or purge this
bool findShapeCollisionsOp(OctreeElement* element, void* extraData) {
    ShapeArgs* args = static_cast<ShapeArgs*>(extraData);

    // coarse check against bounds
    AACube cube = element->getAACube();
    cube.scale(TREE_SCALE);
    if (!cube.expandedContains(args->shape->getTranslation(), args->shape->getBoundingRadius())) {
        return false;
    }
    if (!element->isLeaf()) {
        return true; // recurse on children
    }
    if (element->hasContent()) {
        if (ShapeCollider::collideShapeWithAACube(args->shape, cube.calcCenter(), cube.getScale(), args->collisions)) {
            args->found = true;
            return true;
        }
    }
    return false;
}
*/

bool Octree::findCapsulePenetration(const glm::vec3& start, const glm::vec3& end, float radius, 
                    glm::vec3& penetration, Octree::lockType lockType, bool* accurateResult) {
                    
    CapsuleArgs args = {
        start / (float)(TREE_SCALE),
        end / (float)(TREE_SCALE),
        radius / (float)(TREE_SCALE),
        penetration,
        false };
    penetration = glm::vec3(0.0f, 0.0f, 0.0f);

    bool gotLock = false;
    if (lockType == Octree::Lock) {
        lockForRead();
        gotLock = true;
    } else if (lockType == Octree::TryLock) {
        gotLock = tryLockForRead();
        if (!gotLock) {
            if (accurateResult) {
                *accurateResult = false; // if user asked to accuracy or result, let them know this is inaccurate
            }
            return args.found; // if we wanted to tryLock, and we couldn't then just bail...
        }
    }

    recurseTreeWithOperation(findCapsulePenetrationOp, &args);
    
    if (gotLock) {
        unlock();
    }

    if (accurateResult) {
        *accurateResult = true; // if user asked to accuracy or result, let them know this is accurate
    }
    return args.found;
}

/* TODO: Andrew to reimplement or purge this
bool Octree::findShapeCollisions(const Shape* shape, CollisionList& collisions, 
                    Octree::lockType lockType, bool* accurateResult) {

    ShapeArgs args = { shape, collisions, false };

    bool gotLock = false;
    if (lockType == Octree::Lock) {
        lockForRead();
        gotLock = true;
    } else if (lockType == Octree::TryLock) {
        gotLock = tryLockForRead();
        if (!gotLock) {
            if (accurateResult) {
                *accurateResult = false; // if user asked to accuracy or result, let them know this is inaccurate
            }
            return args.found; // if we wanted to tryLock, and we couldn't then just bail...
        }
    }

    recurseTreeWithOperation(findShapeCollisionsOp, &args);
    
    if (gotLock) {
        unlock();
    }

    if (accurateResult) {
        *accurateResult = true; // if user asked to accuracy or result, let them know this is accurate
    }
    return args.found;
}
*/

class GetElementEnclosingArgs {
public:
    OctreeElement* element;
    glm::vec3 point;
};

// Find the smallest colored voxel enclosing a point (if there is one)
bool getElementEnclosingOperation(OctreeElement* element, void* extraData) {
    GetElementEnclosingArgs* args = static_cast<GetElementEnclosingArgs*>(extraData);
    AACube elementBox = element->getAACube();
    if (elementBox.contains(args->point)) {
        if (element->hasContent() && element->isLeaf()) {
            // we've reached a solid leaf containing the point, return the element.
            args->element = element;
            return false;
        }
    } else {
        //  The point is not inside this voxel, so stop recursing.
        return false;
    }
    return true; // keep looking
}

OctreeElement* Octree::getElementEnclosingPoint(const glm::vec3& point, Octree::lockType lockType, bool* accurateResult) {
    GetElementEnclosingArgs args;
    args.point = point;
    args.element = NULL;
    
    bool gotLock = false;
    if (lockType == Octree::Lock) {
        lockForRead();
        gotLock = true;
    } else if (lockType == Octree::TryLock) {
        gotLock = tryLockForRead();
        if (!gotLock) {
            if (accurateResult) {
                *accurateResult = false; // if user asked to accuracy or result, let them know this is inaccurate
            }
            return args.element; // if we wanted to tryLock, and we couldn't then just bail...
        }
    }

    recurseTreeWithOperation(getElementEnclosingOperation, (void*)&args);
    
    if (gotLock) {
        unlock();
    }

    if (accurateResult) {
        *accurateResult = false; // if user asked to accuracy or result, let them know this is inaccurate
    }
    return args.element;
}



int Octree::encodeTreeBitstream(OctreeElement* element,
                        OctreePacketData* packetData, OctreeElementBag& bag,
                        EncodeBitstreamParams& params) {

    // How many bytes have we written so far at this level;
    int bytesWritten = 0;

    // you can't call this without a valid element
    if (!element) {
        qDebug("WARNING! encodeTreeBitstream() called with element=NULL");
        params.stopReason = EncodeBitstreamParams::NULL_NODE;
        return bytesWritten;
    }

    // If we're at a element that is out of view, then we can return, because no nodes below us will be in view!
    if (params.viewFrustum && !element->isInView(*params.viewFrustum)) {
        params.stopReason = EncodeBitstreamParams::OUT_OF_VIEW;
        return bytesWritten;
    }

    // write the octal code
    bool roomForOctalCode = false; // assume the worst
    int codeLength = 1; // assume root
    if (params.chopLevels) {
        unsigned char* newCode = chopOctalCode(element->getOctalCode(), params.chopLevels);
        roomForOctalCode = packetData->startSubTree(newCode);

        if (newCode) {
            delete newCode;
            codeLength = numberOfThreeBitSectionsInCode(newCode);
        } else {
            codeLength = 1;
        }
    } else {
        roomForOctalCode = packetData->startSubTree(element->getOctalCode());
        codeLength = bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(element->getOctalCode()));
    }

    // If the octalcode couldn't fit, then we can return, because no nodes below us will fit...
    if (!roomForOctalCode) {
        bag.insert(element); // add the element back to the bag so it will eventually get included
        params.stopReason = EncodeBitstreamParams::DIDNT_FIT;
        return bytesWritten;
    }

    bytesWritten += codeLength; // keep track of byte count

    int currentEncodeLevel = 0;

    // record some stats, this is the one element that we won't record below in the recursion function, so we need to
    // track it here
    if (params.stats) {
        params.stats->traversed(element);
    }

    ViewFrustum::location parentLocationThisView = ViewFrustum::INTERSECT; // assume parent is in view, but not fully

    int childBytesWritten = encodeTreeBitstreamRecursion(element, packetData, bag, params,
                                                            currentEncodeLevel, parentLocationThisView);

    // if childBytesWritten == 1 then something went wrong... that's not possible
    assert(childBytesWritten != 1);

    // if includeColor and childBytesWritten == 2, then it can only mean that the lower level trees don't exist or for some
    // reason couldn't be written... so reset them here... This isn't true for the non-color included case
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

    return bytesWritten;
}

int Octree::encodeTreeBitstreamRecursion(OctreeElement* element,
                                            OctreePacketData* packetData, OctreeElementBag& bag,
                                            EncodeBitstreamParams& params, int& currentEncodeLevel,
                                            const ViewFrustum::location& parentLocationThisView) const {
    // How many bytes have we written so far at this level;
    int bytesAtThisLevel = 0;

    // you can't call this without a valid element
    if (!element) {
        qDebug("WARNING! encodeTreeBitstreamRecursion() called with element=NULL");
        params.stopReason = EncodeBitstreamParams::NULL_NODE;
        return bytesAtThisLevel;
    }

    // Keep track of how deep we've encoded.
    currentEncodeLevel++;

    params.maxLevelReached = std::max(currentEncodeLevel, params.maxLevelReached);

    // If we've reached our max Search Level, then stop searching.
    if (currentEncodeLevel >= params.maxEncodeLevel) {
        params.stopReason = EncodeBitstreamParams::TOO_DEEP;
        return bytesAtThisLevel;
    }

    // If we've been provided a jurisdiction map, then we need to honor it.
    if (params.jurisdictionMap) {
        // here's how it works... if we're currently above our root jurisdiction, then we proceed normally.
        // but once we're in our own jurisdiction, then we need to make sure we're not below it.
        if (JurisdictionMap::BELOW == params.jurisdictionMap->isMyJurisdiction(element->getOctalCode(), CHECK_NODE_ONLY)) {
            params.stopReason = EncodeBitstreamParams::OUT_OF_JURISDICTION;
            return bytesAtThisLevel;
        }
    }
    
    ViewFrustum::location nodeLocationThisView = ViewFrustum::INSIDE; // assume we're inside

    // caller can pass NULL as viewFrustum if they want everything
    if (params.viewFrustum) {
        float distance = element->distanceToCamera(*params.viewFrustum);
        float boundaryDistance = boundaryDistanceForRenderLevel(element->getLevel() + params.boundaryLevelAdjust,
                                        params.octreeElementSizeScale);

        // If we're too far away for our render level, then just return
        if (distance >= boundaryDistance) {
            if (params.stats) {
                params.stats->skippedDistance(element);
            }
            params.stopReason = EncodeBitstreamParams::LOD_SKIP;
            return bytesAtThisLevel;
        }

        // if the parent isn't known to be INSIDE, then it must be INTERSECT, and we should double check to see
        // if we are INSIDE, INTERSECT, or OUTSIDE
        if (parentLocationThisView != ViewFrustum::INSIDE) {
            assert(parentLocationThisView != ViewFrustum::OUTSIDE); // we shouldn't be here if our parent was OUTSIDE!
            nodeLocationThisView = element->inFrustum(*params.viewFrustum);
        }

        // If we're at a element that is out of view, then we can return, because no nodes below us will be in view!
        // although technically, we really shouldn't ever be here, because our callers shouldn't be calling us if
        // we're out of view
        if (nodeLocationThisView == ViewFrustum::OUTSIDE) {
            if (params.stats) {
                params.stats->skippedOutOfView(element);
            }
            params.stopReason = EncodeBitstreamParams::OUT_OF_VIEW;
            return bytesAtThisLevel;
        }

        // Ok, we are in view, but if we're in delta mode, then we also want to make sure we weren't already in view
        // because we don't send nodes from the previously know in view frustum.
        bool wasInView = false;

        if (params.deltaViewFrustum && params.lastViewFrustum) {
            ViewFrustum::location location = element->inFrustum(*params.lastViewFrustum);

            // If we're a leaf, then either intersect or inside is considered "formerly in view"
            if (element->isLeaf()) {
                wasInView = location != ViewFrustum::OUTSIDE;
            } else {
                wasInView = location == ViewFrustum::INSIDE;
            }

            // If we were in view, double check that we didn't switch LOD visibility... namely, the was in view doesn't
            // tell us if it was so small we wouldn't have rendered it. Which may be the case. And we may have moved closer
            // to it, and so therefore it may now be visible from an LOD perspective, in which case we don't consider it
            // as "was in view"...
            if (wasInView) {
                float distance = element->distanceToCamera(*params.lastViewFrustum);
                float boundaryDistance = boundaryDistanceForRenderLevel(element->getLevel() + params.boundaryLevelAdjust,
                                                                            params.octreeElementSizeScale);
                if (distance >= boundaryDistance) {
                    // This would have been invisible... but now should be visible (we wouldn't be here otherwise)...
                    wasInView = false;
                }
            }
        }

        // If we were previously in the view, then we normally will return out of here and stop recursing. But
        // if we're in deltaViewFrustum mode, and this element has changed since it was last sent, then we do
        // need to send it.
        if (wasInView && !(params.deltaViewFrustum && element->hasChangedSince(params.lastViewFrustumSent - CHANGE_FUDGE))) {
            if (params.stats) {
                params.stats->skippedWasInView(element);
            }
            params.stopReason = EncodeBitstreamParams::WAS_IN_VIEW;
            return bytesAtThisLevel;
        }

        // If we're not in delta sending mode, and we weren't asked to do a force send, and the voxel hasn't changed,
        // then we can also bail early and save bits
        if (!params.forceSendScene && !params.deltaViewFrustum &&
            !element->hasChangedSince(params.lastViewFrustumSent - CHANGE_FUDGE)) {
            if (params.stats) {
                params.stats->skippedNoChange(element);
            }
            params.stopReason = EncodeBitstreamParams::NO_CHANGE;
            return bytesAtThisLevel;
        }

        // If the user also asked for occlusion culling, check if this element is occluded, but only if it's not a leaf.
        // leaf occlusion is handled down below when we check child nodes
        if (params.wantOcclusionCulling && !element->isLeaf()) {
            AACube voxelBox = element->getAACube();
            voxelBox.scale(TREE_SCALE);
            OctreeProjectedPolygon* voxelPolygon = new OctreeProjectedPolygon(params.viewFrustum->getProjectedPolygon(voxelBox));

            // In order to check occlusion culling, the shadow has to be "all in view" otherwise, we will ignore occlusion
            // culling and proceed as normal
            if (voxelPolygon->getAllInView()) {
                CoverageMapStorageResult result = params.map->checkMap(voxelPolygon, false);
                delete voxelPolygon; // cleanup
                if (result == OCCLUDED) {
                    if (params.stats) {
                        params.stats->skippedOccluded(element);
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
    // is 1 byte for child colors + 3*NUMBER_OF_CHILDREN bytes for the actual colors + 1 byte for child trees.
    // There could be sub trees below this point, which might take many more bytes, but that's ok, because we can
    // always mark our subtrees as not existing and stop the packet at this point, then start up with a new packet
    // for the remaining sub trees.
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
        OctreeElement* childElement = element->getChildAtIndex(i);

        // if the caller wants to include childExistsBits, then include them even if not in view, if however,
        // we're in a portion of the tree that's not our responsibility, then we assume the child nodes exist
        // even if they don't in our local tree
        bool notMyJurisdiction = false;
        if (params.jurisdictionMap) {
            notMyJurisdiction = JurisdictionMap::WITHIN != params.jurisdictionMap->isMyJurisdiction(element->getOctalCode(), i);
        }
        if (params.includeExistsBits) {
            // If the child is known to exist, OR, it's not my jurisdiction, then we mark the bit as existing
            if (childElement || notMyJurisdiction) {
                childrenExistInTreeBits += (1 << (7 - i));
            }
        }

        if (params.wantOcclusionCulling) {
            if (childElement) {
                float distance = params.viewFrustum ? childElement->distanceToCamera(*params.viewFrustum) : 0;

                currentCount = insertIntoSortedArrays((void*)childElement, distance, i,
                                                      (void**)&sortedChildren, (float*)&distancesToChildren,
                                                      (int*)&indexOfChildren, currentCount, NUMBER_OF_CHILDREN);
            }
        } else {
            sortedChildren[i] = childElement;
            indexOfChildren[i] = i;
            distancesToChildren[i] = 0.0f;
            currentCount++;
        }

        // track stats
        // must check childElement here, because it could be we got here with no childElement
        if (params.stats && childElement) {
            params.stats->traversed(childElement);
        }
    }

    // for each child element in Distance sorted order..., check to see if they exist, are colored, and in view, and if so
    // add them to our distance ordered array of children
    for (int i = 0; i < currentCount; i++) {
        OctreeElement* childElement = sortedChildren[i];
        int originalIndex = indexOfChildren[i];

        bool childIsInView  = (childElement && 
                ( !params.viewFrustum || // no view frustum was given, everything is assumed in view
                  (nodeLocationThisView == ViewFrustum::INSIDE) || // parent was fully in view, we can assume ALL children are
                  (nodeLocationThisView == ViewFrustum::INTERSECT && 
                        childElement->isInView(*params.viewFrustum)) // the parent intersects and the child is in view
                ));

        if (!childIsInView) {
            // must check childElement here, because it could be we got here because there was no childElement
            if (params.stats && childElement) {
                params.stats->skippedOutOfView(childElement);
            }
        } else {
            // Before we determine consider this further, let's see if it's in our LOD scope...
            float distance = distancesToChildren[i];
            float boundaryDistance = !params.viewFrustum ? 1 :
                                     boundaryDistanceForRenderLevel(childElement->getLevel() + params.boundaryLevelAdjust,
                                            params.octreeElementSizeScale);

            if (!(distance < boundaryDistance)) {
                // don't need to check childElement here, because we can't get here with no childElement
                if (params.stats) {
                    params.stats->skippedDistance(childElement);
                }
            } else {
                inViewCount++;

                // track children in view as existing and not a leaf, if they're a leaf,
                // we don't care about recursing deeper on them, and we don't consider their
                // subtree to exist
                if (!(childElement && childElement->isLeaf())) {
                    childrenExistInPacketBits += (1 << (7 - originalIndex));
                    inViewNotLeafCount++;
                }

                bool childIsOccluded = false; // assume it's not occluded

                // If the user also asked for occlusion culling, check if this element is occluded
                if (params.wantOcclusionCulling && childElement->isLeaf()) {
                    // Don't check occlusion here, just add them to our distance ordered array...

                    AACube voxelBox = childElement->getAACube();
                    voxelBox.scale(TREE_SCALE);
                    OctreeProjectedPolygon* voxelPolygon = new OctreeProjectedPolygon(
                                params.viewFrustum->getProjectedPolygon(voxelBox));

                    // In order to check occlusion culling, the shadow has to be "all in view" otherwise, we ignore occlusion
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
                                    : childElement->calculateShouldRender(params.viewFrustum,
                                                    params.octreeElementSizeScale, params.boundaryLevelAdjust);

                // track some stats
                if (params.stats) {
                    // don't need to check childElement here, because we can't get here with no childElement
                    if (!shouldRender && childElement->isLeaf()) {
                        params.stats->skippedDistance(childElement);
                    }
                    // don't need to check childElement here, because we can't get here with no childElement
                    if (childIsOccluded) {
                        params.stats->skippedOccluded(childElement);
                    }
                }

                // track children with actual color, only if the child wasn't previously in view!
                if (shouldRender && !childIsOccluded) {
                    bool childWasInView = false;

                    if (childElement && params.deltaViewFrustum && params.lastViewFrustum) {
                        ViewFrustum::location location = childElement->inFrustum(*params.lastViewFrustum);

                        // If we're a leaf, then either intersect or inside is considered "formerly in view"
                        if (childElement->isLeaf()) {
                            childWasInView = location != ViewFrustum::OUTSIDE;
                        } else {
                            childWasInView = location == ViewFrustum::INSIDE;
                        }
                    }

                    // If our child wasn't in view (or we're ignoring wasInView) then we add it to our sending items.
                    // Or if we were previously in the view, but this element has changed since it was last sent, then we do
                    // need to send it.
                    if (!childWasInView ||
                        (params.deltaViewFrustum &&
                         childElement->hasChangedSince(params.lastViewFrustumSent - CHANGE_FUDGE))){

                        childrenColoredBits += (1 << (7 - originalIndex));
                        inViewWithColorCount++;
                    } else {
                        // otherwise just track stats of the items we discarded
                        // don't need to check childElement here, because we can't get here with no childElement
                        if (params.stats) {
                            if (childWasInView) {
                                params.stats->skippedWasInView(childElement);
                            } else {
                                params.stats->skippedNoChange(childElement);
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

    // write the child element data...
    if (continueThisLevel && params.includeColor) {
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            if (oneAtBit(childrenColoredBits, i)) {
                OctreeElement* childElement = element->getChildAtIndex(i);
                if (childElement) {
                
                    int bytesBeforeChild = packetData->getUncompressedSize();
                    
                    // TODO: we want to support the ability for a childElement to "partially" write it's data.
                    //       for example, consider the case of the model server where the entire contents of the
                    //       element may be larger than can fit in a single MTU/packetData. In this case, we want
                    //       to allow the appendElementData() to respond that it produced partial data, which should be
                    //       written, but that the childElement needs to be reprocessed in an additional pass or passes
                    //       to be completed. In the case that an element was partially written, we need to 
                    continueThisLevel = childElement->appendElementData(packetData, params);
                    
                    int bytesAfterChild = packetData->getUncompressedSize();

                    if (!continueThisLevel) {
                        break; // no point in continuing
                    }

                    bytesAtThisLevel += (bytesAfterChild - bytesBeforeChild); // keep track of byte count for this child

                    // don't need to check childElement here, because we can't get here with no childElement
                    if (params.stats) {
                        params.stats->colorSent(childElement);
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

        // for each child element in Distance sorted order..., check to see if they exist, are colored, and in view, and if so
        // add them to our distance ordered array of children
        for (int indexByDistance = 0; indexByDistance < currentCount; indexByDistance++) {
            OctreeElement* childElement = sortedChildren[indexByDistance];
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
                //
                // NOTE: some octree styles (like models and particles) will store content in parent elements, and child
                // elements. In this case, if we stop recursion when we include any data (the colorbits should really be
                // called databits), then we wouldn't send the children. So those types of Octree's should tell us to keep
                // recursing, by returning TRUE in recurseChildrenWithData().
                if (recurseChildrenWithData() || !params.viewFrustum || !oneAtBit(childrenColoredBits, originalIndex)) {
                    childTreeBytesOut = encodeTreeBitstreamRecursion(childElement, packetData, bag, params, 
                                                                            thisLevel, nodeLocationThisView);
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
                //          and at least a color's worth of bytes for the element of colors.
                //   if it had child trees (with something in them) then it would have the 1 byte for child mask
                //         and some number of bytes of lower children...
                // so, if the child returns 2 bytes out, we can actually consider that an empty tree also!!
                //
                // we can make this act like no bytes out, by just resetting the bytes out in this case
                if (params.includeColor && !params.includeExistsBits && childTreeBytesOut == 2) {
                    childTreeBytesOut = 0; // this is the degenerate case of a tree with no colors and no child trees
                }
                // We used to try to collapse trees that didn't contain any data, but this does appear to create a problem
                // in detecting element deletion. So, I've commented this out but left it in here as a warning to anyone else
                // about not attempting to add this optimization back in, without solving the element deletion case.
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

    // If we made it this far, then we've written all of our child data... if this element is the root
    // element, then we also allow the root element to write out it's data...
    if (continueThisLevel && element == _rootElement && rootElementHasData()) {
        int bytesBeforeChild = packetData->getUncompressedSize();
        continueThisLevel = element->appendElementData(packetData, params);
        int bytesAfterChild = packetData->getUncompressedSize();

        if (continueThisLevel) {
            bytesAtThisLevel += (bytesAfterChild - bytesBeforeChild); // keep track of byte count for this child

            if (params.stats) {
                params.stats->colorSent(element);
            }
        }
    }

    // if we were unable to fit this level in our packet, then rewind and add it to the element bag for
    // sending later...
    if (continueThisLevel) {
        continueThisLevel = packetData->endLevel(thisLevelKey);
    } else {
        packetData->discardLevel(thisLevelKey);
    }

    if (!continueThisLevel) {
        bag.insert(element);

        // don't need to check element here, because we can't get here with no element
        if (params.stats) {
            params.stats->didntFit(element);
        }

        params.stopReason = EncodeBitstreamParams::DIDNT_FIT;
        bytesAtThisLevel = 0; // didn't fit
    }

    return bytesAtThisLevel;
}

bool Octree::readFromSVOFile(const char* fileName) {
    bool fileOk = false;
    PacketVersion gotVersion = 0;
    std::ifstream file(fileName, std::ios::in|std::ios::binary|std::ios::ate);
    if(file.is_open()) {
        emit importSize(1.0f, 1.0f, 1.0f);
        emit importProgress(0);

        qDebug("Loading file %s...", fileName);

        // get file length....
        unsigned long fileLength = file.tellg();
        file.seekg( 0, std::ios::beg );

        // read the entire file into a buffer, WHAT!? Why not.
        unsigned char* entireFile = new unsigned char[fileLength];
        file.read((char*)entireFile, fileLength);
        bool wantImportProgress = true;

        unsigned char* dataAt = entireFile;
        unsigned long  dataLength = fileLength;

        // before reading the file, check to see if this version of the Octree supports file versions
        if (getWantSVOfileVersions()) {
            // if so, read the first byte of the file and see if it matches the expected version code
            PacketType expectedType = expectedDataPacketType();
            
            PacketType gotType;
            memcpy(&gotType, dataAt, sizeof(gotType));
            
            if (gotType == expectedType) {
                dataAt += sizeof(expectedType);
                dataLength -= sizeof(expectedType);
                gotVersion = *dataAt;
                if (canProcessVersion(gotVersion)) {
                    dataAt += sizeof(gotVersion);
                    dataLength -= sizeof(gotVersion);
                    fileOk = true;
                    qDebug("SVO file version match. Expected: %d Got: %d", 
                                versionForPacketType(expectedDataPacketType()), gotVersion);
                } else {
                    qDebug("SVO file version mismatch. Expected: %d Got: %d", 
                                versionForPacketType(expectedDataPacketType()), gotVersion);
                }
            } else {
                qDebug("SVO file type mismatch. Expected: %c Got: %c", expectedType, gotType);
            }
        } else {
            fileOk = true; // assume the file is ok
        }
        if (fileOk) {
            ReadBitstreamToTreeParams args(WANT_COLOR, NO_EXISTS_BITS, NULL, 0, 
                                                SharedNodePointer(), wantImportProgress, gotVersion);
            readBitstreamToTree(dataAt, dataLength, args);
        }
        delete[] entireFile;

        emit importProgress(100);

        file.close();
    }
    return fileOk;
}

void Octree::writeToSVOFile(const char* fileName, OctreeElement* element) {
    std::ofstream file(fileName, std::ios::out|std::ios::binary);

    if(file.is_open()) {
        qDebug("Saving to file %s...", fileName);

        // before reading the file, check to see if this version of the Octree supports file versions
        if (getWantSVOfileVersions()) {
            // if so, read the first byte of the file and see if it matches the expected version code
            PacketType expectedType = expectedDataPacketType();
            PacketVersion expectedVersion = versionForPacketType(expectedType);
            file.write(reinterpret_cast<char*>(&expectedType), sizeof(expectedType));
            file.write(&expectedVersion, sizeof(expectedVersion));
        }

        OctreeElementBag nodeBag;
        // If we were given a specific element, start from there, otherwise start from root
        if (element) {
            nodeBag.insert(element);
        } else {
            nodeBag.insert(_rootElement);
        }

        OctreePacketData packetData;
        int bytesWritten = 0;
        bool lastPacketWritten = false;

        while (!nodeBag.isEmpty()) {
            OctreeElement* subTree = nodeBag.extract();
            lockForRead(); // do tree locking down here so that we have shorter slices and less thread contention
            EncodeBitstreamParams params(INT_MAX, IGNORE_VIEW_FRUSTUM, WANT_COLOR, NO_EXISTS_BITS);
            bytesWritten = encodeTreeBitstream(subTree, &packetData, nodeBag, params);
            unlock();

            // if the subTree couldn't fit, and so we should reset the packet and reinsert the element in our bag and try again
            if (bytesWritten == 0 && (params.stopReason == EncodeBitstreamParams::DIDNT_FIT)) {
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

bool Octree::countOctreeElementsOperation(OctreeElement* element, void* extraData) {
    (*(unsigned long*)extraData)++;
    return true; // keep going
}

void Octree::copySubTreeIntoNewTree(OctreeElement* startElement, Octree* destinationTree, bool rebaseToRoot) {
    OctreeElementBag nodeBag;
    nodeBag.insert(startElement);
    int chopLevels = 0;
    if (rebaseToRoot) {
        chopLevels = numberOfThreeBitSectionsInCode(startElement->getOctalCode());
    }

    static OctreePacketData packetData;

    while (!nodeBag.isEmpty()) {
        OctreeElement* subTree = nodeBag.extract();
        packetData.reset(); // reset the packet between usage
        // ask our tree to write a bitsteam
        EncodeBitstreamParams params(INT_MAX, IGNORE_VIEW_FRUSTUM, WANT_COLOR, NO_EXISTS_BITS, chopLevels);
        encodeTreeBitstream(subTree, &packetData, nodeBag, params);
        // ask destination tree to read the bitstream
        ReadBitstreamToTreeParams args(WANT_COLOR, NO_EXISTS_BITS);
        destinationTree->readBitstreamToTree(packetData.getUncompressedData(), packetData.getUncompressedSize(), args);
    }
}

void Octree::copyFromTreeIntoSubTree(Octree* sourceTree, OctreeElement* destinationElement) {
    OctreeElementBag nodeBag;
    // If we were given a specific element, start from there, otherwise start from root
    nodeBag.insert(sourceTree->_rootElement);

    static OctreePacketData packetData;

    while (!nodeBag.isEmpty()) {
        OctreeElement* subTree = nodeBag.extract();

        packetData.reset(); // reset between usage

        // ask our tree to write a bitsteam
        EncodeBitstreamParams params(INT_MAX, IGNORE_VIEW_FRUSTUM, WANT_COLOR, NO_EXISTS_BITS);
        sourceTree->encodeTreeBitstream(subTree, &packetData, nodeBag, params);

        // ask destination tree to read the bitstream
        bool wantImportProgress = true;
        ReadBitstreamToTreeParams args(WANT_COLOR, NO_EXISTS_BITS, destinationElement, 
                                            0, SharedNodePointer(), wantImportProgress);
        readBitstreamToTree(packetData.getUncompressedData(), packetData.getUncompressedSize(), args);
    }
}

void Octree::cancelImport() {
    _stopImport = true;
}

