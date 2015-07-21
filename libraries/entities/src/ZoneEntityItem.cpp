//
//  ZoneEntityItem.cpp
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

#include "ZoneEntityItem.h"
#include "EntityTree.h"
#include "EntitiesLogging.h"
#include "EntityTreeElement.h"

bool ZoneEntityItem::_zonesArePickable = false;
bool ZoneEntityItem::_drawZoneBoundaries = false;

const xColor ZoneEntityItem::DEFAULT_KEYLIGHT_COLOR = { 255, 255, 255 };
const float ZoneEntityItem::DEFAULT_KEYLIGHT_INTENSITY = 1.0f;
const float ZoneEntityItem::DEFAULT_KEYLIGHT_AMBIENT_INTENSITY = 0.5f;
const glm::vec3 ZoneEntityItem::DEFAULT_KEYLIGHT_DIRECTION = { 0.0f, -1.0f, 0.0f };
const ShapeType ZoneEntityItem::DEFAULT_SHAPE_TYPE = SHAPE_TYPE_BOX;
const QString ZoneEntityItem::DEFAULT_COMPOUND_SHAPE_URL = "";

EntityItemPointer ZoneEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return std::make_shared<ZoneEntityItem>(entityID, properties);
}

ZoneEntityItem::ZoneEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID) 
{
    _type = EntityTypes::Zone;

    _keyLightColor[RED_INDEX] = DEFAULT_KEYLIGHT_COLOR.red;
    _keyLightColor[GREEN_INDEX] = DEFAULT_KEYLIGHT_COLOR.green;
    _keyLightColor[BLUE_INDEX] = DEFAULT_KEYLIGHT_COLOR.blue;

    _keyLightIntensity = DEFAULT_KEYLIGHT_INTENSITY;
    _keyLightAmbientIntensity = DEFAULT_KEYLIGHT_AMBIENT_INTENSITY;
    _keyLightDirection = DEFAULT_KEYLIGHT_DIRECTION;
    _shapeType = DEFAULT_SHAPE_TYPE;
    _compoundShapeURL = DEFAULT_COMPOUND_SHAPE_URL;

    _backgroundMode = BACKGROUND_MODE_INHERIT;
    
    setProperties(properties);
}


EnvironmentData ZoneEntityItem::getEnvironmentData() const {
    EnvironmentData result;

    result.setAtmosphereCenter(_atmosphereProperties.getCenter());
    result.setAtmosphereInnerRadius(_atmosphereProperties.getInnerRadius());
    result.setAtmosphereOuterRadius(_atmosphereProperties.getOuterRadius());
    result.setRayleighScattering(_atmosphereProperties.getRayleighScattering());
    result.setMieScattering(_atmosphereProperties.getMieScattering());
    result.setScatteringWavelengths(_atmosphereProperties.getScatteringWavelengths());
    result.setHasStars(_atmosphereProperties.getHasStars());

    // NOTE: The sunLocation and SunBrightness will be overwritten in the EntityTreeRenderer to use the
    // keyLight details from the scene interface
    //result.setSunLocation(1000, 900, 1000));
    //result.setSunBrightness(20.0f);
    
    return result;
}

EntityItemProperties ZoneEntityItem::getProperties() const {
    EntityItemProperties properties = EntityItem::getProperties(); // get the properties from our base class

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(keyLightColor, getKeyLightColor);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(keyLightIntensity, getKeyLightIntensity);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(keyLightAmbientIntensity, getKeyLightAmbientIntensity);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(keyLightDirection, getKeyLightDirection);

    _stageProperties.getProperties(properties);

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(shapeType, getShapeType);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(compoundShapeURL, getCompoundShapeURL);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(backgroundMode, getBackgroundMode);

    _atmosphereProperties.getProperties(properties);
    _skyboxProperties.getProperties(properties);

    return properties;
}

