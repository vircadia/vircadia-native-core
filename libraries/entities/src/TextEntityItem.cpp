//
//  TextEntityItem.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TextEntityItem.h"

#include <glm/gtx/transform.hpp>

#include <QDebug>

#include <ByteCountCoding.h>
#include <GeometryUtil.h>

#include "EntityItemProperties.h"
#include "EntitiesLogging.h"
#include "EntityTree.h"
#include "EntityTreeElement.h"

const QString TextEntityItem::DEFAULT_TEXT("");
const float TextEntityItem::DEFAULT_LINE_HEIGHT = 0.1f;
const glm::u8vec3 TextEntityItem::DEFAULT_TEXT_COLOR = { 255, 255, 255 };
const glm::u8vec3 TextEntityItem::DEFAULT_BACKGROUND_COLOR = { 0, 0, 0};
const bool TextEntityItem::DEFAULT_FACE_CAMERA = false;

EntityItemPointer TextEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity(new TextEntityItem(entityID), [](EntityItem* ptr) { ptr->deleteLater(); });
    entity->setProperties(properties);
    return entity;
}

TextEntityItem::TextEntityItem(const EntityItemID& entityItemID) : EntityItem(entityItemID) {
    _type = EntityTypes::Text;
}

const float TEXT_ENTITY_ITEM_FIXED_DEPTH = 0.01f;

void TextEntityItem::setUnscaledDimensions(const glm::vec3& value) {
    // NOTE: Text Entities always have a "depth" of 1cm.
    EntityItem::setUnscaledDimensions(glm::vec3(value.x, value.y, TEXT_ENTITY_ITEM_FIXED_DEPTH));
}

EntityItemProperties TextEntityItem::getProperties(const EntityPropertyFlags& desiredProperties, bool allowEmptyDesiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties, allowEmptyDesiredProperties); // get the properties from our base class

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(text, getText);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(lineHeight, getLineHeight);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(textColor, getTextColor);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(backgroundColor, getBackgroundColor);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(faceCamera, getFaceCamera);
    return properties;
}

bool TextEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;
    somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(text, setText);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(lineHeight, setLineHeight);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(textColor, setTextColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(backgroundColor, setBackgroundColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(faceCamera, setFaceCamera);

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qCDebug(entities) << "TextEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                    "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEdited(properties._lastEdited);
    }
    
    return somethingChanged;
}

int TextEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_TEXT, QString, setText);
    READ_ENTITY_PROPERTY(PROP_LINE_HEIGHT, float, setLineHeight);
    READ_ENTITY_PROPERTY(PROP_TEXT_COLOR, glm::u8vec3, setTextColor);
    READ_ENTITY_PROPERTY(PROP_BACKGROUND_COLOR, glm::u8vec3, setBackgroundColor);
    READ_ENTITY_PROPERTY(PROP_FACE_CAMERA, bool, setFaceCamera);
    
    return bytesRead;
}

EntityPropertyFlags TextEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_TEXT;
    requestedProperties += PROP_LINE_HEIGHT;
    requestedProperties += PROP_TEXT_COLOR;
    requestedProperties += PROP_BACKGROUND_COLOR;
    requestedProperties += PROP_FACE_CAMERA;
    return requestedProperties;
}

void TextEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                    EntityTreeElementExtraEncodeDataPointer modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const { 

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_TEXT, getText());
    APPEND_ENTITY_PROPERTY(PROP_LINE_HEIGHT, getLineHeight());
    APPEND_ENTITY_PROPERTY(PROP_TEXT_COLOR, getTextColor());
    APPEND_ENTITY_PROPERTY(PROP_BACKGROUND_COLOR, getBackgroundColor());
    APPEND_ENTITY_PROPERTY(PROP_FACE_CAMERA, getFaceCamera());
    
}

bool TextEntityItem::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                                 OctreeElementPointer& element, float& distance,
                                                 BoxFace& face, glm::vec3& surfaceNormal,
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

bool TextEntityItem::findDetailedParabolaIntersection(const glm::vec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration,
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

void TextEntityItem::setText(const QString& value) {
    withWriteLock([&] {
        _text = value;
    });
}

QString TextEntityItem::getText() const { 
    QString result;
    withReadLock([&] {
        result = _text;
    });
    return result;
}

void TextEntityItem::setLineHeight(float value) { 
    withWriteLock([&] {
        _lineHeight = value;
    });
}

float TextEntityItem::getLineHeight() const { 
    float result;
    withReadLock([&] {
        result = _lineHeight;
    });
    return result;
}

void TextEntityItem::setTextColor(const glm::u8vec3& value) {
    withWriteLock([&] {
        _textColor = value;
    });
}

glm::u8vec3 TextEntityItem::getTextColor() const {
    return resultWithReadLock<glm::u8vec3>([&] {
        return _textColor;
    });
}

void TextEntityItem::setBackgroundColor(const glm::u8vec3& value) {
    withWriteLock([&] {
        _backgroundColor = value;
    });
}

glm::u8vec3 TextEntityItem::getBackgroundColor() const {
    return resultWithReadLock<glm::u8vec3>([&] {
        return _backgroundColor;
    });
}

bool TextEntityItem::getFaceCamera() const { 
    bool result;
    withReadLock([&] {
        result = _faceCamera;
    });
    return result;
}

void TextEntityItem::setFaceCamera(bool value) { 
    withWriteLock([&] {
        _faceCamera = value;
    });
}

