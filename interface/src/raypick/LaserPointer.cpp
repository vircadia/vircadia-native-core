//
//  LaserPointer.cpp
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "LaserPointer.h"

#include "Application.h"
#include "avatar/AvatarManager.h"

#include <DependencyManager.h>
#include "RayPick.h"

LaserPointer::LaserPointer(const QVariant& rayProps, const RenderStateMap& renderStates, const DefaultRenderStateMap& defaultRenderStates, bool hover,
                           const PointerTriggers& triggers, bool faceAvatar, bool followNormal, float followNormalTime, bool centerEndY, bool lockEnd,
                           bool distanceScaleEnd, bool scaleWithAvatar, bool enabled) :
    PathPointer(PickQuery::Ray, rayProps, renderStates, defaultRenderStates, hover, triggers, faceAvatar, followNormal, followNormalTime,
                centerEndY, lockEnd, distanceScaleEnd, scaleWithAvatar, enabled)
{
}

void LaserPointer::editRenderStatePath(const std::string& state, const QVariant& pathProps) {
    auto renderState = std::static_pointer_cast<RenderState>(_renderStates[state]);
    if (renderState) {
        updateRenderStateOverlay(renderState->getPathID(), pathProps);
        QVariant lineWidth = pathProps.toMap()["lineWidth"];
        if (lineWidth.isValid()) {
            renderState->setLineWidth(lineWidth.toFloat());
        }
    }
}

glm::vec3 LaserPointer::getPickOrigin(const PickResultPointer& pickResult) const {
    auto rayPickResult = std::static_pointer_cast<RayPickResult>(pickResult);
    return (rayPickResult ? vec3FromVariant(rayPickResult->pickVariant["origin"]) : glm::vec3(0.0f));
}

glm::vec3 LaserPointer::getPickEnd(const PickResultPointer& pickResult, float distance) const {
    auto rayPickResult = std::static_pointer_cast<RayPickResult>(pickResult);
    if (distance > 0.0f) {
        PickRay pick = PickRay(rayPickResult->pickVariant);
        return pick.origin + distance * pick.direction;
    } else {
        return rayPickResult->intersection;
    }
}

glm::vec3 LaserPointer::getPickedObjectNormal(const PickResultPointer& pickResult) const {
    auto rayPickResult = std::static_pointer_cast<RayPickResult>(pickResult);
    return (rayPickResult ? rayPickResult->surfaceNormal : glm::vec3(0.0f));
}

IntersectionType LaserPointer::getPickedObjectType(const PickResultPointer& pickResult) const {
    auto rayPickResult = std::static_pointer_cast<RayPickResult>(pickResult);
    return (rayPickResult ? rayPickResult->type : IntersectionType::NONE);
}

QUuid LaserPointer::getPickedObjectID(const PickResultPointer& pickResult) const {
    auto rayPickResult = std::static_pointer_cast<RayPickResult>(pickResult);
    return (rayPickResult ? rayPickResult->objectID : QUuid());
}

void LaserPointer::setVisualPickResultInternal(PickResultPointer pickResult, IntersectionType type, const QUuid& id,
                                               const glm::vec3& intersection, float distance, const glm::vec3& surfaceNormal) {
    auto rayPickResult = std::static_pointer_cast<RayPickResult>(pickResult);
    if (rayPickResult) {
        rayPickResult->type = type;
        rayPickResult->objectID = id;
        rayPickResult->intersection = intersection;
        rayPickResult->distance = distance;
        rayPickResult->surfaceNormal = surfaceNormal;
        rayPickResult->pickVariant["direction"] = vec3toVariant(-surfaceNormal);
    }
}

LaserPointer::RenderState::RenderState(const OverlayID& startID, const OverlayID& pathID, const OverlayID& endID) :
    StartEndRenderState(startID, endID), _pathID(pathID)
{
    if (!_pathID.isNull()) {
        _pathIgnoreRays = qApp->getOverlays().getProperty(_pathID, "ignoreRayIntersection").value.toBool();
        _lineWidth = qApp->getOverlays().getProperty(_pathID, "lineWidth").value.toFloat();
    }
}

void LaserPointer::RenderState::cleanup() {
    StartEndRenderState::cleanup();
    if (!_pathID.isNull()) {
        qApp->getOverlays().deleteOverlay(_pathID);
    }
}

void LaserPointer::RenderState::disable() {
    StartEndRenderState::disable();
    if (!getPathID().isNull()) {
        QVariantMap pathProps;
        pathProps.insert("visible", false);
        pathProps.insert("ignoreRayIntersection", true);
        qApp->getOverlays().editOverlay(getPathID(), pathProps);
    }
}

