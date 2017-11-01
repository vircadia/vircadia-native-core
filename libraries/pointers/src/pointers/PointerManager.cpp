//
//  Created by Bradley Austin Davis on 2017/10/16
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PointerManager.h"

std::shared_ptr<Pointer> PointerManager::find(unsigned int uid) const {
    return resultWithReadLock<std::shared_ptr<Pointer>>([&] {
        auto itr = _pointers.find(uid);
        if (itr != _pointers.end()) {
            return *itr;
        }
        return std::shared_ptr<Pointer>();
    });
}

unsigned int PointerManager::addPointer(std::shared_ptr<Pointer> pointer) {
    unsigned int result = 0;
    if (pointer->getRayUID() > 0) {
        withWriteLock([&] {
            // Don't let the pointer IDs overflow
            if (_nextPointerID < UINT32_MAX) {
                result = _nextPointerID++;
                _pointers[result] = pointer;
            }
        });
    }
    return result;
}

void PointerManager::removePointer(unsigned int uid) {
    withWriteLock([&] {
        _pointers.remove(uid);
    });
}

void PointerManager::enablePointer(unsigned int uid) const {
    auto pointer = find(uid);
    if (pointer) {
        pointer->enable();
    }
}

void PointerManager::disablePointer(unsigned int uid) const {
    auto pointer = find(uid);
    if (pointer) {
        pointer->disable();
    }
}

void PointerManager::setRenderState(unsigned int uid, const std::string& renderState) const {
    auto pointer = find(uid);
    if (pointer) {
        pointer->setRenderState(renderState);
    }
}

void PointerManager::editRenderState(unsigned int uid, const std::string& state, const QVariant& startProps, const QVariant& pathProps, const QVariant& endProps) const {
    auto pointer = find(uid);
    if (pointer) {
        pointer->editRenderState(state, startProps, pathProps, endProps);
    }
}

const QVariantMap PointerManager::getPrevPickResult(unsigned int uid) const {
    auto pointer = find(uid);
    if (pointer) {
        return pointer->getPrevPickResult();
    }
    return QVariantMap();
}

void PointerManager::update() {
    auto cachedPointers = resultWithReadLock<QHash<unsigned int, std::shared_ptr<Pointer>>>([&] {
        return _pointers;
    });

    for (const auto& uid : cachedPointers.keys()) {
        cachedPointers[uid]->update(uid);
    }
}

void PointerManager::setPrecisionPicking(unsigned int uid, bool precisionPicking) const {
    auto pointer = find(uid);
    if (pointer) {
        pointer->setPrecisionPicking(precisionPicking);
    }
}

void PointerManager::setIgnoreItems(unsigned int uid, const QVector<QUuid>& ignoreEntities) const {
    auto pointer = find(uid);
    if (pointer) {
        pointer->setIgnoreItems(ignoreEntities);
    }
}

void PointerManager::setIncludeItems(unsigned int uid, const QVector<QUuid>& includeEntities) const {
    auto pointer = find(uid);
    if (pointer) {
        pointer->setIncludeItems(includeEntities);
    }
}

void PointerManager::setLength(unsigned int uid, float length) const {
    auto pointer = find(uid);
    if (pointer) {
        pointer->setLength(length);
    }
}

void PointerManager::setLockEndUUID(unsigned int uid, const QUuid& objectID, bool isOverlay) const {
    auto pointer = find(uid);
    if (pointer) {
        pointer->setLockEndUUID(objectID, isOverlay);
    }
}
