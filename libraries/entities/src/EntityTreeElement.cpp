//
//  EntityTreeElement.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntityTreeElement.h"

#include <glm/gtx/transform.hpp>

#include <GeometryUtil.h>
#include <OctreeUtils.h>
#include <Extents.h>

#include "EntitiesLogging.h"
#include "EntityNodeData.h"
#include "EntityItemProperties.h"
#include "EntityTree.h"
#include "EntityTypes.h"

EntityTreeElement::EntityTreeElement(unsigned char* octalCode) : OctreeElement() {
    init(octalCode);
};

EntityTreeElement::~EntityTreeElement() {
    _octreeMemoryUsage -= sizeof(EntityTreeElement);
}

OctreeElementPointer EntityTreeElement::createNewElement(unsigned char* octalCode) {
    auto newChild = EntityTreeElementPointer(new EntityTreeElement(octalCode));
    newChild->setTree(_myTree);
    return newChild;
}

void EntityTreeElement::init(unsigned char* octalCode) {
    OctreeElement::init(octalCode);
    _octreeMemoryUsage += sizeof(EntityTreeElement);
}

OctreeElementPointer EntityTreeElement::addChildAtIndex(int index) {
    OctreeElementPointer newElement = OctreeElement::addChildAtIndex(index);
    std::static_pointer_cast<EntityTreeElement>(newElement)->setTree(_myTree);
    return newElement;
}

void EntityTreeElement::debugExtraEncodeData(EncodeBitstreamParams& params) const {
    qCDebug(entities) << "EntityTreeElement::debugExtraEncodeData()... ";
    qCDebug(entities) << "    element:" << _cube;

    auto entityNodeData = static_cast<EntityNodeData*>(params.nodeData);
    assert(entityNodeData);

    OctreeElementExtraEncodeData* extraEncodeData = &entityNodeData->extraEncodeData;
    assert(extraEncodeData); // EntityTrees always require extra encode data on their encoding passes

    if (extraEncodeData->contains(this)) {
        EntityTreeElementExtraEncodeDataPointer entityTreeElementExtraEncodeData
            = std::static_pointer_cast<EntityTreeElementExtraEncodeData>((*extraEncodeData)[this]);
        qCDebug(entities) << "    encode data:" << &(*entityTreeElementExtraEncodeData);
    } else {
        qCDebug(entities) << "    encode data: MISSING!!";
    }
}

bool EntityTreeElement::containsEntityBounds(EntityItemPointer entity) const {
    bool success;
    auto queryCube = entity->getQueryAACube(success);
    if (!success) {
        return false;
    }
    return containsBounds(queryCube);
}

bool EntityTreeElement::bestFitEntityBounds(EntityItemPointer entity) const {
    bool success;
    auto queryCube = entity->getQueryAACube(success);
    if (!success) {
        qCDebug(entities) << "EntityTreeElement::bestFitEntityBounds couldn't get queryCube for" << entity->getName() << entity->getID();
        return false;
    }
    return bestFitBounds(queryCube);
}

bool EntityTreeElement::containsBounds(const EntityItemProperties& properties) const {
    return containsBounds(properties.getQueryAACube());
}

bool EntityTreeElement::bestFitBounds(const EntityItemProperties& properties) const {
    return bestFitBounds(properties.getQueryAACube());
}

bool EntityTreeElement::containsBounds(const AACube& bounds) const {
    return containsBounds(bounds.getMinimumPoint(), bounds.getMaximumPoint());
}

bool EntityTreeElement::bestFitBounds(const AACube& bounds) const {
    return bestFitBounds(bounds.getMinimumPoint(), bounds.getMaximumPoint());
}

bool EntityTreeElement::containsBounds(const AABox& bounds) const {
    return containsBounds(bounds.getMinimumPoint(), bounds.getMaximumPoint());
}

bool EntityTreeElement::bestFitBounds(const AABox& bounds) const {
    return bestFitBounds(bounds.getMinimumPoint(), bounds.getMaximumPoint());
}

bool EntityTreeElement::containsBounds(const glm::vec3& minPoint, const glm::vec3& maxPoint) const {
    glm::vec3 clampedMin = glm::clamp(minPoint, (float)-HALF_TREE_SCALE, (float)HALF_TREE_SCALE);
    glm::vec3 clampedMax = glm::clamp(maxPoint, (float)-HALF_TREE_SCALE, (float)HALF_TREE_SCALE);
    return _cube.contains(clampedMin) && _cube.contains(clampedMax);
}

