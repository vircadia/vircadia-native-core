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

#include "ShapeFactory.h"
#include "ShapeManager.h"

ShapeManager::ShapeManager() {
}

ShapeManager::~ShapeManager() {
    int numShapes = _shapeMap.size();
    for (int i = 0; i < numShapes; ++i) {
        ShapeReference* shapeRef = _shapeMap.getAtIndex(i);
        delete shapeRef->shape;
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
    //const float MAX_SHAPE_DIAGONAL_SQUARED = 3.0e6f;  // 1000 m cube
    if (diagonal < MIN_SHAPE_DIAGONAL_SQUARED /* || diagonal > MAX_SHAPE_DIAGONAL_SQUARED*/ ) {
        // qCDebug(physics) << "ShapeManager::getShape -- not making shape due to size" << diagonal;
        return NULL;
    }
    DoubleHashKey key = info.getHash();
    ShapeReference* shapeRef = _shapeMap.find(key);
    if (shapeRef) {
        shapeRef->refCount++;
        return shapeRef->shape;
    }
    btCollisionShape* shape = ShapeFactory::createShapeFromInfo(info);
    if (shape) {
        ShapeReference newRef;
        newRef.refCount = 1;
        newRef.shape = shape;
        newRef.key = key;
        _shapeMap.insert(key, newRef);
    }
    return shape;
}

// private helper method
bool ShapeManager::releaseShape(const DoubleHashKey& key) {
    ShapeReference* shapeRef = _shapeMap.find(key);
    if (shapeRef) {
        if (shapeRef->refCount > 0) {
            shapeRef->refCount--;
            if (shapeRef->refCount == 0) {
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

bool ShapeManager::releaseShape(const ShapeInfo& info) {
    return releaseShape(info.getHash());
}

bool ShapeManager::releaseShape(const btCollisionShape* shape) {
    int numShapes = _shapeMap.size();
    for (int i = 0; i < numShapes; ++i) {
        ShapeReference* shapeRef = _shapeMap.getAtIndex(i);
        if (shape == shapeRef->shape) {
            return releaseShape(shapeRef->key);
        }
    }
    return false;
}

void ShapeManager::collectGarbage() {
    int numShapes = _pendingGarbage.size();
    for (int i = 0; i < numShapes; ++i) {
        DoubleHashKey& key = _pendingGarbage[i];
        ShapeReference* shapeRef = _shapeMap.find(key);
        if (shapeRef && shapeRef->refCount == 0) {
            // if the shape we're about to delete is compound, delete the children first.
            if (shapeRef->shape->getShapeType() == COMPOUND_SHAPE_PROXYTYPE) {
                const btCompoundShape* compoundShape = static_cast<const btCompoundShape*>(shapeRef->shape);
                const int numChildShapes = compoundShape->getNumChildShapes();
                for (int i = 0; i < numChildShapes; i ++) {
                    const btCollisionShape* childShape = compoundShape->getChildShape(i);
                    delete childShape;
                }
            }

            delete shapeRef->shape;
            _shapeMap.remove(key);
        }
    }
    _pendingGarbage.clear();
}

int ShapeManager::getNumReferences(const ShapeInfo& info) const {
    DoubleHashKey key = info.getHash();
    const ShapeReference* shapeRef = _shapeMap.find(key);
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
