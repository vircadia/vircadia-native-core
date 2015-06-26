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
    AssignmentAction(EntityActionType type, QUuid id, EntityItemPointer ownerEntity);
    virtual ~AssignmentAction();

    const QUuid& getID() const { return _id; }
    virtual EntityActionType getType() { return _type; }
    virtual void removeFromSimulation(EntitySimulation* simulation) const;
    virtual const EntityItemPointer& getOwnerEntity() const { return _ownerEntity; }
    virtual void setOwnerEntity(const EntityItemPointer ownerEntity) { _ownerEntity = ownerEntity; }
    virtual bool updateArguments(QVariantMap arguments) { assert(false); return false; }

    virtual QByteArray serialize();
    virtual void deserialize(QByteArray serializedArguments);

private:
    QUuid _id;
    EntityActionType _type;
    QByteArray _data;

protected:
    virtual glm::vec3 getPosition() { assert(false); return glm::vec3(0.0f); }
    virtual void setPosition(glm::vec3 position) { assert(false); }
    virtual glm::quat getRotation() { assert(false); return glm::quat(); }
    virtual void setRotation(glm::quat rotation) { assert(false); }
    virtual glm::vec3 getLinearVelocity() { assert(false); return glm::vec3(0.0f); }
    virtual void setLinearVelocity(glm::vec3 linearVelocity) { assert(false); }
    virtual glm::vec3 getAngularVelocity() { assert(false); return glm::vec3(0.0f); }
    virtual void setAngularVelocity(glm::vec3 angularVelocity) { assert(false); }

    bool _active;
    EntityItemPointer _ownerEntity;
};

#endif // hifi_AssignmentAction_h
