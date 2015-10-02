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

/// Updates the state of the joint at the specified index.
void EntityRig::updateJointState(int index, glm::mat4 rootTransform) {
    JointState& state = _jointStates[index];

    // compute model transforms
    int parentIndex = state.getParentIndex();
    if (parentIndex == -1) {
        state.computeTransform(rootTransform);
    } else {
        // guard against out-of-bounds access to _jointStates
        if (parentIndex >= 0 && parentIndex < _jointStates.size()) {
            const JointState& parentState = _jointStates.at(parentIndex);
            state.computeTransform(parentState.getTransform(), parentState.getTransformChanged());
        }
    }
}


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
