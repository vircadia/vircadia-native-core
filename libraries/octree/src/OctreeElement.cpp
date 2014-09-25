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

#include <NodeList.h>
#include <PerfStat.h>
#include <AACubeShape.h>
#include <ShapeCollider.h>

#include "AACube.h"
#include "OctalCode.h"
#include "OctreeConstants.h"
#include "OctreeElement.h"
#include "Octree.h"
#include "SharedUtil.h"

quint64 OctreeElement::_voxelMemoryUsage = 0;
quint64 OctreeElement::_octcodeMemoryUsage = 0;
quint64 OctreeElement::_externalChildrenMemoryUsage = 0;
quint64 OctreeElement::_voxelNodeCount = 0;
quint64 OctreeElement::_voxelNodeLeafCount = 0;

void OctreeElement::resetPopulationStatistics() {
    _voxelNodeCount = 0;
    _voxelNodeLeafCount = 0;
}

OctreeElement::OctreeElement() {
    // Note: you must call init() from your subclass, otherwise the OctreeElement will not be properly
    // initialized. You will see DEADBEEF in your memory debugger if you have not properly called init()
    debug::setDeadBeef(this, sizeof(*this));
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

#ifdef BLENDED_UNION_CHILDREN
    _children.external = NULL;
    _singleChildrenCount++;
#endif
    _childrenCount[0]++;

    // default pointers to child nodes to NULL
#ifdef HAS_AUDIT_CHILDREN
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        _childrenArray[i] = NULL;
    }
#endif // def HAS_AUDIT_CHILDREN

#ifdef SIMPLE_CHILD_ARRAY
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        _simpleChildArray[i] = NULL;
    }
#endif

#ifdef SIMPLE_EXTERNAL_CHILDREN
    _children.single = NULL;
#endif

    _isDirty = true;
    _shouldRender = false;
    _sourceUUIDKey = 0;
    calculateAACube();
    markWithChangedTime();
}

OctreeElement::~OctreeElement() {
    notifyDeleteHooks();
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
    notifyUpdateHooks(); // if the node has changed, notify our hooks
}

// This method is called by Octree when the subtree below this node
// is known to have changed. It's intended to be used as a place to do
// bookkeeping that a node may need to do when the subtree below it has
// changed. However, you should hopefully make your bookkeeping relatively
// localized, because this method will get called for every node in an
// recursive unwinding case like delete or add voxel
void OctreeElement::handleSubtreeChanged(Octree* myTree) {
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
    glm::vec3 corner;

    // copy corner into cube
    copyFirstVertexForCode(getOctalCode(),(float*)&corner);

    // this tells you the "size" of the voxel
    float voxelScale = 1 / powf(2, numberOfThreeBitSectionsInCode(getOctalCode()));
    _cube.setBox(corner,voxelScale);
}

void OctreeElement::deleteChildAtIndex(int childIndex) {
    OctreeElement* childAt = getChildAtIndex(childIndex);
    if (childAt) {
        delete childAt;
        setChildAtIndex(childIndex, NULL);
        _isDirty = true;
        markWithChangedTime();

        // after deleting the child, check to see if we're a leaf
        if (isLeaf()) {
            _voxelNodeLeafCount++;
        }
    }
#ifdef HAS_AUDIT_CHILDREN
    auditChildren("deleteChildAtIndex()");
#endif // def HAS_AUDIT_CHILDREN
}

// does not delete the node!
OctreeElement* OctreeElement::removeChildAtIndex(int childIndex) {
    OctreeElement* returnedChild = getChildAtIndex(childIndex);
    if (returnedChild) {
        setChildAtIndex(childIndex, NULL);
        _isDirty = true;
        markWithChangedTime();

        // after removing the child, check to see if we're a leaf
        if (isLeaf()) {
            _voxelNodeLeafCount++;
        }
    }

#ifdef HAS_AUDIT_CHILDREN
    auditChildren("removeChildAtIndex()");
#endif // def HAS_AUDIT_CHILDREN
    return returnedChild;
}

bool OctreeElement::isParentOf(OctreeElement* possibleChild) const {
    if (possibleChild) {
        for (int childIndex = 0; childIndex < NUMBER_OF_CHILDREN; childIndex++) {
            OctreeElement* childAt = getChildAtIndex(childIndex);
            if (childAt == possibleChild) {
                return true;
            }
        }
    }
    return false;
}


#ifdef HAS_AUDIT_CHILDREN
void OctreeElement::auditChildren(const char* label) const {
    bool auditFailed = false;
    for (int childIndex = 0; childIndex < NUMBER_OF_CHILDREN; childIndex++) {
        OctreeElement* testChildNew = getChildAtIndex(childIndex);
        OctreeElement* testChildOld = _childrenArray[childIndex];

        if (testChildNew != testChildOld) {
            auditFailed = true;
        }
    }

    const bool alwaysReport = false; // set this to true to get additional debugging
    if (alwaysReport || auditFailed) {
        qDebug("%s... auditChildren() %s <<<<", label, (auditFailed ? "FAILED" : "PASSED"));
        qDebug("    _childrenExternal=%s", debug::valueOf(_childrenExternal));
        qDebug("    childCount=%d", getChildCount());

        QDebug bitOutput = qDebug().nospace();
        bitOutput << "    _childBitmask=";
        outputBits(_childBitmask, bitOutput);


        for (int childIndex = 0; childIndex < NUMBER_OF_CHILDREN; childIndex++) {
            OctreeElement* testChildNew = getChildAtIndex(childIndex);
            OctreeElement* testChildOld = _childrenArray[childIndex];

            qDebug("child at index %d... testChildOld=%p testChildNew=%p %s",
                    childIndex, testChildOld, testChildNew ,
                    ((testChildNew != testChildOld) ? " DOES NOT MATCH <<<< BAD <<<<" : " - OK ")
            );
        }
        qDebug("%s... auditChildren() <<<< DONE <<<<", label);
    }
}
#endif // def HAS_AUDIT_CHILDREN


