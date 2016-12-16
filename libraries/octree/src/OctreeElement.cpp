//
//  OctreeElement.cpp
//  libraries/octree/src
//
//  Created by Stephen Birarda on 3/13/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <assert.h>
#include <cmath>
#include <cstring>
#include <stdio.h>

#include <QtCore/QDebug>

#include <Profile.h>

#include <LogHandler.h>
#include <NodeList.h>
#include <PerfStat.h>

#include "AACube.h"
#include "OctalCode.h"
#include "Octree.h"
#include "OctreeConstants.h"
#include "OctreeElement.h"
#include "OctreeLogging.h"
#include "OctreeUtils.h"
#include "SharedUtil.h"
#include <Trace.h>

AtomicUIntStat OctreeElement::_octreeMemoryUsage { 0 };
AtomicUIntStat OctreeElement::_octcodeMemoryUsage { 0 };
AtomicUIntStat OctreeElement::_externalChildrenMemoryUsage { 0 };
AtomicUIntStat OctreeElement::_voxelNodeCount { 0 };
AtomicUIntStat OctreeElement::_voxelNodeLeafCount { 0 };

void OctreeElement::resetPopulationStatistics() {
    _voxelNodeCount = 0;
    _voxelNodeLeafCount = 0;
}

OctreeElement::OctreeElement() {
    // Note: you must call init() from your subclass, otherwise the OctreeElement will not be properly
    // initialized. You will see DEADBEEF in your memory debugger if you have not properly called init()
    // debug::setDeadBeef(this, sizeof(*this));
}

void OctreeElement::init(unsigned char * octalCode) {
    if (!octalCode) {
        octalCode = new unsigned char[1];
        *octalCode = 0;
    }
    _voxelNodeCount++;
    _voxelNodeLeafCount++; // all nodes start as leaf nodes


    size_t octalCodeLength = bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(octalCode));
    if (octalCodeLength > sizeof(_octalCode)) {
        _octalCode.pointer = octalCode;
        _octcodePointer = true;
        _octcodeMemoryUsage += octalCodeLength;
    } else {
        _octcodePointer = false;
        memcpy(_octalCode.buffer, octalCode, octalCodeLength);
        delete[] octalCode;
    }

    // set up the _children union
    _childBitmask = 0;
    _childrenExternal = false;


    _childrenCount[0]++;

    // default pointers to child nodes to NULL
#ifdef SIMPLE_CHILD_ARRAY
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        _simpleChildArray[i] = NULL;
    }
#endif

#ifdef SIMPLE_EXTERNAL_CHILDREN
    _childrenSingle.reset();
#endif

    for (int i = 0; i < NUMBER_OF_CHILDREN; i ++) {
        _externalChildren[i].reset();
    }

    _isDirty = true;
    _shouldRender = false;
    _sourceUUIDKey = 0;
    calculateAACube();
    markWithChangedTime();
}

OctreeElement::~OctreeElement() {
    _voxelNodeCount--;
    if (isLeaf()) {
        _voxelNodeLeafCount--;
    }

    if (_octcodePointer) {
        _octcodeMemoryUsage -= bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(getOctalCode()));
        delete[] _octalCode.pointer;
    }

    // delete all of this node's children, this also takes care of all population tracking data
    deleteAllChildren();
}

void OctreeElement::markWithChangedTime() {
    _lastChanged = usecTimestampNow();
}

// This method is called by Octree when the subtree below this node
// is known to have changed. It's intended to be used as a place to do
// bookkeeping that a node may need to do when the subtree below it has
// changed. However, you should hopefully make your bookkeeping relatively
// localized, because this method will get called for every node in an
// recursive unwinding case like delete or add voxel
void OctreeElement::handleSubtreeChanged(OctreePointer myTree) {
    // here's a good place to do color re-averaging...
    if (myTree->getShouldReaverage()) {
        calculateAverageFromChildren();
    }

    markWithChangedTime();
}

const uint16_t KEY_FOR_NULL = 0;
uint16_t OctreeElement::_nextUUIDKey = KEY_FOR_NULL + 1; // start at 1, 0 is reserved for NULL
std::map<QString, uint16_t> OctreeElement::_mapSourceUUIDsToKeys;
std::map<uint16_t, QString> OctreeElement::_mapKeysToSourceUUIDs;