bool EntityTreeElement::bestFitBounds(const glm::vec3& minPoint, const glm::vec3& maxPoint) const {
    glm::vec3 clampedMin = glm::clamp(minPoint, (float)-HALF_TREE_SCALE, (float)HALF_TREE_SCALE);
    glm::vec3 clampedMax = glm::clamp(maxPoint, (float)-HALF_TREE_SCALE, (float)HALF_TREE_SCALE);

    if (_cube.contains(clampedMin) && _cube.contains(clampedMax)) {

        // If our child would be smaller than our smallest reasonable element, then we are the best fit.
        float childScale = _cube.getScale() / 2.0f;
        if (childScale <= SMALLEST_REASONABLE_OCTREE_ELEMENT_SCALE) {
            return true;
        }
        int childForMinimumPoint = getMyChildContainingPoint(clampedMin);
        int childForMaximumPoint = getMyChildContainingPoint(clampedMax);

        // If I contain both the minimum and maximum point, but two different children of mine
        // contain those points, then I am the best fit for that entity
        if (childForMinimumPoint != childForMaximumPoint) {
            return true;
        }
    }
    return false;
}

bool EntityTreeElement::checkFilterSettings(const EntityItemPointer& entity, PickFilter searchFilter) {
    bool visible = entity->isVisible();
    entity::HostType hostType = entity->getEntityHostType();
    if ((!searchFilter.doesPickVisible() && visible) || (!searchFilter.doesPickInvisible() && !visible) ||
        (!searchFilter.doesPickDomainEntities() && hostType == entity::HostType::DOMAIN) ||
        (!searchFilter.doesPickAvatarEntities() && hostType == entity::HostType::AVATAR) ||
        (!searchFilter.doesPickLocalEntities() && hostType == entity::HostType::LOCAL)) {
        return false;
    }
    // We only check the collidable filters for non-local entities, because local entities are always collisionless,
    // but picks always include COLLIDABLE (see PickScriptingInterface::getPickFilter()), so if we were to respect
    // the getCollisionless() property of Local entities then we would *never* intersect them in a pick.
    // An unfortunate side effect of the following code is that Local entities are intersected even if the
    // pick explicitly requested only COLLIDABLE entities (but, again, Local entities are always collisionless).
    if (hostType != entity::HostType::LOCAL) {
        bool collidable = !entity->getCollisionless() && (entity->getShapeType() != SHAPE_TYPE_NONE);
        if ((collidable && !searchFilter.doesPickCollidable()) || (!collidable && !searchFilter.doesPickNonCollidable())) {
            return false;
        }
    }
    return true;
}

EntityItemID EntityTreeElement::evalRayIntersection(const glm::vec3& origin, const glm::vec3& direction, const glm::vec3& viewFrustumPos,
        OctreeElementPointer& element, float& distance, BoxFace& face, glm::vec3& surfaceNormal,
        const QVector<EntityItemID>& entityIdsToInclude, const QVector<EntityItemID>& entityIdsToDiscard,
        PickFilter searchFilter, QVariantMap& extraInfo) {

    EntityItemID result;
    BoxFace localFace { UNKNOWN_FACE };
    glm::vec3 localSurfaceNormal;

    if (!canPickIntersect()) {
        return result;
    }

    QVariantMap localExtraInfo;
    float distanceToElementDetails = distance;
    EntityItemID entityID = evalDetailedRayIntersection(origin, direction, viewFrustumPos, element, distanceToElementDetails,
            localFace, localSurfaceNormal, entityIdsToInclude, entityIdsToDiscard, searchFilter, localExtraInfo);
    if (!entityID.isNull() && distanceToElementDetails < distance) {
        distance = distanceToElementDetails;
        face = localFace;
        surfaceNormal = localSurfaceNormal;
        extraInfo = localExtraInfo;
        result = entityID;
    }
    return result;
}

