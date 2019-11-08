//
//  Created by Sabrina Shanman 2018/07/16
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CollisionPick.h"

#include <QtCore/QDebug>

#include <glm/gtx/transform.hpp>

#include "PhysicsCollisionGroups.h"
#include "ScriptEngineLogging.h"
#include "UUIDHasher.h"

PickResultPointer CollisionPickResult::compareAndProcessNewResult(const PickResultPointer& newRes) {
    const std::shared_ptr<CollisionPickResult> newCollisionResult = std::static_pointer_cast<CollisionPickResult>(newRes);

    if (entityIntersections.size()) {
        entityIntersections.insert(entityIntersections.cend(), newCollisionResult->entityIntersections.begin(), newCollisionResult->entityIntersections.end());
    } else {
        entityIntersections = newCollisionResult->entityIntersections;
    }

    if (avatarIntersections.size()) {
        avatarIntersections.insert(avatarIntersections.cend(), newCollisionResult->avatarIntersections.begin(), newCollisionResult->avatarIntersections.end());
    } else {
        avatarIntersections = newCollisionResult->avatarIntersections;
    }

    intersects = entityIntersections.size() || avatarIntersections.size();

    return std::make_shared<CollisionPickResult>(*this);
}

void buildObjectIntersectionsMap(IntersectionType intersectionType, const std::vector<ContactTestResult>& objectIntersections, std::unordered_map<QUuid, QVariantMap>& intersections, std::unordered_map<QUuid, QVariantList>& collisionPointPairs) {
    for (auto& objectIntersection : objectIntersections) {
        auto at = intersections.find(objectIntersection.foundID);
        if (at == intersections.end()) {
            QVariantMap intersectingObject;
            intersectingObject["id"] = objectIntersection.foundID;
            intersectingObject["type"] = intersectionType;
            intersections[objectIntersection.foundID] = intersectingObject;

            collisionPointPairs[objectIntersection.foundID] = QVariantList();
        }

        QVariantMap collisionPointPair;
        collisionPointPair["pointOnPick"] = vec3toVariant(objectIntersection.testCollisionPoint);
        collisionPointPair["pointOnObject"] = vec3toVariant(objectIntersection.foundCollisionPoint);
        collisionPointPair["normalOnPick"] = vec3toVariant(objectIntersection.collisionNormal);

        collisionPointPairs[objectIntersection.foundID].append(collisionPointPair);
    }
}

/**jsdoc
 * An intersection result for a collision pick.
 *
 * @typedef {object} CollisionPickResult
 * @property {boolean} intersects - <code>true</code> if there is at least one intersection, <code>false</code> if there isn't.
 * @property {IntersectingObject[]} intersectingObjects - All objects which intersect with the <code>collisionRegion</code>.
 * @property {CollisionRegion} collisionRegion - The collision region that was used. Valid even if there was no intersection.
 */

/**jsdoc
 * Information about a {@link CollisionPick}'s intersection with an object.
 *
 * @typedef {object} IntersectingObject
 * @property {Uuid} id - The ID of the object.
 * @property {IntersectionType} type - The type of the object, either <code>1</code> for INTERSECTED_ENTITY or <code>3</code> 
 *     for INTERSECTED_AVATAR.
 * @property {CollisionContact[]} collisionContacts - Information on the penetration between the pick and the object.
 */

/**jsdoc
 * A pair of points that represents part of an overlap between a {@link CollisionPick} and an object in the physics engine. 
 * Points which are further apart represent deeper overlap.
 *
 * @typedef {object} CollisionContact
 * @property {Vec3} pointOnPick - A point representing a penetration of the object's surface into the volume of the pick, in 
 *     world coordinates.
 * @property {Vec3} pointOnObject - A point representing a penetration of the pick's surface into the volume of the object, in 
 *     world coordinates.
 * @property {Vec3} normalOnPick - The normal vector pointing away from the pick, representing the direction of collision.
 */

QVariantMap CollisionPickResult::toVariantMap() const {
    QVariantMap variantMap;

    variantMap["intersects"] = intersects;

    std::unordered_map<QUuid, QVariantMap> intersections;
    std::unordered_map<QUuid, QVariantList> collisionPointPairs;

    buildObjectIntersectionsMap(ENTITY, entityIntersections, intersections, collisionPointPairs);
    buildObjectIntersectionsMap(AVATAR, avatarIntersections, intersections, collisionPointPairs);

    QVariantList qIntersectingObjects;
    for (auto& intersectionKeyVal : intersections) {
        const QUuid& id = intersectionKeyVal.first;
        QVariantMap& intersection = intersectionKeyVal.second;

        intersection["collisionContacts"] = collisionPointPairs[id];
        qIntersectingObjects.append(intersection);
    }

    variantMap["intersectingObjects"] = qIntersectingObjects;
    variantMap["collisionRegion"] = pickVariant;

    return variantMap;
}

bool CollisionPick::isLoaded() const {
    return !_mathPick.shouldComputeShapeInfo() || (_cachedResource && _cachedResource->isLoaded());
}

