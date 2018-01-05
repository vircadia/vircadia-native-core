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

#include <cstring>
#include <cstdio>
#include <cmath>
#include <fstream> // to load voxels from file

#include <QDataStream>
#include <QDebug>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QVector>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QString>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include <GeometryUtil.h>
#include <Gzip.h>
#include <LogHandler.h>
#include <NetworkAccessManager.h>
#include <OctalCode.h>
#include <udt/PacketHeaders.h>
#include <ResourceManager.h>
#include <SharedUtil.h>
#include <PathUtils.h>
#include <ViewFrustum.h>

#include "Octree.h"
#include "OctreeConstants.h"
#include "OctreeElementBag.h"
#include "OctreeLogging.h"
#include "OctreeQueryNode.h"
#include "OctreeUtils.h"


QVector<QString> PERSIST_EXTENSIONS = {"json", "json.gz"};

Octree::Octree(bool shouldReaverage) :
    _rootElement(NULL),
    _isDirty(true),
    _shouldReaverage(shouldReaverage),
    _stopImport(false),
    _isViewing(false),
    _isServer(false)
{
}

Octree::~Octree() {
    // This will delete all children, don't create a new root in this case.
    eraseAllOctreeElements(false);
}


// Inserts the value and key into three arrays sorted by the key array, the first array is the value,
// the second array is a sorted key for the value, the third array is the index for the value in it original
// non-sorted array
// returns -1 if size exceeded
// originalIndexArray is optional
int insertOctreeElementIntoSortedArrays(const OctreeElementPointer& value, float key, int originalIndex,
                                        OctreeElementPointer* valueArray, float* keyArray, int* originalIndexArray,
                                        int currentCount, int maxCount) {

    if (currentCount < maxCount) {
        int i = 0;
        if (currentCount > 0) {
            while (i < currentCount && key > keyArray[i]) {
                i++;
            }
            // i is our desired location
            // shift array elements to the right
            if (i < currentCount && i+1 < maxCount) {
                for (int j = currentCount - 1; j > i; j--) {
                    valueArray[j] = valueArray[j - 1];
                    keyArray[j] = keyArray[j - 1];
                }
            }
        }
        // place new element at i
        valueArray[i] = value;
        keyArray[i] = key;
        if (originalIndexArray) {
            originalIndexArray[i] = originalIndex;
        }
        return currentCount + 1;
    }
    return -1; // error case
}



// Recurses voxel tree calling the RecurseOctreeOperation function for each element.
// stops recursion if operation function returns false.
void Octree::recurseTreeWithOperation(const RecurseOctreeOperation& operation, void* extraData) {
    recurseElementWithOperation(_rootElement, operation, extraData);
}

// Recurses voxel tree calling the RecurseOctreePostFixOperation function for each element in post-fix order.
void Octree::recurseTreeWithPostOperation(const RecurseOctreeOperation& operation, void* extraData) {
    recurseElementWithPostOperation(_rootElement, operation, extraData);
}

// Recurses voxel element with an operation function
void Octree::recurseElementWithOperation(const OctreeElementPointer& element, const RecurseOctreeOperation& operation, void* extraData,
                        int recursionCount) {
    if (recursionCount > DANGEROUSLY_DEEP_RECURSION) {
        static QString repeatedMessage
            = LogHandler::getInstance().addRepeatedMessageRegex(
                    "Octree::recurseElementWithOperation\\(\\) reached DANGEROUSLY_DEEP_RECURSION, bailing!");

        qCDebug(octree) << "Octree::recurseElementWithOperation() reached DANGEROUSLY_DEEP_RECURSION, bailing!";
        return;
    }

    if (operation(element, extraData)) {
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            OctreeElementPointer child = element->getChildAtIndex(i);
            if (child) {
                recurseElementWithOperation(child, operation, extraData, recursionCount+1);
            }
        }
    }
}

// Recurses voxel element with an operation function
void Octree::recurseElementWithPostOperation(const OctreeElementPointer& element, const RecurseOctreeOperation& operation,
                                             void* extraData, int recursionCount) {
    if (recursionCount > DANGEROUSLY_DEEP_RECURSION) {
        static QString repeatedMessage
            = LogHandler::getInstance().addRepeatedMessageRegex(
                    "Octree::recurseElementWithPostOperation\\(\\) reached DANGEROUSLY_DEEP_RECURSION, bailing!");

        qCDebug(octree) << "Octree::recurseElementWithPostOperation() reached DANGEROUSLY_DEEP_RECURSION, bailing!";
        return;
    }

    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        OctreeElementPointer child = element->getChildAtIndex(i);
        if (child) {
            recurseElementWithPostOperation(child, operation, extraData, recursionCount+1);
        }
    }
    operation(element, extraData);
}

// Recurses voxel tree calling the RecurseOctreeOperation function for each element.
// stops recursion if operation function returns false.
void Octree::recurseTreeWithOperationDistanceSorted(const RecurseOctreeOperation& operation,
                                                       const glm::vec3& point, void* extraData) {

    recurseElementWithOperationDistanceSorted(_rootElement, operation, point, extraData);
}

// Recurses voxel element with an operation function
void Octree::recurseElementWithOperationDistanceSorted(const OctreeElementPointer& element, const RecurseOctreeOperation& operation,
                                                       const glm::vec3& point, void* extraData, int recursionCount) {

    if (recursionCount > DANGEROUSLY_DEEP_RECURSION) {
        static QString repeatedMessage
            = LogHandler::getInstance().addRepeatedMessageRegex(
                    "Octree::recurseElementWithOperationDistanceSorted\\(\\) reached DANGEROUSLY_DEEP_RECURSION, bailing!");

        qCDebug(octree) << "Octree::recurseElementWithOperationDistanceSorted() reached DANGEROUSLY_DEEP_RECURSION, bailing!";
        return;
    }

    if (operation(element, extraData)) {
        // determine the distance sorted order of our children
        OctreeElementPointer sortedChildren[NUMBER_OF_CHILDREN] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
        float distancesToChildren[NUMBER_OF_CHILDREN] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        int indexOfChildren[NUMBER_OF_CHILDREN] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        int currentCount = 0;

        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            OctreeElementPointer childElement = element->getChildAtIndex(i);
            if (childElement) {
                // chance to optimize, doesn't need to be actual distance!! Could be distance squared
                float distanceSquared = childElement->distanceSquareToPoint(point);
                currentCount = insertOctreeElementIntoSortedArrays(childElement, distanceSquared, i,
                                                                   sortedChildren, (float*)&distancesToChildren,
                                                                   (int*)&indexOfChildren, currentCount, NUMBER_OF_CHILDREN);
            }
        }

        for (int i = 0; i < currentCount; i++) {
            OctreeElementPointer childElement = sortedChildren[i];
            if (childElement) {
                recurseElementWithOperationDistanceSorted(childElement, operation, point, extraData);
            }
        }
    }
}

void Octree::recurseTreeWithOperator(RecurseOctreeOperator* operatorObject) {
    recurseElementWithOperator(_rootElement, operatorObject);
}

