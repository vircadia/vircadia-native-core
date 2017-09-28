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

const uint8_t HazePropertyGroup::DEFAULT_HAZE_MODE{ 0 };

const float HazePropertyGroup::DEFAULT_HAZE_RANGE{ 1000.0f };
const xColor HazePropertyGroup::DEFAULT_HAZE_BLEND_IN_COLOR{ 128, 154, 179 };    // Bluish
const xColor HazePropertyGroup::DEFAULT_HAZE_BLEND_OUT_COLOR{ 255, 229, 179 };   // Yellowish
const float HazePropertyGroup::DEFAULT_LIGHT_BLEND_ANGLE{ 20.0 };

const float HazePropertyGroup::DEFAULT_HAZE_ALTITUDE{ 200.0f };
const float HazePropertyGroup::DEFAULT_HAZE_BASE_REF{ 0.0f };

const float HazePropertyGroup::DEFAULT_HAZE_BACKGROUND_BLEND{ 0.0f };

const float HazePropertyGroup::DEFAULT_HAZE_KEYLIGHT_RANGE{ 1000.0 };
const float HazePropertyGroup::DEFAULT_HAZE_KEYLIGHT_ALTITUDE{ 200.0f };

void HazePropertyGroup::copyToScriptValue(const EntityPropertyFlags& desiredProperties, QScriptValue& properties, QScriptEngine* engine, bool skipDefaults, EntityItemProperties& defaultEntityProperties) const {
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_HAZE_MODE, Haze, haze, HazeMode, hazeMode);

    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_HAZE_BLEND_IN_COLOR, Haze, haze, HazeRange, hazeRange);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_HAZE_BLEND_IN_COLOR, Haze, haze, HazeBlendInColor, hazeBlendIncolor);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_HAZE_BLEND_OUT_COLOR, Haze, haze, HazeBlendOutColor, hazeBlendOutcolor);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_HAZE_LIGHT_BLEND_ANGLE, Haze, haze, HazeLightBlendAngle, hazeLightBlendAngle);

    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_HAZE_ALTITUDE, Haze, haze, HazeAltitude, hazeAltitude);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_HAZE_BASE_REF, Haze, haze, HazeBaseRef, hazeBaseRef);

    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_HAZE_BACKGROUND_BLEND, Haze, haze, HazeBackgroundBlend, hazeBackgroundBlend);

    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_HAZE_KEYLIGHT_RANGE, Haze, haze, HazeKeyLightRange, hazeKeyLightRange);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_HAZE_KEYLIGHT_ALTITUDE, Haze, haze, HazeKeyLightAltitude, hazeKeyLightAltitude);
}

void HazePropertyGroup::copyFromScriptValue(const QScriptValue& object, bool& _defaultSettings) {
    COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(hazeMode, uint8_t, setHazeMode, getHazeMode);

    COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(hazeHazeRange, float, setHazeRange, getHazeRange);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(haze, hazeBlendInColor, xColor, setHazeBlendInColor);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(haze, hazeBlendOutColor, xColor, setHazeBlendOutColor);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(hazeHazeLightBlendAngle, float, setHazeLightBlendAngle, getHazeLightBlendAngle);

    COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(hazeHazeAltitude, float, setHazeAltitude, getHazeAltitude);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(hazeHazeBaseRef, float, setHazeBaseRef, getHazeBaseRef);

    COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(hazeHazeBackgroundBlend, float, setHazeBackgroundBlend, getHazeBackgroundBlend);

    COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(hazeHazeKeyLightRange, float, setHazeKeyLightRange, getHazeKeyLightRange);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(hazeHazeKeyLightAltitude, float, setHazeKeyLightAltitude, getHazeKeyLightAltitude);
}

void HazePropertyGroup::merge(const HazePropertyGroup& other) {
    COPY_PROPERTY_IF_CHANGED(hazeMode);

    COPY_PROPERTY_IF_CHANGED(hazeRange);
    COPY_PROPERTY_IF_CHANGED(hazeBlendInColor);
    COPY_PROPERTY_IF_CHANGED(hazeBlendOutColor);
    COPY_PROPERTY_IF_CHANGED(hazeLightBlendAngle);

    COPY_PROPERTY_IF_CHANGED(hazeAltitude);
    COPY_PROPERTY_IF_CHANGED(hazeBaseRef);

    COPY_PROPERTY_IF_CHANGED(hazeBackgroundBlend);

    COPY_PROPERTY_IF_CHANGED(hazeKeyLightRange);
    COPY_PROPERTY_IF_CHANGED(hazeKeyLightAltitude);
}

