//
//  CapsuleShape.h
//  hifi
//
//  Created by Andrew Meadows on 2014.02.20
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__CapsuleShape__
#define __hifi__CapsuleShape__

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

    void setRadius(float radius);
    void setHalfHeight(float height);
    void setRadiusAndHalfHeight(float radius, float height);

protected:
    void updateBoundingRadius() { _boundingRadius = _radius + _halfHeight; }

    float _radius;
    float _halfHeight;
};

#endif /* defined(__hifi__CapsuleShape__) */
