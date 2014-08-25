//
//  AACubeShape.h
//  libraries/shared/src
//
//  Created by Andrew Meadows on 2014.08.22
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AACubeShape_h
#define hifi_AACubeShape_h

#include "Shape.h"

class AACubeShape : public Shape {
public:
    AACubeShape() : Shape(AACUBE_SHAPE), _scale(1.0f) {}
    AACubeShape(float scale) : Shape(AACUBE_SHAPE), _scale(scale) { }
    AACubeShape(float scale, const glm::vec3& position) : Shape(AACUBE_SHAPE, position), _scale(scale) { }

    virtual ~AACubeShape() {}

    float getScale() const { return _scale; }
    void setScale(float scale) { _scale = scale; }

    bool findRayIntersection(const glm::vec3& rayStart, const glm::vec3& rayDirection, float& distance) const;

    float getVolume() const { return _scale * _scale * _scale; }

protected:
    float _scale;
};

#endif // hifi_AACubeShape_h
