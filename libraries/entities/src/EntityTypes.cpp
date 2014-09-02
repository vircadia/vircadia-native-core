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

#include "BoxEntityItem.h"
#include "ModelEntityItem.h"
#include "SphereEntityItem.h"

QMap<EntityTypes::EntityType, QString> EntityTypes::_typeToNameMap;
QMap<QString, EntityTypes::EntityType> EntityTypes::_nameToTypeMap;
EntityTypeFactory EntityTypes::_factories[EntityTypes::LAST];
bool EntityTypes::_factoriesInitialized = false;

const QString ENTITY_TYPE_NAME_UNKNOWN = "Unknown";

// Register Entity the default implementations of entity types here...
REGISTER_ENTITY_TYPE(Model)
REGISTER_ENTITY_TYPE(Box)
REGISTER_ENTITY_TYPE(Sphere)


const QString& EntityTypes::getEntityTypeName(EntityType entityType) {
    QMap<EntityType, QString>::iterator matchedTypeName = _typeToNameMap.find(entityType);
    if (matchedTypeName != _typeToNameMap.end()) {
        return matchedTypeName.value();
    }
    return ENTITY_TYPE_NAME_UNKNOWN;
}

EntityTypes::EntityType EntityTypes::getEntityTypeFromName(const QString& name) {
    QMap<QString, EntityTypes::EntityType>::iterator matchedTypeName = _nameToTypeMap.find(name);
    if (matchedTypeName != _nameToTypeMap.end()) {
        return matchedTypeName.value();
    }
    return Unknown;
}

bool EntityTypes::registerEntityType(EntityType entityType, const char* name, EntityTypeFactory factoryMethod) {
    _typeToNameMap[entityType] = name;
    _nameToTypeMap[name] = entityType;
    if (!_factoriesInitialized) {
        memset(&_factories,0,sizeof(_factories));
        _factoriesInitialized = true;
    }
    _factories[entityType] = factoryMethod;
    return true;
}

EntityItem* EntityTypes::constructEntityItem(EntityType entityType, const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItem* newEntityItem = NULL;
    EntityTypeFactory factory = NULL;
    if (entityType >= 0 && entityType <= LAST) {
        factory = _factories[entityType];
    }
    if (factory) {
        // NOTE: if someone attempts to create an entity with properties that do not include a proper "created" time
        // then set the created time to now
        if (!properties.hasCreatedTime()) {
            EntityItemProperties mutableProperties = properties;
            mutableProperties.setCreated(usecTimestampNow());
            newEntityItem = factory(entityID, mutableProperties);
        } else {
            newEntityItem = factory(entityID, properties);
        }
    }
    return newEntityItem;
}

EntityItem* EntityTypes::constructEntityItem(const unsigned char* data, int bytesToRead,
            ReadBitstreamToTreeParams& args) {

    if (args.bitstreamVersion < VERSION_ENTITIES_SUPPORT_SPLIT_MTU) {
        EntityItemID tempEntityID;
        EntityItemProperties tempProperties;
        return constructEntityItem(Model, tempEntityID, tempProperties);
    }

    // Header bytes
    //    object ID [16 bytes]
    //    ByteCountCoded(type code) [~1 byte]
    //    last edited [8 bytes]
    //    ByteCountCoded(last_edited to last_updated delta) [~1-8 bytes]
    //    PropertyFlags<>( everything ) [1-2 bytes]
    // ~27-35 bytes...
    const int MINIMUM_HEADER_BYTES = 27;

    int bytesRead = 0;
    if (bytesToRead >= MINIMUM_HEADER_BYTES) {
        int originalLength = bytesToRead;
        QByteArray originalDataBuffer((const char*)data, originalLength);

        // id
        QByteArray encodedID = originalDataBuffer.mid(bytesRead, NUM_BYTES_RFC4122_UUID); // maximum possible size
        QUuid actualID = QUuid::fromRfc4122(encodedID);
        bytesRead += encodedID.size();

        // type
        QByteArray encodedType = originalDataBuffer.mid(bytesRead); // maximum possible size
        ByteCountCoded<quint32> typeCoder = encodedType;
        encodedType = typeCoder; // determine true length
        bytesRead += encodedType.size();
        quint32 type = typeCoder;
        EntityTypes::EntityType entityType = (EntityTypes::EntityType)type;
        
        EntityItemID tempEntityID(actualID);
        EntityItemProperties tempProperties;
        tempProperties.setCreated(usecTimestampNow()); // this is temporary...

        return constructEntityItem(entityType, tempEntityID, tempProperties);
    }
    
    return NULL;
}
