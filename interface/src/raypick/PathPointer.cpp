//
//  Created by Sam Gondelman 7/17/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "PathPointer.h"

#include "Application.h"
#include "avatar/AvatarManager.h"

#include <DependencyManager.h>
#include <PickManager.h>
#include "PickScriptingInterface.h"
#include "RayPick.h"

PathPointer::PathPointer(PickQuery::PickType type, const QVariant& rayProps, const RenderStateMap& renderStates, const DefaultRenderStateMap& defaultRenderStates,
                         bool hover, const PointerTriggers& triggers, bool faceAvatar, bool followNormal, float followNormalStrength, bool centerEndY, bool lockEnd,
                         bool distanceScaleEnd, bool scaleWithAvatar, bool enabled) :
    Pointer(DependencyManager::get<PickScriptingInterface>()->createPick(type, rayProps), enabled, hover),
    _renderStates(renderStates),
    _defaultRenderStates(defaultRenderStates),
    _triggers(triggers),
    _faceAvatar(faceAvatar),
    _followNormal(followNormal),
    _followNormalStrength(followNormalStrength),
    _centerEndY(centerEndY),
    _lockEnd(lockEnd),
    _distanceScaleEnd(distanceScaleEnd),
    _scaleWithAvatar(scaleWithAvatar)
{
    for (auto& state : _renderStates) {
        if (!enabled || state.first != _currentRenderState) {
            state.second->disable();
        }
    }
    for (auto& state : _defaultRenderStates) {
        if (!enabled || state.first != _currentRenderState) {
            state.second.second->disable();
        }
    }
}

PathPointer::~PathPointer() {
    for (auto& renderState : _renderStates) {
        renderState.second->cleanup();
    }
    for (auto& renderState : _defaultRenderStates) {
        renderState.second.second->cleanup();
    }
}

void PathPointer::setRenderState(const std::string& state) {
    withWriteLock([&] {
        if (!_currentRenderState.empty() && state != _currentRenderState) {
            if (_renderStates.find(_currentRenderState) != _renderStates.end()) {
                _renderStates[_currentRenderState]->disable();
            }
            if (_defaultRenderStates.find(_currentRenderState) != _defaultRenderStates.end()) {
                _defaultRenderStates[_currentRenderState].second->disable();
            }
        }
        _currentRenderState = state;
    });
}

void PathPointer::setLength(float length) {
    withWriteLock([&] {
        _pathLength = length;
    });
}

void PathPointer::setLockEndUUID(const QUuid& objectID, const bool isOverlay, const glm::mat4& offsetMat) {
    withWriteLock([&] {
        _lockEndObject.id = objectID;
        _lockEndObject.isOverlay = isOverlay;
        _lockEndObject.offsetMat = offsetMat;
    });
}

