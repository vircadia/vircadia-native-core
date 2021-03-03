//
//  AnimSkeleton.h
//
//  Created by Anthony J. Thibault on 9/2/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimSkeleton
#define hifi_AnimSkeleton

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <FBXSerializer.h>
#include "AnimPose.h"

class AnimSkeleton {
public:
    using Pointer = std::shared_ptr<AnimSkeleton>;
    using ConstPointer = std::shared_ptr<const AnimSkeleton>;

    explicit AnimSkeleton(const HFMModel& hfmModel);
    explicit AnimSkeleton(const std::vector<HFMJoint>& joints, const QMap<int, glm::quat> jointOffsets);

    int nameToJointIndex(const QString& jointName) const;
    const QString& getJointName(int jointIndex) const;
    int getNumJoints() const;
    int getChainDepth(int jointIndex) const;
    
    static const int INVALID_JOINT_INDEX { -1 };

    // the default poses are the orientations of the joints on frame 0.
    const AnimPose& getRelativeDefaultPose(int jointIndex) const;
    const AnimPoseVec& getRelativeDefaultPoses() const { return _relativeDefaultPoses; }
    const AnimPose& getAbsoluteDefaultPose(int jointIndex) const;
    const AnimPoseVec& getAbsoluteDefaultPoses() const { return _absoluteDefaultPoses; }
    const glm::mat4& getGeometryOffset() const { return _geometryOffset; }

    // get pre transform which should include FBX pre potations
    const AnimPose& getPreRotationPose(int jointIndex) const;

    // get post transform which might include FBX offset transformations
    const AnimPose& getPostRotationPose(int jointIndex) const;

    int getParentIndex(int jointIndex) const {
        return _parentIndices[jointIndex];
    }

    std::vector<int> getChildrenOfJoint(int jointIndex) const;

    AnimPose getAbsolutePose(int jointIndex, const AnimPoseVec& relativePoses) const;

    void convertRelativePosesToAbsolute(AnimPoseVec& poses) const;
    void convertAbsolutePosesToRelative(AnimPoseVec& poses) const;

    void convertRelativeRotationsToAbsolute(std::vector<glm::quat>& rotations) const;
    void convertAbsoluteRotationsToRelative(std::vector<glm::quat>& rotations) const;

    void saveNonMirroredPoses(const AnimPoseVec& poses) const;
    void restoreNonMirroredPoses(AnimPoseVec& poses) const;

    void mirrorRelativePoses(AnimPoseVec& poses) const;
    void mirrorAbsolutePoses(AnimPoseVec& poses) const;

    void dump(bool verbose) const;
    void dump(const AnimPoseVec& poses) const;

    std::vector<int> lookUpJointIndices(const std::vector<QString>& jointNames) const;
    const HFMCluster getClusterBindMatricesOriginalValues(const int meshIndex, const int clusterIndex) const { return _clusterBindMatrixOriginalValues[meshIndex][clusterIndex]; }

protected:
    void buildSkeletonFromJoints(const std::vector<HFMJoint>& joints, const QMap<int, glm::quat> jointOffsets);

    std::vector<HFMJoint> _joints;
    std::vector<int> _parentIndices;
    int _jointsSize { 0 };
    AnimPoseVec _relativeDefaultPoses;
    AnimPoseVec _absoluteDefaultPoses;
    AnimPoseVec _relativePreRotationPoses;
    AnimPoseVec _relativePostRotationPoses;
    mutable AnimPoseVec _nonMirroredPoses;
    std::vector<int> _nonMirroredIndices;
    std::vector<int> _mirrorMap;
    QHash<QString, int> _jointIndicesByName;
    std::vector<std::vector<HFMCluster>> _clusterBindMatrixOriginalValues;
    glm::mat4 _geometryOffset;

    // no copies
    AnimSkeleton(const AnimSkeleton&) = delete;
    AnimSkeleton& operator=(const AnimSkeleton&) = delete;
};

#endif
