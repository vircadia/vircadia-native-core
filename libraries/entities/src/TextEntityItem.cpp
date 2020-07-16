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
const float TextEntityItem::DEFAULT_TEXT_ALPHA = 1.0f;
const glm::u8vec3 TextEntityItem::DEFAULT_BACKGROUND_COLOR = { 0, 0, 0};
const float TextEntityItem::DEFAULT_MARGIN = 0.0f;
const float TextEntityItem::DEFAULT_TEXT_EFFECT_THICKNESS = 0.2f;

EntityItemPointer TextEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity(new TextEntityItem(entityID), [](EntityItem* ptr) { ptr->deleteLater(); });
    entity->setProperties(properties);
    return entity;
}

TextEntityItem::TextEntityItem(const EntityItemID& entityItemID) : EntityItem(entityItemID) {
    _type = EntityTypes::Text;
}

void TextEntityItem::setUnscaledDimensions(const glm::vec3& value) {
    const float TEXT_ENTITY_ITEM_FIXED_DEPTH = 0.01f;
    // NOTE: Text Entities always have a "depth" of 1cm.
    EntityItem::setUnscaledDimensions(glm::vec3(value.x, value.y, TEXT_ENTITY_ITEM_FIXED_DEPTH));
}

EntityItemProperties TextEntityItem::getProperties(const EntityPropertyFlags& desiredProperties, bool allowEmptyDesiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties, allowEmptyDesiredProperties); // get the properties from our base class

    withReadLock([&] {
        _pulseProperties.getProperties(properties);
    });
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(billboardMode, getBillboardMode);

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(text, getText);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(lineHeight, getLineHeight);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(textColor, getTextColor);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(textAlpha, getTextAlpha);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(backgroundColor, getBackgroundColor);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(backgroundAlpha, getBackgroundAlpha);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(leftMargin, getLeftMargin);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(rightMargin, getRightMargin);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(topMargin, getTopMargin);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(bottomMargin, getBottomMargin);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(unlit, getUnlit);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(font, getFont);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(textEffect, getTextEffect);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(textEffectColor, getTextEffectColor);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(textEffectThickness, getTextEffectThickness);
    return properties;
}

bool TextEntityItem::setSubClassProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;

    withWriteLock([&] {
        bool pulsePropertiesChanged = _pulseProperties.setProperties(properties);
        somethingChanged |= pulsePropertiesChanged;
        _needsRenderUpdate |= pulsePropertiesChanged;
    });
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(billboardMode, setBillboardMode);

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(text, setText);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(lineHeight, setLineHeight);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(textColor, setTextColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(textAlpha, setTextAlpha);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(backgroundColor, setBackgroundColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(backgroundAlpha, setBackgroundAlpha);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(leftMargin, setLeftMargin);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(rightMargin, setRightMargin);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(topMargin, setTopMargin);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(bottomMargin, setBottomMargin);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(unlit, setUnlit);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(font, setFont);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(textEffect, setTextEffect);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(textEffectColor, setTextEffectColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(textEffectThickness, setTextEffectThickness);
    
    return somethingChanged;
}

int TextEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    withWriteLock([&] {
        int bytesFromPulse = _pulseProperties.readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args,
            propertyFlags, overwriteLocalData,
            somethingChanged);
        bytesRead += bytesFromPulse;
        dataAt += bytesFromPulse;
    });
    READ_ENTITY_PROPERTY(PROP_BILLBOARD_MODE, BillboardMode, setBillboardMode);

    READ_ENTITY_PROPERTY(PROP_TEXT, QString, setText);
    READ_ENTITY_PROPERTY(PROP_LINE_HEIGHT, float, setLineHeight);
    READ_ENTITY_PROPERTY(PROP_TEXT_COLOR, glm::u8vec3, setTextColor);
    READ_ENTITY_PROPERTY(PROP_TEXT_ALPHA, float, setTextAlpha);
    READ_ENTITY_PROPERTY(PROP_BACKGROUND_COLOR, glm::u8vec3, setBackgroundColor);
    READ_ENTITY_PROPERTY(PROP_BACKGROUND_ALPHA, float, setBackgroundAlpha);
    READ_ENTITY_PROPERTY(PROP_LEFT_MARGIN, float, setLeftMargin);
    READ_ENTITY_PROPERTY(PROP_RIGHT_MARGIN, float, setRightMargin);
    READ_ENTITY_PROPERTY(PROP_TOP_MARGIN, float, setTopMargin);
    READ_ENTITY_PROPERTY(PROP_BOTTOM_MARGIN, float, setBottomMargin);
    READ_ENTITY_PROPERTY(PROP_UNLIT, bool, setUnlit);
    READ_ENTITY_PROPERTY(PROP_FONT, QString, setFont);
    READ_ENTITY_PROPERTY(PROP_TEXT_EFFECT, TextEffect, setTextEffect);
    READ_ENTITY_PROPERTY(PROP_TEXT_EFFECT_COLOR, glm::u8vec3, setTextEffectColor);
    READ_ENTITY_PROPERTY(PROP_TEXT_EFFECT_THICKNESS, float, setTextEffectThickness);

    return bytesRead;
}