quint64 OctreeElement::_getChildAtIndexTime = 0;
quint64 OctreeElement::_getChildAtIndexCalls = 0;
quint64 OctreeElement::_setChildAtIndexTime = 0;
quint64 OctreeElement::_setChildAtIndexCalls = 0;

#ifdef BLENDED_UNION_CHILDREN
quint64 OctreeElement::_singleChildrenCount = 0;
quint64 OctreeElement::_twoChildrenOffsetCount = 0;
quint64 OctreeElement::_twoChildrenExternalCount = 0;
quint64 OctreeElement::_threeChildrenOffsetCount = 0;
quint64 OctreeElement::_threeChildrenExternalCount = 0;
quint64 OctreeElement::_couldStoreFourChildrenInternally = 0;
quint64 OctreeElement::_couldNotStoreFourChildrenInternally = 0;
#endif

quint64 OctreeElement::_externalChildrenCount = 0;
quint64 OctreeElement::_childrenCount[NUMBER_OF_CHILDREN + 1] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

OctreeElement* OctreeElement::getChildAtIndex(int childIndex) const {
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
                return _children.single;
            } else {
                return NULL;
            }
        } break;

        default : {
            return _children.external[childIndex];
        } break;
    }
#endif // def SIMPLE_EXTERNAL_CHILDREN

#ifdef BLENDED_UNION_CHILDREN
    PerformanceWarning warn(false,"getChildAtIndex",false,&_getChildAtIndexTime,&_getChildAtIndexCalls);
    OctreeElement* result = NULL;
    int childCount = getChildCount();

#ifdef HAS_AUDIT_CHILDREN
    const char* caseStr = NULL;
#endif

    switch (childCount) {
        case 0:
#ifdef HAS_AUDIT_CHILDREN
            caseStr = "0 child case";
#endif
            break;
        case 1: {
#ifdef HAS_AUDIT_CHILDREN
            caseStr = "1 child case";
#endif
            int indexOne = getNthBit(_childBitmask, 1);
            if (indexOne == childIndex) {
                result = _children.single;
            }
        } break;
        case 2: {
#ifdef HAS_AUDIT_CHILDREN
            caseStr = "2 child case";
#endif
            int indexOne = getNthBit(_childBitmask, 1);
            int indexTwo = getNthBit(_childBitmask, 2);

            if (_childrenExternal) {
                //assert(_children.external);
                if (indexOne == childIndex) {
                    result = _children.external[0];
                } else if (indexTwo == childIndex) {
                    result = _children.external[1];
                }
            } else {
                if (indexOne == childIndex) {
                    int32_t offset = _children.offsetsTwoChildren[0];
                    result = (OctreeElement*)((uint8_t*)this + offset);
                } else if (indexTwo == childIndex) {
                    int32_t offset = _children.offsetsTwoChildren[1];
                    result = (OctreeElement*)((uint8_t*)this + offset);
                }
            }
        } break;
        case 3: {
#ifdef HAS_AUDIT_CHILDREN
            caseStr = "3 child case";
#endif
            int indexOne = getNthBit(_childBitmask, 1);
            int indexTwo = getNthBit(_childBitmask, 2);
            int indexThree = getNthBit(_childBitmask, 3);

            if (_childrenExternal) {
                //assert(_children.external);
                if (indexOne == childIndex) {
                    result = _children.external[0];
                } else if (indexTwo == childIndex) {
                    result = _children.external[1];
                } else if (indexThree == childIndex) {
                    result = _children.external[2];
                } else {
                }
            } else {
                int64_t offsetOne, offsetTwo, offsetThree;
                decodeThreeOffsets(offsetOne, offsetTwo, offsetThree);

                if (indexOne == childIndex) {
                    result = (OctreeElement*)((uint8_t*)this + offsetOne);
                } else if (indexTwo == childIndex) {
                    result = (OctreeElement*)((uint8_t*)this + offsetTwo);
                } else if (indexThree == childIndex) {
                    result = (OctreeElement*)((uint8_t*)this + offsetThree);
                }
            }
        } break;
        default: {
#ifdef HAS_AUDIT_CHILDREN
            caseStr = "default";
#endif
            // if we have 4 or more, we know we're in external mode, so we just need to figure out which
            // slot in our external array this child is.
            if (oneAtBit(_childBitmask, childIndex)) {
                childCount = getChildCount();
                for (int ordinal = 1; ordinal <= childCount; ordinal++) {
                    int index = getNthBit(_childBitmask, ordinal);
                    if (index == childIndex) {
                        int externalIndex = ordinal-1;
                        if (externalIndex < childCount && externalIndex >= 0) {
                            result = _children.external[externalIndex];
                        } else {
                            qDebug("getChildAtIndex() attempt to access external client out of "
                                "bounds externalIndex=%d <<<<<<<<<< WARNING!!!", externalIndex);
                        }
                        break;
                    }
                }
            }
        } break;
    }
#ifdef HAS_AUDIT_CHILDREN
    if (result != _childrenArray[childIndex]) {
        qDebug("getChildAtIndex() case:%s result<%p> != _childrenArray[childIndex]<%p> <<<<<<<<<< WARNING!!!",
            caseStr, result,_childrenArray[childIndex]);
    }
#endif // def HAS_AUDIT_CHILDREN
    return result;
#endif
}

