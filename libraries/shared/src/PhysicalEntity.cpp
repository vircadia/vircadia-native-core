//
//  PhysicalEntity.cpp
//  libraries/shared/src
//
//  Created by Andrew Meadows 2014.06.11
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PhysicalEntity.h"
#include "Shape.h"

PhysicalEntity::PhysicalEntity(EntityType type) : 
    _entityType(type), 
    _translation(0.0f), 
    _rotation(), 
    _boundingRadius(0.0f), 
    _shapesAreDirty(true),
    _enableShapes(false) {
}

void PhysicalEntity::setTranslation(const glm::vec3& translation) {
    if (_translation != translation) {
        _shapesAreDirty = !_shapes.isEmpty();
        _translation = translation;
    }
}

void PhysicalEntity::setRotation(const glm::quat& rotation) {
    if (_rotation != rotation) {
        _shapesAreDirty = !_shapes.isEmpty();
        _rotation = rotation;
    }
}   

void PhysicalEntity::setShapeBackPointers() {
    for (int i = 0; i < _shapes.size(); i++) {
        Shape* shape = _shapes[i];
        if (shape) {
            shape->setEntity(this);
        }
    }
}

void PhysicalEntity::setEnableShapes(bool enable) {
    if (enable != _enableShapes) {
        clearShapes();
        _enableShapes = enable;
        if (_enableShapes) {
            buildShapes();
        }
    }
}   

void PhysicalEntity::clearShapes() {
    for (int i = 0; i < _shapes.size(); ++i) {
        delete _shapes[i];
    }
    _shapes.clear();
}


