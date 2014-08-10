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


//static bool registerBox = EntityTypes::registerEntityType(EntityTypes::Box, "Box", BoxEntityItem::factory);
 

EntityItem* BoxEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    qDebug() << "BoxEntityItem::factory(const EntityItemID& entityItemID, const EntityItemProperties& properties)...";
    return new  BoxEntityItem(entityID, properties);
}

// our non-pure virtual subclass for now...
BoxEntityItem::BoxEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID, properties) 
{ 
    qDebug() << "BoxEntityItem::BoxEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties)...";
    _type = EntityTypes::Box;

    qDebug() << "BoxEntityItem::BoxEntityItem() properties.getModelURL()=" << properties.getModelURL();
    
    qDebug() << "BoxEntityItem::BoxEntityItem() calling setProperties()";
    setProperties(properties);


}

EntityItemProperties BoxEntityItem::getProperties() const {
    qDebug() << "BoxEntityItem::getProperties()... <<<<<<<<<<<<<<<<  <<<<<<<<<<<<<<<<<<<<<<<<<";

    EntityItemProperties properties = EntityItem::getProperties(); // get the properties from our base class

    properties._color = getXColor();
    properties._colorChanged = false;

    properties._glowLevel = getGlowLevel();
    properties._glowLevelChanged = false;

    return properties;
}

void BoxEntityItem::setProperties(const EntityItemProperties& properties, bool forceCopy) {
    qDebug() << "BoxEntityItem::setProperties()...";
    qDebug() << "BoxEntityItem::BoxEntityItem() properties.getModelURL()=" << properties.getModelURL();
    bool somethingChanged = false;
    
    EntityItem::setProperties(properties, forceCopy); // set the properties in our base class

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
}

int BoxEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    qDebug() << "BoxEntityItem::readEntitySubclassDataFromBuffer()... <<<<<<<<<<<<<<<<  <<<<<<<<<<<<<<<<<<<<<<<<<";

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



qDebug() << "BoxEntityItem::appendSubclassData()... ********************************************";

    bool successPropertyFits = true;

    // PROP_COLOR
    if (requestedProperties.getHasProperty(PROP_COLOR)) {
        //qDebug() << "PROP_COLOR requested...";
        LevelDetails propertyLevel = packetData->startLevel();
        successPropertyFits = packetData->appendColor(getColor());
        if (successPropertyFits) {
            propertyFlags |= PROP_COLOR;
            propertiesDidntFit -= PROP_COLOR;
            propertyCount++;
            packetData->endLevel(propertyLevel);
        } else {
            //qDebug() << "PROP_COLOR didn't fit...";
            packetData->discardLevel(propertyLevel);
            appendState = OctreeElement::PARTIAL;
        }
    } else {
        //qDebug() << "PROP_COLOR NOT requested...";
        propertiesDidntFit -= PROP_COLOR;
    }

}