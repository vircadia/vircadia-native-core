//
//  ObjectConstraintConeTwist.h
//  libraries/physics/src
//
//  Created by Seth Alves 2017-4-23
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ObjectConstraintConeTwist_h
#define hifi_ObjectConstraintConeTwist_h

#include "ObjectConstraint.h"

// http://bulletphysics.org/Bullet/BulletFull/classbtConeTwistConstraint.html

class ObjectConstraintConeTwist : public ObjectConstraint {
public:
    ObjectConstraintConeTwist(const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~ObjectConstraintConeTwist();

    virtual void prepareForPhysicsSimulation() override;

    virtual bool updateArguments(QVariantMap arguments) override;
    virtual QVariantMap getArguments() override;

    virtual QByteArray serialize() const override;
    virtual void deserialize(QByteArray serializedArguments) override;

    virtual QList<btRigidBody*> getRigidBodies() override;
    virtual btTypedConstraint* getConstraint() override;

protected:
    static const uint16_t constraintVersion;

    void updateConeTwist();

    glm::vec3 _pivotInA;
    glm::vec3 _axisInA;

    glm::vec3 _pivotInB;
    glm::vec3 _axisInB;

    float _swingSpan1 { TWO_PI };
    float _swingSpan2 { TWO_PI };;
    float _twistSpan { TWO_PI };;
};

#endif // hifi_ObjectConstraintConeTwist_h
