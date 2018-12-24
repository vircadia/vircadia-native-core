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

ParabolaPick::ParabolaPick(const glm::vec3& position, const glm::vec3& direction, float speed, const glm::vec3& accelerationAxis, bool rotateAccelerationWithAvatar, bool rotateAccelerationWithParent, bool scaleWithParent, const PickFilter& filter, float maxDistance, bool enabled) :
    Pick(PickParabola(position, speed * direction, accelerationAxis), filter, maxDistance, enabled),
    _rotateAccelerationWithAvatar(rotateAccelerationWithAvatar),
    _rotateAccelerationWithParent(rotateAccelerationWithParent),
    _scaleWithParent(scaleWithParent),
    _speed(speed) {
}

PickParabola ParabolaPick::getMathematicalPick() const {
    if (!parentTransform) {
        PickParabola mathPick = _mathPick;
        if (_rotateAccelerationWithAvatar) {
            mathPick.acceleration = DependencyManager::get<AvatarManager>()->getMyAvatar()->getWorldOrientation() * mathPick.acceleration;
        }
        return mathPick;
    }

    Transform currentParentTransform = parentTransform->getTransform();

    glm::vec3 position = currentParentTransform.transform(_mathPick.origin);
    glm::vec3 velocity = _mathPick.velocity;
    if (_scaleWithParent) {
        velocity = currentParentTransform.transformDirection(velocity);
    } else {
        glm::vec3 transformedVelocity = currentParentTransform.transformDirection(velocity);
        velocity = glm::normalize(transformedVelocity) * _speed;
    }
    glm::vec3 acceleration = _mathPick.acceleration;
    if (_scaleWithParent) {
        acceleration *= currentParentTransform.getScale();
    }
    if (_rotateAccelerationWithAvatar) {
        acceleration = DependencyManager::get<AvatarManager>()->getMyAvatar()->getWorldOrientation() * acceleration;
    } else if (_rotateAccelerationWithParent) {
        acceleration = currentParentTransform.getRotation() * acceleration;
    }

    return PickParabola(position, velocity, acceleration);
}

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