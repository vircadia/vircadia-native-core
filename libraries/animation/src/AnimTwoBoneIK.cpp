//
//  AnimTwoBoneIK.cpp
//
//  Created by Anthony J. Thibault on 5/12/18.
//  Copyright (c) 2018 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimTwoBoneIK.h"

#include <DebugDraw.h>

#include "AnimationLogging.h"
#include "AnimUtil.h"

const float FRAMES_PER_SECOND = 30.0f;

AnimTwoBoneIK::AnimTwoBoneIK(const QString& id, float alpha, bool enabled, float interpDuration,
                             const QString& baseJointName, const QString& midJointName,
                             const QString& tipJointName, const glm::vec3& midHingeAxis,
                             const QString& alphaVar, const QString& enabledVar,
                             const QString& endEffectorRotationVarVar, const QString& endEffectorPositionVarVar) :
    AnimNode(AnimNode::Type::TwoBoneIK, id),
    _alpha(alpha),
    _enabled(enabled),
    _interpDuration(interpDuration),
    _baseJointName(baseJointName),
    _midJointName(midJointName),
    _tipJointName(tipJointName),
    _midHingeAxis(glm::normalize(midHingeAxis)),
    _alphaVar(alphaVar),
    _enabledVar(enabledVar),
    _endEffectorRotationVarVar(endEffectorRotationVarVar),
    _endEffectorPositionVarVar(endEffectorPositionVarVar),
    _prevEndEffectorRotationVar(),
    _prevEndEffectorPositionVar()
{

}

AnimTwoBoneIK::~AnimTwoBoneIK() {

}

