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
    const QString& tipTargetFlexCoefficients,
    const QString& midTargetFlexCoefficients) :
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

    QStringList tipTargetFlexCoefficientsValues = tipTargetFlexCoefficients.split(',', QString::SkipEmptyParts);
    for (int i = 0; i < tipTargetFlexCoefficientsValues.size(); i++) {
        if (i < MAX_NUMBER_FLEX_VARIABLES) {
            qCDebug(animation) << "tip target flex value " << tipTargetFlexCoefficientsValues[i].toDouble();
            _tipTargetFlexCoefficients[i] = (float)tipTargetFlexCoefficientsValues[i].toDouble();
        }
    }
    _numTipTargetFlexCoefficients = std::min(tipTargetFlexCoefficientsValues.size(), MAX_NUMBER_FLEX_VARIABLES);
    QStringList midTargetFlexCoefficientsValues = midTargetFlexCoefficients.split(',', QString::SkipEmptyParts);
    for (int i = 0; i < midTargetFlexCoefficientsValues.size(); i++) {
        if (i < MAX_NUMBER_FLEX_VARIABLES) {
            qCDebug(animation) << "mid target flex value " << midTargetFlexCoefficientsValues[i].toDouble();
            _midTargetFlexCoefficients[i] = (float)midTargetFlexCoefficientsValues[i].toDouble();
        }
    }
    _numMidTargetFlexCoefficients = std::min(midTargetFlexCoefficientsValues.size(), MAX_NUMBER_FLEX_VARIABLES);

}

AnimSplineIK::~AnimSplineIK() {

}

