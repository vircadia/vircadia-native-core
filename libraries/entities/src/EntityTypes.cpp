//
//  EntityTypes.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QObject>

#include <ByteCountCoding.h>
#include <Octree.h>

#include "EntityItem.h"
#include "EntityItemProperties.h"
#include "EntityTypes.h"

QHash<EntityTypes::EntityType_t, QString> EntityTypes::_typeNameHash;

const QString UNKNOWN_EntityType_t_NAME = "Unknown";
const QString& EntityTypes::getEntityTypeName(EntityType_t entityType) {
    QHash<EntityType_t, QString>::iterator matchedTypeName = _typeNameHash.find(entityType);
    return matchedTypeName != _typeNameHash.end() ? matchedTypeName.value() : UNKNOWN_EntityType_t_NAME;
}

bool EntityTypes::registerEntityType(EntityType_t entityType, const QString& name) {
    _typeNameHash.insert(entityType, name);
    return true;
}

EntityTypes::EntityType_t EntityTypes::getEntityTypeFromName(const QString& name) {
    return Base; // TODO: support registration
}

EntityItem* EntityTypes::constructEntityItem(EntityType_t entityType, const EntityItemID& entityID, const EntityItemProperties& properties) {

qDebug() << "EntityTypes::constructEntityItem(EntityType_t entityType, const EntityItemID& entityID, const EntityItemProperties& properties)";

    EntityItem* newEntityItem = NULL;

    // switch statement for now, needs to support registration of constructor
    switch (entityType) {
        // Base, // ??? not supported?
        case Model:
            newEntityItem = new ModelEntityItem(entityID, properties); 
        break;

        case Particle:
            newEntityItem = new ParticleEntityItem(entityID, properties); 
        break;

        case Box:
            newEntityItem = new BoxEntityItem(entityID, properties); 
        break;

        case Sphere:
            newEntityItem = new SphereEntityItem(entityID, properties); 
        break;

        case Plane:
            newEntityItem = new PlaneEntityItem(entityID, properties); 
        break;

        case Cylinder:
            newEntityItem = new CylinderEntityItem(entityID, properties); 
        break;

        case Pyramid:
            newEntityItem = new PyramidEntityItem(entityID, properties); 
        break;

        default:
            newEntityItem = new ModelEntityItem(entityID, properties); 
        break;
    }
    return newEntityItem;
}

EntityItem* EntityTypes::constructEntityItem(const unsigned char* data, int bytesToRead,
            ReadBitstreamToTreeParams& args) {

qDebug() << "EntityTypes::constructEntityItem(const unsigned char* data, int bytesToRead)";

    //qDebug() << "EntityTypes::constructEntityItem(const unsigned char* data, int bytesToRead).... CALLED BUT NOT IMPLEMENTED!!!";
    
    if (args.bitstreamVersion < VERSION_ENTITIES_SUPPORT_SPLIT_MTU) {
        qDebug() << "EntityTypes::constructEntityItem(const unsigned char* data, int bytesToRead).... OLD BITSTREAM!!!";

        EntityItemID tempEntityID;
        EntityItemProperties tempProperties;
        return new ModelEntityItem(tempEntityID, tempProperties);
    }

    // Header bytes
    //    object ID [16 bytes]
    //    ByteCountCoded(type code) [~1 byte]
    //    last edited [8 bytes]
    //    ByteCountCoded(last_edited to last_updated delta) [~1-8 bytes]
    //    PropertyFlags<>( everything ) [1-2 bytes]
    // ~27-35 bytes...
    const int MINIMUM_HEADER_BYTES = 27; // TODO: this is not correct, we don't yet have 16 byte IDs

    int bytesRead = 0;
    if (bytesToRead >= MINIMUM_HEADER_BYTES) {
        int originalLength = bytesToRead;
        QByteArray originalDataBuffer((const char*)data, originalLength);

        // id
        QByteArray encodedID = originalDataBuffer.mid(bytesRead); // maximum possible size
        ByteCountCoded<quint32> idCoder = encodedID;
        encodedID = idCoder; // determine true length
        bytesRead += encodedID.size();

        // type
        QByteArray encodedType = originalDataBuffer.mid(bytesRead); // maximum possible size
        ByteCountCoded<quint32> typeCoder = encodedType;
        encodedType = typeCoder; // determine true length
        bytesRead += encodedType.size();
        quint32 type = typeCoder;
        EntityTypes::EntityType_t entityType = (EntityTypes::EntityType_t)type;
        
        EntityItemID tempEntityID;
        EntityItemProperties tempProperties;

        qDebug() << "EntityTypes::constructEntityItem(data, bytesToRead).... NEW BITSTREAM!!! entityType=" << entityType;
        
        return constructEntityItem(entityType, tempEntityID, tempProperties);
    }
    

    return NULL; // TODO Implement this for real!
}


