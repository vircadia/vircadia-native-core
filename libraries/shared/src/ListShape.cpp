//
//  ListShape.cpp
//  libraries/shared/src
//
//  Created by Andrew Meadows on 02/20/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ListShape.h"

// ListShapeEntry

void ListShapeEntry::updateTransform(const glm::vec3& rootPosition, const glm::quat& rootRotation) {
    _shape->setTranslation(rootPosition + rootRotation * _localPosition);
    _shape->setRotation(_localRotation * rootRotation);
}

// ListShape

ListShape::~ListShape() {
    clear();
}

void ListShape::setTranslation(const glm::vec3& position) {
    _subShapeTransformsAreDirty = true;
    Shape::setTranslation(position);
}

void ListShape::setRotation(const glm::quat& rotation) {
    _subShapeTransformsAreDirty = true;
    Shape::setRotation(rotation);
}

const Shape* ListShape::getSubShape(int index) const {
    if (index < 0 || index > _subShapeEntries.size()) {
        return NULL;
    }
    return _subShapeEntries[index]._shape;
}

void ListShape::updateSubTransforms() {
    if (_subShapeTransformsAreDirty) {
        for (int i = 0; i < _subShapeEntries.size(); ++i) {
            _subShapeEntries[i].updateTransform(_translation, _rotation);
        }
        _subShapeTransformsAreDirty = false;
    }
}

void ListShape::addShape(Shape* shape, const glm::vec3& localPosition, const glm::quat& localRotation) {
    if (shape) {
        ListShapeEntry entry;
        entry._shape = shape;
        entry._localPosition = localPosition;
        entry._localRotation = localRotation;
        _subShapeEntries.push_back(entry);
    }
}

void ListShape::setShapes(QVector<ListShapeEntry>& shapes) {
    clear();
    _subShapeEntries.swap(shapes);
    // TODO: audit our new list of entries and delete any that have null pointers
    computeBoundingRadius();
}

void ListShape::clear() {
    // the ListShape owns its subShapes, so they need to be deleted
    for (int i = 0; i < _subShapeEntries.size(); ++i) {
        delete _subShapeEntries[i]._shape;
    }
    _subShapeEntries.clear();
    setBoundingRadius(0.f);
}

void ListShape::computeBoundingRadius() {
    float maxRadius = 0.f;
    for (int i = 0; i < _subShapeEntries.size(); ++i) {
        ListShapeEntry& entry = _subShapeEntries[i];
        float radius = glm::length(entry._localPosition) + entry._shape->getBoundingRadius();
        if (radius > maxRadius) {
            maxRadius = radius;
        }
    }
    setBoundingRadius(maxRadius);
}
    