#ifdef BLENDED_UNION_CHILDREN
void OctreeElement::storeTwoChildren(OctreeElement* childOne, OctreeElement* childTwo) {
    int64_t offsetOne = (uint8_t*)childOne - (uint8_t*)this;
    int64_t offsetTwo = (uint8_t*)childTwo - (uint8_t*)this;

    const int64_t minOffset = std::numeric_limits<int32_t>::min();
    const int64_t maxOffset = std::numeric_limits<int32_t>::max();

    bool forceExternal = true;
    if (!forceExternal && isBetween(offsetOne, maxOffset, minOffset) && isBetween(offsetTwo, maxOffset, minOffset)) {
        // if previously external, then clean it up...
        if (_childrenExternal) {
            //assert(_children.external);
            const int previousChildCount = 2;
            _externalChildrenMemoryUsage -= previousChildCount * sizeof(OctreeElement*);
            delete[] _children.external;
            _children.external = NULL; // probably not needed!
            _childrenExternal = false;
        }

        // encode in union
        _children.offsetsTwoChildren[0] = offsetOne;
        _children.offsetsTwoChildren[1] = offsetTwo;

        _twoChildrenOffsetCount++;
    } else {
        // encode in array

        // if not previously external, then allocate appropriately
        if (!_childrenExternal) {
            _childrenExternal = true;
            const int newChildCount = 2;
            _externalChildrenMemoryUsage += newChildCount * sizeof(OctreeElement*);
            _children.external = new OctreeElement*[newChildCount];
            memset(_children.external, 0, sizeof(OctreeElement*) * newChildCount);
        }
        _children.external[0] = childOne;
        _children.external[1] = childTwo;
        _twoChildrenExternalCount++;
    }
}

void OctreeElement::retrieveTwoChildren(OctreeElement*& childOne, OctreeElement*& childTwo) {
    // If we previously had an external array, then get the
    if (_childrenExternal) {
        childOne = _children.external[0];
        childTwo = _children.external[1];
        delete[] _children.external;
        _children.external = NULL; // probably not needed!
        _childrenExternal = false;
        _twoChildrenExternalCount--;
        const int newChildCount = 2;
        _externalChildrenMemoryUsage -= newChildCount * sizeof(OctreeElement*);
    } else {
        int64_t offsetOne = _children.offsetsTwoChildren[0];
        int64_t offsetTwo = _children.offsetsTwoChildren[1];
        childOne = (OctreeElement*)((uint8_t*)this + offsetOne);
        childTwo = (OctreeElement*)((uint8_t*)this + offsetTwo);
        _twoChildrenOffsetCount--;
    }
}

void OctreeElement::decodeThreeOffsets(int64_t& offsetOne, int64_t& offsetTwo, int64_t& offsetThree) const {
    const quint64 ENCODE_BITS = 21;
    const quint64 ENCODE_MASK = 0xFFFFF;
    const quint64 ENCODE_MASK_SIGN = 0x100000;

    quint64 offsetEncodedOne = (_children.offsetsThreeChildrenEncoded >> (ENCODE_BITS * 2)) & ENCODE_MASK;
    quint64 offsetEncodedTwo = (_children.offsetsThreeChildrenEncoded >> (ENCODE_BITS * 1)) & ENCODE_MASK;
    quint64 offsetEncodedThree = (_children.offsetsThreeChildrenEncoded & ENCODE_MASK);

    quint64 signEncodedOne = (_children.offsetsThreeChildrenEncoded >> (ENCODE_BITS * 2)) & ENCODE_MASK_SIGN;
    quint64 signEncodedTwo = (_children.offsetsThreeChildrenEncoded >> (ENCODE_BITS * 1)) & ENCODE_MASK_SIGN;
    quint64 signEncodedThree = (_children.offsetsThreeChildrenEncoded & ENCODE_MASK_SIGN);

    bool oneNegative = signEncodedOne == ENCODE_MASK_SIGN;
    bool twoNegative = signEncodedTwo == ENCODE_MASK_SIGN;
    bool threeNegative = signEncodedThree == ENCODE_MASK_SIGN;

    offsetOne = oneNegative ? -offsetEncodedOne : offsetEncodedOne;
    offsetTwo = twoNegative ? -offsetEncodedTwo : offsetEncodedTwo;
    offsetThree = threeNegative ? -offsetEncodedThree : offsetEncodedThree;
}

void OctreeElement::encodeThreeOffsets(int64_t offsetOne, int64_t offsetTwo, int64_t offsetThree) {
    const quint64 ENCODE_BITS = 21;
    const quint64 ENCODE_MASK = 0xFFFFF;
    const quint64 ENCODE_MASK_SIGN = 0x100000;

    quint64 offsetEncodedOne, offsetEncodedTwo, offsetEncodedThree;
    if (offsetOne < 0) {
        offsetEncodedOne = ((-offsetOne & ENCODE_MASK) | ENCODE_MASK_SIGN);
    } else {
        offsetEncodedOne = offsetOne & ENCODE_MASK;
    }
    offsetEncodedOne = offsetEncodedOne << (ENCODE_BITS * 2);

    if (offsetTwo < 0) {
        offsetEncodedTwo = ((-offsetTwo & ENCODE_MASK) | ENCODE_MASK_SIGN);
    } else {
        offsetEncodedTwo = offsetTwo & ENCODE_MASK;
    }
    offsetEncodedTwo = offsetEncodedTwo << ENCODE_BITS;

    if (offsetThree < 0) {
        offsetEncodedThree = ((-offsetThree & ENCODE_MASK) | ENCODE_MASK_SIGN);
    } else {
        offsetEncodedThree = offsetThree & ENCODE_MASK;
    }
    _children.offsetsThreeChildrenEncoded = offsetEncodedOne | offsetEncodedTwo | offsetEncodedThree;
}

