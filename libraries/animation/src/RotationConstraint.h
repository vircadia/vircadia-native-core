//
//  RotationConstraint.h
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RotationConstraint_h
#define hifi_RotationConstraint_h

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class RotationConstraint {
public:
    RotationConstraint() : _referenceRotation() {}
    virtual ~RotationConstraint() {}

    /// \param referenceRotation the rotation from which rotation changes are measured.
    virtual void setReferenceRotation(const glm::quat& rotation) { _referenceRotation = rotation; }

    /// \return the rotation from which rotation changes are measured.
    const glm::quat& getReferenceRotation() const { return _referenceRotation; }

    /// \param rotation rotation to clamp
    /// \return true if rotation is clamped
    virtual bool apply(glm::quat& rotation) const = 0;

    /// \return true if this constraint is part of lower spine
    virtual bool isLowerSpine() const { return false; }

    /// \param rotation rotation to allow
    /// \brief clear previous adjustment and adjust constraint limits to allow rotation
    virtual void dynamicallyAdjustLimits(const glm::quat& rotation) {}

    /// \brief reset any remembered joint limit history
    virtual void clearHistory() {};

protected:
    glm::quat _referenceRotation = glm::quat();
};

#endif // hifi_RotationConstraint_h
