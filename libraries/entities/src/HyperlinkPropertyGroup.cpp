//
//  HyperlinkPropertyGroup.cpp
//  libraries/entities/src
//
//  Created by Niraj Venkat on 6/9/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <OctreePacketData.h>

#include "HyperlinkPropertyGroup.h"
#include "EntityItemProperties.h"
#include "EntityItemPropertiesMacros.h"

HyperlinkPropertyGroup::HyperlinkPropertyGroup() {
    const QString DEFAULT_HREF = QString("");
    const QString DEFAULT_DESCRIPTION = QString("");

    _href = DEFAULT_HREF;
    _description = DEFAULT_DESCRIPTION;
}

void HyperlinkPropertyGroup::copyToScriptValue(QScriptValue& properties, QScriptEngine* engine, bool skipDefaults, EntityItemProperties& defaultEntityProperties) const {
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(Hyperlink, hyperlink, Href, href);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(Hyperlink, hyperlink, Description, description);
}

void HyperlinkPropertyGroup::copyFromScriptValue(const QScriptValue& object, bool& _defaultSettings) {
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(Hyperlink, href, QString, setHref);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(Hyperlink, description, QString, setDescription);
}

void HyperlinkPropertyGroup::debugDump() const {
    qDebug() << "   HyperlinkPropertyGroup: ---------------------------------------------";
    qDebug() << "       Href:" << getHref() << " has changed:" << hrefChanged();
    qDebug() << "       Description:" << getDescription() << " has changed:" << descriptionChanged();
}

bool HyperlinkPropertyGroup::appentToEditPacket(OctreePacketData* packetData,
    EntityPropertyFlags& requestedProperties,
    EntityPropertyFlags& propertyFlags,
    EntityPropertyFlags& propertiesDidntFit,
    int& propertyCount,
    OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_HREF, getHref());
    APPEND_ENTITY_PROPERTY(PROP_DESCRIPTION, getDescription());

    return true;
}


bool HyperlinkPropertyGroup::decodeFromEditPacket(EntityPropertyFlags& propertyFlags, const unsigned char*& dataAt, int& processedBytes) {

    int bytesRead = 0;
    bool overwriteLocalData = true;

    READ_ENTITY_PROPERTY(PROP_HREF, QString, setHref);
    READ_ENTITY_PROPERTY(PROP_DESCRIPTION, QString, setDescription);

    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_HREF, Href);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_DESCRIPTION, Description);

    processedBytes += bytesRead;

    return true;
}

void HyperlinkPropertyGroup::markAllChanged() {
    _hrefChanged = true;
    _descriptionChanged = true;
}

EntityPropertyFlags HyperlinkPropertyGroup::getChangedProperties() const {
    EntityPropertyFlags changedProperties;

    CHECK_PROPERTY_CHANGE(PROP_HREF, href);
    CHECK_PROPERTY_CHANGE(PROP_DESCRIPTION, description);

    return changedProperties;
}

void HyperlinkPropertyGroup::getProperties(EntityItemProperties& properties) const {
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Hyperlink, Href, getHref);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Hyperlink, Description, getDescription);
}

bool HyperlinkPropertyGroup::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;

    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Hyperlink, Href, href, setHref);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Hyperlink, Description, description, setDescription);

    return somethingChanged;
}

EntityPropertyFlags HyperlinkPropertyGroup::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties;

    requestedProperties += PROP_HREF;
    requestedProperties += PROP_DESCRIPTION;

    return requestedProperties;
}

void HyperlinkPropertyGroup::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
    EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData,
    EntityPropertyFlags& requestedProperties,
    EntityPropertyFlags& propertyFlags,
    EntityPropertyFlags& propertiesDidntFit,
    int& propertyCount,
    OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_HREF, getHref());
    APPEND_ENTITY_PROPERTY(PROP_DESCRIPTION, getDescription());
}

int HyperlinkPropertyGroup::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
    ReadBitstreamToTreeParams& args,
    EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_DESCRIPTION, QString, setHref);
    READ_ENTITY_PROPERTY(PROP_DESCRIPTION, QString, setDescription);

    return bytesRead;
}
