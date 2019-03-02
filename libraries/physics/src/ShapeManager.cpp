//
//  ShapeManager.cpp
//  libraries/physics/src
//
//  Created by Andrew Meadows 2014.10.29
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ShapeManager.h"

#include <glm/gtx/norm.hpp>

#include <QDebug>

#include "ShapeFactory.h"

const int MAX_RING_SIZE = 256;

ShapeManager::ShapeManager() {
    _garbageRing.reserve(MAX_RING_SIZE);
}

ShapeManager::~ShapeManager() {
    int numShapes = _shapeMap.size();
    for (int i = 0; i < numShapes; ++i) {
        ShapeReference* shapeRef = _shapeMap.getAtIndex(i);
        ShapeFactory::deleteShape(shapeRef->shape);
    }
    _shapeMap.clear();
}

const btCollisionShape* ShapeManager::getShape(const ShapeInfo& info) {
    if (info.getType() == SHAPE_TYPE_NONE) {
        return nullptr;
    }
    HashKey hashKey(info.getHash());
    ShapeReference* shapeRef = _shapeMap.find(hashKey);
    if (shapeRef) {
        shapeRef->refCount++;
        return shapeRef->shape;
    }
    const btCollisionShape* shape = ShapeFactory::createShapeFromInfo(info);
    if (shape) {
        ShapeReference newRef;
        newRef.refCount = 1;
        newRef.shape = shape;
        newRef.key = info.getHash();
        _shapeMap.insert(hashKey, newRef);
    }
    return shape;
}

// private helper method
bool ShapeManager::releaseShapeByKey(uint64_t key) {
    HashKey hashKey(key);
    ShapeReference* shapeRef = _shapeMap.find(hashKey);
    if (shapeRef) {
        if (shapeRef->refCount > 0) {
            shapeRef->refCount--;
            if (shapeRef->refCount == 0) {
                // look for existing entry in _garbageRing
                int32_t ringSize = (int32_t)(_garbageRing.size());
                for (int32_t i = 0; i < ringSize; ++i) {
                    int32_t j = (_ringIndex + ringSize) % ringSize;
                    if (_garbageRing[j] == key) {
                        // already on the list, don't add it again
                        return true;
                    }
                }
                if (ringSize == MAX_RING_SIZE) {
                    // remove one
                    HashKey hashKeyToRemove(_garbageRing[_ringIndex]);
                    ShapeReference* shapeRef = _shapeMap.find(hashKeyToRemove);
                    if (shapeRef && shapeRef->refCount == 0) {
                        ShapeFactory::deleteShape(shapeRef->shape);
                        _shapeMap.remove(hashKeyToRemove);
                    }
                    // replace at _ringIndex and advance
                    _garbageRing[_ringIndex] = key;
                    _ringIndex = (_ringIndex + 1) % ringSize;
                } else {
                    // add one
                    _garbageRing.push_back(key);
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
    int numShapes = _shapeMap.size();
    for (int i = 0; i < numShapes; ++i) {
        ShapeReference* shapeRef = _shapeMap.getAtIndex(i);
        if (shape == shapeRef->shape) {
            return releaseShapeByKey(shapeRef->key);
        }
    }
    return false;
}

void ShapeManager::collectGarbage() {
    int numShapes = (int32_t)(_garbageRing.size());
    for (int i = 0; i < numShapes; ++i) {
        HashKey key(_garbageRing[i]);
        ShapeReference* shapeRef = _shapeMap.find(key);
        if (shapeRef && shapeRef->refCount == 0) {
            ShapeFactory::deleteShape(shapeRef->shape);
            _shapeMap.remove(key);
        }
    }
    _ringIndex = 0;
    _garbageRing.clear();
}

int ShapeManager::getNumReferences(const ShapeInfo& info) const {
    HashKey hashKey(info.getHash());
    const ShapeReference* shapeRef = _shapeMap.find(hashKey);
    if (shapeRef) {
        return shapeRef->refCount;
    }
    return 0;
}

int ShapeManager::getNumReferences(const btCollisionShape* shape) const {
    int numShapes = _shapeMap.size();
    for (int i = 0; i < numShapes; ++i) {
        const ShapeReference* shapeRef = _shapeMap.getAtIndex(i);
        if (shape == shapeRef->shape) {
            return shapeRef->refCount;
        }
    }
    return 0;
}

bool ShapeManager::hasShape(const btCollisionShape* shape) const {
    int numShapes = _shapeMap.size();
    for (int i = 0; i < numShapes; ++i) {
        const ShapeReference* shapeRef = _shapeMap.getAtIndex(i);
        if (shape == shapeRef->shape) {
            return true;
        }
    }
    return false;
}
