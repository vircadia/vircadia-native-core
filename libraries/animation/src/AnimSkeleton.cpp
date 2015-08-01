//
//  AnimSkeleton.cpp
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimSkeleton.h"

AnimSkeleton::AnimSkeleton(const std::vector<FBXJoint>& joints) {
    _joints = joints;
}

int AnimSkeltion::nameToJointIndex(const QString& jointName) const {
    for (int i = 0; i < _joints.size(); i++) {
        if (_joints.name == jointName) {
            return i;
        }
    }
    return -1;
}

int AnimSkeleton::getNumJoints() const {
    return _joints.size();
}

AnimBone getBindPose(int jointIndex) const {
    // TODO: what coordinate frame is the bindTransform in? local to the bones parent frame? or model?
    return AnimBone bone(glm::vec3(1.0f, 1.0f, 1.0f),
                         glm::quat_cast(_joints[jointIndex].bindTransform),
                         glm::vec3(0.0f, 0.0f, 0.0f));
}


