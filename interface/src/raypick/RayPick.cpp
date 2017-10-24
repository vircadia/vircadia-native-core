//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "RayPick.h"

#include "Application.h"
#include "EntityScriptingInterface.h"
#include "ui/overlays/Overlays.h"
#include "avatar/AvatarManager.h"
#include "scripting/HMDScriptingInterface.h"
#include "DependencyManager.h"

PickResultPointer RayPick::getEntityIntersection(const PickRay& pick) {
    RayToEntityIntersectionResult entityRes =
        DependencyManager::get<EntityScriptingInterface>()->findRayIntersectionVector(pick, !getFilter().doesPickCoarse(),
            getIncludeItemsAs<EntityItemID>(), getIgnoreItemsAs<EntityItemID>(), !getFilter().doesPickInvisible(), !getFilter().doesPickNonCollidable());
    if (entityRes.intersects) {
        return std::make_shared<RayPickResult>(IntersectionType::ENTITY, entityRes.entityID, entityRes.distance, entityRes.intersection, pick, entityRes.surfaceNormal);
    } else {
        return std::make_shared<RayPickResult>(pick.toVariantMap());
    }
}

PickResultPointer RayPick::getOverlayIntersection(const PickRay& pick) {
    RayToOverlayIntersectionResult overlayRes =
        qApp->getOverlays().findRayIntersectionVector(pick, !getFilter().doesPickCoarse(),
            getIncludeItemsAs<OverlayID>(), getIgnoreItemsAs<OverlayID>(), !getFilter().doesPickInvisible(), !getFilter().doesPickNonCollidable());
    if (overlayRes.intersects) {
        return std::make_shared<RayPickResult>(IntersectionType::OVERLAY, overlayRes.overlayID, overlayRes.distance, overlayRes.intersection, pick, overlayRes.surfaceNormal);
    } else {
        return std::make_shared<RayPickResult>(pick.toVariantMap());
    }
}

PickResultPointer RayPick::getAvatarIntersection(const PickRay& pick) {
    RayToAvatarIntersectionResult avatarRes = DependencyManager::get<AvatarManager>()->findRayIntersectionVector(pick, getIncludeItemsAs<EntityItemID>(), getIgnoreItemsAs<EntityItemID>());
    if (avatarRes.intersects) {
        return std::make_shared<RayPickResult>(IntersectionType::AVATAR, avatarRes.avatarID, avatarRes.distance, avatarRes.intersection, pick);
    } else {
        return std::make_shared<RayPickResult>(pick.toVariantMap());
    }
}

PickResultPointer RayPick::getHUDIntersection(const PickRay& pick) {
    glm::vec3 hudRes = DependencyManager::get<HMDScriptingInterface>()->calculateRayUICollisionPoint(pick.origin, pick.direction);
    return std::make_shared<RayPickResult>(IntersectionType::HUD, QUuid(), glm::distance(pick.origin, hudRes), hudRes, pick);
}