void OctreeElement::storeThreeChildren(OctreeElement* childOne, OctreeElement* childTwo, OctreeElement* childThree) {
    int64_t offsetOne = (uint8_t*)childOne - (uint8_t*)this;
    int64_t offsetTwo = (uint8_t*)childTwo - (uint8_t*)this;
    int64_t offsetThree = (uint8_t*)childThree - (uint8_t*)this;

    const int64_t minOffset = -1048576; // what can fit in 20 bits // std::numeric_limits<int16_t>::min();
    const int64_t maxOffset = 1048576; // what can fit in 20 bits  // std::numeric_limits<int16_t>::max();

    bool forceExternal = true;
    if (!forceExternal &&
            isBetween(offsetOne, maxOffset, minOffset) &&
            isBetween(offsetTwo, maxOffset, minOffset) &&
            isBetween(offsetThree, maxOffset, minOffset)) {
        // if previously external, then clean it up...
        if (_childrenExternal) {
            delete[] _children.external;
            _children.external = NULL; // probably not needed!
            _childrenExternal = false;
            const int previousChildCount = 3;
            _externalChildrenMemoryUsage -= previousChildCount * sizeof(OctreeElement*);
        }
        // encode in union
        encodeThreeOffsets(offsetOne, offsetTwo, offsetThree);
        _threeChildrenOffsetCount++;
    } else {
        // encode in array

        // if not previously external, then allocate appropriately
        if (!_childrenExternal) {
            _childrenExternal = true;
            const int newChildCount = 3;
            _externalChildrenMemoryUsage += newChildCount * sizeof(OctreeElement*);
            _children.external = new OctreeElement*[newChildCount];
            memset(_children.external, 0, sizeof(OctreeElement*) * newChildCount);
        }
        _children.external[0] = childOne;
        _children.external[1] = childTwo;
        _children.external[2] = childThree;
        _threeChildrenExternalCount++;
    }
}

void OctreeElement::retrieveThreeChildren(OctreeElement*& childOne, OctreeElement*& childTwo, OctreeElement*& childThree) {
    // If we previously had an external array, then get the
    if (_childrenExternal) {
        childOne = _children.external[0];
        childTwo = _children.external[1];
        childThree = _children.external[2];
        delete[] _children.external;
        _children.external = NULL; // probably not needed!
        _childrenExternal = false;
        _threeChildrenExternalCount--;
        _externalChildrenMemoryUsage -= 3 * sizeof(OctreeElement*);
    } else {
        int64_t offsetOne, offsetTwo, offsetThree;
        decodeThreeOffsets(offsetOne, offsetTwo, offsetThree);

        childOne = (OctreeElement*)((uint8_t*)this + offsetOne);
        childTwo = (OctreeElement*)((uint8_t*)this + offsetTwo);
        childThree = (OctreeElement*)((uint8_t*)this + offsetThree);
        _threeChildrenOffsetCount--;
    }
}

void OctreeElement::checkStoreFourChildren(OctreeElement* childOne, OctreeElement* childTwo, OctreeElement* childThree, OctreeElement* childFour) {
    int64_t offsetOne = (uint8_t*)childOne - (uint8_t*)this;
    int64_t offsetTwo = (uint8_t*)childTwo - (uint8_t*)this;
    int64_t offsetThree = (uint8_t*)childThree - (uint8_t*)this;
    int64_t offsetFour = (uint8_t*)childFour - (uint8_t*)this;

    const int64_t minOffset = std::numeric_limits<int16_t>::min();
    const int64_t maxOffset = std::numeric_limits<int16_t>::max();

    bool forceExternal = true;
    if (!forceExternal &&
            isBetween(offsetOne, maxOffset, minOffset) &&
            isBetween(offsetTwo, maxOffset, minOffset) &&
            isBetween(offsetThree, maxOffset, minOffset) &&
            isBetween(offsetFour, maxOffset, minOffset)
        ) {
        _couldStoreFourChildrenInternally++;
    } else {
        _couldNotStoreFourChildrenInternally++;
    }
}
#endif

void OctreeElement::deleteAllChildren() {
    // first delete all the OctreeElement objects...
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        OctreeElement* childAt = getChildAtIndex(i);
        if (childAt) {
            delete childAt;
        }
    }

#ifdef BLENDED_UNION_CHILDREN
    // now, reset our internal state and ANY and all population data
    int childCount = getChildCount();
    switch (childCount) {
        case 0: {
            _singleChildrenCount--;
            _childrenCount[0]--;
        } break;
        case 1: {
            _singleChildrenCount--;
            _childrenCount[1]--;
        } break;

        case 2: {
            if (_childrenExternal) {
                _twoChildrenExternalCount--;
            } else {
                _twoChildrenOffsetCount--;
            }
            _childrenCount[2]--;
        } break;

        case 3: {
            if (_childrenExternal) {
                _threeChildrenExternalCount--;
            } else {
                _threeChildrenOffsetCount--;
            }
            _childrenCount[3]--;
        } break;

        default: {
            _externalChildrenCount--;
            _childrenCount[childCount]--;
        } break;


    }

    // If we had externally stored children, clean them too.
    if (_childrenExternal && _children.external) {
        delete[] _children.external;
    }
    _children.single = NULL;
#endif // BLENDED_UNION_CHILDREN
}

void OctreeElement::setChildAtIndex(int childIndex, OctreeElement* child) {
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
        _children.single = NULL;
    } else if (previousChildCount == 0 && newChildCount == 1) {
        _children.single = child;
    } else if (previousChildCount == 1 && newChildCount == 2) {
        OctreeElement* previousChild = _children.single;
        _children.external = new OctreeElement*[NUMBER_OF_CHILDREN];
        memset(_children.external, 0, sizeof(OctreeElement*) * NUMBER_OF_CHILDREN);
        _children.external[firstIndex] = previousChild;
        _children.external[childIndex] = child;

        _externalChildrenMemoryUsage += NUMBER_OF_CHILDREN * sizeof(OctreeElement*);

    } else if (previousChildCount == 2 && newChildCount == 1) {
        assert(!child); // we are removing a child, so this must be true!
        OctreeElement* previousFirstChild = _children.external[firstIndex];
        OctreeElement* previousSecondChild = _children.external[secondIndex];
        delete[] _children.external;
        _externalChildrenMemoryUsage -= NUMBER_OF_CHILDREN * sizeof(OctreeElement*);
        if (childIndex == firstIndex) {
            _children.single = previousSecondChild;
        } else {
            _children.single = previousFirstChild;
        }
    } else {
        _children.external[childIndex] = child;
    }

