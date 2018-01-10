//
//  FlatImageEntity.cpp
//  libraries/entities/src
//
//  Created by Elisa Lupin-Jimenez on 1/3/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

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
#include "FlatImageEntity.h"

const QString FlatImageEntity::DEFAULT_IMAGE_URL = QString("");

EntityItemPointer FlatImageEntity::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity(new FlatImageEntity(entityID), [](EntityItem* ptr) { ptr->deleteLater(); });
    entity->setProperties(properties);
    return entity;
}

FlatImageEntity::FlatImageEntity(const EntityItemID& entityItemID) : EntityItem(entityItemID) {
    _type = EntityTypes::Image;
}

EntityItemProperties FlatImageEntity::getProperties(EntityPropertyFlags desiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties); // get the properties from our base class
    properties.setShape("Quad");
    return properties;
}

bool FlatImageEntity::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    //SET_ENTITY_PROPERTY_FROM_PROPERTIES(shape, setShape);

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qCDebug(entities) << "FlatImageEntity::setProperties() AFTER update... edited AGO=" << elapsed <<
                "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEdited(properties.getLastEdited());
    }
    return somethingChanged;
}

// TODO: eventually only include properties changed since the params.nodeData->getLastTimeBagEmpty() time
EntityPropertyFlags FlatImageEntity::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);

    return requestedProperties;
}

void FlatImageEntity::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
    EntityTreeElementExtraEncodeDataPointer modelTreeElementExtraEncodeData,
    EntityPropertyFlags& requestedProperties,
    EntityPropertyFlags& propertyFlags,
    EntityPropertyFlags& propertiesDidntFit,
    int& propertyCount,
    OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;
    // Using "Quad" shape as defined in ShapeEntityItem.cpp
    APPEND_ENTITY_PROPERTY(PROP_SHAPE, "Quad");
}

int FlatImageEntity::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
    ReadBitstreamToTreeParams& args,
    EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
    bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    return bytesRead;
}

void ShapeEntityItem::setUnscaledDimensions(const glm::vec3& value) {
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

QString FlatImageEntity::getImageURL() const {
    return resultWithReadLock<QString>([&] {
        return _imageURL;
    });
}

