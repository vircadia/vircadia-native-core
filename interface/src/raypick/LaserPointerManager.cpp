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
        QWriteLocker containsLock(&_containsLock);
        QUuid id = QUuid::createUuid();
        _laserPointers[id] = laserPointer;
        return id;
    }
    return QUuid();
}

void LaserPointerManager::removeLaserPointer(const QUuid uid) {
    QWriteLocker lock(&_containsLock);
    _laserPointers.remove(uid);
}

void LaserPointerManager::enableLaserPointer(const QUuid uid) {
    QReadLocker lock(&_containsLock);
    auto laserPointer = _laserPointers.find(uid);
    if (laserPointer != _laserPointers.end()) {
        QWriteLocker laserLock(laserPointer.value()->getLock());
        laserPointer.value()->enable();
    }
}

void LaserPointerManager::disableLaserPointer(const QUuid uid) {
    QReadLocker lock(&_containsLock);
    auto laserPointer = _laserPointers.find(uid);
    if (laserPointer != _laserPointers.end()) {
        QWriteLocker laserLock(laserPointer.value()->getLock());
        laserPointer.value()->disable();
    }
}

void LaserPointerManager::setRenderState(QUuid uid, const std::string& renderState) {
    QReadLocker lock(&_containsLock);
    auto laserPointer = _laserPointers.find(uid);
    if (laserPointer != _laserPointers.end()) {
        QWriteLocker laserLock(laserPointer.value()->getLock());
        laserPointer.value()->setRenderState(renderState);
    }
}

void LaserPointerManager::editRenderState(QUuid uid, const std::string& state, const QVariant& startProps, const QVariant& pathProps, const QVariant& endProps) {
    QReadLocker lock(&_containsLock);
    auto laserPointer = _laserPointers.find(uid);
    if (laserPointer != _laserPointers.end()) {
        QWriteLocker laserLock(laserPointer.value()->getLock());
        laserPointer.value()->editRenderState(state, startProps, pathProps, endProps);
    }
}

const RayPickResult LaserPointerManager::getPrevRayPickResult(const QUuid uid) {
    QReadLocker lock(&_containsLock);
    auto laserPointer = _laserPointers.find(uid);
    if (laserPointer != _laserPointers.end()) {
        QReadLocker laserLock(laserPointer.value()->getLock());
        return laserPointer.value()->getPrevRayPickResult();
    }
    return RayPickResult();
}

void LaserPointerManager::update() {
    QReadLocker lock(&_containsLock);
    for (QUuid& uid : _laserPointers.keys()) {
        // This only needs to be a read lock because update won't change any of the properties that can be modified from scripts
        auto laserPointer = _laserPointers.find(uid);
        QReadLocker laserLock(laserPointer.value()->getLock());
        laserPointer.value()->update();
    }
}

void LaserPointerManager::setPrecisionPicking(QUuid uid, const bool precisionPicking) {
    QReadLocker lock(&_containsLock);
    auto laserPointer = _laserPointers.find(uid);
    if (laserPointer != _laserPointers.end()) {
        QWriteLocker laserLock(laserPointer.value()->getLock());
        laserPointer.value()->setPrecisionPicking(precisionPicking);
    }
}

void LaserPointerManager::setLaserLength(QUuid uid, const float laserLength) {
    QReadLocker lock(&_containsLock);
    auto laserPointer = _laserPointers.find(uid);
    if (laserPointer != _laserPointers.end()) {
        QWriteLocker laserLock(laserPointer.value()->getLock());
        laserPointer.value()->setLaserLength(laserLength);
    }
}

void LaserPointerManager::setIgnoreEntities(QUuid uid, const QScriptValue& ignoreEntities) {
    QReadLocker lock(&_containsLock);
    auto laserPointer = _laserPointers.find(uid);
    if (laserPointer != _laserPointers.end()) {
        QWriteLocker laserLock(laserPointer.value()->getLock());
        laserPointer.value()->setIgnoreEntities(ignoreEntities);
    }
}

void LaserPointerManager::setIncludeEntities(QUuid uid, const QScriptValue& includeEntities) {
    QReadLocker lock(&_containsLock);
    auto laserPointer = _laserPointers.find(uid);
    if (laserPointer != _laserPointers.end()) {
        QWriteLocker laserLock(laserPointer.value()->getLock());
        laserPointer.value()->setIncludeEntities(includeEntities);
    }
}

void LaserPointerManager::setIgnoreOverlays(QUuid uid, const QScriptValue& ignoreOverlays) {
    QReadLocker lock(&_containsLock);
    auto laserPointer = _laserPointers.find(uid);
    if (laserPointer != _laserPointers.end()) {
        QWriteLocker laserLock(laserPointer.value()->getLock());
        laserPointer.value()->setIgnoreOverlays(ignoreOverlays);
    }
}

void LaserPointerManager::setIncludeOverlays(QUuid uid, const QScriptValue& includeOverlays) {
    QReadLocker lock(&_containsLock);
    auto laserPointer = _laserPointers.find(uid);
    if (laserPointer != _laserPointers.end()) {
        QWriteLocker laserLock(laserPointer.value()->getLock());
        laserPointer.value()->setIncludeOverlays(includeOverlays);
    }
}

void LaserPointerManager::setIgnoreAvatars(QUuid uid, const QScriptValue& ignoreAvatars) {
    QReadLocker lock(&_containsLock);
    auto laserPointer = _laserPointers.find(uid);
    if (laserPointer != _laserPointers.end()) {
        QWriteLocker laserLock(laserPointer.value()->getLock());
        laserPointer.value()->setIgnoreAvatars(ignoreAvatars);
    }
}

void LaserPointerManager::setIncludeAvatars(QUuid uid, const QScriptValue& includeAvatars) {
    QReadLocker lock(&_containsLock);
    auto laserPointer = _laserPointers.find(uid);
    if (laserPointer != _laserPointers.end()) {
        QWriteLocker laserLock(laserPointer.value()->getLock());
        laserPointer.value()->setIncludeAvatars(includeAvatars);
    }
}

void LaserPointerManager::setLockEndUUID(QUuid uid, QUuid objectID, const bool isOverlay) {
    QReadLocker lock(&_containsLock);
    auto laserPointer = _laserPointers.find(uid);
    if (laserPointer != _laserPointers.end()) {
        QWriteLocker laserLock(laserPointer.value()->getLock());
        laserPointer.value()->setLockEndUUID(objectID, isOverlay);
    }
}
