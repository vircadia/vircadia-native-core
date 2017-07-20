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
#include "LaserPointer.h"
#include "RayPick.h"

unsigned int LaserPointerManager::createLaserPointer(const QString& jointName, const glm::vec3& posOffset, const glm::vec3& dirOffset, const uint16_t filter, const float maxDistance,
        const QHash<QString, RenderState>& renderStates, const bool faceAvatar, const bool centerEndY, const bool lockEnd, const bool enabled) {
    std::shared_ptr<LaserPointer> laserPointer = std::make_shared<LaserPointer>(jointName, posOffset, dirOffset, filter, maxDistance, renderStates, faceAvatar, centerEndY, lockEnd, enabled);
    unsigned int uid = laserPointer->getUID();
    QWriteLocker lock(&_lock);
    _laserPointers[uid] = laserPointer;
    _laserPointerLocks[uid] = std::make_shared<QReadWriteLock>();
    return uid;
}

unsigned int LaserPointerManager::createLaserPointer(const glm::vec3& position, const glm::vec3& direction, const uint16_t filter, const float maxDistance,
        const QHash<QString, RenderState>& renderStates, const bool faceAvatar, const bool centerEndY, const bool lockEnd, const bool enabled) {
    std::shared_ptr<LaserPointer> laserPointer = std::make_shared<LaserPointer>(position, direction, filter, maxDistance, renderStates, faceAvatar, centerEndY, lockEnd, enabled);
    unsigned int uid = laserPointer->getUID();
    QWriteLocker lock(&_lock);
    _laserPointers[uid] = laserPointer;
    _laserPointerLocks[uid] = std::make_shared<QReadWriteLock>();
    return uid;
}

unsigned int LaserPointerManager::createLaserPointer(const uint16_t filter, const float maxDistance, const QHash<QString, RenderState>& renderStates, const bool faceAvatar,
        const bool centerEndY, const bool lockEnd, const bool enabled) {
    std::shared_ptr<LaserPointer> laserPointer = std::make_shared<LaserPointer>(filter, maxDistance, renderStates, faceAvatar, centerEndY, lockEnd, enabled);
    unsigned int uid = laserPointer->getUID();
    QWriteLocker lock(&_lock);
    _laserPointers[uid] = laserPointer;
    _laserPointerLocks[uid] = std::make_shared<QReadWriteLock>();
    return uid;
}

void LaserPointerManager::removeLaserPointer(const unsigned int uid) {
    QWriteLocker lock(&_lock);
    _laserPointers.remove(uid);
    _laserPointerLocks.remove(uid);
}

void LaserPointerManager::enableLaserPointer(const unsigned int uid) {
    QReadLocker lock(&_lock);
    if (_laserPointers.contains(uid)) {
        QWriteLocker laserLock(_laserPointerLocks[uid].get());
        _laserPointers[uid]->enable();
    }
}

void LaserPointerManager::disableLaserPointer(const unsigned int uid) {
    QReadLocker lock(&_lock);
    if (_laserPointers.contains(uid)) {
        QWriteLocker laserLock(_laserPointerLocks[uid].get());
        _laserPointers[uid]->disable();
    }
}

void LaserPointerManager::setRenderState(unsigned int uid, const QString & renderState) {
    QReadLocker lock(&_lock);
    if (_laserPointers.contains(uid)) {
        QWriteLocker laserLock(_laserPointerLocks[uid].get());
        _laserPointers[uid]->setRenderState(renderState);
    }
}

void LaserPointerManager::editRenderState(unsigned int uid, const QString& state, const QVariant& startProps, const QVariant& pathProps, const QVariant& endProps) {
    QReadLocker lock(&_lock);
    if (_laserPointers.contains(uid)) {
        QWriteLocker laserLock(_laserPointerLocks[uid].get());
        _laserPointers[uid]->editRenderState(state, startProps, pathProps, endProps);
    }
}

const RayPickResult LaserPointerManager::getPrevRayPickResult(const unsigned int uid) {
    QReadLocker lock(&_lock);
    if (_laserPointers.contains(uid)) {
        QReadLocker laserLock(_laserPointerLocks[uid].get());
        return _laserPointers[uid]->getPrevRayPickResult();
    }
    return RayPickResult();
}

void LaserPointerManager::update() {
    QReadLocker lock(&_lock);
    for (unsigned int uid : _laserPointers.keys()) {
        // This only needs to be a read lock because update won't change any of the properties that can be modified from scripts
        QReadLocker laserLock(_laserPointerLocks[uid].get());
        _laserPointers[uid]->update();
    }
}

void LaserPointerManager::setIgnoreEntities(unsigned int uid, const QScriptValue& ignoreEntities) {
    QReadLocker lock(&_lock);
    if (_laserPointers.contains(uid)) {
        QWriteLocker laserLock(_laserPointerLocks[uid].get());
        _laserPointers[uid]->setIgnoreEntities(ignoreEntities);
    }
}

void LaserPointerManager::setIncludeEntities(unsigned int uid, const QScriptValue& includeEntities) {
    QReadLocker lock(&_lock);
    if (_laserPointers.contains(uid)) {
        QWriteLocker laserLock(_laserPointerLocks[uid].get());
        _laserPointers[uid]->setIncludeEntities(includeEntities);
    }
}

void LaserPointerManager::setIgnoreOverlays(unsigned int uid, const QScriptValue& ignoreOverlays) {
    QReadLocker lock(&_lock);
    if (_laserPointers.contains(uid)) {
        QWriteLocker laserLock(_laserPointerLocks[uid].get());
        _laserPointers[uid]->setIgnoreOverlays(ignoreOverlays);
    }
}

void LaserPointerManager::setIncludeOverlays(unsigned int uid, const QScriptValue& includeOverlays) {
    QReadLocker lock(&_lock);
    if (_laserPointers.contains(uid)) {
        QWriteLocker laserLock(_laserPointerLocks[uid].get());
        _laserPointers[uid]->setIncludeOverlays(includeOverlays);
    }
}

void LaserPointerManager::setIgnoreAvatars(unsigned int uid, const QScriptValue& ignoreAvatars) {
    QReadLocker lock(&_lock);
    if (_laserPointers.contains(uid)) {
        QWriteLocker laserLock(_laserPointerLocks[uid].get());
        _laserPointers[uid]->setIgnoreAvatars(ignoreAvatars);
    }
}

void LaserPointerManager::setIncludeAvatars(unsigned int uid, const QScriptValue& includeAvatars) {
    QReadLocker lock(&_lock);
    if (_laserPointers.contains(uid)) {
        QWriteLocker laserLock(_laserPointerLocks[uid].get());
        _laserPointers[uid]->setIncludeAvatars(includeAvatars);
    }
}