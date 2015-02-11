//
//  ShapeManager.cpp
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.10.29
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/norm.hpp>

#include "ShapeInfoUtil.h"
#include "ShapeManager.h"

ShapeManager::ShapeManager() {
}

ShapeManager::~ShapeManager() {
    int numShapes = _shapeMap.size();
    for (int i = 0; i < numShapes; ++i) {
        ShapeReference* shapeRef = _shapeMap.getAtIndex(i);
        delete shapeRef->_shape;
    }
    _shapeMap.clear();
}

btCollisionShape* ShapeManager::getShape(const ShapeInfo& info) {
    if (info.getType() == SHAPE_TYPE_NONE) {
        return NULL;
    }
    // Very small or large objects are not supported.
    float diagonal = 4.0f * glm::length2(info.getHalfExtents());
    const float MIN_SHAPE_DIAGONAL_SQUARED = 3.0e-4f; // 1 cm cube
    const float MAX_SHAPE_DIAGONAL_SQUARED = 3.0e4f;  // 100 m cube
    if (diagonal < MIN_SHAPE_DIAGONAL_SQUARED || diagonal > MAX_SHAPE_DIAGONAL_SQUARED) {
        return NULL;
    }
    DoubleHashKey key = info.getHash();
    ShapeReference* shapeRef = _shapeMap.find(key);
    if (shapeRef) {
        shapeRef->_refCount++;
        return shapeRef->_shape;
    }
    btCollisionShape* shape = ShapeInfoUtil::createShapeFromInfo(info);
    if (shape) {
        ShapeReference newRef;
        newRef._refCount = 1;
        newRef._shape = shape;
        _shapeMap.insert(key, newRef);
    }
    return shape;
}

bool ShapeManager::releaseShape(const ShapeInfo& info) {
    DoubleHashKey key = info.getHash();
    ShapeReference* shapeRef = _shapeMap.find(key);
    if (shapeRef) {
        if (shapeRef->_refCount > 0) {
            shapeRef->_refCount--;
            if (shapeRef->_refCount == 0) {
                _pendingGarbage.push_back(key);
                const int MAX_GARBAGE_CAPACITY = 127;
                if (_pendingGarbage.size() > MAX_GARBAGE_CAPACITY) {
                    collectGarbage();
                }
            }
            return true;
        } else {
            // attempt to remove shape that has no refs
            assert(false);
        }
    } else {
        // attempt to remove unmanaged shape
        assert(false);
    }
    return false;
}

bool ShapeManager::releaseShape(const btCollisionShape* shape) {
    ShapeInfo info;
    ShapeInfoUtil::collectInfoFromShape(shape, info);
    return releaseShape(info);
}

void ShapeManager::collectGarbage() {
    int numShapes = _pendingGarbage.size();
    for (int i = 0; i < numShapes; ++i) {
        DoubleHashKey& key = _pendingGarbage[i];
        ShapeReference* shapeRef = _shapeMap.find(key);
        if (shapeRef && shapeRef->_refCount == 0) {
            delete shapeRef->_shape;
            _shapeMap.remove(key);
        }
    }
    _pendingGarbage.clear();
}

int ShapeManager::getNumReferences(const ShapeInfo& info) const {
    DoubleHashKey key = info.getHash();
    const ShapeReference* shapeRef = _shapeMap.find(key);
    if (shapeRef) {
        return shapeRef->_refCount;
    }
    return -1;
}
