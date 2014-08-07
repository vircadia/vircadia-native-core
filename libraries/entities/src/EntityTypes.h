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

#include <OctreeRenderer.h> // for RenderArgs

class EntityItem;
class EntityItemID;
class EntityItemProperties;
class ReadBitstreamToTreeParams;

typedef EntityItem* (*EntityTypeFactory)(const EntityItemID& entityID, const EntityItemProperties& properties);
typedef void (*EntityTypeRenderer)(EntityItem* entity, RenderArgs* args);

class EntityTypes {
public:
    typedef enum EntityType {
        Unknown,
        Model,
        Box,
        Sphere,
        Plane,
        Cylinder,
        Pyramid,
        LAST = Pyramid
    } EntityType_t;

    static const QString& getEntityTypeName(EntityType_t entityType);
    static EntityTypes::EntityType_t getEntityTypeFromName(const QString& name);
    static bool registerEntityType(EntityType_t entityType, const char* name, EntityTypeFactory factoryMethod);
    static EntityItem* constructEntityItem(EntityType_t entityType, const EntityItemID& entityID, const EntityItemProperties& properties);
    static EntityItem* constructEntityItem(const unsigned char* data, int bytesToRead, ReadBitstreamToTreeParams& args);
    static bool decodeEntityEditPacket(const unsigned char* data, int bytesToRead, int& processedBytes, 
                                        EntityItemID& entityID, EntityItemProperties& properties);

    static bool registerEntityTypeRenderer(EntityType_t entityType, EntityTypeRenderer renderMethod);
    static void renderEntityItem(EntityItem* entityItem, RenderArgs* args);

private:
    static QMap<EntityType_t, QString> _typeToNameMap;
    static QMap<QString, EntityTypes::EntityType_t> _nameToTypeMap;
    static EntityTypeFactory _factories[LAST];
    static bool _factoriesInitialized;
    static EntityTypeRenderer _renderers[LAST];
    static bool _renderersInitialized;
};


/// Macro for registering entity types. Make sure to add an element to the EntityType enum with your name, and your class should be
/// named NameEntityItem and must of a static method called factory that takes an EnityItemID, and EntityItemProperties and return a newly
/// constructed (heap allocated) instance of your type. e.g. The following prototype:
//        static EntityItem* factory(const EntityItemID& entityID, const EntityItemProperties& properties);
#define REGISTER_ENTITY_TYPE(x) static bool x##Registration = \
            EntityTypes::registerEntityType(EntityTypes::x, #x, x##EntityItem::factory);

#define REGISTER_ENTITY_TYPE_RENDERER(x,y) EntityTypes::registerEntityTypeRenderer(EntityTypes::x, y);


#endif // hifi_EntityTypes_h
