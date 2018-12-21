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

#include "EntityTypes.h"

#include <QtCore/QObject>

#include <ByteCountCoding.h>
#include <Octree.h>

#include "EntityItem.h"
#include "EntityItemProperties.h"
#include "EntitiesLogging.h"

#include "ShapeEntityItem.h"
#include "ModelEntityItem.h"
#include "ParticleEffectEntityItem.h"
#include "TextEntityItem.h"
#include "ImageEntityItem.h"
#include "WebEntityItem.h"
#include "LineEntityItem.h"
#include "PolyLineEntityItem.h"
#include "PolyVoxEntityItem.h"
#include "GridEntityItem.h"
#include "LightEntityItem.h"
#include "ZoneEntityItem.h"
#include "MaterialEntityItem.h"

QMap<EntityTypes::EntityType, QString> EntityTypes::_typeToNameMap;
QMap<QString, EntityTypes::EntityType> EntityTypes::_nameToTypeMap;
EntityTypeFactory EntityTypes::_factories[EntityTypes::NUM_TYPES];
bool EntityTypes::_factoriesInitialized = false;

const QString ENTITY_TYPE_NAME_UNKNOWN = "Unknown";

// Register Entity the default implementations of entity types here...
REGISTER_ENTITY_TYPE_WITH_FACTORY(Box, ShapeEntityItem::boxFactory)
REGISTER_ENTITY_TYPE_WITH_FACTORY(Sphere, ShapeEntityItem::sphereFactory)
REGISTER_ENTITY_TYPE(Shape)
REGISTER_ENTITY_TYPE(Model)
REGISTER_ENTITY_TYPE(Text)
REGISTER_ENTITY_TYPE(Image)
REGISTER_ENTITY_TYPE(Web)
REGISTER_ENTITY_TYPE(ParticleEffect)
REGISTER_ENTITY_TYPE(Line)
REGISTER_ENTITY_TYPE(PolyLine)
REGISTER_ENTITY_TYPE(PolyVox)
REGISTER_ENTITY_TYPE(Grid)
REGISTER_ENTITY_TYPE(Light)
REGISTER_ENTITY_TYPE(Zone)
REGISTER_ENTITY_TYPE(Material)

bool EntityTypes::typeIsValid(EntityType type) {
    return type > EntityType::Unknown && type <= EntityType::NUM_TYPES;
}

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
    if (name.size() > 0 && name[0].isLower()) {
        qCDebug(entities) << "Entity types must start with an uppercase letter. Please change the type" << name;
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
    if (entityType >= 0 && entityType < NUM_TYPES) {
        _factories[entityType] = factoryMethod;
        return true;
    }
    return false;
}

EntityItemPointer EntityTypes::constructEntityItem(EntityType entityType, const EntityItemID& entityID,
                                                    const EntityItemProperties& properties) {
    EntityItemPointer newEntityItem = NULL;
    EntityTypeFactory factory = NULL;
    if (entityType >= 0 && entityType < NUM_TYPES) {
        factory = _factories[entityType];
    }
    if (factory) {
        auto mutableProperties = properties;
        mutableProperties.markAllChanged();
        newEntityItem = factory(entityID, mutableProperties);
        newEntityItem->moveToThread(qApp->thread());
    }
    return newEntityItem;
}

void EntityTypes::extractEntityTypeAndID(const unsigned char* data, int dataLength, EntityTypes::EntityType& typeOut, QUuid& idOut) {

    // Header bytes
    //    object ID [16 bytes]
    //    ByteCountCoded(type code) [~1 byte]
    //    last edited [8 bytes]
    //    ByteCountCoded(last_edited to last_updated delta) [~1-8 bytes]
    //    PropertyFlags<>( everything ) [1-2 bytes]
    // ~27-35 bytes...
    const int MINIMUM_HEADER_BYTES = 27;

    if (dataLength >= MINIMUM_HEADER_BYTES) {
        int bytesRead = 0;
        QByteArray originalDataBuffer = QByteArray::fromRawData((const char*)data, dataLength);

        // id
        QByteArray encodedID = originalDataBuffer.mid(bytesRead, NUM_BYTES_RFC4122_UUID); // maximum possible size
        idOut = QUuid::fromRfc4122(encodedID);
        bytesRead += encodedID.size();

        // type
        QByteArray encodedType = originalDataBuffer.mid(bytesRead); // maximum possible size
        ByteCountCoded<quint32> typeCoder = encodedType;
        encodedType = typeCoder; // determine true length
        quint32 type = typeCoder;
        typeOut = (EntityTypes::EntityType)type;
    }
}

EntityItemPointer EntityTypes::constructEntityItem(const unsigned char* data, int bytesToRead) {
    QUuid id;
    EntityTypes::EntityType type = EntityTypes::Unknown;
    extractEntityTypeAndID(data, bytesToRead, type, id);
    if (type > EntityTypes::Unknown && type <= EntityTypes::NUM_TYPES) {
        EntityItemID tempEntityID(id);
        EntityItemProperties tempProperties;
        return constructEntityItem(type, tempEntityID, tempProperties);
    }
    return nullptr;
}

EntityItemPointer EntityTypes::constructEntityItem(const QUuid& id, const EntityItemProperties& properties) {
    return constructEntityItem(properties.getType(), id, properties);
}