EntityItemID EntityTreeElement::evalDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction, const glm::vec3& viewFrustumPos,
                                    OctreeElementPointer& element, float& distance, BoxFace& face, glm::vec3& surfaceNormal,
                                    const QVector<EntityItemID>& entityIdsToInclude, const QVector<EntityItemID>& entityIDsToDiscard,
                                    PickFilter searchFilter, QVariantMap& extraInfo) {

    // only called if we do intersect our bounding cube, but find if we actually intersect with entities...
    EntityItemID entityID;
    forEachEntity([&](EntityItemPointer entity) {
        if (entity->getIgnorePickIntersection() && !searchFilter.bypassIgnore()) {
            return;
        }

        // use simple line-sphere for broadphase check
        // (this is faster and more likely to cull results than the filter check below so we do it first)
        bool success;
        AABox entityBox = entity->getAABox(success);
        if (!success || !entityBox.rayHitsBoundingSphere(origin, direction)) {
            return;
        }

        if (!checkFilterSettings(entity, searchFilter) ||
            (entityIdsToInclude.size() > 0 && !entityIdsToInclude.contains(entity->getID())) ||
            (entityIDsToDiscard.size() > 0 && entityIDsToDiscard.contains(entity->getID())) ) {
            return;
        }

        // extents is the entity relative, scaled, centered extents of the entity
        glm::vec3 position = entity->getWorldPosition();
        glm::mat4 translation = glm::translate(position);
        BillboardMode billboardMode = entity->getBillboardMode();
        glm::quat orientation = billboardMode == BillboardMode::NONE ? entity->getWorldOrientation() : entity->getLocalOrientation();
        glm::mat4 rotation = glm::mat4_cast(BillboardModeHelpers::getBillboardRotation(position, orientation, billboardMode,
            viewFrustumPos, entity->getRotateForPicking()));
        glm::mat4 entityToWorldMatrix = translation * rotation;
        glm::mat4 worldToEntityMatrix = glm::inverse(entityToWorldMatrix);

        glm::vec3 dimensions = entity->getScaledDimensions();
        glm::vec3 registrationPoint = entity->getRegistrationPoint();
        glm::vec3 corner = -(dimensions * registrationPoint) + entity->getPivot();

        AABox entityFrameBox(corner, dimensions);

        glm::vec3 entityFrameOrigin = glm::vec3(worldToEntityMatrix * glm::vec4(origin, 1.0f));
        glm::vec3 entityFrameDirection = glm::vec3(worldToEntityMatrix * glm::vec4(direction, 0.0f));

        // we can use the AABox's ray intersection by mapping our origin and direction into the entity frame
        // and testing intersection there.
        float localDistance;
        BoxFace localFace { UNKNOWN_FACE };
        glm::vec3 localSurfaceNormal;
        if (entityFrameBox.findRayIntersection(entityFrameOrigin, entityFrameDirection, 1.0f / entityFrameDirection, localDistance,
                                                localFace, localSurfaceNormal)) {
            if (entityFrameBox.contains(entityFrameOrigin) || localDistance < distance) {
                // now ask the entity if we actually intersect
                if (entity->supportsDetailedIntersection()) {
                    QVariantMap localExtraInfo;
                    if (entity->findDetailedRayIntersection(origin, direction, viewFrustumPos, element, localDistance,
                            localFace, localSurfaceNormal, localExtraInfo, searchFilter.isPrecise())) {
                        if (localDistance < distance) {
                            distance = localDistance;
                            face = localFace;
                            surfaceNormal = localSurfaceNormal;
                            extraInfo = localExtraInfo;
                            entityID = entity->getEntityItemID();
                        }
                    }
                } else {
                    // if the entity type doesn't support a detailed intersection, then just return the non-AABox results
                    // Never intersect with particle entities
                    if (localDistance < distance && entity->getType() != EntityTypes::ParticleEffect) {
                        distance = localDistance;
                        face = localFace;
                        surfaceNormal = glm::vec3(rotation * glm::vec4(localSurfaceNormal, 0.0f));
                        extraInfo = QVariantMap();
                        entityID = entity->getEntityItemID();
                    }
                }
            }
        }
    });
    return entityID;
}

// TODO: change this to use better bounding shape for entity than sphere
bool EntityTreeElement::findSpherePenetration(const glm::vec3& center, float radius,
                                    glm::vec3& penetration, void** penetratedObject) const {
    bool result = false;
    withReadLock([&] {
        foreach(EntityItemPointer entity, _entityItems) {
            bool success;
            glm::vec3 entityCenter = entity->getCenterPosition(success);
            float entityRadius = entity->getRadius();

            // don't penetrate yourself
            if (!success || (entityCenter == center && entityRadius == radius)) {
                return;
            }

            if (findSphereSpherePenetration(center, radius, entityCenter, entityRadius, penetration)) {
                // return true on first valid entity penetration
                *penetratedObject = (void*)(entity.get());

                result = true;
                return;
            }
        }
    });
    return result;
}