void HazePropertyGroup::debugDump() const {
    qCDebug(entities) << "   HazePropertyGroup: ---------------------------------------------";
    qCDebug(entities) << "            _hazeMode:" << _hazeMode;

    qCDebug(entities) << "            _hazeRange:" << _hazeRange;
    qCDebug(entities) << "            _hazeBlendInColor:" << _hazeBlendInColor;
    qCDebug(entities) << "            _hazeBlendOutColor:" << _hazeBlendInColor;
    qCDebug(entities) << "            _hazeLightBlendAngle:" << _hazeLightBlendAngle;

    qCDebug(entities) << "            _hazeAltitude:" << _hazeAltitude;
    qCDebug(entities) << "            _hazeBaseRef:" << _hazeBaseRef;

    qCDebug(entities) << "            _hazeBackgroundBlend:" << _hazeBackgroundBlend;

    qCDebug(entities) << "            _hazeKeyLightRange:" << _hazeKeyLightRange;
    qCDebug(entities) << "            _hazeKeyLightAltitude:" << _hazeKeyLightAltitude;
}

void HazePropertyGroup::listChangedProperties(QList<QString>& out) {
    if (hazeModeChanged()) {
        out << "haze-mode";
    }

    if (hazeRangeChanged()) {
        out << "haze-range";
    }
    if (hazeBlendInColorChanged()) {
        out << "haze-blendInColor";
    }
    if (hazeBlendOutColorChanged()) {
        out << "haze-blendOutColor";
    }
    if (hazeLightBlendAngleChanged()) {
        out << "haze-lightBlendAngle";
    }

    if (hazeAltitudeChanged()) {
        out << "haze-altitude";
    }
    if (hazeBaseRefChanged()) {
        out << "haze-baseRef";
    }

    if (hazeBackgroundBlendChanged()) {
        out << "haze-backgroundBlend";
    }

    if (hazeKeyLightRangeChanged()) {
        out << "haze-keyLightRange";
    }
    if (hazeKeyLightAltitudeChanged()) {
        out << "haze-keyLightAltitude";
    }
}

bool HazePropertyGroup::appendToEditPacket(OctreePacketData* packetData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_HAZE_MODE, getHazeMode());

    APPEND_ENTITY_PROPERTY(PROP_HAZE_RANGE, getHazeRange());
    APPEND_ENTITY_PROPERTY(PROP_HAZE_BLEND_IN_COLOR, getHazeBlendInColor());
    APPEND_ENTITY_PROPERTY(PROP_HAZE_BLEND_OUT_COLOR, getHazeBlendOutColor());
    APPEND_ENTITY_PROPERTY(PROP_HAZE_LIGHT_BLEND_ANGLE, getHazeLightBlendAngle());

    APPEND_ENTITY_PROPERTY(PROP_HAZE_ALTITUDE, getHazeAltitude());
    APPEND_ENTITY_PROPERTY(PROP_HAZE_BASE_REF, getHazeBaseRef());

    APPEND_ENTITY_PROPERTY(PROP_HAZE_BACKGROUND_BLEND, getHazeBackgroundBlend());

    APPEND_ENTITY_PROPERTY(PROP_HAZE_KEYLIGHT_RANGE, getHazeKeyLightRange());
    APPEND_ENTITY_PROPERTY(PROP_HAZE_KEYLIGHT_ALTITUDE, getHazeKeyLightAltitude());

    return true;
}