void LaserPointer::RenderState::update(const glm::vec3& origin, const glm::vec3& end, const glm::vec3& surfaceNormal, bool scaleWithAvatar, bool distanceScaleEnd, bool centerEndY,
                                       bool faceAvatar, bool followNormal, float followNormalStrength, float distance, const PickResultPointer& pickResult) {
    StartEndRenderState::update(origin, end, surfaceNormal, scaleWithAvatar, distanceScaleEnd, centerEndY, faceAvatar, followNormal, followNormalStrength, distance, pickResult);
    QVariant endVariant = vec3toVariant(end);
    if (!getPathID().isNull()) {
        QVariantMap pathProps;
        pathProps.insert("start", vec3toVariant(origin));
        pathProps.insert("end", endVariant);
        pathProps.insert("visible", true);
        pathProps.insert("ignoreRayIntersection", doesPathIgnoreRays());
        if (scaleWithAvatar) {
            pathProps.insert("lineWidth", getLineWidth() * DependencyManager::get<AvatarManager>()->getMyAvatar()->getSensorToWorldScale());
        }
        qApp->getOverlays().editOverlay(getPathID(), pathProps);
    }
}

std::shared_ptr<StartEndRenderState> LaserPointer::buildRenderState(const QVariantMap& propMap) {
    QUuid startID;
    if (propMap["start"].isValid()) {
        QVariantMap startMap = propMap["start"].toMap();
        if (startMap["type"].isValid()) {
            startMap.remove("visible");
            startID = qApp->getOverlays().addOverlay(startMap["type"].toString(), startMap);
        }
    }

    QUuid pathID;
    if (propMap["path"].isValid()) {
        QVariantMap pathMap = propMap["path"].toMap();
        // laser paths must be line3ds
        if (pathMap["type"].isValid() && pathMap["type"].toString() == "line3d") {
            pathMap.remove("visible");
            pathID = qApp->getOverlays().addOverlay(pathMap["type"].toString(), pathMap);
        }
    }

    QUuid endID;
    if (propMap["end"].isValid()) {
        QVariantMap endMap = propMap["end"].toMap();
        if (endMap["type"].isValid()) {
            endMap.remove("visible");
            endID = qApp->getOverlays().addOverlay(endMap["type"].toString(), endMap);
        }
    }

    return std::make_shared<RenderState>(startID, pathID, endID);
}

PointerEvent LaserPointer::buildPointerEvent(const PickedObject& target, const PickResultPointer& pickResult, const std::string& button, bool hover) {
    QUuid pickedID;
    glm::vec3 intersection, surfaceNormal, direction, origin;
    auto rayPickResult = std::static_pointer_cast<RayPickResult>(pickResult);
    if (rayPickResult) {
        intersection = rayPickResult->intersection;
        surfaceNormal = rayPickResult->surfaceNormal;
        const QVariantMap& searchRay = rayPickResult->pickVariant;
        direction = vec3FromVariant(searchRay["direction"]);
        origin = vec3FromVariant(searchRay["origin"]);
        pickedID = rayPickResult->objectID;
    }

    if (pickedID != target.objectID) {
        intersection = findIntersection(target, origin, direction);
    }
    glm::vec2 pos2D = findPos2D(target, intersection);

    // If we just started triggering and we haven't moved too much, don't update intersection and pos2D
    TriggerState& state = hover ? _latestState : _states[button];
    float sensorToWorldScale = DependencyManager::get<AvatarManager>()->getMyAvatar()->getSensorToWorldScale();
    float deadspotSquared = TOUCH_PRESS_TO_MOVE_DEADSPOT_SQUARED * sensorToWorldScale * sensorToWorldScale;
    bool withinDeadspot = usecTimestampNow() - state.triggerStartTime < POINTER_MOVE_DELAY && glm::distance2(pos2D, state.triggerPos2D) < deadspotSquared;
    if ((state.triggering || state.wasTriggering) && !state.deadspotExpired && withinDeadspot) {
        pos2D = state.triggerPos2D;
        intersection = state.intersection;
        surfaceNormal = state.surfaceNormal;
    }
    if (!withinDeadspot) {
        state.deadspotExpired = true;
    }

    return PointerEvent(pos2D, intersection, surfaceNormal, direction);
}

glm::vec3 LaserPointer::findIntersection(const PickedObject& pickedObject, const glm::vec3& origin, const glm::vec3& direction) {
    switch (pickedObject.type) {
        case ENTITY:
            return RayPick::intersectRayWithEntityXYPlane(pickedObject.objectID, origin, direction);
        case OVERLAY:
            return RayPick::intersectRayWithOverlayXYPlane(pickedObject.objectID, origin, direction);
        default:
            return glm::vec3(NAN);
    }
}
