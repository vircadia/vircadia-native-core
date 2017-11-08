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
#include <pointers/PickManager.h>
#include "PickScriptingInterface.h"
#include "RayPick.h"

LaserPointer::LaserPointer(const QVariant& rayProps, const RenderStateMap& renderStates, const DefaultRenderStateMap& defaultRenderStates, bool hover,
        const PointerTriggers& triggers, bool faceAvatar, bool centerEndY, bool lockEnd, bool distanceScaleEnd, bool enabled) :
    Pointer(DependencyManager::get<PickScriptingInterface>()->createRayPick(rayProps), enabled, hover),
    _triggers(triggers),
    _renderStates(renderStates),
    _defaultRenderStates(defaultRenderStates),
    _faceAvatar(faceAvatar),
    _centerEndY(centerEndY),
    _lockEnd(lockEnd),
    _distanceScaleEnd(distanceScaleEnd)
{
    for (auto& state : _renderStates) {
        if (!enabled || state.first != _currentRenderState) {
            disableRenderState(state.second);
        }
    }
    for (auto& state : _defaultRenderStates) {
        if (!enabled || state.first != _currentRenderState) {
            disableRenderState(state.second.second);
        }
    }
}

LaserPointer::~LaserPointer() {
    for (auto& renderState : _renderStates) {
        renderState.second.deleteOverlays();
    }
    for (auto& renderState : _defaultRenderStates) {
        renderState.second.second.deleteOverlays();
    }
}

void LaserPointer::setRenderState(const std::string& state) {
    withWriteLock([&] {
        if (!_currentRenderState.empty() && state != _currentRenderState) {
            if (_renderStates.find(_currentRenderState) != _renderStates.end()) {
                disableRenderState(_renderStates[_currentRenderState]);
            }
            if (_defaultRenderStates.find(_currentRenderState) != _defaultRenderStates.end()) {
                disableRenderState(_defaultRenderStates[_currentRenderState].second);
            }
        }
        _currentRenderState = state;
    });
}

void LaserPointer::editRenderState(const std::string& state, const QVariant& startProps, const QVariant& pathProps, const QVariant& endProps) {
    withWriteLock([&] {
        updateRenderStateOverlay(_renderStates[state].getStartID(), startProps);
        updateRenderStateOverlay(_renderStates[state].getPathID(), pathProps);
        updateRenderStateOverlay(_renderStates[state].getEndID(), endProps);
        QVariant endDim = endProps.toMap()["dimensions"];
        if (endDim.isValid()) {
            _renderStates[state].setEndDim(vec3FromVariant(endDim));
        }
    });
}

void LaserPointer::updateRenderStateOverlay(const OverlayID& id, const QVariant& props) {
    if (!id.isNull() && props.isValid()) {
        QVariantMap propMap = props.toMap();
        propMap.remove("visible");
        qApp->getOverlays().editOverlay(id, propMap);
    }
}

