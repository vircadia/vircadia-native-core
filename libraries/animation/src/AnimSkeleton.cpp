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

AnimSkeleton::AnimSkeleton(const std::vector<FBXJoint>& joints) {
    _joints = joints;
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
    // TODO: perhaps cache these, it's expensive to de-compose the matrix
    // on every call.
    return AnimPose(extractScale(_joints[jointIndex].bindTransform),
                    glm::quat_cast(_joints[jointIndex].bindTransform),
                    extractTranslation(_joints[jointIndex].bindTransform));
}

int AnimSkeleton::getParentIndex(int jointIndex) const {
    return _joints[jointIndex].parentIndex;
}


