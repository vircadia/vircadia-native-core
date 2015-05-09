//
//  StagePropertyGroup.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <OctreePacketData.h>

#include "StagePropertyGroup.h"
#include "EntityItemProperties.h"
#include "EntityItemPropertiesMacros.h"

const bool StagePropertyGroup::DEFAULT_STAGE_SUN_MODEL_ENABLED = false;
const float StagePropertyGroup::DEFAULT_STAGE_LATITUDE = 37.777f;
const float StagePropertyGroup::DEFAULT_STAGE_LONGITUDE = 122.407f;
const float StagePropertyGroup::DEFAULT_STAGE_ALTITUDE = 0.03f;
const quint16 StagePropertyGroup::DEFAULT_STAGE_DAY = 60;
const float StagePropertyGroup::DEFAULT_STAGE_HOUR = 12.0f;

StagePropertyGroup::StagePropertyGroup() {
    _sunModelEnabled = DEFAULT_STAGE_SUN_MODEL_ENABLED;
    _latitude = DEFAULT_STAGE_LATITUDE;
    _longitude = DEFAULT_STAGE_LONGITUDE;
    _altitude = DEFAULT_STAGE_ALTITUDE;
    _day = DEFAULT_STAGE_DAY;
    _hour = DEFAULT_STAGE_HOUR;
}

void StagePropertyGroup::copyToScriptValue(QScriptValue& properties, QScriptEngine* engine, bool skipDefaults, EntityItemProperties& defaultEntityProperties) const {
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(Stage, stage, SunModelEnabled, sunModelEnabled);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(Stage, stage, Latitude, latitude);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(Stage, stage, Longitude, longitude);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(Stage, stage, Altitude, altitude);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(Stage, stage, Day, day);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(Stage, stage, Hour, hour);
}

void StagePropertyGroup::copyFromScriptValue(const QScriptValue& object, bool& _defaultSettings) {
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE_BOOL(stage, sunModelEnabled, setSunModelEnabled);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(stage, latitude, setLatitude);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(stage, longitude, setLongitude);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(stage, altitude, setAltitude);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE_UINT16(stage, day, setDay);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(stage, hour, setHour);
}

void StagePropertyGroup::debugDump() const {
    qDebug() << "   StagePropertyGroup: ---------------------------------------------";
    qDebug() << "     _sunModelEnabled:" << _sunModelEnabled;
    qDebug() << "            _latitude:" << _latitude;
    qDebug() << "           _longitude:" << _longitude;
    qDebug() << "            _altitude:" << _altitude;
    qDebug() << "                 _day:" << _day;
    qDebug() << "                _hour:" << _hour;

}

bool StagePropertyGroup::appentToEditPacket(OctreePacketData* packetData,                                     
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_STAGE_SUN_MODEL_ENABLED, appendValue, getSunModelEnabled());
    APPEND_ENTITY_PROPERTY(PROP_STAGE_LATITUDE, appendValue, getLatitude());
    APPEND_ENTITY_PROPERTY(PROP_STAGE_LONGITUDE, appendValue, getLongitude());
    APPEND_ENTITY_PROPERTY(PROP_STAGE_ALTITUDE, appendValue, getAltitude());
    APPEND_ENTITY_PROPERTY(PROP_STAGE_DAY, appendValue, getDay());
    APPEND_ENTITY_PROPERTY(PROP_STAGE_HOUR, appendValue, getHour());

    return true;
}


bool StagePropertyGroup::decodeFromEditPacket(EntityPropertyFlags& propertyFlags, const unsigned char*& dataAt , int& processedBytes) {

    int bytesRead = 0;
    bool overwriteLocalData = true;

    READ_ENTITY_PROPERTY(PROP_STAGE_SUN_MODEL_ENABLED, bool, _sunModelEnabled);
    READ_ENTITY_PROPERTY(PROP_STAGE_LATITUDE, float, _latitude);
    READ_ENTITY_PROPERTY(PROP_STAGE_LONGITUDE, float, _longitude);
    READ_ENTITY_PROPERTY(PROP_STAGE_ALTITUDE, float, _altitude);
    READ_ENTITY_PROPERTY(PROP_STAGE_DAY, quint16, _day);
    READ_ENTITY_PROPERTY(PROP_STAGE_HOUR, float, _hour);

    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_STAGE_SUN_MODEL_ENABLED, SunModelEnabled);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_STAGE_LATITUDE, Latitude);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_STAGE_LONGITUDE, Longitude);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_STAGE_ALTITUDE, Altitude);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_STAGE_DAY, Day);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_STAGE_HOUR, Hour);

    processedBytes += bytesRead;

    return true;
}

