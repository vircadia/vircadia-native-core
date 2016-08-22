//
//  AssignmentAction.h
//  assignment-client/src/
//
//  Created by Seth Alves 2015-6-19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  http://bulletphysics.org/Bullet/BulletFull/classbtActionInterface.html

#ifndef hifi_AssignmentAction_h
#define hifi_AssignmentAction_h

#include <QUuid>
#include <EntityItem.h>

#include "EntityActionInterface.h"


class AssignmentAction : public EntityActionInterface, public ReadWriteLockable {
public:
    AssignmentAction(EntityActionType type, const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~AssignmentAction();

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
    virtual void setPosition(glm::vec3 position) override;
    virtual glm::quat getRotation() override;
    virtual void setRotation(glm::quat rotation) override;
    virtual glm::vec3 getLinearVelocity() override;
    virtual void setLinearVelocity(glm::vec3 linearVelocity) override;
    virtual glm::vec3 getAngularVelocity() override;
    virtual void setAngularVelocity(glm::vec3 angularVelocity) override;

    bool _active;
    EntityItemWeakPointer _ownerEntity;
};

#endif // hifi_AssignmentAction_h
