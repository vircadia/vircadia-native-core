//
//  BoxEntityItem.cpp
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
#include "BoxEntityItem.h"


EntityItem* BoxEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return new  BoxEntityItem(entityID, properties);
}

BoxEntityItem::BoxEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID, properties) 
{ 
    _type = EntityTypes::Box;
    setProperties(properties, true);
}

EntityItemProperties BoxEntityItem::getProperties() const {
    EntityItemProperties properties = EntityItem::getProperties(); // get the properties from our base class

    properties._color = getXColor();
    properties._colorChanged = false;

    properties._glowLevel = getGlowLevel();
    properties._glowLevelChanged = false;

    return properties;
}

bool BoxEntityItem::setProperties(const EntityItemProperties& properties, bool forceCopy) {
    bool somethingChanged = false;
    
    somethingChanged = EntityItem::setProperties(properties, forceCopy); // set the properties in our base class

    if (properties._colorChanged || forceCopy) {
        setColor(properties._color);
        somethingChanged = true;
    }

    if (properties._glowLevelChanged || forceCopy) {
        setGlowLevel(properties._glowLevel);
        somethingChanged = true;
    }

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - _lastEdited;
            qDebug() << "BoxEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                    "now=" << now << " _lastEdited=" << _lastEdited;
        }
        setLastEdited(properties._lastEdited);
    }
    return somethingChanged;
}

int BoxEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    // PROP_COLOR
    if (propertyFlags.getHasProperty(PROP_COLOR)) {
        rgbColor color;
        if (overwriteLocalData) {
            memcpy(_color, dataAt, sizeof(_color));
        }
        dataAt += sizeof(color);
        bytesRead += sizeof(color);
    }


    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags BoxEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_COLOR;
    return requestedProperties;
}

void BoxEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                    EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const { 

    bool successPropertyFits = true;

    // PROP_COLOR
    if (requestedProperties.getHasProperty(PROP_COLOR)) {
        LevelDetails propertyLevel = packetData->startLevel();
        successPropertyFits = packetData->appendColor(getColor());
        if (successPropertyFits) {
            propertyFlags |= PROP_COLOR;
            propertiesDidntFit -= PROP_COLOR;
            propertyCount++;
            packetData->endLevel(propertyLevel);
        } else {
            packetData->discardLevel(propertyLevel);
            appendState = OctreeElement::PARTIAL;
        }
    } else {
        propertiesDidntFit -= PROP_COLOR;
    }

}