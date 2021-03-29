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

AnimSkeleton::AnimSkeleton(const HFMModel& hfmModel) {

    _geometryOffset = hfmModel.offset;

    // convert to std::vector of joints
    std::vector<HFMJoint> joints;
    joints.reserve(hfmModel.joints.size());
    for (auto& joint : hfmModel.joints) {
        joints.push_back(joint);
    }
    buildSkeletonFromJoints(joints, hfmModel.jointRotationOffsets);

    // we make a copy of the inverseBindMatrices in order to prevent mutating the model bind pose
    // when we are dealing with a joint offset in the model
    for (int i = 0; i < (int)hfmModel.meshes.size(); i++) {
        const HFMMesh& mesh = hfmModel.meshes.at(i);
        std::vector<HFMCluster> dummyClustersList;

        for (int j = 0; j < mesh.clusters.size(); j++) {
            std::vector<glm::mat4> bindMatrices;
            // cast into a non-const reference, so we can mutate the FBXCluster
            HFMCluster& cluster = const_cast<HFMCluster&>(mesh.clusters.at(j));

            HFMCluster localCluster;
            localCluster.jointIndex = cluster.jointIndex;
            localCluster.inverseBindMatrix = cluster.inverseBindMatrix;
            localCluster.inverseBindTransform.evalFromRawMatrix(localCluster.inverseBindMatrix);

            // if we have a joint offset in the fst file then multiply its inverse by the
            // model cluster inverse bind matrix
            if (hfmModel.jointRotationOffsets.contains(cluster.jointIndex)) {
                AnimPose localOffset(hfmModel.jointRotationOffsets[cluster.jointIndex], glm::vec3());
                localCluster.inverseBindMatrix = (glm::mat4)localOffset.inverse() * cluster.inverseBindMatrix;
                localCluster.inverseBindTransform.evalFromRawMatrix(localCluster.inverseBindMatrix);
            }
            dummyClustersList.push_back(localCluster);
        }
        _clusterBindMatrixOriginalValues.push_back(dummyClustersList);
    }
}

AnimSkeleton::AnimSkeleton(const std::vector<HFMJoint>& joints, const QMap<int, glm::quat> jointOffsets) {
    buildSkeletonFromJoints(joints, jointOffsets);
}

int AnimSkeleton::nameToJointIndex(const QString& jointName) const {
    auto itr = _jointIndicesByName.find(jointName);
    if (_jointIndicesByName.end() != itr) {
        return itr.value();
    }
    return INVALID_JOINT_INDEX;
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
            index = _parentIndices[index];
        } while (index != INVALID_JOINT_INDEX);
        return chainDepth;
    } else {
        return 0;
    }
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

std::vector<int> AnimSkeleton::getChildrenOfJoint(int jointIndex) const {
    // Children and grandchildren, etc.
    std::vector<int> result;
    if (jointIndex != INVALID_JOINT_INDEX) {
        for (int i = jointIndex + 1; i < (int)_parentIndices.size(); i++) {
            if (_parentIndices[i] == jointIndex || (std::find(result.begin(), result.end(), _parentIndices[i]) != result.end())) {
                result.push_back(i);
            }
        }
    }
    return result;
}

const QString& AnimSkeleton::getJointName(int jointIndex) const {
    return _joints[jointIndex].name;
}

AnimPose AnimSkeleton::getAbsolutePose(int jointIndex, const AnimPoseVec& relativePoses) const {
    if (jointIndex < 0 || jointIndex >= (int)relativePoses.size() || jointIndex >= _jointsSize) {
        return AnimPose::identity;
    } else {
        return getAbsolutePose(_parentIndices[jointIndex], relativePoses) * relativePoses[jointIndex];
    }
}

void AnimSkeleton::convertRelativePosesToAbsolute(AnimPoseVec& poses) const {
    // poses start off relative and leave in absolute frame
    int lastIndex = std::min((int)poses.size(), _jointsSize);
    for (int i = 0; i < lastIndex; ++i) {
        int parentIndex = _parentIndices[i];
        if (parentIndex != INVALID_JOINT_INDEX) {
            poses[i] = poses[parentIndex] * poses[i];
        }
    }
}

void AnimSkeleton::convertAbsolutePosesToRelative(AnimPoseVec& poses) const {
    // poses start off absolute and leave in relative frame
    int lastIndex = std::min((int)poses.size(), _jointsSize);
    for (int i = lastIndex - 1; i >= 0; --i) {
        int parentIndex = _parentIndices[i];
        if (parentIndex != INVALID_JOINT_INDEX) {
            poses[i] = poses[parentIndex].inverse() * poses[i];
        }
    }
}

void AnimSkeleton::convertRelativeRotationsToAbsolute(std::vector<glm::quat>& rotations) const {
    // rotations start off relative and leave in absolute frame
    int lastIndex = std::min((int)rotations.size(), _jointsSize);
    for (int i = 0; i < lastIndex; ++i) {
        int parentIndex = _parentIndices[i];
        if (parentIndex != INVALID_JOINT_INDEX) {
            rotations[i] = rotations[parentIndex] * rotations[i];
        }
    }
}