// TODO: 
//   how to handle lastEdited?
//   how to handle lastUpdated?
//   consider handling case where no properties are included... we should just ignore this packet...
bool EntityTypes::decodeEntityEditPacket(const unsigned char* data, int bytesToRead, int& processedBytes,
                        EntityItemID& entityID, EntityItemProperties& properties) {
    bool valid = false;

    bool wantDebug = false;
    if (wantDebug) {
        qDebug() << "EntityItem EntityItem::decodeEntityEditPacket() bytesToRead=" << bytesToRead;
    }

    const unsigned char* dataAt = data;
    processedBytes = 0;

    // the first part of the data is an octcode, this is a required element of the edit packet format, but we don't
    // actually use it, we do need to skip it and read to the actual data we care about.
    int octets = numberOfThreeBitSectionsInCode(data);
    int bytesToReadOfOctcode = bytesRequiredForCodeLength(octets);

    if (wantDebug) {
        qDebug() << "EntityItem EntityItem::decodeEntityEditPacket() bytesToReadOfOctcode=" << bytesToReadOfOctcode;
    }

    // we don't actually do anything with this octcode...
    dataAt += bytesToReadOfOctcode;
    processedBytes += bytesToReadOfOctcode;
    
    // Edit packets have a last edited time stamp immediately following the octcode.
    // NOTE: the edit times have been set by the editor to match out clock, so we don't need to adjust
    // these times for clock skew at this point.
    quint64 lastEdited;
    memcpy(&lastEdited, dataAt, sizeof(lastEdited));
    dataAt += sizeof(lastEdited);
    processedBytes += sizeof(lastEdited);

    // encoded id
    QByteArray encodedID((const char*)dataAt, (bytesToRead - processedBytes));
    ByteCountCoded<quint32> idCoder = encodedID;
    quint32 editID = idCoder;
    encodedID = idCoder; // determine true bytesToRead
    dataAt += encodedID.size();
    processedBytes += encodedID.size();

    if (wantDebug) {
        qDebug() << "EntityItem EntityItem::decodeEntityEditPacket() editID=" << editID;
    }

    bool isNewEntityItem = (editID == NEW_ENTITY);

    if (isNewEntityItem) {
        // If this is a NEW_ENTITY, then we assume that there's an additional uint32_t creatorToken, that
        // we want to send back to the creator as an map to the actual id

        QByteArray encodedToken((const char*)dataAt, (bytesToRead - processedBytes));
        ByteCountCoded<quint32> tokenCoder = encodedToken;
        quint32 creatorTokenID = tokenCoder;
        encodedToken = tokenCoder; // determine true bytesToRead
        dataAt += encodedToken.size();
        processedBytes += encodedToken.size();

        //newEntityItem.setCreatorTokenID(creatorTokenID);
        //newEntityItem._newlyCreated = true;
        
        entityID.id = NEW_ENTITY;
        entityID.creatorTokenID = creatorTokenID;
        entityID.isKnownID = false;

        valid = true;

    } else {
        entityID.id = editID;
        entityID.creatorTokenID = UNKNOWN_ENTITY_TOKEN;
        entityID.isKnownID = true;
        valid = true;
    }
    
    // Entity Type...
    QByteArray encodedType((const char*)dataAt, (bytesToRead - processedBytes));
    ByteCountCoded<quint32> typeCoder = encodedType;
    quint32 entityTypeCode = typeCoder;
    properties.setType((EntityTypes::EntityType_t)entityTypeCode);
    encodedType = typeCoder; // determine true bytesToRead
    dataAt += encodedType.size();
    processedBytes += encodedType.size();
    

    // Update Delta - when was this item updated relative to last edit... this really should be 0
    // TODO: Should we get rid of this in this in edit packets, since this has to always be 0?
    // TODO: do properties need to handle lastupdated???

    // last updated is stored as ByteCountCoded delta from lastEdited
    QByteArray encodedUpdateDelta((const char*)dataAt, (bytesToRead - processedBytes));
    ByteCountCoded<quint64> updateDeltaCoder = encodedUpdateDelta;
    quint64 updateDelta = updateDeltaCoder;
    quint64 lastUpdated = lastEdited + updateDelta; // don't adjust for clock skew since we already did that for lastEdited
    encodedUpdateDelta = updateDeltaCoder; // determine true bytesToRead
    dataAt += encodedUpdateDelta.size();
    processedBytes += encodedUpdateDelta.size();
    
    // Property Flags...
    QByteArray encodedPropertyFlags((const char*)dataAt, (bytesToRead - processedBytes));
    EntityPropertyFlags propertyFlags = encodedPropertyFlags;
    dataAt += propertyFlags.getEncodedLength();
    processedBytes += propertyFlags.getEncodedLength();
    

    // PROP_POSITION
    if (propertyFlags.getHasProperty(PROP_POSITION)) {
        glm::vec3 position;
        memcpy(&position, dataAt, sizeof(position));
        dataAt += sizeof(position);
        processedBytes += sizeof(position);
        properties.setPosition(position);
    }
    
    // PROP_RADIUS
    if (propertyFlags.getHasProperty(PROP_RADIUS)) {
        float radius;
        memcpy(&radius, dataAt, sizeof(radius));
        dataAt += sizeof(radius);
        processedBytes += sizeof(radius);
        properties.setRadius(radius);
    }

    // PROP_ROTATION
    if (propertyFlags.getHasProperty(PROP_ROTATION)) {
        glm::quat rotation;
        int bytes = unpackOrientationQuatFromBytes(dataAt, rotation);
        dataAt += bytes;
        processedBytes += bytes;
        properties.setRotation(rotation);
    }

    // PROP_SHOULD_BE_DELETED
    if (propertyFlags.getHasProperty(PROP_SHOULD_BE_DELETED)) {
        bool shouldBeDeleted;
        memcpy(&shouldBeDeleted, dataAt, sizeof(shouldBeDeleted));
        dataAt += sizeof(shouldBeDeleted);
        processedBytes += sizeof(shouldBeDeleted);
        properties.setShouldBeDeleted(shouldBeDeleted);
    }

    // PROP_SCRIPT
    //     script would go here...
    
    
#ifdef HIDE_SUBCLASS_METHODS    
    // PROP_COLOR
    if (propertyFlags.getHasProperty(PROP_COLOR)) {
        xColor color;
        memcpy(&color, dataAt, sizeof(color));
        dataAt += sizeof(color);
        processedBytes += sizeof(color);
        properties.setColor(color);
    }

    // PROP_MODEL_URL
    if (propertyFlags.getHasProperty(PROP_MODEL_URL)) {
    
        // TODO: fix to new format...
        uint16_t modelURLbytesToRead;
        memcpy(&modelURLbytesToRead, dataAt, sizeof(modelURLbytesToRead));
        dataAt += sizeof(modelURLbytesToRead);
        processedBytes += sizeof(modelURLbytesToRead);
        QString modelURLString((const char*)dataAt);
        dataAt += modelURLbytesToRead;
        processedBytes += modelURLbytesToRead;

        properties.setModelURL(modelURLString);
    }

    // PROP_ANIMATION_URL
    if (propertyFlags.getHasProperty(PROP_ANIMATION_URL)) {
        // animationURL
        uint16_t animationURLbytesToRead;
        memcpy(&animationURLbytesToRead, dataAt, sizeof(animationURLbytesToRead));
        dataAt += sizeof(animationURLbytesToRead);
        processedBytes += sizeof(animationURLbytesToRead);
        QString animationURLString((const char*)dataAt);
        dataAt += animationURLbytesToRead;
        processedBytes += animationURLbytesToRead;
        properties.setAnimationURL(animationURLString);
    }        

    // PROP_ANIMATION_FPS
    if (propertyFlags.getHasProperty(PROP_ANIMATION_FPS)) {
        float animationFPS;
        memcpy(&animationFPS, dataAt, sizeof(animationFPS));
        dataAt += sizeof(animationFPS);
        processedBytes += sizeof(animationFPS);
        properties.setAnimationFPS(animationFPS);
    }

    // PROP_ANIMATION_FRAME_INDEX
    if (propertyFlags.getHasProperty(PROP_ANIMATION_FRAME_INDEX)) {
        float animationFrameIndex; // we keep this as a float and round to int only when we need the exact index
        memcpy(&animationFrameIndex, dataAt, sizeof(animationFrameIndex));
        dataAt += sizeof(animationFrameIndex);
        processedBytes += sizeof(animationFrameIndex);
        properties.setAnimationFrameIndex(animationFrameIndex);
    }

    // PROP_ANIMATION_PLAYING
    if (propertyFlags.getHasProperty(PROP_ANIMATION_PLAYING)) {
        bool animationIsPlaying;
        memcpy(&animationIsPlaying, dataAt, sizeof(animationIsPlaying));
        dataAt += sizeof(animationIsPlaying);
        processedBytes += sizeof(animationIsPlaying);
        properties.setAnimationIsPlaying(animationIsPlaying);
    }
#endif

    const bool wantDebugging = false;
    if (wantDebugging) {
        qDebug("EntityItem::fromEditPacket()...");
        qDebug() << "   EntityItem id in packet:" << editID;
        //newEntityItem.debugDump();
    }
    
    return valid;
}


bool registered = EntityTypes::registerEntityType(EntityTypes::Base, "Base")
                    && EntityTypes::registerEntityType(EntityTypes::Model, "Model"); // TODO: move this to model subclass


