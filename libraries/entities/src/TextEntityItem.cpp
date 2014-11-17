//
//  TextEntityItem.cpp
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
#include "TextEntityItem.h"


const QString TextEntityItem::DEFAULT_TEXT("");
const float TextEntityItem::DEFAULT_LINE_HEIGHT = 0.1f;
const xColor TextEntityItem::DEFAULT_TEXT_COLOR = { 255, 255, 255 };
const xColor TextEntityItem::DEFAULT_BACKGROUND_COLOR = { 0, 0, 0};

EntityItem* TextEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItem* result = new TextEntityItem(entityID, properties);
    return result;
}

TextEntityItem::TextEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID) 
{
    _type = EntityTypes::Text;
    _created = properties.getCreated();
    setProperties(properties, true);
}

EntityItemProperties TextEntityItem::getProperties() const {
    EntityItemProperties properties = EntityItem::getProperties(); // get the properties from our base class

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(text, getText);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(lineHeight, getLineHeight);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(textColor, getTextColorX);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(backgroundColor, getBackgroundColorX);
    return properties;
}

bool TextEntityItem::setProperties(const EntityItemProperties& properties, bool forceCopy) {
    bool somethingChanged = false;
    somethingChanged = EntityItem::setProperties(properties, forceCopy); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(text, setText);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(lineHeight, setLineHeight);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(textColor, setTextColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(backgroundColor, setBackgroundColor);

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qDebug() << "TextEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                    "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEdited(properties._lastEdited);
    }
    
    return somethingChanged;
}

int TextEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY_STRING(PROP_TEXT, setText);
    READ_ENTITY_PROPERTY(PROP_LINE_HEIGHT, float, _lineHeight);
    READ_ENTITY_PROPERTY_COLOR(PROP_TEXT_COLOR, _textColor);
    READ_ENTITY_PROPERTY_COLOR(PROP_BACKGROUND_COLOR, _backgroundColor);

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags TextEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_TEXT;
    requestedProperties += PROP_LINE_HEIGHT;
    requestedProperties += PROP_TEXT_COLOR;
    requestedProperties += PROP_BACKGROUND_COLOR;
    return requestedProperties;
}

void TextEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                    EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const { 

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_TEXT, appendValue, getText());
    APPEND_ENTITY_PROPERTY(PROP_LINE_HEIGHT, appendValue, getLineHeight());
    APPEND_ENTITY_PROPERTY(PROP_TEXT_COLOR, appendColor, getTextColor());
    APPEND_ENTITY_PROPERTY(PROP_BACKGROUND_COLOR, appendColor, getBackgroundColor());
}