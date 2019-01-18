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

AnimSplineIK::AnimSplineIK(const QString& id, float alpha, bool enabled, float interpDuration,
    const QString& baseJointName,
    const QString& tipJointName,
    const QString& alphaVar, const QString& enabledVar,
    const QString& endEffectorRotationVarVar, const QString& endEffectorPositionVarVar) :
    AnimNode(AnimNode::Type::SplineIK, id),
    _alpha(alpha),
    _enabled(enabled),
    _interpDuration(interpDuration),
    _baseJointName(baseJointName),
    _tipJointName(tipJointName),
    _alphaVar(alphaVar),
    _enabledVar(enabledVar),
    _endEffectorRotationVarVar(endEffectorRotationVarVar),
    _endEffectorPositionVarVar(endEffectorPositionVarVar),
    _prevEndEffectorRotationVar(),
    _prevEndEffectorPositionVar() {

}

AnimSplineIK::~AnimSplineIK() {

}

const AnimPoseVec& AnimSplineIK::evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, AnimVariantMap& triggersOut) {
    assert(_children.size() == 1);
    if (_children.size() != 1) {
        return _poses;
    }
    // evalute underPoses
    AnimPoseVec underPoses = _children[0]->evaluate(animVars, context, dt, triggersOut);
    _poses = underPoses;
    
    // check to see if we actually need absolute poses.
    AnimPoseVec absolutePoses;
    absolutePoses.resize(_poses.size());
    computeAbsolutePoses(absolutePoses);
    AnimPoseVec absolutePoses2;
    absolutePoses2.resize(_poses.size());
    // do this later 
    // computeAbsolutePoses(absolutePoses2);

    int jointIndex2 = _skeleton->nameToJointIndex("Spine2");
    AnimPose origSpine2PoseAbs = _skeleton->getAbsolutePose(jointIndex2, _poses);
    if ((jointIndex2 != -1) && (_poses.size() > 0)) {
        AnimPose origSpine2 = _skeleton->getAbsolutePose(jointIndex2, _poses);
        AnimPose origSpine1 = _skeleton->getAbsolutePose(_skeleton->nameToJointIndex("Spine1"),_poses);
        //origSpine2PoseRel = origSpine1.inverse() * origSpine2;
        //qCDebug(animation) << "origSpine2Pose: " << origSpine2Pose.rot();
        qCDebug(animation) << "original relative spine2  " << origSpine2PoseAbs;
    }
    
    IKTarget target;
    int jointIndex = _skeleton->nameToJointIndex("Head");
    if (jointIndex != -1) {
        target.setType(animVars.lookup("headType", (int)IKTarget::Type::RotationAndPosition));
        target.setIndex(jointIndex);
        AnimPose absPose = _skeleton->getAbsolutePose(jointIndex, _poses);
        glm::quat rotation = animVars.lookupRigToGeometry("headRotation", absPose.rot());
        glm::vec3 translation = animVars.lookupRigToGeometry("headPosition", absPose.trans());
        float weight = animVars.lookup("headWeight", "4.0");

        //qCDebug(animation) << "target 1 rotation absolute" << rotation;
        target.setPose(rotation, translation);
        target.setWeight(weight);
        const float* flexCoefficients = new float[5]{ 1.0f, 0.5f, 0.25f, 0.2f, 0.1f };
        target.setFlexCoefficients(5, flexCoefficients);

        // record the index of the hips ik target.
        if (target.getIndex() == _hipsIndex) {
            _hipsTargetIndex = 1;
        }
    }

    AnimPose origAbsAfterHeadSpline;
    AnimPose finalSpine2;
    AnimChain jointChain;
    AnimPose targetSpine2;
    if (_poses.size() > 0) {
        
        _snapshotChain.buildFromRelativePoses(_skeleton, _poses, target.getIndex());
        jointChain.buildFromRelativePoses(_skeleton, _poses, target.getIndex());
        // jointChain2.buildFromRelativePoses(_skeleton, _poses, target2.getIndex());

        // for each target solve target with spline
        //qCDebug(animation) << "before head spline";
        //jointChain.dump();
        solveTargetWithSpline(context, target, absolutePoses, false, jointChain);
        AnimPose afterSolveSpine2 = jointChain.getAbsolutePoseFromJointIndex(jointIndex2);
        AnimPose afterSolveSpine1 = jointChain.getAbsolutePoseFromJointIndex(_skeleton->nameToJointIndex("Spine1"));
        AnimPose afterSolveSpine2Rel = afterSolveSpine1.inverse() * afterSolveSpine2;
        AnimPose originalSpine2Relative = afterSolveSpine1.inverse() * origSpine2PoseAbs;
        glm::quat rotation3 = animVars.lookupRigToGeometry("spine2Rotation", afterSolveSpine2.rot());
        glm::vec3 translation3 = animVars.lookupRigToGeometry("spine2Position", afterSolveSpine2.trans());
        targetSpine2 = AnimPose(rotation3, afterSolveSpine2.trans());
        finalSpine2 = afterSolveSpine1.inverse() * targetSpine2;

        qCDebug(animation) << "relative spine2 after solve" << afterSolveSpine2Rel;
        qCDebug(animation) << "relative spine2 orig" << originalSpine2Relative;
        AnimPose latestSpine2Relative(originalSpine2Relative.rot(), afterSolveSpine2Rel.trans());
        //jointChain.setRelativePoseAtJointIndex(jointIndex2, finalSpine2);
        jointChain.outputRelativePoses(_poses);
        //qCDebug(animation) << "after head spline";
        //jointChain.dump();
        
        computeAbsolutePoses(absolutePoses2);
        origAbsAfterHeadSpline = _skeleton->getAbsolutePose(jointIndex2, _poses);
        // qCDebug(animation) << "Spine2 trans after head spline: " << origAbsAfterHeadSpline.trans();
        
    }
    
    IKTarget target2;
    //int jointIndex2 = _skeleton->nameToJointIndex("Spine2");
    if (jointIndex2 != -1) {
        target2.setType(animVars.lookup("spine2Type", (int)IKTarget::Type::RotationAndPosition));
        target2.setIndex(jointIndex2);
        AnimPose absPose2 = _skeleton->getAbsolutePose(jointIndex2, _poses);
        glm::quat rotation2 = animVars.lookupRigToGeometry("spine2Rotation", absPose2.rot());
        glm::vec3 translation2 = animVars.lookupRigToGeometry("spine2Position", absPose2.trans());
        float weight2 = animVars.lookup("spine2Weight", "2.0");
        qCDebug(animation) << "rig to geometry" << rotation2;
        
        //target2.setPose(rotation2, translation2);
        target2.setPose(targetSpine2.rot(), targetSpine2.trans());
        target2.setWeight(weight2);
        const float* flexCoefficients2 = new float[3]{ 1.0f, 0.5f, 0.25f };
        target2.setFlexCoefficients(3, flexCoefficients2);

        
    }
    AnimChain jointChain2;
    if (_poses.size() > 0) {
        
        // _snapshotChain.buildFromRelativePoses(_skeleton, _poses, target.getIndex());
        // jointChain.buildFromRelativePoses(_skeleton, _poses, target.getIndex());
        // jointChain2.buildFromRelativePoses(_skeleton, underPoses, target2.getIndex());
        jointChain2.buildFromRelativePoses(_skeleton, _poses, target2.getIndex());

        // for each target solve target with spline
        solveTargetWithSpline(context, target2, absolutePoses2, false, jointChain2);
        
        //jointChain.outputRelativePoses(_poses);
        jointChain2.outputRelativePoses(_poses);
        //qCDebug(animation) << "Spine2 spline";
        //jointChain2.dump();
        //qCDebug(animation) << "Spine2Pose trans after spine2 spline: " << _skeleton->getAbsolutePose(jointIndex2, _poses).trans();

    }
    
    const float FRAMES_PER_SECOND = 30.0f;
    _interpAlphaVel = FRAMES_PER_SECOND / _interpDuration;
    _alpha = _interpAlphaVel * dt;
    // we need to blend the old joint chain with the current joint chain, otherwise known as: _snapShotChain
    // we blend the chain info then we accumulate it then we assign to relative poses then we return the value.
    // make the alpha
    // make the prevchain
    // interp from the previous chain to the new chain/or underposes if the ik is disabled.
    // update the relative poses and then we are done
    

    /**/
    return _poses;
}

