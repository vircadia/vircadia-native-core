//
//  Created by Sam Gondelman on 11/29/18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GridEntityItem.h"

#include "EntityItemProperties.h"

EntityItemPointer GridEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    Pointer entity(new GridEntityItem(entityID), [](EntityItem* ptr) { ptr->deleteLater(); });
    entity->setProperties(properties);
    return entity;
}

// our non-pure virtual subclass for now...
GridEntityItem::GridEntityItem(const EntityItemID& entityItemID) : EntityItem(entityItemID) {
    _type = EntityTypes::Grid;
}

EntityItemProperties GridEntityItem::getProperties(const EntityPropertyFlags& desiredProperties, bool allowEmptyDesiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties, allowEmptyDesiredProperties); // get the properties from our base class

    //COPY_ENTITY_PROPERTY_TO_PROPERTIES(imageURL, getGridURL);
    //COPY_ENTITY_PROPERTY_TO_PROPERTIES(emissive, getEmissive);
    //COPY_ENTITY_PROPERTY_TO_PROPERTIES(keepAspectRatio, getKeepAspectRatio);
    //COPY_ENTITY_PROPERTY_TO_PROPERTIES(faceCamera, getFaceCamera);
    //COPY_ENTITY_PROPERTY_TO_PROPERTIES(subGrid, getSubGrid);

    return properties;
}

bool GridEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    //SET_ENTITY_PROPERTY_FROM_PROPERTIES(imageURL, setGridURL);
    //SET_ENTITY_PROPERTY_FROM_PROPERTIES(emissive, setEmissive);
    //SET_ENTITY_PROPERTY_FROM_PROPERTIES(keepAspectRatio, setKeepAspectRatio);
    //SET_ENTITY_PROPERTY_FROM_PROPERTIES(faceCamera, setFaceCamera);
    //SET_ENTITY_PROPERTY_FROM_PROPERTIES(subGrid, setSubGrid);

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qCDebug(entities) << "GridEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                    "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEdited(properties.getLastEdited());
    }
    return somethingChanged;
}

int GridEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    //READ_ENTITY_PROPERTY(PROP_IMAGE_URL, QString, setGridURL);
    //READ_ENTITY_PROPERTY(PROP_EMISSIVE, bool, setEmissive);
    //READ_ENTITY_PROPERTY(PROP_KEEP_ASPECT_RATIO, bool, setKeepAspectRatio);
    //READ_ENTITY_PROPERTY(PROP_FACE_CAMERA, bool, setFaceCamera);
    //READ_ENTITY_PROPERTY(PROP_SUB_IMAGE, QRect, setSubGrid);

    return bytesRead;
}

EntityPropertyFlags GridEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);

    //requestedProperties += PROP_IMAGE_URL;
    //requestedProperties += PROP_EMISSIVE;
    //requestedProperties += PROP_KEEP_ASPECT_RATIO;
    //requestedProperties += PROP_FACE_CAMERA;
    //requestedProperties += PROP_SUB_IMAGE;

    return requestedProperties;
}

void GridEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeDataPointer modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    //APPEND_ENTITY_PROPERTY(PROP_IMAGE_URL, getGridURL());
    //APPEND_ENTITY_PROPERTY(PROP_EMISSIVE, getEmissive());
    //APPEND_ENTITY_PROPERTY(PROP_KEEP_ASPECT_RATIO, getKeepAspectRatio());
    //APPEND_ENTITY_PROPERTY(PROP_FACE_CAMERA, getFaceCamera());
    //APPEND_ENTITY_PROPERTY(PROP_SUB_IMAGE, getSubGrid());
}

bool GridEntityItem::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                                   OctreeElementPointer& element,
                                                   float& distance, BoxFace& face, glm::vec3& surfaceNormal,
                                                   QVariantMap& extraInfo, bool precisionPicking) const {
    glm::vec3 dimensions = getScaledDimensions();
    glm::vec2 xyDimensions(dimensions.x, dimensions.y);
    glm::quat rotation = getWorldOrientation();
    glm::vec3 position = getWorldPosition() + rotation * (dimensions * (ENTITY_ITEM_DEFAULT_REGISTRATION_POINT - getRegistrationPoint()));

    if (findRayRectangleIntersection(origin, direction, rotation, position, xyDimensions, distance)) {
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
    return false;
}

bool GridEntityItem::findDetailedParabolaIntersection(const glm::vec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration,
                                                       OctreeElementPointer& element, float& parabolicDistance,
                                                       BoxFace& face, glm::vec3& surfaceNormal,
                                                       QVariantMap& extraInfo, bool precisionPicking) const {
    glm::vec3 dimensions = getScaledDimensions();
    glm::vec2 xyDimensions(dimensions.x, dimensions.y);
    glm::quat rotation = getWorldOrientation();
    glm::vec3 position = getWorldPosition() + rotation * (dimensions * (ENTITY_ITEM_DEFAULT_REGISTRATION_POINT - getRegistrationPoint()));

    glm::quat inverseRot = glm::inverse(rotation);
    glm::vec3 localOrigin = inverseRot * (origin - position);
    glm::vec3 localVelocity = inverseRot * velocity;
    glm::vec3 localAcceleration = inverseRot * acceleration;

    if (findParabolaRectangleIntersection(localOrigin, localVelocity, localAcceleration, xyDimensions, parabolicDistance)) {
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
    return false;
}