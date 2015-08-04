//
//  AnimSkeleton.cpp
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimSkeleton.h"
#include "AnimationLogging.h"
#include "glmHelpers.h"

const AnimPose AnimPose::identity = AnimPose(glm::vec3(1.0f),
                                             glm::quat(),
                                             glm::vec3(0.0f));

AnimPose::AnimPose(const glm::mat4& mat) {
    scale = extractScale(mat);
    rot = glm::normalize(glm::quat_cast(mat));
    trans = extractTranslation(mat);
}

glm::vec3 AnimPose::operator*(const glm::vec3& rhs) const {
    return trans + (rot * (scale * rhs));
}

AnimPose AnimPose::operator*(const AnimPose& rhs) const {
    return AnimPose(static_cast<glm::mat4>(*this) * static_cast<glm::mat4>(rhs));
}

AnimPose AnimPose::inverse() const {
    return AnimPose(glm::inverse(static_cast<glm::mat4>(*this)));
}

AnimPose::operator glm::mat4() const {
    glm::vec3 xAxis = rot * glm::vec3(scale.x, 0.0f, 0.0f);
    glm::vec3 yAxis = rot * glm::vec3(0.0f, scale.y, 0.0f);
    glm::vec3 zAxis = rot * glm::vec3(0.0f, 0.0f, scale.z);
    return glm::mat4(glm::vec4(xAxis, 0.0f), glm::vec4(yAxis, 0.0f),
                     glm::vec4(zAxis, 0.0f), glm::vec4(trans, 1.0f));
}


AnimSkeleton::AnimSkeleton(const std::vector<FBXJoint>& joints) {
    _joints = joints;

    // build a cache of bind poses
    _absoluteBindPoses.reserve(joints.size());
    _relativeBindPoses.reserve(joints.size());
    for (size_t i = 0; i < joints.size(); i++) {

        AnimPose absoluteBindPose(_joints[i].bindTransform);
        _absoluteBindPoses.push_back(absoluteBindPose);

        int parentIndex = getParentIndex(i);
        if (parentIndex >= 0) {
            AnimPose inverseParentAbsoluteBindPose = _absoluteBindPoses[parentIndex].inverse();
            _relativeBindPoses.push_back(inverseParentAbsoluteBindPose * absoluteBindPose);
        } else {
            _relativeBindPoses.push_back(absoluteBindPose);
        }

        // AJT:
        // Attempt to use relative bind pose.. but it's not working.
        /*
        AnimPose relBindPose(glm::vec3(1.0f), _joints[i].rotation, _joints[i].translation);
        _relativeBindPoses.push_back(relBindPose);

        int parentIndex = getParentIndex(i);
        if (parentIndex >= 0) {
            AnimPose parentAbsBindPose = _absoluteBindPoses[parentIndex];
            _absoluteBindPoses.push_back(parentAbsBindPose * relBindPose);
        } else {
            _absoluteBindPoses.push_back(relBindPose);
        }
        */
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

AnimPose AnimSkeleton::getAbsoluteBindPose(int jointIndex) const {
    return _absoluteBindPoses[jointIndex];
}

AnimPose AnimSkeleton::getRelativeBindPose(int jointIndex) const {
    return _relativeBindPoses[jointIndex];
}

int AnimSkeleton::getParentIndex(int jointIndex) const {
    return _joints[jointIndex].parentIndex;
}

const QString& AnimSkeleton::getJointName(int jointIndex) const {
    return _joints[jointIndex].name;
}
