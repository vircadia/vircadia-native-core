//
//  PhysicsEntity.cpp
//  libraries/shared/src
//
//  Created by Andrew Meadows 2014.06.11
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PhysicsEntity.h"
#include "Shape.h"
#include "ShapeCollider.h"

PhysicsEntity::PhysicsEntity() : 
    _translation(0.0f), 
    _rotation(), 
    _boundingRadius(0.0f), 
    _shapesAreDirty(true),
    _enableShapes(false),
    _simulation(NULL) {
}

PhysicsEntity::~PhysicsEntity() {
    // entity should be removed from the simulation before it is deleted
    assert(_simulation == NULL);
}

void PhysicsEntity::setTranslation(const glm::vec3& translation) {
    if (_translation != translation) {
        _shapesAreDirty = !_shapes.isEmpty();
        _translation = translation;
    }
}

void PhysicsEntity::setRotation(const glm::quat& rotation) {
    if (_rotation != rotation) {
        _shapesAreDirty = !_shapes.isEmpty();
        _rotation = rotation;
    }
}   

void PhysicsEntity::setShapeBackPointers() {
    for (int i = 0; i < _shapes.size(); i++) {
        Shape* shape = _shapes[i];
        if (shape) {
            shape->setEntity(this);
        }
    }
}

void PhysicsEntity::setEnableShapes(bool enable) {
    if (enable != _enableShapes) {
        clearShapes();
        _enableShapes = enable;
        if (_enableShapes) {
            buildShapes();
        }
    }
}   

void PhysicsEntity::clearShapes() {
    for (int i = 0; i < _shapes.size(); ++i) {
        delete _shapes[i];
    }
    _shapes.clear();
}

bool PhysicsEntity::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const {
    /* TODO: Andrew to make this work
    int numShapes = _shapes.size();
    float minDistance = FLT_MAX;
    for (int j = 0; j < numShapes; ++j) {
        const Shape* shape = _shapes[j];
        float thisDistance = FLT_MAX;
        if (shape && ShapeCollider::findRayIntersection(ourShape, origin, direction, thisDistance)) {
            if (thisDistance < minDistance) {
                minDistance = thisDistance;
            }
        }
    }
    if (minDistance < FLT_MAX) {
        distance = minDistance;
        return true;
    }
    */
    return false;
}

bool PhysicsEntity::findCollisions(const QVector<const Shape*> shapes, CollisionList& collisions) {
    bool collided = false;
    int numTheirShapes = shapes.size();
    for (int i = 0; i < numTheirShapes; ++i) {
        const Shape* theirShape = shapes[i];
        if (!theirShape) {
            continue;
        }
        int numOurShapes = _shapes.size();
        for (int j = 0; j < numOurShapes; ++j) {
            const Shape* ourShape = _shapes.at(j);
            if (ourShape && ShapeCollider::collideShapes(theirShape, ourShape, collisions)) {
                collided = true;
            }
        }
    }
    return collided;
}

bool PhysicsEntity::findSphereCollisions(const glm::vec3& sphereCenter, float sphereRadius,
    CollisionList& collisions, int skipIndex) {
    bool collided = false;
    // TODO: Andrew to implement this or make it unecessary
    /*
    SphereShape sphere(sphereRadius, sphereCenter);
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    for (int i = 0; i < _shapes.size(); i++) {
        Shape* shape = _shapes[i];
        if (!shape) {
            continue;
        }
        const FBXJoint& joint = geometry.joints[i];
        if (joint.parentIndex != -1) {
            if (skipIndex != -1) {
                int ancestorIndex = joint.parentIndex;
                do {
                    if (ancestorIndex == skipIndex) {
                        goto outerContinue;
                    }
                    ancestorIndex = geometry.joints[ancestorIndex].parentIndex;
                    
                } while (ancestorIndex != -1);
            }
        }
        if (ShapeCollider::collideShapes(&sphere, shape, collisions)) {
            CollisionInfo* collision = collisions.getLastCollision();
            collision->_data = (void*)(this);
            collision->_intData = i;
            collided = true;
        }
        outerContinue: ;
    }
    */
    return collided;
}

bool PhysicsEntity::findPlaneCollisions(const glm::vec4& plane, CollisionList& collisions) {
    bool collided = false;
    PlaneShape planeShape(plane);
    for (int i = 0; i < _shapes.size(); i++) {
        if (_shapes.at(i) && ShapeCollider::collideShapes(&planeShape, _shapes.at(i), collisions)) {
            CollisionInfo* collision = collisions.getLastCollision();
            collision->_data = (void*)(this);
            collision->_intData = i;
            collided = true;
        }
    }
    return collided;
}
