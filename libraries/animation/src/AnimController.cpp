//
//  AnimController.cpp
//
//  Created by Anthony J. Thibault on 9/8/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimController.h"
#include "AnimUtil.h"
#include "AnimationLogging.h"

AnimController::AnimController(const std::string& id, float alpha) :
    AnimNode(AnimNode::Type::Controller, id),
    _alpha(alpha) {

}

AnimController::~AnimController() {

}

const AnimPoseVec& AnimController::evaluate(const AnimVariantMap& animVars, float dt, Triggers& triggersOut) {
    return overlay(animVars, dt, triggersOut, _skeleton->getRelativeBindPoses());
}

const AnimPoseVec& AnimController::overlay(const AnimVariantMap& animVars, float dt, Triggers& triggersOut, const AnimPoseVec& underPoses) {
    _alpha = animVars.lookup(_alphaVar, _alpha);

    for (auto& jointVar : _jointVars) {
        if (!jointVar.hasPerformedJointLookup) {
            QString qJointName = QString::fromStdString(jointVar.jointName);
            jointVar.jointIndex = _skeleton->nameToJointIndex(qJointName);
            if (jointVar.jointIndex < 0) {
                qCWarning(animation) << "AnimController could not find jointName" << qJointName << "in skeleton";
            }
            jointVar.hasPerformedJointLookup = true;
        }

        if (jointVar.jointIndex >= 0) {

            AnimPose defaultAbsPose;
            AnimPose parentAbsPose = AnimPose::identity;
            if (jointVar.jointIndex <= (int)underPoses.size()) {

                // jointVar is an absolute rotation, if it is not set we will use the underPose as our default value
                defaultAbsPose = _skeleton->getAbsolutePose(jointVar.jointIndex, underPoses);
                defaultAbsPose.rot = animVars.lookup(jointVar.var, defaultAbsPose.rot);

                // because jointVar is absolute, we must use an absolute parent frame to convert into a relative pose.
                int parentIndex = _skeleton->getParentIndex(jointVar.jointIndex);
                if (parentIndex >= 0) {
                    parentAbsPose = _skeleton->getAbsolutePose(parentIndex, underPoses);
                }

            } else {

                // jointVar is an absolute rotation, if it is not set we will use the bindPose as our default value
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
            ::blend(1, &underPoses[jointVar.jointIndex], &relPose, _alpha, &_poses[jointVar.jointIndex]);
        }
    }

    return _poses;
}

void AnimController::setSkeletonInternal(AnimSkeleton::ConstPointer skeleton) {
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
const AnimPoseVec& AnimController::getPosesInternal() const {
    return _poses;
}

void AnimController::addJointVar(const JointVar& jointVar) {
    _jointVars.push_back(jointVar);
}