PickResultPointer PathPointer::getVisualPickResult(const PickResultPointer& pickResult) {
    PickResultPointer visualPickResult = pickResult;
    glm::vec3 origin = getPickOrigin(pickResult);
    IntersectionType type = getPickedObjectType(pickResult);
    QUuid id;
    glm::vec3 intersection;
    float distance;
    glm::vec3 surfaceNormal;

    if (type != IntersectionType::HUD) {
        glm::vec3 endVec;
        if (!_lockEndObject.id.isNull()) {
            glm::vec3 pos;
            glm::quat rot;
            glm::vec3 dim;
            glm::vec3 registrationPoint;
            if (_lockEndObject.isOverlay) {
                pos = vec3FromVariant(qApp->getOverlays().getProperty(_lockEndObject.id, "position").value);
                rot = quatFromVariant(qApp->getOverlays().getProperty(_lockEndObject.id, "rotation").value);
                dim = vec3FromVariant(qApp->getOverlays().getProperty(_lockEndObject.id, "dimensions").value);
                registrationPoint = glm::vec3(0.5f);
            } else {
                EntityItemProperties props = DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(_lockEndObject.id);
                glm::mat4 entityMat = createMatFromQuatAndPos(props.getRotation(), props.getPosition());
                glm::mat4 finalPosAndRotMat = entityMat * _lockEndObject.offsetMat;
                pos = extractTranslation(finalPosAndRotMat);
                rot = glmExtractRotation(finalPosAndRotMat);
                dim = props.getDimensions();
                registrationPoint = props.getRegistrationPoint();
            }
            const glm::vec3 DEFAULT_REGISTRATION_POINT = glm::vec3(0.5f);
            endVec = pos + rot * (dim * (DEFAULT_REGISTRATION_POINT - registrationPoint));
            glm::vec3 direction = endVec - origin;
            distance = glm::distance(origin, endVec);
            glm::vec3 normalizedDirection = glm::normalize(direction);

            type = _lockEndObject.isOverlay ? IntersectionType::OVERLAY : IntersectionType::ENTITY;
            id = _lockEndObject.id;
            intersection = endVec;
            surfaceNormal = -normalizedDirection;
            setVisualPickResultInternal(visualPickResult, type, id, intersection, distance, surfaceNormal);
        } else if (type != IntersectionType::NONE && _lockEnd) {
            id = getPickedObjectID(pickResult);
            if (type == IntersectionType::ENTITY) {
                endVec = DependencyManager::get<EntityScriptingInterface>()->getEntityTransform(id)[3];
            } else if (type == IntersectionType::OVERLAY) {
                endVec = vec3FromVariant(qApp->getOverlays().getProperty(id, "position").value);
            } else if (type == IntersectionType::AVATAR) {
                endVec = DependencyManager::get<AvatarHashMap>()->getAvatar(id)->getPosition();
            }
            glm::vec3 direction = endVec - origin;
            distance = glm::distance(origin, endVec);
            glm::vec3 normalizedDirection = glm::normalize(direction);
            intersection = endVec;
            surfaceNormal = -normalizedDirection;
            setVisualPickResultInternal(visualPickResult, type, id, intersection, distance, surfaceNormal);
        }
    }
    return visualPickResult;
}

void PathPointer::updateVisuals(const PickResultPointer& pickResult) {
    IntersectionType type = getPickedObjectType(pickResult);
    if (_enabled && !_currentRenderState.empty() && _renderStates.find(_currentRenderState) != _renderStates.end() &&
        (type != IntersectionType::NONE || _pathLength > 0.0f)) {
        glm::vec3 origin = getPickOrigin(pickResult);
        glm::vec3 end = getPickEnd(pickResult, _pathLength);
        glm::vec3 surfaceNormal = getPickedObjectNormal(pickResult);
        _renderStates[_currentRenderState]->update(origin, end, surfaceNormal, _scaleWithAvatar, _distanceScaleEnd, _centerEndY, _faceAvatar,
                                                   _followNormal, _followNormalStrength, _pathLength, pickResult);
        if (_defaultRenderStates.find(_currentRenderState) != _defaultRenderStates.end()) {
            _defaultRenderStates[_currentRenderState].second->disable();
        }
    } else if (_enabled && !_currentRenderState.empty() && _defaultRenderStates.find(_currentRenderState) != _defaultRenderStates.end()) {
        if (_renderStates.find(_currentRenderState) != _renderStates.end()) {
            _renderStates[_currentRenderState]->disable();
        }
        glm::vec3 origin = getPickOrigin(pickResult);
        glm::vec3 end = getPickEnd(pickResult, _defaultRenderStates[_currentRenderState].first);
        _defaultRenderStates[_currentRenderState].second->update(origin, end, Vectors::UP, _scaleWithAvatar, _distanceScaleEnd, _centerEndY,
                                                                _faceAvatar, _followNormal, _followNormalStrength, _defaultRenderStates[_currentRenderState].first, pickResult);
    } else if (!_currentRenderState.empty()) {
        if (_renderStates.find(_currentRenderState) != _renderStates.end()) {
            _renderStates[_currentRenderState]->disable();
        }
        if (_defaultRenderStates.find(_currentRenderState) != _defaultRenderStates.end()) {
            _defaultRenderStates[_currentRenderState].second->disable();
        }
    }
}

