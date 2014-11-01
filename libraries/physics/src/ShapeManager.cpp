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

#ifdef USE_BULLET_PHYSICS
#include "ShapeManager.h"

ShapeManager::ShapeManager() {
}

ShapeManager::~ShapeManager() {
    int numShapes = _shapeMap.size();
    for (int i = 0; i < numShapes; ++i) {
        ShapeReference* shapeRef = _shapeMap.getAtIndex(i);
        delete shapeRef->_shape;
    }
}


btCollisionShape* ShapeManager::getShape(const ShapeInfo& info) {
    int key = info.computeHash();
    ShapeReference* shapeRef = _shapeMap.find(key);
    if (shapeRef) {
        shapeRef->_refCount++;
        return shapeRef->_shape;
    } else {
        btCollisionShape* shape = info.createShape();
        if (shape) {
            ShapeReference newRef;
            newRef._refCount = 1;
            newRef._shape = shape;
            _shapeMap.insert(key, newRef);
        }
        return shape;
    }
}

bool ShapeManager::releaseShape(const ShapeInfo& info) {
    int key = info.computeHash();
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

/*
bool ShapeManager::releaseShape(const btCollisionShape* shape) {
    // when the number of shapes is high it's probably cheaper to try to construct a ShapeInfo 
    // and then compute the hash rather than walking the list in search of the pointer.
    int key = info.computeHash();
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
*/

void ShapeManager::collectGarbage() {
    int numShapes = _pendingGarbage.size();
    for (int i = 0; i < numShapes; ++i) {
        int key = _pendingGarbage[i];
        ShapeReference* shapeRef = _shapeMap.find(key);
        assert(shapeRef != NULL);
        if (shapeRef->_refCount == 0) {
            delete shapeRef->_shape;
            _shapeMap.remove(key);
        }
    }
    _pendingGarbage.clear();
}

int ShapeManager::getNumReferences(const ShapeInfo& info) const {
    int key = info.computeHash();
    const ShapeReference* shapeRef = _shapeMap.find(key);
    if (shapeRef) {
        return shapeRef->_refCount;
    }
    return -1;
}


#endif // USE_BULLET_PHYSICS