EntityPropertyFlags TextEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);

    requestedProperties += _pulseProperties.getEntityProperties(params);
    requestedProperties += PROP_BILLBOARD_MODE;

    requestedProperties += PROP_TEXT;
    requestedProperties += PROP_LINE_HEIGHT;
    requestedProperties += PROP_TEXT_COLOR;
    requestedProperties += PROP_TEXT_ALPHA;
    requestedProperties += PROP_BACKGROUND_COLOR;
    requestedProperties += PROP_BACKGROUND_ALPHA;
    requestedProperties += PROP_LEFT_MARGIN;
    requestedProperties += PROP_RIGHT_MARGIN;
    requestedProperties += PROP_TOP_MARGIN;
    requestedProperties += PROP_BOTTOM_MARGIN;
    requestedProperties += PROP_UNLIT;
    requestedProperties += PROP_FONT;
    requestedProperties += PROP_TEXT_EFFECT;
    requestedProperties += PROP_TEXT_EFFECT_COLOR;
    requestedProperties += PROP_TEXT_EFFECT_THICKNESS;

    return requestedProperties;
}

void TextEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                    EntityTreeElementExtraEncodeDataPointer entityTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    withReadLock([&] {
        _pulseProperties.appendSubclassData(packetData, params, entityTreeElementExtraEncodeData, requestedProperties,
            propertyFlags, propertiesDidntFit, propertyCount, appendState);
    });
    APPEND_ENTITY_PROPERTY(PROP_BILLBOARD_MODE, (uint32_t)getBillboardMode());

    APPEND_ENTITY_PROPERTY(PROP_TEXT, getText());
    APPEND_ENTITY_PROPERTY(PROP_LINE_HEIGHT, getLineHeight());
    APPEND_ENTITY_PROPERTY(PROP_TEXT_COLOR, getTextColor());
    APPEND_ENTITY_PROPERTY(PROP_TEXT_ALPHA, getTextAlpha());
    APPEND_ENTITY_PROPERTY(PROP_BACKGROUND_COLOR, getBackgroundColor());
    APPEND_ENTITY_PROPERTY(PROP_BACKGROUND_ALPHA, getBackgroundAlpha());
    APPEND_ENTITY_PROPERTY(PROP_LEFT_MARGIN, getLeftMargin());
    APPEND_ENTITY_PROPERTY(PROP_RIGHT_MARGIN, getRightMargin());
    APPEND_ENTITY_PROPERTY(PROP_TOP_MARGIN, getTopMargin());
    APPEND_ENTITY_PROPERTY(PROP_BOTTOM_MARGIN, getBottomMargin());
    APPEND_ENTITY_PROPERTY(PROP_UNLIT, getUnlit());
    APPEND_ENTITY_PROPERTY(PROP_FONT, getFont());
    APPEND_ENTITY_PROPERTY(PROP_TEXT_EFFECT, (uint32_t)getTextEffect());
    APPEND_ENTITY_PROPERTY(PROP_TEXT_EFFECT_COLOR, getTextEffectColor());
    APPEND_ENTITY_PROPERTY(PROP_TEXT_EFFECT_THICKNESS, getTextEffectThickness());
}

glm::vec3 TextEntityItem::getRaycastDimensions() const {
    glm::vec3 dimensions = getScaledDimensions();
    if (getBillboardMode() != BillboardMode::NONE) {
        float max = glm::max(dimensions.x, glm::max(dimensions.y, dimensions.z));
        const float SQRT_2 = 1.41421356237f;
        return glm::vec3(SQRT_2 * max);
    }
    return dimensions;
}

