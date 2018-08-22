//
//  LightEntityItem.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "LightEntityItem.h"

#include <QDebug>

#include <ByteCountCoding.h>

#include "EntitiesLogging.h"
#include "EntityItemID.h"
#include "EntityItemProperties.h"
#include "EntityTree.h"
#include "EntityTreeElement.h"

const bool LightEntityItem::DEFAULT_IS_SPOTLIGHT = false;
const float LightEntityItem::DEFAULT_INTENSITY = 1.0f;
const float LightEntityItem::DEFAULT_FALLOFF_RADIUS = 0.1f;
const float LightEntityItem::DEFAULT_EXPONENT = 0.0f;
const float LightEntityItem::DEFAULT_CUTOFF = PI / 2.0f;

bool LightEntityItem::_lightsArePickable = false;

EntityItemPointer LightEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity(new LightEntityItem(entityID), [](EntityItem* ptr) { ptr->deleteLater(); });
    entity->setProperties(properties);
    return entity;
}

// our non-pure virtual subclass for now...
LightEntityItem::LightEntityItem(const EntityItemID& entityItemID) : EntityItem(entityItemID) {
    _type = EntityTypes::Light;
    _color[RED_INDEX] = _color[GREEN_INDEX] = _color[BLUE_INDEX] = 0;
}

void LightEntityItem::setUnscaledDimensions(const glm::vec3& value) {
    if (_isSpotlight) {
        // If we are a spotlight, treat the z value as our radius or length, and
        // recalculate the x/y dimensions to properly encapsulate the spotlight.
        const float length = value.z;
        const float width = length * glm::sin(glm::radians(_cutoff));
        EntityItem::setUnscaledDimensions(glm::vec3(width, width, length));
    } else {
        float maxDimension = glm::compMax(value);
        EntityItem::setUnscaledDimensions(glm::vec3(maxDimension, maxDimension, maxDimension));
    }
}

void LightEntityItem::locationChanged(bool tellPhysics) {
    EntityItem::locationChanged(tellPhysics);
    withWriteLock([&] {
        _lightPropertiesChanged = true;
    });
}

void LightEntityItem::dimensionsChanged() {
    EntityItem::dimensionsChanged();
    withWriteLock([&] {
        _lightPropertiesChanged = true;
    });
}


EntityItemProperties LightEntityItem::getProperties(EntityPropertyFlags desiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties); // get the properties from our base class

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(isSpotlight, getIsSpotlight);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(color, getXColor);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(intensity, getIntensity);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(exponent, getExponent);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(cutoff, getCutoff);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(falloffRadius, getFalloffRadius);

    return properties;
}

void LightEntityItem::setFalloffRadius(float value) {
    value = glm::max(value, 0.0f);
    if (value == getFalloffRadius()) {
        return;
    }
    withWriteLock([&] {
        _falloffRadius = value;
        _lightPropertiesChanged = true;
    });
}

void LightEntityItem::setIsSpotlight(bool value) {
    if (value == getIsSpotlight()) {
        return;
    }

    glm::vec3 dimensions = getScaledDimensions();
    glm::vec3 newDimensions;
    if (value) {
        const float length = dimensions.z;
        const float width = length * glm::sin(glm::radians(getCutoff()));
        newDimensions = glm::vec3(width, width, length);
    } else {
        newDimensions = glm::vec3(glm::compMax(dimensions));
    }

    withWriteLock([&] {
        _isSpotlight = value;
        _lightPropertiesChanged = true;
    });
    setScaledDimensions(newDimensions);
}

void LightEntityItem::setCutoff(float value) {
    value = glm::clamp(value, 0.0f, 90.0f);
    if (value == getCutoff()) {
        return;
    }

    withWriteLock([&] {
        _cutoff = value;
    });

    if (getIsSpotlight()) {
        // If we are a spotlight, adjusting the cutoff will affect the area we encapsulate,
        // so update the dimensions to reflect this.
        const float length = getScaledDimensions().z;
        const float width = length * glm::sin(glm::radians(_cutoff));
        setScaledDimensions(glm::vec3(width, width, length));
    }
    
    withWriteLock([&] {
        _lightPropertiesChanged = true;
    });
}

bool LightEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class
    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qCDebug(entities) << "LightEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEdited(properties.getLastEdited());
    }
    return somethingChanged;
}

bool LightEntityItem::setSubClassProperties(const EntityItemProperties& properties) {
    bool somethingChanged = EntityItem::setSubClassProperties(properties); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(isSpotlight, setIsSpotlight);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(color, setColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(intensity, setIntensity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(exponent, setExponent);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(cutoff, setCutoff);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(falloffRadius, setFalloffRadius);

    return somethingChanged;
}


int LightEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_IS_SPOTLIGHT, bool, setIsSpotlight);
    READ_ENTITY_PROPERTY(PROP_COLOR, rgbColor, setColor);
    READ_ENTITY_PROPERTY(PROP_INTENSITY, float, setIntensity);
    READ_ENTITY_PROPERTY(PROP_EXPONENT, float, setExponent);
    READ_ENTITY_PROPERTY(PROP_CUTOFF, float, setCutoff);
    READ_ENTITY_PROPERTY(PROP_FALLOFF_RADIUS, float, setFalloffRadius);

    return bytesRead;
}


