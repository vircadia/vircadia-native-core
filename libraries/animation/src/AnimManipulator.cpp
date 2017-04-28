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

AnimManipulator::JointVar::JointVar(const QString& jointNameIn, Type rotationTypeIn, Type translationTypeIn,
                                    const QString& rotationVarIn, const QString& translationVarIn) :
    jointName(jointNameIn),
    rotationType(rotationTypeIn),
    translationType(translationTypeIn),
    rotationVar(rotationVarIn),
    translationVar(translationVarIn),
    jointIndex(-1),
    hasPerformedJointLookup(false) {}

AnimManipulator::AnimManipulator(const QString& id, float alpha) :
    AnimNode(AnimNode::Type::Manipulator, id),
    _alpha(alpha) {

}

AnimManipulator::~AnimManipulator() {

}

const AnimPoseVec& AnimManipulator::evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, Triggers& triggersOut) {
    return overlay(animVars, context, dt, triggersOut, _skeleton->getRelativeBindPoses());
}

const AnimPoseVec& AnimManipulator::overlay(const AnimVariantMap& animVars, const AnimContext& context, float dt, Triggers& triggersOut, const AnimPoseVec& underPoses) {
    _alpha = animVars.lookup(_alphaVar, _alpha);

    _poses = underPoses;

    if (underPoses.size() == 0) {
        return _poses;
    }

    for (auto& jointVar : _jointVars) {

        if (!jointVar.hasPerformedJointLookup) {

            // map from joint name to joint index and cache the result.
            jointVar.jointIndex = _skeleton->nameToJointIndex(jointVar.jointName);
            if (jointVar.jointIndex < 0) {
                qCWarning(animation) << "AnimManipulator could not find jointName" << jointVar.jointName << "in skeleton";
            }
            jointVar.hasPerformedJointLookup = true;
        }

        if (jointVar.jointIndex >= 0) {

            // use the underPose as our default value if we can.
            AnimPose defaultRelPose;
            if (jointVar.jointIndex <= (int)underPoses.size()) {
                defaultRelPose = underPoses[jointVar.jointIndex];
            } else {
                defaultRelPose = AnimPose::identity;
            }

            AnimPose relPose = computeRelativePoseFromJointVar(animVars, jointVar, defaultRelPose, underPoses);

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

void AnimManipulator::removeAllJointVars() {
    _jointVars.clear();
}

AnimPose AnimManipulator::computeRelativePoseFromJointVar(const AnimVariantMap& animVars, const JointVar& jointVar,
                                                          const AnimPose& defaultRelPose, const AnimPoseVec& underPoses) {

    AnimPose defaultAbsPose = _skeleton->getAbsolutePose(jointVar.jointIndex, underPoses);

    // compute relative translation
    glm::vec3 relTrans;
    switch (jointVar.translationType) {
        case JointVar::Type::Absolute: {
            glm::vec3 absTrans = animVars.lookupRigToGeometry(jointVar.translationVar, defaultAbsPose.trans());

            // convert to from absolute to relative.
            AnimPose parentAbsPose;
            int parentIndex = _skeleton->getParentIndex(jointVar.jointIndex);
            if (parentIndex >= 0) {
                parentAbsPose = _skeleton->getAbsolutePose(parentIndex, underPoses);
            }

            // convert from absolute to relative
            relTrans = transformPoint(parentAbsPose.inverse(), absTrans);
            break;
        }
        case JointVar::Type::Relative:
            relTrans = animVars.lookupRigToGeometryVector(jointVar.translationVar, defaultRelPose.trans());
            break;
        case JointVar::Type::UnderPose:
            relTrans = underPoses[jointVar.jointIndex].trans();
            break;
        case JointVar::Type::Default:
        default:
            relTrans = defaultRelPose.trans();
            break;
    }

    glm::quat relRot;
    switch (jointVar.rotationType) {
        case JointVar::Type::Absolute: {
            glm::quat absRot = animVars.lookupRigToGeometry(jointVar.translationVar, defaultAbsPose.rot());

            // convert to from absolute to relative.
            AnimPose parentAbsPose;
            int parentIndex = _skeleton->getParentIndex(jointVar.jointIndex);
            if (parentIndex >= 0) {
                parentAbsPose = _skeleton->getAbsolutePose(parentIndex, underPoses);
            }

            // convert from absolute to relative
            relRot = glm::inverse(parentAbsPose.rot()) * absRot;
            break;
        }
        case JointVar::Type::Relative:
            relRot = animVars.lookupRigToGeometry(jointVar.translationVar, defaultRelPose.rot());
            break;
        case JointVar::Type::UnderPose:
            relRot = underPoses[jointVar.jointIndex].rot();
            break;
        case JointVar::Type::Default:
        default:
            relRot = defaultRelPose.rot();
            break;
    }

    return AnimPose(glm::vec3(1), relRot, relTrans);
}