#endif // def SIMPLE_EXTERNAL_CHILDREN

#ifdef BLENDED_UNION_CHILDREN
    PerformanceWarning warn(false,"setChildAtIndex",false,&_setChildAtIndexTime,&_setChildAtIndexCalls);

    // Here's how we store things...
    // If we have 0 or 1 children, then we just store them in the _children.single;
    // If we have 2 children,
    //     then if we can we store them as 32 bit signed offsets from our own this pointer,
    //     _children.offsetsTwoChildren[0]-[1]
    //     these are 32 bit offsets

    unsigned char previousChildMask = _childBitmask;
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

    // If we had 0 children and we still have 0 children, then there is nothing to do.
    if (previousChildCount == 0 && newChildCount == 0) {
        // nothing to do...
    } else if ((previousChildCount == 0 || previousChildCount == 1) && newChildCount == 1) {
        // If we had 0 children, and we're setting our first child or if we had 1 child, or we're resetting the same child,
        // then we can just store it in _children.single
        _children.single = child;
    } else if (previousChildCount == 1 && newChildCount == 0) {
        // If we had 1 child, and we've removed our last child, then we can just store NULL in _children.single
        _children.single = NULL;
    } else if (previousChildCount == 1 && newChildCount == 2) {
        // If we had 1 child, and we're adding a second child, then we need to determine
        // if we can use offsets to store them

        OctreeElement* childOne;
        OctreeElement* childTwo;

        if (getNthBit(previousChildMask, 1) < childIndex) {
            childOne = _children.single;
            childTwo = child;
        } else {
            childOne = child;
            childTwo = _children.single;
        }

        _singleChildrenCount--;
        storeTwoChildren(childOne, childTwo);
    } else if (previousChildCount == 2 && newChildCount == 1) {
        // If we had 2 children, and we're removing one, then we know we can go down to single mode
        //assert(child == NULL); // this is the only logical case

        int indexTwo = getNthBit(previousChildMask, 2);
        bool keepChildOne = indexTwo == childIndex;

        OctreeElement* childOne;
        OctreeElement* childTwo;

        retrieveTwoChildren(childOne, childTwo);

        _singleChildrenCount++;

        if (keepChildOne) {
            _children.single = childOne;
        } else {
            _children.single = childTwo;
        }
    } else if (previousChildCount == 2 && newChildCount == 2) {
        // If we had 2 children, and still have 2, then we know we are resetting one of our existing children

        int indexOne = getNthBit(previousChildMask, 1);
        bool replaceChildOne = indexOne == childIndex;

        // Get the existing two children out of their encoding...
        OctreeElement* childOne;
        OctreeElement* childTwo;
        retrieveTwoChildren(childOne, childTwo);

        if (replaceChildOne) {
            childOne = child;
        } else {
            childTwo = child;
        }

        storeTwoChildren(childOne, childTwo);

    } else if (previousChildCount == 2 && newChildCount == 3) {
        // If we had 2 children, and now have 3, then we know we are going to an external case...

        // First, decode the children...
        OctreeElement* childOne;
        OctreeElement* childTwo;
        OctreeElement* childThree;

        // Get the existing two children out of their encoding...
        retrieveTwoChildren(childOne, childTwo);

        // determine order of the existing children
        int indexOne = getNthBit(previousChildMask, 1);
        int indexTwo = getNthBit(previousChildMask, 2);

        if (childIndex < indexOne) {
            childThree = childTwo;
            childTwo = childOne;
            childOne = child;
        } else if (childIndex < indexTwo) {
            childThree = childTwo;
            childTwo = child;
        } else {
            childThree = child;
        }
        storeThreeChildren(childOne, childTwo, childThree);
    } else if (previousChildCount == 3 && newChildCount == 2) {
        // If we had 3 children, and now have 2, then we know we are going from an external case to a potential internal case

        // We need to determine which children we had, and which one we got rid of...
        int indexOne = getNthBit(previousChildMask, 1);
        int indexTwo = getNthBit(previousChildMask, 2);

        bool removeChildOne = indexOne == childIndex;
        bool removeChildTwo = indexTwo == childIndex;

        OctreeElement* childOne;
        OctreeElement* childTwo;
        OctreeElement* childThree;

        // Get the existing two children out of their encoding...
        retrieveThreeChildren(childOne, childTwo, childThree);

        if (removeChildOne) {
            childOne = childTwo;
            childTwo = childThree;
        } else if (removeChildTwo) {
            childTwo = childThree;
        } else {
            // removing child three, nothing to do.
        }

        storeTwoChildren(childOne, childTwo);
    } else if (previousChildCount == 3 && newChildCount == 3) {
        // If we had 3 children, and now have 3, then we need to determine which item we're replacing...

        // We need to determine which children we had, and which one we got rid of...
        int indexOne = getNthBit(previousChildMask, 1);
        int indexTwo = getNthBit(previousChildMask, 2);

        bool replaceChildOne = indexOne == childIndex;
        bool replaceChildTwo = indexTwo == childIndex;

        OctreeElement* childOne;
        OctreeElement* childTwo;
        OctreeElement* childThree;

        // Get the existing two children out of their encoding...
        retrieveThreeChildren(childOne, childTwo, childThree);

        if (replaceChildOne) {
            childOne = child;
        } else if (replaceChildTwo) {
            childTwo = child;
        } else {
            childThree = child;
        }

        storeThreeChildren(childOne, childTwo, childThree);
    } else if (previousChildCount == 3 && newChildCount == 4) {
        // If we had 3 children, and now have 4, then we know we are going to an external case...

        // First, decode the children...
        OctreeElement* childOne;
        OctreeElement* childTwo;
        OctreeElement* childThree;
        OctreeElement* childFour;

        // Get the existing two children out of their encoding...
        retrieveThreeChildren(childOne, childTwo, childThree);

        // determine order of the existing children
        int indexOne = getNthBit(previousChildMask, 1);
        int indexTwo = getNthBit(previousChildMask, 2);
        int indexThree = getNthBit(previousChildMask, 3);

        if (childIndex < indexOne) {
            childFour = childThree;
            childThree = childTwo;
            childTwo = childOne;
            childOne = child;
        } else if (childIndex < indexTwo) {
            childFour = childThree;
            childThree = childTwo;
            childTwo = child;
        } else if (childIndex < indexThree) {
            childFour = childThree;
            childThree = child;
        } else {
            childFour = child;
        }

        // now, allocate the external...
        _childrenExternal = true;
        const int newChildCount = 4;
        _children.external = new OctreeElement*[newChildCount];
        memset(_children.external, 0, sizeof(OctreeElement*) * newChildCount);

        _externalChildrenMemoryUsage += newChildCount * sizeof(OctreeElement*);

        _children.external[0] = childOne;
        _children.external[1] = childTwo;
        _children.external[2] = childThree;
        _children.external[3] = childFour;
        _externalChildrenCount++;
    } else if (previousChildCount == 4 && newChildCount == 3) {
        // If we had 4 children, and now have 3, then we know we are going from an external case to a potential internal case
        //assert(_children.external && _childrenExternal && previousChildCount == 4);

        // We need to determine which children we had, and which one we got rid of...
        int indexOne = getNthBit(previousChildMask, 1);
        int indexTwo = getNthBit(previousChildMask, 2);
        int indexThree = getNthBit(previousChildMask, 3);

        bool removeChildOne = indexOne == childIndex;
        bool removeChildTwo = indexTwo == childIndex;
        bool removeChildThree = indexThree == childIndex;

        OctreeElement* childOne = _children.external[0];
        OctreeElement* childTwo = _children.external[1];
        OctreeElement* childThree = _children.external[2];
        OctreeElement* childFour = _children.external[3];

        if (removeChildOne) {
            childOne = childTwo;
            childTwo = childThree;
            childThree = childFour;
        } else if (removeChildTwo) {
            childTwo = childThree;
            childThree = childFour;
        } else if (removeChildThree) {
            childThree = childFour;
        } else {
            // removing child four, nothing to do.
        }

        // clean up the external children...
        _childrenExternal = false;
        delete[] _children.external;
        _children.external = NULL;
        _externalChildrenCount--;
        _externalChildrenMemoryUsage -= previousChildCount * sizeof(OctreeElement*);
        storeThreeChildren(childOne, childTwo, childThree);
    } else if (previousChildCount == newChildCount) {
        //assert(_children.external && _childrenExternal && previousChildCount >= 4);
        //assert(previousChildCount == newChildCount);

        // 4 or more children, one item being replaced, we know we're stored externally, we just need to find the one
        // that needs to be replaced and replace it.
        for (int ordinal = 1; ordinal <= 8; ordinal++) {
            int index = getNthBit(previousChildMask, ordinal);
            if (index == childIndex) {
                // this is our child to be replaced
                int nthChild = ordinal-1;
                _children.external[nthChild] = child;
                break;
            }
        }
    } else if (previousChildCount < newChildCount) {
        // Growing case... previous must be 4 or greater
        //assert(_children.external && _childrenExternal && previousChildCount >= 4);
        //assert(previousChildCount == newChildCount-1);

        // 4 or more children, one item being added, we know we're stored externally, we just figure out where to insert
        // this child pointer into our external list
        OctreeElement** newExternalList = new OctreeElement*[newChildCount];
        memset(newExternalList, 0, sizeof(OctreeElement*) * newChildCount);

        int copiedCount = 0;
        for (int ordinal = 1; ordinal <= newChildCount; ordinal++) {
            int index = getNthBit(previousChildMask, ordinal);
            if (index != -1 && index < childIndex) {
                newExternalList[ordinal - 1] = _children.external[ordinal - 1];
                copiedCount++;
            } else {

                // insert our new child here...
                newExternalList[ordinal - 1] = child;

                // if we didn't copy all of our previous children, then we need to
                if (copiedCount < previousChildCount) {
                    // our child needs to be inserted before this index, and everything else pushed out...
                    for (int oldOrdinal = ordinal; oldOrdinal <= previousChildCount; oldOrdinal++) {
                        newExternalList[oldOrdinal] = _children.external[oldOrdinal - 1];
                    }
                }
                break;
            }
        }
        delete[] _children.external;
        _children.external = newExternalList;
        _externalChildrenMemoryUsage -= previousChildCount * sizeof(OctreeElement*);
        _externalChildrenMemoryUsage += newChildCount * sizeof(OctreeElement*);

    } else if (previousChildCount > newChildCount) {
        //assert(_children.external && _childrenExternal && previousChildCount >= 4);
        //assert(previousChildCount == newChildCount+1);

        // 4 or more children, one item being removed, we know we're stored externally, we just figure out which
        // item to remove from our external list
        OctreeElement** newExternalList = new OctreeElement*[newChildCount];

        for (int ordinal = 1; ordinal <= previousChildCount; ordinal++) {
            int index = getNthBit(previousChildMask, ordinal);
            //assert(index != -1);
            if (index < childIndex) {
                newExternalList[ordinal - 1] = _children.external[ordinal - 1];
            } else {
                // our child needs to be removed from here, and everything else pulled in...
                for (int moveOrdinal = ordinal; moveOrdinal <= newChildCount; moveOrdinal++) {
                    newExternalList[moveOrdinal - 1] = _children.external[moveOrdinal];
                }
                break;
            }
        }
        delete[] _children.external;
        _children.external = newExternalList;
        _externalChildrenMemoryUsage -= previousChildCount * sizeof(OctreeElement*);
        _externalChildrenMemoryUsage += newChildCount * sizeof(OctreeElement*);
    } else {
        //assert(false);
        qDebug("THIS SHOULD NOT HAPPEN previousChildCount == %d && newChildCount == %d",previousChildCount, newChildCount);
    }

    // check to see if we could store these 4 children locally
    if (getChildCount() == 4 && _childrenExternal && _children.external) {
        checkStoreFourChildren(_children.external[0], _children.external[1], _children.external[2], _children.external[3]);
    }


