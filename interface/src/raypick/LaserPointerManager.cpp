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
    const bool faceAvatar, const bool centerEndY, const bool lockEnd, const bool distanceScaleEnd, const bool enabled) {
    QUuid result;
    std::shared_ptr<LaserPointer> laserPointer = std::make_shared<LaserPointer>(rayProps, renderStates, defaultRenderStates, faceAvatar, centerEndY, lockEnd, distanceScaleEnd, enabled);
    if (!laserPointer->getRayUID().isNull()) {
        result = QUuid::createUuid();
        withWriteLock([&] { _laserPointers[result] = laserPointer; });
    }
    return result;
}


LaserPointer::Pointer LaserPointerManager::find(const QUuid& uid) const {
    return resultWithReadLock<LaserPointer::Pointer>([&] {
        auto itr = _laserPointers.find(uid);
        if (itr != _laserPointers.end()) {
            return *itr;
        }
        return LaserPointer::Pointer();
    });
}


void LaserPointerManager::removeLaserPointer(const QUuid& uid) {
    withWriteLock([&] {
        _laserPointers.remove(uid);
    });
}

void LaserPointerManager::enableLaserPointer(const QUuid& uid) const {
    auto laserPointer = find(uid);
    if (laserPointer) {
        laserPointer->enable();
    }
}

void LaserPointerManager::disableLaserPointer(const QUuid& uid) const  {
    auto laserPointer = find(uid);
    if (laserPointer) {
        laserPointer->disable();
    }
}

void LaserPointerManager::setRenderState(const QUuid& uid, const std::string& renderState) const {
    auto laserPointer = find(uid);
    if (laserPointer) {
        laserPointer->setRenderState(renderState);
    }
}

void LaserPointerManager::editRenderState(const QUuid& uid, const std::string& state, const QVariant& startProps, const QVariant& pathProps, const QVariant& endProps) const {
    auto laserPointer = find(uid);
    if (laserPointer) {
        laserPointer->editRenderState(state, startProps, pathProps, endProps);
    }
}

const RayPickResult LaserPointerManager::getPrevRayPickResult(const QUuid& uid) const {
    auto laserPointer = find(uid);
    if (laserPointer) {
        return laserPointer->getPrevRayPickResult();
    }
    return RayPickResult();
}

void LaserPointerManager::update() {
    auto cachedLaserPointers = resultWithReadLock<QList<std::shared_ptr<LaserPointer>>>([&] { 
        return _laserPointers.values(); 
    });

    for (const auto& laserPointer : cachedLaserPointers) {
        laserPointer->update();
    }
}

void LaserPointerManager::setPrecisionPicking(const QUuid& uid, const bool precisionPicking) const {
    auto laserPointer = find(uid);
    if (laserPointer) {
        laserPointer->setPrecisionPicking(precisionPicking);
    }
}

void LaserPointerManager::setLaserLength(const QUuid& uid, const float laserLength) const {
    auto laserPointer = find(uid);
    if (laserPointer) {
        laserPointer->setLaserLength(laserLength);
    }
}

void LaserPointerManager::setIgnoreItems(const QUuid& uid, const QVector<QUuid>& ignoreEntities) const {
    auto laserPointer = find(uid);
    if (laserPointer) {
        laserPointer->setIgnoreItems(ignoreEntities);
    }
}

void LaserPointerManager::setIncludeItems(const QUuid& uid, const QVector<QUuid>& includeEntities) const {
    auto laserPointer = find(uid);
    if (laserPointer) {
        laserPointer->setIncludeItems(includeEntities);
    }
}

void LaserPointerManager::setLockEndUUID(const QUuid& uid, const QUuid& objectID, const bool isOverlay) const {
    auto laserPointer = find(uid);
    if (laserPointer) {
        laserPointer->setLockEndUUID(objectID, isOverlay);
    }
}