void StagePropertyGroup::markAllChanged() {
    _sunModelEnabledChanged = true;
    _latitudeChanged = true;
    _longitudeChanged = true;
    _altitudeChanged = true;
    _dayChanged = true;
    _hourChanged = true;
}

EntityPropertyFlags StagePropertyGroup::getChangedProperties() const {
    EntityPropertyFlags changedProperties;
    
    CHECK_PROPERTY_CHANGE(PROP_STAGE_SUN_MODEL_ENABLED, sunModelEnabled);
    CHECK_PROPERTY_CHANGE(PROP_STAGE_LATITUDE, latitude);
    CHECK_PROPERTY_CHANGE(PROP_STAGE_LONGITUDE, longitude);
    CHECK_PROPERTY_CHANGE(PROP_STAGE_ALTITUDE, altitude);
    CHECK_PROPERTY_CHANGE(PROP_STAGE_DAY, day);
    CHECK_PROPERTY_CHANGE(PROP_STAGE_HOUR, hour);

    return changedProperties;
}

void StagePropertyGroup::getProperties(EntityItemProperties& properties) const {
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Stage, SunModelEnabled, getSunModelEnabled);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Stage, Latitude, getLatitude);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Stage, Longitude, getLongitude);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Stage, Altitude, getAltitude);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Stage, Day, getDay);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Stage, Hour, getHour);
}

bool StagePropertyGroup::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;

    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Stage, SunModelEnabled, sunModelEnabled, setSunModelEnabled);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Stage, Latitude, latitude, setLatitude);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Stage, Longitude, longitude, setLongitude);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Stage, Altitude, altitude, setAltitude);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Stage, Day, day, setDay);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Stage, Hour, hour, setHour);

    return somethingChanged;
}

EntityPropertyFlags StagePropertyGroup::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties;

    requestedProperties += PROP_STAGE_SUN_MODEL_ENABLED;
    requestedProperties += PROP_STAGE_LATITUDE;
    requestedProperties += PROP_STAGE_LONGITUDE;
    requestedProperties += PROP_STAGE_ALTITUDE;
    requestedProperties += PROP_STAGE_DAY;
    requestedProperties += PROP_STAGE_HOUR;

    return requestedProperties;
}
    
void StagePropertyGroup::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData,
                                EntityPropertyFlags& requestedProperties,
                                EntityPropertyFlags& propertyFlags,
                                EntityPropertyFlags& propertiesDidntFit,
                                int& propertyCount, 
                                OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_STAGE_SUN_MODEL_ENABLED, appendValue, getSunModelEnabled());
    APPEND_ENTITY_PROPERTY(PROP_STAGE_LATITUDE, appendValue, getLatitude());
    APPEND_ENTITY_PROPERTY(PROP_STAGE_LONGITUDE, appendValue, getLongitude());
    APPEND_ENTITY_PROPERTY(PROP_STAGE_ALTITUDE, appendValue, getAltitude());
    APPEND_ENTITY_PROPERTY(PROP_STAGE_DAY, appendValue, getDay());
    APPEND_ENTITY_PROPERTY(PROP_STAGE_HOUR, appendValue, getHour());

}

int StagePropertyGroup::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                            ReadBitstreamToTreeParams& args,
                                            EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_STAGE_SUN_MODEL_ENABLED, bool, _sunModelEnabled);
    READ_ENTITY_PROPERTY(PROP_STAGE_LATITUDE, float, _latitude);
    READ_ENTITY_PROPERTY(PROP_STAGE_LONGITUDE, float, _longitude);
    READ_ENTITY_PROPERTY(PROP_STAGE_ALTITUDE, float, _altitude);
    READ_ENTITY_PROPERTY(PROP_STAGE_DAY, quint16, _day);
    READ_ENTITY_PROPERTY(PROP_STAGE_HOUR, float, _hour);

    return bytesRead;
}
