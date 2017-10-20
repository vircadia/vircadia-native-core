//
//  Created by Sam Gondelman 10/20/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_PointerScriptingInterface_h
#define hifi_PointerScriptingInterface_h

#include <QtCore/QObject>

#include "DependencyManager.h"
#include <pointers/PointerManager.h>

#include "LaserPointer.h"

class PointerScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    QUuid createLaserPointer(const QVariant& properties) const;

public slots:
    Q_INVOKABLE QUuid createPointer(const PickQuery::PickType type, const QVariant& properties) const;
    Q_INVOKABLE void enablePointer(const QUuid& uid) const { DependencyManager::get<PointerManager>()->enablePointer(uid); }
    Q_INVOKABLE void disablePointer(const QUuid& uid) const { DependencyManager::get<PointerManager>()->disablePointer(uid); }
    Q_INVOKABLE void removePointer(const QUuid& uid) const { DependencyManager::get<PointerManager>()->removePointer(uid); }
    Q_INVOKABLE void editRenderState(const QUuid& uid, const QString& renderState, const QVariant& properties) const;
    Q_INVOKABLE void setRenderState(const QUuid& uid, const QString& renderState) const { DependencyManager::get<PointerManager>()->setRenderState(uid, renderState.toStdString()); }
    Q_INVOKABLE QVariantMap getPrevPickResult(QUuid uid) const { return DependencyManager::get<PointerManager>()->getPrevPickResult(uid); }

    Q_INVOKABLE void setPrecisionPicking(const QUuid& uid, bool precisionPicking) const { DependencyManager::get<PointerManager>()->setPrecisionPicking(uid, precisionPicking); }
    Q_INVOKABLE void setLaserLength(const QUuid& uid, float laserLength) const { DependencyManager::get<PointerManager>()->setLength(uid, laserLength); }
    Q_INVOKABLE void setIgnoreItems(const QUuid& uid, const QScriptValue& ignoreEntities) const;
    Q_INVOKABLE void setIncludeItems(const QUuid& uid, const QScriptValue& includeEntities) const;

    Q_INVOKABLE void setLockEndUUID(const QUuid& uid, const QUuid& objectID, bool isOverlay) const { DependencyManager::get<PointerManager>()->setLockEndUUID(uid, objectID, isOverlay); }

private:
    static RenderState buildRenderState(const QVariantMap& propMap);

};

#endif // hifi_PointerScriptingInterface_h
