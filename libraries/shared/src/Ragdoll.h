//
//  Ragdoll.h
//  libraries/shared/src
//
//  Created by Andrew Meadows 2014.05.30
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Ragdoll_h
#define hifi_Ragdoll_h

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "VerletPoint.h"

#include <QVector>

//#include "PhysicsSimulation.h"

class DistanceConstraint;
class FixedConstraint;
class PhysicsSimulation;

// TODO: don't derive SkeletonModel from Ragdoll so we can clean up the Ragdoll API
// (==> won't need to worry about namespace conflicts between Entity and Ragdoll).

class Ragdoll {
public:

    Ragdoll();
    virtual ~Ragdoll();

    virtual void stepForward(float deltaTime);

    /// \return max distance of point movement
    float enforceConstraints();

    // both const and non-const getPoints()
    const QVector<VerletPoint>& getPoints() const { return _points; }
    QVector<VerletPoint>& getPoints() { return _points; }

    void initTransform();

    /// set the translation and rotation of the Ragdoll and adjust all VerletPoints.
    void setTransform(const glm::vec3& translation, const glm::quat& rotation);

    const glm::vec3& getTranslationInSimulationFrame() const { return _translationInSimulationFrame; }

    void setMassScale(float scale);
    float getMassScale() const { return _massScale; }

    void clearConstraintsAndPoints();
    virtual void initPoints() = 0;
    virtual void buildConstraints() = 0;

protected:
    float _massScale;
    glm::vec3 _translation;  // world-frame
    glm::quat _rotation; // world-frame
    glm::vec3 _translationInSimulationFrame;
    glm::quat _rotationInSimulationFrame;

    QVector<VerletPoint> _points;
    QVector<DistanceConstraint*> _boneConstraints;
    QVector<FixedConstraint*> _fixedConstraints;
private:
    void updateSimulationTransforms(const glm::vec3& translation, const glm::quat& rotation);

    friend class PhysicsSimulation;
    PhysicsSimulation* _simulation;
};

#endif // hifi_Ragdoll_h
