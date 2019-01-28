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
    const QString& alphaVar, const QString& enabledVar,
    const QString& endEffectorRotationVarVar, const QString& endEffectorPositionVarVar,
    const QString& basePositionVar,
    const QString& baseRotationVar,
    const QString& midPositionVar,
    const QString& midRotationVar,
    const QString& tipPositionVar,
    const QString& tipRotationVar,
    const QString& primaryFlexCoefficients,
    const QString& secondaryFlexCoefficients) :
    AnimNode(AnimNode::Type::SplineIK, id),
    _alpha(alpha),
    _enabled(enabled),
    _interpDuration(interpDuration),
    _baseJointName(baseJointName),
    _midJointName(midJointName),
    _tipJointName(tipJointName),
    _alphaVar(alphaVar),
    _enabledVar(enabledVar),
    _endEffectorRotationVarVar(endEffectorRotationVarVar),
    _endEffectorPositionVarVar(endEffectorPositionVarVar),
    _prevEndEffectorRotationVar(),
    _prevEndEffectorPositionVar(),
    _basePositionVar(basePositionVar),
    _baseRotationVar(baseRotationVar),
    _midPositionVar(midPositionVar),
    _midRotationVar(midRotationVar),
    _tipPositionVar(tipPositionVar),
    _tipRotationVar(tipRotationVar)
{

    QStringList flexCoefficientsValues = primaryFlexCoefficients.split(',', QString::SkipEmptyParts);
    for (int i = 0; i < flexCoefficientsValues.size(); i++) {
        if (i < MAX_NUMBER_FLEX_VARIABLES) {
            qCDebug(animation) << "flex value " << flexCoefficientsValues[i].toDouble();
            _primaryFlexCoefficients[i] = (float)flexCoefficientsValues[i].toDouble();
        }
    }
    _numPrimaryFlexCoefficients = std::min(flexCoefficientsValues.size(), MAX_NUMBER_FLEX_VARIABLES);
    QStringList secondaryFlexCoefficientsValues = secondaryFlexCoefficients.split(',', QString::SkipEmptyParts);
    for (int i = 0; i < secondaryFlexCoefficientsValues.size(); i++) {
        if (i < MAX_NUMBER_FLEX_VARIABLES) {
            qCDebug(animation) << "secondaryflex value " << secondaryFlexCoefficientsValues[i].toDouble();
            _secondaryFlexCoefficients[i] = (float)secondaryFlexCoefficientsValues[i].toDouble();
        }
    }
    _numSecondaryFlexCoefficients = std::min(secondaryFlexCoefficientsValues.size(), MAX_NUMBER_FLEX_VARIABLES);

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

    // guard against size changes
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

    glm::quat hipsTargetRotation;
    glm::vec3 hipsTargetTranslation;

    // now we override the hips with the hips target pose.
    if (_poses.size() > 0) {
        AnimPose hipsUnderPose = _skeleton->getAbsolutePose(_hipsIndex, _poses);
        hipsTargetRotation = animVars.lookupRigToGeometry("hipsRotation", hipsUnderPose.rot());
        hipsTargetTranslation = animVars.lookupRigToGeometry("hipsPosition", hipsUnderPose.trans());
        AnimPose absHipsTargetPose(hipsTargetRotation, hipsTargetTranslation);

        int hipsParentIndex = _skeleton->getParentIndex(_hipsIndex);
        AnimPose hipsParentAbsPose = _skeleton->getAbsolutePose(hipsParentIndex, _poses);

        _poses[_hipsIndex] = hipsParentAbsPose.inverse() * absHipsTargetPose;
        _poses[_hipsIndex].scale() = glm::vec3(1.0f);
    }

    AnimPoseVec absolutePoses;
    absolutePoses.resize(_poses.size());
    computeAbsolutePoses(absolutePoses);

    IKTarget target;
    if (_tipJointIndex != -1) {
        target.setType((int)IKTarget::Type::Spline);
        target.setIndex(_tipJointIndex);
        AnimPose absPose = _skeleton->getAbsolutePose(_tipJointIndex, _poses);
        glm::quat rotation = animVars.lookupRigToGeometry(_tipRotationVar, absPose.rot());
        glm::vec3 translation = animVars.lookupRigToGeometry(_tipPositionVar, absPose.trans());
        float weight = 1.0f;

        target.setPose(rotation, translation);
        target.setWeight(weight);



        const float* flexCoefficients = new float[5]{ 1.0f, 0.5f, 0.25f, 0.2f, 0.1f };
        target.setFlexCoefficients(_numPrimaryFlexCoefficients, _primaryFlexCoefficients);
    }

    AnimChain jointChain;
    AnimPose updatedSecondaryTarget;
    AnimPoseVec absolutePoses2;
    absolutePoses2.resize(_poses.size());
    if (_poses.size() > 0) {

        jointChain.buildFromRelativePoses(_skeleton, _poses, target.getIndex());
        solveTargetWithSpline(context, target, absolutePoses, false, jointChain);
        jointChain.buildDirtyAbsolutePoses();
        qCDebug(animation) << "the jointChain Result for head " << jointChain.getAbsolutePoseFromJointIndex(_tipJointIndex);
        qCDebug(animation) << "the orig target pose for  head " << target.getPose();
        jointChain.outputRelativePoses(_poses);

        AnimPose afterSolveSecondaryTarget = _skeleton->getAbsolutePose(_midJointIndex, _poses);
        glm::quat secondaryTargetRotation = animVars.lookupRigToGeometry(_midRotationVar, afterSolveSecondaryTarget.rot());
        updatedSecondaryTarget = AnimPose(secondaryTargetRotation, afterSolveSecondaryTarget.trans());
        //updatedSecondaryTarget = AnimPose(afterSolveSecondaryTarget.rot(), afterSolveSecondaryTarget.trans());
    }

    IKTarget secondaryTarget;
    computeAbsolutePoses(absolutePoses2);
    if (_midJointIndex != -1) {
        secondaryTarget.setType((int)IKTarget::Type::Spline);
        secondaryTarget.setIndex(_midJointIndex);

        float weight2 = 1.0f;
        secondaryTarget.setPose(updatedSecondaryTarget.rot(), updatedSecondaryTarget.trans());
        secondaryTarget.setWeight(weight2);

        const float* flexCoefficients2 = new float[3]{ 1.0f, 0.5f, 0.25f };
        secondaryTarget.setFlexCoefficients(_numSecondaryFlexCoefficients, _secondaryFlexCoefficients);
    }

    AnimChain secondaryJointChain;
    AnimPose beforeSolveChestNeck;
    int startJoint;
    AnimPose correctJoint;
    if (_poses.size() > 0) {

        // start at the tip
        
        for (startJoint = _tipJointIndex; _skeleton->getParentIndex(startJoint) != _midJointIndex; startJoint = _skeleton->getParentIndex(startJoint)) {
            // find the child of the midJoint
        }
        correctJoint = _skeleton->getAbsolutePose(startJoint, _poses);

        // fix this to deal with no neck AA
        beforeSolveChestNeck = _skeleton->getAbsolutePose(_skeleton->nameToJointIndex("Neck"), _poses);

        secondaryJointChain.buildFromRelativePoses(_skeleton, _poses, secondaryTarget.getIndex());
        solveTargetWithSpline(context, secondaryTarget, absolutePoses2, false, secondaryJointChain);
        secondaryJointChain.outputRelativePoses(_poses);
    }

    // set the tip/head rotation to match the absolute rotation of the target.
    int tipParent = _skeleton->getParentIndex(_tipJointIndex);
    int secondaryTargetParent = _skeleton->getParentIndex(_midJointIndex);
    if ((secondaryTargetParent != -1) && (tipParent != -1) && (_poses.size() > 0)) {

        

        AnimPose secondaryTargetPose(secondaryTarget.getRotation(), secondaryTarget.getTranslation());
        //AnimPose neckAbsolute = _skeleton->getAbsolutePose(tipParent, _poses);
        //_poses[tipParent] = secondaryTargetPose.inverse() * beforeSolveChestNeck;
        _poses[startJoint] = secondaryTargetPose.inverse() * correctJoint;

        //AnimPose tipTarget(target.getRotation(),target.getTranslation());
        //AnimPose tipRelativePose = _skeleton->getAbsolutePose(tipParent,_poses).inverse() * tipTarget;
        //_poses[_tipJointIndex] = tipRelativePose;
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

        glm::mat4 geomTargetMat = createMatFromQuatAndPos(target.getRotation(), target.getTranslation());
        glm::mat4 avatarTargetMat = rigToAvatarMat * context.getGeometryToRigMatrix() * geomTargetMat;
        QString name = QString("ikTargetHead");
        DebugDraw::getInstance().addMyAvatarMarker(name, glmExtractRotation(avatarTargetMat), extractTranslation(avatarTargetMat), WHITE);

        glm::mat4 geomTargetMat2 = createMatFromQuatAndPos(secondaryTarget.getRotation(), secondaryTarget.getTranslation());
        glm::mat4 avatarTargetMat2 = rigToAvatarMat * context.getGeometryToRigMatrix() * geomTargetMat2;
        QString name2 = QString("ikTargetSpine2");
        DebugDraw::getInstance().addMyAvatarMarker(name2, glmExtractRotation(avatarTargetMat2), extractTranslation(avatarTargetMat2), WHITE);


        glm::mat4 geomTargetMat3 = createMatFromQuatAndPos(hipsTargetRotation, hipsTargetTranslation);
        glm::mat4 avatarTargetMat3 = rigToAvatarMat * context.getGeometryToRigMatrix() * geomTargetMat3;
        QString name3 = QString("ikTargetHips");
        DebugDraw::getInstance().addMyAvatarMarker(name3, glmExtractRotation(avatarTargetMat3), extractTranslation(avatarTargetMat3), WHITE);


    } else if (context.getEnableDebugDrawIKTargets() != _previousEnableDebugIKTargets) {
        // remove markers if they were added last frame.

        QString name = QString("ikTargetHead");
        DebugDraw::getInstance().removeMyAvatarMarker(name);
        QString name2 = QString("ikTargetSpine2");
        DebugDraw::getInstance().removeMyAvatarMarker(name2);
        QString name3 = QString("ikTargetHips");
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
    //_headIndex = _skeleton->nameToJointIndex("Head");
    _hipsIndex = _skeleton->nameToJointIndex("Hips");
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
    AnimPose basePose;
    //if (target.getIndex() == _tipJointIndex) {
    //    basePose = absolutePoses[tipBaseIndex];
    //} else {
        basePose = absolutePoses[baseIndex];
    //}

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

            // for head splines, preform most twist toward the tip by using ease in function. t^2
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
            if (splineJointInfo.jointIndex == _skeleton->nameToJointIndex("Neck")) {
                qCDebug(animation) << "neck is " << relPose;
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
        //endIndex = _skeleton->getParentIndex(_secondaryTargetIndex);
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