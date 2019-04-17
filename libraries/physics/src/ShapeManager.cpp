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
#include <QThreadPool>

#include <NumericalConstants.h>

const int MAX_RING_SIZE = 256;

ShapeManager::ShapeManager() {
    _garbageRing.reserve(MAX_RING_SIZE);
    _nextOrphanExpiry = std::chrono::steady_clock::now();
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
    const btCollisionShape* shape = nullptr;
    if (info.getType() == SHAPE_TYPE_STATIC_MESH) {
        uint64_t hash = info.getHash();
        const auto itr = std::find(_pendingMeshShapes.begin(), _pendingMeshShapes.end(), hash);
        if (itr == _pendingMeshShapes.end()) {
            // start a worker
            _pendingMeshShapes.push_back(hash);
            // try to recycle old deadWorker
            ShapeFactory::Worker* worker = _deadWorker;
            if (!worker) {
                worker = new ShapeFactory::Worker(info);
            } else {
                worker->shapeInfo = info;
                _deadWorker = nullptr;
            }
            // we will delete worker manually later
            worker->setAutoDelete(false);
            QObject::connect(worker, &ShapeFactory::Worker::submitWork, this, &ShapeManager::acceptWork);
            QThreadPool::globalInstance()->start(worker);
        }
        // else we're still waiting for the shape to be created on another thread
    } else {
        shape = ShapeFactory::createShapeFromInfo(info);
        if (shape) {
            ShapeReference newRef;
            newRef.refCount = 1;
            newRef.shape = shape;
            newRef.key = info.getHash();
            _shapeMap.insert(hashKey, newRef);
        }
    }
    return shape;
}

void ShapeManager::addToGarbage(uint64_t key) {
    // look for existing entry in _garbageRing
    int32_t ringSize = (int32_t)(_garbageRing.size());
    for (int32_t i = 0; i < ringSize; ++i) {
        int32_t j = (_ringIndex + ringSize) % ringSize;
        if (_garbageRing[j] == key) {
            // already on the list, don't add it again
            return;
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

// private helper method
bool ShapeManager::releaseShapeByKey(uint64_t key) {
    HashKey hashKey(key);
    ShapeReference* shapeRef = _shapeMap.find(hashKey);
    if (shapeRef) {
        if (shapeRef->refCount > 0) {
            shapeRef->refCount--;
            if (shapeRef->refCount == 0) {
                addToGarbage(key);
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

// slot: called when ShapeFactory::Worker is done building shape
void ShapeManager::acceptWork(ShapeFactory::Worker* worker) {
    auto itr = std::find(_pendingMeshShapes.begin(), _pendingMeshShapes.end(), worker->shapeInfo.getHash());
    if (itr == _pendingMeshShapes.end()) {
        // we've received a shape but don't remember asking for it
        // (should not fall in here, but if we do: delete the unwanted shape)
        if (worker->shape) {
            ShapeFactory::deleteShape(worker->shape);
        }
    } else {
        // clear pending status
        *itr = _pendingMeshShapes.back();
        _pendingMeshShapes.pop_back();

        // cache the new shape
        if (worker->shape) {
            ShapeReference newRef;
            // refCount is zero because nothing is using the shape yet
            newRef.refCount = 0;
            newRef.shape = worker->shape;
            newRef.key = worker->shapeInfo.getHash();
            HashKey hashKey(newRef.key);
            _shapeMap.insert(hashKey, newRef);

            // This shape's refCount is zero because an object requested it but is not yet using it.  We expect it to be
            // used later but there is a possibility it will never be used (e.g. the object that wanted it was removed
            // before the shape could be added, or has changed its mind and now wants a different shape).
            // Normally zero refCount shapes belong on _garbageRing for possible cleanup but we don't want to add it there
            // because it might get reaped too soon.  So we add it to _orphans to check later.  If it still has zero
            // refCount on expiry we will move it to _garbageRing.
            const int64_t SHAPE_EXPIRY = USECS_PER_SECOND;
            auto now = std::chrono::steady_clock::now();
            auto expiry = now + std::chrono::microseconds(SHAPE_EXPIRY);
            if (_nextOrphanExpiry < now) {
                // check for expired orphan shapes
                size_t i = 0;
                while (i < _orphans.size()) {
                    if (_orphans[i].expiry < now) {
                        uint64_t key = _orphans[i].key;
                        HashKey hashKey(key);
                        ShapeReference* shapeRef = _shapeMap.find(hashKey);
                        if (shapeRef) {
                            if (shapeRef->refCount == 0) {
                                // shape unused after expiry
                                addToGarbage(key);
                            }
                        }
                        _orphans[i] = _orphans.back();
                        _orphans.pop_back();
                    } else {
                        ++i;
                    }
                }
            }
            _nextOrphanExpiry = expiry;
            _orphans.push_back(KeyExpiry(newRef.key, expiry));
        }
    }
    disconnect(worker, &ShapeFactory::Worker::submitWork, this, &ShapeManager::acceptWork);

    if (_deadWorker) {
        // delete the previous deadWorker manually
        delete _deadWorker;
    }
    // save this dead worker for later
    worker->shapeInfo.clear();
    worker->shape = nullptr;
    _deadWorker = worker;
}
