//
//  AngularConstraint.h
//  interface/src/renderer
//
//  Created by Andrew Meadows on 2014.05.30
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AngularConstraint_h
#define hifi_AngularConstraint_h

#include <glm/glm.hpp>

class AngularConstraint {
public:
    /// \param minAngles minumum euler angles for the constraint
    /// \param maxAngles minumum euler angles for the constraint
    /// \return pointer to new AngularConstraint of the right type or NULL if none could be made
    static AngularConstraint* newAngularConstraint(const glm::vec3& minAngles, const glm::vec3& maxAngles);

    AngularConstraint() {}
    virtual ~AngularConstraint() {}
    virtual bool clamp(glm::quat& rotation) const = 0;
    virtual bool softClamp(glm::quat& targetRotation, const glm::quat& oldRotation, float mixFraction);
protected:
};

class HingeConstraint : public AngularConstraint {
public:
    HingeConstraint(const glm::vec3& forwardAxis, const glm::vec3& rotationAxis, float minAngle, float maxAngle);
    virtual bool clamp(glm::quat& rotation) const;
    virtual bool softClamp(glm::quat& targetRotation, const glm::quat& oldRotation, float mixFraction);
protected:
    glm::vec3 _forwardAxis;
    glm::vec3 _rotationAxis;
    float _minAngle;
    float _maxAngle;
};

class ConeRollerConstraint : public AngularConstraint {
public:
    ConeRollerConstraint(float coneAngle, const glm::vec3& coneAxis, float minRoll, float maxRoll);
    virtual bool clamp(glm::quat& rotation) const;
private:
    float _coneAngle;
    glm::vec3 _coneAxis;
    float _minRoll;
    float _maxRoll;
};

#endif // hifi_AngularConstraint_h
