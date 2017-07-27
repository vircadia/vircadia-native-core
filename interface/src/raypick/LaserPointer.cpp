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

LaserPointer::LaserPointer(const QVariantMap& rayProps, const QHash<QString, RenderState>& renderStates, QHash<QString, QPair<float, RenderState>>& defaultRenderStates,
        const bool faceAvatar, const bool centerEndY, const bool lockEnd, const bool enabled) :
    _renderingEnabled(enabled),
    _renderStates(renderStates),
    _defaultRenderStates(defaultRenderStates),
    _faceAvatar(faceAvatar),
    _centerEndY(centerEndY),
    _lockEnd(lockEnd)
{
    _rayPickUID = DependencyManager::get<RayPickManager>()->createRayPick(rayProps);

    for (QString& state : _renderStates.keys()) {
        if (!enabled || state != _currentRenderState) {
            disableRenderState(_renderStates[state]);
        }
    }
    for (QString& state : _defaultRenderStates.keys()) {
        if (!enabled || state != _currentRenderState) {
            disableRenderState(_defaultRenderStates[state].second);
        }
    }
}

LaserPointer::~LaserPointer() {
    DependencyManager::get<RayPickManager>()->removeRayPick(_rayPickUID);
    for (RenderState& renderState : _renderStates) {
        if (!renderState.getStartID().isNull()) {
            qApp->getOverlays().deleteOverlay(renderState.getStartID());
        }
        if (!renderState.getPathID().isNull()) {
            qApp->getOverlays().deleteOverlay(renderState.getPathID());
        }
        if (!renderState.getEndID().isNull()) {
            qApp->getOverlays().deleteOverlay(renderState.getEndID());
        }
    }
    for (QPair<float, RenderState>& renderState : _defaultRenderStates) {
        if (!renderState.second.getStartID().isNull()) {
            qApp->getOverlays().deleteOverlay(renderState.second.getStartID());
        }
        if (!renderState.second.getPathID().isNull()) {
            qApp->getOverlays().deleteOverlay(renderState.second.getPathID());
        }
        if (!renderState.second.getEndID().isNull()) {
            qApp->getOverlays().deleteOverlay(renderState.second.getEndID());
        }
    }
}

void LaserPointer::enable() {
    DependencyManager::get<RayPickManager>()->enableRayPick(_rayPickUID);
    _renderingEnabled = true;
}

void LaserPointer::disable() {
    DependencyManager::get<RayPickManager>()->disableRayPick(_rayPickUID);
    _renderingEnabled = false;
    if (!_currentRenderState.isEmpty()) {
        if (_renderStates.contains(_currentRenderState)) {
            disableRenderState(_renderStates[_currentRenderState]);
        }
        if (_defaultRenderStates.contains(_currentRenderState)) {
            disableRenderState(_defaultRenderStates[_currentRenderState].second);
        }
    }
}

void LaserPointer::setRenderState(const QString& state) {
    if (!_currentRenderState.isEmpty()) {
        if (_renderStates.contains(_currentRenderState)) {
            disableRenderState(_renderStates[_currentRenderState]);
        }
        if (_defaultRenderStates.contains(_currentRenderState)) {
            disableRenderState(_defaultRenderStates[_currentRenderState].second);
        }
    }
    _currentRenderState = state;
}

void LaserPointer::editRenderState(const QString& state, const QVariant& startProps, const QVariant& pathProps, const QVariant& endProps) {
    _renderStates[state].setStartID(updateRenderStateOverlay(_renderStates[state].getStartID(), startProps));
    _renderStates[state].setPathID(updateRenderStateOverlay(_renderStates[state].getPathID(), pathProps));
    _renderStates[state].setEndID(updateRenderStateOverlay(_renderStates[state].getEndID(), endProps));
}

OverlayID LaserPointer::updateRenderStateOverlay(const OverlayID& id, const QVariant& props) {
    if (props.isValid()) {
        if (!id.isNull()) {
            qApp->getOverlays().editOverlay(id, props);
            return id;
        } else {
            QVariantMap propsMap = props.toMap();
            if (propsMap["type"].isValid()) {
                return qApp->getOverlays().addOverlay(propsMap["type"].toString(), props);
            }
        }
    }
    return OverlayID();
}

