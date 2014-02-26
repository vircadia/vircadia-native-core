//
//  Shape.h
//
//  Created by Andrew Meadows on 2014.02.20
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__Shape__
#define __hifi__Shape__

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


class Shape {
public:
    enum Type{
        UNKNOWN_SHAPE = 0,
        SPHERE_SHAPE,
        CAPSULE_SHAPE,
        BOX_SHAPE,
        LIST_SHAPE
    };

    Shape() : _type(UNKNOWN_SHAPE), _boundingRadius(0.f), _position(0.f), _rotation() { }

    int getType() const { return _type; }
    float getBoundingRadius() const { return _boundingRadius; }
    const glm::vec3& getPosition() const { return _position; }
    const glm::quat& getRotation() const { return _rotation; }

    virtual void setPosition(const glm::vec3& position) { _position = position; }
    virtual void setRotation(const glm::quat& rotation) { _rotation = rotation; }

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

#endif /* defined(__hifi__Shape__) */
