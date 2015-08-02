//
//  AnimSkeleton.cpp
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimSkeleton.h"
#include "glmHelpers.h"

const AnimPose AnimPose::identity = AnimPose(glm::vec3(1.0f),
                                             glm::quat(),
                                             glm::vec3(0.0f));

AnimSkeleton::AnimSkeleton(const std::vector<FBXJoint>& joints) {
    _joints = joints;

    // build a cache of bind poses
    _bindPoses.reserve(joints.size());
    for (size_t i = 0; i < joints.size(); i++) {
        _bindPoses.push_back(AnimPose(extractScale(_joints[i].bindTransform),
                                      glm::quat_cast(_joints[i].bindTransform),
                                      extractTranslation(_joints[i].bindTransform)));
    }
}

int AnimSkeleton::nameToJointIndex(const QString& jointName) const {
    for (size_t i = 0; i < _joints.size(); i++) {
        if (_joints[i].name == jointName) {
            return i;
        }
    }
    return -1;
}

int AnimSkeleton::getNumJoints() const {
    return _joints.size();
}

AnimPose AnimSkeleton::getBindPose(int jointIndex) const {
    return _bindPoses[jointIndex];
}

int AnimSkeleton::getParentIndex(int jointIndex) const {
    return _joints[jointIndex].parentIndex;
}