const AnimPoseVec& AnimTwoBoneIK::evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, AnimVariantMap& triggersOut) {

    assert(_children.size() == 1);
    if (_children.size() != 1) {
        return _poses;
    }

    // evalute underPoses
    AnimPoseVec underPoses = _children[0]->evaluate(animVars, context, dt, triggersOut);

    // if we don't have a skeleton, or jointName lookup failed.
    if (!_skeleton || _baseJointIndex == -1 || _midJointIndex == -1 || _tipJointIndex == -1 || underPoses.size() == 0) {
        // pass underPoses through unmodified.
        _poses = underPoses;
        return _poses;
    }

    // guard against size changes
    if (underPoses.size() != _poses.size()) {
        _poses = underPoses;
    }

    const float MIN_ALPHA = 0.0f;
    const float MAX_ALPHA = 1.0f;
    float alpha = glm::clamp(animVars.lookup(_alphaVar, _alpha), MIN_ALPHA, MAX_ALPHA);

    // don't perform IK if we have bad indices, or alpha is zero
    if (_tipJointIndex == -1 || _midJointIndex == -1 || _baseJointIndex == -1 || alpha == 0.0f) {
        _poses = underPoses;
        return _poses;
    }

    // determine if we should interpolate
    bool enabled = animVars.lookup(_enabledVar, _enabled);
    if (enabled != _enabled) {
        AnimChain poseChain;
        poseChain.buildFromRelativePoses(_skeleton, _poses, _tipJointIndex);
        if (enabled) {
            beginInterp(InterpType::SnapshotToSolve, poseChain);
        } else {
            beginInterp(InterpType::SnapshotToUnderPoses, poseChain);
        }
    }
    _enabled = enabled;

    // don't build chains or do IK if we are disbled & not interping.
    if (_interpType == InterpType::None && !enabled) {
        _poses = underPoses;
        return _poses;
    }

    // compute chain
    AnimChain underChain;
    underChain.buildFromRelativePoses(_skeleton, underPoses, _tipJointIndex);
    AnimChain ikChain = underChain;

    AnimPose baseParentPose = ikChain.getAbsolutePoseFromJointIndex(_baseParentJointIndex);
    AnimPose basePose = ikChain.getAbsolutePoseFromJointIndex(_baseJointIndex);
    AnimPose midPose = ikChain.getAbsolutePoseFromJointIndex(_midJointIndex);
    AnimPose tipPose = ikChain.getAbsolutePoseFromJointIndex(_tipJointIndex);

    QString endEffectorRotationVar = animVars.lookup(_endEffectorRotationVarVar, QString(""));
    QString endEffectorPositionVar = animVars.lookup(_endEffectorPositionVarVar, QString(""));

    // if either of the endEffectorVars have changed
    if ((!_prevEndEffectorRotationVar.isEmpty() && (_prevEndEffectorRotationVar != endEffectorRotationVar)) ||
        (!_prevEndEffectorPositionVar.isEmpty() && (_prevEndEffectorPositionVar != endEffectorPositionVar))) {
        // begin interp to smooth out transition between prev and new end effector.
        AnimChain poseChain;
        poseChain.buildFromRelativePoses(_skeleton, _poses, _tipJointIndex);
        beginInterp(InterpType::SnapshotToSolve, poseChain);
    }

    // Look up end effector from animVars, make sure to convert into geom space.
    // First look in the triggers then look in the animVars, so we can follow output joints underneath us in the anim graph
    AnimPose targetPose(tipPose);
    if (triggersOut.hasKey(endEffectorRotationVar)) {
        targetPose.rot() = triggersOut.lookupRigToGeometry(endEffectorRotationVar, tipPose.rot());
    } else if (animVars.hasKey(endEffectorRotationVar)) {
        targetPose.rot() = animVars.lookupRigToGeometry(endEffectorRotationVar, tipPose.rot());
    }

    if (triggersOut.hasKey(endEffectorPositionVar)) {
        targetPose.trans() = triggersOut.lookupRigToGeometry(endEffectorPositionVar, tipPose.trans());
    } else if (animVars.hasKey(endEffectorRotationVar)) {
        targetPose.trans() = animVars.lookupRigToGeometry(endEffectorPositionVar, tipPose.trans());
    }

    _prevEndEffectorRotationVar = endEffectorRotationVar;
    _prevEndEffectorPositionVar = endEffectorPositionVar;

    glm::vec3 bicepVector = midPose.trans() - basePose.trans();
    float r0 = glm::length(bicepVector);
    bicepVector = bicepVector / r0;

    glm::vec3 forearmVector = tipPose.trans() - midPose.trans();
    float r1 = glm::length(forearmVector);
    forearmVector = forearmVector / r1;

    float d = glm::length(targetPose.trans() - basePose.trans());

    // http://mathworld.wolfram.com/Circle-CircleIntersection.html
    float midAngle = 0.0f;
    if (d < r0 + r1) {
        float y = sqrtf((-d + r1 - r0) * (-d - r1 + r0) * (-d + r1 + r0) * (d + r1 + r0)) / (2.0f * d);
        midAngle = PI - (acosf(y / r0) + acosf(y / r1));
    }

    // compute midJoint rotation
    glm::quat relMidRot = glm::angleAxis(midAngle, _midHingeAxis);

    // insert new relative pose into the chain and rebuild it.
    ikChain.setRelativePoseAtJointIndex(_midJointIndex, AnimPose(relMidRot, underPoses[_midJointIndex].trans()));
    ikChain.buildDirtyAbsolutePoses();

    // recompute tip pose after mid joint has been rotated
    AnimPose newTipPose = ikChain.getAbsolutePoseFromJointIndex(_tipJointIndex);

    glm::vec3 leverArm = newTipPose.trans() - basePose.trans();
    glm::vec3 targetLine = targetPose.trans() - basePose.trans();

    // compute delta rotation that brings leverArm parallel to targetLine
    glm::vec3 axis = glm::cross(leverArm, targetLine);
    float axisLength = glm::length(axis);
    const float MIN_AXIS_LENGTH = 1.0e-4f;
    if (axisLength > MIN_AXIS_LENGTH) {
        axis /= axisLength;
        float cosAngle = glm::clamp(glm::dot(leverArm, targetLine) / (glm::length(leverArm) * glm::length(targetLine)), -1.0f, 1.0f);
        float angle = acosf(cosAngle);
        glm::quat deltaRot = glm::angleAxis(angle, axis);

        // combine deltaRot with basePose.
        glm::quat absRot = deltaRot * basePose.rot();

        // transform result back into parent relative frame.
        glm::quat relBaseRot = glm::inverse(baseParentPose.rot()) * absRot;
        ikChain.setRelativePoseAtJointIndex(_baseJointIndex, AnimPose(relBaseRot, underPoses[_baseJointIndex].trans()));
    }

    // recompute midJoint pose after base has been rotated.
    ikChain.buildDirtyAbsolutePoses();
    AnimPose midJointPose = ikChain.getAbsolutePoseFromJointIndex(_midJointIndex);

    // transform target rotation in to parent relative frame.
    glm::quat relTipRot = glm::inverse(midJointPose.rot()) * targetPose.rot();
    ikChain.setRelativePoseAtJointIndex(_tipJointIndex, AnimPose(relTipRot, underPoses[_tipJointIndex].trans()));

    // blend with the underChain
    ikChain.blend(underChain, alpha);

    // start off by initializing output poses with the underPoses
    _poses = underPoses;

    // apply smooth interpolation
    if (_interpType != InterpType::None) {
        _interpAlpha += _interpAlphaVel * dt;

        if (_interpAlpha < 1.0f) {
            AnimChain interpChain;
            if (_interpType == InterpType::SnapshotToUnderPoses) {
                interpChain = underChain;
                interpChain.blend(_snapshotChain, _interpAlpha);
            } else if (_interpType == InterpType::SnapshotToSolve) {
                interpChain = ikChain;
                interpChain.blend(_snapshotChain, _interpAlpha);
            }
            // copy interpChain into _poses
            interpChain.outputRelativePoses(_poses);
        } else {
            // interpolation complete
            _interpType = InterpType::None;
        }
    }

    if (_interpType == InterpType::None) {
        if (enabled) {
            // copy chain into _poses
            ikChain.outputRelativePoses(_poses);
        } else {
            // copy under chain into _poses
            underChain.outputRelativePoses(_poses);
        }
    }

    if (context.getEnableDebugDrawIKTargets()) {
        const vec4 RED(1.0f, 0.0f, 0.0f, 1.0f);
        const vec4 GREEN(0.0f, 1.0f, 0.0f, 1.0f);
        glm::mat4 rigToAvatarMat = createMatFromQuatAndPos(Quaternions::Y_180, glm::vec3());

        glm::mat4 geomTargetMat = createMatFromQuatAndPos(targetPose.rot(), targetPose.trans());
        glm::mat4 avatarTargetMat = rigToAvatarMat * context.getGeometryToRigMatrix() * geomTargetMat;

        QString name = QString("%1_target").arg(_id);
        DebugDraw::getInstance().addMyAvatarMarker(name, glmExtractRotation(avatarTargetMat),
                                                   extractTranslation(avatarTargetMat), _enabled ? GREEN : RED);
    } else if (_lastEnableDebugDrawIKTargets) {
        QString name = QString("%1_target").arg(_id);
        DebugDraw::getInstance().removeMyAvatarMarker(name);
    }
    _lastEnableDebugDrawIKTargets = context.getEnableDebugDrawIKTargets();

    if (context.getEnableDebugDrawIKChains()) {
        if (_interpType == InterpType::None && enabled) {
            const vec4 CYAN(0.0f, 1.0f, 1.0f, 1.0f);
            ikChain.debugDraw(context.getRigToWorldMatrix() * context.getGeometryToRigMatrix(), CYAN);
        }
    }

    processOutputJoints(triggersOut);

    return _poses;
}

// for AnimDebugDraw rendering
const AnimPoseVec& AnimTwoBoneIK::getPosesInternal() const {
    return _poses;
}

void AnimTwoBoneIK::setSkeletonInternal(AnimSkeleton::ConstPointer skeleton) {
    AnimNode::setSkeletonInternal(skeleton);
    lookUpIndices();
}

void AnimTwoBoneIK::lookUpIndices() {
    assert(_skeleton);

    // look up bone indices by name
    std::vector<int> indices = _skeleton->lookUpJointIndices({_baseJointName, _midJointName, _tipJointName});

    // cache the results
    _baseJointIndex = indices[0];
    _midJointIndex = indices[1];
    _tipJointIndex = indices[2];

    if (_baseJointIndex != -1) {
        _baseParentJointIndex = _skeleton->getParentIndex(_baseJointIndex);
    }
}

void AnimTwoBoneIK::beginInterp(InterpType interpType, const AnimChain& chain) {
    // capture the current poses in a snapshot.
    _snapshotChain = chain;

    _interpType = interpType;
    _interpAlphaVel = FRAMES_PER_SECOND / _interpDuration;
    _interpAlpha = 0.0f;
}
