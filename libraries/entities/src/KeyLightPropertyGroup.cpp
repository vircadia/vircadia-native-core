//
//  KeyLightPropertyGroup.h
//  libraries/entities/src
//
//  Created by Sam Gateau on 2015/10/23.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QJsonDocument>
#include <OctreePacketData.h>

#include <AnimationLoop.h>

#include "KeyLightPropertyGroup.h"
#include "EntityItemProperties.h"
#include "EntityItemPropertiesMacros.h"


const xColor KeyLightPropertyGroup::DEFAULT_KEYLIGHT_COLOR = { 255, 255, 255 };
const float KeyLightPropertyGroup::DEFAULT_KEYLIGHT_INTENSITY = 1.0f;
const float KeyLightPropertyGroup::DEFAULT_KEYLIGHT_AMBIENT_INTENSITY = 0.5f;
const glm::vec3 KeyLightPropertyGroup::DEFAULT_KEYLIGHT_DIRECTION = { 0.0f, -1.0f, 0.0f };

void KeyLightPropertyGroup::copyToScriptValue(const EntityPropertyFlags& desiredProperties, QScriptValue& properties, QScriptEngine* engine, bool skipDefaults, EntityItemProperties& defaultEntityProperties) const {
    
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_KEYLIGHT_COLOR, KeyLight, keyLight, Color, color);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_KEYLIGHT_INTENSITY, KeyLight, keyLight, Intensity, intensity);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_KEYLIGHT_AMBIENT_INTENSITY, KeyLight, keyLight, AmbientIntensity, ambientIntensity);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_KEYLIGHT_DIRECTION, KeyLight, keyLight, Direction, direction);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_KEYLIGHT_AMBIENT_URL, KeyLight, keyLight, AmbientURL, ambientURL);

}

void KeyLightPropertyGroup::copyFromScriptValue(const QScriptValue& object, bool& _defaultSettings) {

    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(keyLight, color, xColor, setColor);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(keyLight, intensity, float, setIntensity);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(keyLight, ambientIntensity, float, setAmbientIntensity);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(keyLight, direction, glmVec3, setDirection);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(keyLight, ambientURL, QString, setAmbientURL);
    
    // legacy property support
    COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(keyLightColor, xColor, setColor, getColor);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(keyLightIntensity, float, setIntensity, getIntensity);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(keyLightAmbientIntensity, float, setAmbientIntensity, getAmbientIntensity);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(keyLightDirection, glmVec3, setDirection, getDirection);
}

void KeyLightPropertyGroup::merge(const KeyLightPropertyGroup& other) {
    COPY_PROPERTY_IF_CHANGED(color);
    COPY_PROPERTY_IF_CHANGED(intensity);
    COPY_PROPERTY_IF_CHANGED(ambientIntensity);
    COPY_PROPERTY_IF_CHANGED(direction);
    COPY_PROPERTY_IF_CHANGED(ambientURL);
}



void KeyLightPropertyGroup::debugDump() const {
    qCDebug(entities) << "   KeyLightPropertyGroup: ---------------------------------------------";
    qCDebug(entities) << "        color:" << getColor(); // << "," << getColor()[1] << "," << getColor()[2];
    qCDebug(entities) << "        intensity:" << getIntensity();
    qCDebug(entities) << "        direction:" << getDirection();
    qCDebug(entities) << "        ambientIntensity:" << getAmbientIntensity();
    qCDebug(entities) << "        ambientURL:" << getAmbientURL();
}

void KeyLightPropertyGroup::listChangedProperties(QList<QString>& out) {
    if (colorChanged()) {
        out << "keyLight-color";
    }
    if (intensityChanged()) {
        out << "keyLight-intensity";
    }
    if (directionChanged()) {
        out << "keyLight-direction";
    }
    if (ambientIntensityChanged()) {
        out << "keyLight-ambientIntensity";
    }
    if (ambientURLChanged()) {
        out << "keyLight-ambientURL";
    }
}


bool KeyLightPropertyGroup::appendToEditPacket(OctreePacketData* packetData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;
    
    APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_COLOR, getColor());
    APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_INTENSITY, getIntensity());
    APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_AMBIENT_INTENSITY, getAmbientIntensity());
    APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_DIRECTION, getDirection());
    APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_AMBIENT_URL, getAmbientURL());
    
    return true;
}


