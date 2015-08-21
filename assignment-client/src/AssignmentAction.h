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


class AssignmentAction : public EntityActionInterface {
public:
    AssignmentAction(EntityActionType type, const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~AssignmentAction();

    virtual void removeFromSimulation(EntitySimulation* simulation) const;
    virtual EntityItemWeakPointer getOwnerEntity() const { return _ownerEntity; }
    virtual void setOwnerEntity(const EntityItemPointer ownerEntity) { _ownerEntity = ownerEntity; }
    virtual bool updateArguments(QVariantMap arguments);
    virtual QVariantMap getArguments();

    virtual QByteArray serialize() const;
    virtual void deserialize(QByteArray serializedArguments);

private:
    QByteArray _data;

protected:
    virtual glm::vec3 getPosition();
    virtual void setPosition(glm::vec3 position);
    virtual glm::quat getRotation();
    virtual void setRotation(glm::quat rotation);
    virtual glm::vec3 getLinearVelocity();
    virtual void setLinearVelocity(glm::vec3 linearVelocity);
    virtual glm::vec3 getAngularVelocity();
    virtual void setAngularVelocity(glm::vec3 angularVelocity);

    bool _active;
    EntityItemWeakPointer _ownerEntity;
};

#endif // hifi_AssignmentAction_h
