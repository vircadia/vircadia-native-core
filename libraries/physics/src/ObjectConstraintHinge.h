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

    virtual void prepareForPhysicsSimulation() override;

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

    float _low { -TWO_PI };
    float _high { TWO_PI };

    // https://gamedev.stackexchange.com/questions/71436/what-are-the-parameters-for-bthingeconstraintsetlimit
    //
    // softness: a negative measure of the friction that determines how much the hinge rotates for a given force. A high
    //           softness would make the hinge rotate easily like it's oiled then.
    // biasFactor: an offset for the relaxed rotation of the hinge. It won't be right in the middle of the low and high angles
    //           anymore. 1.0f is the neural value.
    // relaxationFactor: a measure of how much force is applied internally to bring the hinge in its central rotation.
    //           This is right in the middle of the low and high angles. For example, consider a western swing door. After
    //           walking through it will swing in both directions but at the end it stays right in the middle.

    // http://javadoc.jmonkeyengine.org/com/jme3/bullet/joints/HingeJoint.html
    //
    // _softness - the factor at which the velocity error correction starts operating, i.e. a softness of 0.9 means that
    //             the vel. corr starts at 90% of the limit range.
    // _biasFactor - the magnitude of the position correction. It tells you how strictly the position error (drift) is
    //             corrected.
    // _relaxationFactor - the rate at which velocity errors are corrected. This can be seen as the strength of the
    //             limits. A low value will make the the limits more spongy.


    float _softness { 0.9f };
    float _biasFactor { 0.3f };
    float _relaxationFactor { 1.0f };
};

#endif // hifi_ObjectConstraintHinge_h
