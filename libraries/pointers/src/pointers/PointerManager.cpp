//
//  Created by Bradley Austin Davis on 2017/10/16
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PointerManager.h"

std::shared_ptr<Pointer> PointerManager::find(const QUuid& uid) const {
    return resultWithReadLock<std::shared_ptr<Pointer>>([&] {
        auto itr = _pointers.find(uid);
        if (itr != _pointers.end()) {
            return *itr;
        }
        return std::shared_ptr<Pointer>();
    });
}

QUuid PointerManager::addPointer(std::shared_ptr<Pointer> pointer) {
    QUuid result;
    if (!pointer->getRayUID().isNull()) {
        result = QUuid::createUuid();
        withWriteLock([&] { _pointers[result] = pointer; });
    }
    return result;
}

void PointerManager::removePointer(const QUuid& uid) {
    withWriteLock([&] {
        _pointers.remove(uid);
    });
}

void PointerManager::enablePointer(const QUuid& uid) const {
    auto pointer = find(uid);
    if (pointer) {
        pointer->enable();
    }
}

void PointerManager::disablePointer(const QUuid& uid) const {
    auto pointer = find(uid);
    if (pointer) {
        pointer->disable();
    }
}

void PointerManager::setRenderState(const QUuid& uid, const std::string& renderState) const {
    auto pointer = find(uid);
    if (pointer) {
        pointer->setRenderState(renderState);
    }
}

void PointerManager::editRenderState(const QUuid& uid, const std::string& state, const QVariant& startProps, const QVariant& pathProps, const QVariant& endProps) const {
    auto pointer = find(uid);
    if (pointer) {
        pointer->editRenderState(state, startProps, pathProps, endProps);
    }
}

const QVariantMap PointerManager::getPrevPickResult(const QUuid& uid) const {
    auto pointer = find(uid);
    if (pointer) {
        return pointer->getPrevPickResult();
    }
    return QVariantMap();
}

void PointerManager::update() {
    auto cachedPointers = resultWithReadLock<QList<std::shared_ptr<Pointer>>>([&] {
        return _pointers.values();
    });

    for (const auto& pointer : cachedPointers) {
        pointer->update();
    }
}

void PointerManager::setPrecisionPicking(const QUuid& uid, const bool precisionPicking) const {
    auto pointer = find(uid);
    if (pointer) {
        pointer->setPrecisionPicking(precisionPicking);
    }
}

void PointerManager::setIgnoreItems(const QUuid& uid, const QVector<QUuid>& ignoreEntities) const {
    auto pointer = find(uid);
    if (pointer) {
        pointer->setIgnoreItems(ignoreEntities);
    }
}

void PointerManager::setIncludeItems(const QUuid& uid, const QVector<QUuid>& includeEntities) const {
    auto pointer = find(uid);
    if (pointer) {
        pointer->setIncludeItems(includeEntities);
    }
}

void PointerManager::setLength(const QUuid& uid, const float length) const {
    auto pointer = find(uid);
    if (pointer) {
        pointer->setLength(length);
    }
}

void PointerManager::setLockEndUUID(const QUuid& uid, const QUuid& objectID, const bool isOverlay) const {
    auto pointer = find(uid);
    if (pointer) {
        pointer->setLockEndUUID(objectID, isOverlay);
    }
}
