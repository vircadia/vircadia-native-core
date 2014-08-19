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

#include "PhysicsSimulation.h"
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
    if (_simulation) {
        _simulation->removeEntity(this);
        _simulation = NULL;
    }
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
    if (_simulation) {
        _simulation->removeShapes(this);
    }
    for (int i = 0; i < _shapes.size(); ++i) {
        delete _shapes[i];
    }
    _shapes.clear();
}

bool PhysicsEntity::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const {
    int numShapes = _shapes.size();
    float minDistance = FLT_MAX;
    for (int j = 0; j < numShapes; ++j) {
        const Shape* shape = _shapes[j];
        float thisDistance = FLT_MAX;
        if (shape && shape->findRayIntersection(origin, direction, thisDistance)) {
            if (thisDistance < minDistance) {
                minDistance = thisDistance;
            }
        }
    }
    if (minDistance < FLT_MAX) {
        distance = minDistance;
        return true;
    }
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

bool PhysicsEntity::findSphereCollisions(const glm::vec3& sphereCenter, float sphereRadius, CollisionList& collisions) {
    bool collided = false;
    SphereShape sphere(sphereRadius, sphereCenter);
    for (int i = 0; i < _shapes.size(); i++) {
        Shape* shape = _shapes[i];
        if (!shape) {
            continue;
        }
        if (ShapeCollider::collideShapes(&sphere, shape, collisions)) {
            CollisionInfo* collision = collisions.getLastCollision();
            collision->_data = (void*)(this);
            collision->_intData = i;
            collided = true;
        }
    }
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

// -----------------------------------------------------------
// TODO: enforce this maximum when shapes are actually built.  The gotcha here is 
// that the Model class (derived from PhysicsEntity) expects numShapes == numJoints, 
// so we have to modify that code to be safe.
const int MAX_SHAPES_PER_ENTITY = 256;

// the first 256 prime numbers
const int primes[256] = {
      2,    3,    5,    7,   11,   13,   17,   19,   23,   29,
     31,   37,   41,   43,   47,   53,   59,   61,   67,   71,
     73,   79,   83,   89,   97,  101,  103,  107,  109,  113,
    127,  131,  137,  139,  149,  151,  157,  163,  167,  173,
    179,  181,  191,  193,  197,  199,  211,  223,  227,  229,
    233,  239,  241,  251,  257,  263,  269,  271,  277,  281,
    283,  293,  307,  311,  313,  317,  331,  337,  347,  349,
    353,  359,  367,  373,  379,  383,  389,  397,  401,  409,
    419,  421,  431,  433,  439,  443,  449,  457,  461,  463,
    467,  479,  487,  491,  499,  503,  509,  521,  523,  541,
    547,  557,  563,  569,  571,  577,  587,  593,  599,  601,
    607,  613,  617,  619,  631,  641,  643,  647,  653,  659,
    661,  673,  677,  683,  691,  701,  709,  719,  727,  733,
    739,  743,  751,  757,  761,  769,  773,  787,  797,  809,
    811,  821,  823,  827,  829,  839,  853,  857,  859,  863,
    877,  881,  883,  887,  907,  911,  919,  929,  937,  941,
    947,  953,  967,  971,  977,  983,  991,  997, 1009, 1013,
   1019, 1021, 1031, 1033, 1039, 1049, 1051, 1061, 1063, 1069,
   1087, 1091, 1093, 1097, 1103, 1109, 1117, 1123, 1129, 1151,
   1153, 1163, 1171, 1181, 1187, 1193, 1201, 1213, 1217, 1223,
   1229, 1231, 1237, 1249, 1259, 1277, 1279, 1283, 1289, 1291,
   1297, 1301, 1303, 1307, 1319, 1321, 1327, 1361, 1367, 1373,
   1381, 1399, 1409, 1423, 1427, 1429, 1433, 1439, 1447, 1451,
   1453, 1459, 1471, 1481, 1483, 1487, 1489, 1493, 1499, 1511,
   1523, 1531, 1543, 1549, 1553, 1559, 1567, 1571, 1579, 1583,
   1597, 1601, 1607, 1609, 1613, 1619 };

void PhysicsEntity::disableCollisions(int shapeIndexA, int shapeIndexB) {
    if (shapeIndexA < MAX_SHAPES_PER_ENTITY && shapeIndexB < MAX_SHAPES_PER_ENTITY) {
        _disabledCollisions.insert(primes[shapeIndexA] * primes[shapeIndexB]);
    }
}

bool PhysicsEntity::collisionsAreEnabled(int shapeIndexA, int shapeIndexB) const {
    if (shapeIndexA < MAX_SHAPES_PER_ENTITY && shapeIndexB < MAX_SHAPES_PER_ENTITY) {
        return !_disabledCollisions.contains(primes[shapeIndexA] * primes[shapeIndexB]);
    }
    return false;
}

void PhysicsEntity::disableCurrentSelfCollisions() {
    CollisionList collisions(10);
    int numShapes = _shapes.size();
    for (int i = 0; i < numShapes; ++i) {
        const Shape* shape = _shapes.at(i);
        if (!shape) {
            continue;
        }
        for (int j = i+1; j < numShapes; ++j) {
            if (!collisionsAreEnabled(i, j)) {
                continue;
            }
            const Shape* otherShape = _shapes.at(j);
            if (otherShape && ShapeCollider::collideShapes(shape, otherShape, collisions)) {
                disableCollisions(i, j);
                collisions.clear();
            }
        }
    }
}
