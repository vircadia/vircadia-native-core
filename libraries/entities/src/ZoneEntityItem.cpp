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
const bool ZoneEntityItem::DEFAULT_STAGE_SUN_MODEL_ENABLED = false;
const float ZoneEntityItem::DEFAULT_STAGE_LATITUDE = 37.777f;
const float ZoneEntityItem::DEFAULT_STAGE_LONGITUDE = 122.407f;
const float ZoneEntityItem::DEFAULT_STAGE_ALTITUDE = 0.03f;
const quint16 ZoneEntityItem::DEFAULT_STAGE_DAY = 60;
const float ZoneEntityItem::DEFAULT_STAGE_HOUR = 12.0f;
const ShapeType ZoneEntityItem::DEFAULT_SHAPE_TYPE = SHAPE_TYPE_BOX;
const QString ZoneEntityItem::DEFAULT_COMPOUND_SHAPE_URL = "";

EntityItem* ZoneEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItem* result = new ZoneEntityItem(entityID, properties);
    return result;
}

ZoneEntityItem::ZoneEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID) 
{
    _type = EntityTypes::Zone;
    _created = properties.getCreated();

    _keyLightColor[RED_INDEX] = DEFAULT_KEYLIGHT_COLOR.red;
    _keyLightColor[GREEN_INDEX] = DEFAULT_KEYLIGHT_COLOR.green;
    _keyLightColor[BLUE_INDEX] = DEFAULT_KEYLIGHT_COLOR.blue;

    _keyLightIntensity = DEFAULT_KEYLIGHT_INTENSITY;
    _keyLightAmbientIntensity = DEFAULT_KEYLIGHT_AMBIENT_INTENSITY;
    _keyLightDirection = DEFAULT_KEYLIGHT_DIRECTION;
    _stageSunModelEnabled = DEFAULT_STAGE_SUN_MODEL_ENABLED;
    _stageLatitude = DEFAULT_STAGE_LATITUDE;
    _stageLongitude = DEFAULT_STAGE_LONGITUDE;
    _stageAltitude = DEFAULT_STAGE_ALTITUDE;
    _stageDay = DEFAULT_STAGE_DAY;
    _stageHour = DEFAULT_STAGE_HOUR;
    _shapeType = DEFAULT_SHAPE_TYPE;
    _compoundShapeURL = DEFAULT_COMPOUND_SHAPE_URL;

    _backgroundMode = BACKGROUND_MODE_INHERIT;
    
    setProperties(properties);
}


EnvironmentData ZoneEntityItem::getEnvironmentData() const {
    EnvironmentData result;

    result.setAtmosphereCenter(_atmospherePropeties.getCenter());
    result.setAtmosphereInnerRadius(_atmospherePropeties.getInnerRadius());
    result.setAtmosphereOuterRadius(_atmospherePropeties.getOuterRadius());
    result.setRayleighScattering(_atmospherePropeties.getRayleighScattering());
    result.setMieScattering(_atmospherePropeties.getMieScattering());
    result.setScatteringWavelengths(_atmospherePropeties.getScatteringWavelengths());
    result.setHasStars(_atmospherePropeties.getHasStars());

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
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(stageSunModelEnabled, getStageSunModelEnabled);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(stageLatitude, getStageLatitude);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(stageLongitude, getStageLongitude);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(stageAltitude, getStageAltitude);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(stageDay, getStageDay);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(stageHour, getStageHour);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(shapeType, getShapeType);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(compoundShapeURL, getCompoundShapeURL);

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(backgroundMode, getBackgroundMode);

    _atmospherePropeties.getProperties(properties);

    return properties;
}