EntityItemID EntityTreeElement::evalParabolaIntersection(const glm::vec3& origin, const glm::vec3& velocity,
    const glm::vec3& acceleration, const glm::vec3& viewFrustumPos, OctreeElementPointer& element, float& parabolicDistance,
    BoxFace& face, glm::vec3& surfaceNormal, const QVector<EntityItemID>& entityIdsToInclude,
    const QVector<EntityItemID>& entityIdsToDiscard, PickFilter searchFilter, QVariantMap& extraInfo) {

    EntityItemID result;
    BoxFace localFace;
    glm::vec3 localSurfaceNormal;

    if (!canPickIntersect()) {
        return result;
    }

    QVariantMap localExtraInfo;
    float distanceToElementDetails = parabolicDistance;
    // We can precompute the world-space parabola normal and reuse it for the parabola plane intersects AABox sphere check
    glm::vec3 vectorOnPlane = velocity;
    if (glm::dot(glm::normalize(velocity), glm::normalize(acceleration)) > 1.0f - EPSILON) {
        // Handle the degenerate case where velocity is parallel to acceleration
        // We pick t = 1 and calculate a second point on the plane
        vectorOnPlane = velocity + 0.5f * acceleration;
    }
    // Get the normal of the plane, the cross product of two vectors on the plane
    glm::vec3 normal = glm::normalize(glm::cross(vectorOnPlane, acceleration));
    EntityItemID entityID = evalDetailedParabolaIntersection(origin, velocity, acceleration, viewFrustumPos, normal, element, distanceToElementDetails,
            localFace, localSurfaceNormal, entityIdsToInclude, entityIdsToDiscard, searchFilter, localExtraInfo);
    if (!entityID.isNull() && distanceToElementDetails < parabolicDistance) {
        parabolicDistance = distanceToElementDetails;
        face = localFace;
        surfaceNormal = localSurfaceNormal;
        extraInfo = localExtraInfo;
        result = entityID;
    }
    return result;
}

EntityItemID EntityTreeElement::evalDetailedParabolaIntersection(const glm::vec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration,
                                    const glm::vec3& viewFrustumPos,const glm::vec3& normal, OctreeElementPointer& element, float& parabolicDistance,
                                    BoxFace& face, glm::vec3& surfaceNormal, const QVector<EntityItemID>& entityIdsToInclude,
                                    const QVector<EntityItemID>& entityIDsToDiscard, PickFilter searchFilter, QVariantMap& extraInfo) {

    // only called if we do intersect our bounding cube, but find if we actually intersect with entities...
    EntityItemID entityID;
    forEachEntity([&](EntityItemPointer entity) {
        if (entity->getIgnorePickIntersection() && !searchFilter.bypassIgnore()) {
            return;
        }

        // use simple line-sphere for broadphase check
        // (this is faster and more likely to cull results than the filter check below so we do it first)
        bool success;
        AABox entityBox = entity->getAABox(success);

        // Instead of checking parabolaInstersectsBoundingSphere here, we are just going to check if the plane
        // defined by the parabola slices the sphere.  The solution to parabolaIntersectsBoundingSphere is cubic,
        // the solution to which is more computationally expensive than the quadratic AABox::findParabolaIntersection
        // below
        if (!success || !entityBox.parabolaPlaneIntersectsBoundingSphere(origin, velocity, acceleration, normal)) {
            return;
        }

        if (!checkFilterSettings(entity, searchFilter) ||
            (entityIdsToInclude.size() > 0 && !entityIdsToInclude.contains(entity->getID())) ||
            (entityIDsToDiscard.size() > 0 && entityIDsToDiscard.contains(entity->getID()))) {
            return;
        }

        // extents is the entity relative, scaled, centered extents of the entity
        glm::vec3 position = entity->getWorldPosition();
        glm::mat4 translation = glm::translate(position);
        BillboardMode billboardMode = entity->getBillboardMode();
        glm::quat orientation = billboardMode == BillboardMode::NONE ? entity->getWorldOrientation() : entity->getLocalOrientation();
        glm::mat4 rotation = glm::mat4_cast(BillboardModeHelpers::getBillboardRotation(position, orientation, billboardMode,
            viewFrustumPos, entity->getRotateForPicking()));
        glm::mat4 entityToWorldMatrix = translation * rotation;
        glm::mat4 worldToEntityMatrix = glm::inverse(entityToWorldMatrix);

        glm::vec3 dimensions = entity->getScaledDimensions();
        glm::vec3 registrationPoint = entity->getRegistrationPoint();
        glm::vec3 corner = -(dimensions * registrationPoint) + entity->getPivot();

        AABox entityFrameBox(corner, dimensions);

        glm::vec3 entityFrameOrigin = glm::vec3(worldToEntityMatrix * glm::vec4(origin, 1.0f));
        glm::vec3 entityFrameVelocity = glm::vec3(worldToEntityMatrix * glm::vec4(velocity, 0.0f));
        glm::vec3 entityFrameAcceleration = glm::vec3(worldToEntityMatrix * glm::vec4(acceleration, 0.0f));

        // we can use the AABox's ray intersection by mapping our origin and direction into the entity frame
        // and testing intersection there.
        float localDistance;
        BoxFace localFace;
        glm::vec3 localSurfaceNormal;
        if (entityFrameBox.findParabolaIntersection(entityFrameOrigin, entityFrameVelocity, entityFrameAcceleration, localDistance,
                                                localFace, localSurfaceNormal)) {
            if (entityFrameBox.contains(entityFrameOrigin) || localDistance < parabolicDistance) {
                // now ask the entity if we actually intersect
                if (entity->supportsDetailedIntersection()) {
                    QVariantMap localExtraInfo;
                    if (entity->findDetailedParabolaIntersection(origin, velocity, acceleration, viewFrustumPos, element, localDistance,
                            localFace, localSurfaceNormal, localExtraInfo, searchFilter.isPrecise())) {
                        if (localDistance < parabolicDistance) {
                            parabolicDistance = localDistance;
                            face = localFace;
                            surfaceNormal = localSurfaceNormal;
                            extraInfo = localExtraInfo;
                            entityID = entity->getEntityItemID();
                        }
                    }
                } else {
                    // if the entity type doesn't support a detailed intersection, then just return the non-AABox results
                    // Never intersect with particle entities
                    if (localDistance < parabolicDistance && entity->getType() != EntityTypes::ParticleEffect) {
                        parabolicDistance = localDistance;
                        face = localFace;
                        surfaceNormal = glm::vec3(rotation * glm::vec4(localSurfaceNormal, 0.0f));
                        extraInfo = QVariantMap();
                        entityID = entity->getEntityItemID();
                    }
                }
            }
        }
    });
    return entityID;
}

