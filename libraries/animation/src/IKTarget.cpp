//
//  IKTarget.cpp
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "IKTarget.h"

void IKTarget::setPose(const glm::quat& rotation, const glm::vec3& translation) {
    _pose.rot() = rotation;
    _pose.trans() = translation;
}

void IKTarget::setType(int type) {
    switch (type) {
        case (int)Type::RotationAndPosition:
            _type = Type::RotationAndPosition;
            break;
        case (int)Type::RotationOnly:
            _type = Type::RotationOnly;
            break;
        case (int)Type::HmdHead:
            _type = Type::HmdHead;
            break;
        case (int)Type::HipsRelativeRotationAndPosition:
            _type = Type::HipsRelativeRotationAndPosition;
            break;
        default:
            _type = Type::Unknown;
    }
}
