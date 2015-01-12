//
//  VerletCapsuleShape.h
//  libraries/physics/src
//
//  Created by Andrew Meadows on 2014.06.16
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_VerletCapsuleShape_h
#define hifi_VerletCapsuleShape_h

#include "CapsuleShape.h"


// The VerletCapsuleShape is similar to a regular CapsuleShape, except it keeps a pointer
// to its endpoints which are owned by some other data structure (a verlet simulation system).  
// This makes it easier for the points to be moved around by constraints in the system
// as well as collisions with the shape, however it has some drawbacks:
//
// (1) The Shape::_translation and ::_rotation data members are not used (wasted)
//
// (2) A VerletShape doesn't own the points that it uses, so you must be careful not to
//     leave dangling pointers around.
//
// (3) Some const methods of VerletCapsuleShape are much more expensive than you might think.
//     For example getHalfHeight() and setHalfHeight() methods must do extra computation.  In
//     particular setRotation() is significantly more expensive than for the CapsuleShape.
//     Not too expensive to use when setting up shapes, but you woudln't want to use it deep
//     down in a hot simulation loop, such as when processing collision results.  Best to
//     just let the verlet simulation do its thing and not try to constantly force a rotation.

class VerletPoint;

class VerletCapsuleShape : public CapsuleShape {
public:
    VerletCapsuleShape(VerletPoint* startPoint, VerletPoint* endPoint);
    VerletCapsuleShape(float radius, VerletPoint* startPoint, VerletPoint* endPoint);

    // virtual overrides from Shape
    const glm::quat& getRotation() const;
    void setRotation(const glm::quat& rotation);
    void setTranslation(const glm::vec3& position);
    const glm::vec3& getTranslation() const;
    float computeEffectiveMass(const glm::vec3& penetration, const glm::vec3& contactPoint);
    void accumulateDelta(float relativeMassFactor, const glm::vec3& penetration);
    void applyAccumulatedDelta();
    virtual void getVerletPoints(QVector<VerletPoint*>& points);

    //float getRadius() const { return _radius; }
    virtual float getHalfHeight() const;

    /// \param[out] startPoint is the center of start cap
    void getStartPoint(glm::vec3& startPoint) const;

    /// \param[out] endPoint is the center of the end cap
    void getEndPoint(glm::vec3& endPoint) const;

    /// \param[out] axis is a normalized vector that points from start to end
    void computeNormalizedAxis(glm::vec3& axis) const;

    //void setRadius(float radius);
    void setHalfHeight(float halfHeight);
    void setRadiusAndHalfHeight(float radius, float halfHeight);
    void setEndPoints(const glm::vec3& startPoint, const glm::vec3& endPoint);

    //void assignEndPoints(glm::vec3* startPoint, glm::vec3* endPoint);

protected:
    // NOTE: VerletCapsuleShape does NOT own the data in its points.
    VerletPoint* _startPoint;
    VerletPoint* _endPoint;

    // The LagrangeCoef's are numerical weights for distributing collision movement
    // between the relevant VerletPoints associated with this shape.  They are functions
    // of the movement parameters and are computed (and cached) in computeEffectiveMass() 
    // and then used in the subsequent accumulateDelta().
    float _startLagrangeCoef;
    float _endLagrangeCoef;
};

#endif // hifi_VerletCapsuleShape_h