QUuid EntityTreeElement::evalClosetEntity(const glm::vec3& position, PickFilter searchFilter, float& closestDistanceSquared) const {
    QUuid closestEntity;
    forEachEntity([&](EntityItemPointer entity) {
        if (!checkFilterSettings(entity, searchFilter)) {
            return;
        }

        float distanceToEntity = glm::distance2(position, entity->getWorldPosition());
        if (distanceToEntity < closestDistanceSquared) {
            closestEntity = entity->getID();
            closestDistanceSquared = distanceToEntity;
        }
    });
    return closestEntity;
}

void EntityTreeElement::evalEntitiesInSphere(const glm::vec3& position, float radius, PickFilter searchFilter, QVector<QUuid>& foundEntities) const {
    forEachEntity([&](EntityItemPointer entity) {
        if (!checkFilterSettings(entity, searchFilter)) {
            return;
        }

        bool success;
        AABox entityBox = entity->getAABox(success);
        // if the sphere doesn't intersect with our world frame AABox, we don't need to consider the more complex case
        glm::vec3 penetration;
        if (success && entityBox.findSpherePenetration(position, radius, penetration)) {

            glm::vec3 dimensions = entity->getScaledDimensions();

            // FIXME - consider allowing the entity to determine penetration so that
            //         entities could presumably do actual hull testing if they wanted to
            // FIXME - handle entity->getShapeType() == SHAPE_TYPE_SPHERE case better in particular
            //         can we handle the ellipsoid case better? We only currently handle perfect spheres
            //         with centered registration points
            if (entity->getShapeType() == SHAPE_TYPE_SPHERE && (dimensions.x == dimensions.y && dimensions.y == dimensions.z)) {

                // NOTE: entity->getRadius() doesn't return the true radius, it returns the radius of the
                //       maximum bounding sphere, which is actually larger than our actual radius
                float entityTrueRadius = dimensions.x / 2.0f;

                bool success;
                glm::vec3 center = entity->getCenterPosition(success);
                if (success && findSphereSpherePenetration(position, radius, center, entityTrueRadius, penetration)) {
                    foundEntities.push_back(entity->getID());
                }
            } else {
                // determine the worldToEntityMatrix that doesn't include scale because
                // we're going to use the registration aware aa box in the entity frame
                glm::mat4 translation = glm::translate(entity->getWorldPosition());
                glm::mat4 rotation = glm::mat4_cast(entity->getWorldOrientation());
                glm::mat4 entityToWorldMatrix = translation * rotation;
                glm::mat4 worldToEntityMatrix = glm::inverse(entityToWorldMatrix);

                glm::vec3 registrationPoint = entity->getRegistrationPoint();
                glm::vec3 corner = -(dimensions * registrationPoint) + entity->getPivot();

                AABox entityFrameBox(corner, dimensions);

                glm::vec3 entityFrameSearchPosition = glm::vec3(worldToEntityMatrix * glm::vec4(position, 1.0f));
                if (entityFrameBox.findSpherePenetration(entityFrameSearchPosition, radius, penetration)) {
                    foundEntities.push_back(entity->getID());
                }
            }
        }
    });
}

