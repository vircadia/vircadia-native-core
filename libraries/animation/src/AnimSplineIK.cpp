//
//  AnimSplineIK.cpp
//
//  Created by Angus Antley on 1/7/19.
//  Copyright (c) 2019 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimSplineIK.h"
#include "AnimationLogging.h"
#include "CubicHermiteSpline.h"
#include <DebugDraw.h>
#include "AnimUtil.h"

static const float JOINT_CHAIN_INTERP_TIME = 0.5f;
static const float FRAMES_PER_SECOND = 30.0f;

AnimSplineIK::AnimSplineIK(const QString& id, float alpha, bool enabled, float interpDuration,
    const QString& baseJointName,
    const QString& midJointName,
    const QString& tipJointName,
    const QString& basePositionVar,
    const QString& baseRotationVar,
    const QString& midPositionVar,
    const QString& midRotationVar,
    const QString& tipPositionVar,
    const QString& tipRotationVar,
    const QString& alphaVar,
    const QString& enabledVar,
    const std::vector<float> tipTargetFlexCoefficients,
    const std::vector<float> midTargetFlexCoefficients) :
    AnimNode(AnimNode::Type::SplineIK, id),
    _alpha(alpha),
    _enabled(enabled),
    _interpDuration(interpDuration),
    _baseJointName(baseJointName),
    _midJointName(midJointName),
    _tipJointName(tipJointName),
    _basePositionVar(basePositionVar),
    _baseRotationVar(baseRotationVar),
    _midPositionVar(midPositionVar),
    _midRotationVar(midRotationVar),
    _tipPositionVar(tipPositionVar),
    _tipRotationVar(tipRotationVar),
    _alphaVar(alphaVar),
    _enabledVar(enabledVar)
{

    for (int i = 0; i < (int)tipTargetFlexCoefficients.size(); i++) {
        if (i < MAX_NUMBER_FLEX_VARIABLES) {
            _tipTargetFlexCoefficients[i] = tipTargetFlexCoefficients[i];
        }
     }
    _numTipTargetFlexCoefficients = std::min((int)tipTargetFlexCoefficients.size(), MAX_NUMBER_FLEX_VARIABLES);

    for (int i = 0; i < (int)midTargetFlexCoefficients.size(); i++) {
        if (i < MAX_NUMBER_FLEX_VARIABLES) {
            _midTargetFlexCoefficients[i] = midTargetFlexCoefficients[i];
        }
    }
    _numMidTargetFlexCoefficients = std::min((int)midTargetFlexCoefficients.size(), MAX_NUMBER_FLEX_VARIABLES);

}

AnimSplineIK::~AnimSplineIK() {

}