void AnimSplineIK::lookUpIndices() {
    assert(_skeleton);

    // look up bone indices by name
    std::vector<int> indices = _skeleton->lookUpJointIndices({ _baseJointName, _tipJointName });

    // cache the results
    _baseJointIndex = indices[0];
    _tipJointIndex = indices[1];

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
    _headIndex = _skeleton->nameToJointIndex("Head");
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

    const int baseIndex = _hipsIndex;

    // build spline from tip to base
    AnimPose tipPose = AnimPose(glm::vec3(1.0f), target.getRotation(), target.getTranslation());
    AnimPose basePose = absolutePoses[baseIndex];
    CubicHermiteSplineFunctorWithArcLength spline;
    if (target.getIndex() == _headIndex) {
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

            // for head splines, preform most twist toward the tip by using ease in function. t^2
            float rotT = t;
            if (target.getIndex() == _headIndex) {
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
            if (splineJointInfo.jointIndex != _hipsIndex) {
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
    AnimPose basePose = _skeleton->getAbsoluteDefaultPose(_hipsIndex);

    CubicHermiteSplineFunctorWithArcLength spline;
    if (target.getIndex() == _headIndex) {
        // set gain factors so that more curvature occurs near the tip of the spline.
        const float HIPS_GAIN = 0.5f;
        const float HEAD_GAIN = 1.0f;
        spline = computeSplineFromTipAndBase(tipPose, basePose, HIPS_GAIN, HEAD_GAIN);
    } else {
        spline = computeSplineFromTipAndBase(tipPose, basePose);
    }

    // measure the total arc length along the spline
    float totalArcLength = spline.arcLength(1.0f);

    glm::vec3 baseToTip = tipPose.trans() - basePose.trans();
    float baseToTipLength = glm::length(baseToTip);
    glm::vec3 baseToTipNormal = baseToTip / baseToTipLength;

    int index = target.getIndex();
    int endIndex = _skeleton->getParentIndex(_hipsIndex);
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