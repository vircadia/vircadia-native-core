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

    AnimSkeleton(const FBXGeometry& fbxGeometry);
    AnimSkeleton(const std::vector<FBXJoint>& joints, const AnimPose& geometryOffset);
    int nameToJointIndex(const QString& jointName) const;
    const QString& getJointName(int jointIndex) const;
    int getNumJoints() const;

    // absolute pose, not relative to parent
    const AnimPose& getAbsoluteBindPose(int jointIndex) const;
    AnimPose getRootAbsoluteBindPoseByChildName(const QString& childName) const;

    // relative to parent pose
    const AnimPose& getRelativeBindPose(int jointIndex) const;
    const AnimPoseVec& getRelativeBindPoses() const { return _relativeBindPoses; }

    int getParentIndex(int jointIndex) const;

    AnimPose getAbsolutePose(int jointIndex, const AnimPoseVec& poses) const;

#ifndef NDEBUG
    void dump() const;
    void dump(const AnimPoseVec& poses) const;
#endif

protected:
    void buildSkeletonFromJoints(const std::vector<FBXJoint>& joints, const AnimPose& geometryOffset);

    std::vector<FBXJoint> _joints;
    AnimPoseVec _absoluteBindPoses;
    AnimPoseVec _relativeBindPoses;

    // no copies
    AnimSkeleton(const AnimSkeleton&) = delete;
    AnimSkeleton& operator=(const AnimSkeleton&) = delete;
};

#endif
