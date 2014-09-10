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

#include "SharedUtil.h"

// default axis of CapsuleShape is Y-axis
const glm::vec3 DEFAULT_CAPSULE_AXIS(0.0f, 1.0f, 0.0f);


class CapsuleShape : public Shape {
public:
    CapsuleShape();
    CapsuleShape(float radius, float halfHeight);
    CapsuleShape(float radius, float halfHeight, const glm::vec3& position, const glm::quat& rotation);
    CapsuleShape(float radius, const glm::vec3& startPoint, const glm::vec3& endPoint);

    virtual ~CapsuleShape() {}

    float getRadius() const { return _radius; }
    virtual float getHalfHeight() const { return _halfHeight; }

    /// \param[out] startPoint is the center of start cap
    virtual void getStartPoint(glm::vec3& startPoint) const;

    /// \param[out] endPoint is the center of the end cap
    virtual void getEndPoint(glm::vec3& endPoint) const;

    virtual void computeNormalizedAxis(glm::vec3& axis) const;

    void setRadius(float radius);
    virtual void setHalfHeight(float height);
    virtual void setRadiusAndHalfHeight(float radius, float height);

    /// Sets the endpoints and updates center, rotation, and halfHeight to agree.
    virtual void setEndPoints(const glm::vec3& startPoint, const glm::vec3& endPoint);

    bool findRayIntersection(RayIntersectionInfo& intersection) const;

    virtual float getVolume() const { return (PI * _radius * _radius) * (1.3333333333f * _radius + getHalfHeight()); }

protected:
    virtual void updateBoundingRadius() { _boundingRadius = _radius + getHalfHeight(); }
    static glm::quat computeNewRotation(const glm::vec3& newAxis);

    float _radius;
    float _halfHeight;
};

#endif // hifi_CapsuleShape_h
