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
#include <pointers/PointerManager.h>

class LaserPointerScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public slots:
    Q_INVOKABLE QUuid createLaserPointer(const QVariant& properties) const;
    Q_INVOKABLE void enableLaserPointer(const QUuid& uid) const { DependencyManager::get<PointerManager>()->enablePointer(uid); }
    Q_INVOKABLE void disableLaserPointer(const QUuid& uid) const { DependencyManager::get<PointerManager>()->disablePointer(uid); }
    Q_INVOKABLE void removeLaserPointer(const QUuid& uid) const { DependencyManager::get<PointerManager>()->removePointer(uid); }
    Q_INVOKABLE void editRenderState(const QUuid& uid, const QString& renderState, const QVariant& properties) const;
    Q_INVOKABLE void setRenderState(const QUuid& uid, const QString& renderState) const { DependencyManager::get<PointerManager>()->setRenderState(uid, renderState.toStdString()); }
    Q_INVOKABLE QVariantMap getPrevRayPickResult(const QUuid& uid) const;

    Q_INVOKABLE void setPrecisionPicking(const QUuid& uid, bool precisionPicking) const { DependencyManager::get<PointerManager>()->setPrecisionPicking(uid, precisionPicking); }
    Q_INVOKABLE void setLaserLength(const QUuid& uid, float laserLength) const { DependencyManager::get<PointerManager>()->setLength(uid, laserLength); }
    Q_INVOKABLE void setIgnoreItems(const QUuid& uid, const QScriptValue& ignoreEntities) const;
    Q_INVOKABLE void setIncludeItems(const QUuid& uid, const QScriptValue& includeEntities) const;

    Q_INVOKABLE void setLockEndUUID(const QUuid& uid, const QUuid& objectID, bool isOverlay) const { DependencyManager::get<PointerManager>()->setLockEndUUID(uid, objectID, isOverlay); }

};

#endif // hifi_LaserPointerScriptingInterface_h
