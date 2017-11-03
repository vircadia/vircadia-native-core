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
#include <pointers/Pick.h>

class PointerScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    unsigned int createLaserPointer(const QVariant& properties) const;

public slots:
    Q_INVOKABLE unsigned int createPointer(const PickQuery::PickType& type, const QVariant& properties) const;
    Q_INVOKABLE void enablePointer(unsigned int uid) const { DependencyManager::get<PointerManager>()->enablePointer(uid); }
    Q_INVOKABLE void disablePointer(unsigned int uid) const { DependencyManager::get<PointerManager>()->disablePointer(uid); }
    Q_INVOKABLE void removePointer(unsigned int uid) const { DependencyManager::get<PointerManager>()->removePointer(uid); }
    Q_INVOKABLE void editRenderState(unsigned int uid, const QString& renderState, const QVariant& properties) const;
    Q_INVOKABLE void setRenderState(unsigned int uid, const QString& renderState) const { DependencyManager::get<PointerManager>()->setRenderState(uid, renderState.toStdString()); }
    Q_INVOKABLE QVariantMap getPrevPickResult(unsigned int uid) const { return DependencyManager::get<PointerManager>()->getPrevPickResult(uid); }

    Q_INVOKABLE void setPrecisionPicking(unsigned int uid, bool precisionPicking) const { DependencyManager::get<PointerManager>()->setPrecisionPicking(uid, precisionPicking); }
    Q_INVOKABLE void setLaserLength(unsigned int uid, float laserLength) const { DependencyManager::get<PointerManager>()->setLength(uid, laserLength); }
    Q_INVOKABLE void setIgnoreItems(unsigned int uid, const QScriptValue& ignoreEntities) const;
    Q_INVOKABLE void setIncludeItems(unsigned int uid, const QScriptValue& includeEntities) const;

    Q_INVOKABLE void setLockEndUUID(unsigned int uid, const QUuid& objectID, bool isOverlay) const { DependencyManager::get<PointerManager>()->setLockEndUUID(uid, objectID, isOverlay); }

signals:
    void triggerBegin(const QUuid& id, const PointerEvent& pointerEvent);
    void triggerContinue(const QUuid& id, const PointerEvent& pointerEvent);
    void triggerEnd(const QUuid& id, const PointerEvent& pointerEvent);
    void hoverBegin(const QUuid& id, const PointerEvent& pointerEvent);
    void hoverContinue(const QUuid& id, const PointerEvent& pointerEvent);
    void hoverEnd(const QUuid& id, const PointerEvent& pointerEvent);

};

#endif // hifi_PointerScriptingInterface_h