void LaserPointer::updateRenderState(const RenderState& renderState, const IntersectionType type, const float distance, const QUuid& objectID, const bool defaultState) {
    PickRay pickRay = DependencyManager::get<RayPickManager>()->getPickRay(_rayPickUID);
    if (!renderState.getStartID().isNull()) {
        QVariantMap startProps;
        startProps.insert("position", vec3toVariant(pickRay.origin));
        startProps.insert("visible", true);
        startProps.insert("ignoreRayIntersection", renderState.doesStartIgnoreRays());
        qApp->getOverlays().editOverlay(renderState.getStartID(), startProps);
    }
    glm::vec3 endVec;
    if (defaultState || !_lockEnd || type == IntersectionType::HUD) {
        endVec = pickRay.origin + pickRay.direction * distance;
    }
    else {
        if (type == IntersectionType::ENTITY) {
            endVec = DependencyManager::get<EntityScriptingInterface>()->getEntityTransform(objectID)[3];
        }
        else if (type == IntersectionType::OVERLAY) {
            endVec = vec3FromVariant(qApp->getOverlays().getProperty(objectID, "position").value);
        }
        else if (type == IntersectionType::AVATAR) {
            endVec = DependencyManager::get<AvatarHashMap>()->getAvatar(objectID)->getPosition();
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
        if (_centerEndY) {
            endProps.insert("position", end);
        }
        else {
            glm::vec3 dim = vec3FromVariant(qApp->getOverlays().getProperty(renderState.getEndID(), "dimensions").value);
            endProps.insert("position", vec3toVariant(endVec + glm::vec3(0, 0.5f * dim.y, 0)));
        }
        if (_faceAvatar) {
            glm::quat rotation = glm::inverse(glm::quat_cast(glm::lookAt(endVec, DependencyManager::get<AvatarManager>()->getMyAvatar()->getPosition(), Vectors::UP)));
            endProps.insert("rotation", quatToVariant(glm::quat(glm::radians(glm::vec3(0, glm::degrees(safeEulerAngles(rotation)).y, 0)))));
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
    RayPickResult prevRayPickResult = DependencyManager::get<RayPickManager>()->getPrevRayPickResult(_rayPickUID);
    if (_renderingEnabled && !_currentRenderState.isEmpty() && _renderStates.contains(_currentRenderState) && prevRayPickResult.type != IntersectionType::NONE) {
        updateRenderState(_renderStates[_currentRenderState], prevRayPickResult.type, prevRayPickResult.distance, prevRayPickResult.objectID, false);
        disableRenderState(_defaultRenderStates[_currentRenderState].second);
    } else if (_renderingEnabled && !_currentRenderState.isEmpty() && _defaultRenderStates.contains(_currentRenderState)) {
        disableRenderState(_renderStates[_currentRenderState]);
        updateRenderState(_defaultRenderStates[_currentRenderState].second, IntersectionType::NONE, _defaultRenderStates[_currentRenderState].first, QUuid(), true);
    } else if (!_currentRenderState.isEmpty()) {
        disableRenderState(_renderStates[_currentRenderState]);
        disableRenderState(_defaultRenderStates[_currentRenderState].second);
    }
}

RenderState::RenderState(const OverlayID& startID, const OverlayID& pathID, const OverlayID& endID) :
    _startID(startID), _pathID(pathID), _endID(endID)
{
    if (!_startID.isNull()) {
        _startIgnoreRays = qApp->getOverlays().getOverlay(_startID)->getProperty("ignoreRayIntersection").toBool();
    }
    if (!_pathID.isNull()) {
        _pathIgnoreRays = qApp->getOverlays().getOverlay(_pathID)->getProperty("ignoreRayIntersection").toBool();
    }
    if (!_endID.isNull()) {
        _endIgnoreRays = qApp->getOverlays().getOverlay(_endID)->getProperty("ignoreRayIntersection").toBool();
    }
}