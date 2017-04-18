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
    virtual glm::vec3 getPosition() override;
    virtual glm::quat getRotation() override;
    virtual glm::vec3 getLinearVelocity() override;
    virtual void setLinearVelocity(glm::vec3 linearVelocity) override;
    virtual glm::vec3 getAngularVelocity() override;
    virtual void setAngularVelocity(glm::vec3 angularVelocity) override;

    bool _active;
    EntityItemWeakPointer _ownerEntity;
};

#endif // hifi_AssignmentDynamic_h
