//
//  ObjectConstraintHinge.h
//  libraries/physics/src
//
//  Created by Seth Alves 2017-4-11
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ObjectConstraintHinge_h
#define hifi_ObjectConstraintHinge_h

#include "ObjectConstraint.h"

// http://bulletphysics.org/Bullet/BulletFull/classbtHingeConstraint.html

class ObjectConstraintHinge : public ObjectConstraint {
public:
    ObjectConstraintHinge(const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~ObjectConstraintHinge();

    virtual bool updateArguments(QVariantMap arguments) override;
    virtual QVariantMap getArguments() override;

    virtual QByteArray serialize() const override;
    virtual void deserialize(QByteArray serializedArguments) override;

    virtual QList<btRigidBody*> getRigidBodies() override;
    virtual btTypedConstraint* getConstraint() override;

protected:
    static const uint16_t constraintVersion;

    void updateHinge();

    glm::vec3 _pivotInA;
    glm::vec3 _axisInA;

    EntityItemID _otherEntityID;
    glm::vec3 _pivotInB;
    glm::vec3 _axisInB;

    float _low { -2.0f * PI };
    float _high { 2.0f * PI };
    float _softness { 0.9f };
    float _biasFactor { 0.3f };
    float _relaxationFactor { 1.0f };
    float _motorVelocity { 0.0f };
};

#endif // hifi_ObjectConstraintHinge_h
