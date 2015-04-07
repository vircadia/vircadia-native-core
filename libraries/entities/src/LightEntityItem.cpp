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


#include <QDebug>

#include <ByteCountCoding.h>

#include "EntityItemID.h"
#include "EntityTree.h"
#include "EntityTreeElement.h"
#include "EntitiesLogging.h"
#include "LightEntityItem.h"

bool LightEntityItem::_lightsArePickable = false;

EntityItem* LightEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return new LightEntityItem(entityID, properties);
}

// our non-pure virtual subclass for now...
LightEntityItem::LightEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID, properties) 
{
    _type = EntityTypes::Light;
    
    // default property values
    _color[RED_INDEX] = _color[GREEN_INDEX] = _color[BLUE_INDEX] = 0;
    _intensity = 1.0f;
    _exponent = 0.0f;
    _cutoff = PI;

    setProperties(properties);
}

void LightEntityItem::setDimensions(const glm::vec3& value) {
    if (_isSpotlight) {
        // If we are a spotlight, treat the z value as our radius or length, and
        // recalculate the x/y dimensions to properly encapsulate the spotlight.
        const float length = value.z;
        const float width = length * glm::tan(glm::radians(_cutoff));
        _dimensions = glm::vec3(width, width, length);
    } else {
        float maxDimension = glm::max(value.x, value.y, value.z);
        _dimensions = glm::vec3(maxDimension, maxDimension, maxDimension);
    }
}


EntityItemProperties LightEntityItem::getProperties() const {
    EntityItemProperties properties = EntityItem::getProperties(); // get the properties from our base class

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(isSpotlight, getIsSpotlight);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(color, getXColor);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(intensity, getIntensity);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(exponent, getExponent);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(cutoff, getCutoff);

    return properties;
}

void LightEntityItem::setIsSpotlight(bool value) {
    if (value != _isSpotlight) {
        _isSpotlight = value;

        if (_isSpotlight) {
            const float length = _dimensions.z;
            const float width = length * glm::tan(glm::radians(_cutoff));
            _dimensions = glm::vec3(width, width, length);
        } else {
            float maxDimension = glm::max(_dimensions.x, _dimensions.y, _dimensions.z);
            _dimensions = glm::vec3(maxDimension, maxDimension, maxDimension);
        }
    }
}

void LightEntityItem::setCutoff(float value) {
    _cutoff = glm::clamp(value, 0.0f, 90.0f);

    if (_isSpotlight) {
        // If we are a spotlight, adjusting the cutoff will affect the area we encapsulate,
        // so update the dimensions to reflect this.
        const float length = _dimensions.z;
        const float width = length * glm::tan(glm::radians(_cutoff));
        _dimensions = glm::vec3(width, width, length);
    }
}

bool LightEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(isSpotlight, setIsSpotlight);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(color, setColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(intensity, setIntensity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(exponent, setExponent);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(cutoff, setCutoff);

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

int LightEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    if (args.bitstreamVersion < VERSION_ENTITIES_LIGHT_HAS_INTENSITY_AND_COLOR_PROPERTIES) {
        rgbColor ignoredColor;
        float ignoredAttenuation;

        READ_ENTITY_PROPERTY(PROP_IS_SPOTLIGHT, bool, _isSpotlight);

        // _diffuseColor has been renamed to _color
        READ_ENTITY_PROPERTY_COLOR(PROP_DIFFUSE_COLOR_UNUSED, _color);

        // Ambient and specular color are from an older format and are no longer supported.
        // Their values will be ignored.
        READ_ENTITY_PROPERTY_COLOR(PROP_AMBIENT_COLOR_UNUSED, ignoredColor);
        READ_ENTITY_PROPERTY_COLOR(PROP_SPECULAR_COLOR_UNUSED, ignoredColor);

        // _constantAttenuation has been renamed to _intensity
        READ_ENTITY_PROPERTY(PROP_INTENSITY, float, _intensity);

        // Linear and quadratic attenuation are from an older format and are no longer supported.
        // Their values will be ignored.
        READ_ENTITY_PROPERTY(PROP_LINEAR_ATTENUATION_UNUSED, float, ignoredAttenuation);
        READ_ENTITY_PROPERTY(PROP_QUADRATIC_ATTENUATION_UNUSED, float, ignoredAttenuation);

        READ_ENTITY_PROPERTY(PROP_EXPONENT, float, _exponent);
        READ_ENTITY_PROPERTY(PROP_CUTOFF, float, _cutoff);

        (void) ignoredAttenuation; // suppress compiler warning
    } else {
        READ_ENTITY_PROPERTY(PROP_IS_SPOTLIGHT, bool, _isSpotlight);
        READ_ENTITY_PROPERTY_COLOR(PROP_COLOR, _color);
        READ_ENTITY_PROPERTY(PROP_INTENSITY, float, _intensity);
        READ_ENTITY_PROPERTY(PROP_EXPONENT, float, _exponent);
        READ_ENTITY_PROPERTY(PROP_CUTOFF, float, _cutoff);
    }

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags LightEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_IS_SPOTLIGHT;
    requestedProperties += PROP_COLOR;
    requestedProperties += PROP_INTENSITY;
    requestedProperties += PROP_EXPONENT;
    requestedProperties += PROP_CUTOFF;
    return requestedProperties;
}

void LightEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                    EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const { 

    bool successPropertyFits = true;
    APPEND_ENTITY_PROPERTY(PROP_IS_SPOTLIGHT, appendValue, getIsSpotlight());
    APPEND_ENTITY_PROPERTY(PROP_COLOR, appendColor, getColor());
    APPEND_ENTITY_PROPERTY(PROP_INTENSITY, appendValue, getIntensity());
    APPEND_ENTITY_PROPERTY(PROP_EXPONENT, appendValue, getExponent());
    APPEND_ENTITY_PROPERTY(PROP_CUTOFF, appendValue, getCutoff());
}
