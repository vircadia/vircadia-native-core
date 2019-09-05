//
//  Created by Bradley Austin Davis on 2017/10/24
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "StylusPointer.h"

#include "RayPick.h"

#include "Application.h"
#include "avatar/AvatarManager.h"
#include "avatar/MyAvatar.h"

#include <DependencyManager.h>
#include "PickScriptingInterface.h"
#include <PickManager.h>

static const float TABLET_MIN_HOVER_DISTANCE = -0.1f;
static const float TABLET_MAX_HOVER_DISTANCE = 0.1f;
static const float TABLET_MIN_TOUCH_DISTANCE = -0.1f;
static const float TABLET_MAX_TOUCH_DISTANCE = 0.005f;

static const float HOVER_HYSTERESIS = 0.01f;
static const float TOUCH_HYSTERESIS = 0.001f;

static const QString DEFAULT_STYLUS_MODEL_URL = PathUtils::resourcesUrl() + "/meshes/tablet-stylus-fat.fbx";

StylusPointer::StylusPointer(const QVariant& props, const QUuid& stylus, bool hover, bool enabled,
                             const glm::vec3& modelPositionOffset, const glm::quat& modelRotationOffset, const glm::vec3& modelDimensions) :
    Pointer(DependencyManager::get<PickScriptingInterface>()->createPick(PickQuery::PickType::Stylus, props), enabled, hover),
    _stylus(stylus),
    _modelPositionOffset(modelPositionOffset),
    _modelDimensions(modelDimensions),
    _modelRotationOffset(modelRotationOffset)
{
}

StylusPointer::~StylusPointer() {
    if (!_stylus.isNull()) {
        DependencyManager::get<EntityScriptingInterface>()->deleteEntity(_stylus);
    }
}

PickQuery::PickType StylusPointer::getType() const {
    return PickQuery::PickType::Stylus;
}

QUuid StylusPointer::buildStylus(const QVariantMap& properties) {
    // FIXME: we have to keep using the Overlays interface here, because existing scripts use overlay properties to define pointers
    QVariantMap propertiesMap;

    QString modelUrl = DEFAULT_STYLUS_MODEL_URL;

    if (properties["model"].isValid()) {
        QVariantMap modelData = properties["model"].toMap();

        if (modelData["url"].isValid()) {
            modelUrl = modelData["url"].toString();
        }
    }
    // TODO: make these configurable per pointer
    propertiesMap["name"] = "stylus";
    propertiesMap["url"] = modelUrl;
    propertiesMap["loadPriority"] = 10.0f;
    propertiesMap["solid"] = true;
    propertiesMap["visible"] = false;
    propertiesMap["ignorePickIntersection"] = true;
    propertiesMap["drawInFront"] = false;

    return qApp->getOverlays().addOverlay("model", propertiesMap);
}

void StylusPointer::updateVisuals(const PickResultPointer& pickResult) {
    auto stylusPickResult = std::static_pointer_cast<const StylusPickResult>(pickResult);

    if (_enabled && !qApp->getPreferAvatarFingerOverStylus() && _renderState != DISABLED && stylusPickResult) {
        StylusTip tip(stylusPickResult->pickVariant);
        if (tip.side != bilateral::Side::Invalid) {
            show(tip);
            return;
        }
    }
    if (_showing) {
        hide();
    }
}

void StylusPointer::show(const StylusTip& tip) {
    if (!_stylus.isNull()) {
        auto modelOrientation = tip.orientation * _modelRotationOffset;
        auto sensorToWorldScale = DependencyManager::get<AvatarManager>()->getMyAvatar()->getSensorToWorldScale();
        auto modelPositionOffset = modelOrientation * (_modelPositionOffset * sensorToWorldScale);
        EntityItemProperties properties;
        properties.setPosition(tip.position + modelPositionOffset);
        properties.setRotation(modelOrientation);
        properties.setDimensions(sensorToWorldScale * _modelDimensions);
        properties.setVisible(true);
        DependencyManager::get<EntityScriptingInterface>()->editEntity(_stylus, properties);
    }
    _showing = true;
}

void StylusPointer::hide() {
    if (!_stylus.isNull()) {
        EntityItemProperties properties;
        properties.setVisible(false);
        DependencyManager::get<EntityScriptingInterface>()->editEntity(_stylus, properties);
    }
    _showing = false;
}

bool StylusPointer::shouldHover(const PickResultPointer& pickResult) {
    auto stylusPickResult = std::static_pointer_cast<const StylusPickResult>(pickResult);
    if (_renderState == EVENTS_ON && stylusPickResult && stylusPickResult->intersects) {
        auto sensorScaleFactor = DependencyManager::get<AvatarManager>()->getMyAvatar()->getSensorToWorldScale();
        float hysteresis = _state.hovering ? HOVER_HYSTERESIS * sensorScaleFactor : 0.0f;
        bool hovering = isWithinBounds(stylusPickResult->distance, TABLET_MIN_HOVER_DISTANCE * sensorScaleFactor,
                                       TABLET_MAX_HOVER_DISTANCE * sensorScaleFactor, hysteresis);

        _state.hovering = hovering;
        return hovering;
    }

    _state.hovering = false;
    return false;
}

