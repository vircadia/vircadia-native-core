//
//  AnimManipulator.cpp
//
//  Created by Anthony J. Thibault on 9/8/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimManipulator.h"
#include "AnimUtil.h"
#include "AnimationLogging.h"

AnimManipulator::AnimManipulator(const QString& id, float alpha) :
    AnimNode(AnimNode::Type::Manipulator, id),
    _alpha(alpha) {

}

AnimManipulator::~AnimManipulator() {

}

const AnimPoseVec& AnimManipulator::evaluate(const AnimVariantMap& animVars, float dt, Triggers& triggersOut) {
    return overlay(animVars, dt, triggersOut, _skeleton->getRelativeBindPoses());
}

const AnimPoseVec& AnimManipulator::overlay(const AnimVariantMap& animVars, float dt, Triggers& triggersOut, const AnimPoseVec& underPoses) {
    _alpha = animVars.lookup(_alphaVar, _alpha);

    for (auto& jointVar : _jointVars) {
        if (!jointVar.hasPerformedJointLookup) {
            jointVar.jointIndex = _skeleton->nameToJointIndex(jointVar.jointName);
            if (jointVar.jointIndex < 0) {
                qCWarning(animation) << "AnimManipulator could not find jointName" << jointVar.jointName << "in skeleton";
            }
            jointVar.hasPerformedJointLookup = true;
        }

        if (jointVar.jointIndex >= 0) {

            AnimPose defaultAbsPose;
            AnimPose defaultRelPose;
            AnimPose parentAbsPose = AnimPose::identity;
            if (jointVar.jointIndex <= (int)underPoses.size()) {

                // jointVar is an absolute rotation, if it is not set we will use the underPose as our default value
                defaultRelPose = underPoses[jointVar.jointIndex];
                defaultAbsPose = _skeleton->getAbsolutePose(jointVar.jointIndex, underPoses);
                defaultAbsPose.rot = animVars.lookup(jointVar.var, defaultAbsPose.rot);

                // because jointVar is absolute, we must use an absolute parent frame to convert into a relative pose.
                int parentIndex = _skeleton->getParentIndex(jointVar.jointIndex);
                if (parentIndex >= 0) {
                    parentAbsPose = _skeleton->getAbsolutePose(parentIndex, underPoses);
                }

            } else {

                // jointVar is an absolute rotation, if it is not set we will use the bindPose as our default value
                defaultRelPose = AnimPose::identity;
                defaultAbsPose = _skeleton->getAbsoluteBindPose(jointVar.jointIndex);
                defaultAbsPose.rot = animVars.lookup(jointVar.var, defaultAbsPose.rot);

                // because jointVar is absolute, we must use an absolute parent frame to convert into a relative pose
                // here we use the bind pose
                int parentIndex = _skeleton->getParentIndex(jointVar.jointIndex);
                if (parentIndex >= 0) {
                    parentAbsPose = _skeleton->getAbsoluteBindPose(parentIndex);
                }
            }

            // convert from absolute to relative
            AnimPose relPose = parentAbsPose.inverse() * defaultAbsPose;

            // blend with underPose
            ::blend(1, &defaultRelPose, &relPose, _alpha, &_poses[jointVar.jointIndex]);
        }
    }

    return _poses;
}

void AnimManipulator::setSkeletonInternal(AnimSkeleton::ConstPointer skeleton) {
    AnimNode::setSkeletonInternal(skeleton);

    // invalidate all jointVar indices
    for (auto& jointVar : _jointVars) {
        jointVar.jointIndex = -1;
        jointVar.hasPerformedJointLookup = false;
    }

    // potentially allocate new joints.
    _poses.resize(skeleton->getNumJoints());

    // set all joints to identity
    for (auto& pose : _poses) {
        pose = AnimPose::identity;
    }
}

// for AnimDebugDraw rendering
const AnimPoseVec& AnimManipulator::getPosesInternal() const {
    return _poses;
}

void AnimManipulator::addJointVar(const JointVar& jointVar) {
    _jointVars.push_back(jointVar);
}
