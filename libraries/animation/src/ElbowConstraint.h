//
//  ElbowConstraint.h
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ElbowConstraint_h
#define hifi_ElbowConstraint_h

#include "RotationConstraint.h"

class ElbowConstraint : public RotationConstraint {
public:
    ElbowConstraint();
    virtual void setReferenceRotation(const glm::quat& rotation) override { _referenceRotation = rotation; }
    void setHingeAxis(const glm::vec3& axis);
    void setAngleLimits(float minAngle, float maxAngle);
    virtual bool apply(glm::quat& rotation) const override;
protected:
    glm::quat _referenceRotation;
    glm::vec3 _axis;
    glm::vec3 _perpAxis;
    float _minAngle;
    float _maxAngle;
};

#endif // hifi_ElbowConstraint_h
