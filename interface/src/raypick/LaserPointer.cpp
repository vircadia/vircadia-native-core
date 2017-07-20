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

LaserPointer::LaserPointer(const QVariantMap& rayProps, const QHash<QString, RenderState>& renderStates, const bool faceAvatar, const bool centerEndY, const bool lockEnd, const bool enabled) :
    _renderingEnabled(enabled),
    _renderStates(renderStates),
    _faceAvatar(faceAvatar),
    _centerEndY(centerEndY),
    _lockEnd(lockEnd)
{
    _rayPickUID = DependencyManager::get<RayPickManager>()->createRayPick(rayProps);

    for (auto& state : _renderStates.keys()) {
        if (!enabled || state != _currentRenderState) {
            disableRenderState(state);
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
}

void LaserPointer::enable() {
    DependencyManager::get<RayPickManager>()->enableRayPick(_rayPickUID);
    _renderingEnabled = true;
}

void LaserPointer::disable() {
    DependencyManager::get<RayPickManager>()->disableRayPick(_rayPickUID);
    _renderingEnabled = false;
    if (!_currentRenderState.isEmpty() && _renderStates.contains(_currentRenderState)) {
        disableRenderState(_currentRenderState);
    }
}

void LaserPointer::setRenderState(const QString& state) {
    if (!_currentRenderState.isEmpty() && _renderStates.contains(_currentRenderState)) {
        disableRenderState(_currentRenderState);
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

void LaserPointer::disableRenderState(const QString& renderState) {
    if (!_renderStates[renderState].getStartID().isNull()) {
        QVariantMap startProps;
        startProps.insert("visible", false);
        startProps.insert("ignoreRayIntersection", true);
        qApp->getOverlays().editOverlay(_renderStates[renderState].getStartID(), startProps);
    }
    if (!_renderStates[renderState].getPathID().isNull()) {
        QVariantMap pathProps;
        pathProps.insert("visible", false);
        pathProps.insert("ignoreRayIntersection", true);
        qApp->getOverlays().editOverlay(_renderStates[renderState].getPathID(), pathProps);
    }
    if (!_renderStates[renderState].getEndID().isNull()) {
        QVariantMap endProps;
        endProps.insert("visible", false);
        endProps.insert("ignoreRayIntersection", true);
        qApp->getOverlays().editOverlay(_renderStates[renderState].getEndID(), endProps);
    }
}

void LaserPointer::update() {
    RayPickResult prevRayPickResult = DependencyManager::get<RayPickManager>()->getPrevRayPickResult(_rayPickUID);
    if (_renderingEnabled && !_currentRenderState.isEmpty() && _renderStates.contains(_currentRenderState) && prevRayPickResult.type != IntersectionType::NONE) {
        PickRay pickRay = DependencyManager::get<RayPickManager>()->getPickRay(_rayPickUID);
        if (!_renderStates[_currentRenderState].getStartID().isNull()) {
            QVariantMap startProps;
            startProps.insert("position", vec3toVariant(pickRay.origin));
            startProps.insert("visible", true);
            startProps.insert("ignoreRayIntersection", _renderStates[_currentRenderState].doesStartIgnoreRays());
            qApp->getOverlays().editOverlay(_renderStates[_currentRenderState].getStartID(), startProps);
        }
        glm::vec3 endVec;
        if (!_lockEnd || prevRayPickResult.type == IntersectionType::HUD) {
            endVec = pickRay.origin + pickRay.direction * prevRayPickResult.distance;
        } else {
            if (prevRayPickResult.type == IntersectionType::ENTITY) {
                endVec = DependencyManager::get<EntityScriptingInterface>()->getEntityTransform(prevRayPickResult.objectID)[3];
            } else if (prevRayPickResult.type == IntersectionType::OVERLAY) {
                endVec = vec3FromVariant(qApp->getOverlays().getProperty(prevRayPickResult.objectID, "position").value);
            } else if (prevRayPickResult.type == IntersectionType::AVATAR) {
                endVec = DependencyManager::get<AvatarHashMap>()->getAvatar(prevRayPickResult.objectID)->getPosition();
            }
        }
        QVariant end = vec3toVariant(endVec);
        if (!_renderStates[_currentRenderState].getPathID().isNull()) {
            QVariantMap pathProps;
            pathProps.insert("start", vec3toVariant(pickRay.origin));
            pathProps.insert("end", end);
            pathProps.insert("visible", true);
            pathProps.insert("ignoreRayIntersection", _renderStates[_currentRenderState].doesPathIgnoreRays());
            qApp->getOverlays().editOverlay(_renderStates[_currentRenderState].getPathID(), pathProps);
        }
        if (!_renderStates[_currentRenderState].getEndID().isNull()) {
            QVariantMap endProps;
            if (_centerEndY) {
                endProps.insert("position", end);
            } else {
                glm::vec3 dim = vec3FromVariant(qApp->getOverlays().getProperty(_renderStates[_currentRenderState].getEndID(), "dimensions").value);
                endProps.insert("position", vec3toVariant(endVec + glm::vec3(0, 0.5f * dim.y, 0)));
            }
            if (_faceAvatar) {
                glm::quat rotation = glm::inverse(glm::quat_cast(glm::lookAt(endVec, DependencyManager::get<AvatarManager>()->getMyAvatar()->getPosition(), Vectors::UP)));
                endProps.insert("rotation", quatToVariant(glm::quat(glm::radians(glm::vec3(0, glm::degrees(safeEulerAngles(rotation)).y, 0)))));
            }
            endProps.insert("visible", true);
            endProps.insert("ignoreRayIntersection", _renderStates[_currentRenderState].doesEndIgnoreRays());
            qApp->getOverlays().editOverlay(_renderStates[_currentRenderState].getEndID(), endProps);
        }
    } else {
        disableRenderState(_currentRenderState);
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