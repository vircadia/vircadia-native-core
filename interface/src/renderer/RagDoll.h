//
//  RagDoll.h
//  interface/src/avatar
//
//  Created by Andrew Meadows 2014.05.30
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RagDoll_h
#define hifi_RagDoll_h

#include "renderer/Model.h"

class Constraint {
public:
    Constraint() {}
    virtual ~Constraint() {}

    /// Enforce contraint by moving relevant points.
    /// \return max distance of point movement
    virtual float enforce() = 0;
};

class FixedConstraint : public Constraint {
public:
    FixedConstraint();
    float enforce();
    void setPoint(glm::vec3* point);
    void setAnchor(const glm::vec3& anchor);
private:
    glm::vec3* _point;
    glm::vec3 _anchor;
};

class DistanceConstraint : public Constraint {
public:
    DistanceConstraint(glm::vec3* pointA, glm::vec3* pointB);
    DistanceConstraint(const DistanceConstraint& other);
    float enforce();
    void setDistance(float distance);
private:
    float _distance;
    glm::vec3* _points[2];
};

class RagDoll {
public:

    RagDoll();
    virtual ~RagDoll();
    
    /// Create points and constraints based on topology of collection of joints
    /// \param joints list of connected joint states
    void init(const QVector<JointState>& states);

    /// Delete all data.
    void clear();

    /// \param states list of joint states
    /// \param fraction range from 0.0 (no movement) to 1.0 (use joint locations)
    /// \return max distance of point movement
    float slaveToSkeleton(const QVector<JointState>& states, float fraction);

    /// Enforce contraints.
    /// \return max distance of point movement
    float enforceConstraints();

    const QVector<glm::vec3>& getPoints() const { return _points; }
    
private:
    QVector<Constraint*> _constraints;
    QVector<glm::vec3> _points;
};

#endif // hifi_RagDoll_h
