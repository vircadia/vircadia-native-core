//
//  Created by Bradley Austin Davis on 2017/10/16
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_pointers_PointerManager_h
#define hifi_pointers_PointerManager_h

#include <DependencyManager.h>
#include <PointerEvent.h>

#include <memory>

#include <shared/ReadWriteLockable.h>

#include "Pointer.h"

class PointerManager : public QObject, public Dependency, protected ReadWriteLockable {
    Q_OBJECT
    SINGLETON_DEPENDENCY
public:
    PointerManager() {}

    QUuid addPointer(std::shared_ptr<Pointer> pointer);
    void removePointer(const QUuid& uid);
    void enablePointer(const QUuid& uid) const;
    void disablePointer(const QUuid& uid) const;
    void setRenderState(const QUuid& uid, const std::string& renderState) const;
    void editRenderState(const QUuid& uid, const std::string& state, const QVariant& startProps, const QVariant& pathProps, const QVariant& endProps) const;
    const QVariantMap getPrevPickResult(const QUuid& uid) const;

    void setPrecisionPicking(const QUuid& uid, bool precisionPicking) const;
    void setIgnoreItems(const QUuid& uid, const QVector<QUuid>& ignoreEntities) const;
    void setIncludeItems(const QUuid& uid, const QVector<QUuid>& includeEntities) const;

    void setLength(const QUuid& uid, float length) const;
    void setLockEndUUID(const QUuid& uid, const QUuid& objectID, bool isOverlay) const;

    void update();

private:
    std::shared_ptr<Pointer> find(const QUuid& uid) const;
    QHash<QUuid, std::shared_ptr<Pointer>> _pointers;

signals:
    void triggerBeginOverlay(const QUuid& id, const PointerEvent& pointerEvent);
    void triggerContinueOverlay(const QUuid& id, const PointerEvent& pointerEvent);
    void triggerEndOverlay(const QUuid& id, const PointerEvent& pointerEvent);
    void hoverBeginOverlay(const QUuid& id, const PointerEvent& pointerEvent);
    void hoverContinueOverlay(const QUuid& id, const PointerEvent& pointerEvent);
    void hoverEndOverlay(const QUuid& id, const PointerEvent& pointerEvent);

    void triggerBeginEntity(const QUuid& id, const PointerEvent& pointerEvent);
    void triggerContinueEntity(const QUuid& id, const PointerEvent& pointerEvent);
    void triggerEndEntity(const QUuid& id, const PointerEvent& pointerEvent);
    void hoverBeginEntity(const QUuid& id, const PointerEvent& pointerEvent);
    void hoverContinueEntity(const QUuid& id, const PointerEvent& pointerEvent);
    void hoverEndEntity(const QUuid& id, const PointerEvent& pointerEvent);
};

#endif // hifi_pointers_PointerManager_h