void OctreeElement::setSourceUUID(const QUuid& sourceUUID) {
    uint16_t key;
    QString sourceUUIDString = sourceUUID.toString();
    if (_mapSourceUUIDsToKeys.end() != _mapSourceUUIDsToKeys.find(sourceUUIDString)) {
        key = _mapSourceUUIDsToKeys[sourceUUIDString];
    } else {
        key = _nextUUIDKey;
        _nextUUIDKey++;
        _mapSourceUUIDsToKeys[sourceUUIDString] = key;
        _mapKeysToSourceUUIDs[key] = sourceUUIDString;
    }
    _sourceUUIDKey = key;
}

QUuid OctreeElement::getSourceUUID() const {
    if (_sourceUUIDKey > KEY_FOR_NULL) {
        if (_mapKeysToSourceUUIDs.end() != _mapKeysToSourceUUIDs.find(_sourceUUIDKey)) {
            return QUuid(_mapKeysToSourceUUIDs[_sourceUUIDKey]);
        }
    }
    return QUuid();
}

bool OctreeElement::matchesSourceUUID(const QUuid& sourceUUID) const {
    if (_sourceUUIDKey > KEY_FOR_NULL) {
        if (_mapKeysToSourceUUIDs.end() != _mapKeysToSourceUUIDs.find(_sourceUUIDKey)) {
            return QUuid(_mapKeysToSourceUUIDs[_sourceUUIDKey]) == sourceUUID;
        }
    }
    return sourceUUID.isNull();
}

uint16_t OctreeElement::getSourceNodeUUIDKey(const QUuid& sourceUUID) {
    uint16_t key = KEY_FOR_NULL;
    QString sourceUUIDString = sourceUUID.toString();
    if (_mapSourceUUIDsToKeys.end() != _mapSourceUUIDsToKeys.find(sourceUUIDString)) {
        key = _mapSourceUUIDsToKeys[sourceUUIDString];
    }
    return key;
}



void OctreeElement::setShouldRender(bool shouldRender) {
    // if shouldRender is changing, then consider ourselves dirty
    if (shouldRender != _shouldRender) {
        _shouldRender = shouldRender;
        _isDirty = true;
        markWithChangedTime();
    }
}

void OctreeElement::calculateAACube() {
    // copy corner into cube
    glm::vec3 corner;
    copyFirstVertexForCode(getOctalCode(), (float*)&corner);

    // this tells you the "size" of the voxel
    float voxelScale = (float)TREE_SCALE / powf(2.0f, numberOfThreeBitSectionsInCode(getOctalCode()));
    corner *= (float)TREE_SCALE;
    corner -= (float)HALF_TREE_SCALE;
    _cube.setBox(corner, voxelScale);
}

void OctreeElement::deleteChildAtIndex(int childIndex) {
    OctreeElementPointer childAt = getChildAtIndex(childIndex);
    if (childAt) {
        childAt.reset();
        setChildAtIndex(childIndex, NULL);
        _isDirty = true;
        markWithChangedTime();

        // after deleting the child, check to see if we're a leaf
        if (isLeaf()) {
            _voxelNodeLeafCount++;
        }
    }
}

// does not delete the node!
OctreeElementPointer OctreeElement::removeChildAtIndex(int childIndex) {
    OctreeElementPointer returnedChild = getChildAtIndex(childIndex);
    if (returnedChild) {
        setChildAtIndex(childIndex, NULL);
        _isDirty = true;
        markWithChangedTime();

        // after removing the child, check to see if we're a leaf
        if (isLeaf()) {
            _voxelNodeLeafCount++;
        }
    }
    return returnedChild;
}

bool OctreeElement::isParentOf(OctreeElementPointer possibleChild) const {
    if (possibleChild) {
        for (int childIndex = 0; childIndex < NUMBER_OF_CHILDREN; childIndex++) {
            OctreeElementPointer childAt = getChildAtIndex(childIndex);
            if (childAt == possibleChild) {
                return true;
            }
        }
    }
    return false;
}

AtomicUIntStat OctreeElement::_getChildAtIndexTime { 0 };
AtomicUIntStat OctreeElement::_getChildAtIndexCalls { 0 };
AtomicUIntStat OctreeElement::_setChildAtIndexTime { 0 };
AtomicUIntStat OctreeElement::_setChildAtIndexCalls { 0 };
AtomicUIntStat OctreeElement::_externalChildrenCount { 0 };
AtomicUIntStat OctreeElement::_childrenCount[NUMBER_OF_CHILDREN + 1];

