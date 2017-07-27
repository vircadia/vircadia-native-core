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

#include <QHash>
#include <QString>
#include <memory>
#include <glm/glm.hpp>

#include "LaserPointer.h"
#include "DependencyManager.h"

class RayPickResult;

class LaserPointerManager : public Dependency {
    SINGLETON_DEPENDENCY

public:
    QUuid createLaserPointer(const QVariantMap& rayProps, const QHash<QString, RenderState>& renderStates, QHash<QString, QPair<float, RenderState>>& defaultRenderStates,
        const bool faceAvatar, const bool centerEndY, const bool lockEnd, const bool enabled);
    void removeLaserPointer(const QUuid uid);
    void enableLaserPointer(const QUuid uid);
    void disableLaserPointer(const QUuid uid);
    void setRenderState(QUuid uid, const QString& renderState);
    void editRenderState(QUuid uid, const QString& state, const QVariant& startProps, const QVariant& pathProps, const QVariant& endProps);
    const RayPickResult getPrevRayPickResult(const QUuid uid);

    void setIgnoreEntities(QUuid uid, const QScriptValue& ignoreEntities);
    void setIncludeEntities(QUuid uid, const QScriptValue& includeEntities);
    void setIgnoreOverlays(QUuid uid, const QScriptValue& ignoreOverlays);
    void setIncludeOverlays(QUuid uid, const QScriptValue& includeOverlays);
    void setIgnoreAvatars(QUuid uid, const QScriptValue& ignoreAvatars);
    void setIncludeAvatars(QUuid uid, const QScriptValue& includeAvatars);

    void update();

private:
    QHash<QUuid, std::shared_ptr<LaserPointer>> _laserPointers;
    QHash<QUuid, std::shared_ptr<QReadWriteLock>> _laserPointerLocks;
    QReadWriteLock _addLock;
    QQueue<QPair<QUuid, std::shared_ptr<LaserPointer>>> _laserPointersToAdd;
    QReadWriteLock _removeLock;
    QQueue<QUuid> _laserPointersToRemove;
    QReadWriteLock _containsLock;

};

#endif // hifi_LaserPointerManager_h
