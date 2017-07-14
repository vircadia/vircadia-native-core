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
}

LaserPointer::~LaserPointer() {
    RayPickManager::getInstance().removeRayPick(_rayPickUID);
}

void LaserPointer::enable() {
    RayPickManager::getInstance().enableRayPick(_rayPickUID);
    _renderingEnabled = true;
}

void LaserPointer::disable() {
    RayPickManager::getInstance().disableRayPick(_rayPickUID);
    _renderingEnabled = false;
}

const RayPickResult& LaserPointer::getPrevRayPickResult() {
    return RayPickManager::getInstance().getPrevRayPickResult(_rayPickUID);
}

void LaserPointer::render(RenderArgs* args) {
    if (_renderingEnabled && !_currentRenderState.isEmpty() && _renderStates.contains(_currentRenderState)) {
        _renderStates[_currentRenderState].render(args);
    }
}

void RenderState::render(RenderArgs * args) {
    if (!_startID.isNull()) qApp->getOverlays().getOverlay(_startID)->render(args);
    if (!_pathID.isNull()) qApp->getOverlays().getOverlay(_pathID)->render(args);
    if (!_endID.isNull()) qApp->getOverlays().getOverlay(_endID)->render(args);
}
