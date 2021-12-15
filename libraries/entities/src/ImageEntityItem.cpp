//
//  Created by Sam Gondelman on 11/29/18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ImageEntityItem.h"

#include "EntityItemProperties.h"

EntityItemPointer ImageEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    Pointer entity(new ImageEntityItem(entityID), [](ImageEntityItem* ptr) { ptr->deleteLater(); });
    entity->setProperties(properties);
    return entity;
}

// our non-pure virtual subclass for now...
ImageEntityItem::ImageEntityItem(const EntityItemID& entityItemID) : EntityItem(entityItemID) {
    _type = EntityTypes::Image;
}

void ImageEntityItem::setUnscaledDimensions(const glm::vec3& value) {
    const float IMAGE_ENTITY_ITEM_FIXED_DEPTH = 0.01f;
    // NOTE: Image Entities always have a "depth" of 1cm.
    EntityItem::setUnscaledDimensions(glm::vec3(value.x, value.y, IMAGE_ENTITY_ITEM_FIXED_DEPTH));
}

EntityItemProperties ImageEntityItem::getProperties(const EntityPropertyFlags& desiredProperties, bool allowEmptyDesiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties, allowEmptyDesiredProperties); // get the properties from our base class

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(color, getColor);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(alpha, getAlpha);
    withReadLock([&] {
        _pulseProperties.getProperties(properties);
        properties.setNaturalDimensions(_naturalDimensions);
    });

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(imageURL, getImageURL);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(emissive, getEmissive);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(keepAspectRatio, getKeepAspectRatio);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(subImage, getSubImage);

    return properties;
}

bool ImageEntityItem::setSubClassProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(color, setColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(alpha, setAlpha);
    withWriteLock([&] {
        bool pulsePropertiesChanged = _pulseProperties.setProperties(properties);
        somethingChanged |= pulsePropertiesChanged;
        _needsRenderUpdate |= pulsePropertiesChanged;
    });

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(imageURL, setImageURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(emissive, setEmissive);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(keepAspectRatio, setKeepAspectRatio);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(subImage, setSubImage);

    return somethingChanged;
}

int ImageEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_COLOR, u8vec3Color, setColor);
    READ_ENTITY_PROPERTY(PROP_ALPHA, float, setAlpha);
    withWriteLock([&] {
        int bytesFromPulse = _pulseProperties.readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args,
            propertyFlags, overwriteLocalData,
            somethingChanged);
        bytesRead += bytesFromPulse;
        dataAt += bytesFromPulse;
    });

    READ_ENTITY_PROPERTY(PROP_IMAGE_URL, QString, setImageURL);
    READ_ENTITY_PROPERTY(PROP_EMISSIVE, bool, setEmissive);
    READ_ENTITY_PROPERTY(PROP_KEEP_ASPECT_RATIO, bool, setKeepAspectRatio);
    READ_ENTITY_PROPERTY(PROP_SUB_IMAGE, QRect, setSubImage);

    return bytesRead;
}

EntityPropertyFlags ImageEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);

    requestedProperties += PROP_COLOR;
    requestedProperties += PROP_ALPHA;
    requestedProperties += _pulseProperties.getEntityProperties(params);

    requestedProperties += PROP_IMAGE_URL;
    requestedProperties += PROP_EMISSIVE;
    requestedProperties += PROP_KEEP_ASPECT_RATIO;
    requestedProperties += PROP_SUB_IMAGE;

    return requestedProperties;
}

void ImageEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeDataPointer entityTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_COLOR, getColor());
    APPEND_ENTITY_PROPERTY(PROP_ALPHA, getAlpha());
    withReadLock([&] {
        _pulseProperties.appendSubclassData(packetData, params, entityTreeElementExtraEncodeData, requestedProperties,
            propertyFlags, propertiesDidntFit, propertyCount, appendState);
    });

    APPEND_ENTITY_PROPERTY(PROP_IMAGE_URL, getImageURL());
    APPEND_ENTITY_PROPERTY(PROP_EMISSIVE, getEmissive());
    APPEND_ENTITY_PROPERTY(PROP_KEEP_ASPECT_RATIO, getKeepAspectRatio());
    APPEND_ENTITY_PROPERTY(PROP_SUB_IMAGE, getSubImage());
}

QString ImageEntityItem::getImageURL() const {
    QString result;
    withReadLock([&] {
        result = _imageURL;
    });
    return result;
}

void ImageEntityItem::setImageURL(const QString& url) {
    withWriteLock([&] {
        _needsRenderUpdate |= _imageURL != url;
        _imageURL = url;
    });
}

bool ImageEntityItem::getEmissive() const {
    bool result;
    withReadLock([&] {
        result = _emissive;
    });
    return result;
}

void ImageEntityItem::setEmissive(bool emissive) {
    withWriteLock([&] {
        _needsRenderUpdate |= _emissive != emissive;
        _emissive = emissive;
    });
}

bool ImageEntityItem::getKeepAspectRatio() const {
    bool result;
    withReadLock([&] {
        result = _keepAspectRatio;
    });
    return result;
}

void ImageEntityItem::setKeepAspectRatio(bool keepAspectRatio) {
    withWriteLock([&] {
        _needsRenderUpdate |= _keepAspectRatio != keepAspectRatio;
        _keepAspectRatio = keepAspectRatio;
    });
}

QRect ImageEntityItem::getSubImage() const {
    QRect result;
    withReadLock([&] {
        result = _subImage;
    });
    return result;
}

void ImageEntityItem::setSubImage(const QRect& subImage) {
    withWriteLock([&] {
        _needsRenderUpdate |= _subImage != subImage;
        _subImage = subImage;
    });
}

void ImageEntityItem::setColor(const glm::u8vec3& color) {
    withWriteLock([&] {
        _needsRenderUpdate |= _color != color;
        _color = color;
    });
}

glm::u8vec3 ImageEntityItem::getColor() const {
    return resultWithReadLock<glm::u8vec3>([&] {
        return _color;
    });
}

void ImageEntityItem::setAlpha(float alpha) {
    withWriteLock([&] {
        _needsRenderUpdate |= _alpha != alpha;
        _alpha = alpha;
    });
}

float ImageEntityItem::getAlpha() const {
    return resultWithReadLock<float>([&] {
        return _alpha;
    });
}

PulsePropertyGroup ImageEntityItem::getPulseProperties() const {
    return resultWithReadLock<PulsePropertyGroup>([&] {
        return _pulseProperties;
    });
}

void ImageEntityItem::setNaturalDimension(const glm::vec3& naturalDimensions) const {
    withWriteLock([&] {
        _naturalDimensions = naturalDimensions;
    });
}
