//
//  LaserPointerManager.cpp
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "LaserPointerManager.h"

QUuid LaserPointerManager::createLaserPointer(const QVariant& rayProps, const LaserPointer::RenderStateMap& renderStates, const LaserPointer::DefaultRenderStateMap& defaultRenderStates,
    const bool faceAvatar, const bool centerEndY, const bool lockEnd, const bool enabled) {
    std::shared_ptr<LaserPointer> laserPointer = std::make_shared<LaserPointer>(rayProps, renderStates, defaultRenderStates, faceAvatar, centerEndY, lockEnd, enabled);
    if (!laserPointer->getRayUID().isNull()) {
        QWriteLocker lock(&_addLock);
        QUuid id = QUuid::createUuid();
        _laserPointersToAdd.push(std::pair<QUuid, std::shared_ptr<LaserPointer>>(id, laserPointer));
        return id;
    }
    return QUuid();
}

void LaserPointerManager::removeLaserPointer(const QUuid uid) {
    QWriteLocker lock(&_removeLock);
    _laserPointersToRemove.push(uid);
}

void LaserPointerManager::enableLaserPointer(const QUuid uid) {
    QReadLocker lock(&_containsLock);
    if (_laserPointers.contains(uid)) {
        QWriteLocker laserLock(_laserPointerLocks[uid].get());
        _laserPointers[uid]->enable();
    }
}

void LaserPointerManager::disableLaserPointer(const QUuid uid) {
    QReadLocker lock(&_containsLock);
    if (_laserPointers.contains(uid)) {
        QWriteLocker laserLock(_laserPointerLocks[uid].get());
        _laserPointers[uid]->disable();
    }
}

void LaserPointerManager::setRenderState(QUuid uid, const std::string& renderState) {
    QReadLocker lock(&_containsLock);
    if (_laserPointers.contains(uid)) {
        QWriteLocker laserLock(_laserPointerLocks[uid].get());
        _laserPointers[uid]->setRenderState(renderState);
    }
}

void LaserPointerManager::editRenderState(QUuid uid, const std::string& state, const QVariant& startProps, const QVariant& pathProps, const QVariant& endProps) {
    QReadLocker lock(&_containsLock);
    if (_laserPointers.contains(uid)) {
        QWriteLocker laserLock(_laserPointerLocks[uid].get());
        _laserPointers[uid]->editRenderState(state, startProps, pathProps, endProps);
    }
}

const RayPickResult LaserPointerManager::getPrevRayPickResult(const QUuid uid) {
    QReadLocker lock(&_containsLock);
    if (_laserPointers.contains(uid)) {
        QReadLocker laserLock(_laserPointerLocks[uid].get());
        return _laserPointers[uid]->getPrevRayPickResult();
    }
    return RayPickResult();
}

void LaserPointerManager::update() {
    for (QUuid& uid : _laserPointers.keys()) {
        // This only needs to be a read lock because update won't change any of the properties that can be modified from scripts
        QReadLocker laserLock(_laserPointerLocks[uid].get());
        _laserPointers[uid]->update();
    }

    QWriteLocker containsLock(&_containsLock);
    {
        QWriteLocker lock(&_addLock);
        while (!_laserPointersToAdd.empty()) {
            std::pair<QUuid, std::shared_ptr<LaserPointer>> laserPointerToAdd = _laserPointersToAdd.front();
            _laserPointersToAdd.pop();
            _laserPointers[laserPointerToAdd.first] = laserPointerToAdd.second;
            _laserPointerLocks[laserPointerToAdd.first] = std::make_shared<QReadWriteLock>();
        }
    }

    {
        QWriteLocker lock(&_removeLock);
        while (!_laserPointersToRemove.empty()) {
            QUuid uid = _laserPointersToRemove.front();
            _laserPointersToRemove.pop();
            _laserPointers.remove(uid);
            _laserPointerLocks.remove(uid);
        }
    }
}

void LaserPointerManager::setPrecisionPicking(QUuid uid, const bool precisionPicking) {
    QReadLocker lock(&_containsLock);
    if (_laserPointers.contains(uid)) {
        QWriteLocker laserLock(_laserPointerLocks[uid].get());
        _laserPointers[uid]->setPrecisionPicking(precisionPicking);
    }
}

void LaserPointerManager::setIgnoreEntities(QUuid uid, const QScriptValue& ignoreEntities) {
    QReadLocker lock(&_containsLock);
    if (_laserPointers.contains(uid)) {
        QWriteLocker laserLock(_laserPointerLocks[uid].get());
        _laserPointers[uid]->setIgnoreEntities(ignoreEntities);
    }
}

void LaserPointerManager::setIncludeEntities(QUuid uid, const QScriptValue& includeEntities) {
    QReadLocker lock(&_containsLock);
    if (_laserPointers.contains(uid)) {
        QWriteLocker laserLock(_laserPointerLocks[uid].get());
        _laserPointers[uid]->setIncludeEntities(includeEntities);
    }
}

void LaserPointerManager::setIgnoreOverlays(QUuid uid, const QScriptValue& ignoreOverlays) {
    QReadLocker lock(&_containsLock);
    if (_laserPointers.contains(uid)) {
        QWriteLocker laserLock(_laserPointerLocks[uid].get());
        _laserPointers[uid]->setIgnoreOverlays(ignoreOverlays);
    }
}

void LaserPointerManager::setIncludeOverlays(QUuid uid, const QScriptValue& includeOverlays) {
    QReadLocker lock(&_containsLock);
    if (_laserPointers.contains(uid)) {
        QWriteLocker laserLock(_laserPointerLocks[uid].get());
        _laserPointers[uid]->setIncludeOverlays(includeOverlays);
    }
}

void LaserPointerManager::setIgnoreAvatars(QUuid uid, const QScriptValue& ignoreAvatars) {
    QReadLocker lock(&_containsLock);
    if (_laserPointers.contains(uid)) {
        QWriteLocker laserLock(_laserPointerLocks[uid].get());
        _laserPointers[uid]->setIgnoreAvatars(ignoreAvatars);
    }
}

void LaserPointerManager::setIncludeAvatars(QUuid uid, const QScriptValue& includeAvatars) {
    QReadLocker lock(&_containsLock);
    if (_laserPointers.contains(uid)) {
        QWriteLocker laserLock(_laserPointerLocks[uid].get());
        _laserPointers[uid]->setIncludeAvatars(includeAvatars);
    }
}

void LaserPointerManager::setLockEndUUID(QUuid uid, QUuid objectID, const bool isOverlay) {
    QReadLocker lock(&_containsLock);
    if (_laserPointers.contains(uid)) {
        QWriteLocker laserLock(_laserPointerLocks[uid].get());
        _laserPointers[uid]->setLockEndUUID(objectID, isOverlay);
    }
}
