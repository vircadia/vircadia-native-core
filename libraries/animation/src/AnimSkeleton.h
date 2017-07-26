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

#include <FBXReader.h>
#include "AnimPose.h"

class AnimSkeleton {
public:
    using Pointer = std::shared_ptr<AnimSkeleton>;
    using ConstPointer = std::shared_ptr<const AnimSkeleton>;

    explicit AnimSkeleton(const FBXGeometry& fbxGeometry);
    explicit AnimSkeleton(const std::vector<FBXJoint>& joints);
    int nameToJointIndex(const QString& jointName) const;
    const QString& getJointName(int jointIndex) const;
    int getNumJoints() const;
    int getChainDepth(int jointIndex) const;

    // absolute pose, not relative to parent
    const AnimPose& getAbsoluteBindPose(int jointIndex) const;

    // relative to parent pose
    const AnimPose& getRelativeBindPose(int jointIndex) const;
    const AnimPoseVec& getRelativeBindPoses() const { return _relativeBindPoses; }

    // the default poses are the orientations of the joints on frame 0.
    const AnimPose& getRelativeDefaultPose(int jointIndex) const;
    const AnimPoseVec& getRelativeDefaultPoses() const { return _relativeDefaultPoses; }
    const AnimPose& getAbsoluteDefaultPose(int jointIndex) const;
    const AnimPoseVec& getAbsoluteDefaultPoses() const { return _absoluteDefaultPoses; }

    // get pre transform which should include FBX pre potations
    const AnimPose& getPreRotationPose(int jointIndex) const;

    // get post transform which might include FBX offset transformations
    const AnimPose& getPostRotationPose(int jointIndex) const;

    int getParentIndex(int jointIndex) const;

    AnimPose getAbsolutePose(int jointIndex, const AnimPoseVec& relativePoses) const;

    void convertRelativePosesToAbsolute(AnimPoseVec& poses) const;
    void convertAbsolutePosesToRelative(AnimPoseVec& poses) const;

    void convertAbsoluteRotationsToRelative(std::vector<glm::quat>& rotations) const;

    void saveNonMirroredPoses(const AnimPoseVec& poses) const;
    void restoreNonMirroredPoses(AnimPoseVec& poses) const;

    void mirrorRelativePoses(AnimPoseVec& poses) const;
    void mirrorAbsolutePoses(AnimPoseVec& poses) const;

    void dump(bool verbose) const;
    void dump(const AnimPoseVec& poses) const;

protected:
    void buildSkeletonFromJoints(const std::vector<FBXJoint>& joints);

    std::vector<FBXJoint> _joints;
    int _jointsSize { 0 };
    AnimPoseVec _absoluteBindPoses;
    AnimPoseVec _relativeBindPoses;
    AnimPoseVec _relativeDefaultPoses;
    AnimPoseVec _absoluteDefaultPoses;
    AnimPoseVec _relativePreRotationPoses;
    AnimPoseVec _relativePostRotationPoses;
    mutable AnimPoseVec _nonMirroredPoses;
    std::vector<int> _nonMirroredIndices;
    std::vector<int> _mirrorMap;
    QHash<QString, int> _jointIndicesByName;

    // no copies
    AnimSkeleton(const AnimSkeleton&) = delete;
    AnimSkeleton& operator=(const AnimSkeleton&) = delete;
};

#endif
