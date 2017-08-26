//
//  LaserPointerManager.h
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_LaserPointerManager_h
#define hifi_LaserPointerManager_h

#include <memory>
#include <glm/glm.hpp>
#include <QReadWriteLock>

#include "LaserPointer.h"

class RayPickResult;

class LaserPointerManager {

public:
    QUuid createLaserPointer(const QVariant& rayProps, const LaserPointer::RenderStateMap& renderStates, const LaserPointer::DefaultRenderStateMap& defaultRenderStates,
        const bool faceAvatar, const bool centerEndY, const bool lockEnd, const bool enabled);
    void removeLaserPointer(const QUuid uid);
    void enableLaserPointer(const QUuid uid);
    void disableLaserPointer(const QUuid uid);
    void setRenderState(QUuid uid, const std::string& renderState);
    void editRenderState(QUuid uid, const std::string& state, const QVariant& startProps, const QVariant& pathProps, const QVariant& endProps);
    const RayPickResult getPrevRayPickResult(const QUuid uid);

    void setPrecisionPicking(QUuid uid, const bool precisionPicking);
    void setIgnoreEntities(QUuid uid, const QScriptValue& ignoreEntities);
    void setIncludeEntities(QUuid uid, const QScriptValue& includeEntities);
    void setIgnoreOverlays(QUuid uid, const QScriptValue& ignoreOverlays);
    void setIncludeOverlays(QUuid uid, const QScriptValue& includeOverlays);
    void setIgnoreAvatars(QUuid uid, const QScriptValue& ignoreAvatars);
    void setIncludeAvatars(QUuid uid, const QScriptValue& includeAvatars);

    void setLockEndUUID(QUuid uid, QUuid objectID, const bool isOverlay);

    void update();

private:
    QHash<QUuid, std::shared_ptr<LaserPointer>> _laserPointers;
    QHash<QUuid, std::shared_ptr<QReadWriteLock>> _laserPointerLocks;
    QReadWriteLock _addLock;
    std::queue<std::pair<QUuid, std::shared_ptr<LaserPointer>>> _laserPointersToAdd;
    QReadWriteLock _removeLock;
    std::queue<QUuid> _laserPointersToRemove;
    QReadWriteLock _containsLock;

};

#endif // hifi_LaserPointerManager_h
