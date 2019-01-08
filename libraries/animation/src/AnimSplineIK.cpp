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
    qCDebug(animation) << "evaluating the spline function";
    assert(_children.size() == 1);
    if (_children.size() != 1) {
        return _poses;
    }

    // evalute underPoses
    AnimPoseVec underPoses = _children[0]->evaluate(animVars, context, dt, triggersOut);
    _poses = underPoses;
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

// for AnimDebugDraw rendering
const AnimPoseVec& AnimSplineIK::getPosesInternal() const {
    return _poses;
}

void AnimSplineIK::setSkeletonInternal(AnimSkeleton::ConstPointer skeleton) {
    AnimNode::setSkeletonInternal(skeleton);
    lookUpIndices();
}