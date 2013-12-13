//
//  OctreeElement.cpp
//  hifi
//
//  Created by Stephen Birarda on 3/13/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <cmath>
#include <cstring>
#include <stdio.h>

#include <QtCore/QDebug>

#include <NodeList.h>
#include <PerfStat.h>
#include <assert.h>

#include "AABox.h"
#include "OctalCode.h"
#include "SharedUtil.h"
#include "OctreeConstants.h"
#include "OctreeElement.h"
#include "Octree.h"

uint64_t OctreeElement::_voxelMemoryUsage = 0;
uint64_t OctreeElement::_octcodeMemoryUsage = 0;
uint64_t OctreeElement::_externalChildrenMemoryUsage = 0;
uint64_t OctreeElement::_voxelNodeCount = 0;
uint64_t OctreeElement::_voxelNodeLeafCount = 0;

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


    int octalCodeLength = bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(octalCode));
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
    calculateAABox();
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

void OctreeElement::calculateAABox() {
    glm::vec3 corner;
    
    // copy corner into box
    copyFirstVertexForCode(getOctalCode(),(float*)&corner);
    
    // this tells you the "size" of the voxel
    float voxelScale = 1 / powf(2, numberOfThreeBitSectionsInCode(getOctalCode()));
    _box.setBox(corner,voxelScale);
}

void OctreeElement::deleteChildAtIndex(int childIndex) {
    OctreeElement* childAt = getChildAtIndex(childIndex);
    if (childAt) {
        //printf("deleteChildAtIndex()... about to call delete childAt=%p\n",childAt);
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
        qDebug("%s... auditChildren() %s <<<< \n", label, (auditFailed ? "FAILED" : "PASSED"));
        qDebug("    _childrenExternal=%s\n", debug::valueOf(_childrenExternal));
        qDebug("    childCount=%d\n", getChildCount());
        qDebug("    _childBitmask=");
        outputBits(_childBitmask);


        for (int childIndex = 0; childIndex < NUMBER_OF_CHILDREN; childIndex++) {
            OctreeElement* testChildNew = getChildAtIndex(childIndex);
            OctreeElement* testChildOld = _childrenArray[childIndex];

            qDebug("child at index %d... testChildOld=%p testChildNew=%p %s \n",
                    childIndex, testChildOld, testChildNew ,
                    ((testChildNew != testChildOld) ? " DOES NOT MATCH <<<< BAD <<<<" : " - OK ")
            );
        }
        qDebug("%s... auditChildren() <<<< DONE <<<< \n", label);
    }
}
#endif // def HAS_AUDIT_CHILDREN


uint64_t OctreeElement::_getChildAtIndexTime = 0;
uint64_t OctreeElement::_getChildAtIndexCalls = 0;
uint64_t OctreeElement::_setChildAtIndexTime = 0;
uint64_t OctreeElement::_setChildAtIndexCalls = 0;

#ifdef BLENDED_UNION_CHILDREN
uint64_t OctreeElement::_singleChildrenCount = 0;
uint64_t OctreeElement::_twoChildrenOffsetCount = 0;
uint64_t OctreeElement::_twoChildrenExternalCount = 0;
uint64_t OctreeElement::_threeChildrenOffsetCount = 0;
uint64_t OctreeElement::_threeChildrenExternalCount = 0;
uint64_t OctreeElement::_couldStoreFourChildrenInternally = 0;
uint64_t OctreeElement::_couldNotStoreFourChildrenInternally = 0;
#endif

uint64_t OctreeElement::_externalChildrenCount = 0;
uint64_t OctreeElement::_childrenCount[NUMBER_OF_CHILDREN + 1] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

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
                            qDebug("getChildAtIndex() attempt to access external client out of bounds externalIndex=%d <<<<<<<<<< WARNING!!! \n",externalIndex);
                        }
                        break;
                    }
                }
            }
        } break;
    }