EntityPropertyFlags LightEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_IS_SPOTLIGHT;
    requestedProperties += PROP_COLOR;
    requestedProperties += PROP_INTENSITY;
    requestedProperties += PROP_EXPONENT;
    requestedProperties += PROP_CUTOFF;
    requestedProperties += PROP_FALLOFF_RADIUS;
    return requestedProperties;
}

void LightEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                    EntityTreeElementExtraEncodeDataPointer modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const { 

    bool successPropertyFits = true;
    APPEND_ENTITY_PROPERTY(PROP_IS_SPOTLIGHT, getIsSpotlight());
    APPEND_ENTITY_PROPERTY(PROP_COLOR, getColor());
    APPEND_ENTITY_PROPERTY(PROP_INTENSITY, getIntensity());
    APPEND_ENTITY_PROPERTY(PROP_EXPONENT, getExponent());
    APPEND_ENTITY_PROPERTY(PROP_CUTOFF, getCutoff());
    APPEND_ENTITY_PROPERTY(PROP_FALLOFF_RADIUS, getFalloffRadius());
}

const rgbColor& LightEntityItem::getColor() const { 
    return _color; 
}

xColor LightEntityItem::getXColor() const {
    xColor color = { _color[RED_INDEX], _color[GREEN_INDEX], _color[BLUE_INDEX] }; return color;
}

void LightEntityItem::setColor(const rgbColor& value) { 
    withWriteLock([&] {
        memcpy(_color, value, sizeof(_color));
        _lightPropertiesChanged = true;
    });
}

void LightEntityItem::setColor(const xColor& value) {
    withWriteLock([&] {
        _color[RED_INDEX] = value.red;
        _color[GREEN_INDEX] = value.green;
        _color[BLUE_INDEX] = value.blue;
        _lightPropertiesChanged = true;
    });
}

bool LightEntityItem::getIsSpotlight() const {
    bool result;
    withReadLock([&] {
        result = _isSpotlight;
    });
    return result;
}

float LightEntityItem::getIntensity() const { 
    float result;
    withReadLock([&] {
        result = _intensity;
    });
    return result;
}

void LightEntityItem::setIntensity(float value) {
    withWriteLock([&] {
        _intensity = value;
        _lightPropertiesChanged = true;
    });
}

float LightEntityItem::getFalloffRadius() const { 
    float result;
    withReadLock([&] {
        result = _falloffRadius;
    });
    return result;
}

float LightEntityItem::getExponent() const { 
    float result;
    withReadLock([&] {
        result = _exponent;
    });
    return result;
}

void LightEntityItem::setExponent(float value) {
    withWriteLock([&] {
        _exponent = value;
        _lightPropertiesChanged = true;
    });
}

float LightEntityItem::getCutoff() const { 
    float result;
    withReadLock([&] {
        result = _cutoff;
    });
    return result;
}

void LightEntityItem::resetLightPropertiesChanged() {
    withWriteLock([&] { _lightPropertiesChanged = false; });
}

bool LightEntityItem::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                        OctreeElementPointer& element, float& distance,
                        BoxFace& face, glm::vec3& surfaceNormal,
                        QVariantMap& extraInfo, bool precisionPicking) const {

    // TODO: consider if this is really what we want to do. We've made it so that "lights are pickable" is a global state
    // this is probably reasonable since there's typically only one tree you'd be picking on at a time. Technically we could
    // be on the clipboard and someone might be trying to use the ray intersection API there. Anyway... if you ever try to
    // do ray intersection testing off of trees other than the main tree of the main entity renderer, then we'll need to
    // fix this mechanism.
    return _lightsArePickable;
}

bool LightEntityItem::findDetailedParabolaIntersection(const glm::vec3& origin, const glm::vec3& velocity,
                        const glm::vec3& acceleration, OctreeElementPointer& element, float& parabolicDistance,
                        BoxFace& face, glm::vec3& surfaceNormal,
                        QVariantMap& extraInfo, bool precisionPicking) const {
    // TODO: consider if this is really what we want to do. We've made it so that "lights are pickable" is a global state
    // this is probably reasonable since there's typically only one tree you'd be picking on at a time. Technically we could
    // be on the clipboard and someone might be trying to use the parabola intersection API there. Anyway... if you ever try to
    // do parabola intersection testing off of trees other than the main tree of the main entity renderer, then we'll need to
    // fix this mechanism.
    return _lightsArePickable;
}
