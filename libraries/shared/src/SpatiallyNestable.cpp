//
//  SpatiallyNestable.cpp
//  libraries/shared/src/
//
//  Created by Seth Alves on 2015-10-18
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SpatiallyNestable.h"

SpatiallyNestable::SpatiallyNestable() :
    _transform() {
}


const glm::vec3& SpatiallyNestable::getPosition() const {
    return _transform.getTranslation();
}

void SpatiallyNestable::setPosition(const glm::vec3& position) {
    _transform.setTranslation(position);
}

const glm::quat& SpatiallyNestable::getOrientation() const {
    return _transform.getRotation();
}

void SpatiallyNestable::setOrientation(const glm::quat& orientation) {
    _transform.setRotation(orientation);
}

const Transform& SpatiallyNestable::getTransform() const {
    return _transform;
}
