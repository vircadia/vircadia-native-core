//
//  CapsuleShape.h
//  libraries/shared/src
//
//  Created by Andrew Meadows on 02/20/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CapsuleShape_h
#define hifi_CapsuleShape_h

#include "Shape.h"

// default axis of CapsuleShape is Y-axis

class CapsuleShape : public Shape {
public:
    CapsuleShape();
    CapsuleShape(float radius, float halfHeight);
    CapsuleShape(float radius, float halfHeight, const glm::vec3& position, const glm::quat& rotation);
    CapsuleShape(float radius, const glm::vec3& startPoint, const glm::vec3& endPoint);

    float getRadius() const { return _radius; }
    float getHalfHeight() const { return _halfHeight; }

    /// \param[out] startPoint is the center of start cap
    void getStartPoint(glm::vec3& startPoint) const;

    /// \param[out] endPoint is the center of the end cap
    void getEndPoint(glm::vec3& endPoint) const;

    void computeNormalizedAxis(glm::vec3& axis) const;

    void setRadius(float radius);
    void setHalfHeight(float height);
    void setRadiusAndHalfHeight(float radius, float height);
    void setEndPoints(const glm::vec3& startPoint, const glm::vec3& endPoint);

    bool findRayIntersection(const glm::vec3& rayStart, const glm::vec3& rayDirection, float& distance) const;

protected:
    void updateBoundingRadius() { _boundingRadius = _radius + _halfHeight; }

    float _radius;
    float _halfHeight;
};

#endif // hifi_CapsuleShape_h
