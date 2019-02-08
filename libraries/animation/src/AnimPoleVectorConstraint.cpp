//
//  AnimPoleVectorConstraint.cpp
//
//  Created by Anthony J. Thibault on 5/12/18.
//  Copyright (c) 2018 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimPoleVectorConstraint.h"
#include "AnimationLogging.h"
#include "AnimUtil.h"
#include "GLMHelpers.h"

const float FRAMES_PER_SECOND = 30.0f;
const float INTERP_DURATION = 6.0f;

AnimPoleVectorConstraint::AnimPoleVectorConstraint(const QString& id, bool enabled, glm::vec3 referenceVector,
                                                   const QString& baseJointName, const QString& midJointName, const QString& tipJointName,
                                                   const QString& enabledVar, const QString& poleVectorVar) :
    AnimNode(AnimNode::Type::PoleVectorConstraint, id),
    _enabled(enabled),
    _referenceVector(referenceVector),
    _baseJointName(baseJointName),
    _midJointName(midJointName),
    _tipJointName(tipJointName),
    _enabledVar(enabledVar),
    _poleVectorVar(poleVectorVar) {

}

AnimPoleVectorConstraint::~AnimPoleVectorConstraint() {

}

static float correctElbowForHandRotation(const AnimPose& hand, const AnimPose& lowerArm) {

    // first calculate the ulnar/radial deviation
    // use the lower arm x-axis and the hand x-axis
    glm::vec3 xAxisLowerArm = lowerArm.rot() * glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 yAxisLowerArm = lowerArm.rot() * glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 zAxisLowerArm = lowerArm.rot() * glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 xAxisHand = hand.rot() * glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 yAxisHand = hand.rot() * glm::vec3(0.0f, 1.0f, 0.0f);

    float ulnarRadialDeviation = atan2(glm::dot(xAxisHand, xAxisLowerArm), glm::dot(xAxisHand, yAxisLowerArm));
    float flexionExtension = atan2(glm::dot(yAxisHand, zAxisLowerArm), glm::dot(yAxisHand, yAxisLowerArm));

    qCDebug(animation) << "flexion angle " << flexionExtension;


    float deltaInDegrees = (flexionExtension / PI) * 180.0f;

    qCDebug(animation) << "delta in degrees " << deltaInDegrees;

    float deltaFinal = glm::sign(deltaInDegrees) * powf(fabsf(deltaInDegrees/180.0f), 1.5f) * 180.0f * -0.3f;
    return deltaFinal;
}

float AnimPoleVectorConstraint::findThetaNewWay(const glm::vec3& hand, const glm::vec3& shoulder, bool left) const {
    // get the default poses for the upper and lower arm
    // then use this length to judge how far the arm is away from the shoulder.
    // then create weights that make the elbow angle less when the x value is large in either direction.
    // make the angle less when z is small.  
    // lower y with x center lower angle
    // lower y with x out higher angle
    AnimPose shoulderPose = _skeleton->getAbsoluteDefaultPose(_skeleton->nameToJointIndex("RightShoulder"));
    AnimPose handPose = _skeleton->getAbsoluteDefaultPose(_skeleton->nameToJointIndex("RightHand"));
    // subtract 10 centimeters from the arm length for some reason actual arm position is clamped to length - 10cm.
    float defaultArmLength = glm::length( handPose.trans() - shoulderPose.trans() ) - 10.0f;
    // qCDebug(animation) << "default arm length " << defaultArmLength;

    // phi_0 is the lowest angle we can have
    const float phi_0 = 15.0f;
    const float zStart = 0.6f;
    const float xStart = 0.1f;
    // biases
    //const glm::vec3 biases(30.0f, 120.0f, -30.0f);
    const glm::vec3 biases(0.0f, 135.0f, 0.0f);
    // weights
    const float zWeightBottom = -100.0f;
    //const glm::vec3 weights(-30.0f, 30.0f, 210.0f);
    const glm::vec3 weights(-50.0f, -60.0f, 260.0f);
    glm::vec3 armToHand = hand - shoulder;
    // qCDebug(animation) << "current arm length " << glm::length(armToHand);
    float initial_valuesY = ((armToHand[1] / defaultArmLength) * weights[1]) + biases[1];
    float initial_valuesZ;
    if (armToHand[1] > 0.0f) {
        initial_valuesZ = weights[2] * glm::max(zStart - (armToHand[2] / defaultArmLength), 0.0f) * fabs(armToHand[1] / defaultArmLength);
    } else {
        initial_valuesZ = zWeightBottom * glm::max(zStart - (armToHand[2] / defaultArmLength), 0.0f) * fabs(armToHand[1] / defaultArmLength);
    }

    float initial_valuesX;
    if (left) {
        initial_valuesX = weights[0] * glm::max(-1.0f * (armToHand[0] / defaultArmLength) + xStart, 0.0f);
    } else {
        initial_valuesX = weights[0] * glm::max((armToHand[0] / defaultArmLength) + xStart, 0.0f);
    }

    float theta = initial_valuesX + initial_valuesY + initial_valuesZ;

    if (theta < 13.0f) {
        theta = 13.0f;
    }
    if (theta > 175.0f) {
        theta = 175.0f;
    }

    // qCDebug(animation) << "relative hand" << initial_valuesX << " " << initial_valuesY << " " << initial_valuesZ << "theta value for pole vector is " << theta;
    return theta;
}

