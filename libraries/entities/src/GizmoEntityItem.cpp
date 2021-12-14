//
//  Created by Sam Gondelman on 1/22/19
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GizmoEntityItem.h"

#include "EntityItemProperties.h"

#include <qmath.h>

EntityItemPointer GizmoEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    Pointer entity(new GizmoEntityItem(entityID), [](GizmoEntityItem* ptr) { ptr->deleteLater(); });
    entity->setProperties(properties);
    return entity;
}

// our non-pure virtual subclass for now...
GizmoEntityItem::GizmoEntityItem(const EntityItemID& entityItemID) : EntityItem(entityItemID) {
    _type = EntityTypes::Gizmo;
}

void GizmoEntityItem::setUnscaledDimensions(const glm::vec3& value) {
    // NOTE: Gizmo Entities always have a "height" of 1mm.
    EntityItem::setUnscaledDimensions(glm::vec3(value.x, ENTITY_ITEM_MIN_DIMENSION, value.z));
}

EntityItemProperties GizmoEntityItem::getProperties(const EntityPropertyFlags& desiredProperties, bool allowEmptyDesiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties, allowEmptyDesiredProperties); // get the properties from our base class

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(gizmoType, getGizmoType);
    withReadLock([&] {
        _ringProperties.getProperties(properties);
    });

    return properties;
}

bool GizmoEntityItem::setSubClassProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(gizmoType, setGizmoType);
    withWriteLock([&] {
        bool ringPropertiesChanged = _ringProperties.setProperties(properties);
        somethingChanged |= ringPropertiesChanged;
        _needsRenderUpdate |= ringPropertiesChanged;
    });

    return somethingChanged;
}

int GizmoEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_GIZMO_TYPE, GizmoType, setGizmoType);
    withWriteLock([&] {
        int bytesFromRing = _ringProperties.readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args,
            propertyFlags, overwriteLocalData,
            somethingChanged);
        bytesRead += bytesFromRing;
        dataAt += bytesFromRing;
    });

    return bytesRead;
}

EntityPropertyFlags GizmoEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);

    requestedProperties += PROP_GIZMO_TYPE;
    requestedProperties += _ringProperties.getEntityProperties(params);

    return requestedProperties;
}

void GizmoEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeDataPointer entityTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_GIZMO_TYPE, (uint32_t)getGizmoType());
    withReadLock([&] {
        _ringProperties.appendSubclassData(packetData, params, entityTreeElementExtraEncodeData, requestedProperties,
            propertyFlags, propertiesDidntFit, propertyCount, appendState);
    });
}

bool GizmoEntityItem::supportsDetailedIntersection() const {
    return _gizmoType == GizmoType::RING;
}

bool GizmoEntityItem::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                                  const glm::vec3& viewFrustumPos, OctreeElementPointer& element,
                                                  float& distance, BoxFace& face, glm::vec3& surfaceNormal,
                                                  QVariantMap& extraInfo, bool precisionPicking) const {
    glm::vec3 dimensions = getScaledDimensions();
    glm::vec2 xyDimensions(dimensions.x, dimensions.z);
    BillboardMode billboardMode = getBillboardMode();
    glm::quat rotation = billboardMode == BillboardMode::NONE ? getWorldOrientation() : getLocalOrientation();
    rotation *= glm::angleAxis(-(float)M_PI_2, Vectors::RIGHT);
    glm::vec3 position = getWorldPosition() + rotation * (dimensions * (ENTITY_ITEM_DEFAULT_REGISTRATION_POINT - getRegistrationPoint()));
    rotation = BillboardModeHelpers::getBillboardRotation(position, rotation, billboardMode, viewFrustumPos);

    if (findRayRectangleIntersection(origin, direction, rotation, position, xyDimensions, distance)) {
        glm::vec3 hitPosition = origin + (distance * direction);
        glm::vec3 localHitPosition = glm::inverse(rotation) * (hitPosition - getWorldPosition());
        localHitPosition.x /= xyDimensions.x;
        localHitPosition.y /= xyDimensions.y;
        float distanceToHit = glm::length(localHitPosition);

        if (0.5f * _ringProperties.getInnerRadius() <= distanceToHit && distanceToHit <= 0.5f) {
            glm::vec3 forward = rotation * Vectors::FRONT;
            if (glm::dot(forward, direction) > 0.0f) {
                face = MAX_Z_FACE;
                surfaceNormal = -forward;
            } else {
                face = MIN_Z_FACE;
                surfaceNormal = forward;
            }
            return true;
        }
    }

    return false;
}

bool GizmoEntityItem::findDetailedParabolaIntersection(const glm::vec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration,
                                                       const glm::vec3& viewFrustumPos, OctreeElementPointer& element, float& parabolicDistance,
                                                       BoxFace& face, glm::vec3& surfaceNormal,
                                                       QVariantMap& extraInfo, bool precisionPicking) const {
    //// Scale the dimensions by the diameter
    glm::vec3 dimensions = getScaledDimensions();
    glm::vec2 xyDimensions(dimensions.x, dimensions.z);
    BillboardMode billboardMode = getBillboardMode();
    glm::quat rotation = billboardMode == BillboardMode::NONE ? getWorldOrientation() : getLocalOrientation();
    rotation *= glm::angleAxis(-(float)M_PI_2, Vectors::RIGHT);
    glm::vec3 position = getWorldPosition();
    rotation = BillboardModeHelpers::getBillboardRotation(position, rotation, billboardMode, viewFrustumPos);

    glm::quat inverseRot = glm::inverse(rotation);
    glm::vec3 localOrigin = inverseRot * (origin - position);
    glm::vec3 localVelocity = inverseRot * velocity;
    glm::vec3 localAcceleration = inverseRot * acceleration;

    if (findParabolaRectangleIntersection(localOrigin, localVelocity, localAcceleration, xyDimensions, parabolicDistance)) {
        glm::vec3 localHitPosition = localOrigin + localVelocity * parabolicDistance + 0.5f * localAcceleration * parabolicDistance * parabolicDistance;
        localHitPosition.x /= xyDimensions.x;
        localHitPosition.y /= xyDimensions.y;
        float distanceToHit = glm::length(localHitPosition);

        if (0.5f * _ringProperties.getInnerRadius() <= distanceToHit && distanceToHit <= 0.5f) {
            float localIntersectionVelocityZ = localVelocity.z + localAcceleration.z * parabolicDistance;
            glm::vec3 forward = rotation * Vectors::FRONT;
            if (localIntersectionVelocityZ > 0.0f) {
                face = MIN_Z_FACE;
                surfaceNormal = forward;
            } else {
                face = MAX_Z_FACE;
                surfaceNormal = -forward;
            }
            return true;
        }
    }

    return false;
}

void GizmoEntityItem::setGizmoType(GizmoType value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _gizmoType != value;
        _gizmoType = value;
    });
}

GizmoType GizmoEntityItem::getGizmoType() const {
    return resultWithReadLock<GizmoType>([&] {
        return _gizmoType;
    });
}

RingGizmoPropertyGroup GizmoEntityItem::getRingProperties() const {
    return resultWithReadLock<RingGizmoPropertyGroup>([&] {
        return _ringProperties;
    });
}