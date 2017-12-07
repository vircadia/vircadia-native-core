//
//  LaserPointerScriptingInterface.h
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_LaserPointerScriptingInterface_h
#define hifi_LaserPointerScriptingInterface_h

#include <QtCore/QObject>

#include "DependencyManager.h"
#include <PointerManager.h>

class LaserPointerScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    Q_INVOKABLE unsigned int createLaserPointer(const QVariant& properties) const;
    Q_INVOKABLE void enableLaserPointer(unsigned int uid) const { DependencyManager::get<PointerManager>()->enablePointer(uid); }
    Q_INVOKABLE void disableLaserPointer(unsigned int uid) const { DependencyManager::get<PointerManager>()->disablePointer(uid); }
    Q_INVOKABLE void removeLaserPointer(unsigned int uid) const { DependencyManager::get<PointerManager>()->removePointer(uid); }
    Q_INVOKABLE void editRenderState(unsigned int uid, const QString& renderState, const QVariant& properties) const;
    Q_INVOKABLE void setRenderState(unsigned int uid, const QString& renderState) const { DependencyManager::get<PointerManager>()->setRenderState(uid, renderState.toStdString()); }
    Q_INVOKABLE QVariantMap getPrevRayPickResult(unsigned int uid) const;

    Q_INVOKABLE void setPrecisionPicking(unsigned int uid, bool precisionPicking) const { DependencyManager::get<PointerManager>()->setPrecisionPicking(uid, precisionPicking); }
    Q_INVOKABLE void setLaserLength(unsigned int uid, float laserLength) const { DependencyManager::get<PointerManager>()->setLength(uid, laserLength); }
    Q_INVOKABLE void setIgnoreItems(unsigned int uid, const QScriptValue& ignoreEntities) const;
    Q_INVOKABLE void setIncludeItems(unsigned int uid, const QScriptValue& includeEntities) const;

    Q_INVOKABLE void setLockEndUUID(unsigned int uid, const QUuid& objectID, bool isOverlay, const glm::mat4& offsetMat = glm::mat4()) const { DependencyManager::get<PointerManager>()->setLockEndUUID(uid, objectID, isOverlay, offsetMat); }

    Q_INVOKABLE bool isLeftHand(unsigned int uid) { return DependencyManager::get<PointerManager>()->isLeftHand(uid); }
    Q_INVOKABLE bool isRightHand(unsigned int uid) { return DependencyManager::get<PointerManager>()->isRightHand(uid); }
    Q_INVOKABLE bool isMouse(unsigned int uid) { return DependencyManager::get<PointerManager>()->isMouse(uid); }
};

#endif // hifi_LaserPointerScriptingInterface_h