bool KeyLightPropertyGroup::decodeFromEditPacket(EntityPropertyFlags& propertyFlags, const unsigned char*& dataAt , int& processedBytes) {

    int bytesRead = 0;
    bool overwriteLocalData = true;
    bool somethingChanged = false;
    
    READ_ENTITY_PROPERTY(PROP_KEYLIGHT_COLOR, xColor, setColor);
    READ_ENTITY_PROPERTY(PROP_KEYLIGHT_INTENSITY, float, setIntensity);
    READ_ENTITY_PROPERTY(PROP_KEYLIGHT_AMBIENT_INTENSITY, float, setAmbientIntensity);
    READ_ENTITY_PROPERTY(PROP_KEYLIGHT_DIRECTION, glm::vec3, setDirection);
    READ_ENTITY_PROPERTY(PROP_KEYLIGHT_AMBIENT_URL, QString, setAmbientURL);
    
    
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_KEYLIGHT_COLOR, Color);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_KEYLIGHT_INTENSITY, Intensity);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_KEYLIGHT_AMBIENT_INTENSITY, AmbientIntensity);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_KEYLIGHT_DIRECTION, Direction);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_KEYLIGHT_AMBIENT_URL, AmbientURL);
    
    processedBytes += bytesRead;

    Q_UNUSED(somethingChanged);

    return true;
}

void KeyLightPropertyGroup::markAllChanged() {
    _colorChanged = true;
    _intensityChanged = true;
    _ambientIntensityChanged = true;
    _directionChanged = true;
    _ambientURLChanged = true;
}

EntityPropertyFlags KeyLightPropertyGroup::getChangedProperties() const {
    EntityPropertyFlags changedProperties;
 
    CHECK_PROPERTY_CHANGE(PROP_KEYLIGHT_COLOR, color);
    CHECK_PROPERTY_CHANGE(PROP_KEYLIGHT_INTENSITY, intensity);
    CHECK_PROPERTY_CHANGE(PROP_KEYLIGHT_AMBIENT_INTENSITY, ambientIntensity);
    CHECK_PROPERTY_CHANGE(PROP_KEYLIGHT_DIRECTION, direction);
    CHECK_PROPERTY_CHANGE(PROP_KEYLIGHT_AMBIENT_URL, ambientURL);
    
    return changedProperties;
}

void KeyLightPropertyGroup::getProperties(EntityItemProperties& properties) const {
    
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(KeyLight, Color, getColor);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(KeyLight, Intensity, getIntensity);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(KeyLight, AmbientIntensity, getAmbientIntensity);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(KeyLight, Direction, getDirection);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(KeyLight, AmbientURL, getAmbientURL);
}

bool KeyLightPropertyGroup::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;

    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(KeyLight, Color, color, setColor);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(KeyLight, Intensity, intensity, setIntensity);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(KeyLight, AmbientIntensity, ambientIntensity, setAmbientIntensity);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(KeyLight, Direction, direction, setDirection);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(KeyLight, AmbientURL, ambientURL, setAmbientURL);

    return somethingChanged;
}

EntityPropertyFlags KeyLightPropertyGroup::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties;

    requestedProperties += PROP_KEYLIGHT_COLOR;
    requestedProperties += PROP_KEYLIGHT_INTENSITY;
    requestedProperties += PROP_KEYLIGHT_AMBIENT_INTENSITY;
    requestedProperties += PROP_KEYLIGHT_DIRECTION;
    requestedProperties += PROP_KEYLIGHT_AMBIENT_URL;

    return requestedProperties;
}
    
void KeyLightPropertyGroup::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                EntityTreeElementExtraEncodeDataPointer entityTreeElementExtraEncodeData,
                                EntityPropertyFlags& requestedProperties,
                                EntityPropertyFlags& propertyFlags,
                                EntityPropertyFlags& propertiesDidntFit,
                                int& propertyCount, 
                                OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_COLOR, getColor());
    APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_INTENSITY, getIntensity());
    APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_AMBIENT_INTENSITY, getAmbientIntensity());
    APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_DIRECTION, getDirection());
    APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_AMBIENT_URL, getAmbientURL());
}

int KeyLightPropertyGroup::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                            ReadBitstreamToTreeParams& args,
                                            EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                            bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;
    
    READ_ENTITY_PROPERTY(PROP_KEYLIGHT_COLOR, xColor, setColor);
    READ_ENTITY_PROPERTY(PROP_KEYLIGHT_INTENSITY, float, setIntensity);
    READ_ENTITY_PROPERTY(PROP_KEYLIGHT_AMBIENT_INTENSITY, float, setAmbientIntensity);
    READ_ENTITY_PROPERTY(PROP_KEYLIGHT_DIRECTION, glm::vec3, setDirection);
    READ_ENTITY_PROPERTY(PROP_KEYLIGHT_AMBIENT_URL, QString, setAmbientURL);

    return bytesRead;
}
