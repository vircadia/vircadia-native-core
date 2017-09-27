//
//  HazePropertyGroup.h
//  libraries/entities/src
//
//  Created by Nissim hadar on 9/21/17.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <OctreePacketData.h>

#include "HazePropertyGroup.h"
#include "EntityItemProperties.h"
#include "EntityItemPropertiesMacros.h"

const float HazePropertyGroup::DEFAULT_HAZE_RANGE = 1000.0f;
const float HazePropertyGroup::DEFAULT_HAZE_ALTITUDE = 200.0f;

void HazePropertyGroup::copyToScriptValue(const EntityPropertyFlags& desiredProperties, QScriptValue& properties, QScriptEngine* engine, bool skipDefaults, EntityItemProperties& defaultEntityProperties) const {
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_HAZE_HAZE_RANGE, Haze, haze, HazeRange, hazeRange);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_HAZE_HAZE_ALTITUDE, Haze, haze, HazeAltitude, hazeAltitude);
}

void HazePropertyGroup::copyFromScriptValue(const QScriptValue& object, bool& _defaultSettings) {
    COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(hazeHazeRange, float, setHazeRange, getHazeRange);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(hazeHazeAltitude, float, setHazeAltitude, getHazeAltitude);

    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(haze, hazeRange, float, setHazeRange);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(haze, hazeAltitude, float, setHazeAltitude);
}

void HazePropertyGroup::merge(const HazePropertyGroup& other) {
    COPY_PROPERTY_IF_CHANGED(hazeRange);
    COPY_PROPERTY_IF_CHANGED(hazeAltitude);
}


void HazePropertyGroup::debugDump() const {
    qCDebug(entities) << "   HazePropertyGroup: ---------------------------------------------";
    qCDebug(entities) << "            _hazeRange:" << _hazeRange;
    qCDebug(entities) << "            _hazeAltitude:" << _hazeAltitude;
}

void HazePropertyGroup::listChangedProperties(QList<QString>& out) {
    if (hazeRangeChanged()) {
        out << "haze-range";
    }
    if (hazeAltitudeChanged()) {
        out << "haze-altitude";
    }
}

bool HazePropertyGroup::appendToEditPacket(OctreePacketData* packetData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_HAZE_HAZE_RANGE, getHazeRange());
    APPEND_ENTITY_PROPERTY(PROP_HAZE_HAZE_ALTITUDE, getHazeAltitude());

    return true;
}


bool HazePropertyGroup::decodeFromEditPacket(EntityPropertyFlags& propertyFlags, const unsigned char*& dataAt , int& processedBytes) {

    int bytesRead = 0;
    bool overwriteLocalData = true;
    bool somethingChanged = false;

    READ_ENTITY_PROPERTY(PROP_HAZE_HAZE_RANGE, float, setHazeRange);
    READ_ENTITY_PROPERTY(PROP_HAZE_HAZE_ALTITUDE, float, setHazeAltitude);

    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_HAZE_HAZE_RANGE, HazeRange);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_HAZE_HAZE_ALTITUDE, HazeAltitude);

    processedBytes += bytesRead;

    Q_UNUSED(somethingChanged);

    return true;
}

void HazePropertyGroup::markAllChanged() {
    _hazeRangeChanged = true;
    _hazeAltitudeChanged = true;
}

EntityPropertyFlags HazePropertyGroup::getChangedProperties() const {
    EntityPropertyFlags changedProperties;

    CHECK_PROPERTY_CHANGE(PROP_HAZE_HAZE_RANGE, hazeRange);
    CHECK_PROPERTY_CHANGE(PROP_HAZE_HAZE_ALTITUDE, hazeAltitude);

    return changedProperties;
}

void HazePropertyGroup::getProperties(EntityItemProperties& properties) const {
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Haze, HazeRange, getHazeRange);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Haze, HazeAltitude, getHazeAltitude);
}

bool HazePropertyGroup::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;

    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Haze, HazeRange, hazeRange, setHazeRange);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Haze, HazeAltitude, hazeAltitude, setHazeAltitude);

    return somethingChanged;
}

EntityPropertyFlags HazePropertyGroup::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties;

    requestedProperties += PROP_HAZE_HAZE_RANGE;
    requestedProperties += PROP_HAZE_HAZE_ALTITUDE;

    return requestedProperties;
}
    
void HazePropertyGroup::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                EntityTreeElementExtraEncodeDataPointer entityTreeElementExtraEncodeData,
                                EntityPropertyFlags& requestedProperties,
                                EntityPropertyFlags& propertyFlags,
                                EntityPropertyFlags& propertiesDidntFit,
                                int& propertyCount, 
                                OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_HAZE_HAZE_RANGE, getHazeRange());
    APPEND_ENTITY_PROPERTY(PROP_HAZE_HAZE_ALTITUDE, getHazeAltitude());
}

int HazePropertyGroup::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                            ReadBitstreamToTreeParams& args,
                                            EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                            bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_HAZE_HAZE_RANGE, float, setHazeRange);
    READ_ENTITY_PROPERTY(PROP_HAZE_HAZE_ALTITUDE, float, setHazeAltitude);

    return bytesRead;
}
