//
//  Created by Sam Gondelman 10/17/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_Pointer_h
#define hifi_Pointer_h

#include <unordered_set>
#include <unordered_map>

#include <QtCore/QUuid>
#include <QVector>
#include <QVariant>

#include <shared/ReadWriteLockable.h>
#include "Pick.h"

#include <controllers/impl/Endpoint.h>
#include "PointerEvent.h"

#include "Pick.h"

class PointerTrigger {
public:
    PointerTrigger(controller::Endpoint::Pointer endpoint, const std::string& button) : _endpoint(endpoint), _button(button) {}

    controller::Endpoint::Pointer getEndpoint() const { return _endpoint; }
    const std::string& getButton() const { return _button; }

private:
    controller::Endpoint::Pointer _endpoint;
    std::string _button { "" };
};

using PointerTriggers = std::vector<PointerTrigger>;

class Pointer : protected ReadWriteLockable {
public:
    Pointer(const QUuid& uid, bool enabled, bool hover) : _pickUID(uid), _enabled(enabled), _hover(hover) {}

    virtual ~Pointer();

    virtual void enable();
    virtual void disable();
    virtual PickResultPointer getPrevPickResult();

    virtual void setRenderState(const std::string& state) = 0;
    virtual void editRenderState(const std::string& state, const QVariant& startProps, const QVariant& pathProps, const QVariant& endProps) = 0;
    
    virtual void setPrecisionPicking(bool precisionPicking);
    virtual void setIgnoreItems(const QVector<QUuid>& ignoreItems) const;
    virtual void setIncludeItems(const QVector<QUuid>& includeItems) const;

    // Pointers can choose to implement these
    virtual void setLength(float length) {}
    virtual void setLockEndUUID(const QUuid& objectID, bool isOverlay) {}

    virtual void update(float deltaTime);
    virtual void updateVisuals(const PickResultPointer& pickResult) {}
    void generatePointerEvents(const PickResultPointer& pickResult);

    struct PickedObject {
        PickedObject(const QUuid& objectID = QUuid(), IntersectionType type = IntersectionType::NONE) : objectID(objectID), type(type) {}

        QUuid objectID;
        IntersectionType type;
    } typedef PickedObject;

    using Buttons = std::unordered_set<std::string>;

    virtual PickedObject getHoveredObject(const PickResultPointer& pickResult) = 0;
    virtual Buttons getPressedButtons() = 0;

    QUuid getRayUID() { return _pickUID; }

protected:
    const QUuid _pickUID;
    bool _enabled;
    bool _hover;

    virtual PointerEvent buildPointerEvent(const PickedObject& target, const PickResultPointer& pickResult) const = 0;

private:
    PickedObject _prevHoveredObject;
    Buttons _prevButtons;
    std::unordered_map<std::string, PickedObject> _triggeredObjects;

    PointerEvent::Button chooseButton(const std::string& button);

};

#endif // hifi_Pick_h