bool ZoneEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;
    somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(keyLightColor, setKeyLightColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(keyLightIntensity, setKeyLightIntensity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(keyLightAmbientIntensity, setKeyLightAmbientIntensity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(keyLightDirection, setKeyLightDirection);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(stageSunModelEnabled, setStageSunModelEnabled);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(stageLatitude, setStageLatitude);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(stageLongitude, setStageLongitude);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(stageAltitude, setStageAltitude);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(stageDay, setStageDay);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(stageHour, setStageHour);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(shapeType, updateShapeType);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(compoundShapeURL, setCompoundShapeURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(backgroundMode, setBackgroundMode);

    bool somethingChangedInAtmosphere = _atmospherePropeties.setProperties(properties);

    somethingChanged = somethingChanged || somethingChangedInAtmosphere;

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

    READ_ENTITY_PROPERTY_COLOR(PROP_KEYLIGHT_COLOR, _keyLightColor);
    READ_ENTITY_PROPERTY(PROP_KEYLIGHT_INTENSITY, float, _keyLightIntensity);
    READ_ENTITY_PROPERTY(PROP_KEYLIGHT_AMBIENT_INTENSITY, float, _keyLightAmbientIntensity);
    READ_ENTITY_PROPERTY(PROP_KEYLIGHT_DIRECTION, glm::vec3, _keyLightDirection);
    READ_ENTITY_PROPERTY(PROP_STAGE_SUN_MODEL_ENABLED, bool, _stageSunModelEnabled);
    READ_ENTITY_PROPERTY(PROP_STAGE_LATITUDE, float, _stageLatitude);
    READ_ENTITY_PROPERTY(PROP_STAGE_LONGITUDE, float, _stageLongitude);
    READ_ENTITY_PROPERTY(PROP_STAGE_ALTITUDE, float, _stageAltitude);
    READ_ENTITY_PROPERTY(PROP_STAGE_DAY, quint16, _stageDay);
    READ_ENTITY_PROPERTY(PROP_STAGE_HOUR, float, _stageHour);
    READ_ENTITY_PROPERTY_SETTER(PROP_SHAPE_TYPE, ShapeType, updateShapeType);
    READ_ENTITY_PROPERTY_STRING(PROP_COMPOUND_SHAPE_URL, setCompoundShapeURL);
    READ_ENTITY_PROPERTY_SETTER(PROP_BACKGROUND_MODE, BackgroundMode, setBackgroundMode);
    bytesRead += _atmospherePropeties.readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args, 
                                                                           propertyFlags, overwriteLocalData);

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags ZoneEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);

    requestedProperties += PROP_KEYLIGHT_COLOR;
    requestedProperties += PROP_KEYLIGHT_INTENSITY;
    requestedProperties += PROP_KEYLIGHT_AMBIENT_INTENSITY;
    requestedProperties += PROP_KEYLIGHT_DIRECTION;
    requestedProperties += PROP_STAGE_SUN_MODEL_ENABLED;
    requestedProperties += PROP_STAGE_LATITUDE;
    requestedProperties += PROP_STAGE_LONGITUDE;
    requestedProperties += PROP_STAGE_ALTITUDE;
    requestedProperties += PROP_STAGE_DAY;
    requestedProperties += PROP_STAGE_HOUR;
    requestedProperties += PROP_SHAPE_TYPE;
    requestedProperties += PROP_COMPOUND_SHAPE_URL;
    requestedProperties += PROP_BACKGROUND_MODE;
    requestedProperties += _atmospherePropeties.getEntityProperties(params);
    
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

    APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_COLOR, appendColor, _keyLightColor);
    APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_INTENSITY, appendValue, getKeyLightIntensity());
    APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_AMBIENT_INTENSITY, appendValue, getKeyLightAmbientIntensity());
    APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_DIRECTION, appendValue, getKeyLightDirection());
    APPEND_ENTITY_PROPERTY(PROP_STAGE_SUN_MODEL_ENABLED, appendValue, getStageSunModelEnabled());
    APPEND_ENTITY_PROPERTY(PROP_STAGE_LATITUDE, appendValue, getStageLatitude());
    APPEND_ENTITY_PROPERTY(PROP_STAGE_LONGITUDE, appendValue, getStageLongitude());
    APPEND_ENTITY_PROPERTY(PROP_STAGE_ALTITUDE, appendValue, getStageAltitude());
    APPEND_ENTITY_PROPERTY(PROP_STAGE_DAY, appendValue, getStageDay());
    APPEND_ENTITY_PROPERTY(PROP_STAGE_HOUR, appendValue, getStageHour());
    APPEND_ENTITY_PROPERTY(PROP_SHAPE_TYPE, appendValue, (uint32_t)getShapeType());
    APPEND_ENTITY_PROPERTY(PROP_COMPOUND_SHAPE_URL, appendValue, getCompoundShapeURL());
    APPEND_ENTITY_PROPERTY(PROP_BACKGROUND_MODE, appendValue, (uint32_t)getBackgroundMode()); // could this be a uint16??
    
    _atmospherePropeties.appendSubclassData(packetData, params, modelTreeElementExtraEncodeData, requestedProperties,
                                    propertyFlags, propertiesDidntFit, propertyCount, appendState);

}

void ZoneEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "   ZoneEntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "             keyLightColor:" << _keyLightColor[0] << "," << _keyLightColor[1] << "," << _keyLightColor[2];
    qCDebug(entities) << "                  position:" << debugTreeVector(_position);
    qCDebug(entities) << "                dimensions:" << debugTreeVector(_dimensions);
    qCDebug(entities) << "             getLastEdited:" << debugTime(getLastEdited(), now);
    qCDebug(entities) << "        _keyLightIntensity:" << _keyLightIntensity;
    qCDebug(entities) << " _keyLightAmbientIntensity:" << _keyLightAmbientIntensity;
    qCDebug(entities) << "        _keyLightDirection:" << _keyLightDirection;
    qCDebug(entities) << "     _stageSunModelEnabled:" << _stageSunModelEnabled;
    qCDebug(entities) << "            _stageLatitude:" << _stageLatitude;
    qCDebug(entities) << "           _stageLongitude:" << _stageLongitude;
    qCDebug(entities) << "            _stageAltitude:" << _stageAltitude;
    qCDebug(entities) << "                 _stageDay:" << _stageDay;
    qCDebug(entities) << "                _stageHour:" << _stageHour;
    qCDebug(entities) << "               _backgroundMode:" << EntityItemProperties::getBackgroundModeString(_backgroundMode);

    _atmospherePropeties.debugDump();
}

ShapeType ZoneEntityItem::getShapeType() const {
    // Zones are not allowed to have a SHAPE_TYPE_NONE... they are always at least a SHAPE_TYPE_BOX
    if (_shapeType == SHAPE_TYPE_COMPOUND) {
        return hasCompoundShapeURL() ? SHAPE_TYPE_COMPOUND : SHAPE_TYPE_BOX;
    } else {
        return _shapeType == SHAPE_TYPE_NONE ? SHAPE_TYPE_BOX : _shapeType;
    }
}

void ZoneEntityItem::setCompoundShapeURL(const QString& url) {
    _compoundShapeURL = url;
    if (!_compoundShapeURL.isEmpty()) {
        updateShapeType(SHAPE_TYPE_COMPOUND);
    } else if (_shapeType == SHAPE_TYPE_COMPOUND) {
        _shapeType = DEFAULT_SHAPE_TYPE;
    }
}

bool ZoneEntityItem::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face,
                         void** intersectedObject, bool precisionPicking) const {

    return _zonesArePickable;
}
