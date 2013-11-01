//
//  VoxelNode.cpp
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

#include "AABox.h"
#include "OctalCode.h"
#include "SharedUtil.h"
#include "VoxelConstants.h"
#include "VoxelNode.h"
#include "VoxelTree.h"

uint64_t VoxelNode::_voxelMemoryUsage = 0;
uint64_t VoxelNode::_octcodeMemoryUsage = 0;
uint64_t VoxelNode::_externalChildrenMemoryUsage = 0;
uint64_t VoxelNode::_voxelNodeCount = 0;
uint64_t VoxelNode::_voxelNodeLeafCount = 0;

VoxelNode::VoxelNode() {
    unsigned char* rootCode = new unsigned char[1];
    *rootCode = 0;
    init(rootCode);

    _voxelNodeCount++;
    _voxelNodeLeafCount++; // all nodes start as leaf nodes
}

VoxelNode::VoxelNode(unsigned char * octalCode) {
    init(octalCode);
    _voxelNodeCount++;
    _voxelNodeLeafCount++; // all nodes start as leaf nodes
}

void VoxelNode::init(unsigned char * octalCode) {
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
    
#ifndef NO_FALSE_COLOR // !NO_FALSE_COLOR means, does have false color
    _falseColored = false; // assume true color
    _currentColor[0] = _currentColor[1] = _currentColor[2] = _currentColor[3] = 0;
#endif
    _trueColor[0] = _trueColor[1] = _trueColor[2] = _trueColor[3] = 0;
    _density = 0.0f;

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
    
    _unknownBufferIndex = true;
    setBufferIndex(GLBUFFER_INDEX_UNKNOWN);

    setVoxelSystem(NULL);
    _isDirty = true;
    _shouldRender = false;
    _sourceUUIDKey = 0;
    calculateAABox();
    markWithChangedTime();

    _voxelMemoryUsage += sizeof(VoxelNode);
}

