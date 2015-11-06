//
//  AnchorEntityItem.cpp
//  libraries/entities/src
//
//  Created by Thijs Wenker on 11/4/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntitiesLogging.h"
#include "EntityItemID.h"
#include "EntityItemProperties.h"
#include "AnchorEntityItem.h"


EntityItemPointer AnchorEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer result { new AnchorEntityItem(entityID, properties) };
    return result;
}

// our non-pure virtual subclass for now...
AnchorEntityItem::AnchorEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID) 
{
    _type = EntityTypes::Anchor;

    setProperties(properties);
}


EntityItemProperties AnchorEntityItem::getProperties(EntityPropertyFlags desiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties); // get the properties from our base class
    return properties;
}

bool AnchorEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;
    somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qCDebug(entities) << "AnchorEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEdited(properties._lastEdited);
    }

    return somethingChanged;
}

int AnchorEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
    ReadBitstreamToTreeParams& args,
    EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
    bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags AnchorEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    return requestedProperties;
}

void AnchorEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
    EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
    EntityPropertyFlags& requestedProperties,
    EntityPropertyFlags& propertyFlags,
    EntityPropertyFlags& propertiesDidntFit,
    int& propertyCount,
    OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;
}