void PathPointer::editRenderState(const std::string& state, const QVariant& startProps, const QVariant& pathProps, const QVariant& endProps) {
    withWriteLock([&] {
        updateRenderStateOverlay(_renderStates[state]->getStartID(), startProps);
        updateRenderStateOverlay(_renderStates[state]->getEndID(), endProps);
        QVariant startDim = startProps.toMap()["dimensions"];
        if (startDim.isValid()) {
            _renderStates[state]->setStartDim(vec3FromVariant(startDim));
        }
        QVariant endDim = endProps.toMap()["dimensions"];
        if (endDim.isValid()) {
            _renderStates[state]->setEndDim(vec3FromVariant(endDim));
        }
        QVariant rotation = endProps.toMap()["rotation"];
        if (rotation.isValid()) {
            _renderStates[state]->setEndRot(quatFromVariant(rotation));
        }

        editRenderStatePath(state, pathProps);
    });
}

void PathPointer::updateRenderStateOverlay(const OverlayID& id, const QVariant& props) {
    if (!id.isNull() && props.isValid()) {
        QVariantMap propMap = props.toMap();
        propMap.remove("visible");
        qApp->getOverlays().editOverlay(id, propMap);
    }
}

Pointer::PickedObject PathPointer::getHoveredObject(const PickResultPointer& pickResult) {
    return PickedObject(getPickedObjectID(pickResult), getPickedObjectType(pickResult));
}

Pointer::Buttons PathPointer::getPressedButtons(const PickResultPointer& pickResult) {
    std::unordered_set<std::string> toReturn;

    for (const PointerTrigger& trigger : _triggers) {
        std::string button = trigger.getButton();
        TriggerState& state = _states[button];
        // TODO: right now, LaserPointers don't support axes, only on/off buttons
        if (trigger.getEndpoint()->peek() >= 1.0f) {
            toReturn.insert(button);

            if (_previousButtons.find(button) == _previousButtons.end()) {
                // start triggering for buttons that were just pressed
                state.triggeredObject = PickedObject(getPickedObjectID(pickResult), getPickedObjectType(pickResult));
                state.intersection = getPickEnd(pickResult);
                state.triggerPos2D = findPos2D(state.triggeredObject, state.intersection);
                state.triggerStartTime = usecTimestampNow();
                state.surfaceNormal = getPickedObjectNormal(pickResult);
                state.deadspotExpired = false;
                state.wasTriggering = true;
                state.triggering = true;
                _latestState = state;
            }
        } else {
            // stop triggering for buttons that aren't pressed
            state.wasTriggering = state.triggering;
            state.triggering = false;
            _latestState = state;
        }
    }
    _previousButtons = toReturn;
    return toReturn;
}

StartEndRenderState::StartEndRenderState(const OverlayID& startID, const OverlayID& endID) :
    _startID(startID), _endID(endID) {
    if (!_startID.isNull()) {
        _startDim = vec3FromVariant(qApp->getOverlays().getProperty(_startID, "dimensions").value);
        _startIgnoreRays = qApp->getOverlays().getProperty(_startID, "ignoreRayIntersection").value.toBool();
    }
    if (!_endID.isNull()) {
        _endDim = vec3FromVariant(qApp->getOverlays().getProperty(_endID, "dimensions").value);
        _endRot = quatFromVariant(qApp->getOverlays().getProperty(_endID, "rotation").value);
        _endIgnoreRays = qApp->getOverlays().getProperty(_endID, "ignoreRayIntersection").value.toBool();
    }
}

void StartEndRenderState::cleanup() {
    if (!_startID.isNull()) {
        qApp->getOverlays().deleteOverlay(_startID);
    }
    if (!_endID.isNull()) {
        qApp->getOverlays().deleteOverlay(_endID);
    }
}

void StartEndRenderState::disable() {
    if (!getStartID().isNull()) {
        QVariantMap startProps;
        startProps.insert("visible", false);
        startProps.insert("ignoreRayIntersection", true);
        qApp->getOverlays().editOverlay(getStartID(), startProps);
    }
    if (!getEndID().isNull()) {
        QVariantMap endProps;
        endProps.insert("visible", false);
        endProps.insert("ignoreRayIntersection", true);
        qApp->getOverlays().editOverlay(getEndID(), endProps);
    }
}