void EntityTreeElement::evalEntitiesInSphereWithType(const glm::vec3& position, float radius, EntityTypes::EntityType type, PickFilter searchFilter, QVector<QUuid>& foundEntities) const {
    forEachEntity([&](EntityItemPointer entity) {
        if (!checkFilterSettings(entity, searchFilter) || type != entity->getType()) {
            return;
        }

        bool success;
        AABox entityBox = entity->getAABox(success);
        // if the sphere doesn't intersect with our world frame AABox, we don't need to consider the more complex case
        glm::vec3 penetration;
        if (success && entityBox.findSpherePenetration(position, radius, penetration)) {

            glm::vec3 dimensions = entity->getScaledDimensions();

            // FIXME - consider allowing the entity to determine penetration so that
            //         entities could presumably do actual hull testing if they wanted to
            // FIXME - handle entity->getShapeType() == SHAPE_TYPE_SPHERE case better in particular
            //         can we handle the ellipsoid case better? We only currently handle perfect spheres
            //         with centered registration points
            if (entity->getShapeType() == SHAPE_TYPE_SPHERE && (dimensions.x == dimensions.y && dimensions.y == dimensions.z)) {

                // NOTE: entity->getRadius() doesn't return the true radius, it returns the radius of the
                //       maximum bounding sphere, which is actually larger than our actual radius
                float entityTrueRadius = dimensions.x / 2.0f;

                bool success;
                glm::vec3 center = entity->getCenterPosition(success);
                if (success && findSphereSpherePenetration(position, radius, center, entityTrueRadius, penetration)) {
                    foundEntities.push_back(entity->getID());
                }
            } else {
                // determine the worldToEntityMatrix that doesn't include scale because
                // we're going to use the registration aware aa box in the entity frame
                glm::mat4 translation = glm::translate(entity->getWorldPosition());
                glm::mat4 rotation = glm::mat4_cast(entity->getWorldOrientation());
                glm::mat4 entityToWorldMatrix = translation * rotation;
                glm::mat4 worldToEntityMatrix = glm::inverse(entityToWorldMatrix);

                glm::vec3 registrationPoint = entity->getRegistrationPoint();
                glm::vec3 corner = -(dimensions * registrationPoint) + entity->getPivot();

                AABox entityFrameBox(corner, dimensions);

                glm::vec3 entityFrameSearchPosition = glm::vec3(worldToEntityMatrix * glm::vec4(position, 1.0f));
                if (entityFrameBox.findSpherePenetration(entityFrameSearchPosition, radius, penetration)) {
                    foundEntities.push_back(entity->getID());
                }
            }
        }
    });
}

