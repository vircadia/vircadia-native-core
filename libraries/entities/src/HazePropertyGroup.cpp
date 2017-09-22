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

void HazePropertyGroup::copyToScriptValue(const EntityPropertyFlags& desiredProperties, QScriptValue& properties, QScriptEngine* engine, bool skipDefaults, EntityItemProperties& defaultEntityProperties) const {
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_HAZE_HAZE_ACTIVE, Haze, haze, HazeActive, hazeActive);
}

void HazePropertyGroup::copyFromScriptValue(const QScriptValue& object, bool& _defaultSettings) {
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(haze, hazeActive, bool, setHazeActive);
}

void HazePropertyGroup::merge(const HazePropertyGroup& other) {
    COPY_PROPERTY_IF_CHANGED(hazeActive);
}


void HazePropertyGroup::debugDump() const {
    qCDebug(entities) << "   HazePropertyGroup: ---------------------------------------------";
    qCDebug(entities) << "       HazeActive :" << getHazeActive() << " has changed:" << hazeActiveChanged();
}

void HazePropertyGroup::listChangedProperties(QList<QString>& out) {
    if (hazeActiveChanged()) {
        out << "haze-hazeActive";
    }
}

bool HazePropertyGroup::appendToEditPacket(OctreePacketData* packetData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_HAZE_HAZE_ACTIVE, getHazeActive());

    return true;
}


bool HazePropertyGroup::decodeFromEditPacket(EntityPropertyFlags& propertyFlags, const unsigned char*& dataAt , int& processedBytes) {

    int bytesRead = 0;
    bool overwriteLocalData = true;
    bool somethingChanged = false;

    READ_ENTITY_PROPERTY(PROP_HAZE_HAZE_ACTIVE, bool, setHazeActive);

    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_HAZE_HAZE_ACTIVE, HazeActive);

    processedBytes += bytesRead;

    Q_UNUSED(somethingChanged);

    return true;
}

void HazePropertyGroup::markAllChanged() {
    _hazeActiveChanged = true;
}

EntityPropertyFlags HazePropertyGroup::getChangedProperties() const {
    EntityPropertyFlags changedProperties;

    CHECK_PROPERTY_CHANGE(PROP_HAZE_HAZE_ACTIVE, hazeActive);

    return changedProperties;
}

void HazePropertyGroup::getProperties(EntityItemProperties& properties) const {
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Haze, HazeActive, getHazeActive);
}

bool HazePropertyGroup::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;

    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Haze, HazeActive, hazeActive, setHazeActive);

    return somethingChanged;
}

EntityPropertyFlags HazePropertyGroup::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties;

    requestedProperties += PROP_HAZE_HAZE_ACTIVE;
    
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

    APPEND_ENTITY_PROPERTY(PROP_HAZE_HAZE_ACTIVE, getHazeActive());
}

int HazePropertyGroup::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                            ReadBitstreamToTreeParams& args,
                                            EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                            bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_HAZE_HAZE_ACTIVE, bool, setHazeActive);

    return bytesRead;
}