void StartEndRenderState::update(const glm::vec3& origin, const glm::vec3& end, const glm::vec3& surfaceNormal, bool scaleWithAvatar, bool distanceScaleEnd, bool centerEndY,
                                 bool faceAvatar, bool followNormal, float followNormalStrength, float distance, const PickResultPointer& pickResult) {
    if (!getStartID().isNull()) {
        QVariantMap startProps;
        startProps.insert("position", vec3toVariant(origin));
        startProps.insert("visible", true);
        if (scaleWithAvatar) {
            startProps.insert("dimensions", vec3toVariant(getStartDim() * DependencyManager::get<AvatarManager>()->getMyAvatar()->getSensorToWorldScale()));
        }
        startProps.insert("ignoreRayIntersection", doesStartIgnoreRays());
        qApp->getOverlays().editOverlay(getStartID(), startProps);
    }

    if (!getEndID().isNull()) {
        QVariantMap endProps;
        glm::vec3 dim = vec3FromVariant(qApp->getOverlays().getProperty(getEndID(), "dimensions").value);
        if (distanceScaleEnd) {
            dim = getEndDim() * glm::distance(origin, end);
            endProps.insert("dimensions", vec3toVariant(dim));
        } else if (scaleWithAvatar) {
            dim = getEndDim() * DependencyManager::get<AvatarManager>()->getMyAvatar()->getSensorToWorldScale();
            endProps.insert("dimensions", vec3toVariant(dim));
        }

        glm::quat normalQuat = Quat().lookAtSimple(Vectors::ZERO, surfaceNormal);
        normalQuat = normalQuat * glm::quat(glm::vec3(-M_PI_2, 0, 0));
        glm::vec3 avatarUp = DependencyManager::get<AvatarManager>()->getMyAvatar()->getWorldOrientation() * Vectors::UP;
        glm::quat rotation = glm::rotation(Vectors::UP, avatarUp);
        glm::vec3 position = end;
        if (!centerEndY) {
            if (followNormal) {
                position = end + 0.5f * dim.y * surfaceNormal;
            } else {
                position = end + 0.5f * dim.y * avatarUp;
            }
        }
        if (faceAvatar) {
            glm::quat orientation = followNormal ? normalQuat : DependencyManager::get<AvatarManager>()->getMyAvatar()->getWorldOrientation();
            glm::quat lookAtWorld = Quat().lookAt(position, DependencyManager::get<AvatarManager>()->getMyAvatar()->getWorldPosition(), surfaceNormal);
            glm::quat lookAtModel = glm::inverse(orientation) * lookAtWorld;
            glm::quat lookAtFlatModel = Quat().cancelOutRollAndPitch(lookAtModel);
            glm::quat lookAtFlatWorld = orientation * lookAtFlatModel;
            rotation = lookAtFlatWorld;
        } else if (followNormal) {
            rotation = normalQuat;
        }
        if (followNormal && followNormalStrength > 0.0f && followNormalStrength < 1.0f) {
            if (!_avgEndRotInitialized) {
                _avgEndRot = rotation;
                _avgEndRotInitialized = true;
            } else {
                rotation = glm::slerp(_avgEndRot, rotation, followNormalStrength);
                if (!centerEndY) {
                    position = end + 0.5f * dim.y * (rotation * Vectors::UP);
                }
                _avgEndRot = rotation;
            }
        }
        endProps.insert("position", vec3toVariant(position));
        endProps.insert("rotation", quatToVariant(rotation));
        endProps.insert("visible", true);
        endProps.insert("ignoreRayIntersection", doesEndIgnoreRays());
        qApp->getOverlays().editOverlay(getEndID(), endProps);
    }
}

glm::vec2 PathPointer::findPos2D(const PickedObject& pickedObject, const glm::vec3& origin) {
    switch (pickedObject.type) {
    case ENTITY:
        return RayPick::projectOntoEntityXYPlane(pickedObject.objectID, origin);
    case OVERLAY:
        return RayPick::projectOntoOverlayXYPlane(pickedObject.objectID, origin);
    case HUD:
        return DependencyManager::get<PickManager>()->calculatePos2DFromHUD(origin);
    default:
        return glm::vec2(NAN);
    }
}