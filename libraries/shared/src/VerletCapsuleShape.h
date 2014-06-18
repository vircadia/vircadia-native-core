//
//  VerletCapsuleShape.h
//  libraries/shared/src
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


// The VerletCapsuleShape is similar to a regular CapsuleShape, except it keeps pointers 
// to its endpoints which are owned by some other data structure.  This allows it to 
// participate in a verlet integration engine.
//
// Although the true_halfHeight of the VerletCapsuleShape is considered a constant 
class VerletCapsuleShape : public CapsuleShape {
public:
    VerletCapsuleShape(glm::vec3* startPoint, glm::vec3* endPoint);
    VerletCapsuleShape(float radius, glm::vec3* startPoint, glm::vec3* endPoint);

    // virtual overrides from Shape
    const glm::quat& getRotation() const;
    void setRotation(const glm::quat& rotation);
    void setTranslation(const glm::vec3& position);
    const glm::vec3& getTranslation() const;

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
    glm::vec3* _startPoint;
    glm::vec3* _endPoint;
};

#endif // hifi_VerletCapsuleShape_h
