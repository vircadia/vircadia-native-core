//
//  ObjectConstraintBallSocket.h
//  libraries/physics/src
//
//  Created by Seth Alves 2017-4-29
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ObjectConstraintBallSocket_h
#define hifi_ObjectConstraintBallSocket_h

#include "ObjectConstraint.h"

// http://bulletphysics.org/Bullet/BulletFull/classbtBallSocketConstraint.html

class ObjectConstraintBallSocket : public ObjectConstraint {
public:
    ObjectConstraintBallSocket(const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~ObjectConstraintBallSocket();

    virtual void prepareForPhysicsSimulation() override;

    virtual bool updateArguments(QVariantMap arguments) override;
    virtual QVariantMap getArguments() override;

    virtual QByteArray serialize() const override;
    virtual void deserialize(QByteArray serializedArguments) override;

    virtual QList<btRigidBody*> getRigidBodies() override;
    virtual btTypedConstraint* getConstraint() override;

protected:
    static const uint16_t constraintVersion;

    void updateBallSocket();

    glm::vec3 _pivotInA;
    glm::vec3 _pivotInB;
};

#endif // hifi_ObjectConstraintBallSocket_h
