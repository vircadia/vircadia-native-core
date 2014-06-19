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

class PhysicsEntity;

class Shape {
public:
    enum Type{
        UNKNOWN_SHAPE = 0,
        SPHERE_SHAPE,
        CAPSULE_SHAPE,
        PLANE_SHAPE,
        LIST_SHAPE
    };

    Shape() : _type(UNKNOWN_SHAPE), _owningEntity(NULL), _boundingRadius(0.f), _translation(0.f), _rotation() { }
    virtual ~Shape() {}

    int getType() const { return _type; }

    void setEntity(PhysicsEntity* entity) { _owningEntity = entity; }
    PhysicsEntity* getEntity() const { return _owningEntity; }

    float getBoundingRadius() const { return _boundingRadius; }

    virtual const glm::quat& getRotation() const { return _rotation; }
    virtual void setRotation(const glm::quat& rotation) { _rotation = rotation; }

    virtual void setTranslation(const glm::vec3& translation) { _translation = translation; }
    virtual const glm::vec3& getTranslation() const { return _translation; }

    virtual bool findRayIntersection(const glm::vec3& rayStart, const glm::vec3& rayDirection, float& distance) const = 0;

protected:
    // these ctors are protected (used by derived classes only)
    Shape(Type type) : _type(type), _owningEntity(NULL), _boundingRadius(0.f), _translation(0.f), _rotation() {}

    Shape(Type type, const glm::vec3& position) 
        : _type(type), _owningEntity(NULL), _boundingRadius(0.f), _translation(position), _rotation() {}

    Shape(Type type, const glm::vec3& position, const glm::quat& rotation) 
        : _type(type), _owningEntity(NULL), _boundingRadius(0.f), _translation(position), _rotation(rotation) {}

    void setBoundingRadius(float radius) { _boundingRadius = radius; }

    int _type;
    PhysicsEntity* _owningEntity;
    float _boundingRadius;
    glm::vec3 _translation;
    glm::quat _rotation;
};

#endif // hifi_Shape_h
