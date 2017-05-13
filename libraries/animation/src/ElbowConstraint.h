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
    void setHingeAxis(const glm::vec3& axis);
    void setAngleLimits(float minAngle, float maxAngle);
    virtual bool apply(glm::quat& rotation) const override;
    virtual glm::quat computeCenterRotation() const override;

    glm::vec3 getHingeAxis() const { return _axis; }
    float getMinAngle() const { return _minAngle; }
    float getMaxAngle() const { return _maxAngle; }

protected:
    glm::vec3 _axis;
    glm::vec3 _perpAxis;
    float _minAngle;
    float _maxAngle;
};

#endif // hifi_ElbowConstraint_h
