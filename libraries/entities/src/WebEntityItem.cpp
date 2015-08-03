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

#include <ByteCountCoding.h>
#include <PlaneShape.h>

#include "EntityTree.h"
#include "EntityTreeElement.h"
#include "EntitiesLogging.h"


const QString WebEntityItem::DEFAULT_SOURCE_URL("http://www.google.com");

EntityItemPointer WebEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return std::make_shared<WebEntityItem>(entityID, properties);
}

WebEntityItem::WebEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID) 
{
    _type = EntityTypes::Web;
    setProperties(properties);
}

const float WEB_ENTITY_ITEM_FIXED_DEPTH = 0.01f;

void WebEntityItem::setDimensions(const glm::vec3& value) {
    // NOTE: Web Entities always have a "depth" of 1cm.
    EntityItem::setDimensions(glm::vec3(value.x, value.y, WEB_ENTITY_ITEM_FIXED_DEPTH));
}

EntityItemProperties WebEntityItem::getProperties() const {
    EntityItemProperties properties = EntityItem::getProperties(); // get the properties from our base class
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(sourceUrl, getSourceUrl);
    return properties;
}

bool WebEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;
    somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(sourceUrl, setSourceUrl);

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
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_SOURCE_URL, QString, setSourceUrl);

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags WebEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_SOURCE_URL;
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
}


bool WebEntityItem::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                     bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face, 
                     void** intersectedObject, bool precisionPicking) const {
                     
    RayIntersectionInfo rayInfo;
    rayInfo._rayStart = origin;
    rayInfo._rayDirection = direction;
    rayInfo._rayLength = std::numeric_limits<float>::max();

    PlaneShape plane;

    const glm::vec3 UNROTATED_NORMAL(0.0f, 0.0f, -1.0f);
    glm::vec3 normal = getRotation() * UNROTATED_NORMAL;
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

void WebEntityItem::setSourceUrl(const QString& value) { 
    if (_sourceUrl != value) {
        _sourceUrl = value;
    }
}

const QString& WebEntityItem::getSourceUrl() const { return _sourceUrl; }