const AnimPoseVec& AnimPoleVectorConstraint::evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, AnimVariantMap& triggersOut) {

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

    // Look up poleVector from animVars, make sure to convert into geom space.
    glm::vec3 poleVector = animVars.lookupRigToGeometryVector(_poleVectorVar, Vectors::UNIT_Z);

    // determine if we should interpolate
    bool enabled = animVars.lookup(_enabledVar, _enabled);

    const float MIN_LENGTH = 1.0e-4f;
    if (glm::length(poleVector) < MIN_LENGTH) {
        enabled = false;
    }

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

    // Look up refVector from animVars, make sure to convert into geom space.
    glm::vec3 refVector = midPose.xformVectorFast(_referenceVector);
    float refVectorLength = glm::length(refVector);

    glm::vec3 axis = basePose.trans() - tipPose.trans();
    float axisLength = glm::length(axis);
    glm::vec3 unitAxis = axis / axisLength;

    glm::vec3 sideVector = glm::cross(unitAxis, refVector);
    float sideVectorLength = glm::length(sideVector);

    // project refVector onto axis plane
    glm::vec3 refVectorProj = refVector - glm::dot(refVector, unitAxis) * unitAxis;
    float refVectorProjLength = glm::length(refVectorProj);

    // project poleVector on plane formed by axis.
    glm::vec3 poleVectorProj = poleVector - glm::dot(poleVector, unitAxis) * unitAxis;
    float poleVectorProjLength = glm::length(poleVectorProj);

    // double check for zero length vectors or vectors parallel to rotaiton axis.
    if (axisLength > MIN_LENGTH && refVectorLength > MIN_LENGTH && sideVectorLength > MIN_LENGTH &&
        refVectorProjLength > MIN_LENGTH && poleVectorProjLength > MIN_LENGTH) {

        float dot = glm::clamp(glm::dot(refVectorProj / refVectorProjLength, poleVectorProj / poleVectorProjLength), 0.0f, 1.0f);
        float sideDot = glm::dot(poleVector, sideVector);
        float theta = copysignf(1.0f, sideDot) * acosf(dot);

        float fred;
        if ((_skeleton->nameToJointIndex("RightHand") == _tipJointIndex) || (_skeleton->nameToJointIndex("LeftHand") == _tipJointIndex)) {
            //qCDebug(animation) << "theta from the old code " << theta;
            bool isLeft = false;
            if (_skeleton->nameToJointIndex("LeftHand") == _tipJointIndex) {
                isLeft = true;
            }
            fred = findThetaNewWay(tipPose.trans(), basePose.trans(), isLeft);
            if (isLeft){
                fred *= -1.0f;
            }
            theta = ((180.0f - fred) / 180.0f)*PI;

            // here is where we would do the wrist correction.
            float deltaTheta = correctElbowForHandRotation(tipPose, midPose);
            qCDebug(animation) << "the wrist correction theta is -----> " << deltaTheta;

        }

        

        //glm::quat deltaRot = glm::angleAxis(theta, unitAxis);
        glm::quat deltaRot = glm::angleAxis(theta, unitAxis);

        // transform result back into parent relative frame.
        glm::quat relBaseRot = glm::inverse(baseParentPose.rot()) * deltaRot * basePose.rot();
        ikChain.setRelativePoseAtJointIndex(_baseJointIndex, AnimPose(relBaseRot, underPoses[_baseJointIndex].trans()));

        glm::quat relTipRot = glm::inverse(midPose.rot()) * glm::inverse(deltaRot) * tipPose.rot();
        ikChain.setRelativePoseAtJointIndex(_tipJointIndex, AnimPose(relTipRot, underPoses[_tipJointIndex].trans()));
    }

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

    if (context.getEnableDebugDrawIKChains()) {
        if (_interpType == InterpType::None && enabled) {
            const vec4 BLUE(0.0f, 0.0f, 1.0f, 1.0f);
            ikChain.debugDraw(context.getRigToWorldMatrix() * context.getGeometryToRigMatrix(), BLUE);
        }
    }

    if (context.getEnableDebugDrawIKChains()) {
        if (enabled) {
            const glm::vec4 RED(1.0f, 0.0f, 0.0f, 1.0f);
            const glm::vec4 GREEN(0.0f, 1.0f, 0.0f, 1.0f);
            const glm::vec4 CYAN(0.0f, 1.0f, 1.0f, 1.0f);
            const glm::vec4 YELLOW(1.0f, 0.0f, 1.0f, 1.0f);
            const float VECTOR_LENGTH = 0.5f;

            glm::mat4 geomToWorld = context.getRigToWorldMatrix() * context.getGeometryToRigMatrix();

            // draw the pole
            glm::vec3 start = transformPoint(geomToWorld, basePose.trans());
            glm::vec3 end = transformPoint(geomToWorld, tipPose.trans());
            DebugDraw::getInstance().drawRay(start, end, CYAN);

            // draw the poleVector
            glm::vec3 midPoint = 0.5f * (start + end);
            glm::vec3 poleVectorEnd = midPoint + VECTOR_LENGTH * glm::normalize(transformVectorFast(geomToWorld, poleVector));
            DebugDraw::getInstance().drawRay(midPoint, poleVectorEnd, GREEN);

            // draw the refVector
            glm::vec3 refVectorEnd = midPoint + VECTOR_LENGTH * glm::normalize(transformVectorFast(geomToWorld, refVector));
            DebugDraw::getInstance().drawRay(midPoint, refVectorEnd, RED);

            // draw the sideVector
            glm::vec3 sideVector = glm::cross(poleVector, refVector);
            glm::vec3 sideVectorEnd = midPoint + VECTOR_LENGTH * glm::normalize(transformVectorFast(geomToWorld, sideVector));
            DebugDraw::getInstance().drawRay(midPoint, sideVectorEnd, YELLOW);
        }
    }

    processOutputJoints(triggersOut);

    return _poses;
}



// for AnimDebugDraw rendering
const AnimPoseVec& AnimPoleVectorConstraint::getPosesInternal() const {
    return _poses;
}

void AnimPoleVectorConstraint::setSkeletonInternal(AnimSkeleton::ConstPointer skeleton) {
    AnimNode::setSkeletonInternal(skeleton);
    lookUpIndices();
}

void AnimPoleVectorConstraint::lookUpIndices() {
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

void AnimPoleVectorConstraint::beginInterp(InterpType interpType, const AnimChain& chain) {
    // capture the current poses in a snapshot.
    _snapshotChain = chain;

    _interpType = interpType;
    _interpAlphaVel = FRAMES_PER_SECOND / INTERP_DURATION;
    _interpAlpha = 0.0f;
}
