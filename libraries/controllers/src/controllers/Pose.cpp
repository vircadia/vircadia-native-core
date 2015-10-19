//
//  Created by Bradley Austin Davis on 2015/10/18
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Pose.h"

namespace controller {

    Pose::Pose(const vec3& translation, const quat& rotation,
            const vec3& velocity, const quat& angularVelocity) :
            _translation(translation), _rotation(rotation), _velocity(velocity), _angularVelocity(angularVelocity) { }

    bool Pose::operator==(const Pose& right) const {
        // invalid poses return false for comparison, even against identical invalid poses, like NaN
        if (_valid || !right._valid) {
            return false;
        }

        // FIXME add margin of error?  Or add an additional withinEpsilon function?
        return _translation == right.getTranslation() && _rotation == right.getRotation() && 
            _velocity == right.getVelocity() && _angularVelocity == right.getAngularVelocity();
    }


}

