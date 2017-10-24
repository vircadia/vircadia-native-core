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
#include "RayPickScriptingInterface.h"

LaserPointer::LaserPointer(const QVariant& rayProps, const RenderStateMap& renderStates, const DefaultRenderStateMap& defaultRenderStates,
        const bool faceAvatar, const bool centerEndY, const bool lockEnd, const bool distanceScaleEnd, const bool enabled) :
    _renderingEnabled(enabled),
    _renderStates(renderStates),
    _defaultRenderStates(defaultRenderStates),
    _faceAvatar(faceAvatar),
    _centerEndY(centerEndY),
    _lockEnd(lockEnd),
    _distanceScaleEnd(distanceScaleEnd),
    _rayPickUID(DependencyManager::get<RayPickScriptingInterface>()->createRayPick(rayProps))
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
    qApp->getRayPickManager().removeRayPick(_rayPickUID);

    for (auto& renderState : _renderStates) {
        renderState.second.deleteOverlays();
    }
    for (auto& renderState : _defaultRenderStates) {
        renderState.second.second.deleteOverlays();
    }
}

void LaserPointer::enable() {
    qApp->getRayPickManager().enableRayPick(_rayPickUID);
    withWriteLock([&] {
        _renderingEnabled = true;
    });
}

void LaserPointer::disable() {
    qApp->getRayPickManager().disableRayPick(_rayPickUID);
    withWriteLock([&] {
        _renderingEnabled = false;
        if (!_currentRenderState.empty()) {
            if (_renderStates.find(_currentRenderState) != _renderStates.end()) {
                disableRenderState(_renderStates[_currentRenderState]);
            }
            if (_defaultRenderStates.find(_currentRenderState) != _defaultRenderStates.end()) {
                disableRenderState(_defaultRenderStates[_currentRenderState].second);
            }
        }
    });
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

const RayPickResult LaserPointer::getPrevRayPickResult() {
    return qApp->getRayPickManager().getPrevRayPickResult(_rayPickUID);
}

void LaserPointer::updateRenderState(const RenderState& renderState, const IntersectionType type, const float distance, const QUuid& objectID, const PickRay& pickRay, const bool defaultState) {
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

void LaserPointer::update() {
    // This only needs to be a read lock because update won't change any of the properties that can be modified from scripts
    withReadLock([&] {
        RayPickResult prevRayPickResult = qApp->getRayPickManager().getPrevRayPickResult(_rayPickUID);
        if (_renderingEnabled && !_currentRenderState.empty() && _renderStates.find(_currentRenderState) != _renderStates.end() &&
            (prevRayPickResult.type != IntersectionType::NONE || _laserLength > 0.0f || !_objectLockEnd.first.isNull())) {
            float distance = _laserLength > 0.0f ? _laserLength : prevRayPickResult.distance;
            updateRenderState(_renderStates[_currentRenderState], prevRayPickResult.type, distance, prevRayPickResult.objectID, prevRayPickResult.searchRay, false);
            disableRenderState(_defaultRenderStates[_currentRenderState].second);
        } else if (_renderingEnabled && !_currentRenderState.empty() && _defaultRenderStates.find(_currentRenderState) != _defaultRenderStates.end()) {
            disableRenderState(_renderStates[_currentRenderState]);
            updateRenderState(_defaultRenderStates[_currentRenderState].second, IntersectionType::NONE, _defaultRenderStates[_currentRenderState].first, QUuid(), prevRayPickResult.searchRay, true);
        } else if (!_currentRenderState.empty()) {
            disableRenderState(_renderStates[_currentRenderState]);
            disableRenderState(_defaultRenderStates[_currentRenderState].second);
        }
    });
}

void LaserPointer::setPrecisionPicking(const bool precisionPicking) {
    qApp->getRayPickManager().setPrecisionPicking(_rayPickUID, precisionPicking);
}

void LaserPointer::setLaserLength(const float laserLength) {
    withWriteLock([&] {
        _laserLength = laserLength;
    });
}

void LaserPointer::setLockEndUUID(QUuid objectID, const bool isOverlay) {
    withWriteLock([&] {
        _objectLockEnd = std::pair<QUuid, bool>(objectID, isOverlay);
    });
}

void LaserPointer::setIgnoreItems(const QVector<QUuid>& ignoreItems) const {
    qApp->getRayPickManager().setIgnoreItems(_rayPickUID, ignoreItems);
}

void LaserPointer::setIncludeItems(const QVector<QUuid>& includeItems) const {
    qApp->getRayPickManager().setIncludeItems(_rayPickUID, includeItems);
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