bool ZoneEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;
    somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(keyLightColor, setKeyLightColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(keyLightIntensity, setKeyLightIntensity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(keyLightAmbientIntensity, setKeyLightAmbientIntensity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(keyLightDirection, setKeyLightDirection);

    bool somethingChangedInStage = _stageProperties.setProperties(properties);

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(shapeType, updateShapeType);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(compoundShapeURL, setCompoundShapeURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(backgroundMode, setBackgroundMode);

    bool somethingChangedInAtmosphere = _atmosphereProperties.setProperties(properties);
    bool somethingChangedInSkybox = _skyboxProperties.setProperties(properties);

    somethingChanged = somethingChanged || somethingChangedInStage || somethingChangedInAtmosphere || somethingChangedInSkybox;

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qCDebug(entities) << "ZoneEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                    "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEdited(properties._lastEdited);
    }

    return somethingChanged;
}

int ZoneEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {
    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_KEYLIGHT_COLOR, rgbColor, setKeyLightColor);
    READ_ENTITY_PROPERTY(PROP_KEYLIGHT_INTENSITY, float, setKeyLightIntensity);
    READ_ENTITY_PROPERTY(PROP_KEYLIGHT_AMBIENT_INTENSITY, float, setKeyLightAmbientIntensity);
    READ_ENTITY_PROPERTY(PROP_KEYLIGHT_DIRECTION, glm::vec3, setKeyLightDirection);

    int bytesFromStage = _stageProperties.readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args, 
                                                                               propertyFlags, overwriteLocalData);
                                                                               
    bytesRead += bytesFromStage;
    dataAt += bytesFromStage;

    READ_ENTITY_PROPERTY(PROP_SHAPE_TYPE, ShapeType, updateShapeType);
    READ_ENTITY_PROPERTY(PROP_COMPOUND_SHAPE_URL, QString, setCompoundShapeURL);
    READ_ENTITY_PROPERTY(PROP_BACKGROUND_MODE, BackgroundMode, setBackgroundMode);

    int bytesFromAtmosphere = _atmosphereProperties.readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args, 
                                                                               propertyFlags, overwriteLocalData);
                                                                               
    bytesRead += bytesFromAtmosphere;
    dataAt += bytesFromAtmosphere;

    int bytesFromSkybox = _skyboxProperties.readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args, 
                                                                           propertyFlags, overwriteLocalData);
    bytesRead += bytesFromSkybox;
    dataAt += bytesFromSkybox;

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags ZoneEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);

    requestedProperties += PROP_KEYLIGHT_COLOR;
    requestedProperties += PROP_KEYLIGHT_INTENSITY;
    requestedProperties += PROP_KEYLIGHT_AMBIENT_INTENSITY;
    requestedProperties += PROP_KEYLIGHT_DIRECTION;
    requestedProperties += PROP_SHAPE_TYPE;
    requestedProperties += PROP_COMPOUND_SHAPE_URL;
    requestedProperties += PROP_BACKGROUND_MODE;
    requestedProperties += _stageProperties.getEntityProperties(params);
    requestedProperties += _atmosphereProperties.getEntityProperties(params);
    requestedProperties += _skyboxProperties.getEntityProperties(params);
    
    return requestedProperties;
}

void ZoneEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                    EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const { 

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_COLOR, _keyLightColor);
    APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_INTENSITY, getKeyLightIntensity());
    APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_AMBIENT_INTENSITY, getKeyLightAmbientIntensity());
    APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_DIRECTION, getKeyLightDirection());

    _stageProperties.appendSubclassData(packetData, params, modelTreeElementExtraEncodeData, requestedProperties,
                                    propertyFlags, propertiesDidntFit, propertyCount, appendState);


    APPEND_ENTITY_PROPERTY(PROP_SHAPE_TYPE, (uint32_t)getShapeType());
    APPEND_ENTITY_PROPERTY(PROP_COMPOUND_SHAPE_URL, getCompoundShapeURL());
    APPEND_ENTITY_PROPERTY(PROP_BACKGROUND_MODE, (uint32_t)getBackgroundMode()); // could this be a uint16??
    
    _atmosphereProperties.appendSubclassData(packetData, params, modelTreeElementExtraEncodeData, requestedProperties,
                                    propertyFlags, propertiesDidntFit, propertyCount, appendState);

    _skyboxProperties.appendSubclassData(packetData, params, modelTreeElementExtraEncodeData, requestedProperties,
                                    propertyFlags, propertiesDidntFit, propertyCount, appendState);

}

void ZoneEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "   ZoneEntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "             keyLightColor:" << _keyLightColor[0] << "," << _keyLightColor[1] << "," << _keyLightColor[2];
    qCDebug(entities) << "                  position:" << debugTreeVector(getPosition());
    qCDebug(entities) << "                dimensions:" << debugTreeVector(getDimensions());
    qCDebug(entities) << "             getLastEdited:" << debugTime(getLastEdited(), now);
    qCDebug(entities) << "        _keyLightIntensity:" << _keyLightIntensity;
    qCDebug(entities) << " _keyLightAmbientIntensity:" << _keyLightAmbientIntensity;
    qCDebug(entities) << "        _keyLightDirection:" << _keyLightDirection;
    qCDebug(entities) << "               _backgroundMode:" << EntityItemProperties::getBackgroundModeString(_backgroundMode);

    _stageProperties.debugDump();
    _atmosphereProperties.debugDump();
    _skyboxProperties.debugDump();
}

ShapeType ZoneEntityItem::getShapeType() const {
    // Zones are not allowed to have a SHAPE_TYPE_NONE... they are always at least a SHAPE_TYPE_BOX
    if (_shapeType == SHAPE_TYPE_COMPOUND) {
        return hasCompoundShapeURL() ? SHAPE_TYPE_COMPOUND : DEFAULT_SHAPE_TYPE;
    } else {
        return _shapeType == SHAPE_TYPE_NONE ? DEFAULT_SHAPE_TYPE : _shapeType;
    }
}

void ZoneEntityItem::setCompoundShapeURL(const QString& url) {
    _compoundShapeURL = url;
    if (_compoundShapeURL.isEmpty() && _shapeType == SHAPE_TYPE_COMPOUND) {
        _shapeType = DEFAULT_SHAPE_TYPE;
    }
}

bool ZoneEntityItem::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face,
                         void** intersectedObject, bool precisionPicking) const {

    return _zonesArePickable;
}
