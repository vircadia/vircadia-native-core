//
//  Created by Bradley Austin Davis on 2015/10/18
//  (based on UserInputMapper inner class created by Sam Gateau on 4/27/15)
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_controllers_Pose_h
#define hifi_controllers_Pose_h

#include <GLMHelpers.h>

namespace controller {

    struct Pose {
    public:
        vec3 _translation;
        quat _rotation;
        vec3 _velocity;
        quat _angularVelocity;
        bool _valid{ false };

        Pose() {}
        Pose(const vec3& translation, const quat& rotation,
             const vec3& velocity = vec3(), const quat& angularVelocity = quat());

        Pose(const Pose&) = default;
        Pose& operator = (const Pose&) = default;
        bool operator ==(const Pose& right) const;
        bool isValid() const { return _valid; }
        vec3 getTranslation() const { return _translation; }
        quat getRotation() const { return _rotation; }
        vec3 getVelocity() const { return _velocity; }
        quat getAngularVelocity() const { return _angularVelocity; }
    };


}

#endif
