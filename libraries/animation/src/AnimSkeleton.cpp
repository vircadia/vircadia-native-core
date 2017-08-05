//
//  AnimSkeleton.cpp
//
//  Created by Anthony J. Thibault on 9/2/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimSkeleton.h"

#include <glm/gtx/transform.hpp>

#include <GLMHelpers.h>

#include "AnimationLogging.h"

AnimSkeleton::AnimSkeleton(const FBXGeometry& fbxGeometry) {
    // convert to std::vector of joints
    std::vector<FBXJoint> joints;
    joints.reserve(fbxGeometry.joints.size());
    for (auto& joint : fbxGeometry.joints) {
        joints.push_back(joint);
    }
    buildSkeletonFromJoints(joints);
}

AnimSkeleton::AnimSkeleton(const std::vector<FBXJoint>& joints) {
    buildSkeletonFromJoints(joints);
}

int AnimSkeleton::nameToJointIndex(const QString& jointName) const {
    auto itr = _jointIndicesByName.find(jointName);
    if (_jointIndicesByName.end() != itr) {
        return itr.value();
    }
    return -1;
}

int AnimSkeleton::getNumJoints() const {
    return _jointsSize;
}

int AnimSkeleton::getChainDepth(int jointIndex) const {
    if (jointIndex >= 0) {
        int chainDepth = 0;
        int index = jointIndex;
        do {
            chainDepth++;
            index = _joints[index].parentIndex;
        } while (index != -1);
        return chainDepth;
    } else {
        return 0;
    }
}

const AnimPose& AnimSkeleton::getAbsoluteBindPose(int jointIndex) const {
    return _absoluteBindPoses[jointIndex];
}

const AnimPose& AnimSkeleton::getRelativeBindPose(int jointIndex) const {
    return _relativeBindPoses[jointIndex];
}

const AnimPose& AnimSkeleton::getRelativeDefaultPose(int jointIndex) const {
    return _relativeDefaultPoses[jointIndex];
}

const AnimPose& AnimSkeleton::getAbsoluteDefaultPose(int jointIndex) const {
    return _absoluteDefaultPoses[jointIndex];
}

// get pre multiplied transform which should include FBX pre potations
const AnimPose& AnimSkeleton::getPreRotationPose(int jointIndex) const {
    return _relativePreRotationPoses[jointIndex];
}

// get post multiplied transform which might include FBX offset transformations
const AnimPose& AnimSkeleton::getPostRotationPose(int jointIndex) const {
    return _relativePostRotationPoses[jointIndex];
}

int AnimSkeleton::getParentIndex(int jointIndex) const {
    return _joints[jointIndex].parentIndex;
}

const QString& AnimSkeleton::getJointName(int jointIndex) const {
    return _joints[jointIndex].name;
}

AnimPose AnimSkeleton::getAbsolutePose(int jointIndex, const AnimPoseVec& relativePoses) const {
    if (jointIndex < 0 || jointIndex >= (int)relativePoses.size() || jointIndex >= _jointsSize) {
        return AnimPose::identity;
    } else {
        return getAbsolutePose(_joints[jointIndex].parentIndex, relativePoses) * relativePoses[jointIndex];
    }
}

void AnimSkeleton::convertRelativePosesToAbsolute(AnimPoseVec& poses) const {
    // poses start off relative and leave in absolute frame
    int lastIndex = std::min((int)poses.size(), _jointsSize);
    for (int i = 0; i < lastIndex; ++i) {
        int parentIndex = _joints[i].parentIndex;
        if (parentIndex != -1) {
            poses[i] = poses[parentIndex] * poses[i];
        }
    }
}

void AnimSkeleton::convertAbsolutePosesToRelative(AnimPoseVec& poses) const {
    // poses start off absolute and leave in relative frame
    int lastIndex = std::min((int)poses.size(), _jointsSize);
    for (int i = lastIndex - 1; i >= 0; --i) {
        int parentIndex = _joints[i].parentIndex;
        if (parentIndex != -1) {
            poses[i] = poses[parentIndex].inverse() * poses[i];
        }
    }
}

void AnimSkeleton::convertAbsoluteRotationsToRelative(std::vector<glm::quat>& rotations) const {
    // poses start off absolute and leave in relative frame
    int lastIndex = std::min((int)rotations.size(), _jointsSize);
    for (int i = lastIndex - 1; i >= 0; --i) {
        int parentIndex = _joints[i].parentIndex;
        if (parentIndex != -1) {
            rotations[i] = glm::inverse(rotations[parentIndex]) * rotations[i];
        }
    }
}

