//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "RayPick.h"

#include "Application.h"
#include "avatar/AvatarManager.h"
#include "scripting/HMDScriptingInterface.h"
#include "DependencyManager.h"

RayToEntityIntersectionResult RayPick::getEntityIntersection(const PickRay& pick, bool precisionPicking,
                                                             const QVector<EntityItemID>& entitiesToInclude,
                                                             const QVector<EntityItemID>& entitiesToIgnore,
                                                             bool visibleOnly, bool collidableOnly) {
    return DependencyManager::get<EntityScriptingInterface>()->findRayIntersectionVector(pick, precisionPicking,
        entitiesToInclude, entitiesToIgnore, visibleOnly, collidableOnly);
}

RayToOverlayIntersectionResult RayPick::getOverlayIntersection(const PickRay& pick, bool precisionPicking,
                                                               const QVector<OverlayID>& overlaysToInclude,
                                                               const QVector<OverlayID>& overlaysToIgnore,
                                                               bool visibleOnly, bool collidableOnly) {
    return  qApp->getOverlays().findRayIntersectionVector(pick, precisionPicking,
        overlaysToInclude, overlaysToIgnore, visibleOnly, collidableOnly);
}

RayToAvatarIntersectionResult RayPick::getAvatarIntersection(const PickRay& pick,
                                                             const QVector<EntityItemID>& avatarsToInclude,
                                                             const QVector<EntityItemID>& avatarsToIgnore) {
    return DependencyManager::get<AvatarManager>()->findRayIntersectionVector(pick, avatarsToInclude, avatarsToIgnore);
}

glm::vec3 RayPick::getHUDIntersection(const PickRay& pick) {
    return DependencyManager::get<HMDScriptingInterface>()->calculateRayUICollisionPoint(pick.origin, pick.direction);
}