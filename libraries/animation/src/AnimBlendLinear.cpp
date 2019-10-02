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

AnimBlendLinear::AnimBlendLinear(const QString& id, float alpha, AnimBlendType blendType) :
    AnimNode(AnimNode::Type::BlendLinear, id),
    _alpha(alpha),
    _blendType(blendType) {

}

AnimBlendLinear::~AnimBlendLinear() {

}

const AnimPoseVec& AnimBlendLinear::evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, AnimVariantMap& triggersOut) {

    _alpha = animVars.lookup(_alphaVar, _alpha);
    float parentDebugAlpha = context.getDebugAlpha(_id);

    if (_children.size() == 0) {
        for (auto&& pose : _poses) {
            pose = AnimPose::identity;
        }
    } else if (_children.size() == 1) {
        _poses = _children[0]->evaluate(animVars, context, dt, triggersOut);
        context.setDebugAlpha(_children[0]->getID(), parentDebugAlpha, _children[0]->getType());
    } else if (_children.size() == 2 && _blendType != AnimBlendType_Normal) {
        // special case for additive blending
        float alpha = glm::clamp(_alpha, 0.0f, 1.0f);
        const size_t prevPoseIndex = 0;
        const size_t nextPoseIndex = 1;
        evaluateAndBlendChildren(animVars, context, triggersOut, alpha, prevPoseIndex, nextPoseIndex, dt);

        // for animation stack debugging
        float weight2 = alpha;
        float weight1 = 1.0f - weight2;
        context.setDebugAlpha(_children[prevPoseIndex]->getID(), weight1 * parentDebugAlpha, _children[prevPoseIndex]->getType());
        context.setDebugAlpha(_children[nextPoseIndex]->getID(), weight2 * parentDebugAlpha, _children[nextPoseIndex]->getType());

    } else {
        float clampedAlpha = glm::clamp(_alpha, 0.0f, (float)(_children.size() - 1));
        size_t prevPoseIndex = glm::floor(clampedAlpha);
        size_t nextPoseIndex = glm::ceil(clampedAlpha);
        auto alpha = glm::fract(clampedAlpha);
        evaluateAndBlendChildren(animVars, context, triggersOut, alpha, prevPoseIndex, nextPoseIndex, dt);

        // weights are for animation stack debug purposes only.
        float weight1 = 0.0f;
        float weight2 = 0.0f;
        if (prevPoseIndex == nextPoseIndex) {
            weight2 = 1.0f;
            context.setDebugAlpha(_children[nextPoseIndex]->getID(), weight2 * parentDebugAlpha, _children[nextPoseIndex]->getType());
        } else {
            weight2 = alpha;
            weight1 = 1.0f - weight2;
            context.setDebugAlpha(_children[prevPoseIndex]->getID(), weight1 * parentDebugAlpha, _children[prevPoseIndex]->getType());
            context.setDebugAlpha(_children[nextPoseIndex]->getID(), weight2 * parentDebugAlpha, _children[nextPoseIndex]->getType());
        }
    }
    processOutputJoints(triggersOut);

    return _poses;
}

// for AnimDebugDraw rendering
const AnimPoseVec& AnimBlendLinear::getPosesInternal() const {
    return _poses;
}

void AnimBlendLinear::evaluateAndBlendChildren(const AnimVariantMap& animVars, const AnimContext& context, AnimVariantMap& triggersOut, float alpha,
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

            if (_blendType == AnimBlendType_Normal) {
                ::blend(_poses.size(), &prevPoses[0], &nextPoses[0], alpha, &_poses[0]);
            } else if (_blendType == AnimBlendType_AddRelative) {
                ::blendAdd(_poses.size(), &prevPoses[0], &nextPoses[0], alpha, &_poses[0]);
            } else if (_blendType == AnimBlendType_AddAbsolute) {
                // convert prev from relative to absolute
                AnimPoseVec absPrev = prevPoses;
                _skeleton->convertRelativePosesToAbsolute(absPrev);

                // rotate the offset rotations from next into the parent relative frame of each joint.
                AnimPoseVec relOffsetPoses;
                relOffsetPoses.reserve(nextPoses.size());
                for (size_t i = 0; i < nextPoses.size(); ++i) {

                    // copy translation and scale from nextPoses
                    AnimPose pose = nextPoses[i];

                    // convert from a rotation that happens in the absolute space of the joint
                    // into a rotation that happens in the relative space of the joint.
                    pose.rot() = glm::inverse(absPrev[i].rot()) * pose.rot() * absPrev[i].rot();

                    relOffsetPoses.push_back(pose);
                }

                // then blend
                ::blendAdd(_poses.size(), &prevPoses[0], &relOffsetPoses[0], alpha, &_poses[0]);
            }
        }
    }
}