void AnimSkeleton::convertAbsoluteRotationsToRelative(std::vector<glm::quat>& rotations) const {
    // rotations start off absolute and leave in relative frame
    int lastIndex = std::min((int)rotations.size(), _jointsSize);
    for (int i = lastIndex - 1; i >= 0; --i) {
        int parentIndex = _parentIndices[i];
        if (parentIndex != INVALID_JOINT_INDEX) {
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

void AnimSkeleton::buildSkeletonFromJoints(const std::vector<HFMJoint>& joints, const QMap<int, glm::quat> jointOffsets) {

    _joints = joints;

    // build a seperate vector of parentIndices for cache coherency
    // AnimSkeleton::getParentIndex is called very frequently in tight loops.
    _parentIndices.reserve(_joints.size());
    for (auto& joint : _joints) {
        _parentIndices.push_back(joint.parentIndex);
    }

    _jointsSize = (int)joints.size();
    // build a cache of bind poses

    // build a chache of default poses
    _absoluteDefaultPoses.reserve(_jointsSize);
    _relativeDefaultPoses.reserve(_jointsSize);
    _relativePreRotationPoses.reserve(_jointsSize);
    _relativePostRotationPoses.reserve(_jointsSize);

    // iterate over HFMJoints and extract the bind pose information.
    for (int i = 0; i < _jointsSize; i++) {

        // build pre and post transforms
        glm::mat4 preRotationTransform = _joints[i].preTransform * glm::mat4_cast(_joints[i].preRotation);
        glm::mat4 postRotationTransform = glm::mat4_cast(_joints[i].postRotation) * _joints[i].postTransform;
        _relativePreRotationPoses.push_back(AnimPose(preRotationTransform));
        _relativePostRotationPoses.push_back(AnimPose(postRotationTransform));

        // build relative and absolute default poses
        glm::mat4 relDefaultMat = glm::translate(_joints[i].translation) * preRotationTransform * glm::mat4_cast(_joints[i].rotation) * postRotationTransform;
        AnimPose relDefaultPose(relDefaultMat);

        int parentIndex = getParentIndex(i);
        if (parentIndex >= 0) {
            _absoluteDefaultPoses.push_back(_absoluteDefaultPoses[parentIndex] * relDefaultPose);
        } else {
            _absoluteDefaultPoses.push_back(relDefaultPose);
        }
    }

    for (int k = 0; k < _jointsSize; k++) {
        if (jointOffsets.contains(k)) {
            AnimPose localOffset(jointOffsets[k], glm::vec3());
            _absoluteDefaultPoses[k] = _absoluteDefaultPoses[k] * localOffset;
        }
    }
    // re-compute relative poses
    _relativeDefaultPoses = _absoluteDefaultPoses;
    convertAbsolutePosesToRelative(_relativeDefaultPoses);

    // build _jointIndicesByName hash
    for (int i = 0; i < _jointsSize; i++) {
        auto iter = _jointIndicesByName.find(_joints[i].name);
        if (iter != _jointIndicesByName.end()) {
            // prefer joints over meshes if there is a name collision.
            if (_joints[i].isSkeletonJoint && !_joints[iter.value()].isSkeletonJoint) {
                iter.value() = i;
            }
        } else {
            _jointIndicesByName.insert(_joints[i].name, i);
        }
    }

    // build mirror map.
    _nonMirroredIndices.clear();
    _mirrorMap.reserve(_jointsSize);
    for (int i = 0; i < _jointsSize; i++) {
        if (_joints[i].name != "Hips" && _joints[i].name != "Spine" &&
            _joints[i].name != "Spine1" && _joints[i].name != "Spine2" &&
            _joints[i].name != "Neck" && _joints[i].name != "Head" &&
            !((_joints[i].name.startsWith("Left") || _joints[i].name.startsWith("Right")) &&
              _joints[i].name != "LeftEye" && _joints[i].name != "RightEye")) {
            // HACK: we don't want to mirror some joints so we remember their indices
            // so we can restore them after a future mirror operation
            _nonMirroredIndices.push_back(i);
        }
        int mirrorJointIndex = INVALID_JOINT_INDEX;
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
        qCDebug(animation) << "        absDefaultPose =" << getAbsoluteDefaultPose(i);
        qCDebug(animation) << "        relDefaultPose =" << getRelativeDefaultPose(i);
        if (verbose) {
            qCDebug(animation) << "        hfmJoint =";
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

std::vector<int> AnimSkeleton::lookUpJointIndices(const std::vector<QString>& jointNames) const {
    std::vector<int> result;
    result.reserve(jointNames.size());
    for (auto& name : jointNames) {
        int index = nameToJointIndex(name);
        if (index == INVALID_JOINT_INDEX) {
            qWarning(animation) << "AnimSkeleton::lookUpJointIndices(): could not find bone with name " << name;
        }
        result.push_back(index);
    }
    return result;
}


