//
//  EntityRig.cpp
//  libraries/animation/src/
//
//  Created by SethAlves on 2015-7-22.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntityRig.h"

void EntityRig::setJointState(int index, bool valid, const glm::quat& rotation, const glm::vec3& translation, float priority) {
    if (index != -1 && index < _jointStates.size()) {
        JointState& state = _jointStates[index];
        if (valid) {
            state.setRotationInConstrainedFrame(rotation, priority);
            // state.setTranslation(translation, priority);
        } else {
            state.restoreRotation(1.0f, priority);
            // state.restoreTranslation(1.0f, priority);
        }
    }
}