bool HazePropertyGroup::decodeFromEditPacket(EntityPropertyFlags& propertyFlags, const unsigned char*& dataAt , int& processedBytes) {

    int bytesRead = 0;
    bool overwriteLocalData = true;
    bool somethingChanged = false;

    READ_ENTITY_PROPERTY(PROP_HAZE_MODE, uint8_t, setHazeMode);

    READ_ENTITY_PROPERTY(PROP_HAZE_RANGE, float, setHazeRange);
    READ_ENTITY_PROPERTY(PROP_HAZE_BLEND_IN_COLOR, xColor, setHazeBlendInColor);
    READ_ENTITY_PROPERTY(PROP_HAZE_BLEND_OUT_COLOR, xColor, setHazeBlendOutColor);
    READ_ENTITY_PROPERTY(PROP_HAZE_LIGHT_BLEND_ANGLE, float, setHazeLightBlendAngle);

    READ_ENTITY_PROPERTY(PROP_HAZE_ALTITUDE, float, setHazeAltitude);
    READ_ENTITY_PROPERTY(PROP_HAZE_BASE_REF, float, setHazeBaseRef);

    READ_ENTITY_PROPERTY(PROP_HAZE_BACKGROUND_BLEND, float, setHazeBackgroundBlend);

    READ_ENTITY_PROPERTY(PROP_HAZE_KEYLIGHT_RANGE, float, setHazeKeyLightRange);
    READ_ENTITY_PROPERTY(PROP_HAZE_KEYLIGHT_ALTITUDE, float, setHazeKeyLightAltitude);

    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_HAZE_RANGE, HazeMode);

    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_HAZE_RANGE, HazeRange);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_HAZE_BLEND_IN_COLOR, HazeBlendInColor);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_HAZE_BLEND_OUT_COLOR, HazeBlendOutColor);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_HAZE_LIGHT_BLEND_ANGLE, HazeLightBlendAngle);

    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_HAZE_ALTITUDE, HazeAltitude);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_HAZE_BASE_REF, HazeBaseRef);

    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_HAZE_BACKGROUND_BLEND, HazeBackgroundBlend);

    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_HAZE_KEYLIGHT_RANGE, HazeKeyLightRange);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_HAZE_KEYLIGHT_ALTITUDE, HazeKeyLightAltitude);

    processedBytes += bytesRead;

    Q_UNUSED(somethingChanged);

    return true;
}

void HazePropertyGroup::markAllChanged() {
    _hazeModeChanged;

    _hazeRangeChanged = true;
    _hazeBlendInColorChanged = true;
    _hazeBlendOutColorChanged = true;
    _hazeLightBlendAngleChanged = true;

    _hazeAltitudeChanged = true;
    _hazeBaseRefChanged = true;

    _hazeBackgroundBlendChanged = true;

    _hazeKeyLightRangeChanged = true;
    _hazeAltitudeChanged = true;
}

EntityPropertyFlags HazePropertyGroup::getChangedProperties() const {
    EntityPropertyFlags changedProperties;

    CHECK_PROPERTY_CHANGE(PROP_HAZE_MODE, hazeMode);

    CHECK_PROPERTY_CHANGE(PROP_HAZE_RANGE, hazeRange);
    CHECK_PROPERTY_CHANGE(PROP_HAZE_BLEND_IN_COLOR, hazeBlendInColor);
    CHECK_PROPERTY_CHANGE(PROP_HAZE_BLEND_OUT_COLOR, hazeBlendOutColor);
    CHECK_PROPERTY_CHANGE(PROP_HAZE_LIGHT_BLEND_ANGLE, hazeLightBlendAngle);

    CHECK_PROPERTY_CHANGE(PROP_HAZE_ALTITUDE, hazeAltitude);
    CHECK_PROPERTY_CHANGE(PROP_HAZE_BASE_REF, hazeBaseRef);

    CHECK_PROPERTY_CHANGE(PROP_HAZE_BACKGROUND_BLEND, hazeBackgroundBlend);

    CHECK_PROPERTY_CHANGE(PROP_HAZE_KEYLIGHT_RANGE, hazeKeyLightRange);
    CHECK_PROPERTY_CHANGE(PROP_HAZE_KEYLIGHT_ALTITUDE, hazeKeyLightAltitude);

    return changedProperties;
}

void HazePropertyGroup::getProperties(EntityItemProperties& properties) const {
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Haze, HazeMode, getHazeMode);

    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Haze, HazeRange, getHazeRange);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Haze, HazeBlendInColor, getHazeBlendInColor);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Haze, HazeBlendOutColor, getHazeBlendOutColor);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Haze, HazeLightBlendAngle, getHazeLightBlendAngle);

    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Haze, HazeAltitude, getHazeAltitude);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Haze, HazeBaseRef, getHazeBaseRef);

    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Haze, HazeBackgroundBlend, getHazeBackgroundBlend);

    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Haze, HazeKeyLightRange, getHazeKeyLightRange);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Haze, HazeKeyLightAltitude, getHazeKeyLightAltitude);
}

