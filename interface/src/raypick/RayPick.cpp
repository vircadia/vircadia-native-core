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
#include "PickManager.h"

PickResultPointer RayPick::getEntityIntersection(const PickRay& pick) {
    bool precisionPicking = !(getFilter().doesPickCoarse() || DependencyManager::get<PickManager>()->getForceCoarsePicking());
    RayToEntityIntersectionResult entityRes =
        DependencyManager::get<EntityScriptingInterface>()->findRayIntersectionVector(pick, precisionPicking,
            getIncludeItemsAs<EntityItemID>(), getIgnoreItemsAs<EntityItemID>(), !getFilter().doesPickInvisible(), !getFilter().doesPickNonCollidable());
    if (entityRes.intersects) {
        return std::make_shared<RayPickResult>(IntersectionType::ENTITY, entityRes.entityID, entityRes.distance, entityRes.intersection, pick, entityRes.surfaceNormal, entityRes.extraInfo);
    } else {
        return std::make_shared<RayPickResult>(pick.toVariantMap());
    }
}

PickResultPointer RayPick::getOverlayIntersection(const PickRay& pick) {
    bool precisionPicking = !(getFilter().doesPickCoarse() || DependencyManager::get<PickManager>()->getForceCoarsePicking());
    RayToOverlayIntersectionResult overlayRes =
        qApp->getOverlays().findRayIntersectionVector(pick, precisionPicking,
            getIncludeItemsAs<OverlayID>(), getIgnoreItemsAs<OverlayID>(), !getFilter().doesPickInvisible(), !getFilter().doesPickNonCollidable());
    if (overlayRes.intersects) {
        return std::make_shared<RayPickResult>(IntersectionType::OVERLAY, overlayRes.overlayID, overlayRes.distance, overlayRes.intersection, pick, overlayRes.surfaceNormal, overlayRes.extraInfo);
    } else {
        return std::make_shared<RayPickResult>(pick.toVariantMap());
    }
}

PickResultPointer RayPick::getAvatarIntersection(const PickRay& pick) {
    RayToAvatarIntersectionResult avatarRes = DependencyManager::get<AvatarManager>()->findRayIntersectionVector(pick, getIncludeItemsAs<EntityItemID>(), getIgnoreItemsAs<EntityItemID>());
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

glm::vec3 RayPick::intersectRayWithOverlayXYPlane(const QUuid& overlayID, const glm::vec3& origin, const glm::vec3& direction) {
    glm::vec3 position = vec3FromVariant(qApp->getOverlays().getProperty(overlayID, "position").value);
    glm::quat rotation = quatFromVariant(qApp->getOverlays().getProperty(overlayID, "rotation").value);
    return intersectRayWithXYPlane(origin, direction, position, rotation, ENTITY_ITEM_DEFAULT_REGISTRATION_POINT);
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

glm::vec2 RayPick::projectOntoOverlayXYPlane(const QUuid& overlayID, const glm::vec3& worldPos, bool unNormalized) {
    glm::vec3 position = vec3FromVariant(qApp->getOverlays().getProperty(overlayID, "position").value);
    glm::quat rotation = quatFromVariant(qApp->getOverlays().getProperty(overlayID, "rotation").value);
    glm::vec3 dimensions = glm::vec3(vec2FromVariant(qApp->getOverlays().getProperty(overlayID, "dimensions").value), 0.01f);

    return projectOntoXYPlane(worldPos, position, rotation, dimensions, ENTITY_ITEM_DEFAULT_REGISTRATION_POINT, unNormalized);
}

glm::vec2 RayPick::projectOntoEntityXYPlane(const QUuid& entityID, const glm::vec3& worldPos, bool unNormalized) {
    auto props = DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(entityID);
    return projectOntoXYPlane(worldPos, props.getPosition(), props.getRotation(), props.getDimensions(), props.getRegistrationPoint(), unNormalized);
}