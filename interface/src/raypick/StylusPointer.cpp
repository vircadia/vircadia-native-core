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
#include <pointers/PickManager.h>

using namespace controller;

// TODO: make these configurable per pointer
static const float WEB_STYLUS_LENGTH = 0.2f;

static const float TABLET_MIN_HOVER_DISTANCE = -0.1f;
static const float TABLET_MAX_HOVER_DISTANCE = 0.1f;
static const float TABLET_MIN_TOUCH_DISTANCE = -0.1f;
static const float TABLET_MAX_TOUCH_DISTANCE = 0.01f;

static const float HOVER_HYSTERESIS = 0.01f;
static const float TOUCH_HYSTERESIS = 0.02f;

StylusPointer::StylusPointer(const QVariant& props, const OverlayID& stylusOverlay, bool hover, bool enabled) :
    Pointer(DependencyManager::get<PickScriptingInterface>()->createStylusPick(props), enabled, hover),
    _stylusOverlay(stylusOverlay)
{
}

StylusPointer::~StylusPointer() {
    if (!_stylusOverlay.isNull()) {
        qApp->getOverlays().deleteOverlay(_stylusOverlay);
    }
}

OverlayID StylusPointer::buildStylusOverlay(const QVariantMap& properties) {
    QVariantMap overlayProperties;
    // TODO: make these configurable per pointer
    overlayProperties["name"] = "stylus";
    overlayProperties["url"] = PathUtils::resourcesPath() + "/meshes/tablet-stylus-fat.fbx";
    overlayProperties["loadPriority"] = 10.0f;
    overlayProperties["solid"] = true;
    overlayProperties["visible"] = false;
    overlayProperties["ignoreRayIntersection"] = true;
    overlayProperties["drawInFront"] = false;

    return qApp->getOverlays().addOverlay("model", overlayProperties);
}

void StylusPointer::updateVisuals(const PickResultPointer& pickResult) {
    auto stylusPickResult = std::static_pointer_cast<const StylusPickResult>(pickResult);

    if (_enabled && _renderState != DISABLED && stylusPickResult) {
        StylusTip tip(stylusPickResult->pickVariant);
        if (tip.side != bilateral::Side::Invalid) {
            show(tip);
            return;
        }
    }
    hide();
}

void StylusPointer::show(const StylusTip& tip) {
    if (!_stylusOverlay.isNull()) {
        QVariantMap props;
        static const glm::quat X_ROT_NEG_90{ 0.70710678f, -0.70710678f, 0.0f, 0.0f };
        auto modelOrientation = tip.orientation * X_ROT_NEG_90;
        auto sensorToWorldScale = DependencyManager::get<AvatarManager>()->getMyAvatar()->getSensorToWorldScale();
        auto modelPositionOffset = modelOrientation * (vec3(0.0f, 0.0f, -WEB_STYLUS_LENGTH / 2.0f) * sensorToWorldScale);
        props["position"] = vec3toVariant(tip.position + modelPositionOffset);
        props["rotation"] = quatToVariant(modelOrientation);
        props["dimensions"] = vec3toVariant(sensorToWorldScale * vec3(0.01f, 0.01f, WEB_STYLUS_LENGTH));
        props["visible"] = true;
        qApp->getOverlays().editOverlay(_stylusOverlay, props);
    }
}

void StylusPointer::hide() {
    if (!_stylusOverlay.isNull()) {
        QVariantMap props;
        props.insert("visible", false);
        qApp->getOverlays().editOverlay(_stylusOverlay, props);
    }
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
    if (_renderState == EVENTS_ON && stylusPickResult) {
        auto sensorScaleFactor = DependencyManager::get<AvatarManager>()->getMyAvatar()->getSensorToWorldScale();
        float distance = stylusPickResult->distance;

        // If we're triggering on an object, recalculate the distance instead of using the pickResult
        if (!_state.triggeredObject.objectID.isNull() && stylusPickResult->objectID != _state.triggeredObject.objectID) {
            glm::vec3 intersection;
            glm::vec3 origin = vec3FromVariant(stylusPickResult->pickVariant["position"]);
            glm::vec3 direction = -_state.surfaceNormal;
            if (_state.triggeredObject.type == ENTITY) {
                intersection = RayPick::intersectRayWithEntityXYPlane(_state.triggeredObject.objectID, origin, direction);
            } else if (_state.triggeredObject.type == OVERLAY) {
                intersection = RayPick::intersectRayWithOverlayXYPlane(_state.triggeredObject.objectID, origin, direction);
            }
            distance = glm::dot(intersection - origin, direction);
        }

        float hysteresis = _state.triggering ? TOUCH_HYSTERESIS * sensorScaleFactor : 0.0f;
        if (isWithinBounds(distance, TABLET_MIN_TOUCH_DISTANCE * sensorScaleFactor,
                           TABLET_MAX_TOUCH_DISTANCE * sensorScaleFactor, hysteresis)) {
            if (_state.triggeredObject.objectID.isNull()) {
                _state.triggeredObject = PickedObject(stylusPickResult->objectID, stylusPickResult->type);
                _state.surfaceNormal = stylusPickResult->surfaceNormal;
                _state.triggering = true;
            }
            return true;
        }
    }

    _state.triggeredObject = PickedObject();
    _state.surfaceNormal = glm::vec3(NAN);
    _state.triggering = false;
    return false;
}

Pointer::PickedObject StylusPointer::getHoveredObject(const PickResultPointer& pickResult) {
    auto stylusPickResult = std::static_pointer_cast<const StylusPickResult>(pickResult);
    if (!stylusPickResult) {
        return PickedObject();
    }
    return PickedObject(stylusPickResult->objectID, stylusPickResult->type);
}

Pointer::Buttons StylusPointer::getPressedButtons() {
    // TODO: custom buttons for styluses
    Pointer::Buttons toReturn({ "Primary", "Focus" });
    return toReturn;
}

PointerEvent StylusPointer::buildPointerEvent(const PickedObject& target, const PickResultPointer& pickResult) const {
    QUuid pickedID;
    glm::vec3 intersection, surfaceNormal, direction, origin;
    auto stylusPickResult = std::static_pointer_cast<StylusPickResult>(pickResult);
    if (stylusPickResult) {
        intersection = stylusPickResult->intersection;
        surfaceNormal = _state.surfaceNormal;
        const QVariantMap& stylusTip = stylusPickResult->pickVariant;
        origin = vec3FromVariant(stylusTip["position"]);
        direction = -_state.surfaceNormal;
        pickedID = stylusPickResult->objectID;
    }

    if (pickedID != target.objectID) {
        if (target.type == ENTITY) {
            intersection = RayPick::intersectRayWithEntityXYPlane(target.objectID, origin, direction);
        } else if (target.type == OVERLAY) {
            intersection = RayPick::intersectRayWithOverlayXYPlane(target.objectID, origin, direction);
        }
    }
    glm::vec2 pos2D;
    if (target.type == ENTITY) {
        pos2D = RayPick::projectOntoEntityXYPlane(target.objectID, origin);
    } else if (target.type == OVERLAY) {
        pos2D = RayPick::projectOntoOverlayXYPlane(target.objectID, origin);
    } else if (target.type == HUD) {
        pos2D = DependencyManager::get<PickManager>()->calculatePos2DFromHUD(origin);
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