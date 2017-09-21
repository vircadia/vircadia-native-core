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

const xColor HazePropertyGroup::DEFAULT_COLOR = { 0, 0, 0 };

void HazePropertyGroup::copyToScriptValue(const EntityPropertyFlags& desiredProperties, QScriptValue& properties, QScriptEngine* engine, bool skipDefaults, EntityItemProperties& defaultEntityProperties) const {
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_HAZE_COLOR, Haze, haze, Color, color);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_HAZE_URL, Haze, haze, URL, url);
}

void HazePropertyGroup::copyFromScriptValue(const QScriptValue& object, bool& _defaultSettings) {
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(haze, color, xColor, setColor);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(haze, url, QString, setURL);
}

void HazePropertyGroup::merge(const HazePropertyGroup& other) {
    COPY_PROPERTY_IF_CHANGED(color);
    COPY_PROPERTY_IF_CHANGED(url);
}


void HazePropertyGroup::debugDump() const {
    qCDebug(entities) << "   HazePropertyGroup: ---------------------------------------------";
    qCDebug(entities) << "       Color:" << getColor() << " has changed:" << colorChanged();
    qCDebug(entities) << "       URL:" << getURL() << " has changed:" << urlChanged();
}

void HazePropertyGroup::listChangedProperties(QList<QString>& out) {
    if (colorChanged()) {
        out << "haze-color";
    }
    if (urlChanged()) {
        out << "haze-url";
    }
}

bool HazePropertyGroup::appendToEditPacket(OctreePacketData* packetData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_HAZE_COLOR, getColor());
    APPEND_ENTITY_PROPERTY(PROP_HAZE_URL, getURL());

    return true;
}


bool HazePropertyGroup::decodeFromEditPacket(EntityPropertyFlags& propertyFlags, const unsigned char*& dataAt , int& processedBytes) {

    int bytesRead = 0;
    bool overwriteLocalData = true;
    bool somethingChanged = false;

    READ_ENTITY_PROPERTY(PROP_HAZE_COLOR, xColor, setColor);
    READ_ENTITY_PROPERTY(PROP_HAZE_URL, QString, setURL);

    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_HAZE_COLOR, Color);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_HAZE_URL, URL);

    processedBytes += bytesRead;

    Q_UNUSED(somethingChanged);

    return true;
}

void HazePropertyGroup::markAllChanged() {
    _colorChanged = true;
    _urlChanged = true;
}

EntityPropertyFlags HazePropertyGroup::getChangedProperties() const {
    EntityPropertyFlags changedProperties;
    
    CHECK_PROPERTY_CHANGE(PROP_HAZE_COLOR, color);
    CHECK_PROPERTY_CHANGE(PROP_HAZE_URL, url);

    return changedProperties;
}

void HazePropertyGroup::getProperties(EntityItemProperties& properties) const {
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Haze, Color, getColor);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Haze, URL, getURL);
}

bool HazePropertyGroup::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;

    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Haze, Color, color, setColor);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Haze, URL, url, setURL);

    return somethingChanged;
}

EntityPropertyFlags HazePropertyGroup::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties;

    requestedProperties += PROP_HAZE_COLOR;
    requestedProperties += PROP_HAZE_URL;
    
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

    APPEND_ENTITY_PROPERTY(PROP_HAZE_COLOR, getColor());
    APPEND_ENTITY_PROPERTY(PROP_HAZE_URL, getURL());
}

int HazePropertyGroup::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                            ReadBitstreamToTreeParams& args,
                                            EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                            bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_HAZE_COLOR, xColor, setColor);
    READ_ENTITY_PROPERTY(PROP_HAZE_URL, QString, setURL);

    return bytesRead;
}