void EntityTreeElement::evalEntitiesInSphereWithName(const glm::vec3& position, float radius, const QString& name, bool caseSensitive, PickFilter searchFilter, QVector<QUuid>& foundEntities) const {
    forEachEntity([&](EntityItemPointer entity) {
        if (!checkFilterSettings(entity, searchFilter)) {
            return;
        }

        QString entityName = entity->getName();
        if ((caseSensitive && name != entityName) || (!caseSensitive && name.toLower() != entityName.toLower())) {
            return;
        }

        bool success;
        AABox entityBox = entity->getAABox(success);

        // if the sphere doesn't intersect with our world frame AABox, we don't need to consider the more complex case
        glm::vec3 penetration;
        if (success && entityBox.findSpherePenetration(position, radius, penetration)) {

            glm::vec3 dimensions = entity->getScaledDimensions();

            // FIXME - consider allowing the entity to determine penetration so that
            //         entities could presumably do actual hull testing if they wanted to
            // FIXME - handle entity->getShapeType() == SHAPE_TYPE_SPHERE case better in particular
            //         can we handle the ellipsoid case better? We only currently handle perfect spheres
            //         with centered registration points
            if (entity->getShapeType() == SHAPE_TYPE_SPHERE && (dimensions.x == dimensions.y && dimensions.y == dimensions.z)) {

                // NOTE: entity->getRadius() doesn't return the true radius, it returns the radius of the
                //       maximum bounding sphere, which is actually larger than our actual radius
                float entityTrueRadius = dimensions.x / 2.0f;
                bool success;
                glm::vec3 center = entity->getCenterPosition(success);

                if (success && findSphereSpherePenetration(position, radius, center, entityTrueRadius, penetration)) {
                    foundEntities.push_back(entity->getID());
                }
            } else {
                // determine the worldToEntityMatrix that doesn't include scale because
                // we're going to use the registration aware aa box in the entity frame
                glm::mat4 translation = glm::translate(entity->getWorldPosition());
                glm::mat4 rotation = glm::mat4_cast(entity->getWorldOrientation());
                glm::mat4 entityToWorldMatrix = translation * rotation;
                glm::mat4 worldToEntityMatrix = glm::inverse(entityToWorldMatrix);

                glm::vec3 registrationPoint = entity->getRegistrationPoint();
                glm::vec3 corner = -(dimensions * registrationPoint) + entity->getPivot();

                AABox entityFrameBox(corner, dimensions);

                glm::vec3 entityFrameSearchPosition = glm::vec3(worldToEntityMatrix * glm::vec4(position, 1.0f));
                if (entityFrameBox.findSpherePenetration(entityFrameSearchPosition, radius, penetration)) {
                    foundEntities.push_back(entity->getID());
                }
            }
        }
    });
}

void EntityTreeElement::evalEntitiesInCube(const AACube& cube, PickFilter searchFilter, QVector<QUuid>& foundEntities) const {
    forEachEntity([&](EntityItemPointer entity) {
        if (!checkFilterSettings(entity, searchFilter)) {
            return;
        }

        bool success;
        AABox entityBox = entity->getAABox(success);

        // FIXME - handle entity->getShapeType() == SHAPE_TYPE_SPHERE case better
        // FIXME - consider allowing the entity to determine penetration so that
        //         entities could presumably dull actuall hull testing if they wanted to
        // FIXME - is there an easy way to translate the search cube into something in the
        //         entity frame that can be easily tested against?
        //         simple algorithm is probably:
        //             if target box is fully inside search box == yes
        //             if search box is fully inside target box == yes
        //             for each face of search box:
        //                 translate the triangles of the face into the box frame
        //                 test the triangles of the face against the box?
        //                 if translated search face triangle intersect target box
        //                     add to result
        //

        // If the entities AABox touches the search cube then consider it to be found
        if (success && entityBox.touches(cube)) {
            foundEntities.push_back(entity->getID());
        }
    });
}

void EntityTreeElement::evalEntitiesInBox(const AABox& box, PickFilter searchFilter, QVector<QUuid>& foundEntities) const {
    forEachEntity([&](EntityItemPointer entity) {
        if (!checkFilterSettings(entity, searchFilter)) {
            return;
        }

        bool success;
        AABox entityBox = entity->getAABox(success);

        // FIXME - handle entity->getShapeType() == SHAPE_TYPE_SPHERE case better
        // FIXME - consider allowing the entity to determine penetration so that
        //         entities could presumably dull actuall hull testing if they wanted to
        // FIXME - is there an easy way to translate the search cube into something in the
        //         entity frame that can be easily tested against?
        //         simple algorithm is probably:
        //             if target box is fully inside search box == yes
        //             if search box is fully inside target box == yes
        //             for each face of search box:
        //                 translate the triangles of the face into the box frame
        //                 test the triangles of the face against the box?
        //                 if translated search face triangle intersect target box
        //                     add to result
        //

        // If the entities AABox touches the search cube then consider it to be found
        if (success && entityBox.touches(box)) {
            foundEntities.push_back(entity->getID());
        }
    });
}

void EntityTreeElement::evalEntitiesInFrustum(const ViewFrustum& frustum, PickFilter searchFilter, QVector<QUuid>& foundEntities) const {
    forEachEntity([&](EntityItemPointer entity) {
        if (!checkFilterSettings(entity, searchFilter)) {
            return;
        }

        bool success;
        AABox entityBox = entity->getAABox(success);

        // FIXME - See FIXMEs for similar methods above.
        if (success && (frustum.boxIntersectsFrustum(entityBox) || frustum.boxIntersectsKeyhole(entityBox))) {
            foundEntities.push_back(entity->getID());
        }
    });
}

void EntityTreeElement::getEntities(EntityItemFilter& filter, QVector<EntityItemPointer>& foundEntities) {
    forEachEntity([&](EntityItemPointer entity) {
        if (filter(entity)) {
            foundEntities.push_back(entity);
        }
    });
}

