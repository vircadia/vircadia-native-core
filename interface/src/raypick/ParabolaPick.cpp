//
//  Created by Sam Gondelman 7/2/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "ParabolaPick.h"

#include "Application.h"
#include "EntityScriptingInterface.h"
#include "ui/overlays/Overlays.h"
#include "avatar/AvatarManager.h"
#include "scripting/HMDScriptingInterface.h"
#include "DependencyManager.h"
#include "PickManager.h"

PickResultPointer ParabolaPick::getEntityIntersection(const PickParabola& pick) {
    if (glm::length2(pick.acceleration) > EPSILON && glm::length2(pick.velocity) > EPSILON) {
        bool precisionPicking = !(getFilter().doesPickCoarse() || DependencyManager::get<PickManager>()->getForceCoarsePicking());
        ParabolaToEntityIntersectionResult entityRes =
            DependencyManager::get<EntityScriptingInterface>()->findParabolaIntersectionVector(pick, precisionPicking,
                getIncludeItemsAs<EntityItemID>(), getIgnoreItemsAs<EntityItemID>(), !getFilter().doesPickInvisible(), !getFilter().doesPickNonCollidable());
        if (entityRes.intersects) {
            return std::make_shared<ParabolaPickResult>(IntersectionType::ENTITY, entityRes.entityID, entityRes.distance, entityRes.parabolicDistance, entityRes.intersection, pick, entityRes.surfaceNormal, entityRes.extraInfo);
        }
    }
    return std::make_shared<ParabolaPickResult>(pick.toVariantMap());
}

PickResultPointer ParabolaPick::getOverlayIntersection(const PickParabola& pick) {
    if (glm::length2(pick.acceleration) > EPSILON && glm::length2(pick.velocity) > EPSILON) {
        bool precisionPicking = !(getFilter().doesPickCoarse() || DependencyManager::get<PickManager>()->getForceCoarsePicking());
        ParabolaToOverlayIntersectionResult overlayRes =
            qApp->getOverlays().findParabolaIntersectionVector(pick, precisionPicking,
                getIncludeItemsAs<OverlayID>(), getIgnoreItemsAs<OverlayID>(), !getFilter().doesPickInvisible(), !getFilter().doesPickNonCollidable());
        if (overlayRes.intersects) {
            return std::make_shared<ParabolaPickResult>(IntersectionType::OVERLAY, overlayRes.overlayID, overlayRes.distance, overlayRes.parabolicDistance, overlayRes.intersection, pick, overlayRes.surfaceNormal, overlayRes.extraInfo);
        }
    }
    return std::make_shared<ParabolaPickResult>(pick.toVariantMap());
}

PickResultPointer ParabolaPick::getAvatarIntersection(const PickParabola& pick) {
    if (glm::length2(pick.acceleration) > EPSILON && glm::length2(pick.velocity) > EPSILON) {
        ParabolaToAvatarIntersectionResult avatarRes = DependencyManager::get<AvatarManager>()->findParabolaIntersectionVector(pick, getIncludeItemsAs<EntityItemID>(), getIgnoreItemsAs<EntityItemID>());
        if (avatarRes.intersects) {
            return std::make_shared<ParabolaPickResult>(IntersectionType::AVATAR, avatarRes.avatarID, avatarRes.distance, avatarRes.parabolicDistance, avatarRes.intersection, pick, avatarRes.surfaceNormal, avatarRes.extraInfo);
        }
    }
    return std::make_shared<ParabolaPickResult>(pick.toVariantMap());
}

PickResultPointer ParabolaPick::getHUDIntersection(const PickParabola& pick) {
    if (glm::length2(pick.acceleration) > EPSILON && glm::length2(pick.velocity) > EPSILON) {
        float parabolicDistance;
        glm::vec3 hudRes = DependencyManager::get<HMDScriptingInterface>()->calculateParabolaUICollisionPoint(pick.origin, pick.velocity, pick.acceleration, parabolicDistance);
        return std::make_shared<ParabolaPickResult>(IntersectionType::HUD, QUuid(), glm::distance(pick.origin, hudRes), parabolicDistance, hudRes, pick);
    }
    return std::make_shared<ParabolaPickResult>(pick.toVariantMap());
}

float ParabolaPick::getSpeed() const {
    return (_scaleWithAvatar ? DependencyManager::get<AvatarManager>()->getMyAvatar()->getSensorToWorldScale() * _speed : _speed);
}

glm::vec3 ParabolaPick::getAcceleration() const {
    float scale = (_scaleWithAvatar ? DependencyManager::get<AvatarManager>()->getMyAvatar()->getSensorToWorldScale() : 1.0f);
    if (_rotateAccelerationWithAvatar) {
        return scale * (DependencyManager::get<AvatarManager>()->getMyAvatar()->getWorldOrientation() * _accelerationAxis);
    }
    return scale * _accelerationAxis;
}

Transform ParabolaPick::getResultTransform() const {
    PickResultPointer result = getPrevPickResult();
    if (!result) {
        return Transform();
    }

    auto parabolaResult = std::static_pointer_cast<ParabolaPickResult>(result);
    Transform transform;
    transform.setTranslation(parabolaResult->intersection);
    return transform;
}