//
//  ObjectConstraintSlider.h
//  libraries/physics/src
//
//  Created by Seth Alves 2017-4-23
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ObjectConstraintSlider_h
#define hifi_ObjectConstraintSlider_h

#include "ObjectConstraint.h"

// http://bulletphysics.org/Bullet/BulletFull/classbtSliderConstraint.html

class ObjectConstraintSlider : public ObjectConstraint {
public:
    ObjectConstraintSlider(const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~ObjectConstraintSlider();

    virtual void prepareForPhysicsSimulation() override;

    virtual bool updateArguments(QVariantMap arguments) override;
    virtual QVariantMap getArguments() override;

    virtual QByteArray serialize() const override;
    virtual void deserialize(QByteArray serializedArguments) override;

    virtual QList<btRigidBody*> getRigidBodies() override;
    virtual btTypedConstraint* getConstraint() override;

protected:
    static const uint16_t constraintVersion;

    void updateSlider();

    glm::vec3 _pointInA;
    glm::vec3 _axisInA;

    glm::vec3 _pointInB;
    glm::vec3 _axisInB;

    float _linearLow { std::numeric_limits<float>::max() };
    float _linearHigh { std::numeric_limits<float>::min() };

    float _angularLow { -TWO_PI };
    float _angularHigh { TWO_PI };
};

#endif // hifi_ObjectConstraintSlider_h
