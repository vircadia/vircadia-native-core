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
#include "AnimationLogging.h"

AnimController::AnimController(const std::string& id, float alpha) :
    AnimNode(AnimNode::Type::Controller, id),
    _alpha(alpha) {

}

AnimController::~AnimController() {

}

const AnimPoseVec& AnimController::evaluate(const AnimVariantMap& animVars, float dt, Triggers& triggersOut) {
    _alpha = animVars.lookup(_alphaVar, _alpha);

    for (auto& jointVar : _jointVars) {
        if (!jointVar.hasPerformedJointLookup) {
            QString qJointName = QString::fromStdString(jointVar.jointName);
            jointVar.jointIndex = _skeleton->nameToJointIndex(qJointName);
            if (jointVar.jointIndex < 0) {
                qCWarning(animation) << "AnimController could not find jointName" << jointVar.jointIndex << "in skeleton";
            }
            jointVar.hasPerformedJointLookup = true;
        }
        if (jointVar.jointIndex >= 0) {
            AnimPose pose(animVars.lookup(jointVar.var, glm::mat4()));
            _poses[jointVar.jointIndex] = pose;
        }
    }

    return _poses;
}

const AnimPoseVec& AnimController::overlay(const AnimVariantMap& animVars, float dt, Triggers& triggersOut, const AnimPoseVec& underPoses) {
    _alpha = animVars.lookup(_alphaVar, _alpha);

    for (auto& jointVar : _jointVars) {
        if (!jointVar.hasPerformedJointLookup) {
            QString qJointName = QString::fromStdString(jointVar.jointName);
            jointVar.jointIndex = _skeleton->nameToJointIndex(qJointName);
            if (jointVar.jointIndex < 0) {
                qCWarning(animation) << "AnimController could not find jointName" << jointVar.jointIndex << "in skeleton";
            }
            jointVar.hasPerformedJointLookup = true;
        }

        if (jointVar.jointIndex >= 0) {
            AnimPose pose;
            if (jointVar.jointIndex <= (int)underPoses.size()) {
                pose = AnimPose(animVars.lookup(jointVar.var, underPoses[jointVar.jointIndex]));
            } else {
                pose = AnimPose(animVars.lookup(jointVar.var, AnimPose::identity));
            }
            _poses[jointVar.jointIndex] = pose;
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
