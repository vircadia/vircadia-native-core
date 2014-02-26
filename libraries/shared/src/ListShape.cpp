//
//  ListShape.cpp
//
//  ListShape: A collection of shapes, each with a local transform.
//
//  Created by Andrew Meadows on 2014.02.20
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include "ListShape.h"

// ListShapeEntry

void ListShapeEntry::updateTransform(const glm::vec3& rootPosition, const glm::quat& rootRotation) {
    _shape->setPosition(rootPosition + rootRotation * _localPosition);
    _shape->setRotation(_localRotation * rootRotation);
}

// ListShape

ListShape::~ListShape() {
    clear();
}

void ListShape::setPosition(const glm::vec3& position) {
    _subShapeTransformsAreDirty = true;
    Shape::setPosition(position);
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
            _subShapeEntries[i].updateTransform(_position, _rotation);
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
    
