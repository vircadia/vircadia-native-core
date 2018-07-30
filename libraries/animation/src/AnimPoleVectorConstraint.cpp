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
    float poleVectorLength = glm::length(poleVector);

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
