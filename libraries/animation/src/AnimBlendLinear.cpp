//
//  AnimBlendLinear.cpp
//
//  Created by Anthony J. Thibault on 9/2/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimBlendLinear.h"
#include "GLMHelpers.h"
#include "AnimationLogging.h"
#include "AnimUtil.h"
#include "AnimClip.h"

AnimBlendLinear::AnimBlendLinear(const QString& id, float alpha) :
    AnimNode(AnimNode::Type::BlendLinear, id),
    _alpha(alpha) {

}

AnimBlendLinear::~AnimBlendLinear() {

}

const AnimPoseVec& AnimBlendLinear::evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, Triggers& triggersOut) {

    _alpha = animVars.lookup(_alphaVar, _alpha);

    if (_children.size() == 0) {
        for (auto&& pose : _poses) {
            pose = AnimPose::identity;
        }
    } else if (_children.size() == 1) {
        _poses = _children[0]->evaluate(animVars, context, dt, triggersOut);
    } else {

        float clampedAlpha = glm::clamp(_alpha, 0.0f, (float)(_children.size() - 1));
        size_t prevPoseIndex = glm::floor(clampedAlpha);
        size_t nextPoseIndex = glm::ceil(clampedAlpha);
        float alpha = glm::fract(clampedAlpha);

        evaluateAndBlendChildren(animVars, context, triggersOut, alpha, prevPoseIndex, nextPoseIndex, dt);
    }
    return _poses;
}

// for AnimDebugDraw rendering
const AnimPoseVec& AnimBlendLinear::getPosesInternal() const {
    return _poses;
}

void AnimBlendLinear::evaluateAndBlendChildren(const AnimVariantMap& animVars, const AnimContext& context, Triggers& triggersOut, float alpha,
                                               size_t prevPoseIndex, size_t nextPoseIndex, float dt) {
    if (prevPoseIndex == nextPoseIndex) {
        // this can happen if alpha is on an integer boundary
        _poses = _children[prevPoseIndex]->evaluate(animVars, context, dt, triggersOut);
    } else {
        // need to eval and blend between two children.
        auto prevPoses = _children[prevPoseIndex]->evaluate(animVars, context, dt, triggersOut);
        auto nextPoses = _children[nextPoseIndex]->evaluate(animVars, context, dt, triggersOut);

        if (prevPoses.size() > 0 && prevPoses.size() == nextPoses.size()) {
            _poses.resize(prevPoses.size());

            ::blend(_poses.size(), &prevPoses[0], &nextPoses[0], alpha, &_poses[0]);
        }
    }
}
