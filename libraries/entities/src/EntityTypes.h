//
//  EntityTypes.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityTypes_h
#define hifi_EntityTypes_h

#include <stdint.h>

#include <QHash>
#include <QString>

#include "EntitiesLogging.h"

class EntityItem;
using EntityItemPointer = std::shared_ptr<EntityItem>;
using EntityItemWeakPointer = std::weak_ptr<EntityItem>;

inline uint qHash(const EntityItemPointer& a, uint seed) {
    return qHash(a.get(), seed);
}

class EntityItemID;
class EntityItemProperties;
class ReadBitstreamToTreeParams;

typedef EntityItemPointer (*EntityTypeFactory)(const EntityItemID& entityID, const EntityItemProperties& properties);

class EntityTypes {
public:
    typedef enum EntityType_t {
        Unknown,
        Model,
        Box,
        Sphere,
        Light,
        Text,
        ParticleEffect,
        Zone,
        Web,
        Line,
        PolyVox,
        PolyLine,
        Shape,
        LAST = Shape
    } EntityType;

    static const QString& getEntityTypeName(EntityType entityType);
    static EntityTypes::EntityType getEntityTypeFromName(const QString& name);
    static bool registerEntityType(EntityType entityType, const char* name, EntityTypeFactory factoryMethod);
    static EntityItemPointer constructEntityItem(EntityType entityType, const EntityItemID& entityID, const EntityItemProperties& properties);
    static EntityItemPointer constructEntityItem(const unsigned char* data, int bytesToRead, ReadBitstreamToTreeParams& args);

private:
    static QMap<EntityType, QString> _typeToNameMap;
    static QMap<QString, EntityTypes::EntityType> _nameToTypeMap;
    static EntityTypeFactory _factories[LAST + 1];
    static bool _factoriesInitialized;
};


/// Macro for registering entity types. Make sure to add an element to the EntityType enum with your name, and your class should be
/// named NameEntityItem and must of a static method called factory that takes an EnityItemID, and EntityItemProperties and return a newly
/// constructed (heap allocated) instance of your type. e.g. The following prototype:
//        static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);
#define REGISTER_ENTITY_TYPE(x) bool x##Registration = \
            EntityTypes::registerEntityType(EntityTypes::x, #x, x##EntityItem::factory);


struct EntityRegistrationChecker {
    EntityRegistrationChecker(bool result, const char* debugMessage) {
        if (!result) {
            qCDebug(entities) << debugMessage;
        }
    }
};

/// Macro for registering entity types with an overloaded factory. Like using the REGISTER_ENTITY_TYPE macro: Make sure to add
/// an element to the EntityType enum with your name. But unlike  REGISTER_ENTITY_TYPE, your class can be named anything
/// so long as you provide a static method passed to the macro, that takes an EnityItemID, and EntityItemProperties and 
/// returns a newly constructed (heap allocated) instance of your type. e.g. The following prototype:
//        static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);
#define REGISTER_ENTITY_TYPE_WITH_FACTORY(x,y) static bool x##Registration = \
            EntityTypes::registerEntityType(EntityTypes::x, #x, y); \
            EntityRegistrationChecker x##RegistrationChecker( \
                x##Registration, \
                "UNEXPECTED: REGISTER_ENTITY_TYPE_WITH_FACTORY(" #x "," #y ") FAILED.!");


#endif // hifi_EntityTypes_h