VoxelNode::~VoxelNode() {
    notifyDeleteHooks();

    _voxelMemoryUsage -= sizeof(VoxelNode);

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

void VoxelNode::markWithChangedTime() { 
    _lastChanged = usecTimestampNow(); 
    notifyUpdateHooks(); // if the node has changed, notify our hooks
}

// This method is called by VoxelTree when the subtree below this node
// is known to have changed. It's intended to be used as a place to do
// bookkeeping that a node may need to do when the subtree below it has
// changed. However, you should hopefully make your bookkeeping relatively
// localized, because this method will get called for every node in an
// recursive unwinding case like delete or add voxel
void VoxelNode::handleSubtreeChanged(VoxelTree* myTree) {
    // here's a good place to do color re-averaging...
    if (myTree->getShouldReaverage()) {
        setColorFromAverageOfChildren();
    }
    
    markWithChangedTime();
}

const uint8_t INDEX_FOR_NULL = 0;
uint8_t VoxelNode::_nextIndex = INDEX_FOR_NULL + 1; // start at 1, 0 is reserved for NULL
std::map<VoxelSystem*, uint8_t> VoxelNode::_mapVoxelSystemPointersToIndex;
std::map<uint8_t, VoxelSystem*> VoxelNode::_mapIndexToVoxelSystemPointers;

VoxelSystem* VoxelNode::getVoxelSystem() const { 
    if (_voxelSystemIndex > INDEX_FOR_NULL) {
        if (_mapIndexToVoxelSystemPointers.end() != _mapIndexToVoxelSystemPointers.find(_voxelSystemIndex)) {

            VoxelSystem* voxelSystem = _mapIndexToVoxelSystemPointers[_voxelSystemIndex]; 
            return voxelSystem;
        }
    }
    return NULL;
}

void VoxelNode::setVoxelSystem(VoxelSystem* voxelSystem) {
    if (voxelSystem == NULL) {
        _voxelSystemIndex = INDEX_FOR_NULL;
    } else {
        uint8_t index;
        if (_mapVoxelSystemPointersToIndex.end() != _mapVoxelSystemPointersToIndex.find(voxelSystem)) {
            index = _mapVoxelSystemPointersToIndex[voxelSystem];
        } else {
            index = _nextIndex;
            _nextIndex++;
            _mapVoxelSystemPointersToIndex[voxelSystem] = index;
            _mapIndexToVoxelSystemPointers[index] = voxelSystem;
        }
        _voxelSystemIndex = index;
    }
}

const uint16_t KEY_FOR_NULL = 0;
uint16_t VoxelNode::_nextUUIDKey = KEY_FOR_NULL + 1; // start at 1, 0 is reserved for NULL
std::map<QString, uint16_t> VoxelNode::_mapSourceUUIDsToKeys;
std::map<uint16_t, QString> VoxelNode::_mapKeysToSourceUUIDs;

void VoxelNode::setSourceUUID(const QUuid& sourceUUID) {
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

QUuid VoxelNode::getSourceUUID() const {
    if (_sourceUUIDKey > KEY_FOR_NULL) {
        if (_mapKeysToSourceUUIDs.end() != _mapKeysToSourceUUIDs.find(_sourceUUIDKey)) {
            return QUuid(_mapKeysToSourceUUIDs[_sourceUUIDKey]);
        }
    }
    return QUuid();
}

bool VoxelNode::matchesSourceUUID(const QUuid& sourceUUID) const {
    if (_sourceUUIDKey > KEY_FOR_NULL) {
        if (_mapKeysToSourceUUIDs.end() != _mapKeysToSourceUUIDs.find(_sourceUUIDKey)) {
            return QUuid(_mapKeysToSourceUUIDs[_sourceUUIDKey]) == sourceUUID;
        }
    }
    return sourceUUID.isNull();
}

uint16_t VoxelNode::getSourceNodeUUIDKey(const QUuid& sourceUUID) {
    uint16_t key = KEY_FOR_NULL;
    QString sourceUUIDString = sourceUUID.toString();
    if (_mapSourceUUIDsToKeys.end() != _mapSourceUUIDsToKeys.find(sourceUUIDString)) {
        key = _mapSourceUUIDsToKeys[sourceUUIDString];
    }
    return key;
}



void VoxelNode::setShouldRender(bool shouldRender) {
    // if shouldRender is changing, then consider ourselves dirty
    if (shouldRender != _shouldRender) {
        _shouldRender = shouldRender;
        _isDirty = true;
        markWithChangedTime();
    }
}

void VoxelNode::calculateAABox() {
    glm::vec3 corner;
    
    // copy corner into box
    copyFirstVertexForCode(getOctalCode(),(float*)&corner);
    
    // this tells you the "size" of the voxel
    float voxelScale = 1 / powf(2, numberOfThreeBitSectionsInCode(getOctalCode()));
    _box.setBox(corner,voxelScale);
}

void VoxelNode::deleteChildAtIndex(int childIndex) {
    VoxelNode* childAt = getChildAtIndex(childIndex);
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
VoxelNode* VoxelNode::removeChildAtIndex(int childIndex) {
    VoxelNode* returnedChild = getChildAtIndex(childIndex);
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
void VoxelNode::auditChildren(const char* label) const {
    bool auditFailed = false;
    for (int childIndex = 0; childIndex < NUMBER_OF_CHILDREN; childIndex++) {
        VoxelNode* testChildNew = getChildAtIndex(childIndex);
        VoxelNode* testChildOld = _childrenArray[childIndex];
        
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
            VoxelNode* testChildNew = getChildAtIndex(childIndex);
            VoxelNode* testChildOld = _childrenArray[childIndex];

            qDebug("child at index %d... testChildOld=%p testChildNew=%p %s \n",
                    childIndex, testChildOld, testChildNew ,
                    ((testChildNew != testChildOld) ? " DOES NOT MATCH <<<< BAD <<<<" : " - OK ")
            );
        }
        qDebug("%s... auditChildren() <<<< DONE <<<< \n", label);
    }
}
#endif // def HAS_AUDIT_CHILDREN


uint64_t VoxelNode::_getChildAtIndexTime = 0;
uint64_t VoxelNode::_getChildAtIndexCalls = 0;
uint64_t VoxelNode::_setChildAtIndexTime = 0;
uint64_t VoxelNode::_setChildAtIndexCalls = 0;

#ifdef BLENDED_UNION_CHILDREN
uint64_t VoxelNode::_singleChildrenCount = 0;
uint64_t VoxelNode::_twoChildrenOffsetCount = 0;
uint64_t VoxelNode::_twoChildrenExternalCount = 0;
uint64_t VoxelNode::_threeChildrenOffsetCount = 0;
uint64_t VoxelNode::_threeChildrenExternalCount = 0;
uint64_t VoxelNode::_couldStoreFourChildrenInternally = 0;
uint64_t VoxelNode::_couldNotStoreFourChildrenInternally = 0;
#endif

uint64_t VoxelNode::_externalChildrenCount = 0;
uint64_t VoxelNode::_childrenCount[NUMBER_OF_CHILDREN + 1] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

VoxelNode* VoxelNode::getChildAtIndex(int childIndex) const {
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
    VoxelNode* result = NULL;
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
                    result = (VoxelNode*)((uint8_t*)this + offset);
                } else if (indexTwo == childIndex) {
                    int32_t offset = _children.offsetsTwoChildren[1];
                    result = (VoxelNode*)((uint8_t*)this + offset);
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
                    result = (VoxelNode*)((uint8_t*)this + offsetOne);
                } else if (indexTwo == childIndex) {
                    result = (VoxelNode*)((uint8_t*)this + offsetTwo);
                } else if (indexThree == childIndex) {
                    result = (VoxelNode*)((uint8_t*)this + offsetThree);
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
void VoxelNode::storeTwoChildren(VoxelNode* childOne, VoxelNode* childTwo) {
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
            _externalChildrenMemoryUsage -= previousChildCount * sizeof(VoxelNode*);
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
            _externalChildrenMemoryUsage += newChildCount * sizeof(VoxelNode*);
            _children.external = new VoxelNode*[newChildCount];
        }
        _children.external[0] = childOne;
        _children.external[1] = childTwo;
        _twoChildrenExternalCount++;
    }
}

void VoxelNode::retrieveTwoChildren(VoxelNode*& childOne, VoxelNode*& childTwo) {
    // If we previously had an external array, then get the
    if (_childrenExternal) {
        childOne = _children.external[0];
        childTwo = _children.external[1];
        delete[] _children.external;
        _children.external = NULL; // probably not needed!
        _childrenExternal = false;
        _twoChildrenExternalCount--;
        const int newChildCount = 2;
        _externalChildrenMemoryUsage -= newChildCount * sizeof(VoxelNode*);
    } else {
        int64_t offsetOne = _children.offsetsTwoChildren[0];
        int64_t offsetTwo = _children.offsetsTwoChildren[1];
        childOne = (VoxelNode*)((uint8_t*)this + offsetOne);
        childTwo = (VoxelNode*)((uint8_t*)this + offsetTwo);
        _twoChildrenOffsetCount--;
    }
}

void VoxelNode::decodeThreeOffsets(int64_t& offsetOne, int64_t& offsetTwo, int64_t& offsetThree) const {
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

void VoxelNode::encodeThreeOffsets(int64_t offsetOne, int64_t offsetTwo, int64_t offsetThree) {
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

void VoxelNode::storeThreeChildren(VoxelNode* childOne, VoxelNode* childTwo, VoxelNode* childThree) {
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
            _externalChildrenMemoryUsage -= previousChildCount * sizeof(VoxelNode*);
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
            _externalChildrenMemoryUsage += newChildCount * sizeof(VoxelNode*);
            _children.external = new VoxelNode*[newChildCount];
        }
        _children.external[0] = childOne;
        _children.external[1] = childTwo;
        _children.external[2] = childThree;
        _threeChildrenExternalCount++;
    }
}

void VoxelNode::retrieveThreeChildren(VoxelNode*& childOne, VoxelNode*& childTwo, VoxelNode*& childThree) {
    // If we previously had an external array, then get the
    if (_childrenExternal) {
        childOne = _children.external[0];
        childTwo = _children.external[1];
        childThree = _children.external[2];
        delete[] _children.external;
        _children.external = NULL; // probably not needed!
        _childrenExternal = false;
        _threeChildrenExternalCount--;
        _externalChildrenMemoryUsage -= 3 * sizeof(VoxelNode*);
    } else {
        int64_t offsetOne, offsetTwo, offsetThree;
        decodeThreeOffsets(offsetOne, offsetTwo, offsetThree);

        childOne = (VoxelNode*)((uint8_t*)this + offsetOne);
        childTwo = (VoxelNode*)((uint8_t*)this + offsetTwo);
        childThree = (VoxelNode*)((uint8_t*)this + offsetThree);
        _threeChildrenOffsetCount--;
    }
}

void VoxelNode::checkStoreFourChildren(VoxelNode* childOne, VoxelNode* childTwo, VoxelNode* childThree, VoxelNode* childFour) {
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

void VoxelNode::deleteAllChildren() {
    // first delete all the VoxelNode objects...
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        VoxelNode* childAt = getChildAtIndex(i);
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

void VoxelNode::setChildAtIndex(int childIndex, VoxelNode* child) {
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
        VoxelNode* previousChild = _children.single;
        _children.external = new VoxelNode*[NUMBER_OF_CHILDREN];
        memset(_children.external, 0, sizeof(VoxelNode*) * NUMBER_OF_CHILDREN);
        _children.external[firstIndex] = previousChild;
        _children.external[childIndex] = child;

        _externalChildrenMemoryUsage += NUMBER_OF_CHILDREN * sizeof(VoxelNode*);
        
    } else if (previousChildCount == 2 && newChildCount == 1) {
        assert(child == NULL); // we are removing a child, so this must be true!
        VoxelNode* previousFirstChild = _children.external[firstIndex];
        VoxelNode* previousSecondChild = _children.external[secondIndex];
        delete[] _children.external;
        _externalChildrenMemoryUsage -= NUMBER_OF_CHILDREN * sizeof(VoxelNode*);
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

        VoxelNode* childOne;
        VoxelNode* childTwo;

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

        VoxelNode* childOne;
        VoxelNode* childTwo;

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
        VoxelNode* childOne;
        VoxelNode* childTwo;
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
        VoxelNode* childOne;
        VoxelNode* childTwo;
        VoxelNode* childThree;

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

        VoxelNode* childOne;
        VoxelNode* childTwo;
        VoxelNode* childThree;

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

        VoxelNode* childOne;
        VoxelNode* childTwo;
        VoxelNode* childThree;

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
        VoxelNode* childOne;
        VoxelNode* childTwo;
        VoxelNode* childThree;
        VoxelNode* childFour;

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
        _children.external = new VoxelNode*[newChildCount];
        _externalChildrenMemoryUsage += newChildCount * sizeof(VoxelNode*);
        
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

        VoxelNode* childOne = _children.external[0];
        VoxelNode* childTwo = _children.external[1];
        VoxelNode* childThree = _children.external[2];
        VoxelNode* childFour = _children.external[3];

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
        _externalChildrenMemoryUsage -= previousChildCount * sizeof(VoxelNode*);
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
        VoxelNode** newExternalList = new VoxelNode*[newChildCount];
        
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
        _externalChildrenMemoryUsage -= previousChildCount * sizeof(VoxelNode*);
        _externalChildrenMemoryUsage += newChildCount * sizeof(VoxelNode*);

    } else if (previousChildCount > newChildCount) {
        //assert(_children.external && _childrenExternal && previousChildCount >= 4);
        //assert(previousChildCount == newChildCount+1);

        // 4 or more children, one item being removed, we know we're stored externally, we just figure out which 
        // item to remove from our external list
        VoxelNode** newExternalList = new VoxelNode*[newChildCount];
        
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
        _externalChildrenMemoryUsage -= previousChildCount * sizeof(VoxelNode*);
        _externalChildrenMemoryUsage += newChildCount * sizeof(VoxelNode*);
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


VoxelNode* VoxelNode::addChildAtIndex(int childIndex) {
    VoxelNode* childAt = getChildAtIndex(childIndex);
    if (!childAt) {
        // before adding a child, see if we're currently a leaf 
        if (isLeaf()) {
            _voxelNodeLeafCount--;
        }
    
        childAt = new VoxelNode(childOctalCode(getOctalCode(), childIndex));
        childAt->setVoxelSystem(getVoxelSystem()); // our child is always part of our voxel system NULL ok
        setChildAtIndex(childIndex, childAt);

        _isDirty = true;
        markWithChangedTime();
    }
    return childAt;
}

// handles staging or deletion of all deep children
void VoxelNode::safeDeepDeleteChildAtIndex(int childIndex, int recursionCount) {
    if (recursionCount > DANGEROUSLY_DEEP_RECURSION) {
        qDebug() << "VoxelNode::safeDeepDeleteChildAtIndex() reached DANGEROUSLY_DEEP_RECURSION, bailing!\n";
        return;
    }
    VoxelNode* childToDelete = getChildAtIndex(childIndex);
    if (childToDelete) {
        // If the child is not a leaf, then call ourselves recursively on all the children
        if (!childToDelete->isLeaf()) {
            // delete all it's children
            for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
                childToDelete->safeDeepDeleteChildAtIndex(i,recursionCount+1);
            }
        }
        deleteChildAtIndex(childIndex);
        _isDirty = true;
        markWithChangedTime();
    }
}

// will average the child colors...
void VoxelNode::setColorFromAverageOfChildren() {
    int colorArray[4] = {0,0,0,0};
    float density = 0.0f;
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        VoxelNode* childAt = getChildAtIndex(i);
        if (childAt && childAt->isColored()) {
            for (int j = 0; j < 3; j++) {
                colorArray[j] += childAt->getTrueColor()[j]; // color averaging should always be based on true colors
            }
            colorArray[3]++;
        }
        if (childAt) {
            density += childAt->getDensity();
        }
    }
    density /= (float) NUMBER_OF_CHILDREN;    
    //
    //  The VISIBLE_ABOVE_DENSITY sets the density of matter above which an averaged color voxel will
    //  be set.  It is an important physical constant in our universe.  A number below 0.5 will cause
    //  things to get 'fatter' at a distance, because upward averaging will make larger voxels out of
    //  less data, which is (probably) going to be preferable because it gives a sense that there is
    //  something out there to go investigate.   A number above 0.5 would cause the world to become
    //  more 'empty' at a distance.  Exactly 0.5 would match the physical world, at least for materials
    //  that are not shiny and have equivalent ambient reflectance.  
    //
    const float VISIBLE_ABOVE_DENSITY = 0.10f;        
    nodeColor newColor = { 0, 0, 0, 0};
    if (density > VISIBLE_ABOVE_DENSITY) {
        // The density of material in the space of the voxel sets whether it is actually colored
        for (int c = 0; c < 3; c++) {
            // set the average color value
            newColor[c] = colorArray[c] / colorArray[3];
        }
        // set the alpha to 1 to indicate that this isn't transparent
        newColor[3] = 1;
    }
    //  Set the color from the average of the child colors, and update the density 
    setColor(newColor);
    setDensity(density);
}

// Note: !NO_FALSE_COLOR implementations of setFalseColor(), setFalseColored(), and setColor() here.
//       the actual NO_FALSE_COLOR version are inline in the VoxelNode.h
#ifndef NO_FALSE_COLOR // !NO_FALSE_COLOR means, does have false color
void VoxelNode::setFalseColor(colorPart red, colorPart green, colorPart blue) {
    if (_falseColored != true || _currentColor[0] != red || _currentColor[1] != green || _currentColor[2] != blue) {
        _falseColored=true;
        _currentColor[0] = red;
        _currentColor[1] = green;
        _currentColor[2] = blue;
        _currentColor[3] = 1; // XXXBHG - False colors are always considered set
        _isDirty = true;
        markWithChangedTime();
    }
}

void VoxelNode::setFalseColored(bool isFalseColored) {
    if (_falseColored != isFalseColored) {
        // if we were false colored, and are no longer false colored, then swap back
        if (_falseColored && !isFalseColored) {
            memcpy(&_currentColor,&_trueColor,sizeof(nodeColor));
        }
        _falseColored = isFalseColored; 
        _isDirty = true;
        _density = 1.0f;       //   If color set, assume leaf, re-averaging will update density if needed.
        markWithChangedTime();
    }
};


void VoxelNode::setColor(const nodeColor& color) {
    if (_trueColor[0] != color[0] || _trueColor[1] != color[1] || _trueColor[2] != color[2]) {
        memcpy(&_trueColor,&color,sizeof(nodeColor));
        if (!_falseColored) {
            memcpy(&_currentColor,&color,sizeof(nodeColor));
        }
        _isDirty = true;
        _density = 1.0f;       //   If color set, assume leaf, re-averaging will update density if needed.
        markWithChangedTime();
    }
}
#endif




// will detect if children are leaves AND the same color
// and in that case will delete the children and make this node
// a leaf, returns TRUE if all the leaves are collapsed into a 
// single node
bool VoxelNode::collapseIdenticalLeaves() {
    // scan children, verify that they are ALL present and accounted for
    bool allChildrenMatch = true; // assume the best (ottimista)
    int red,green,blue;
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        VoxelNode* childAt = getChildAtIndex(i);
        // if no child, child isn't a leaf, or child doesn't have a color
        if (!childAt || !childAt->isLeaf() || !childAt->isColored()) {
            allChildrenMatch=false;
            //qDebug("SADNESS child missing or not colored! i=%d\n",i);
            break;
        } else {
            if (i==0) {
                red   = childAt->getColor()[0];
                green = childAt->getColor()[1];
                blue  = childAt->getColor()[2];
            } else if (red != childAt->getColor()[0] || 
                    green != childAt->getColor()[1] || blue != childAt->getColor()[2]) {
                allChildrenMatch=false;
                break;
            }
        }
    }
    
    
    if (allChildrenMatch) {
        //qDebug("allChildrenMatch: pruning tree\n");
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            VoxelNode* childAt = getChildAtIndex(i);
            delete childAt; // delete all the child nodes
            setChildAtIndex(i, NULL); // set it to NULL
        }
        nodeColor collapsedColor;
        collapsedColor[0]=red;        
        collapsedColor[1]=green;        
        collapsedColor[2]=blue;        
        collapsedColor[3]=1;    // color is set
        setColor(collapsedColor);
    }
    return allChildrenMatch;
}

void VoxelNode::setRandomColor(int minimumBrightness) {
    nodeColor newColor;
    for (int c = 0; c < 3; c++) {
        newColor[c] = randomColorValue(minimumBrightness);
    }
    
    newColor[3] = 1;
    setColor(newColor);
}

void VoxelNode::printDebugDetails(const char* label) const {
    unsigned char childBits = 0;
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        VoxelNode* childAt = getChildAtIndex(i);
        if (childAt) {
            setAtBit(childBits,i);            
        }
    }

    qDebug("%s - Voxel at corner=(%f,%f,%f) size=%f\n isLeaf=%s isColored=%s (%d,%d,%d,%d) isDirty=%s shouldRender=%s\n children=", label,
        _box.getCorner().x, _box.getCorner().y, _box.getCorner().z, _box.getScale(),
        debug::valueOf(isLeaf()), debug::valueOf(isColored()), getColor()[0], getColor()[1], getColor()[2], getColor()[3],
        debug::valueOf(isDirty()), debug::valueOf(getShouldRender()));
        
    outputBits(childBits, false);
    qDebug("\n octalCode=");
    printOctalCode(getOctalCode());
}

