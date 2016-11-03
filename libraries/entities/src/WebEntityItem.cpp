//
//  Created by Bradley Austin Davis on 2015/05/12
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "WebEntityItem.h"

#include <glm/gtx/transform.hpp>

#include <QDebug>
#include <QJsonDocument>

#include <ByteCountCoding.h>
#include <GeometryUtil.h>

#include "EntitiesLogging.h"
#include "EntityItemProperties.h"
#include "EntityTree.h"
#include "EntityTreeElement.h"

const QString WebEntityItem::DEFAULT_SOURCE_URL("http://www.google.com");

EntityItemPointer WebEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity { new WebEntityItem(entityID) };
    entity->setProperties(properties);
    return entity;
}

WebEntityItem::WebEntityItem(const EntityItemID& entityItemID) : EntityItem(entityItemID) {
    _type = EntityTypes::Web;
    _dpi = ENTITY_ITEM_DEFAULT_DPI;
}

const float WEB_ENTITY_ITEM_FIXED_DEPTH = 0.01f;

void WebEntityItem::setDimensions(const glm::vec3& value) {
    // NOTE: Web Entities always have a "depth" of 1cm.
    EntityItem::setDimensions(glm::vec3(value.x, value.y, WEB_ENTITY_ITEM_FIXED_DEPTH));
}

EntityItemProperties WebEntityItem::getProperties(EntityPropertyFlags desiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties); // get the properties from our base class
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(sourceUrl, getSourceUrl);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(dpi, getDPI);
    return properties;
}

bool WebEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;
    somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(sourceUrl, setSourceUrl);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(dpi, setDPI);

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qCDebug(entities) << "WebEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                    "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEdited(properties._lastEdited);
    }

    return somethingChanged;
}

int WebEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_SOURCE_URL, QString, setSourceUrl);
    READ_ENTITY_PROPERTY(PROP_DPI, uint16_t, setDPI);

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags WebEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_SOURCE_URL;
    requestedProperties += PROP_DPI;
    return requestedProperties;
}

void WebEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;
    APPEND_ENTITY_PROPERTY(PROP_SOURCE_URL, _sourceUrl);
    APPEND_ENTITY_PROPERTY(PROP_DPI, _dpi);
}

bool WebEntityItem::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                                bool& keepSearching, OctreeElementPointer& element, float& distance,
                                                BoxFace& face, glm::vec3& surfaceNormal,
                                                void** intersectedObject, bool precisionPicking) const {
    glm::vec3 dimensions = getDimensions();
    glm::vec2 xyDimensions(dimensions.x, dimensions.y);
    glm::quat rotation = getRotation();
    glm::vec3 position = getPosition() + rotation * (dimensions * (getRegistrationPoint() - ENTITY_ITEM_DEFAULT_REGISTRATION_POINT));

    if (findRayRectangleIntersection(origin, direction, rotation, position, xyDimensions, distance)) {
        surfaceNormal = rotation * Vectors::UNIT_Z;
        face = glm::dot(surfaceNormal, direction) > 0 ? MIN_Z_FACE : MAX_Z_FACE;
        return true;
    } else {
        return false;
    }
}

void WebEntityItem::setSourceUrl(const QString& value) {
    if (_sourceUrl != value) {
        _sourceUrl = value;
    }
}

const QString& WebEntityItem::getSourceUrl() const { return _sourceUrl; }

void WebEntityItem::setDPI(uint16_t value) {
    _dpi = value;
}

uint16_t WebEntityItem::getDPI() const {
    return _dpi;
}
