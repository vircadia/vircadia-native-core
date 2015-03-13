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

#include <QDebug>
#include <glm/gtx/norm.hpp>

#include "ShapeManager.h"
#include "ShapeInfoUtil.h"

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
        // qDebug() << "OOOOOO type is null";
        return NULL;
    }
    // Very small or large objects are not supported.
    float diagonal = 4.0f * glm::length2(info.getHalfExtents());
    const float MIN_SHAPE_DIAGONAL_SQUARED = 3.0e-4f; // 1 cm cube
    const float MAX_SHAPE_DIAGONAL_SQUARED = 3.0e4f;  // 100 m cube
    if (diagonal < MIN_SHAPE_DIAGONAL_SQUARED || diagonal > MAX_SHAPE_DIAGONAL_SQUARED) {
        // qDebug() << "OOOOOO too small " << diagonal << MIN_SHAPE_DIAGONAL_SQUARED << MAX_SHAPE_DIAGONAL_SQUARED;
        return NULL;
    }
    DoubleHashKey key = info.getHash();
    ShapeReference* shapeRef = _shapeMap.find(key);
    if (shapeRef) {
        // qDebug() << "OOOOOO ref";
        shapeRef->_refCount++;
        return shapeRef->_shape;
    }
    btCollisionShape* shape = ShapeInfoUtil::createShapeFromInfo(info);
    if (shape) {
        // qDebug() << "OOOOOO shape";
        ShapeReference newRef;
        newRef._refCount = 1;
        newRef._shape = shape;
        _shapeMap.insert(key, newRef);
    }
    return shape;
}


void ShapeManager::dereferenceShapeReferece(ShapeReference* shapeRef, DoubleHashKey key) {
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
            return;
        } else {
            // attempt to remove shape that has no refs
            assert(false);
        }
    } else {
        // attempt to remove unmanaged shape
        assert(false);
    }
    assert(false);
}


void ShapeManager::releaseShape(const ShapeInfo& info) {
    DoubleHashKey key = info.getHash();
    ShapeReference* shapeRef = _shapeMap.find(key);
    dereferenceShapeReferece(shapeRef, key);
}

void ShapeManager::releaseShape(const btCollisionShape* shape) {
    // XXX make a table for this
    int numShapes = _shapeMap.size();
    for (int i = 0; i < numShapes; ++i) {
        ShapeReference* shapeRef = _shapeMap.getAtIndex(i);
        if (shapeRef->_shape == shape) {
            // XXX how can I find the key?
            // dereferenceShapeReferece(shapeRef);
            break;
        }
    }


    // ShapeInfo info;
    // ShapeInfoUtil::collectInfoFromShape(shape, info);
    // releaseShape(info);
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
