//
//  Shape.h
//  libraries/shared/src
//
//  Created by Andrew Meadows on 2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Shape_h
#define hifi_Shape_h

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


class Shape {
public:
    enum Type{
        UNKNOWN_SHAPE = 0,
        SPHERE_SHAPE,
        CAPSULE_SHAPE,
        PLANE_SHAPE,
        BOX_SHAPE,
        LIST_SHAPE
    };

    Shape() : _type(UNKNOWN_SHAPE), _boundingRadius(0.f), _position(0.f), _rotation() { }
    virtual ~Shape() {}

    int getType() const { return _type; }
    float getBoundingRadius() const { return _boundingRadius; }
    const glm::vec3& getPosition() const { return _position; }
    const glm::quat& getRotation() const { return _rotation; }

    virtual void setPosition(const glm::vec3& position) { _position = position; }
    virtual void setRotation(const glm::quat& rotation) { _rotation = rotation; }

    virtual bool findRayIntersection(const glm::vec3& rayStart, const glm::vec3& rayDirection, float& distance) const = 0;

protected:
    // these ctors are protected (used by derived classes only)
    Shape(Type type) : _type(type), _boundingRadius(0.f), _position(0.f), _rotation() {}

    Shape(Type type, const glm::vec3& position) 
        : _type(type), _boundingRadius(0.f), _position(position), _rotation() {}

    Shape(Type type, const glm::vec3& position, const glm::quat& rotation) 
        : _type(type), _boundingRadius(0.f), _position(position), _rotation(rotation) {}

    void setBoundingRadius(float radius) { _boundingRadius = radius; }

    int _type;
    float _boundingRadius;
    glm::vec3 _position;
    glm::quat _rotation;
};

#endif // hifi_Shape_h
