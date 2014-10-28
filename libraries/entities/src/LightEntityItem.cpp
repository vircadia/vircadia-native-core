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

#include "EntityTree.h"
#include "EntityTreeElement.h"
#include "LightEntityItem.h"


EntityItem* LightEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return new LightEntityItem(entityID, properties);
}

// our non-pure virtual subclass for now...
LightEntityItem::LightEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID, properties) 
{
    _type = EntityTypes::Light;
    
    // default property values
    const quint8 MAX_COLOR = 255;
    _ambientColor[RED_INDEX] = _ambientColor[GREEN_INDEX] = _ambientColor[BLUE_INDEX] = 0;
    _diffuseColor[RED_INDEX] = _diffuseColor[GREEN_INDEX] = _diffuseColor[BLUE_INDEX] = MAX_COLOR;
    _specularColor[RED_INDEX] = _specularColor[GREEN_INDEX] = _specularColor[BLUE_INDEX] = MAX_COLOR;
    _constantAttenuation = 1.0f;
    _linearAttenuation = 0.0f; 
    _quadraticAttenuation = 0.0f;
    _exponent = 0.0f;
    _cutoff = PI;

    setProperties(properties, true);

    // a light is not collide-able so we make it's shape be a tiny sphere at origin
    _emptyShape.setTranslation(glm::vec3(0.0f, 0.0f, 0.0f));
    _emptyShape.setRadius(0.0f);
}

EntityItemProperties LightEntityItem::getProperties() const {
    EntityItemProperties properties = EntityItem::getProperties(); // get the properties from our base class

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(isSpotlight, getIsSpotlight);
    properties.setDiffuseColor(getDiffuseXColor()); // special case
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(ambientColor, getAmbientXColor);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(specularColor, getSpecularXColor);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(constantAttenuation, getConstantAttenuation);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(linearAttenuation, getLinearAttenuation);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(quadraticAttenuation, getQuadraticAttenuation);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(exponent, getExponent);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(cutoff, getCutoff);

    return properties;
}

bool LightEntityItem::setProperties(const EntityItemProperties& properties, bool forceCopy) {
    bool somethingChanged = EntityItem::setProperties(properties, forceCopy); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(isSpotlight, setIsSpotlight);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(diffuseColor, setDiffuseColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(ambientColor, setAmbientColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(specularColor, setSpecularColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(isSpotlight, setIsSpotlight);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(constantAttenuation, setConstantAttenuation);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(linearAttenuation, setLinearAttenuation);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(quadraticAttenuation, setQuadraticAttenuation);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(exponent, setExponent);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(cutoff, setCutoff);


    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qDebug() << "LightEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
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

    READ_ENTITY_PROPERTY(PROP_IS_SPOTLIGHT, bool, _isSpotlight);
    READ_ENTITY_PROPERTY_COLOR(PROP_DIFFUSE_COLOR, _diffuseColor);
    READ_ENTITY_PROPERTY_COLOR(PROP_AMBIENT_COLOR, _ambientColor);
    READ_ENTITY_PROPERTY_COLOR(PROP_SPECULAR_COLOR, _specularColor);
    READ_ENTITY_PROPERTY(PROP_CONSTANT_ATTENUATION, float, _constantAttenuation);
    READ_ENTITY_PROPERTY(PROP_LINEAR_ATTENUATION, float, _linearAttenuation);
    READ_ENTITY_PROPERTY(PROP_QUADRATIC_ATTENUATION, float, _quadraticAttenuation);
    READ_ENTITY_PROPERTY(PROP_EXPONENT, float, _exponent);
    READ_ENTITY_PROPERTY(PROP_CUTOFF, float, _cutoff);

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags LightEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_IS_SPOTLIGHT;
    requestedProperties += PROP_DIFFUSE_COLOR;
    requestedProperties += PROP_AMBIENT_COLOR;
    requestedProperties += PROP_SPECULAR_COLOR;
    requestedProperties += PROP_CONSTANT_ATTENUATION;
    requestedProperties += PROP_LINEAR_ATTENUATION;
    requestedProperties += PROP_QUADRATIC_ATTENUATION;
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
    APPEND_ENTITY_PROPERTY(PROP_DIFFUSE_COLOR, appendColor, getDiffuseColor());
    APPEND_ENTITY_PROPERTY(PROP_AMBIENT_COLOR, appendColor, getAmbientColor());
    APPEND_ENTITY_PROPERTY(PROP_SPECULAR_COLOR, appendColor, getSpecularColor());
    APPEND_ENTITY_PROPERTY(PROP_CONSTANT_ATTENUATION, appendValue, getConstantAttenuation());
    APPEND_ENTITY_PROPERTY(PROP_LINEAR_ATTENUATION, appendValue, getLinearAttenuation());
    APPEND_ENTITY_PROPERTY(PROP_QUADRATIC_ATTENUATION, appendValue, getQuadraticAttenuation());
    APPEND_ENTITY_PROPERTY(PROP_EXPONENT, appendValue, getExponent());
    APPEND_ENTITY_PROPERTY(PROP_CUTOFF, appendValue, getCutoff());
}