bool HazePropertyGroup::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;

    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Haze, HazeMode, hazeMode, setHazeMode);

    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Haze, HazeRange, hazeRange, setHazeRange);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Haze, HazeBlendInColor, hazeBlendInColor, setHazeBlendInColor);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Haze, HazeBlendOutColor, hazeBlendOutColor, setHazeBlendOutColor);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Haze, HazeLightBlendAngle, hazeLightBlendAngle, setHazeLightBlendAngle);

    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Haze, HazeAltitude, hazeAltitude, setHazeAltitude);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Haze, HazeBaseRef, hazeBaseRef, setHazeBaseRef);

    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Haze, HazeBackgroundBlend, hazeBackgroundBlend, setHazeBackgroundBlend);

    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Haze, HazeKeyLightRange, hazeKeyLightRange, setHazeKeyLightRange);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Haze, HazeKeyLightAltitude, hazeKeyLightAltitude, setHazeKeyLightAltitude);

    return somethingChanged;
}

EntityPropertyFlags HazePropertyGroup::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties;

    requestedProperties += PROP_HAZE_MODE;

    requestedProperties += PROP_HAZE_RANGE;
    requestedProperties += PROP_HAZE_BLEND_IN_COLOR;
    requestedProperties += PROP_HAZE_BLEND_OUT_COLOR;
    requestedProperties += PROP_HAZE_LIGHT_BLEND_ANGLE;

    requestedProperties += PROP_HAZE_ALTITUDE;
    requestedProperties += PROP_HAZE_BASE_REF;

    requestedProperties += PROP_HAZE_BACKGROUND_BLEND;

    requestedProperties += PROP_HAZE_KEYLIGHT_RANGE;
    requestedProperties += PROP_HAZE_KEYLIGHT_ALTITUDE;

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

    APPEND_ENTITY_PROPERTY(PROP_HAZE_MODE, getHazeMode());

    APPEND_ENTITY_PROPERTY(PROP_HAZE_RANGE, getHazeRange());
    APPEND_ENTITY_PROPERTY(PROP_HAZE_BLEND_IN_COLOR, getHazeBlendInColor());
    APPEND_ENTITY_PROPERTY(PROP_HAZE_BLEND_OUT_COLOR, getHazeBlendOutColor());
    APPEND_ENTITY_PROPERTY(PROP_HAZE_LIGHT_BLEND_ANGLE, getHazeLightBlendAngle());

    APPEND_ENTITY_PROPERTY(PROP_HAZE_ALTITUDE, getHazeAltitude());
    APPEND_ENTITY_PROPERTY(PROP_HAZE_BASE_REF, getHazeBaseRef());

    APPEND_ENTITY_PROPERTY(PROP_HAZE_BACKGROUND_BLEND, getHazeBackgroundBlend());

    APPEND_ENTITY_PROPERTY(PROP_HAZE_KEYLIGHT_RANGE, getHazeKeyLightRange());
    APPEND_ENTITY_PROPERTY(PROP_HAZE_KEYLIGHT_ALTITUDE, getHazeKeyLightAltitude());
}

int HazePropertyGroup::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                            ReadBitstreamToTreeParams& args,
                                            EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                            bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_HAZE_MODE, uint8_t, setHazeMode);

    READ_ENTITY_PROPERTY(PROP_HAZE_RANGE, float, setHazeRange);
    READ_ENTITY_PROPERTY(PROP_HAZE_BLEND_IN_COLOR, xColor, setHazeBlendInColor);
    READ_ENTITY_PROPERTY(PROP_HAZE_BLEND_OUT_COLOR, xColor, setHazeBlendOutColor);
    READ_ENTITY_PROPERTY(PROP_HAZE_LIGHT_BLEND_ANGLE, float, setHazeLightBlendAngle);

    READ_ENTITY_PROPERTY(PROP_HAZE_ALTITUDE, float, setHazeAltitude);
    READ_ENTITY_PROPERTY(PROP_HAZE_BASE_REF, float, setHazeBaseRef);

    READ_ENTITY_PROPERTY(PROP_HAZE_BACKGROUND_BLEND, float, setHazeBackgroundBlend);

    READ_ENTITY_PROPERTY(PROP_HAZE_KEYLIGHT_RANGE, float, setHazeKeyLightRange);
    READ_ENTITY_PROPERTY(PROP_HAZE_KEYLIGHT_ALTITUDE, float, setHazeKeyLightAltitude);

    return bytesRead;
}
