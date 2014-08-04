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

class EntityItem;
class EntityItemID;
class EntityItemProperties;
class ReadBitstreamToTreeParams;

class EntityTypes {
public:
    typedef enum EntityType {
        Base,
        Model,
        Box,
        Sphere,
        Plane,
        Cylinder,
        Pyramid
    } EntityType_t;

    static const QString& getEntityTypeName(EntityType_t entityType);
    static bool registerEntityType(EntityType_t entityType, const QString& name);
    static EntityTypes::EntityType_t getEntityTypeFromName(const QString& name);

    static EntityItem* constructEntityItem(EntityType_t entityType, const EntityItemID& entityID, const EntityItemProperties& properties);
    static EntityItem* constructEntityItem(const unsigned char* data, int bytesToRead, ReadBitstreamToTreeParams& args);
    static bool decodeEntityEditPacket(const unsigned char* data, int bytesToRead, int& processedBytes, 
                                        EntityItemID& entityID, EntityItemProperties& properties);
private:
    static QHash<EntityType_t, QString> _typeNameHash;
};

#endif // hifi_EntityTypes_h
