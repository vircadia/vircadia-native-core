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

/*
bool ZoneEntityItem::_zonesArePickable = false;

const xColor ZoneEntityItem::DEFAULT_SUN_sunColor = { 255, 255, 255 };
const float ZoneEntityItem::DEFAULT_SUN_INTENSITY = 0.1f;
const float ZoneEntityItem::DEFAULT_SUN_AMBIENT_INTENSITY = 0.0f;
const bool ZoneEntityItem::DEFAULT_SUN_USE_EARTH_MODEL = true;
const float ZoneEntityItem::DEFAULT_SUN_APPARENT_LATITUDE = 37.777f;
const float ZoneEntityItem::DEFAULT_SUN_APPARENT_LONGITUDE = 122.407f;
const float ZoneEntityItem::DEFAULT_SUN_APPARENT_ALTITUDE = 0.03f;
const quint16 ZoneEntityItem::DEFAULT_SUN_APPARENT_DAY = 60;
const float ZoneEntityItem::DEFAULT_SUN_APPARENT_HOUR = 12.0f;
const glm::vec3 ZoneEntityItem::DEFAULT_SUN_DIRECTION = { 0.0f, -1.0f, 0.0f };
*/

EntityItem* ZoneEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItem* result = new ZoneEntityItem(entityID, properties);
    return result;
}

ZoneEntityItem::ZoneEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID) 
{
    _type = EntityTypes::Zone;
    _created = properties.getCreated();
    setProperties(properties);
}

EntityItemProperties ZoneEntityItem::getProperties() const {
    EntityItemProperties properties = EntityItem::getProperties(); // get the properties from our base class

    properties._color = getXColor();
    properties._colorChanged = false;

    properties._glowLevel = getGlowLevel();
    properties._glowLevelChanged = false;

    return properties;
}

bool ZoneEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;
    somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(color, setColor);

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

    READ_ENTITY_PROPERTY_COLOR(PROP_COLOR, _sunColor);

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags ZoneEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_COLOR;
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

    APPEND_ENTITY_PROPERTY(PROP_COLOR, appendColor, getColor());
}

void ZoneEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "   BOX EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "               color:" << _sunColor[0] << "," << _sunColor[1] << "," << _sunColor[2];
    qCDebug(entities) << "            position:" << debugTreeVector(_position);
    qCDebug(entities) << "          dimensions:" << debugTreeVector(_dimensions);
    qCDebug(entities) << "       getLastEdited:" << debugTime(getLastEdited(), now);
}

