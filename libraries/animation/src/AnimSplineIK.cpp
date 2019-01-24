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
    // evalute underPoses
    AnimPoseVec underPoses = _children[0]->evaluate(animVars, context, dt, triggersOut);
    _poses = underPoses;

    // now we override the hips relative pose based on the hips target that has been set.
    ////////////////////////////////////////////////////
    if (_poses.size() > 0) {
        AnimPose hipsUnderPose = _skeleton->getAbsolutePose(_hipsIndex, _poses);
        glm::quat hipsTargetRotation = animVars.lookupRigToGeometry("hipsRotation", hipsUnderPose.rot());
        glm::vec3 hipsTargetTranslation = animVars.lookupRigToGeometry("hipsPosition", hipsUnderPose.trans());
        AnimPose absHipsTargetPose(hipsTargetRotation, hipsTargetTranslation);

        int hipsParentIndex = _skeleton->getParentIndex(_hipsIndex);
        AnimPose hipsParentAbsPose = _skeleton->getAbsolutePose(hipsParentIndex, _poses);

        _poses[_hipsIndex] = hipsParentAbsPose.inverse() * absHipsTargetPose;
        _poses[_hipsIndex].scale() = glm::vec3(1.0f);
    }
    //////////////////////////////////////////////////////////////////////////////////
    
    AnimPoseVec absolutePoses;
    absolutePoses.resize(_poses.size());
    computeAbsolutePoses(absolutePoses);
   
    IKTarget target;
    if (_headIndex != -1) {
        target.setType(animVars.lookup("headType", (int)IKTarget::Type::RotationAndPosition));
        target.setIndex(_headIndex);
        AnimPose absPose = _skeleton->getAbsolutePose(_headIndex, _poses);
        glm::quat rotation = animVars.lookupRigToGeometry("headRotation", absPose.rot());
        glm::vec3 translation = animVars.lookupRigToGeometry("headPosition", absPose.trans());
        float weight = animVars.lookup("headWeight", "4.0");

        target.setPose(rotation, translation);
        target.setWeight(weight);
        const float* flexCoefficients = new float[5]{ 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
        //const float* flexCoefficients = new float[2]{ 0.0f, 1.0f };
        target.setFlexCoefficients(5, flexCoefficients);
    }

    AnimChain jointChain;
    AnimPose targetSpine2;
    AnimPoseVec absolutePoses2;
    absolutePoses2.resize(_poses.size());
    if (_poses.size() > 0) {
        
        //_snapshotChain.buildFromRelativePoses(_skeleton, _poses, target.getIndex());
        jointChain.buildFromRelativePoses(_skeleton, _poses, target.getIndex());
        solveTargetWithSpline(context, target, absolutePoses, false, jointChain);
        jointChain.outputRelativePoses(_poses);

        AnimPose afterSolveSpine2 = _skeleton->getAbsolutePose(_spine2Index, _poses);
        glm::quat spine2RotationTarget = animVars.lookupRigToGeometry("spine2Rotation", afterSolveSpine2.rot());
        
        targetSpine2 = AnimPose(spine2RotationTarget, afterSolveSpine2.trans());  
    }

    /*
    IKTarget target2;
    computeAbsolutePoses(absolutePoses2);
    if (_spine2Index != -1) {
        target2.setType(animVars.lookup("spine2Type", (int)IKTarget::Type::RotationAndPosition));
        target2.setIndex(_spine2Index);

        float weight2 = animVars.lookup("spine2Weight", "2.0");
       
        target2.setPose(targetSpine2.rot(), targetSpine2.trans());
        target2.setWeight(weight2);
        
        const float* flexCoefficients2 = new float[3]{ 1.0f, 1.0f, 1.0f };
        target2.setFlexCoefficients(3, flexCoefficients2);
    }

    AnimChain jointChain2;
    AnimPose beforeSolveChestNeck;
    if (_poses.size() > 0) {

        
        beforeSolveChestNeck = _skeleton->getAbsolutePose(_skeleton->nameToJointIndex("Neck"), _poses);

        jointChain2.buildFromRelativePoses(_skeleton, _poses, target2.getIndex());
        solveTargetWithSpline(context, target2, absolutePoses2, false, jointChain2);
        jointChain2.outputRelativePoses(_poses);
    }
    */
    
    
    const float FRAMES_PER_SECOND = 30.0f;
    _interpAlphaVel = FRAMES_PER_SECOND / _interpDuration;
    _alpha = _interpAlphaVel * dt;
    // we need to blend the old joint chain with the current joint chain, otherwise known as: _snapShotChain
    // we blend the chain info then we accumulate it then we assign to relative poses then we return the value.
    // make the alpha
    // make the prevchain
    // interp from the previous chain to the new chain/or underposes if the ik is disabled.
    // update the relative poses and then we are done

    // set the tip/head rotation to match the absolute rotation of the target.
    int headParent = _skeleton->getParentIndex(_headIndex);
    int spine2Parent = _skeleton->getParentIndex(_spine2Index);
    if ((spine2Parent != -1) && (headParent != -1) && (_poses.size() > 0)) {
        /*
        AnimPose spine2Target(target2.getRotation(), target2.getTranslation());
        AnimPose finalSpine2RelativePose = _skeleton->getAbsolutePose(spine2Parent, _poses).inverse() * spine2Target;
        _poses[_spine2Index] = finalSpine2RelativePose;

        
        AnimPose neckAbsolute = _skeleton->getAbsolutePose(headParent, _poses);
        AnimPose finalNeckAbsolute = AnimPose(safeLerp(target2.getRotation(), target.getRotation(), 1.0f),neckAbsolute.trans());
        _poses[headParent] = spine2Target.inverse() * beforeSolveChestNeck;
        */
        AnimPose headTarget(target.getRotation(),target.getTranslation());
        AnimPose finalHeadRelativePose = _skeleton->getAbsolutePose(headParent,_poses).inverse() * headTarget;
        _poses[_headIndex] = finalHeadRelativePose;
    }
 
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
    _spine2Index = _skeleton->nameToJointIndex("Spine2");
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
    const int headBaseIndex =  _spine2Index;

    // build spline from tip to base
    AnimPose tipPose = AnimPose(glm::vec3(1.0f), target.getRotation(), target.getTranslation());
    AnimPose basePose;
    //if (target.getIndex() == _headIndex) {
    //    basePose = absolutePoses[headBaseIndex];
    //} else {
        basePose = absolutePoses[baseIndex];
    //}

    CubicHermiteSplineFunctorWithArcLength spline;
    if (target.getIndex() == _headIndex) {
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
    if (target.getIndex() == _headIndex) {
        basePose = _skeleton->getAbsoluteDefaultPose(_hipsIndex);
        //basePose = _skeleton->getAbsoluteDefaultPose(_spine2Index);
    } else {
        basePose = _skeleton->getAbsoluteDefaultPose(_hipsIndex);
    }

    CubicHermiteSplineFunctorWithArcLength spline;
    if (target.getIndex() == _headIndex) {
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
    if (target.getIndex() == _headIndex) {
        endIndex = _skeleton->getParentIndex(_spine2Index);
        // endIndex = _skeleton->getParentIndex(_hipsIndex);
    } else {
        endIndex = _skeleton->getParentIndex(_hipsIndex);
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

void AnimSplineIK ::loadPoses(const AnimPoseVec& poses) {
    assert(_skeleton && ((poses.size() == 0) || (_skeleton->getNumJoints() == (int)poses.size())));
    if (_skeleton->getNumJoints() == (int)poses.size()) {
        _poses = poses;
    } else {
        _poses.clear();
    }
}