OctreeElementPointer OctreeElement::getChildAtIndex(int childIndex) const {
#ifdef SIMPLE_CHILD_ARRAY
    return _simpleChildArray[childIndex];
#endif // SIMPLE_CHILD_ARRAY

#ifdef SIMPLE_EXTERNAL_CHILDREN
    int childCount = getChildCount();

    switch (childCount) {
        case 0: {
            return NULL;
        } break;

        case 1: {
            // if our single child is the one being requested, return it, otherwise
            // return null
            int firstIndex = getNthBit(_childBitmask, 1);
            if (firstIndex == childIndex) {
                return _childrenSingle;
            } else {
                return NULL;
            }
        } break;

        default : {
            return _externalChildren[childIndex];
        } break;
    }
#endif // def SIMPLE_EXTERNAL_CHILDREN
}

void OctreeElement::deleteAllChildren() {
    // first delete all the OctreeElement objects...
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        OctreeElementPointer childAt = getChildAtIndex(i);
        if (childAt) {
            childAt.reset();
        }
    }

    if (_childrenExternal) {
        // if the children_t union represents _children.external we need to delete it here
        for (int i = 0; i < NUMBER_OF_CHILDREN; i ++) {
            _externalChildren[i].reset();
        }
    }
}

void OctreeElement::setChildAtIndex(int childIndex, OctreeElementPointer child) {
#ifdef SIMPLE_CHILD_ARRAY
    int previousChildCount = getChildCount();
    if (child) {
        setAtBit(_childBitmask, childIndex);
    } else {
        clearAtBit(_childBitmask, childIndex);
    }
    int newChildCount = getChildCount();

    // store the child in our child array
    _simpleChildArray[childIndex] = child;

    // track our population data
    if (previousChildCount != newChildCount) {
        _childrenCount[previousChildCount]--;
        _childrenCount[newChildCount]++;
    }
#endif

#ifdef SIMPLE_EXTERNAL_CHILDREN

    int firstIndex = getNthBit(_childBitmask, 1);
    int secondIndex = getNthBit(_childBitmask, 2);

    int previousChildCount = getChildCount();
    if (child) {
        setAtBit(_childBitmask, childIndex);
    } else {
        clearAtBit(_childBitmask, childIndex);
    }
    int newChildCount = getChildCount();

    // track our population data
    if (previousChildCount != newChildCount) {
        _childrenCount[previousChildCount]--;
        _childrenCount[newChildCount]++;
    }

    if ((previousChildCount == 0 || previousChildCount == 1) && newChildCount == 0) {
        _childrenSingle.reset();
    } else if (previousChildCount == 0 && newChildCount == 1) {
        _childrenSingle = child;
    } else if (previousChildCount == 1 && newChildCount == 2) {
        OctreeElementPointer previousChild = _childrenSingle;
        for (int i = 0; i < NUMBER_OF_CHILDREN; i ++) {
            _externalChildren[i].reset();
        }
        _externalChildren[firstIndex] = previousChild;
        _externalChildren[childIndex] = child;

        _childrenExternal = true;

        _externalChildrenMemoryUsage += NUMBER_OF_CHILDREN * sizeof(OctreeElementPointer);

    } else if (previousChildCount == 2 && newChildCount == 1) {
        assert(!child); // we are removing a child, so this must be true!
        OctreeElementPointer previousFirstChild = _externalChildren[firstIndex];
        OctreeElementPointer previousSecondChild = _externalChildren[secondIndex];

        for (int i = 0; i < NUMBER_OF_CHILDREN; i ++) {
            _externalChildren[i].reset();
        }
        _childrenExternal = false;

        _externalChildrenMemoryUsage -= NUMBER_OF_CHILDREN * sizeof(OctreeElementPointer);
        if (childIndex == firstIndex) {
            _childrenSingle = previousSecondChild;
        } else {
            _childrenSingle = previousFirstChild;
        }
    } else {
        _externalChildren[childIndex] = child;
    }

#endif // def SIMPLE_EXTERNAL_CHILDREN
}


OctreeElementPointer OctreeElement::addChildAtIndex(int childIndex) {
    OctreeElementPointer childAt = getChildAtIndex(childIndex);
    if (!childAt) {
        // before adding a child, see if we're currently a leaf
        if (isLeaf()) {
            _voxelNodeLeafCount--;
        }

        unsigned char* newChildCode = childOctalCode(getOctalCode(), childIndex);
        childAt = createNewElement(newChildCode);
        setChildAtIndex(childIndex, childAt);

        _isDirty = true;
        markWithChangedTime();
        PROFILE_INSTANT(octree, "EntityAdd", "g");
    }
    return childAt;
}

