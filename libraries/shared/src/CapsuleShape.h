//
//  CapsuleShape.h
//  libraries/shared/src
//
//  Created by Andrew Meadows on 2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __hifi__CapsuleShape__
#define __hifi__CapsuleShape__

#include "Shape.h"

// adebug bookmark TODO: convert to new world-frame approach
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

protected:
    void updateBoundingRadius() { _boundingRadius = _radius + _halfHeight; }

    float _radius;
    float _halfHeight;
};

#endif /* defined(__hifi__CapsuleShape__) */