float VoxelNode::getEnclosingRadius() const {
    return getScale() * sqrtf(3.0f) / 2.0f;
}

bool VoxelNode::isInView(const ViewFrustum& viewFrustum) const {
    AABox box = _box; // use temporary box so we can scale it
    box.scale(TREE_SCALE);
    bool inView = (ViewFrustum::OUTSIDE != viewFrustum.boxInFrustum(box));
    return inView;
}

ViewFrustum::location VoxelNode::inFrustum(const ViewFrustum& viewFrustum) const {
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
bool VoxelNode::calculateShouldRender(const ViewFrustum* viewFrustum, float voxelScaleSize, int boundaryLevelAdjust) const {
    bool shouldRender = false;
    if (isColored()) {
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
float VoxelNode::furthestDistanceToCamera(const ViewFrustum& viewFrustum) const {
    AABox box = getAABox();
    box.scale(TREE_SCALE);
    glm::vec3 furthestPoint = viewFrustum.getFurthestPointFromCamera(box);
    glm::vec3 temp = viewFrustum.getPosition() - furthestPoint;
    float distanceToVoxelCenter = sqrtf(glm::dot(temp, temp));
    return distanceToVoxelCenter;
}

float VoxelNode::distanceToCamera(const ViewFrustum& viewFrustum) const {
    glm::vec3 center = _box.calcCenter() * (float)TREE_SCALE;
    glm::vec3 temp = viewFrustum.getPosition() - center;
    float distanceToVoxelCenter = sqrtf(glm::dot(temp, temp));
    return distanceToVoxelCenter;
}

float VoxelNode::distanceSquareToPoint(const glm::vec3& point) const {
    glm::vec3 temp = point - _box.calcCenter();
    float distanceSquare = glm::dot(temp, temp);
    return distanceSquare;
}

float VoxelNode::distanceToPoint(const glm::vec3& point) const {
    glm::vec3 temp = point - _box.calcCenter();
    float distance = sqrtf(glm::dot(temp, temp));
    return distance;
}

std::vector<VoxelNodeDeleteHook*> VoxelNode::_deleteHooks;

void VoxelNode::addDeleteHook(VoxelNodeDeleteHook* hook) {
    _deleteHooks.push_back(hook);
}

void VoxelNode::removeDeleteHook(VoxelNodeDeleteHook* hook) {
    for (int i = 0; i < _deleteHooks.size(); i++) {
        if (_deleteHooks[i] == hook) {
            _deleteHooks.erase(_deleteHooks.begin() + i);
            return;
        }
    }
}

void VoxelNode::notifyDeleteHooks() {
    for (int i = 0; i < _deleteHooks.size(); i++) {
        _deleteHooks[i]->voxelDeleted(this);
    }
}

std::vector<VoxelNodeUpdateHook*> VoxelNode::_updateHooks;

void VoxelNode::addUpdateHook(VoxelNodeUpdateHook* hook) {
    _updateHooks.push_back(hook);
}

void VoxelNode::removeUpdateHook(VoxelNodeUpdateHook* hook) {
    for (int i = 0; i < _updateHooks.size(); i++) {
        if (_updateHooks[i] == hook) {
            _updateHooks.erase(_updateHooks.begin() + i);
            return;
        }
    }
}

void VoxelNode::notifyUpdateHooks() {
    for (int i = 0; i < _updateHooks.size(); i++) {
        _updateHooks[i]->voxelUpdated(this);
    }
}