bool TextEntityItem::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                                 OctreeElementPointer& element, float& distance,
                                                 BoxFace& face, glm::vec3& surfaceNormal,
                                                 QVariantMap& extraInfo, bool precisionPicking) const {
    glm::vec3 dimensions = getScaledDimensions();
    glm::vec2 xyDimensions(dimensions.x, dimensions.y);
    glm::quat rotation = getWorldOrientation();
    glm::vec3 position = getWorldPosition() + rotation * (dimensions * (ENTITY_ITEM_DEFAULT_REGISTRATION_POINT - getRegistrationPoint()));
    rotation = EntityItem::getBillboardRotation(position, rotation, _billboardMode, EntityItem::getPrimaryViewFrustumPosition());

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
        _needsRenderUpdate |= _text != value;
        _text = value;
    });
}

QString TextEntityItem::getText() const {
    return resultWithReadLock<QString>([&] {
        return _text;
    });
}

void TextEntityItem::setLineHeight(float value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _lineHeight != value;
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
        _needsRenderUpdate |= _textColor != value;
        _textColor = value;
    });
}

glm::u8vec3 TextEntityItem::getTextColor() const {
    return resultWithReadLock<glm::u8vec3>([&] {
        return _textColor;
    });
}

void TextEntityItem::setTextAlpha(float value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _textAlpha != value;
        _textAlpha = value;
    });
}

float TextEntityItem::getTextAlpha() const {
    return resultWithReadLock<float>([&] {
        return _textAlpha;
    });
}

void TextEntityItem::setBackgroundColor(const glm::u8vec3& value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _backgroundColor != value;
        _backgroundColor = value;
    });
}

glm::u8vec3 TextEntityItem::getBackgroundColor() const {
    return resultWithReadLock<glm::u8vec3>([&] {
        return _backgroundColor;
    });
}

void TextEntityItem::setBackgroundAlpha(float value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _backgroundAlpha != value;
        _backgroundAlpha = value;
    });
}

float TextEntityItem::getBackgroundAlpha() const {
    return resultWithReadLock<float>([&] {
        return _backgroundAlpha;
    });
}

BillboardMode TextEntityItem::getBillboardMode() const {
    BillboardMode result;
    withReadLock([&] {
        result = _billboardMode;
    });
    return result;
}

void TextEntityItem::setBillboardMode(BillboardMode value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _billboardMode != value;
        _billboardMode = value;
    });
}

void TextEntityItem::setLeftMargin(float value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _leftMargin != value;
        _leftMargin = value;
    });
}

float TextEntityItem::getLeftMargin() const {
    return resultWithReadLock<float>([&] {
        return _leftMargin;
    });
}

void TextEntityItem::setRightMargin(float value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _rightMargin != value;
        _rightMargin = value;
    });
}

float TextEntityItem::getRightMargin() const {
    return resultWithReadLock<float>([&] {
        return _rightMargin;
    });
}

void TextEntityItem::setTopMargin(float value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _topMargin != value;
        _topMargin = value;
    });
}

float TextEntityItem::getTopMargin() const {
    return resultWithReadLock<float>([&] {
        return _topMargin;
    });
}

void TextEntityItem::setBottomMargin(float value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _bottomMargin != value;
        _bottomMargin = value;
    });
}

float TextEntityItem::getBottomMargin() const {
    return resultWithReadLock<float>([&] {
        return _bottomMargin;
    });
}

void TextEntityItem::setUnlit(bool value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _unlit != value;
        _unlit = value;
    });
}

bool TextEntityItem::getUnlit() const {
    return resultWithReadLock<bool>([&] {
        return _unlit;
    });
}

void TextEntityItem::setFont(const QString& value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _font != value;
        _font = value;
    });
}

QString TextEntityItem::getFont() const {
    return resultWithReadLock<QString>([&] {
        return _font;
    });
}

void TextEntityItem::setTextEffect(TextEffect value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _effect != value;
        _effect = value;
    });
}

TextEffect TextEntityItem::getTextEffect() const {
    return resultWithReadLock<TextEffect>([&] {
        return _effect;
    });
}

void TextEntityItem::setTextEffectColor(const glm::u8vec3& value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _effectColor != value;
        _effectColor = value;
    });
}

glm::u8vec3 TextEntityItem::getTextEffectColor() const {
    return resultWithReadLock<glm::u8vec3>([&] {
        return _effectColor;
    });
}

void TextEntityItem::setTextEffectThickness(float value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _effectThickness != value;
        _effectThickness = value;
    });
}

float TextEntityItem::getTextEffectThickness() const {
    return resultWithReadLock<float>([&] {
        return _effectThickness;
    });
}

PulsePropertyGroup TextEntityItem::getPulseProperties() const {
    return resultWithReadLock<PulsePropertyGroup>([&] {
        return _pulseProperties;
    });
}