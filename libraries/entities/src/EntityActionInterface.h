//
//  EntityItem.h
//  libraries/entities/src
//
//  Created by Seth Alves on 2015-6-2
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityActionInterface_h
#define hifi_EntityActionInterface_h

class EntityActionInterface {
 public:
    virtual ~EntityActionInterface();
    virtual const QUuid& getID() const = 0;
    virtual const EntityItemPointer& getOwnerEntity() const = 0;
    virtual void setOwnerEntity(const EntityItemPointer ownerEntity) = 0;
    virtual QByteArray serialize() = 0;
    static EntityActionInterface* deserialize(EntityItemPointer ownerEntity, QByteArray data);
};

#endif // hifi_EntityActionInterface_h