#ifdef HAS_AUDIT_CHILDREN
    _childrenArray[childIndex] = child;
    auditChildren("setChildAtIndex()");
#endif // def HAS_AUDIT_CHILDREN

#endif
}


OctreeElement* OctreeElement::addChildAtIndex(int childIndex) {
    OctreeElement* childAt = getChildAtIndex(childIndex);
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
    }
    return childAt;
}

// handles staging or deletion of all deep children
bool OctreeElement::safeDeepDeleteChildAtIndex(int childIndex, int recursionCount) {
    bool deleteApproved = false;
    if (recursionCount > DANGEROUSLY_DEEP_RECURSION) {
        qDebug() << "OctreeElement::safeDeepDeleteChildAtIndex() reached DANGEROUSLY_DEEP_RECURSION, bailing!";
        return deleteApproved;
    }
    OctreeElement* childToDelete = getChildAtIndex(childIndex);
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
        OctreeElement* childAt = getChildAtIndex(i);
        if (childAt) {
            setAtBit(childBits,i);
        }
    }
    
    QDebug elementDebug = qDebug().nospace();

    QString resultString;
    resultString.sprintf("%s - Voxel at corner=(%f,%f,%f) size=%f\n isLeaf=%s isDirty=%s shouldRender=%s\n children=", label,
                         _cube.getCorner().x, _cube.getCorner().y, _cube.getCorner().z, _cube.getScale(),
                         debug::valueOf(isLeaf()), debug::valueOf(isDirty()), debug::valueOf(getShouldRender()));
    elementDebug << resultString;

    outputBits(childBits, &elementDebug);
    qDebug("octalCode=");
    printOctalCode(getOctalCode());
}

