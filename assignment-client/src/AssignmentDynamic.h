//
//  AssignmentDynamic.h
//  assignment-client/src/
//
//  Created by Seth Alves 2015-6-19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  http://bulletphysics.org/Bullet/BulletFull/classbtDynamicInterface.html

#ifndef hifi_AssignmentDynamic_h
#define hifi_AssignmentDynamic_h

#include <QUuid>
#include <EntityItem.h>

#include "EntityDynamicInterface.h"


class AssignmentDynamic : public EntityDynamicInterface, public ReadWriteLockable {
public:
    AssignmentDynamic(EntityDynamicType type, const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~AssignmentDynamic();

    virtual void remapIDs(QHash<EntityItemID, EntityItemID>& map) override {};

    virtual void removeFromSimulation(EntitySimulationPointer simulation) const override;
    virtual EntityItemWeakPointer getOwnerEntity() const override { return _ownerEntity; }
    virtual void setOwnerEntity(const EntityItemPointer ownerEntity) override { _ownerEntity = ownerEntity; }
    virtual bool updateArguments(QVariantMap arguments) override;
    virtual QVariantMap getArguments() override;

    virtual QByteArray serialize() const override;
    virtual void deserialize(QByteArray serializedArguments) override;

private:
    QByteArray _data;

protected:
    bool _active;
    EntityItemWeakPointer _ownerEntity;
};

#endif // hifi_AssignmentDynamic_h