bool Octree::recurseElementWithOperator(const OctreeElementPointer& element,
                                        RecurseOctreeOperator* operatorObject, int recursionCount) {
    if (recursionCount > DANGEROUSLY_DEEP_RECURSION) {
        static QString repeatedMessage
            = LogHandler::getInstance().addRepeatedMessageRegex(
                    "Octree::recurseElementWithOperator\\(\\) reached DANGEROUSLY_DEEP_RECURSION, bailing!");

        qCDebug(octree) << "Octree::recurseElementWithOperator() reached DANGEROUSLY_DEEP_RECURSION, bailing!";
        return false;
    }

    if (operatorObject->preRecursion(element)) {
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            OctreeElementPointer child = element->getChildAtIndex(i);

            // If there is no child at that location, the Operator may want to create a child at that location.
            // So give the operator a chance to do so....
            if (!child) {
                child = operatorObject->possiblyCreateChildAt(element, i);
            }

            if (child) {
                if (!recurseElementWithOperator(child, operatorObject, recursionCount + 1)) {
                    break; // stop recursing if operator returns false...
                }
            }
        }
    }

    return operatorObject->postRecursion(element);
}


OctreeElementPointer Octree::nodeForOctalCode(const OctreeElementPointer& ancestorElement, const unsigned char* needleCode,
                                              OctreeElementPointer* parentOfFoundElement) const {
    // special case for NULL octcode
    if (!needleCode) {
        return _rootElement;
    }

    // find the appropriate branch index based on this ancestorElement
    if (*needleCode > 0) {
        int branchForNeedle = branchIndexWithDescendant(ancestorElement->getOctalCode(), needleCode);
        OctreeElementPointer childElement = ancestorElement->getChildAtIndex(branchForNeedle);

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
OctreeElementPointer Octree::createMissingElement(const OctreeElementPointer& lastParentElement,
                                                  const unsigned char* codeToReach, int recursionCount) {

    if (recursionCount > DANGEROUSLY_DEEP_RECURSION) {
        static QString repeatedMessage
            = LogHandler::getInstance().addRepeatedMessageRegex(
                    "Octree::createMissingElement\\(\\) reached DANGEROUSLY_DEEP_RECURSION, bailing!");

        qCDebug(octree) << "Octree::createMissingElement() reached DANGEROUSLY_DEEP_RECURSION, bailing!";
        return lastParentElement;
    }
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
        return createMissingElement(lastParentElement->getChildAtIndex(indexOfNewChild), codeToReach, recursionCount + 1);
    }
}

int Octree::readElementData(const OctreeElementPointer& destinationElement, const unsigned char* nodeData, int bytesAvailable,
                            ReadBitstreamToTreeParams& args) {

    int bytesLeftToRead = bytesAvailable;
    int bytesRead = 0;

    // give this destination element the child mask from the packet
    const unsigned char ALL_CHILDREN_ASSUMED_TO_EXIST = 0xFF;

    if ((size_t)bytesLeftToRead < sizeof(unsigned char)) {
        qCDebug(octree) << "UNEXPECTED: readElementData() only had " << bytesLeftToRead << " bytes. "
                    "Not enough for meaningful data.";
        return bytesAvailable; // assume we read the entire buffer...
    }

    if (destinationElement->getScale() < SCALE_AT_DANGEROUSLY_DEEP_RECURSION) {
        qCDebug(octree) << "UNEXPECTED: readElementData() destination element is unreasonably small ["
                << destinationElement->getScale() << " meters] "
                << " Discarding " << bytesAvailable << " remaining bytes.";
        return bytesAvailable; // assume we read the entire buffer...
    }

    unsigned char colorInPacketMask = *nodeData;
    bytesRead += sizeof(colorInPacketMask);
    bytesLeftToRead -= sizeof(colorInPacketMask);

    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        // check the colors mask to see if we have a child to color in
        if (oneAtBit(colorInPacketMask, i)) {
            // addChildAtIndex() should actually be called getOrAddChildAtIndex().
            // When it adds the child it automatically sets the detinationElement dirty.
            OctreeElementPointer childElementAt = destinationElement->addChildAtIndex(i);

            int childElementDataRead = childElementAt->readElementDataFromBuffer(nodeData + bytesRead, bytesLeftToRead, args);
            childElementAt->setSourceUUID(args.sourceUUID);

            bytesRead += childElementDataRead;
            bytesLeftToRead -= childElementDataRead;

            // It's possible that we already had this version of element data, in which case the unpacking of the data
            // wouldn't flag childElementAt as dirty, so we manually flag it here... if the element is to be rendered.
            if (childElementAt->getShouldRender() && !childElementAt->isRendered()) {
                childElementAt->setDirtyBit(); // force dirty!
                _isDirty = true;
            }
        }
        if (destinationElement->isDirty()) {
            _isDirty = true;
        }
    }

    unsigned char childrenInTreeMask = ALL_CHILDREN_ASSUMED_TO_EXIST;
    unsigned char childInBufferMask = 0;
    int bytesForMasks = args.includeExistsBits ? sizeof(childrenInTreeMask) + sizeof(childInBufferMask)
                                                : sizeof(childInBufferMask);

    if (bytesLeftToRead < bytesForMasks) {
        if (bytesLeftToRead > 0) {
            qCDebug(octree) << "UNEXPECTED: readElementDataFromBuffer() only had " << bytesLeftToRead << " bytes before masks. "
                        "Not enough for meaningful data.";
        }
        return bytesAvailable; // assume we read the entire buffer...
    }

    childrenInTreeMask = args.includeExistsBits ? *(nodeData + bytesRead) : ALL_CHILDREN_ASSUMED_TO_EXIST;
    childInBufferMask = *(nodeData + bytesRead + (args.includeExistsBits ? sizeof(childrenInTreeMask) : 0));

    int childIndex = 0;
    bytesRead += bytesForMasks;
    bytesLeftToRead -= bytesForMasks;

    while (bytesLeftToRead > 0 && childIndex < NUMBER_OF_CHILDREN) {
        // check the exists mask to see if we have a child to traverse into

        if (oneAtBit(childInBufferMask, childIndex)) {
            auto childAt = destinationElement->getChildAtIndex(childIndex);
            if (!childAt) {
                // add a child at that index, if it doesn't exist
                childAt = destinationElement->addChildAtIndex(childIndex);
                bool nodeIsDirty = destinationElement->isDirty();
                if (nodeIsDirty) {
                    _isDirty = true;
                }
            }

            // tell the child to read the subsequent data
            int lowerLevelBytes = readElementData(childAt, nodeData + bytesRead, bytesLeftToRead, args);

            bytesRead += lowerLevelBytes;
            bytesLeftToRead -= lowerLevelBytes;
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
    if (destinationElement == _rootElement  && rootElementHasData() && bytesLeftToRead > 0) {
        // tell the element to read the subsequent data
        int rootDataSize = _rootElement->readElementDataFromBuffer(nodeData + bytesRead, bytesLeftToRead, args);
        bytesRead += rootDataSize;
        bytesLeftToRead -= rootDataSize;
    }

    return bytesRead;
}

void Octree::readBitstreamToTree(const unsigned char * bitstream, uint64_t bufferSizeBytes,
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
        OctreeElementPointer bitstreamRootElement = nodeForOctalCode(args.destinationElement,
                                                                     (unsigned char *)bitstreamAt, NULL);
        int numberOfThreeBitSectionsInStream = numberOfThreeBitSectionsInCode(bitstreamAt, bufferSizeBytes);
        if (numberOfThreeBitSectionsInStream > UNREASONABLY_DEEP_RECURSION) {
            static QString repeatedMessage
                = LogHandler::getInstance().addRepeatedMessageRegex(
                        "UNEXPECTED: parsing of the octal code would make UNREASONABLY_DEEP_RECURSION... "
                        "numberOfThreeBitSectionsInStream: \\d+ This buffer is corrupt. Returning."
                    );


            qCDebug(octree) << "UNEXPECTED: parsing of the octal code would make UNREASONABLY_DEEP_RECURSION... "
                        "numberOfThreeBitSectionsInStream:" << numberOfThreeBitSectionsInStream <<
                        "This buffer is corrupt. Returning.";
            return;
        }

        if (numberOfThreeBitSectionsInStream == OVERFLOWED_OCTCODE_BUFFER) {
            qCDebug(octree) << "UNEXPECTED: parsing of the octal code would overflow the buffer. "
                        "This buffer is corrupt. Returning.";
            return;
        }

        int numberOfThreeBitSectionsFromNode = numberOfThreeBitSectionsInCode(bitstreamRootElement->getOctalCode());

        // if the octal code returned is not on the same level as the code being searched for, we have OctreeElements to create
        if (numberOfThreeBitSectionsInStream != numberOfThreeBitSectionsFromNode) {

            // Note: we need to create this element relative to root, because we're assuming that the bitstream for the initial
            // octal code is always relative to root!
            bitstreamRootElement = createMissingElement(args.destinationElement, (unsigned char*) bitstreamAt);
            if (bitstreamRootElement->isDirty()) {
                _isDirty = true;
            }
        }

        auto octalCodeBytes = bytesRequiredForCodeLength(numberOfThreeBitSectionsInStream);

        int theseBytesRead = 0;
        theseBytesRead += (int)octalCodeBytes;
        int lowerLevelBytes = readElementData(bitstreamRootElement, bitstreamAt + octalCodeBytes,
                                       bufferSizeBytes - (bytesRead + (int)octalCodeBytes), args);

        theseBytesRead += lowerLevelBytes;

        // skip bitstream to new startPoint
        bitstreamAt += theseBytesRead;
        bytesRead += theseBytesRead;

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

    withWriteLock([&] {
        deleteOctalCodeFromTreeRecursion(_rootElement, &args);
    });
}

void Octree::deleteOctalCodeFromTreeRecursion(const OctreeElementPointer& element, void* extraData) {
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
    OctreeElementPointer childElement = element->getChildAtIndex(childIndex);

    // If there is no child at the target location, and the current parent element is a colored leaf,
    // then it means we were asked to delete a child out of a larger leaf voxel.
    // We support this by breaking up the parent voxel into smaller pieces.
    if (!childElement && element->requiresSplit()) {
        // we need to break up ancestors until we get to the right level
        OctreeElementPointer ancestorElement = element;
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
        element->handleSubtreeChanged(shared_from_this());
    }
}

void Octree::eraseAllOctreeElements(bool createNewRoot) {
    if (createNewRoot) {
        _rootElement = createNewElement();
    } else {
        _rootElement.reset(); // this will recurse and delete all children
    }

    _isDirty = true;
}

// Note: this is an expensive call. Don't call it unless you really need to reaverage the entire tree (from startElement)
void Octree::reaverageOctreeElements(OctreeElementPointer startElement) {
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
            qCDebug(octree, "Octree::reaverageOctreeElements()... bailing out of UNREASONABLY_DEEP_RECURSION");
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

OctreeElementPointer Octree::getOctreeElementAt(float x, float y, float z, float s) const {
    unsigned char* octalCode = pointToOctalCode(x,y,z,s);
    OctreeElementPointer element = nodeForOctalCode(_rootElement, octalCode, NULL);
    if (*element->getOctalCode() != *octalCode) {
        element = NULL;
    }
    delete[] octalCode; // cleanup memory
    return element;
}

OctreeElementPointer Octree::getOctreeEnclosingElementAt(float x, float y, float z, float s) const {
    unsigned char* octalCode = pointToOctalCode(x,y,z,s);
    OctreeElementPointer element = nodeForOctalCode(_rootElement, octalCode, NULL);

    delete[] octalCode; // cleanup memory
    return element;
}


OctreeElementPointer Octree::getOrCreateChildElementAt(float x, float y, float z, float s) {
    return getRoot()->getOrCreateChildElementAt(x, y, z, s);
}

OctreeElementPointer Octree::getOrCreateChildElementContaining(const AACube& box) {
    return getRoot()->getOrCreateChildElementContaining(box);
}



class SphereArgs {
public:
    glm::vec3 center;
    float radius;
    glm::vec3& penetration;
    bool found;
    void* penetratedObject; /// the type is defined by the type of Octree, the caller is assumed to know the type
};

bool findSpherePenetrationOp(const OctreeElementPointer& element, void* extraData) {
    SphereArgs* args = static_cast<SphereArgs*>(extraData);

    // coarse check against bounds
    if (!element->getAACube().expandedContains(args->center, args->radius)) {
        return false;
    }
    if (element->hasContent()) {
        glm::vec3 elementPenetration;
        if (element->findSpherePenetration(args->center, args->radius, elementPenetration, &args->penetratedObject)) {
            // NOTE: it is possible for this penetration accumulation algorithm to produce a
            // final penetration vector with zero length.
            args->penetration = addPenetrations(args->penetration, elementPenetration);
            args->found = true;
        }
    }
    if (!element->isLeaf()) {
        return true; // recurse on children
    }
    return false;
}

bool Octree::findSpherePenetration(const glm::vec3& center, float radius, glm::vec3& penetration,
                    void** penetratedObject, Octree::lockType lockType, bool* accurateResult) {

    SphereArgs args = {
        center,
        radius,
        penetration,
        false,
        NULL };
    penetration = glm::vec3(0.0f, 0.0f, 0.0f);

    bool requireLock = lockType == Octree::Lock;
    bool lockResult = withReadLock([&]{
        recurseTreeWithOperation(findSpherePenetrationOp, &args);
        if (penetratedObject) {
            *penetratedObject = args.penetratedObject;
        }
    }, requireLock);

    if (accurateResult) {
        *accurateResult = lockResult; // if user asked to accuracy or result, let them know this is accurate
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

class ContentArgs {
public:
    AACube cube;
    CubeList* cubes;
};

bool findCapsulePenetrationOp(const OctreeElementPointer& element, void* extraData) {
    CapsuleArgs* args = static_cast<CapsuleArgs*>(extraData);

    // coarse check against bounds
    if (!element->getAACube().expandedIntersectsSegment(args->start, args->end, args->radius)) {
        return false;
    }
    if (element->hasContent()) {
        glm::vec3 nodePenetration;
        if (element->getAACube().findCapsulePenetration(args->start, args->end, args->radius, nodePenetration)) {
            args->penetration = addPenetrations(args->penetration, nodePenetration);
            args->found = true;
        }
    }
    if (!element->isLeaf()) {
        return true; // recurse on children
    }
    return false;
}

uint qHash(const glm::vec3& point) {
    // NOTE: TREE_SCALE = 16384 (15 bits) and multiplier is 1024 (11 bits),
    // so each component (26 bits) uses more than its alloted 21 bits.
    // however we don't expect to span huge cubes so it is ok if we wrap
    // (every 2^21 / 2^10 = 2048 meters).
    const uint BITS_PER_COMPONENT = 21;
    const quint64 MAX_SCALED_COMPONENT = 2097152; // 2^21
    const float RESOLUTION_PER_METER = 1024.0f; // 2^10
    return qHash((quint64)(point.x * RESOLUTION_PER_METER) % MAX_SCALED_COMPONENT +
        (((quint64)(point.y * RESOLUTION_PER_METER)) % MAX_SCALED_COMPONENT << BITS_PER_COMPONENT) +
        (((quint64)(point.z * RESOLUTION_PER_METER)) % MAX_SCALED_COMPONENT << 2 * BITS_PER_COMPONENT));
}

bool findContentInCubeOp(const OctreeElementPointer& element, void* extraData) {
    ContentArgs* args = static_cast<ContentArgs*>(extraData);

    // coarse check against bounds
    const AACube& cube = element->getAACube();
    if (!cube.touches(args->cube)) {
        return false;
    }
    if (!element->isLeaf()) {
        return true; // recurse on children
    }
    if (element->hasContent()) {
        // NOTE: the voxel's center is unique so we use it as the input for the key.
        // We use the qHash(glm::vec()) as the key as an optimization for the code that uses CubeLists.
        args->cubes->insert(qHash(cube.calcCenter()), cube);
        return true;
    }
    return false;
}

bool Octree::findCapsulePenetration(const glm::vec3& start, const glm::vec3& end, float radius,
                    glm::vec3& penetration, Octree::lockType lockType, bool* accurateResult) {

    CapsuleArgs args = { start, end, radius, penetration, false };
    penetration = glm::vec3(0.0f, 0.0f, 0.0f);


    bool requireLock = lockType == Octree::Lock;
    bool lockResult = withReadLock([&]{
        recurseTreeWithOperation(findCapsulePenetrationOp, &args);
    }, requireLock);

    if (accurateResult) {
        *accurateResult = lockResult; // if user asked to accuracy or result, let them know this is accurate
    }

    return args.found;
}

bool Octree::findContentInCube(const AACube& cube, CubeList& cubes) {
    return withTryReadLock([&]{
        ContentArgs args = { cube, &cubes };
        recurseTreeWithOperation(findContentInCubeOp, &args);
    });
}

class GetElementEnclosingArgs {
public:
    OctreeElementPointer element;
    glm::vec3 point;
};

// Find the smallest colored voxel enclosing a point (if there is one)
bool getElementEnclosingOperation(const OctreeElementPointer& element, void* extraData) {
    GetElementEnclosingArgs* args = static_cast<GetElementEnclosingArgs*>(extraData);
    if (element->getAACube().contains(args->point)) {
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

OctreeElementPointer Octree::getElementEnclosingPoint(const glm::vec3& point, Octree::lockType lockType, bool* accurateResult) {
    GetElementEnclosingArgs args;
    args.point = point;
    args.element = NULL;

    bool requireLock = lockType == Octree::Lock;
    bool lockResult = withReadLock([&]{
        recurseTreeWithOperation(getElementEnclosingOperation, (void*)&args);
    }, requireLock);

    if (accurateResult) {
        *accurateResult = lockResult; // if user asked to accuracy or result, let them know this is accurate
    }

    return args.element;
}



int Octree::encodeTreeBitstream(const OctreeElementPointer& element,
                                OctreePacketData* packetData, OctreeElementBag& bag,
                                EncodeBitstreamParams& params) {

    // How many bytes have we written so far at this level;
    int bytesWritten = 0;

    // you can't call this without a valid element
    if (!element) {
        qCDebug(octree, "WARNING! encodeTreeBitstream() called with element=NULL");
        params.stopReason = EncodeBitstreamParams::NULL_NODE;
        return bytesWritten;
    }

    // you can't call this without a valid nodeData
    auto octreeQueryNode = static_cast<OctreeQueryNode*>(params.nodeData);
    if (!octreeQueryNode) {
        qCDebug(octree, "WARNING! encodeTreeBitstream() called with nodeData=NULL");
        params.stopReason = EncodeBitstreamParams::NULL_NODE_DATA;
        return bytesWritten;
    }

    // If we're at a element that is out of view, then we can return, because no nodes below us will be in view!
    if (octreeQueryNode->getUsesFrustum() && !params.recurseEverything && !element->isInView(params.viewFrustum)) {
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
            codeLength = numberOfThreeBitSectionsInCode(newCode);
            delete[] newCode;
        } else {
            codeLength = 1;
        }
    } else {
        roomForOctalCode = packetData->startSubTree(element->getOctalCode());
        codeLength = (int)bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(element->getOctalCode()));
    }

    // If the octalcode couldn't fit, then we can return, because no nodes below us will fit...
    if (!roomForOctalCode) {
        bag.insert(element);
        params.stopReason = EncodeBitstreamParams::DIDNT_FIT;
        return bytesWritten;
    }

    bytesWritten += codeLength; // keep track of byte count

    int currentEncodeLevel = 0;

    // record some stats, this is the one element that we won't record below in the recursion function, so we need to
    // track it here
    octreeQueryNode->stats.traversed(element);

    ViewFrustum::intersection parentLocationThisView = ViewFrustum::INTERSECT; // assume parent is in view, but not fully

    int childBytesWritten = encodeTreeBitstreamRecursion(element, packetData, bag, params,
                                                         currentEncodeLevel, parentLocationThisView);

    // if childBytesWritten == 1 then something went wrong... that's not possible
    assert(childBytesWritten != 1);

    // if childBytesWritten == 2, then it can only mean that the lower level trees don't exist or for some
    // reason couldn't be written... so reset them here... This isn't true for the non-color included case
    if (suppressEmptySubtrees() && childBytesWritten == 2) {
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

int Octree::encodeTreeBitstreamRecursion(const OctreeElementPointer& element,
                                         OctreePacketData* packetData, OctreeElementBag& bag,
                                         EncodeBitstreamParams& params, int& currentEncodeLevel,
                                         const ViewFrustum::intersection& parentLocationThisView) const {


    const bool wantDebug = false;

    // The append state of this level/element.
    OctreeElement::AppendState elementAppendState = OctreeElement::COMPLETED; // assume the best

    // How many bytes have we written so far at this level;
    int bytesAtThisLevel = 0;

    // you can't call this without a valid element
    if (!element) {
        qCDebug(octree, "WARNING! encodeTreeBitstreamRecursion() called with element=NULL");
        params.stopReason = EncodeBitstreamParams::NULL_NODE;
        return bytesAtThisLevel;
    }

    // you can't call this without a valid nodeData
    auto octreeQueryNode = static_cast<OctreeQueryNode*>(params.nodeData);
    if (!octreeQueryNode) {
        qCDebug(octree, "WARNING! encodeTreeBitstream() called with nodeData=NULL");
        params.stopReason = EncodeBitstreamParams::NULL_NODE_DATA;
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

    ViewFrustum::intersection nodeLocationThisView = ViewFrustum::INSIDE; // assume we're inside
    if (octreeQueryNode->getUsesFrustum() && !params.recurseEverything) {
        float boundaryDistance = boundaryDistanceForRenderLevel(element->getLevel() + params.boundaryLevelAdjust,
                                        params.octreeElementSizeScale);

        // If we're too far away for our render level, then just return
        if (element->distanceToCamera(params.viewFrustum) >= boundaryDistance) {
            octreeQueryNode->stats.skippedDistance(element);
            params.stopReason = EncodeBitstreamParams::LOD_SKIP;
            return bytesAtThisLevel;
        }

        // if the parent isn't known to be INSIDE, then it must be INTERSECT, and we should double check to see
        // if we are INSIDE, INTERSECT, or OUTSIDE
        if (parentLocationThisView != ViewFrustum::INSIDE) {
            assert(parentLocationThisView != ViewFrustum::OUTSIDE); // we shouldn't be here if our parent was OUTSIDE!
            nodeLocationThisView = element->computeViewIntersection(params.viewFrustum);
        }

        // If we're at a element that is out of view, then we can return, because no nodes below us will be in view!
        // although technically, we really shouldn't ever be here, because our callers shouldn't be calling us if
        // we're out of view
        if (nodeLocationThisView == ViewFrustum::OUTSIDE) {
            octreeQueryNode->stats.skippedOutOfView(element);
            params.stopReason = EncodeBitstreamParams::OUT_OF_VIEW;
            return bytesAtThisLevel;
        }

        // Ok, we are in view, but if we're in delta mode, then we also want to make sure we weren't already in view
        // because we don't send nodes from the previously know in view frustum.
        bool wasInView = false;

        if (params.deltaView) {
            ViewFrustum::intersection location = element->computeViewIntersection(params.lastViewFrustum);

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
                float boundaryDistance = boundaryDistanceForRenderLevel(element->getLevel() + params.boundaryLevelAdjust,
                                                                        params.octreeElementSizeScale);
                if (element->distanceToCamera(params.lastViewFrustum) >= boundaryDistance) {
                    // This would have been invisible... but now should be visible (we wouldn't be here otherwise)...
                    wasInView = false;
                }
            }
        }

        // If we were previously in the view, then we normally will return out of here and stop recursing. But
        // if we're in deltaView mode, and this element has changed since it was last sent, then we do
        // need to send it.
        if (wasInView && !(params.deltaView && element->hasChangedSince(octreeQueryNode->getLastTimeBagEmpty() - CHANGE_FUDGE))) {
            octreeQueryNode->stats.skippedWasInView(element);
            params.stopReason = EncodeBitstreamParams::WAS_IN_VIEW;
            return bytesAtThisLevel;
        }
    }

    // If we're not in delta sending mode, and we weren't asked to do a force send, and the voxel hasn't changed,
    // then we can also bail early and save bits
    if (!params.forceSendScene && !params.deltaView &&
        !element->hasChangedSince(octreeQueryNode->getLastTimeBagEmpty() - CHANGE_FUDGE)) {

        octreeQueryNode->stats.skippedNoChange(element);

        params.stopReason = EncodeBitstreamParams::NO_CHANGE;
        return bytesAtThisLevel;
    }

    bool keepDiggingDeeper = true; // Assuming we're in view we have a great work ethic, we're always ready for more!

    // At any given point in writing the bitstream, the largest minimum we might need to flesh out the current level
    // is 1 byte for child colors + 3*NUMBER_OF_CHILDREN bytes for the actual colors + 1 byte for child trees.
    // There could be sub trees below this point, which might take many more bytes, but that's ok, because we can
    // always mark our subtrees as not existing and stop the packet at this point, then start up with a new packet
    // for the remaining sub trees.
    unsigned char childrenExistInTreeBits = 0;
    unsigned char childrenExistInPacketBits = 0;
    unsigned char childrenDataBits = 0;

    // Make our local buffer large enough to handle writing at this level in case we need to.
    LevelDetails thisLevelKey = packetData->startLevel();
    int requiredBytes = sizeof(childrenDataBits) + sizeof(childrenExistInPacketBits);
    if (params.includeExistsBits) {
        requiredBytes += sizeof(childrenExistInTreeBits);
    }

    // If this datatype allows root elements to include data, and this is the root, then ask the tree for the
    // minimum bytes needed for root data and reserve those also
    if (element == _rootElement && rootElementHasData()) {
        requiredBytes += minimumRequiredRootDataBytes();
    }

    bool continueThisLevel = packetData->reserveBytes(requiredBytes);

    // If we can't reserve our minimum bytes then we can discard this level and return as if none of this level fits
    if (!continueThisLevel) {
        packetData->discardLevel(thisLevelKey);
        params.stopReason = EncodeBitstreamParams::DIDNT_FIT;
        bag.insert(element);
        return bytesAtThisLevel;
    }

    int inViewCount = 0;
    int inViewNotLeafCount = 0;
    int inViewWithColorCount = 0;

    OctreeElementPointer sortedChildren[NUMBER_OF_CHILDREN] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    float distancesToChildren[NUMBER_OF_CHILDREN] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    int indexOfChildren[NUMBER_OF_CHILDREN] = { 0, 0, 0, 0, 0, 0, 0, 0 };

    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        OctreeElementPointer childElement = element->getChildAtIndex(i);

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

        sortedChildren[i] = childElement;
        indexOfChildren[i] = i;
        distancesToChildren[i] = 0.0f;

        // track stats
        // must check childElement here, because it could be we got here with no childElement
        if (childElement) {
            octreeQueryNode->stats.traversed(childElement);
        }
    }

    // for each child element in Distance sorted order..., check to see if they exist, are colored, and in view, and if so
    // add them to our distance ordered array of children
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        OctreeElementPointer childElement = sortedChildren[i];
        int originalIndex = indexOfChildren[i];

        bool childIsInView  = (childElement &&
                (params.recurseEverything || !octreeQueryNode->getUsesFrustum() ||
                 (nodeLocationThisView == ViewFrustum::INSIDE) || // parent was fully in view, we can assume ALL children are
                  (nodeLocationThisView == ViewFrustum::INTERSECT &&
                        childElement->isInView(params.viewFrustum)) // the parent intersects and the child is in view
                ));

        if (!childIsInView) {
            // must check childElement here, because it could be we got here because there was no childElement
            if (childElement) {
                octreeQueryNode->stats.skippedOutOfView(childElement);
            }
        } else {
            // Before we consider this further, let's see if it's in our LOD scope...
            float boundaryDistance = params.recurseEverything || !octreeQueryNode->getUsesFrustum() ? 1 :
                                    boundaryDistanceForRenderLevel(childElement->getLevel() + params.boundaryLevelAdjust,
                                            params.octreeElementSizeScale);

            if (!(distancesToChildren[i] < boundaryDistance)) {
                // don't need to check childElement here, because we can't get here with no childElement
                octreeQueryNode->stats.skippedDistance(childElement);
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

                bool shouldRender = params.recurseEverything || !octreeQueryNode->getUsesFrustum() ||
                        childElement->calculateShouldRender(params.viewFrustum,
                                params.octreeElementSizeScale, params.boundaryLevelAdjust);

                // track some stats
                // don't need to check childElement here, because we can't get here with no childElement
                if (!shouldRender && childElement->isLeaf()) {
                    octreeQueryNode->stats.skippedDistance(childElement);
                }
                // don't need to check childElement here, because we can't get here with no childElement
                if (childIsOccluded) {
                    octreeQueryNode->stats.skippedOccluded(childElement);
                }

                // track children with actual color, only if the child wasn't previously in view!
                if (shouldRender && !childIsOccluded) {
                    bool childWasInView = false;

                    if (childElement && params.deltaView) {
                        ViewFrustum::intersection location = childElement->computeViewIntersection(params.lastViewFrustum);

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
                        (params.deltaView &&
                         childElement->hasChangedSince(octreeQueryNode->getLastTimeBagEmpty() - CHANGE_FUDGE))){

                        childrenDataBits += (1 << (7 - originalIndex));
                        inViewWithColorCount++;
                    } else {
                        // otherwise just track stats of the items we discarded
                        // don't need to check childElement here, because we can't get here with no childElement
                        if (childWasInView) {
                            octreeQueryNode->stats.skippedWasInView(childElement);
                        } else {
                            octreeQueryNode->stats.skippedNoChange(childElement);
                        }
                    }
                }
            }
        }
    }

    // NOTE: the childrenDataBits indicates that there is an array of child element data included in this packet.
    // We will write this bit mask but we may come back later and update the bits that are actually included
    packetData->releaseReservedBytes(sizeof(childrenDataBits));
    continueThisLevel = packetData->appendBitMask(childrenDataBits);

    int childDataBitsPlaceHolder = packetData->getUncompressedByteOffset(sizeof(childrenDataBits));
    unsigned char actualChildrenDataBits = 0;

    assert(continueThisLevel); // since we used reserved bits, this really shouldn't fail
    bytesAtThisLevel += sizeof(childrenDataBits); // keep track of byte count

    octreeQueryNode->stats.colorBitsWritten(); // really data bits not just color bits

    // NOW might be a good time to give our tree subclass and this element a chance to set up and check any extra encode data
    element->initializeExtraEncodeData(params);

    // write the child element data...
    // NOTE: the format of the bitstream is generally this:
    //    [octalcode]
    //    [bitmask for existence of child data]
    //        N x [child data]
    //    [bitmask for existence of child elements in tree]
    //    [bitmask for existence of child elements in buffer]
    //        N x [ ... tree for children ...]
    //
    // This section of the code, is writing the "N x [child data]" portion of this bitstream
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        if (oneAtBit(childrenDataBits, i)) {
            OctreeElementPointer childElement = element->getChildAtIndex(i);

            // the childrenDataBits were set up by the in view/LOD logic, it may contain children that we've already
            // processed and sent the data bits for. Let our tree subclass determine if it really wants to send the
            // data for this child at this point
            if (childElement && element->shouldIncludeChildData(i, params)) {

                int bytesBeforeChild = packetData->getUncompressedSize();

                // a childElement may "partially" write it's data. for example, the model server where the entire
                // contents of the element may be larger than can fit in a single MTU/packetData. In this case,
                // we want to allow the appendElementData() to respond that it produced partial data, which should be
                // written, but that the childElement needs to be reprocessed in an additional pass or passes
                // to be completed.
                LevelDetails childDataLevelKey = packetData->startLevel();

                OctreeElement::AppendState childAppendState = childElement->appendElementData(packetData, params);

                // allow our tree subclass to do any additional bookkeeping it needs to do with encoded data state
                element->updateEncodedData(i, childAppendState, params);

                // Continue this level so long as some part of this child element was appended.
                bool childFit = (childAppendState != OctreeElement::NONE);

                // some datatypes (like Voxels) assume that all child data will fit, if it doesn't fit
                // the data type wants to bail on this element level completely
                if (!childFit && mustIncludeAllChildData()) {
                    continueThisLevel = false;
                    break;
                }

                // If the child was partially or fully appended, then mark the actualChildrenDataBits as including
                // this child data
                if (childFit) {
                    actualChildrenDataBits += (1 << (7 - i));
                    continueThisLevel = packetData->endLevel(childDataLevelKey);
                } else {
                    packetData->discardLevel(childDataLevelKey);
                    elementAppendState = OctreeElement::PARTIAL;
                    params.stopReason = EncodeBitstreamParams::DIDNT_FIT;
                }

                // If this child was partially appended, then consider this element to be partially appended
                if (childAppendState == OctreeElement::PARTIAL) {
                    elementAppendState = OctreeElement::PARTIAL;
                    params.stopReason = EncodeBitstreamParams::DIDNT_FIT;
                }

                int bytesAfterChild = packetData->getUncompressedSize();

                bytesAtThisLevel += (bytesAfterChild - bytesBeforeChild); // keep track of byte count for this child

                // don't need to check childElement here, because we can't get here with no childElement
                if (childAppendState != OctreeElement::NONE) {
                    octreeQueryNode->stats.colorSent(childElement);
                }
            }
        }
    }

    if (!mustIncludeAllChildData() && !continueThisLevel) {
        qCDebug(octree) << "WARNING UNEXPECTED CASE: reached end of child element data loop with continueThisLevel=FALSE";
        qCDebug(octree) << "This is not expected!!!!  -- continueThisLevel=FALSE....";
    }

    if (continueThisLevel && actualChildrenDataBits != childrenDataBits) {
        // repair the child data mask
        continueThisLevel = packetData->updatePriorBitMask(childDataBitsPlaceHolder, actualChildrenDataBits);
        if (!continueThisLevel) {
            qCDebug(octree) << "WARNING UNEXPECTED CASE: Failed to update childDataBitsPlaceHolder";
            qCDebug(octree) << "This is not expected!!!!  -- continueThisLevel=FALSE....";
        }
    }

    // if the caller wants to include childExistsBits, then include them even if not in view, put them before the
    // childrenExistInPacketBits, so that the lower code can properly repair the packet exists bits
    if (continueThisLevel && params.includeExistsBits) {
        packetData->releaseReservedBytes(sizeof(childrenExistInTreeBits));
        continueThisLevel = packetData->appendBitMask(childrenExistInTreeBits);
        if (continueThisLevel) {
            bytesAtThisLevel += sizeof(childrenExistInTreeBits); // keep track of byte count

            octreeQueryNode->stats.existsBitsWritten();
        } else {
            qCDebug(octree) << "WARNING UNEXPECTED CASE: Failed to append childrenExistInTreeBits";
            qCDebug(octree) << "This is not expected!!!!  -- continueThisLevel=FALSE....";
        }
    }

    // write the child exist bits
    if (continueThisLevel) {
        packetData->releaseReservedBytes(sizeof(childrenExistInPacketBits));
        continueThisLevel = packetData->appendBitMask(childrenExistInPacketBits);
        if (continueThisLevel) {
            bytesAtThisLevel += sizeof(childrenExistInPacketBits); // keep track of byte count

            octreeQueryNode->stats.existsInPacketBitsWritten();
        } else {
            qCDebug(octree) << "WARNING UNEXPECTED CASE: Failed to append childrenExistInPacketBits";
            qCDebug(octree) << "This is not expected!!!!  -- continueThisLevel=FALSE....";
        }
    }

    // We only need to keep digging, if there is at least one child that is inView, and not a leaf.
    keepDiggingDeeper = (inViewNotLeafCount > 0);

    //
    // NOTE: the format of the bitstream is generally this:
    //    [octalcode]
    //    [bitmask for existence of child data]
    //        N x [child data]
    //    [bitmask for existence of child elements in tree]
    //    [bitmask for existence of child elements in buffer]
    //        N x [ ... tree for children ...]
    //
    // This section of the code, is writing the "N x [ ... tree for children ...]" portion of this bitstream
    //
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

        // for each child element in Distance sorted order..., check to see if they exist, are colored, and in view, and if so
        // add them to our distance ordered array of children
        for (int indexByDistance = 0; indexByDistance < NUMBER_OF_CHILDREN; indexByDistance++) {
            OctreeElementPointer childElement = sortedChildren[indexByDistance];
            int originalIndex = indexOfChildren[indexByDistance];

            if (oneAtBit(childrenExistInPacketBits, originalIndex)) {

                int thisLevel = currentEncodeLevel;

                int childTreeBytesOut = 0;

                // NOTE: some octree styles (like models and particles) will store content in parent elements, and child
                // elements. In this case, if we stop recursion when we include any data (the colorbits should really be
                // called databits), then we wouldn't send the children. So those types of Octree's should tell us to keep
                // recursing, by returning TRUE in recurseChildrenWithData().

                if (params.recurseEverything || !octreeQueryNode->getUsesFrustum()
                    || recurseChildrenWithData() || !oneAtBit(childrenDataBits, originalIndex)) {

                    // Allow the datatype a chance to determine if it really wants to recurse this tree. Usually this
                    // will be true. But if the tree has already been encoded, we will skip this.
                    if (element->shouldRecurseChildTree(originalIndex, params)) {
                        childTreeBytesOut = encodeTreeBitstreamRecursion(childElement, packetData, bag, params,
                                                                                thisLevel, nodeLocationThisView);
                    } else {
                        childTreeBytesOut = 0;
                    }
                }

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
                if (suppressEmptySubtrees() && !params.includeExistsBits && childTreeBytesOut == 2) {
                    childTreeBytesOut = 0; // this is the degenerate case of a tree with no colors and no child trees

                }

                bytesAtThisLevel += childTreeBytesOut;

                // If we had previously started writing, and if the child DIDN'T write any bytes,
                // then we want to remove their bit from the childExistsPlaceHolder bitmask
                if (childTreeBytesOut == 0) {

                    // remove this child's bit...
                    childrenExistInPacketBits -= (1 << (7 - originalIndex));

                    // repair the child exists mask
                    continueThisLevel = packetData->updatePriorBitMask(childExistsPlaceHolder, childrenExistInPacketBits);
                    if (!continueThisLevel) {
                        qCDebug(octree) << "WARNING UNEXPECTED CASE: Failed to update childExistsPlaceHolder";
                        qCDebug(octree) << "This is not expected!!!!  -- continueThisLevel=FALSE....";
                    }

                    // If this is the last of the child exists bits, then we're actually be rolling out the entire tree
                    if (childrenExistInPacketBits == 0) {
                        octreeQueryNode->stats.childBitsRemoved(params.includeExistsBits);
                    }

                    if (!continueThisLevel) {
                        if (wantDebug) {
                            qCDebug(octree) << "    WARNING line:" << __LINE__;
                            qCDebug(octree) << "       breaking the child recursion loop with continueThisLevel=false!!!";
                            qCDebug(octree) << "       AFTER attempting to updatePriorBitMask() for empty sub tree....";
                            qCDebug(octree) << "       IS THIS ACCEPTABLE!!!!";
                        }
                        break; // can't continue...
                    }

                    // Note: no need to move the pointer, cause we already stored this
                } // end if (childTreeBytesOut == 0)
            } // end if (oneAtBit(childrenExistInPacketBits, originalIndex))
        } // end for
    } // end keepDiggingDeeper

    // If we made it this far, then we've written all of our child data... if this element is the root
    // element, then we also allow the root element to write out it's data...
    if (continueThisLevel && element == _rootElement && rootElementHasData()) {
        int bytesBeforeChild = packetData->getUncompressedSize();

        // release the bytes we reserved...
        packetData->releaseReservedBytes(minimumRequiredRootDataBytes());

        LevelDetails rootDataLevelKey = packetData->startLevel();
        OctreeElement::AppendState rootAppendState = element->appendElementData(packetData, params);

        bool partOfRootFit = (rootAppendState != OctreeElement::NONE);
        bool allOfRootFit = (rootAppendState == OctreeElement::COMPLETED);

        if (partOfRootFit) {
            continueThisLevel = packetData->endLevel(rootDataLevelKey);
            if (!continueThisLevel) {
                qCDebug(octree) << " UNEXPECTED ROOT ELEMENT -- could not packetData->endLevel(rootDataLevelKey) -- line:" << __LINE__;
            }
        } else {
            packetData->discardLevel(rootDataLevelKey);
        }

        if (!allOfRootFit) {
            elementAppendState = OctreeElement::PARTIAL;
            params.stopReason = EncodeBitstreamParams::DIDNT_FIT;
        }

        // do we really ever NOT want to continue this level???
        //continueThisLevel = (rootAppendState == OctreeElement::COMPLETED);

        int bytesAfterChild = packetData->getUncompressedSize();

        if (continueThisLevel) {
            bytesAtThisLevel += (bytesAfterChild - bytesBeforeChild); // keep track of byte count for this child

            octreeQueryNode->stats.colorSent(element);
        }

        if (!continueThisLevel) {
            qCDebug(octree) << "WARNING UNEXPECTED CASE: Something failed in packing ROOT data";
            qCDebug(octree) << "This is not expected!!!! -- continueThisLevel=FALSE....";
        }
    }

    // if we were unable to fit this level in our packet, then rewind and add it to the element bag for
    // sending later...
    if (continueThisLevel) {
        continueThisLevel = packetData->endLevel(thisLevelKey);
    } else {
        packetData->discardLevel(thisLevelKey);

        if (!mustIncludeAllChildData()) {
            qCDebug(octree) << "WARNING UNEXPECTED CASE: Something failed in attempting to pack this element";
            qCDebug(octree) << "This is not expected!!!! -- continueThisLevel=FALSE....";
        }
    }

    // This happens if the element could not be written at all. In the case of Octree's that support partial
    // element data, continueThisLevel will be true. So this only happens if the full element needs to be
    // added back to the element bag.
    if (!continueThisLevel) {
        if (!mustIncludeAllChildData()) {
            qCDebug(octree) << "WARNING UNEXPECTED CASE - Something failed in attempting to pack this element.";
            qCDebug(octree) << "   If the datatype requires all child data, then this might happen. Otherwise" ;
            qCDebug(octree) << "   this is an unexpected case and we should research a potential logic error." ;
        }

        bag.insert(element);

        // don't need to check element here, because we can't get here with no element
        octreeQueryNode->stats.didntFit(element);

        params.stopReason = EncodeBitstreamParams::DIDNT_FIT;
        bytesAtThisLevel = 0; // didn't fit
    } else {

        // assuming we made it here with continueThisLevel == true, we STILL might want
        // to add our element back to the bag for additional encoding, specifically if
        // the appendState is PARTIAL, in this case, we re-add our element to the bag
        // and assume that the appendElementData() has stored any required state data
        // in the params extraEncodeData
        if (elementAppendState == OctreeElement::PARTIAL) {
            bag.insert(element);
        }
    }

    // If our element is completed let the element know so it can do any cleanup it of extra  wants
    if (elementAppendState == OctreeElement::COMPLETED) {
        element->elementEncodeComplete(params);
    }

    return bytesAtThisLevel;
}

bool Octree::readFromFile(const char* fileName) {
    QString qFileName = findMostRecentFileExtension(fileName, PERSIST_EXTENSIONS);

    if (qFileName.endsWith(".json.gz")) {
        return readJSONFromGzippedFile(qFileName);
    }

    QFile file(qFileName);

    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "unable to open for reading: " << fileName;
        return false;
    }

    QDataStream fileInputStream(&file);
    QFileInfo fileInfo(qFileName);
    uint64_t fileLength = fileInfo.size();

    emit importSize(1.0f, 1.0f, 1.0f);
    emit importProgress(0);

    qCDebug(octree) << "Loading file" << qFileName << "...";

    bool success = readFromStream(fileLength, fileInputStream);

    emit importProgress(100);
    file.close();

    return success;
}

bool Octree::readJSONFromGzippedFile(QString qFileName) {
    QFile file(qFileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Cannot open gzipped json file for reading: " << qFileName;
        return false;
    }
    QByteArray compressedJsonData = file.readAll();
    QByteArray jsonData;

    if (!gunzip(compressedJsonData, jsonData)) {
        qCritical() << "json File not in gzip format: " << qFileName;
        return false;
    }

    QDataStream jsonStream(jsonData);
    return readJSONFromStream(-1, jsonStream);
}

// hack to get the marketplace id into the entities.  We will create a way to get this from a hash of
// the entity later, but this helps us move things along for now
QString getMarketplaceID(const QString& urlString) {
    // the url should be http://mpassets.highfidelity.com/<uuid>-v1/<item name>.extension
    // a regex for the this is a PITA as there are several valid versions of uuids, and so
    // lets strip out the uuid (if any) and try to create a UUID from the string, relying on
    // QT to parse it
    static const QRegularExpression re("^http:\\/\\/mpassets.highfidelity.com\\/([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12})-v[\\d]+\\/.*");
    QRegularExpressionMatch match = re.match(urlString);
    if (match.hasMatch()) {
        QString matched = match.captured(1);
        if (QUuid(matched).isNull()) {
            qDebug() << "invalid uuid for marketplaceID";
        } else {
            return matched;
        }
    }
    return QString();
}

bool Octree::readFromURL(const QString& urlString) {
    QString marketplaceID = getMarketplaceID(urlString);
    auto request =
        std::unique_ptr<ResourceRequest>(DependencyManager::get<ResourceManager>()->createResourceRequest(this, urlString));

    if (!request) {
        return false;
    }

    QEventLoop loop;
    connect(request.get(), &ResourceRequest::finished, &loop, &QEventLoop::quit);
    request->send();
    loop.exec();

    if (request->getResult() != ResourceRequest::Success) {
        return false;
    }

    auto data = request->getData();
    QDataStream inputStream(data);
    return readFromStream(data.size(), inputStream, marketplaceID);
}


bool Octree::readFromStream(uint64_t streamLength, QDataStream& inputStream, const QString& marketplaceID) {
    // decide if this is binary SVO or JSON-formatted SVO
    QIODevice *device = inputStream.device();
    char firstChar;
    device->getChar(&firstChar);
    device->ungetChar(firstChar);

    if (firstChar == (char) PacketType::EntityData) {
        qCWarning(octree) << "Reading from binary SVO no longer supported";
        return false;
    } else {
        qCDebug(octree) << "Reading from JSON SVO Stream length:" << streamLength;
        return readJSONFromStream(streamLength, inputStream, marketplaceID);
    }
}


// hack to get the marketplace id into the entities.  We will create a way to get this from a hash of
// the entity later, but this helps us move things along for now
QJsonDocument addMarketplaceIDToDocumentEntities(QJsonDocument& doc, const QString& marketplaceID) {
    if (!marketplaceID.isEmpty()) {
        QJsonDocument newDoc;
        QJsonObject rootObj = doc.object();
        QJsonArray newEntitiesArray;

        // build a new entities array
        auto entitiesArray = rootObj["Entities"].toArray();
        for(auto it = entitiesArray.begin(); it != entitiesArray.end(); it++) {
            auto entity = (*it).toObject();
            entity["marketplaceID"] = marketplaceID;
            newEntitiesArray.append(entity);
        }
        rootObj["Entities"] = newEntitiesArray;
        newDoc.setObject(rootObj);
        return newDoc;
    }
    return doc;
}

const int READ_JSON_BUFFER_SIZE = 2048;

bool Octree::readJSONFromStream(uint64_t streamLength, QDataStream& inputStream, const QString& marketplaceID /*=""*/) {
    // if the data is gzipped we may not have a useful bytesAvailable() result, so just keep reading until
    // we get an eof.  Leave streamLength parameter for consistency.

    QByteArray jsonBuffer;
    char* rawData = new char[READ_JSON_BUFFER_SIZE];
    while (!inputStream.atEnd()) {
        int got = inputStream.readRawData(rawData, READ_JSON_BUFFER_SIZE - 1);
        if (got < 0) {
            qCritical() << "error while reading from json stream";
            delete[] rawData;
            return false;
        }
        if (got == 0) {
            break;
        }
        jsonBuffer += QByteArray(rawData, got);
    }

    QJsonDocument asDocument = QJsonDocument::fromJson(jsonBuffer);
    if (!marketplaceID.isEmpty()) {
        asDocument = addMarketplaceIDToDocumentEntities(asDocument, marketplaceID);
    }
    QVariant asVariant = asDocument.toVariant();
    QVariantMap asMap = asVariant.toMap();
    bool success = readFromMap(asMap);
    delete[] rawData;
    return success;
}

bool Octree::writeToFile(const char* fileName, const OctreeElementPointer& element, QString persistAsFileType) {
    // make the sure file extension makes sense
    QString qFileName = fileNameWithoutExtension(QString(fileName), PERSIST_EXTENSIONS) + "." + persistAsFileType;
    QByteArray byteArray = qFileName.toUtf8();
    const char* cFileName = byteArray.constData();

    bool success = false;
    if (persistAsFileType == "json") {
        success = writeToJSONFile(cFileName, element);
    } else if (persistAsFileType == "json.gz") {
        success = writeToJSONFile(cFileName, element, true);
    } else {
        qCDebug(octree) << "unable to write octree to file of type" << persistAsFileType;
    }
    return success;
}

bool Octree::writeToJSONFile(const char* fileName, const OctreeElementPointer& element, bool doGzip) {
    QVariantMap entityDescription;

    qCDebug(octree, "Saving JSON SVO to file %s...", fileName);

    OctreeElementPointer top;
    if (element) {
        top = element;
    } else {
        top = _rootElement;
    }

    // include the "bitstream" version
    PacketType expectedType = expectedDataPacketType();
    PacketVersion expectedVersion = versionForPacketType(expectedType);
    entityDescription["Version"] = (int) expectedVersion;

    // store the entity data
    bool entityDescriptionSuccess = writeToMap(entityDescription, top, true, true);
    if (!entityDescriptionSuccess) {
        qCritical("Failed to convert Entities to QVariantMap while saving to json.");
        return false;
    }

    // convert the QVariantMap to JSON
    QByteArray jsonData = QJsonDocument::fromVariant(entityDescription).toJson();
    QByteArray jsonDataForFile;

    if (doGzip) {
        if (!gzip(jsonData, jsonDataForFile, -1)) {
            qCritical("unable to gzip data while saving to json.");
            return false;
        }
    } else {
        jsonDataForFile = jsonData;
    }

    QFile persistFile(fileName);
    bool success = false;
    if (persistFile.open(QIODevice::WriteOnly)) {
        success = persistFile.write(jsonDataForFile) != -1;
    } else {
        qCritical("Could not write to JSON description of entities.");
    }

    return success;
}

uint64_t Octree::getOctreeElementsCount() {
    uint64_t nodeCount = 0;
    recurseTreeWithOperation(countOctreeElementsOperation, &nodeCount);
    return nodeCount;
}

bool Octree::countOctreeElementsOperation(const OctreeElementPointer& element, void* extraData) {
    (*(uint64_t*)extraData)++;
    return true; // keep going
}

void Octree::cancelImport() {
    _stopImport = true;
}