void LaserPointer::updateRenderState(const RenderState& renderState, const IntersectionType type, float distance, const QUuid& objectID, const PickRay& pickRay, bool defaultState) {
    if (!renderState.getStartID().isNull()) {
        QVariantMap startProps;
        startProps.insert("position", vec3toVariant(pickRay.origin));
        startProps.insert("visible", true);
        startProps.insert("ignoreRayIntersection", renderState.doesStartIgnoreRays());
        qApp->getOverlays().editOverlay(renderState.getStartID(), startProps);
    }
    glm::vec3 endVec;
    if (((defaultState || !_lockEnd) && _objectLockEnd.first.isNull()) || type == IntersectionType::HUD) {
        endVec = pickRay.origin + pickRay.direction * distance;
    } else {
        if (!_objectLockEnd.first.isNull()) {
            glm::vec3 pos;
            glm::quat rot;
            glm::vec3 dim;
            glm::vec3 registrationPoint;
            if (_objectLockEnd.second) {
                pos = vec3FromVariant(qApp->getOverlays().getProperty(_objectLockEnd.first, "position").value);
                rot = quatFromVariant(qApp->getOverlays().getProperty(_objectLockEnd.first, "rotation").value);
                dim = vec3FromVariant(qApp->getOverlays().getProperty(_objectLockEnd.first, "dimensions").value);
                registrationPoint = glm::vec3(0.5f);
            } else {
                EntityItemProperties props = DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(_objectLockEnd.first);
                pos = props.getPosition();
                rot = props.getRotation();
                dim = props.getDimensions();
                registrationPoint = props.getRegistrationPoint();
            }
            const glm::vec3 DEFAULT_REGISTRATION_POINT = glm::vec3(0.5f);
            endVec = pos + rot * (dim * (DEFAULT_REGISTRATION_POINT - registrationPoint));
        } else {
            if (type == IntersectionType::ENTITY) {
                endVec = DependencyManager::get<EntityScriptingInterface>()->getEntityTransform(objectID)[3];
            } else if (type == IntersectionType::OVERLAY) {
                endVec = vec3FromVariant(qApp->getOverlays().getProperty(objectID, "position").value);
            } else if (type == IntersectionType::AVATAR) {
                endVec = DependencyManager::get<AvatarHashMap>()->getAvatar(objectID)->getPosition();
            }
        }
    }
    QVariant end = vec3toVariant(endVec);
    if (!renderState.getPathID().isNull()) {
        QVariantMap pathProps;
        pathProps.insert("start", vec3toVariant(pickRay.origin));
        pathProps.insert("end", end);
        pathProps.insert("visible", true);
        pathProps.insert("ignoreRayIntersection", renderState.doesPathIgnoreRays());
        qApp->getOverlays().editOverlay(renderState.getPathID(), pathProps);
    }
    if (!renderState.getEndID().isNull()) {
        QVariantMap endProps;
        glm::quat faceAvatarRotation = DependencyManager::get<AvatarManager>()->getMyAvatar()->getOrientation() * glm::quat(glm::radians(glm::vec3(0.0f, 180.0f, 0.0f)));
        glm::vec3 dim = vec3FromVariant(qApp->getOverlays().getProperty(renderState.getEndID(), "dimensions").value);
        if (_distanceScaleEnd) {
            dim = renderState.getEndDim() * glm::distance(pickRay.origin, endVec) * DependencyManager::get<AvatarManager>()->getMyAvatar()->getSensorToWorldScale();
            endProps.insert("dimensions", vec3toVariant(dim));
        }
        if (_centerEndY) {
            endProps.insert("position", end);
        } else {
            glm::vec3 currentUpVector = faceAvatarRotation * Vectors::UP;
            endProps.insert("position", vec3toVariant(endVec + glm::vec3(currentUpVector.x * 0.5f * dim.y, currentUpVector.y * 0.5f * dim.y, currentUpVector.z * 0.5f * dim.y)));
        }
        if (_faceAvatar) {
            endProps.insert("rotation", quatToVariant(faceAvatarRotation));
        }
        endProps.insert("visible", true);
        endProps.insert("ignoreRayIntersection", renderState.doesEndIgnoreRays());
        qApp->getOverlays().editOverlay(renderState.getEndID(), endProps);
    }
}

void LaserPointer::disableRenderState(const RenderState& renderState) {
    if (!renderState.getStartID().isNull()) {
        QVariantMap startProps;
        startProps.insert("visible", false);
        startProps.insert("ignoreRayIntersection", true);
        qApp->getOverlays().editOverlay(renderState.getStartID(), startProps);
    }
    if (!renderState.getPathID().isNull()) {
        QVariantMap pathProps;
        pathProps.insert("visible", false);
        pathProps.insert("ignoreRayIntersection", true);
        qApp->getOverlays().editOverlay(renderState.getPathID(), pathProps);
    }
    if (!renderState.getEndID().isNull()) {
        QVariantMap endProps;
        endProps.insert("visible", false);
        endProps.insert("ignoreRayIntersection", true);
        qApp->getOverlays().editOverlay(renderState.getEndID(), endProps);
    }
}