void AnimSkeleton::saveNonMirroredPoses(const AnimPoseVec& poses) const {
    _nonMirroredPoses.clear();
    for (int i = 0; i < (int)_nonMirroredIndices.size(); ++i) {
        _nonMirroredPoses.push_back(poses[_nonMirroredIndices[i]]);
    }
}

void AnimSkeleton::restoreNonMirroredPoses(AnimPoseVec& poses) const {
    for (int i = 0; i < (int)_nonMirroredIndices.size(); ++i) {
        int index = _nonMirroredIndices[i];
        poses[index] = _nonMirroredPoses[i];
    }
}

void AnimSkeleton::mirrorRelativePoses(AnimPoseVec& poses) const {
    saveNonMirroredPoses(poses);
    convertRelativePosesToAbsolute(poses);
    mirrorAbsolutePoses(poses);
    convertAbsolutePosesToRelative(poses);
    restoreNonMirroredPoses(poses);
}

void AnimSkeleton::mirrorAbsolutePoses(AnimPoseVec& poses) const {
    AnimPoseVec temp = poses;
    for (int i = 0; i < (int)poses.size(); i++) {
        poses[_mirrorMap[i]] = temp[i].mirror();
    }
}

void AnimSkeleton::buildSkeletonFromJoints(const std::vector<FBXJoint>& joints) {
    _joints = joints;
    _jointsSize = (int)joints.size();
    // build a cache of bind poses
    _absoluteBindPoses.reserve(_jointsSize);
    _relativeBindPoses.reserve(_jointsSize);

    // build a chache of default poses
    _absoluteDefaultPoses.reserve(_jointsSize);
    _relativeDefaultPoses.reserve(_jointsSize);
    _relativePreRotationPoses.reserve(_jointsSize);
    _relativePostRotationPoses.reserve(_jointsSize);

    // iterate over FBXJoints and extract the bind pose information.
    for (int i = 0; i < _jointsSize; i++) {

        // build pre and post transforms
        glm::mat4 preRotationTransform = _joints[i].preTransform * glm::mat4_cast(_joints[i].preRotation);
        glm::mat4 postRotationTransform = glm::mat4_cast(_joints[i].postRotation) * _joints[i].postTransform;
        _relativePreRotationPoses.push_back(AnimPose(preRotationTransform));
        _relativePostRotationPoses.push_back(AnimPose(postRotationTransform));

        // build relative and absolute default poses
        glm::mat4 relDefaultMat = glm::translate(_joints[i].translation) * preRotationTransform * glm::mat4_cast(_joints[i].rotation) * postRotationTransform;
        AnimPose relDefaultPose(relDefaultMat);
        _relativeDefaultPoses.push_back(relDefaultPose);
        int parentIndex = getParentIndex(i);
        if (parentIndex >= 0) {
            _absoluteDefaultPoses.push_back(_absoluteDefaultPoses[parentIndex] * relDefaultPose);
        } else {
            _absoluteDefaultPoses.push_back(relDefaultPose);
        }

        // build relative and absolute bind poses
        if (_joints[i].bindTransformFoundInCluster) {
            // Use the FBXJoint::bindTransform, which is absolute model coordinates
            // i.e. not relative to it's parent.
            AnimPose absoluteBindPose(_joints[i].bindTransform);
            _absoluteBindPoses.push_back(absoluteBindPose);
            if (parentIndex >= 0) {
                AnimPose inverseParentAbsoluteBindPose = _absoluteBindPoses[parentIndex].inverse();
                _relativeBindPoses.push_back(inverseParentAbsoluteBindPose * absoluteBindPose);
            } else {
                _relativeBindPoses.push_back(absoluteBindPose);
            }
        } else {
            // use default transform instead
            _relativeBindPoses.push_back(relDefaultPose);
            if (parentIndex >= 0) {
                _absoluteBindPoses.push_back(_absoluteBindPoses[parentIndex] * relDefaultPose);
            } else {
                _absoluteBindPoses.push_back(relDefaultPose);
            }
        }
    }

    for (int i = 0; i < _jointsSize; i++) {
        _jointIndicesByName[_joints[i].name] = i;
    }

    // build mirror map.
    _nonMirroredIndices.clear();
    _mirrorMap.reserve(_jointsSize);
    for (int i = 0; i < _jointsSize; i++) {
        if (_joints[i].name.endsWith("tEye")) {
            // HACK: we don't want to mirror some joints so we remember their indices
            // so we can restore them after a future mirror operation
            _nonMirroredIndices.push_back(i);
        }
        int mirrorJointIndex = -1;
        if (_joints[i].name.startsWith("Left")) {
            QString mirrorJointName = QString(_joints[i].name).replace(0, 4, "Right");
            mirrorJointIndex = nameToJointIndex(mirrorJointName);
        } else if (_joints[i].name.startsWith("Right")) {
            QString mirrorJointName = QString(_joints[i].name).replace(0, 5, "Left");
            mirrorJointIndex = nameToJointIndex(mirrorJointName);
        }
        if (mirrorJointIndex >= 0) {
            _mirrorMap.push_back(mirrorJointIndex);
        } else {
            _mirrorMap.push_back(i);
        }
    }
}