// handles staging or deletion of all deep children
bool OctreeElement::safeDeepDeleteChildAtIndex(int childIndex, int recursionCount) {
    bool deleteApproved = false;
    if (recursionCount > DANGEROUSLY_DEEP_RECURSION) {
        static QString repeatedMessage
            = LogHandler::getInstance().addRepeatedMessageRegex(
                    "OctreeElement::safeDeepDeleteChildAtIndex\\(\\) reached DANGEROUSLY_DEEP_RECURSION, bailing!");

        qCDebug(octree) << "OctreeElement::safeDeepDeleteChildAtIndex() reached DANGEROUSLY_DEEP_RECURSION, bailing!";
        return deleteApproved;
    }
    OctreeElementPointer childToDelete = getChildAtIndex(childIndex);
    if (childToDelete) {
        if (childToDelete->deleteApproved()) {
            // If the child is not a leaf, then call ourselves recursively on all the children
            if (!childToDelete->isLeaf()) {
                // delete all it's children
                for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
                    if (childToDelete->getChildAtIndex(i)) {
                        deleteApproved = childToDelete->safeDeepDeleteChildAtIndex(i,recursionCount+1);
                        if (!deleteApproved) {
                            break; // no point in continuing...
                        }
                    }
                }
            } else {
                deleteApproved = true; // because we got here after checking that delete was approved
            }
            if (deleteApproved) {
                deleteChildAtIndex(childIndex);
                _isDirty = true;
                markWithChangedTime();
            }
        }
    }
    return deleteApproved;
}


void OctreeElement::printDebugDetails(const char* label) const {
    unsigned char childBits = 0;
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        OctreeElementPointer childAt = getChildAtIndex(i);
        if (childAt) {
            setAtBit(childBits,i);
        }
    }

    QDebug elementDebug = qDebug().nospace();

    QString resultString;
    resultString.sprintf("%s - Voxel at corner=(%f,%f,%f) size=%f\n isLeaf=%s isDirty=%s shouldRender=%s\n children=", label,
                         (double)_cube.getCorner().x, (double)_cube.getCorner().y, (double)_cube.getCorner().z,
                         (double)_cube.getScale(),
                         debug::valueOf(isLeaf()), debug::valueOf(isDirty()), debug::valueOf(getShouldRender()));
    elementDebug << resultString;

    outputBits(childBits, &elementDebug);
    qDebug("octalCode=");
    printOctalCode(getOctalCode());
}

float OctreeElement::getEnclosingRadius() const {
    return getScale() * sqrtf(3.0f) / 2.0f;
}

ViewFrustum::intersection OctreeElement::computeViewIntersection(const ViewFrustum& viewFrustum) const {
    return viewFrustum.calculateCubeKeyholeIntersection(_cube);
}

// There are two types of nodes for which we want to "render"
// 1) Leaves that are in the LOD
// 2) Non-leaves are more complicated though... usually you don't want to render them, but if their children
//    wouldn't be rendered, then you do want to render them. But sometimes they have some children that ARE
//    in the LOD, and others that are not. In this case we want to render the parent, and none of the children.
//
//    Since, if we know the camera position and orientation, we can know which of the corners is the "furthest"
//    corner. We can use we can use this corner as our "voxel position" to do our distance calculations off of.
//    By doing this, we don't need to test each child voxel's position vs the LOD boundary
bool OctreeElement::calculateShouldRender(const ViewFrustum& viewFrustum, float voxelScaleSize, int boundaryLevelAdjust) const {
    bool shouldRender = false;

    if (hasContent()) {
        float furthestDistance = furthestDistanceToCamera(viewFrustum);
        float childBoundary = boundaryDistanceForRenderLevel(getLevel() + 1 + boundaryLevelAdjust, voxelScaleSize);
        bool inChildBoundary = (furthestDistance <= childBoundary);
        if (hasDetailedContent() && inChildBoundary) {
            shouldRender = true;
        } else {
            float boundary = childBoundary * 2.0f; // the boundary is always twice the distance of the child boundary
            bool inBoundary = (furthestDistance <= boundary);
            shouldRender = inBoundary && !inChildBoundary;
        }
    }
    return shouldRender;
}