void LaserPointer::updateVisuals(const PickResultPointer& pickResult) {
    auto rayPickResult = std::static_pointer_cast<const RayPickResult>(pickResult);

    IntersectionType type = rayPickResult ? rayPickResult->type : IntersectionType::NONE;
    if (_enabled && !_currentRenderState.empty() && _renderStates.find(_currentRenderState) != _renderStates.end() &&
        (type != IntersectionType::NONE || _laserLength > 0.0f || !_objectLockEnd.first.isNull())) {
        PickRay pickRay{ rayPickResult->pickVariant };
        QUuid uid = rayPickResult->objectID;
        float distance = _laserLength > 0.0f ? _laserLength : rayPickResult->distance;
        updateRenderState(_renderStates[_currentRenderState], type, distance, uid, pickRay, false);
        disableRenderState(_defaultRenderStates[_currentRenderState].second);
    } else if (_enabled && !_currentRenderState.empty() && _defaultRenderStates.find(_currentRenderState) != _defaultRenderStates.end()) {
        disableRenderState(_renderStates[_currentRenderState]);
        PickRay pickRay = rayPickResult ? PickRay(rayPickResult->pickVariant) : PickRay();
        updateRenderState(_defaultRenderStates[_currentRenderState].second, IntersectionType::NONE, _defaultRenderStates[_currentRenderState].first, QUuid(), pickRay, true);
    } else if (!_currentRenderState.empty()) {
        disableRenderState(_renderStates[_currentRenderState]);
        disableRenderState(_defaultRenderStates[_currentRenderState].second);
    }
}

Pointer::PickedObject LaserPointer::getHoveredObject(const PickResultPointer& pickResult) {
    auto rayPickResult = std::static_pointer_cast<const RayPickResult>(pickResult);
    if (!rayPickResult) {
        return PickedObject();
    }
    return PickedObject(rayPickResult->objectID, rayPickResult->type);
}

Pointer::Buttons LaserPointer::getPressedButtons() {
    std::unordered_set<std::string> toReturn;
    for (const PointerTrigger& trigger : _triggers) {
        // TODO: right now, LaserPointers don't support axes, only on/off buttons
        if (trigger.getEndpoint()->peek() >= 1.0f) {
            toReturn.insert(trigger.getButton());
        }
    }
    return toReturn;
}

void LaserPointer::setLength(float length) {
    withWriteLock([&] {
        _laserLength = length;
    });
}

void LaserPointer::setLockEndUUID(const QUuid& objectID, bool isOverlay) {
    withWriteLock([&] {
        _objectLockEnd = std::pair<QUuid, bool>(objectID, isOverlay);
    });
}

RenderState::RenderState(const OverlayID& startID, const OverlayID& pathID, const OverlayID& endID) :
    _startID(startID), _pathID(pathID), _endID(endID)
{
    if (!_startID.isNull()) {
        _startIgnoreRays = qApp->getOverlays().getProperty(_startID, "ignoreRayIntersection").value.toBool();
    }
    if (!_pathID.isNull()) {
        _pathIgnoreRays = qApp->getOverlays().getProperty(_pathID, "ignoreRayIntersection").value.toBool();
    }
    if (!_endID.isNull()) {
        _endDim = vec3FromVariant(qApp->getOverlays().getProperty(_endID, "dimensions").value);
        _endIgnoreRays = qApp->getOverlays().getProperty(_endID, "ignoreRayIntersection").value.toBool();
    }
}

void RenderState::deleteOverlays() {
    if (!_startID.isNull()) {
        qApp->getOverlays().deleteOverlay(_startID);
    }
    if (!_pathID.isNull()) {
        qApp->getOverlays().deleteOverlay(_pathID);
    }
    if (!_endID.isNull()) {
        qApp->getOverlays().deleteOverlay(_endID);
    }
}

RenderState LaserPointer::buildRenderState(const QVariantMap& propMap) {
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
        // right now paths must be line3ds
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

    return RenderState(startID, pathID, endID);
}

PointerEvent LaserPointer::buildPointerEvent(const PickedObject& target, const PickResultPointer& pickResult) const {
    QUuid pickedID;
    glm::vec3 intersection, surfaceNormal, direction, origin;
    if (target.type != NONE) {
        auto rayPickResult = std::static_pointer_cast<RayPickResult>(pickResult);
        intersection = rayPickResult->intersection;
        surfaceNormal = rayPickResult->surfaceNormal;
        const QVariantMap& searchRay = rayPickResult->pickVariant;
        direction = vec3FromVariant(searchRay["direction"]);
        origin = vec3FromVariant(searchRay["origin"]);
        pickedID = rayPickResult->objectID;;
    }

    glm::vec2 pos2D;
    if (pickedID != target.objectID) {
        if (target.type == ENTITY) {
            intersection = intersectRayWithEntityXYPlane(target.objectID, origin, direction);
        } else if (target.type == OVERLAY) {
            intersection = intersectRayWithOverlayXYPlane(target.objectID, origin, direction);
        }
    }
    if (target.type == ENTITY) {
        pos2D = projectOntoEntityXYPlane(target.objectID, intersection);
    } else if (target.type == OVERLAY) {
        pos2D = projectOntoOverlayXYPlane(target.objectID, intersection);
    } else if (target.type == HUD) {
        pos2D = DependencyManager::get<PickManager>()->calculatePos2DFromHUD(intersection);
    }
    return PointerEvent(pos2D, intersection, surfaceNormal, direction);
}