#ifdef HAS_AUDIT_CHILDREN
    if (result != _childrenArray[childIndex]) {
        qDebug("getChildAtIndex() case:%s result<%p> != _childrenArray[childIndex]<%p> <<<<<<<<<< WARNING!!! \n",
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
    const uint64_t ENCODE_BITS = 21;
    const uint64_t ENCODE_MASK = 0xFFFFF;
    const uint64_t ENCODE_MASK_SIGN = 0x100000;

    uint64_t offsetEncodedOne = (_children.offsetsThreeChildrenEncoded >> (ENCODE_BITS * 2)) & ENCODE_MASK;
    uint64_t offsetEncodedTwo = (_children.offsetsThreeChildrenEncoded >> (ENCODE_BITS * 1)) & ENCODE_MASK;
    uint64_t offsetEncodedThree = (_children.offsetsThreeChildrenEncoded & ENCODE_MASK);

    uint64_t signEncodedOne = (_children.offsetsThreeChildrenEncoded >> (ENCODE_BITS * 2)) & ENCODE_MASK_SIGN;
    uint64_t signEncodedTwo = (_children.offsetsThreeChildrenEncoded >> (ENCODE_BITS * 1)) & ENCODE_MASK_SIGN;
    uint64_t signEncodedThree = (_children.offsetsThreeChildrenEncoded & ENCODE_MASK_SIGN);

    bool oneNegative = signEncodedOne == ENCODE_MASK_SIGN;
    bool twoNegative = signEncodedTwo == ENCODE_MASK_SIGN;
    bool threeNegative = signEncodedThree == ENCODE_MASK_SIGN;

    offsetOne = oneNegative ? -offsetEncodedOne : offsetEncodedOne;
    offsetTwo = twoNegative ? -offsetEncodedTwo : offsetEncodedTwo;
    offsetThree = threeNegative ? -offsetEncodedThree : offsetEncodedThree;
}

void OctreeElement::encodeThreeOffsets(int64_t offsetOne, int64_t offsetTwo, int64_t offsetThree) {
    const uint64_t ENCODE_BITS = 21;
    const uint64_t ENCODE_MASK = 0xFFFFF;
    const uint64_t ENCODE_MASK_SIGN = 0x100000;
    
    uint64_t offsetEncodedOne, offsetEncodedTwo, offsetEncodedThree;
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
        assert(child == NULL); // we are removing a child, so this must be true!
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
        qDebug("THIS SHOULD NOT HAPPEN previousChildCount == %d && newChildCount == %d\n",previousChildCount, newChildCount);
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
        qDebug() << "OctreeElement::safeDeepDeleteChildAtIndex() reached DANGEROUSLY_DEEP_RECURSION, bailing!\n";
        return deleteApproved;
    }
    OctreeElement* childToDelete = getChildAtIndex(childIndex);
    if (childToDelete) {
        if (childToDelete->deleteApproved()) {
            // If the child is not a leaf, then call ourselves recursively on all the children
            if (!childToDelete->isLeaf()) {
                // delete all it's children
                for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
                    deleteApproved = childToDelete->safeDeepDeleteChildAtIndex(i,recursionCount+1);
                    if (!deleteApproved) {
                        break; // no point in continuing...
                    }
                }
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

    qDebug("%s - Voxel at corner=(%f,%f,%f) size=%f\n isLeaf=%s isDirty=%s shouldRender=%s\n children=", label,
        _box.getCorner().x, _box.getCorner().y, _box.getCorner().z, _box.getScale(),
        debug::valueOf(isLeaf()), debug::valueOf(isDirty()), debug::valueOf(getShouldRender()));
        
    outputBits(childBits, false);
    qDebug("\n octalCode=");
    printOctalCode(getOctalCode());
}

float OctreeElement::getEnclosingRadius() const {
    return getScale() * sqrtf(3.0f) / 2.0f;
}

bool OctreeElement::isInView(const ViewFrustum& viewFrustum) const {
    AABox box = _box; // use temporary box so we can scale it
    box.scale(TREE_SCALE);
    bool inView = (ViewFrustum::OUTSIDE != viewFrustum.boxInFrustum(box));
    return inView;
}

ViewFrustum::location OctreeElement::inFrustum(const ViewFrustum& viewFrustum) const {
    AABox box = _box; // use temporary box so we can scale it
    box.scale(TREE_SCALE);
    return viewFrustum.boxInFrustum(box);
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
        float boundary         = boundaryDistanceForRenderLevel(getLevel() + boundaryLevelAdjust, voxelScaleSize);
        float childBoundary    = boundaryDistanceForRenderLevel(getLevel() + 1 + boundaryLevelAdjust, voxelScaleSize);
        bool  inBoundary       = (furthestDistance <= boundary);
        bool  inChildBoundary  = (furthestDistance <= childBoundary);
        shouldRender = (isLeaf() && inChildBoundary) || (inBoundary && !inChildBoundary);
    }
    return shouldRender;
}

// Calculates the distance to the furthest point of the voxel to the camera
float OctreeElement::furthestDistanceToCamera(const ViewFrustum& viewFrustum) const {
    AABox box = getAABox();
    box.scale(TREE_SCALE);
    glm::vec3 furthestPoint = viewFrustum.getFurthestPointFromCamera(box);
    glm::vec3 temp = viewFrustum.getPosition() - furthestPoint;
    float distanceToVoxelCenter = sqrtf(glm::dot(temp, temp));
    return distanceToVoxelCenter;
}

float OctreeElement::distanceToCamera(const ViewFrustum& viewFrustum) const {
    glm::vec3 center = _box.calcCenter() * (float)TREE_SCALE;
    glm::vec3 temp = viewFrustum.getPosition() - center;
    float distanceToVoxelCenter = sqrtf(glm::dot(temp, temp));
    return distanceToVoxelCenter;
}

float OctreeElement::distanceSquareToPoint(const glm::vec3& point) const {
    glm::vec3 temp = point - _box.calcCenter();
    float distanceSquare = glm::dot(temp, temp);
    return distanceSquare;
}

float OctreeElement::distanceToPoint(const glm::vec3& point) const {
    glm::vec3 temp = point - _box.calcCenter();
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
    for (int i = 0; i < _deleteHooks.size(); i++) {
        if (_deleteHooks[i] == hook) {
            _deleteHooks.erase(_deleteHooks.begin() + i);
            break;
        }
    }
    _deleteHooksLock.unlock();
}

void OctreeElement::notifyDeleteHooks() {
    _deleteHooksLock.lockForRead();
    for (int i = 0; i < _deleteHooks.size(); i++) {
        _deleteHooks[i]->elementDeleted(this);
    }
    _deleteHooksLock.unlock();
}

std::vector<OctreeElementUpdateHook*> OctreeElement::_updateHooks;

void OctreeElement::addUpdateHook(OctreeElementUpdateHook* hook) {
    _updateHooks.push_back(hook);
}

void OctreeElement::removeUpdateHook(OctreeElementUpdateHook* hook) {
    for (int i = 0; i < _updateHooks.size(); i++) {
        if (_updateHooks[i] == hook) {
            _updateHooks.erase(_updateHooks.begin() + i);
            return;
        }
    }
}

void OctreeElement::notifyUpdateHooks() {
    for (int i = 0; i < _updateHooks.size(); i++) {
        _updateHooks[i]->elementUpdated(this);
    }
}

bool OctreeElement::findSpherePenetration(const glm::vec3& center, float radius, glm::vec3& penetration) const {
    return _box.findSpherePenetration(center, radius, penetration);
}


OctreeElement* OctreeElement::getOrCreateChildElementAt(float x, float y, float z, float s) {
    OctreeElement* child = NULL;
    // If the requested size is less than or equal to our scale, but greater than half our scale, then
    // we are the Element they are looking for.
    float ourScale = getScale();
    float halfOurScale = ourScale / 2.0f;

    if(s > ourScale) {
        printf("UNEXPECTED -- OctreeElement::getOrCreateChildElementAt() s=[%f] > ourScale=[%f] \n", s, ourScale);
    }

    if (s > halfOurScale) {
        return this;
    }
    // otherwise, we need to find which of our children we should recurse
    glm::vec3 ourCenter = _box.calcCenter();
    
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
