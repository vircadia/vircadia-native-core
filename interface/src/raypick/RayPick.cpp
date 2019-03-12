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
#include "avatar/AvatarManager.h"
#include "scripting/HMDScriptingInterface.h"
#include "DependencyManager.h"
#include "PickManager.h"

PickRay RayPick::getMathematicalPick() const {
    if (!parentTransform) {
        return _mathPick;
    }

    Transform currentParentTransform = parentTransform->getTransform();
    glm::vec3 origin = currentParentTransform.transform(_mathPick.origin);
    glm::vec3 direction = glm::normalize(currentParentTransform.transformDirection(_mathPick.direction));
    return PickRay(origin, direction);
}

PickResultPointer RayPick::getEntityIntersection(const PickRay& pick) {
    PickFilter searchFilter = getFilter();
    if (DependencyManager::get<PickManager>()->getForceCoarsePicking()) {
        searchFilter.setFlag(PickFilter::COARSE, true);
        searchFilter.setFlag(PickFilter::PRECISE, false);
    }

    RayToEntityIntersectionResult entityRes =
        DependencyManager::get<EntityScriptingInterface>()->evalRayIntersectionVector(pick, searchFilter,
            getIncludeItemsAs<EntityItemID>(), getIgnoreItemsAs<EntityItemID>());
    if (entityRes.intersects) {
        IntersectionType type = IntersectionType::ENTITY;
        if (getFilter().doesPickLocalEntities()) {
            EntityPropertyFlags desiredProperties;
            desiredProperties += PROP_ENTITY_HOST_TYPE;
            if (DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(entityRes.entityID, desiredProperties).getEntityHostType() == entity::HostType::LOCAL) {
                type = IntersectionType::LOCAL_ENTITY;
            }
        }
        return std::make_shared<RayPickResult>(type, entityRes.entityID, entityRes.distance, entityRes.intersection, pick, entityRes.surfaceNormal, entityRes.extraInfo);
    } else {
        return std::make_shared<RayPickResult>(pick.toVariantMap());
    }
}

PickResultPointer RayPick::getAvatarIntersection(const PickRay& pick) {
    bool precisionPicking = !(getFilter().isCoarse() || DependencyManager::get<PickManager>()->getForceCoarsePicking());
    RayToAvatarIntersectionResult avatarRes = DependencyManager::get<AvatarManager>()->findRayIntersectionVector(pick, getIncludeItemsAs<EntityItemID>(), getIgnoreItemsAs<EntityItemID>(), precisionPicking);
    if (avatarRes.intersects) {
        return std::make_shared<RayPickResult>(IntersectionType::AVATAR, avatarRes.avatarID, avatarRes.distance, avatarRes.intersection, pick, avatarRes.surfaceNormal, avatarRes.extraInfo);
    } else {
        return std::make_shared<RayPickResult>(pick.toVariantMap());
    }
}

PickResultPointer RayPick::getHUDIntersection(const PickRay& pick) {
    glm::vec3 hudRes = DependencyManager::get<HMDScriptingInterface>()->calculateRayUICollisionPoint(pick.origin, pick.direction);
    return std::make_shared<RayPickResult>(IntersectionType::HUD, QUuid(), glm::distance(pick.origin, hudRes), hudRes, pick);
}

Transform RayPick::getResultTransform() const {
    PickResultPointer result = getPrevPickResult();
    if (!result) {
        return Transform();
    }

    auto rayResult = std::static_pointer_cast<RayPickResult>(result);
    Transform transform;
    transform.setTranslation(rayResult->intersection);
    return transform;
}

glm::vec3 RayPick::intersectRayWithXYPlane(const glm::vec3& origin, const glm::vec3& direction, const glm::vec3& point, const glm::quat& rotation, const glm::vec3& registration) {
    // TODO: take into account registration
    glm::vec3 n = rotation * Vectors::FRONT;
    float t = glm::dot(n, point - origin) / glm::dot(n, direction);
    return origin + t * direction;
}

glm::vec3 RayPick::intersectRayWithEntityXYPlane(const QUuid& entityID, const glm::vec3& origin, const glm::vec3& direction) {
    auto props = DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(entityID);
    return intersectRayWithXYPlane(origin, direction, props.getPosition(), props.getRotation(), props.getRegistrationPoint());
}



glm::vec2 RayPick::projectOntoXYPlane(const glm::vec3& worldPos, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& dimensions, const glm::vec3& registrationPoint, bool unNormalized) {
    glm::quat invRot = glm::inverse(rotation);
    glm::vec3 localPos = invRot * (worldPos - position);

    glm::vec3 normalizedPos = (localPos / dimensions) + registrationPoint;

    glm::vec2 pos2D = glm::vec2(normalizedPos.x, (1.0f - normalizedPos.y));
    if (unNormalized) {
        pos2D *= glm::vec2(dimensions.x, dimensions.y);
    }
    return pos2D;
}

glm::vec2 RayPick::projectOntoXZPlane(const glm::vec3& worldPos, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& dimensions, const glm::vec3& registrationPoint, bool unNormalized) {
    glm::quat invRot = glm::inverse(rotation);
    glm::vec3 localPos = invRot * (worldPos - position);

    glm::vec3 normalizedPos = (localPos / dimensions) + registrationPoint;

    glm::vec2 pos2D = glm::vec2(normalizedPos.x, (1.0f - normalizedPos.z));
    if (unNormalized) {
        pos2D *= glm::vec2(dimensions.x, dimensions.z);
    }
    return pos2D;
}

glm::vec2 RayPick::projectOntoEntityXYPlane(const QUuid& entityID, const glm::vec3& worldPos, bool unNormalized) {
    EntityPropertyFlags desiredProperties;
    desiredProperties += PROP_POSITION;
    desiredProperties += PROP_ROTATION;
    desiredProperties += PROP_DIMENSIONS;
    desiredProperties += PROP_REGISTRATION_POINT;
    auto props = DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(entityID, desiredProperties);
    return projectOntoXYPlane(worldPos, props.getPosition(), props.getRotation(), props.getDimensions(), props.getRegistrationPoint(), unNormalized);
}
