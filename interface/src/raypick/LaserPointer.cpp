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

#include "RayPickManager.h"
#include "JointRayPick.h"

#include "Application.h"

LaserPointer::LaserPointer(const QString& jointName, const glm::vec3& posOffset, const glm::vec3& dirOffset, const uint16_t filter, const float maxDistance,
    const QHash<QString, RenderState>& renderStates, const bool enabled) :
    _renderingEnabled(enabled),
    _renderStates(renderStates)
{
    _rayPickUID = RayPickManager::getInstance().addRayPick(std::make_shared<JointRayPick>(jointName, posOffset, dirOffset, filter, maxDistance, enabled));

    if (!enabled) {
        disableCurrentRenderState();
    }
}

LaserPointer::~LaserPointer() {
    RayPickManager::getInstance().removeRayPick(_rayPickUID);
    for (RenderState& renderState : _renderStates) {
        if (!renderState.getStartID().isNull()) qApp->getOverlays().deleteOverlay(renderState.getStartID());
        if (!renderState.getPathID().isNull()) qApp->getOverlays().deleteOverlay(renderState.getPathID());
        if (!renderState.getEndID().isNull()) qApp->getOverlays().deleteOverlay(renderState.getEndID());
    }
}

void LaserPointer::enable() {
    RayPickManager::getInstance().enableRayPick(_rayPickUID);
    _renderingEnabled = true;
}

void LaserPointer::disable() {
    RayPickManager::getInstance().disableRayPick(_rayPickUID);
    _renderingEnabled = false;
    if (!_currentRenderState.isEmpty() && _renderStates.contains(_currentRenderState)) disableCurrentRenderState();
}

void LaserPointer::setRenderState(const QString& state) {
    if (!_currentRenderState.isEmpty() && _renderStates.contains(_currentRenderState)) disableCurrentRenderState();
    _currentRenderState = state;
}

const RayPickResult& LaserPointer::getPrevRayPickResult() {
    return RayPickManager::getInstance().getPrevRayPickResult(_rayPickUID);
}

void LaserPointer::disableCurrentRenderState() {
    if (!_renderStates[_currentRenderState].getStartID().isNull()) {
        QVariantMap startProps;
        startProps.insert("visible", false);
        startProps.insert("ignoreRayIntersection", true);
        qApp->getOverlays().editOverlay(_renderStates[_currentRenderState].getStartID(), startProps);
    }
    if (!_renderStates[_currentRenderState].getPathID().isNull()) {
        QVariantMap pathProps;
        pathProps.insert("visible", false);
        pathProps.insert("ignoreRayIntersection", true);
        qApp->getOverlays().editOverlay(_renderStates[_currentRenderState].getPathID(), pathProps);
    }
    if (!_renderStates[_currentRenderState].getEndID().isNull()) {
        QVariantMap endProps;
        endProps.insert("visible", false);
        endProps.insert("ignoreRayIntersection", true);
        qApp->getOverlays().editOverlay(_renderStates[_currentRenderState].getEndID(), endProps);
    }
}

void LaserPointer::update() {
    RayPickResult prevRayPickResult = RayPickManager::getInstance().getPrevRayPickResult(_rayPickUID);
    if (_renderingEnabled && !_currentRenderState.isEmpty() && _renderStates.contains(_currentRenderState) && prevRayPickResult.type != IntersectionType::NONE) {
        PickRay pickRay = RayPickManager::getInstance().getPickRay(_rayPickUID);
        if (!_renderStates[_currentRenderState].getStartID().isNull()) {
            QVariantMap startProps;
            startProps.insert("position", vec3toVariant(pickRay.origin));
            startProps.insert("visible", true);
            startProps.insert("ignoreRayIntersection", _renderStates[_currentRenderState].doesStartIgnoreRays());
            qApp->getOverlays().editOverlay(_renderStates[_currentRenderState].getStartID(), startProps);
        }
        QVariant end = vec3toVariant(pickRay.origin + pickRay.direction * prevRayPickResult.distance);
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
            endProps.insert("position", end);
            endProps.insert("visible", true);
            endProps.insert("ignoreRayIntersection", _renderStates[_currentRenderState].doesEndIgnoreRays());
            qApp->getOverlays().editOverlay(_renderStates[_currentRenderState].getEndID(), endProps);
        }
    } else {
        disableCurrentRenderState();
    }
}

void LaserPointer::render(RenderArgs* args) {
    if (_renderingEnabled && !_currentRenderState.isEmpty() && _renderStates.contains(_currentRenderState)) {
        _renderStates[_currentRenderState].render(args);
    }
}

RenderState::RenderState(const OverlayID& startID, const OverlayID& pathID, const OverlayID& endID) :
    _startID(startID), _pathID(pathID), _endID(endID)
{
    if (!_startID.isNull()) _startIgnoreRays = qApp->getOverlays().getOverlay(_startID)->getProperty("ignoreRayIntersection").toBool();
    if (!_pathID.isNull()) _pathIgnoreRays = qApp->getOverlays().getOverlay(_pathID)->getProperty("ignoreRayIntersection").toBool();
    if (!_endID.isNull()) _endIgnoreRays = qApp->getOverlays().getOverlay(_endID)->getProperty("ignoreRayIntersection").toBool();
}

void RenderState::render(RenderArgs * args) {
    if (!_startID.isNull()) qApp->getOverlays().getOverlay(_startID)->render(args);
    if (!_pathID.isNull()) qApp->getOverlays().getOverlay(_pathID)->render(args);
    if (!_endID.isNull()) qApp->getOverlays().getOverlay(_endID)->render(args);
}