// Calculates the distance to the furthest point of the voxel to the camera
// does as much math as possible in voxel scale and then scales up to TREE_SCALE at end
float OctreeElement::furthestDistanceToCamera(const ViewFrustum& viewFrustum) const {
    glm::vec3 furthestPoint;
    viewFrustum.getFurthestPointFromCamera(_cube, furthestPoint);
    glm::vec3 temp = viewFrustum.getPosition() - furthestPoint;
    return sqrtf(glm::dot(temp, temp));
}

float OctreeElement::distanceToCamera(const ViewFrustum& viewFrustum) const {
    glm::vec3 center = _cube.calcCenter();
    glm::vec3 temp = viewFrustum.getPosition() - center;
    float distanceToVoxelCenter = sqrtf(glm::dot(temp, temp));
    return distanceToVoxelCenter;
}

float OctreeElement::distanceSquareToPoint(const glm::vec3& point) const {
    glm::vec3 temp = point - _cube.calcCenter();
    float distanceSquare = glm::dot(temp, temp);
    return distanceSquare;
}

float OctreeElement::distanceToPoint(const glm::vec3& point) const {
    glm::vec3 temp = point - _cube.calcCenter();
    float distance = sqrtf(glm::dot(temp, temp));
    return distance;
}

bool OctreeElement::findSpherePenetration(const glm::vec3& center, float radius,
                        glm::vec3& penetration, void** penetratedObject) const {
    // center and radius are in meters, so we have to scale the _cube into world-frame
    return _cube.findSpherePenetration(center, radius, penetration);
}

// TODO: consider removing this, or switching to using getOrCreateChildElementContaining(const AACube& box)...
OctreeElementPointer OctreeElement::getOrCreateChildElementAt(float x, float y, float z, float s) {
    OctreeElementPointer child = NULL;
    // If the requested size is less than or equal to our scale, but greater than half our scale, then
    // we are the Element they are looking for.
    float ourScale = getScale();
    float halfOurScale = ourScale / 2.0f;

    if(s > ourScale) {
        qCDebug(octree, "UNEXPECTED -- OctreeElement::getOrCreateChildElementAt() s=[%f] > ourScale=[%f] ",
                (double)s, (double)ourScale);
    }

    if (s > halfOurScale) {
        return shared_from_this();
    }

    int childIndex = getMyChildContainingPoint(glm::vec3(x, y, z));

    // Now, check if we have a child at that location
    child = getChildAtIndex(childIndex);
    if (!child) {
        child = addChildAtIndex(childIndex);
    }

    // Now that we have the child to recurse down, let it answer the original question...
    return child->getOrCreateChildElementAt(x, y, z, s);
}


OctreeElementPointer OctreeElement::getOrCreateChildElementContaining(const AACube& cube) {
    OctreeElementPointer child = NULL;

    int childIndex = getMyChildContaining(cube);

    // If getMyChildContaining() returns CHILD_UNKNOWN then it means that our level
    // is the correct level for this cube
    if (childIndex == CHILD_UNKNOWN) {
        return shared_from_this();
    }

    // Now, check if we have a child at that location
    child = getChildAtIndex(childIndex);
    if (!child) {
        child = addChildAtIndex(childIndex);
    }

    // if we've made a really small child, then go ahead and use that one.
    if (child->getScale() <= SMALLEST_REASONABLE_OCTREE_ELEMENT_SCALE) {
        return child;
    }

    // Now that we have the child to recurse down, let it answer the original question...
    return child->getOrCreateChildElementContaining(cube);
}

OctreeElementPointer OctreeElement::getOrCreateChildElementContaining(const AABox& box) {
    OctreeElementPointer child = NULL;

    int childIndex = getMyChildContaining(box);

    // If getMyChildContaining() returns CHILD_UNKNOWN then it means that our level
    // is the correct level for this cube
    if (childIndex == CHILD_UNKNOWN) {
        return shared_from_this();
    }

    // Now, check if we have a child at that location
    child = getChildAtIndex(childIndex);
    if (!child) {
        child = addChildAtIndex(childIndex);
    }

    // if we've made a really small child, then go ahead and use that one.
    if (child->getScale() <= SMALLEST_REASONABLE_OCTREE_ELEMENT_SCALE) {
        return child;
    }

    // Now that we have the child to recurse down, let it answer the original question...
    return child->getOrCreateChildElementContaining(box);
}

