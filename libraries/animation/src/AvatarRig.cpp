//
//  AvatarRig.cpp
//  libraries/animation/src/
//
//  Created by SethAlves on 2015-7-22.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AvatarRig.h"

/// Updates the state of the joint at the specified index.
void AvatarRig::updateJointState(int index, glm::mat4 parentTransform) {
    if (index < 0 && index >= _jointStates.size()) {
        return; // bail
    }
    JointState& state = _jointStates[index];
    const FBXJoint& joint = state.getFBXJoint();

    // compute model transforms
    int parentIndex = joint.parentIndex;
    if (parentIndex == -1) {
        state.computeTransform(parentTransform);
        clearJointTransformTranslation(index);
    } else {
        // guard against out-of-bounds access to _jointStates
        if (joint.parentIndex >= 0 && joint.parentIndex < _jointStates.size()) {
            const JointState& parentState = _jointStates.at(parentIndex);
            state.computeTransform(parentState.getTransform(), parentState.getTransformChanged());
        }
    }
}