float OctreeElement::getEnclosingRadius() const {
    return getScale() * sqrtf(3.0f) / 2.0f;
}

ViewFrustum::location OctreeElement::inFrustum(const ViewFrustum& viewFrustum) const {
    AACube cube = _cube; // use temporary cube so we can scale it
    cube.scale(TREE_SCALE);
    return viewFrustum.cubeInFrustum(cube);
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
bool OctreeElement::calculateShouldRender(const ViewFrustum* viewFrustum, float voxelScaleSize, int boundaryLevelAdjust) const {
    bool shouldRender = false;
    
    if (hasContent()) {
        float furthestDistance = furthestDistanceToCamera(*viewFrustum);
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
    viewFrustum.getFurthestPointFromCameraVoxelScale(getAACube(), furthestPoint);
    glm::vec3 temp = viewFrustum.getPositionVoxelScale() - furthestPoint;
    float distanceToFurthestPoint = sqrtf(glm::dot(temp, temp));
    return distanceToFurthestPoint * (float)TREE_SCALE;
}

float OctreeElement::distanceToCamera(const ViewFrustum& viewFrustum) const {
    glm::vec3 center = _cube.calcCenter() * (float)TREE_SCALE;
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

QReadWriteLock OctreeElement::_deleteHooksLock;
std::vector<OctreeElementDeleteHook*> OctreeElement::_deleteHooks;

void OctreeElement::addDeleteHook(OctreeElementDeleteHook* hook) {
    _deleteHooksLock.lockForWrite();
    _deleteHooks.push_back(hook);
    _deleteHooksLock.unlock();
}

void OctreeElement::removeDeleteHook(OctreeElementDeleteHook* hook) {
    _deleteHooksLock.lockForWrite();
    for (unsigned int i = 0; i < _deleteHooks.size(); i++) {
        if (_deleteHooks[i] == hook) {
            _deleteHooks.erase(_deleteHooks.begin() + i);
            break;
        }
    }
    _deleteHooksLock.unlock();
}

void OctreeElement::notifyDeleteHooks() {
    _deleteHooksLock.lockForRead();
    for (unsigned int i = 0; i < _deleteHooks.size(); i++) {
        _deleteHooks[i]->elementDeleted(this);
    }
    _deleteHooksLock.unlock();
}

std::vector<OctreeElementUpdateHook*> OctreeElement::_updateHooks;

void OctreeElement::addUpdateHook(OctreeElementUpdateHook* hook) {
    _updateHooks.push_back(hook);
}

void OctreeElement::removeUpdateHook(OctreeElementUpdateHook* hook) {
    for (unsigned int i = 0; i < _updateHooks.size(); i++) {
        if (_updateHooks[i] == hook) {
            _updateHooks.erase(_updateHooks.begin() + i);
            return;
        }
    }
}

void OctreeElement::notifyUpdateHooks() {
    for (unsigned int i = 0; i < _updateHooks.size(); i++) {
        _updateHooks[i]->elementUpdated(this);
    }
}

bool OctreeElement::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face, 
                         void** intersectedObject) {

    keepSearching = true; // assume that we will continue searching after this.

    AACube cube = getAACube();
    float localDistance;
    BoxFace localFace;

    // if the ray doesn't intersect with our cube, we can stop searching!
    if (!cube.findRayIntersection(origin, direction, localDistance, localFace)) {
        keepSearching = false; // no point in continuing to search
        return false; // we did not intersect
    }

    // by default, we only allow intersections with leaves with content
    if (!canRayIntersect()) {
        return false; // we don't intersect with non-leaves, and we keep searching
    }

    // we did hit this element, so calculate appropriate distances    
    localDistance *= TREE_SCALE;
    if (localDistance < distance) {
        if (findDetailedRayIntersection(origin, direction, keepSearching,
                                        element, distance, face, intersectedObject)) {
            distance = localDistance;
            face = localFace;
            return true;
        }
    }
    return false;
}

bool OctreeElement::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face, 
                         void** intersectedObject) {

    // we did hit this element, so calculate appropriate distances    
    if (hasContent()) {
        element = this;
        if (intersectedObject) {
            *intersectedObject = this;
        }
        keepSearching = false;
        return true; // we did intersect
    }
    return false; // we did not intersect
}

bool OctreeElement::findSpherePenetration(const glm::vec3& center, float radius,
                        glm::vec3& penetration, void** penetratedObject) const {
    return _cube.findSpherePenetration(center, radius, penetration);
}

bool OctreeElement::findShapeCollisions(const Shape* shape, CollisionList& collisions) const {
    AACube cube = getAACube();
    cube.scale(TREE_SCALE);
    return ShapeCollider::collideShapeWithAACubeLegacy(shape, cube.calcCenter(), cube.getScale(), collisions);
}

// TODO: consider removing this, or switching to using getOrCreateChildElementContaining(const AACube& box)...
OctreeElement* OctreeElement::getOrCreateChildElementAt(float x, float y, float z, float s) {
    OctreeElement* child = NULL;
    // If the requested size is less than or equal to our scale, but greater than half our scale, then
    // we are the Element they are looking for.
    float ourScale = getScale();
    float halfOurScale = ourScale / 2.0f;

    if(s > ourScale) {
        qDebug("UNEXPECTED -- OctreeElement::getOrCreateChildElementAt() s=[%f] > ourScale=[%f] ", s, ourScale);
    }

    if (s > halfOurScale) {
        return this;
    }
    // otherwise, we need to find which of our children we should recurse
    glm::vec3 ourCenter = _cube.calcCenter();

    int childIndex = CHILD_UNKNOWN;
    // left half
    if (x > ourCenter.x) {
        if (y > ourCenter.y) {
            // top left
            if (z > ourCenter.z) {
                // top left far
                childIndex = CHILD_TOP_LEFT_FAR;
            } else {
                // top left near
                childIndex = CHILD_TOP_LEFT_NEAR;
            }
        } else {
            // bottom left
            if (z > ourCenter.z) {
                // bottom left far
                childIndex = CHILD_BOTTOM_LEFT_FAR;
            } else {
                // bottom left near
                childIndex = CHILD_BOTTOM_LEFT_NEAR;
            }
        }
    } else {
        // right half
        if (y > ourCenter.y) {
            // top right
            if (z > ourCenter.z) {
                // top right far
                childIndex = CHILD_TOP_RIGHT_FAR;
            } else {
                // top right near
                childIndex = CHILD_TOP_RIGHT_NEAR;
            }
        } else {
            // bottom right
            if (z > ourCenter.z) {
                // bottom right far
                childIndex = CHILD_BOTTOM_RIGHT_FAR;
            } else {
                // bottom right near
                childIndex = CHILD_BOTTOM_RIGHT_NEAR;
            }
        }
    }

    // Now, check if we have a child at that location
    child = getChildAtIndex(childIndex);
    if (!child) {
        child = addChildAtIndex(childIndex);
    }

    // Now that we have the child to recurse down, let it answer the original question...
    return child->getOrCreateChildElementAt(x, y, z, s);
}


OctreeElement* OctreeElement::getOrCreateChildElementContaining(const AACube& cube) {
    OctreeElement* child = NULL;
    
    int childIndex = getMyChildContaining(cube);
    
    // If getMyChildContaining() returns CHILD_UNKNOWN then it means that our level
    // is the correct level for this cube
    if (childIndex == CHILD_UNKNOWN) {
        return this;
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

OctreeElement* OctreeElement::getOrCreateChildElementContaining(const AABox& box) {
    OctreeElement* child = NULL;
    
    int childIndex = getMyChildContaining(box);
    
    // If getMyChildContaining() returns CHILD_UNKNOWN then it means that our level
    // is the correct level for this cube
    if (childIndex == CHILD_UNKNOWN) {
        return this;
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
        qDebug() << "UNEXPECTED -- OctreeElement::getMyChildContaining() -- (cubeScale > ourScale)";
        qDebug() << "    cube=" << cube;
        qDebug() << "    elements AACube=" << getAACube();
        qDebug() << "    cubeScale=" << cubeScale;
        qDebug() << "    ourScale=" << ourScale;
        assert(false);
    }

    // Determine which of our children the minimum and maximum corners of the cube live in...
    glm::vec3 cubeCornerMinimum = glm::clamp(cube.getCorner(), 0.0f, 1.0f);
    glm::vec3 cubeCornerMaximum = glm::clamp(cube.calcTopFarLeft(), 0.0f, 1.0f);

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
        qDebug("UNEXPECTED -- OctreeElement::getMyChildContaining() "
                    "boxLargestScale=[%f] > ourScale=[%f] ", boxLargestScale, ourScale);
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