int OctreeElement::getMyChildContaining(const AACube& cube) const {
    float ourScale = getScale();
    float cubeScale = cube.getScale();

    // TODO: consider changing this to assert()
    if (cubeScale > ourScale) {
        qCDebug(octree) << "UNEXPECTED -- OctreeElement::getMyChildContaining() -- (cubeScale > ourScale)";
        qCDebug(octree) << "    cube=" << cube;
        qCDebug(octree) << "    elements AACube=" << _cube;
        qCDebug(octree) << "    cubeScale=" << cubeScale;
        qCDebug(octree) << "    ourScale=" << ourScale;
        assert(false);
    }

    // Determine which of our children the minimum and maximum corners of the cube live in...
    glm::vec3 cubeCornerMinimum = glm::clamp(cube.getCorner(), (float)-HALF_TREE_SCALE, (float)HALF_TREE_SCALE);
    glm::vec3 cubeCornerMaximum = glm::clamp(cube.calcTopFarLeft(), (float)-HALF_TREE_SCALE, (float)HALF_TREE_SCALE);

    if (_cube.contains(cubeCornerMinimum) && _cube.contains(cubeCornerMaximum)) {
        int childIndexCubeMinimum = getMyChildContainingPoint(cubeCornerMinimum);
        int childIndexCubeMaximum = getMyChildContainingPoint(cubeCornerMaximum);

        // If the minimum and maximum corners of the cube are in two different children's cubes, then we are the containing element
        if (childIndexCubeMinimum != childIndexCubeMaximum) {
            return CHILD_UNKNOWN;
        }

        return childIndexCubeMinimum; // either would do, they are the same
    }
    return CHILD_UNKNOWN; // since cube is not contained in our element, it can't be in one of our children
}

int OctreeElement::getMyChildContaining(const AABox& box) const {
    float ourScale = getScale();
    float boxLargestScale = box.getLargestDimension();

    // TODO: consider changing this to assert()
    if(boxLargestScale > ourScale) {
        qCDebug(octree, "UNEXPECTED -- OctreeElement::getMyChildContaining() "
                "boxLargestScale=[%f] > ourScale=[%f] ", (double)boxLargestScale, (double)ourScale);
    }

    // Determine which of our children the minimum and maximum corners of the cube live in...
    glm::vec3 cubeCornerMinimum = box.getCorner();
    glm::vec3 cubeCornerMaximum = box.calcTopFarLeft();

    if (_cube.contains(cubeCornerMinimum) && _cube.contains(cubeCornerMaximum)) {
        int childIndexCubeMinimum = getMyChildContainingPoint(cubeCornerMinimum);
        int childIndexCubeMaximum = getMyChildContainingPoint(cubeCornerMaximum);

        // If the minimum and maximum corners of the cube are in two different children's cubes,
        // then we are the containing element
        if (childIndexCubeMinimum != childIndexCubeMaximum) {
            return CHILD_UNKNOWN;
        }
        return childIndexCubeMinimum; // either would do, they are the same
    }
    return CHILD_UNKNOWN; // since box is not contained in our element, it can't be in one of our children
}

int OctreeElement::getMyChildContainingPoint(const glm::vec3& point) const {
    glm::vec3 ourCenter = _cube.calcCenter();
    int childIndex = CHILD_UNKNOWN;

    // since point is not contained in our element, it can't be in one of our children
    if (!_cube.contains(point)) {
        return CHILD_UNKNOWN;
    }

    // left half
    if (point.x > ourCenter.x) {
        if (point.y > ourCenter.y) {
            // top left
            if (point.z > ourCenter.z) {
                // top left far
                childIndex = CHILD_TOP_LEFT_FAR;
            } else {
                // top left near
                childIndex = CHILD_TOP_LEFT_NEAR;
            }
        } else {
            // bottom left
            if (point.z > ourCenter.z) {
                // bottom left far
                childIndex = CHILD_BOTTOM_LEFT_FAR;
            } else {
                // bottom left near
                childIndex = CHILD_BOTTOM_LEFT_NEAR;
            }
        }
    } else {
        // right half
        if (point.y > ourCenter.y) {
            // top right
            if (point.z > ourCenter.z) {
                // top right far
                childIndex = CHILD_TOP_RIGHT_FAR;
            } else {
                // top right near
                childIndex = CHILD_TOP_RIGHT_NEAR;
            }
        } else {
            // bottom right
            if (point.z > ourCenter.z) {
                // bottom right far
                childIndex = CHILD_BOTTOM_RIGHT_FAR;
            } else {
                // bottom right near
                childIndex = CHILD_BOTTOM_RIGHT_NEAR;
            }
        }
    }
    return childIndex;
}

