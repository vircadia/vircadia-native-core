//
//  HyperlinkEntityItem.cpp
//  libraries/entities/src
//
//  Created by Niraj Venkat on 6/8/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include <glm/gtx/transform.hpp>

#include <QDebug>

#include <ByteCountCoding.h>
#include <PlaneShape.h>

#include "EntityTree.h"
#include "EntityTreeElement.h"
#include "EntitiesLogging.h"
#include "HyperlinkEntityItem.h"


const QString HyperlinkEntityItem::DEFAULT_HREF("");
const QString HyperlinkEntityItem::DEFAULT_DESCRIPTION("");

const float HyperlinkEntityItem::DEFAULT_LINE_HEIGHT = 0.1f;
const xColor HyperlinkEntityItem::DEFAULT_TEXT_COLOR = { 255, 255, 255 };
const xColor HyperlinkEntityItem::DEFAULT_BACKGROUND_COLOR = { 0, 0, 0 };

EntityItemPointer HyperlinkEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return EntityItemPointer(new HyperlinkEntityItem(entityID, properties));
}

HyperlinkEntityItem::HyperlinkEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
EntityItem(entityItemID)
{
    _type = EntityTypes::Text;
    _created = properties.getCreated();
    setProperties(properties);
}

const float TEXT_ENTITY_ITEM_FIXED_DEPTH = 0.01f;

void HyperlinkEntityItem::setDimensions(const glm::vec3& value) {
    // NOTE: Text Entities always have a "depth" of 1cm.
    _dimensions = glm::vec3(value.x, value.y, TEXT_ENTITY_ITEM_FIXED_DEPTH);
}

EntityItemProperties HyperlinkEntityItem::getProperties() const {
    EntityItemProperties properties = EntityItem::getProperties(); // get the properties from our base class

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(href, getHref);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(description, getDescription);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(lineHeight, getLineHeight);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(textColor, getTextColorX);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(backgroundColor, getBackgroundColorX);
    return properties;
}

bool HyperlinkEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;
    somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(href, setHref);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(description, setDescription);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(lineHeight, setLineHeight);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(textColor, setTextColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(backgroundColor, setBackgroundColor);

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qCDebug(entities) << "HyperlinkEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEdited(properties._lastEdited);
    }

    return somethingChanged;
}

int HyperlinkEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
    ReadBitstreamToTreeParams& args,
    EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_HREF, QString, setHref);
    READ_ENTITY_PROPERTY(PROP_DESCRIPTION, QString, setDescription);
    READ_ENTITY_PROPERTY(PROP_LINE_HEIGHT, float, setLineHeight);
    READ_ENTITY_PROPERTY(PROP_TEXT_COLOR, rgbColor, setTextColor);
    READ_ENTITY_PROPERTY(PROP_BACKGROUND_COLOR, rgbColor, setBackgroundColor);

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags HyperlinkEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_HREF;
    requestedProperties += PROP_DESCRIPTION;
    requestedProperties += PROP_LINE_HEIGHT;
    requestedProperties += PROP_TEXT_COLOR;
    requestedProperties += PROP_BACKGROUND_COLOR;
    return requestedProperties;
}

void HyperlinkEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
    EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
    EntityPropertyFlags& requestedProperties,
    EntityPropertyFlags& propertyFlags,
    EntityPropertyFlags& propertiesDidntFit,
    int& propertyCount,
    OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_HREF, getHref());
    APPEND_ENTITY_PROPERTY(PROP_DESCRIPTION, getDescription());
    APPEND_ENTITY_PROPERTY(PROP_LINE_HEIGHT, getLineHeight());
    APPEND_ENTITY_PROPERTY(PROP_TEXT_COLOR, getTextColor());
    APPEND_ENTITY_PROPERTY(PROP_BACKGROUND_COLOR, getBackgroundColor());
}


bool HyperlinkEntityItem::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
    bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face,
    void** intersectedObject, bool precisionPicking) const {

    RayIntersectionInfo rayInfo;
    rayInfo._rayStart = origin;
    rayInfo._rayDirection = direction;
    rayInfo._rayLength = std::numeric_limits<float>::max();

    PlaneShape plane;

    const glm::vec3 UNROTATED_NORMAL(0.0f, 0.0f, -1.0f);
    glm::vec3 normal = _rotation * UNROTATED_NORMAL;
    plane.setNormal(normal);
    plane.setPoint(getPosition()); // the position is definitely a point on our plane

    bool intersects = plane.findRayIntersection(rayInfo);

    if (intersects) {
        glm::vec3 hitAt = origin + (direction * rayInfo._hitDistance);
        // now we know the point the ray hit our plane

        glm::mat4 rotation = glm::mat4_cast(getRotation());
        glm::mat4 translation = glm::translate(getPosition());
        glm::mat4 entityToWorldMatrix = translation * rotation;
        glm::mat4 worldToEntityMatrix = glm::inverse(entityToWorldMatrix);

        glm::vec3 dimensions = getDimensions();
        glm::vec3 registrationPoint = getRegistrationPoint();
        glm::vec3 corner = -(dimensions * registrationPoint);
        AABox entityFrameBox(corner, dimensions);

        glm::vec3 entityFrameHitAt = glm::vec3(worldToEntityMatrix * glm::vec4(hitAt, 1.0f));

        intersects = entityFrameBox.contains(entityFrameHitAt);
    }

    if (intersects) {
        distance = rayInfo._hitDistance;
    }
    return intersects;
}