bool CollisionPick::getShapeInfoReady(const CollisionRegion& pick) {
    if (_mathPick.shouldComputeShapeInfo()) {
        if (_cachedResource && _cachedResource->isLoaded()) {
            // TODO: Model CollisionPick support
            //computeShapeInfo(pick, *_mathPick.shapeInfo, _cachedResource);
            //_mathPick.loaded = true;
        } else {
            _mathPick.loaded = false;
        }
    } else {
        computeShapeInfoDimensionsOnly(pick, *_mathPick.shapeInfo, _cachedResource);
        _mathPick.loaded = true;
    }

    return _mathPick.loaded;
}

void CollisionPick::computeShapeInfoDimensionsOnly(const CollisionRegion& pick, ShapeInfo& shapeInfo, QSharedPointer<ModelResource> resource) {
    ShapeType type = shapeInfo.getType();
    glm::vec3 dimensions = pick.transform.getScale();
    QString modelURL = (resource ? resource->getURL().toString() : "");
    if (type == SHAPE_TYPE_COMPOUND) {
        shapeInfo.setParams(type, dimensions, modelURL);
    } else if (type >= SHAPE_TYPE_SIMPLE_HULL && type <= SHAPE_TYPE_STATIC_MESH) {
        shapeInfo.setParams(type, 0.5f * dimensions, modelURL);
    } else {
        shapeInfo.setParams(type, 0.5f * dimensions, modelURL);
    }
}

CollisionPick::CollisionPick(const PickFilter& filter, float maxDistance, bool enabled, bool scaleWithParent, CollisionRegion collisionRegion, PhysicsEnginePointer physicsEngine) :
    Pick(collisionRegion, filter, maxDistance, enabled),
    _scaleWithParent(scaleWithParent),
    _physicsEngine(physicsEngine) {
    if (collisionRegion.shouldComputeShapeInfo()) {
        _cachedResource = DependencyManager::get<ModelCache>()->getCollisionModelResource(collisionRegion.modelURL);
    }
    _mathPick.loaded = isLoaded();
}

CollisionRegion CollisionPick::getMathematicalPick() const {
    CollisionRegion mathPick = _mathPick;
    mathPick.loaded = isLoaded();
    if (parentTransform) {
        Transform parentTransformValue = parentTransform->getTransform();
        mathPick.transform = parentTransformValue.worldTransform(mathPick.transform);

        if (_scaleWithParent) {
            glm::vec3 scale = parentTransformValue.getScale();
            float largestDimension = glm::max(glm::max(scale.x, scale.y), scale.z);
            mathPick.threshold *= largestDimension;
        } else {
            // We need to undo parent scaling after-the-fact because the parent's scale was needed to calculate this mathPick's position
            mathPick.transform.setScale(_mathPick.transform.getScale());
        }
    }
    return mathPick;
}

void CollisionPick::filterIntersections(std::vector<ContactTestResult>& intersections) const {
    const QVector<QUuid>& ignoreItems = getIgnoreItems();
    const QVector<QUuid>& includeItems = getIncludeItems();
    bool isWhitelist = !includeItems.empty();

    if (!isWhitelist && ignoreItems.empty()) {
        return;
    }

    std::vector<ContactTestResult> filteredIntersections;

    int n = (int)intersections.size();
    for (int i = 0; i < n; i++) {
        auto& intersection = intersections[i];
        const QUuid& id = intersection.foundID;
        if (!ignoreItems.contains(id) && (!isWhitelist || includeItems.contains(id))) {
            filteredIntersections.push_back(intersection);
        }
    }

    intersections = filteredIntersections;
}

PickResultPointer CollisionPick::getEntityIntersection(const CollisionRegion& pick) {
    if (!pick.loaded) {
        // Cannot compute result
        return std::make_shared<CollisionPickResult>(pick.toVariantMap(), std::vector<ContactTestResult>(), std::vector<ContactTestResult>());
    }
    getShapeInfoReady(pick);
    
    auto entityIntersections = _physicsEngine->contactTest(USER_COLLISION_MASK_ENTITIES, *_mathPick.shapeInfo, pick.transform, pick.collisionGroup, pick.threshold);
    filterIntersections(entityIntersections);
    return std::make_shared<CollisionPickResult>(pick, entityIntersections, std::vector<ContactTestResult>());
}

PickResultPointer CollisionPick::getAvatarIntersection(const CollisionRegion& pick) {
    if (!pick.loaded) {
        // Cannot compute result
        return std::make_shared<CollisionPickResult>(pick, std::vector<ContactTestResult>(), std::vector<ContactTestResult>());
    }
    getShapeInfoReady(pick);
    
    auto avatarIntersections = _physicsEngine->contactTest(USER_COLLISION_MASK_AVATARS, *_mathPick.shapeInfo, pick.transform, pick.collisionGroup, pick.threshold);
    filterIntersections(avatarIntersections);
    return std::make_shared<CollisionPickResult>(pick, std::vector<ContactTestResult>(), avatarIntersections);
}

PickResultPointer CollisionPick::getHUDIntersection(const CollisionRegion& pick) {
    return std::make_shared<CollisionPickResult>(pick, std::vector<ContactTestResult>(), std::vector<ContactTestResult>());
}

Transform CollisionPick::getResultTransform() const {
    Transform transform;
    transform.setTranslation(_mathPick.transform.getTranslation());
    return transform;
}