//virtual
const AnimPoseVec& AnimSplineIK::overlay(const AnimVariantMap& animVars, const AnimContext& context, float dt, AnimVariantMap& triggersOut, const AnimPoseVec& underPoses) {
    loadPoses(underPoses);
    return _poses;
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

    // if we don't have a skeleton, or jointName lookup failed.
    if (!_skeleton || _baseJointIndex == -1 || _tipJointIndex == -1 || alpha == 0.0f || underPoses.size() == 0) {
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

    _poses = underPoses;

    // don't build chains or do IK if we are disabled & not interping.
    if (_interpType == InterpType::None && !enabled) {
        return _poses;
    }

    // compute under chain for possible interpolation
    AnimChain underChain;
    underChain.buildFromRelativePoses(_skeleton, underPoses, _tipJointIndex);

    AnimPose baseTargetAbsolutePose;
    // now we override the hips with the hips target pose.
    // if there is a baseJoint ik target in animvars then use that
    // otherwise use the underpose
    if (_poses.size() > 0) {
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
    }

    IKTarget tipTarget;
    if (_tipJointIndex != -1) {
        tipTarget.setType((int)IKTarget::Type::Spline);
        tipTarget.setIndex(_tipJointIndex);

        AnimPose absPose = _skeleton->getAbsolutePose(_tipJointIndex, _poses);
        glm::quat rotation = animVars.lookupRigToGeometry(_tipRotationVar, absPose.rot());
        glm::vec3 translation = animVars.lookupRigToGeometry(_tipPositionVar, absPose.trans());
        tipTarget.setPose(rotation, translation);
        tipTarget.setWeight(1.0f);
        tipTarget.setFlexCoefficients(_numTipTargetFlexCoefficients, _tipTargetFlexCoefficients);
    }

    AnimChain jointChain;
    
    if (_poses.size() > 0) {

        AnimPoseVec absolutePoses;
        absolutePoses.resize(_poses.size());
        computeAbsolutePoses(absolutePoses);
        jointChain.buildFromRelativePoses(_skeleton, _poses, tipTarget.getIndex());
        solveTargetWithSpline(context, tipTarget, absolutePoses, false, jointChain);
        jointChain.buildDirtyAbsolutePoses();
        jointChain.outputRelativePoses(_poses);
    }

    IKTarget midTarget;
    if (_midJointIndex != -1) {
        midTarget.setType((int)IKTarget::Type::Spline);
        midTarget.setIndex(_midJointIndex);

        // set the middle joint to the position that is determined by the base-tip spline.
        AnimPose afterSolveMidTarget = _skeleton->getAbsolutePose(_midJointIndex, _poses);
        glm::quat midTargetRotation = animVars.lookupRigToGeometry(_midRotationVar, afterSolveMidTarget.rot());
        AnimPose updatedMidTarget = AnimPose(midTargetRotation, afterSolveMidTarget.trans());
        midTarget.setPose(updatedMidTarget.rot(), updatedMidTarget.trans());
        midTarget.setWeight(1.0f);
        midTarget.setFlexCoefficients(_numMidTargetFlexCoefficients, _midTargetFlexCoefficients);
    }

    AnimChain midJointChain;
    int childOfMiddleJointIndex = -1;
    AnimPose childOfMiddleJointAbsolutePoseAfterBaseTipSpline;
    if (_poses.size() > 0) {

        childOfMiddleJointIndex = _tipJointIndex;
        // start at the tip
        while (_skeleton->getParentIndex(childOfMiddleJointIndex) != _midJointIndex) {
            childOfMiddleJointIndex = _skeleton->getParentIndex(childOfMiddleJointIndex);
        }
        childOfMiddleJointAbsolutePoseAfterBaseTipSpline = _skeleton->getAbsolutePose(childOfMiddleJointIndex, _poses);

        AnimPoseVec absolutePosesAfterBaseTipSpline;
        absolutePosesAfterBaseTipSpline.resize(_poses.size());
        computeAbsolutePoses(absolutePosesAfterBaseTipSpline);
        midJointChain.buildFromRelativePoses(_skeleton, _poses, midTarget.getIndex());
        solveTargetWithSpline(context, midTarget, absolutePosesAfterBaseTipSpline, false, midJointChain);
        midJointChain.outputRelativePoses(_poses);
    }

    // set the tip/head rotation to match the absolute rotation of the target.
    int tipParent = _skeleton->getParentIndex(_tipJointIndex);
    int midTargetParent = _skeleton->getParentIndex(_midJointIndex);
    if ((midTargetParent != -1) && (tipParent != -1) && (_poses.size() > 0)) {
        AnimPose midTargetPose(midTarget.getRotation(), midTarget.getTranslation());
        _poses[childOfMiddleJointIndex] = midTargetPose.inverse() * childOfMiddleJointAbsolutePoseAfterBaseTipSpline;
    }

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

    if (_baseJointIndex != -1) {
        _baseParentJointIndex = _skeleton->getParentIndex(_baseJointIndex);
    }
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

static CubicHermiteSplineFunctorWithArcLength computeSplineFromTipAndBase(const AnimPose& tipPose, const AnimPose& basePose, float baseGain = 1.0f, float tipGain = 1.0f) {
    float linearDistance = glm::length(basePose.trans() - tipPose.trans());
    glm::vec3 p0 = basePose.trans();
    glm::vec3 m0 = baseGain * linearDistance * (basePose.rot() * Vectors::UNIT_Y);
    glm::vec3 p1 = tipPose.trans();
    glm::vec3 m1 = tipGain * linearDistance * (tipPose.rot() * Vectors::UNIT_Y);

    return CubicHermiteSplineFunctorWithArcLength(p0, m0, p1, m1);
}

void AnimSplineIK::solveTargetWithSpline(const AnimContext& context, const IKTarget& target, const AnimPoseVec& absolutePoses, bool debug, AnimChain& chainInfoOut) const {

    const int baseIndex = _baseJointIndex;
    const int tipBaseIndex =  _midJointIndex;

    // build spline from tip to base
    AnimPose tipPose = AnimPose(glm::vec3(1.0f), target.getRotation(), target.getTranslation());
    AnimPose basePose = absolutePoses[baseIndex];

    CubicHermiteSplineFunctorWithArcLength spline;
    if (target.getIndex() == _tipJointIndex) {
        // set gain factors so that more curvature occurs near the tip of the spline.
        const float HIPS_GAIN = 0.5f;
        const float HEAD_GAIN = 1.0f;
        spline = computeSplineFromTipAndBase(tipPose, basePose, HIPS_GAIN, HEAD_GAIN);
    } else {
        spline = computeSplineFromTipAndBase(tipPose, basePose);
    }
    float totalArcLength = spline.arcLength(1.0f);

    // This prevents the rotation interpolation from rotating the wrong physical way (but correct mathematical way)
    // when the head is arched backwards very far.
    glm::quat halfRot = safeLerp(basePose.rot(), tipPose.rot(), 0.5f);
    if (glm::dot(halfRot * Vectors::UNIT_Z, basePose.rot() * Vectors::UNIT_Z) < 0.0f) {
        tipPose.rot() = -tipPose.rot();
    }
    // find or create splineJointInfo for this target
    const std::vector<SplineJointInfo>* splineJointInfoVec = findOrCreateSplineJointInfo(context, target);

    if (splineJointInfoVec && splineJointInfoVec->size() > 0) {
        const int baseParentIndex = _skeleton->getParentIndex(baseIndex);
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
            ::blend(1, &absolutePoses[splineJointInfo.jointIndex], &desiredAbsPose, target.getFlexCoefficient(i), &flexedAbsPose);

            AnimPose relPose = parentAbsPose.inverse() * flexedAbsPose;

            bool constrained = false;
            if (splineJointInfo.jointIndex != _baseJointIndex) {
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
            // note we are ignoring the constrained info for now.
            if (!chainInfoOut.setRelativePoseAtJointIndex(splineJointInfo.jointIndex, relPose)) {
                qCDebug(animation) << "we didn't find the joint index for the spline!!!!";
            }

            parentAbsPose = flexedAbsPose;
        }
    }

    if (debug) {
        //debugDrawIKChain(jointChainInfoOut, context);
    }
}

const std::vector<AnimSplineIK::SplineJointInfo>* AnimSplineIK::findOrCreateSplineJointInfo(const AnimContext& context, const IKTarget& target) const {
    // find or create splineJointInfo for this target
    auto iter = _splineJointInfoMap.find(target.getIndex());
    if (iter != _splineJointInfoMap.end()) {
        return &(iter->second);
    } else {
        computeAndCacheSplineJointInfosForIKTarget(context, target);
        auto iter = _splineJointInfoMap.find(target.getIndex());
        if (iter != _splineJointInfoMap.end()) {
            return &(iter->second);
        }
    }

    return nullptr;
}

// pre-compute information about each joint influenced by this spline IK target.
void AnimSplineIK::computeAndCacheSplineJointInfosForIKTarget(const AnimContext& context, const IKTarget& target) const {
    std::vector<SplineJointInfo> splineJointInfoVec;

    // build spline between the default poses.
    AnimPose tipPose = _skeleton->getAbsoluteDefaultPose(target.getIndex());
    AnimPose basePose;
    if (target.getIndex() == _tipJointIndex) {
        basePose = _skeleton->getAbsoluteDefaultPose(_baseJointIndex);
        //basePose = _skeleton->getAbsoluteDefaultPose(_secondaryTargetIndex);
    } else {
        basePose = _skeleton->getAbsoluteDefaultPose(_baseJointIndex);
    }

    CubicHermiteSplineFunctorWithArcLength spline;
    if (target.getIndex() == _tipJointIndex) {
        // set gain factors so that more curvature occurs near the tip of the spline.
        const float HIPS_GAIN = 0.5f;
        const float HEAD_GAIN = 1.0f;
        spline = computeSplineFromTipAndBase(tipPose, basePose, HIPS_GAIN, HEAD_GAIN);
        // spline = computeSplineFromTipAndBase(tipPose, basePose);
    } else {
        spline = computeSplineFromTipAndBase(tipPose, basePose);
    }

    // measure the total arc length along the spline
    float totalArcLength = spline.arcLength(1.0f);

    glm::vec3 baseToTip = tipPose.trans() - basePose.trans();
    float baseToTipLength = glm::length(baseToTip);
    glm::vec3 baseToTipNormal = baseToTip / baseToTipLength;

    int index = target.getIndex();
    int endIndex;
    if (target.getIndex() == _tipJointIndex) {
        endIndex = _skeleton->getParentIndex(_baseJointIndex);
    } else {
        endIndex = _skeleton->getParentIndex(_baseJointIndex);
    }
    while (index != endIndex) {
        AnimPose defaultPose = _skeleton->getAbsoluteDefaultPose(index);

        float ratio = glm::dot(defaultPose.trans() - basePose.trans(), baseToTipNormal) / baseToTipLength;

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

void AnimSplineIK::loadPoses(const AnimPoseVec& poses) {
    assert(_skeleton && ((poses.size() == 0) || (_skeleton->getNumJoints() == (int)poses.size())));
    if (_skeleton->getNumJoints() == (int)poses.size()) {
        _poses = poses;
    } else {
        _poses.clear();
    }
}

void AnimSplineIK::beginInterp(InterpType interpType, const AnimChain& chain) {
    // capture the current poses in a snapshot.
    _snapshotChain = chain;

    _interpType = interpType;
    _interpAlphaVel = FRAMES_PER_SECOND / _interpDuration;
    _interpAlpha = 0.0f;
}