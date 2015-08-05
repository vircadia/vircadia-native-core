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
#include "GLMHelpers.h"
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>

const AnimPose AnimPose::identity = AnimPose(glm::vec3(1.0f),
                                             glm::quat(),
                                             glm::vec3(0.0f));

AnimPose::AnimPose(const glm::mat4& mat) {
    scale = extractScale(mat);
    rot = extractRotation(mat);
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

//#define TRUST_BIND_TRANSFORM

AnimSkeleton::AnimSkeleton(const std::vector<FBXJoint>& joints) {
    _joints = joints;

    // build a cache of bind poses
    _absoluteBindPoses.reserve(joints.size());
    _relativeBindPoses.reserve(joints.size());
    for (size_t i = 0; i < joints.size(); i++) {

        /*
        // AJT: dump the skeleton, because wtf.
        qCDebug(animation) << getJointName(i);
        qCDebug(animation) << "    isFree =" << _joints[i].isFree;
        qCDebug(animation) << "    freeLineage =" << _joints[i].freeLineage;
        qCDebug(animation) << "    parentIndex =" << _joints[i].parentIndex;
        qCDebug(animation) << "    boneRadius =" << _joints[i].boneRadius;
        qCDebug(animation) << "    translation =" << _joints[i].translation;
        qCDebug(animation) << "    preTransform =" << _joints[i].preTransform;
        qCDebug(animation) << "    preRotation =" << _joints[i].preRotation;
        qCDebug(animation) << "    rotation =" << _joints[i].rotation;
        qCDebug(animation) << "    postRotation =" << _joints[i].postRotation;
        qCDebug(animation) << "    postTransform =" << _joints[i].postTransform;
        qCDebug(animation) << "    transform =" << _joints[i].transform;
        qCDebug(animation) << "    rotationMin =" << _joints[i].rotationMin << ", rotationMax =" << _joints[i].rotationMax;
        qCDebug(animation) << "    inverseDefaultRotation" << _joints[i].inverseDefaultRotation;
        qCDebug(animation) << "    inverseBindRotation" << _joints[i].inverseBindRotation;
        qCDebug(animation) << "    bindTransform" << _joints[i].bindTransform;
        qCDebug(animation) << "    isSkeletonJoint" << _joints[i].isSkeletonJoint;
        */

#ifdef TRUST_BIND_TRANSFORM
        // trust FBXJoint::bindTransform (which is wrong for joints NOT bound to anything)
        AnimPose absoluteBindPose(_joints[i].bindTransform);
        _absoluteBindPoses.push_back(absoluteBindPose);
        int parentIndex = getParentIndex(i);
        if (parentIndex >= 0) {
            AnimPose inverseParentAbsoluteBindPose = _absoluteBindPoses[parentIndex].inverse();
            _relativeBindPoses.push_back(inverseParentAbsoluteBindPose * absoluteBindPose);
        } else {
            _relativeBindPoses.push_back(absoluteBindPose);
        }
#else
        // trust FBXJoint's local transforms (which is not really the bind pose, but the default pose in the fbx)
        glm::mat4 rotTransform = glm::mat4_cast(_joints[i].preRotation * _joints[i].rotation * _joints[i].postRotation);
        glm::mat4 relBindMat = glm::translate(_joints[i].translation) * _joints[i].preTransform * rotTransform * _joints[i].postTransform;
        AnimPose relBindPose(relBindMat);
        _relativeBindPoses.push_back(relBindPose);

        int parentIndex = getParentIndex(i);
        if (parentIndex >= 0) {
            _absoluteBindPoses.push_back(_absoluteBindPoses[parentIndex] * relBindPose);
        } else {
            _absoluteBindPoses.push_back(relBindPose);
        }
#endif
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