glm::vec3 LaserPointer::intersectRayWithXYPlane(const glm::vec3& origin, const glm::vec3& direction, const glm::vec3& point, const glm::quat rotation, const glm::vec3& registration) const {
    glm::vec3 n = rotation * Vectors::FRONT;
    float t = glm::dot(n, point - origin) / glm::dot(n, direction);
    return origin + t * direction;
}

glm::vec3 LaserPointer::intersectRayWithOverlayXYPlane(const QUuid& overlayID, const glm::vec3& origin, const glm::vec3& direction) const {
    glm::vec3 position = vec3FromVariant(qApp->getOverlays().getProperty(overlayID, "position").value);
    glm::quat rotation = quatFromVariant(qApp->getOverlays().getProperty(overlayID, "rotation").value);
    const glm::vec3 DEFAULT_REGISTRATION_POINT = glm::vec3(0.5f);
    return intersectRayWithXYPlane(origin, direction, position, rotation, DEFAULT_REGISTRATION_POINT);
}

glm::vec3 LaserPointer::intersectRayWithEntityXYPlane(const QUuid& entityID, const glm::vec3& origin, const glm::vec3& direction) const {
    auto props = DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(entityID);
    return intersectRayWithXYPlane(origin, direction, props.getPosition(), props.getRotation(), props.getRegistrationPoint());
}

glm::vec2 LaserPointer::projectOntoXYPlane(const glm::vec3& worldPos, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& dimensions, const glm::vec3& registrationPoint) const {
    glm::quat invRot = glm::inverse(rotation);
    glm::vec3 localPos = invRot * (worldPos - position);
    glm::vec3 invDimensions = glm::vec3(1.0f / dimensions.x, 1.0f / dimensions.y, 1.0f / dimensions.z);

    glm::vec3 normalizedPos = (localPos * invDimensions) + registrationPoint;
    return glm::vec2(normalizedPos.x * dimensions.x, (1.0f - normalizedPos.y) * dimensions.y);
}

glm::vec2 LaserPointer::projectOntoOverlayXYPlane(const QUuid& overlayID, const glm::vec3& worldPos) const {
    glm::vec3 position = vec3FromVariant(qApp->getOverlays().getProperty(overlayID, "position").value);
    glm::quat rotation = quatFromVariant(qApp->getOverlays().getProperty(overlayID, "rotation").value);
    glm::vec3 dimensions;

    float dpi = qApp->getOverlays().getProperty(overlayID, "dpi").value.toFloat();
    if (dpi > 0) {
        // Calculate physical dimensions for web3d overlay from resolution and dpi; "dimensions" property is used as a scale.
        glm::vec3 resolution = glm::vec3(vec2FromVariant(qApp->getOverlays().getProperty(overlayID, "resolution").value), 1);
        glm::vec3 scale = glm::vec3(vec2FromVariant(qApp->getOverlays().getProperty(overlayID, "dimensions").value), 0.01f);
        const float INCHES_TO_METERS = 1.0f / 39.3701f;
        dimensions = (resolution * INCHES_TO_METERS / dpi) * scale;
    } else {
        dimensions = glm::vec3(vec2FromVariant(qApp->getOverlays().getProperty(overlayID, "dimensions").value), 0.01);
    }

    const glm::vec3 DEFAULT_REGISTRATION_POINT = glm::vec3(0.5f);
    return projectOntoXYPlane(worldPos, position, rotation, dimensions, DEFAULT_REGISTRATION_POINT);
}

glm::vec2 LaserPointer::projectOntoEntityXYPlane(const QUuid& entityID, const glm::vec3& worldPos) const {
    auto props = DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(entityID);
    return projectOntoXYPlane(worldPos, props.getPosition(), props.getRotation(), props.getDimensions(), props.getRegistrationPoint());
}
