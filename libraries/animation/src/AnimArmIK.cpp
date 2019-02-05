//
//  AnimArmIK.cpp
//
//  Created by Angus Antley on 1/9/19.
//  Copyright (c) 2019 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimArmIK.h"

#include <DebugDraw.h>

#include "AnimationLogging.h"
#include "AnimUtil.h"

AnimArmIK::AnimArmIK(const QString& id, float alpha, bool enabled, float interpDuration,
    const QString& baseJointName, const QString& midJointName,
    const QString& tipJointName, const glm::vec3& midHingeAxis,
    const QString& alphaVar, const QString& enabledVar,
    const QString& endEffectorRotationVarVar, const QString& endEffectorPositionVarVar) :
    AnimTwoBoneIK(id, alpha, enabled, interpDuration, baseJointName, midJointName, tipJointName, midHingeAxis, alphaVar, enabledVar, endEffectorRotationVarVar, endEffectorPositionVarVar) {

}

AnimArmIK::~AnimArmIK() {

}

const AnimPoseVec& AnimArmIK::evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, AnimVariantMap& triggersOut) {
    qCDebug(animation) << "evaluating the arm IK";

    assert(_children.size() == 1);
    if (_children.size() != 1) {
        return _poses;
    } else {
        return _poses;
    }
}