EntityItemPointer EntityTreeElement::getEntityWithEntityItemID(const EntityItemID& id) const {
    EntityItemPointer foundEntity = NULL;
    withReadLock([&] {
        foreach(EntityItemPointer entity, _entityItems) {
            if (entity->getEntityItemID() == id) {
                foundEntity = entity;
                break;
            }
        }
    });
    return foundEntity;
}

void EntityTreeElement::cleanupDomainAndNonOwnedEntities() {
    withWriteLock([&] {
        EntityItems savedEntities;
        foreach(EntityItemPointer entity, _entityItems) {
            if (!(entity->isLocalEntity() || entity->isMyAvatarEntity())) {
                entity->preDelete();
                entity->_element = NULL;
            } else {
                savedEntities.push_back(entity);
            }
        }

        _entityItems = savedEntities;
    });
    bumpChangedContent();
}

void EntityTreeElement::cleanupEntities() {
    withWriteLock([&] {
        foreach(EntityItemPointer entity, _entityItems) {
            entity->preDelete();
            // NOTE: only EntityTreeElement should ever be changing the value of entity->_element
            // NOTE: We explicitly don't delete the EntityItem here because since we only
            // access it by smart pointers, when we remove it from the _entityItems
            // we know that it will be deleted.
            entity->_element = NULL;
        }
        _entityItems.clear();
    });
    bumpChangedContent();
}

bool EntityTreeElement::removeEntityItem(EntityItemPointer entity, bool deletion) {
    if (deletion) {
        entity->preDelete();
    }
    int numEntries = 0;
    withWriteLock([&] {
        numEntries = _entityItems.removeAll(entity);
    });
    if (numEntries > 0) {
        // NOTE: only EntityTreeElement should ever be changing the value of entity->_element
        assert(entity->_element.get() == this);
        entity->_element = NULL;
        bumpChangedContent();
        return true;
    }
    return false;
}


int EntityTreeElement::readElementDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
            ReadBitstreamToTreeParams& args) {
    return _myTree->readEntityDataFromBuffer(data, bytesLeftToRead, args);
}

void EntityTreeElement::addEntityItem(EntityItemPointer entity) {
    assert(entity);
    assert(entity->_element == nullptr);
    withWriteLock([&] {
        _entityItems.push_back(entity);
    });
    bumpChangedContent();
    entity->_element = getThisPointer();
}

// will average a "common reduced LOD view" from the the child elements...
void EntityTreeElement::calculateAverageFromChildren() {
    // nothing to do here yet...
}

// will detect if children are leaves AND collapsable into the parent node
// and in that case will collapse children and make this node
// a leaf, returns TRUE if all the leaves are collapsed into a
// single node
bool EntityTreeElement::collapseChildren() {
    // nothing to do here yet...
    return false;
}

bool EntityTreeElement::pruneChildren() {
    bool somethingPruned = false;
    for (int childIndex = 0; childIndex < NUMBER_OF_CHILDREN; childIndex++) {
        EntityTreeElementPointer child = getChildAtIndex(childIndex);

        // if my child is a leaf, but has no entities, then it's safe to delete my child
        if (child && child->isLeaf() && !child->hasEntities()) {
            deleteChildAtIndex(childIndex);
            somethingPruned = true;
        }
    }
    return somethingPruned;
}

void EntityTreeElement::expandExtentsToContents(Extents& extents) {
    withReadLock([&] {
        foreach(EntityItemPointer entity, _entityItems) {
            bool success;
            AABox aaBox = entity->getAABox(success);
            if (success) {
                extents.add(aaBox);
            }
        }
    });
}

uint16_t EntityTreeElement::size() const {
    uint16_t result = 0;
    withReadLock([&] {
        result = _entityItems.size();
    });
    return result;
}


void EntityTreeElement::debugDump() {
    qCDebug(entities) << "EntityTreeElement...";
    qCDebug(entities) << "    cube:" << _cube;
    qCDebug(entities) << "    has child elements:" << getChildCount();

    withReadLock([&] {
        if (_entityItems.size()) {
            qCDebug(entities) << "    has entities:" << _entityItems.size();
            qCDebug(entities) << "--------------------------------------------------";
            for (uint16_t i = 0; i < _entityItems.size(); i++) {
                EntityItemPointer entity = _entityItems[i];
                entity->debugDump();
            }
            qCDebug(entities) << "--------------------------------------------------";
        } else {
            qCDebug(entities) << "    NO entities!";
        }
    });
}