void AnimSkeleton::dump(bool verbose) const {
    qCDebug(animation) << "[";
    for (int i = 0; i < getNumJoints(); i++) {
        qCDebug(animation) << "    {";
        qCDebug(animation) << "        index =" << i;
        qCDebug(animation) << "        name =" << getJointName(i);
        qCDebug(animation) << "        absBindPose =" << getAbsoluteBindPose(i);
        qCDebug(animation) << "        relBindPose =" << getRelativeBindPose(i);
        qCDebug(animation) << "        absDefaultPose =" << getAbsoluteDefaultPose(i);
        qCDebug(animation) << "        relDefaultPose =" << getRelativeDefaultPose(i);
        if (verbose) {
            qCDebug(animation) << "        fbxJoint =";
            qCDebug(animation) << "            isFree =" << _joints[i].isFree;
            qCDebug(animation) << "            freeLineage =" << _joints[i].freeLineage;
            qCDebug(animation) << "            parentIndex =" << _joints[i].parentIndex;
            qCDebug(animation) << "            translation =" << _joints[i].translation;
            qCDebug(animation) << "            preTransform =" << _joints[i].preTransform;
            qCDebug(animation) << "            preRotation =" << _joints[i].preRotation;
            qCDebug(animation) << "            rotation =" << _joints[i].rotation;
            qCDebug(animation) << "            postRotation =" << _joints[i].postRotation;
            qCDebug(animation) << "            postTransform =" << _joints[i].postTransform;
            qCDebug(animation) << "            transform =" << _joints[i].transform;
            qCDebug(animation) << "            rotationMin =" << _joints[i].rotationMin << ", rotationMax =" << _joints[i].rotationMax;
            qCDebug(animation) << "            inverseDefaultRotation" << _joints[i].inverseDefaultRotation;
            qCDebug(animation) << "            inverseBindRotation" << _joints[i].inverseBindRotation;
            qCDebug(animation) << "            bindTransform" << _joints[i].bindTransform;
            qCDebug(animation) << "            isSkeletonJoint" << _joints[i].isSkeletonJoint;
        }
        if (getParentIndex(i) >= 0) {
            qCDebug(animation) << "        parent =" << getJointName(getParentIndex(i));
        }
        qCDebug(animation) << "    },";
    }
    qCDebug(animation) << "]";
}

void AnimSkeleton::dump(const AnimPoseVec& poses) const {
    qCDebug(animation) << "[";
    for (int i = 0; i < getNumJoints(); i++) {
        qCDebug(animation) << "    {";
        qCDebug(animation) << "        index =" << i;
        qCDebug(animation) << "        name =" << getJointName(i);
        qCDebug(animation) << "        absBindPose =" << getAbsoluteBindPose(i);
        qCDebug(animation) << "        relBindPose =" << getRelativeBindPose(i);
        qCDebug(animation) << "        absDefaultPose =" << getAbsoluteDefaultPose(i);
        qCDebug(animation) << "        relDefaultPose =" << getRelativeDefaultPose(i);
        qCDebug(animation) << "        pose =" << poses[i];
        if (getParentIndex(i) >= 0) {
            qCDebug(animation) << "        parent =" << getJointName(getParentIndex(i));
        }
        qCDebug(animation) << "    },";
    }
    qCDebug(animation) << "]";
}

