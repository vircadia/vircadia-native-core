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

    unsigned int addPointer(std::shared_ptr<Pointer> pointer);
    void removePointer(unsigned int uid);
    void enablePointer(unsigned int uid) const;
    void disablePointer(unsigned int uid) const;
    bool isPointerEnabled(unsigned int uid) const;
    QVector<unsigned int> getPointers() const;

    void setRenderState(unsigned int uid, const std::string& renderState) const;
    void editRenderState(unsigned int uid, const std::string& state, const QVariant& startProps, const QVariant& pathProps, const QVariant& endProps) const;

    PickResultPointer getPrevPickResult(unsigned int uid) const;
    // The actual current properties of the pointer
    QVariantMap getPointerProperties(unsigned int uid) const;
    // The properties that were passed in to create the pointer (may be empty if the pointer was created by invoking the constructor)
    QVariantMap getPointerScriptParameters(unsigned int uid) const;

    void setPrecisionPicking(unsigned int uid, bool precisionPicking) const;
    void setIgnoreItems(unsigned int uid, const QVector<QUuid>& ignoreEntities) const;
    void setIncludeItems(unsigned int uid, const QVector<QUuid>& includeEntities) const;

    void setLength(unsigned int uid, float length) const;
    void setLockEndUUID(unsigned int uid, const QUuid& objectID, bool isAvatar, const glm::mat4& offsetMat = glm::mat4()) const;

    void update();

    bool isLeftHand(unsigned int uid);
    bool isRightHand(unsigned int uid);
    bool isMouse(unsigned int uid);

    static const unsigned int MOUSE_POINTER_ID { PointerEvent::INVALID_POINTER_ID + 1 };

private:
    std::shared_ptr<Pointer> find(unsigned int uid) const;
    std::unordered_map<unsigned int, std::shared_ptr<Pointer>> _pointers;
    unsigned int _nextPointerID { MOUSE_POINTER_ID + 1 };

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

    void triggerBeginHUD(const PointerEvent& pointerEvent);
    void triggerContinueHUD(const PointerEvent& pointerEvent);
    void triggerEndHUD(const PointerEvent& pointerEvent);
    void hoverBeginHUD(const PointerEvent& pointerEvent);
    void hoverContinueHUD(const PointerEvent& pointerEvent);
    void hoverEndHUD(const PointerEvent& pointerEvent);
};

#endif // hifi_pointers_PointerManager_h