const AnimPoseVec& AnimSplineIK::evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, AnimVariantMap& triggersOut) {
    assert(_children.size() == 1);
    if (_children.size() != 1) {
        return _poses;
    }

    const float MIN_ALPHA = 0.0f;
    const float MAX_ALPHA = 1.0f;
    float alpha = glm::clamp(animVars.lookup(_alphaVar, _alpha), MIN_ALPHA, MAX_ALPHA);

    // evaluate underPoses
    AnimPoseVec underPoses = _children[0]->evaluate(animVars, context, dt, triggersOut);

    // if we don't have a skeleton, or jointName lookup failed or the spline alpha is 0 or there are no underposes.
    if (!_skeleton || _baseJointIndex == -1 || _midJointIndex == -1 || _tipJointIndex == -1 || alpha == 0.0f || underPoses.size() == 0) {
        // pass underPoses through unmodified.
        _poses = underPoses;
        return _poses;
    }

    // guard against size change
    if (underPoses.size() != _poses.size()) {
        _poses = underPoses;
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

    // now that we have saved the previous _poses in _snapshotChain, we can update to the current underposes
    _poses = underPoses;

    // don't build chains or do IK if we are disabled & not interping.
    if (_interpType == InterpType::None && !enabled) {
        return _poses;
    }

    // compute under chain for possible interpolation
    AnimChain underChain;
    underChain.buildFromRelativePoses(_skeleton, underPoses, _tipJointIndex);

    AnimPose baseTargetAbsolutePose;
    // if there is a baseJoint ik target in animvars then set the joint to that
    // otherwise use the underpose
    AnimPose baseJointUnderPose = _skeleton->getAbsolutePose(_baseJointIndex, _poses);
    baseTargetAbsolutePose.rot() = animVars.lookupRigToGeometry(_baseRotationVar, baseJointUnderPose.rot());
    baseTargetAbsolutePose.trans() = animVars.lookupRigToGeometry(_basePositionVar, baseJointUnderPose.trans());

    int baseParentIndex = _skeleton->getParentIndex(_baseJointIndex);
    AnimPose baseParentAbsPose(Quaternions::IDENTITY,glm::vec3());
    if (baseParentIndex >= 0) {
        baseParentAbsPose = _skeleton->getAbsolutePose(baseParentIndex, _poses);
    }
    _poses[_baseJointIndex] = baseParentAbsPose.inverse() * baseTargetAbsolutePose;
    _poses[_baseJointIndex].scale() = glm::vec3(1.0f);

    // initialize the middle joint target
    IKTarget midTarget;
    midTarget.setType((int)IKTarget::Type::Spline);
    midTarget.setIndex(_midJointIndex);
    AnimPose absPoseMid = _skeleton->getAbsolutePose(_midJointIndex, _poses);
    glm::quat midTargetRotation = animVars.lookupRigToGeometry(_midRotationVar, absPoseMid.rot());
    glm::vec3 midTargetPosition = animVars.lookupRigToGeometry(_midPositionVar, absPoseMid.trans());
    midTarget.setPose(midTargetRotation, midTargetPosition);
    midTarget.setWeight(1.0f);
    midTarget.setFlexCoefficients(_numMidTargetFlexCoefficients, _midTargetFlexCoefficients);

    // solve the lower spine spline
    AnimChain midJointChain;
    AnimPoseVec absolutePosesAfterBaseTipSpline;
    absolutePosesAfterBaseTipSpline.resize(_poses.size());
    computeAbsolutePoses(absolutePosesAfterBaseTipSpline);
    midJointChain.buildFromRelativePoses(_skeleton, _poses, midTarget.getIndex());
    solveTargetWithSpline(context, _baseJointIndex, midTarget, absolutePosesAfterBaseTipSpline, context.getEnableDebugDrawIKChains(), midJointChain);
    midJointChain.outputRelativePoses(_poses);

    // initialize the tip target
    IKTarget tipTarget;
    tipTarget.setType((int)IKTarget::Type::Spline);
    tipTarget.setIndex(_tipJointIndex);
    AnimPose absPoseTip = _skeleton->getAbsolutePose(_tipJointIndex, _poses);
    glm::quat tipRotation = animVars.lookupRigToGeometry(_tipRotationVar, absPoseTip.rot());
    glm::vec3 tipTranslation = animVars.lookupRigToGeometry(_tipPositionVar, absPoseTip.trans());
    tipTarget.setPose(tipRotation, tipTranslation);
    tipTarget.setWeight(1.0f);
    tipTarget.setFlexCoefficients(_numTipTargetFlexCoefficients, _tipTargetFlexCoefficients);

    // solve the upper spine spline
    AnimChain upperJointChain;
    AnimPoseVec finalAbsolutePoses;
    finalAbsolutePoses.resize(_poses.size());
    computeAbsolutePoses(finalAbsolutePoses);
    upperJointChain.buildFromRelativePoses(_skeleton, _poses, tipTarget.getIndex());
    solveTargetWithSpline(context, _midJointIndex, tipTarget, finalAbsolutePoses, context.getEnableDebugDrawIKChains(), upperJointChain);
    upperJointChain.buildDirtyAbsolutePoses();
    upperJointChain.outputRelativePoses(_poses);

    // compute chain
    AnimChain ikChain;
    ikChain.buildFromRelativePoses(_skeleton, _poses, _tipJointIndex);
    // blend with the underChain
    ikChain.blend(underChain, alpha);

    // apply smooth interpolation when turning ik on and off
    if (_interpType != InterpType::None) {
        _interpAlpha += _interpAlphaVel * dt;

        // ease in expo
        float easeInAlpha = 1.0f - powf(2.0f, -10.0f * _interpAlpha);

        if (_interpAlpha < 1.0f) {
            AnimChain interpChain;
            if (_interpType == InterpType::SnapshotToUnderPoses) {
                interpChain = underChain;
                interpChain.blend(_snapshotChain, easeInAlpha);
            } else if (_interpType == InterpType::SnapshotToSolve) {
                interpChain = ikChain;
                interpChain.blend(_snapshotChain, easeInAlpha);
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

    // debug render ik targets
    if (context.getEnableDebugDrawIKTargets()) {
        const vec4 WHITE(1.0f);
        const vec4 GREEN(0.0f, 1.0f, 0.0f, 1.0f);
        glm::mat4 rigToAvatarMat = createMatFromQuatAndPos(Quaternions::Y_180, glm::vec3());

        glm::mat4 geomTargetMat = createMatFromQuatAndPos(tipTarget.getRotation(), tipTarget.getTranslation());
        glm::mat4 avatarTargetMat = rigToAvatarMat * context.getGeometryToRigMatrix() * geomTargetMat;
        QString name = QString("ikTargetSplineTip");
        DebugDraw::getInstance().addMyAvatarMarker(name, glmExtractRotation(avatarTargetMat), extractTranslation(avatarTargetMat), WHITE);

        glm::mat4 geomTargetMat2 = createMatFromQuatAndPos(midTarget.getRotation(), midTarget.getTranslation());
        glm::mat4 avatarTargetMat2 = rigToAvatarMat * context.getGeometryToRigMatrix() * geomTargetMat2;
        QString name2 = QString("ikTargetSplineMid");
        DebugDraw::getInstance().addMyAvatarMarker(name2, glmExtractRotation(avatarTargetMat2), extractTranslation(avatarTargetMat2), WHITE);

        glm::mat4 geomTargetMat3 = createMatFromQuatAndPos(baseTargetAbsolutePose.rot(), baseTargetAbsolutePose.trans());
        glm::mat4 avatarTargetMat3 = rigToAvatarMat * context.getGeometryToRigMatrix() * geomTargetMat3;
        QString name3 = QString("ikTargetSplineBase");
        DebugDraw::getInstance().addMyAvatarMarker(name3, glmExtractRotation(avatarTargetMat3), extractTranslation(avatarTargetMat3), WHITE);


    } else if (context.getEnableDebugDrawIKTargets() != _previousEnableDebugIKTargets) {

        // remove markers if they were added last frame.
        QString name = QString("ikTargetSplineTip");
        DebugDraw::getInstance().removeMyAvatarMarker(name);
        QString name2 = QString("ikTargetSplineMid");
        DebugDraw::getInstance().removeMyAvatarMarker(name2);
        QString name3 = QString("ikTargetSplineBase");
        DebugDraw::getInstance().removeMyAvatarMarker(name3);
    }
    _previousEnableDebugIKTargets = context.getEnableDebugDrawIKTargets();

    return _poses;
}

void AnimSplineIK::lookUpIndices() {
    assert(_skeleton);

    // look up bone indices by name
    std::vector<int> indices = _skeleton->lookUpJointIndices({ _baseJointName, _tipJointName, _midJointName });

    // cache the results
    _baseJointIndex = indices[0];
    _tipJointIndex = indices[1];
    _midJointIndex = indices[2];
}

void AnimSplineIK::computeAbsolutePoses(AnimPoseVec& absolutePoses) const {
    int numJoints = (int)_poses.size();
    assert(numJoints <= _skeleton->getNumJoints());
    assert(numJoints == (int)absolutePoses.size());
    for (int i = 0; i < numJoints; ++i) {
        int parentIndex = _skeleton->getParentIndex(i);
        if (parentIndex < 0) {
            absolutePoses[i] = _poses[i];
        } else {
            absolutePoses[i] = absolutePoses[parentIndex] * _poses[i];
        }
    }
}

// for AnimDebugDraw rendering
const AnimPoseVec& AnimSplineIK::getPosesInternal() const {
    return _poses;
}

void AnimSplineIK::setSkeletonInternal(AnimSkeleton::ConstPointer skeleton) {
    AnimNode::setSkeletonInternal(skeleton);
    lookUpIndices();
}

void AnimSplineIK::solveTargetWithSpline(const AnimContext& context, int base, const IKTarget& target, const AnimPoseVec& absolutePoses, bool debug, AnimChain& chainInfoOut) const {

    // build spline from tip to base
    AnimPose tipPose = AnimPose(glm::vec3(1.0f), target.getRotation(), target.getTranslation());
    AnimPose basePose = absolutePoses[base];

    CubicHermiteSplineFunctorWithArcLength spline;
    if (target.getIndex() == _tipJointIndex) {
        // set gain factors so that more curvature occurs near the tip of the spline.
        const float HIPS_GAIN = 0.5f;
        const float HEAD_GAIN = 1.0f;
        spline = CubicHermiteSplineFunctorWithArcLength(tipPose.rot(), tipPose.trans(), basePose.rot(), basePose.trans(), HIPS_GAIN, HEAD_GAIN);
    } else {
        spline = CubicHermiteSplineFunctorWithArcLength(tipPose.rot(),tipPose.trans(), basePose.rot(), basePose.trans());
    }
    float totalArcLength = spline.arcLength(1.0f);

    // This prevents the rotation interpolation from rotating the wrong physical way (but correct mathematical way)
    // when the head is arched backwards very far.
    glm::quat halfRot = safeLerp(basePose.rot(), tipPose.rot(), 0.5f);
    if (glm::dot(halfRot * Vectors::UNIT_Z, basePose.rot() * Vectors::UNIT_Z) < 0.0f) {
        tipPose.rot() = -tipPose.rot();
    }

    // find or create splineJointInfo for this target
    const std::vector<SplineJointInfo>* splineJointInfoVec = findOrCreateSplineJointInfo(context, base, target);

    if (splineJointInfoVec && splineJointInfoVec->size() > 0) {
        const int baseParentIndex = _skeleton->getParentIndex(base);
        AnimPose parentAbsPose = (baseParentIndex >= 0) ? absolutePoses[baseParentIndex] : AnimPose();
        // go thru splineJointInfoVec backwards (base to tip)
        for (int i = (int)splineJointInfoVec->size() - 1; i >= 0; i--) {
            const SplineJointInfo& splineJointInfo = (*splineJointInfoVec)[i];
            float t = spline.arcLengthInverse(splineJointInfo.ratio * totalArcLength);
            glm::vec3 trans = spline(t);

            // for base->tip splines, preform most twist toward the tip by using ease in function. t^2
            float rotT = t;
            if (target.getIndex() == _tipJointIndex) {
                rotT = t * t;
            }
            glm::quat twistRot = safeLerp(basePose.rot(), tipPose.rot(), rotT);

            // compute the rotation by using the derivative of the spline as the y-axis, and the twistRot x-axis
            glm::vec3 y = glm::normalize(spline.d(t));
            glm::vec3 x = twistRot * Vectors::UNIT_X;
            glm::vec3 u, v, w;
            generateBasisVectors(y, x, v, u, w);
            glm::mat3 m(u, v, glm::cross(u, v));
            glm::quat rot = glm::normalize(glm::quat_cast(m));

            AnimPose desiredAbsPose = AnimPose(glm::vec3(1.0f), rot, trans) * splineJointInfo.offsetPose;

            // apply flex coefficent
            AnimPose flexedAbsPose;
            // get the number of flex coeff for this spline
            float interpedCoefficient = 1.0f;
            int numFlexCoeff = target.getNumFlexCoefficients();
            if (numFlexCoeff == (int)splineJointInfoVec->size()) {
                // then do nothing special
                interpedCoefficient = target.getFlexCoefficient(i);
            } else {
                // interp based on ratio of the joint.
                if (splineJointInfo.ratio < 1.0f) {
                    float flexInterp = splineJointInfo.ratio * (float)(numFlexCoeff - 1);
                    int startCoeff = (int)glm::floor(flexInterp);
                    float partial = flexInterp - startCoeff;
                    interpedCoefficient = target.getFlexCoefficient(startCoeff) * (1.0f - partial) + target.getFlexCoefficient(startCoeff + 1) * partial;
                } else {
                    interpedCoefficient = target.getFlexCoefficient(numFlexCoeff - 1);
                }
            }
            ::blend(1, &absolutePoses[splineJointInfo.jointIndex], &desiredAbsPose, interpedCoefficient, &flexedAbsPose);

            AnimPose relPose = parentAbsPose.inverse() * flexedAbsPose;

            bool constrained = false;
            if (splineJointInfo.jointIndex != base) {
                // constrain the amount the spine can stretch or compress
                float length = glm::length(relPose.trans());
                const float EPSILON = 0.0001f;
                if (length > EPSILON) {
                    float defaultLength = glm::length(_skeleton->getRelativeDefaultPose(splineJointInfo.jointIndex).trans());
                    const float STRETCH_COMPRESS_PERCENTAGE = 0.15f;
                    const float MAX_LENGTH = defaultLength * (1.0f + STRETCH_COMPRESS_PERCENTAGE);
                    const float MIN_LENGTH = defaultLength * (1.0f - STRETCH_COMPRESS_PERCENTAGE);
                    if (length > MAX_LENGTH) {
                        relPose.trans() = (relPose.trans() / length) * MAX_LENGTH;
                        constrained = true;
                    } else if (length < MIN_LENGTH) {
                        relPose.trans() = (relPose.trans() / length) * MIN_LENGTH;
                        constrained = true;
                    }
                } else {
                    relPose.trans() = glm::vec3(0.0f);
                }
            }

            if (!chainInfoOut.setRelativePoseAtJointIndex(splineJointInfo.jointIndex, relPose)) {
                qCDebug(animation) << "error: joint not found in spline chain";
            }

            parentAbsPose = flexedAbsPose;
        }
    }

    if (debug) {
        const vec4 CYAN(0.0f, 1.0f, 1.0f, 1.0f);
        chainInfoOut.debugDraw(context.getRigToWorldMatrix() * context.getGeometryToRigMatrix(), CYAN);
    }
}

const std::vector<AnimSplineIK::SplineJointInfo>* AnimSplineIK::findOrCreateSplineJointInfo(const AnimContext& context, int base, const IKTarget& target) const {
    // find or create splineJointInfo for this target
    auto iter = _splineJointInfoMap.find(target.getIndex());
    if (iter != _splineJointInfoMap.end()) {
        return &(iter->second);
    } else {
        computeAndCacheSplineJointInfosForIKTarget(context, base, target);
        auto iter = _splineJointInfoMap.find(target.getIndex());
        if (iter != _splineJointInfoMap.end()) {
            return &(iter->second);
        }
    }
    return nullptr;
}

// pre-compute information about each joint influenced by this spline IK target.
void AnimSplineIK::computeAndCacheSplineJointInfosForIKTarget(const AnimContext& context, int base, const IKTarget& target) const {
    std::vector<SplineJointInfo> splineJointInfoVec;

    // build spline between the default poses.
    AnimPose tipPose = _skeleton->getAbsoluteDefaultPose(target.getIndex());
    AnimPose basePose = _skeleton->getAbsoluteDefaultPose(base);

    CubicHermiteSplineFunctorWithArcLength spline;
    if (target.getIndex() == _tipJointIndex) {
        // set gain factors so that more curvature occurs near the tip of the spline.
        const float HIPS_GAIN = 0.5f;
        const float HEAD_GAIN = 1.0f;
        spline = CubicHermiteSplineFunctorWithArcLength(tipPose.rot(), tipPose.trans(), basePose.rot(), basePose.trans(), HIPS_GAIN, HEAD_GAIN);
    } else {
        spline = CubicHermiteSplineFunctorWithArcLength(tipPose.rot(), tipPose.trans(), basePose.rot(), basePose.trans());
    }
    // measure the total arc length along the spline
    float totalArcLength = spline.arcLength(1.0f);

    glm::vec3 baseToTip = tipPose.trans() - basePose.trans();
    float baseToTipLength = glm::length(baseToTip);
    glm::vec3 baseToTipNormal = baseToTip / baseToTipLength;

    int index = target.getIndex();
    int endIndex = _skeleton->getParentIndex(base);

    while (index != endIndex) {
        AnimPose defaultPose = _skeleton->getAbsoluteDefaultPose(index);
        glm::vec3 baseToCurrentJoint = defaultPose.trans() - basePose.trans();
        float ratio = glm::dot(baseToCurrentJoint, baseToTipNormal) / baseToTipLength;

        // compute offset from spline to the default pose.
        float t = spline.arcLengthInverse(ratio * totalArcLength);

        // compute the rotation by using the derivative of the spline as the y-axis, and the defaultPose x-axis
        glm::vec3 y = glm::normalize(spline.d(t));
        glm::vec3 x = defaultPose.rot() * Vectors::UNIT_X;
        glm::vec3 u, v, w;
        generateBasisVectors(y, x, v, u, w);
        glm::mat3 m(u, v, glm::cross(u, v));
        glm::quat rot = glm::normalize(glm::quat_cast(m));

        AnimPose pose(glm::vec3(1.0f), rot, spline(t));
        AnimPose offsetPose = pose.inverse() * defaultPose;

        SplineJointInfo splineJointInfo = { index, ratio, offsetPose };
        splineJointInfoVec.push_back(splineJointInfo);
        index = _skeleton->getParentIndex(index);
    }
    _splineJointInfoMap[target.getIndex()] = splineJointInfoVec;
}

void AnimSplineIK::beginInterp(InterpType interpType, const AnimChain& chain) {
    // capture the current poses in a snapshot.
    _snapshotChain = chain;

    _interpType = interpType;
    _interpAlphaVel = FRAMES_PER_SECOND / _interpDuration;
    _interpAlpha = 0.0f;
}