bool StylusPointer::shouldTrigger(const PickResultPointer& pickResult) {
    auto stylusPickResult = std::static_pointer_cast<const StylusPickResult>(pickResult);
    bool wasTriggering = false;
    if (_renderState == EVENTS_ON && stylusPickResult) {
        auto sensorScaleFactor = DependencyManager::get<AvatarManager>()->getMyAvatar()->getSensorToWorldScale();
        float distance = stylusPickResult->distance;

        // If we're triggering on an object, recalculate the distance instead of using the pickResult
        glm::vec3 origin = vec3FromVariant(stylusPickResult->pickVariant["position"]);
        glm::vec3 direction = _state.triggering ? -_state.surfaceNormal : -stylusPickResult->surfaceNormal;
        if ((_state.triggering || _state.wasTriggering) && stylusPickResult->objectID != _state.triggeredObject.objectID) {
            distance = glm::dot(findIntersection(_state.triggeredObject, origin, direction) - origin, direction);
        }

        float hysteresis = _state.triggering ? TOUCH_HYSTERESIS * sensorScaleFactor : 0.0f;
        if (isWithinBounds(distance, TABLET_MIN_TOUCH_DISTANCE * sensorScaleFactor,
            TABLET_MAX_TOUCH_DISTANCE * sensorScaleFactor, hysteresis)) {
            _state.wasTriggering = _state.triggering;
            if (!_state.triggering) {
                _state.triggeredObject = PickedObject(stylusPickResult->objectID, stylusPickResult->type);
                _state.intersection = stylusPickResult->intersection;
                _state.triggerPos2D = findPos2D(_state.triggeredObject, origin);
                _state.triggerStartTime = usecTimestampNow();
                _state.surfaceNormal = stylusPickResult->surfaceNormal;
                _state.deadspotExpired = false;
                _state.triggering = true;
            }
            return true;
        }
        wasTriggering = _state.triggering;
    }

    _state.wasTriggering = wasTriggering;
    _state.triggering = false;
    return false;
}

PickResultPointer StylusPointer::getPickResultCopy(const PickResultPointer& pickResult) const {
    auto stylusPickResult = std::dynamic_pointer_cast<StylusPickResult>(pickResult);
    if (!stylusPickResult) {
        return std::make_shared<StylusPickResult>();
    }
    return std::make_shared<StylusPickResult>(*stylusPickResult.get());
}

Pointer::PickedObject StylusPointer::getHoveredObject(const PickResultPointer& pickResult) {
    auto stylusPickResult = std::static_pointer_cast<const StylusPickResult>(pickResult);
    if (!stylusPickResult) {
        return PickedObject();
    }
    return PickedObject(stylusPickResult->objectID, stylusPickResult->type);
}

Pointer::Buttons StylusPointer::getPressedButtons(const PickResultPointer& pickResult) {
    // TODO: custom buttons for styluses
    Pointer::Buttons toReturn({ "Primary", "Focus" });
    return toReturn;
}

PointerEvent StylusPointer::buildPointerEvent(const PickedObject& target, const PickResultPointer& pickResult, const std::string& button, bool hover) {
    QUuid pickedID;
    glm::vec2 pos2D;
    glm::vec3 intersection, surfaceNormal, direction, origin;
    auto stylusPickResult = std::static_pointer_cast<StylusPickResult>(pickResult);
    if (stylusPickResult) {
        intersection = stylusPickResult->intersection;
        surfaceNormal = hover ? stylusPickResult->surfaceNormal : _state.surfaceNormal;
        const QVariantMap& stylusTip = stylusPickResult->pickVariant;
        origin = vec3FromVariant(stylusTip["position"]);
        direction = -surfaceNormal;
        pos2D = findPos2D(target, origin);
        pickedID = stylusPickResult->objectID;
    }

    // If we just started triggering and we haven't moved too much, don't update intersection and pos2D
    float sensorToWorldScale = DependencyManager::get<AvatarManager>()->getMyAvatar()->getSensorToWorldScale();
    float deadspotSquared = TOUCH_PRESS_TO_MOVE_DEADSPOT_SQUARED * sensorToWorldScale * sensorToWorldScale;
    bool withinDeadspot = usecTimestampNow() - _state.triggerStartTime < POINTER_MOVE_DELAY && glm::distance2(pos2D, _state.triggerPos2D) < deadspotSquared;
    if ((_state.triggering || _state.wasTriggering) && !_state.deadspotExpired && withinDeadspot) {
        pos2D = _state.triggerPos2D;
        intersection = _state.intersection;
    } else if (pickedID != target.objectID) {
        intersection = findIntersection(target, origin, direction);
    }
    if (!withinDeadspot) {
        _state.deadspotExpired = true;
    }

    return PointerEvent(pos2D, intersection, surfaceNormal, direction);
}

bool StylusPointer::isWithinBounds(float distance, float min, float max, float hysteresis) {
    return (distance == glm::clamp(distance, min - hysteresis, max + hysteresis));
}

void StylusPointer::setRenderState(const std::string& state) {
    if (state == "events on") {
        _renderState = EVENTS_ON;
    } else if (state == "events off") {
        _renderState = EVENTS_OFF;
    } else if (state == "disabled") {
        _renderState = DISABLED;
    }
}

QVariantMap StylusPointer::toVariantMap() const {
    return Parent::toVariantMap();
}

glm::vec3 StylusPointer::findIntersection(const PickedObject& pickedObject, const glm::vec3& origin, const glm::vec3& direction) {
    switch (pickedObject.type) {
        case ENTITY:
        case LOCAL_ENTITY:
            return RayPick::intersectRayWithEntityXYPlane(pickedObject.objectID, origin, direction);
        default:
            return glm::vec3(NAN);
    }
}

glm::vec2 StylusPointer::findPos2D(const PickedObject& pickedObject, const glm::vec3& origin) {
    switch (pickedObject.type) {
        case ENTITY:
        case LOCAL_ENTITY:
            return RayPick::projectOntoEntityXYPlane(pickedObject.objectID, origin);
        case HUD:
            return DependencyManager::get<PickManager>()->calculatePos2DFromHUD(origin);
        default:
            return glm::vec2(NAN);
    }
}
