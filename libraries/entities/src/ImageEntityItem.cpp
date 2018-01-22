//
//  ImageEntityItem.cpp
//  libraries/entities/src
//
//  Created by Elisa Lupin-Jimenez on 1/3/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* NOT IN USE

#include <QtCore/QJsonDocument>

#include <ByteCountCoding.h>
#include <GLMHelpers.h>
#include <glm/gtx/transform.hpp>
#include <GeometryUtil.h>

#include "EntitiesLogging.h"
#include "EntityItemProperties.h"
#include "EntityTree.h"
#include "EntityTreeElement.h"
#include "ResourceCache.h"
#include "ShapeEntityItem.h"
#include "ImageEntityItem.h"

const QString ImageEntityItem::DEFAULT_IMAGE_URL("");

EntityItemPointer ImageEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity(new ImageEntityItem(entityID), [](EntityItem* ptr) { ptr->deleteLater(); });
    entity->setProperties(properties);
    return entity;
}

ImageEntityItem::ImageEntityItem(const EntityItemID& entityItemID) : EntityItem(entityItemID) {
    _type = EntityTypes::Image;
}

EntityItemProperties ImageEntityItem::getProperties(EntityPropertyFlags desiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties); // get the properties from our base class
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(imageURL, getImageURL);
    // Using "Quad" shape as defined in ShapeEntityItem.cpp
    properties.setShape("Quad");
    return properties;
}

bool ImageEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(imageURL, setImageURL);

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qCDebug(entities) << "ImageEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEdited(properties.getLastEdited());
    }
    return somethingChanged;
}

// TODO: eventually only include properties changed since the params.nodeData->getLastTimeBagEmpty() time
EntityPropertyFlags ImageEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_IMAGE_URL;
    return requestedProperties;
}

void ImageEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
    EntityTreeElementExtraEncodeDataPointer modelTreeElementExtraEncodeData,
    EntityPropertyFlags& requestedProperties,
    EntityPropertyFlags& propertyFlags,
    EntityPropertyFlags& propertiesDidntFit,
    int& propertyCount,
    OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;
    // Using "Quad" shape as defined in ShapeEntityItem.cpp
    APPEND_ENTITY_PROPERTY(PROP_IMAGE_URL, _imageURL);
}

int ImageEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
    ReadBitstreamToTreeParams& args,
    EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
    bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_IMAGE_URL, QString, setImageURL);

    return bytesRead;
}

void ImageEntityItem::setUnscaledDimensions(const glm::vec3& value) {
    const float MAX_FLAT_DIMENSION = 0.0001f;
    if (value.y > MAX_FLAT_DIMENSION) {
        // enforce flatness in Y
        glm::vec3 newDimensions = value;
        newDimensions.y = MAX_FLAT_DIMENSION;
        EntityItem::setUnscaledDimensions(newDimensions);
    } else {
        EntityItem::setUnscaledDimensions(value);
    }
}

void ImageEntityItem::setImageURL(const QString& value) {
    withWriteLock([&] {
        if (_imageURL != value) {
            auto newURL = QUrl::fromUserInput(value);

            if (newURL.isValid()) {
                _imageURL = newURL.toDisplayString();
            } else {
                qCDebug(entities) << "Clearing image entity source URL since" << value << "cannot be parsed to a valid URL.";
            }
        }
    });
}

QString ImageEntityItem::getImageURL() const {
    return resultWithReadLock<QString>([&] {
        return _imageURL;
    });
}

void ImageEntityItem::computeShapeInfo(ShapeInfo& info) {
    // This will be called whenever DIRTY_SHAPE flag (set by dimension change, etc)
    // is set.

    EntityItem::computeShapeInfo(info);
}

// This value specifies how the shape should be treated by physics calculations.
ShapeType ImageEntityItem::getShapeType() const {
    return _collisionShapeType;
}

*/
