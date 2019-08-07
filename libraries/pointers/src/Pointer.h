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
    Pointer(unsigned int uid, bool enabled, bool hover) : _pickUID(uid), _enabled(enabled), _hover(hover) {}

    virtual ~Pointer();

    virtual void enable();
    virtual void disable();
    virtual bool isEnabled();
    virtual PickQuery::PickType getType() const = 0;
    virtual PickResultPointer getPrevPickResult();

    virtual void setRenderState(const std::string& state) = 0;
    virtual void editRenderState(const std::string& state, const QVariant& startProps, const QVariant& pathProps, const QVariant& endProps) = 0;
    
    virtual QVariantMap toVariantMap() const;
    virtual void setScriptParameters(const QVariantMap& scriptParameters);
    virtual QVariantMap getScriptParameters() const;

    virtual void setPrecisionPicking(bool precisionPicking);
    virtual void setIgnoreItems(const QVector<QUuid>& ignoreItems) const;
    virtual void setIncludeItems(const QVector<QUuid>& includeItems) const;

    bool isLeftHand() const;
    bool isRightHand() const;
    bool isMouse() const;

    // Pointers can choose to implement these
    virtual void setLength(float length) {}
    virtual void setLockEndUUID(const QUuid& objectID, bool isAvatar, const glm::mat4& offsetMat = glm::mat4()) {}

    void update(unsigned int pointerID);
    virtual void updateVisuals(const PickResultPointer& pickResult) = 0;
    void generatePointerEvents(unsigned int pointerID, const PickResultPointer& pickResult);

    struct PickedObject {
        PickedObject(const QUuid& objectID = QUuid(), IntersectionType type = IntersectionType::NONE) : objectID(objectID), type(type) {}

        QUuid objectID;
        IntersectionType type;
    } typedef PickedObject;

    using Buttons = std::unordered_set<std::string>;

    unsigned int getRayUID() { return _pickUID; }

protected:
    const unsigned int _pickUID;
    bool _enabled;
    bool _hover;

    // The parameters used to create this pointer when created through a script
    QVariantMap _scriptParameters;

    virtual PointerEvent buildPointerEvent(const PickedObject& target, const PickResultPointer& pickResult, const std::string& button = "", bool hover = true) = 0;

    virtual PickedObject getHoveredObject(const PickResultPointer& pickResult) = 0;
    virtual Buttons getPressedButtons(const PickResultPointer& pickResult) = 0;

    virtual bool shouldHover(const PickResultPointer& pickResult) { return true; }
    virtual bool shouldTrigger(const PickResultPointer& pickResult) { return true; }
    virtual PickResultPointer getPickResultCopy(const PickResultPointer& pickResult) const = 0;
    virtual PickResultPointer getVisualPickResult(const PickResultPointer& pickResult) { return pickResult; };

    static const float POINTER_MOVE_DELAY;
    static const float TOUCH_PRESS_TO_MOVE_DEADSPOT_SQUARED;

private:
    PickedObject _prevHoveredObject;
    Buttons _prevButtons;
    bool _prevEnabled { false };
    bool _prevDoHover { false };
    std::unordered_map<std::string, PickedObject> _triggeredObjects;

    PointerEvent::Button chooseButton(const std::string& button);

};

#endif // hifi_Pick_h
