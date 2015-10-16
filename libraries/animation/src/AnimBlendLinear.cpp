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

AnimBlendLinear::AnimBlendLinear(const QString& id, float alpha, bool sync) :
    AnimNode(AnimNode::Type::BlendLinear, id),
    _alpha(alpha),
    _sync(sync) {

}

AnimBlendLinear::~AnimBlendLinear() {

}

const AnimPoseVec& AnimBlendLinear::evaluate(const AnimVariantMap& animVars, float dt, Triggers& triggersOut) {

    _alpha = animVars.lookup(_alphaVar, _alpha);

    if (_children.size() == 0) {
        for (auto&& pose : _poses) {
            pose = AnimPose::identity;
        }
    } else if (_children.size() == 1) {
        _poses = _children[0]->evaluate(animVars, dt, triggersOut);
    } else {

        float clampedAlpha = glm::clamp(_alpha, 0.0f, (float)(_children.size() - 1));
        size_t prevPoseIndex = glm::floor(clampedAlpha);
        size_t nextPoseIndex = glm::ceil(clampedAlpha);
        float alpha = glm::fract(clampedAlpha);

        float prevPoseDeltaTime = dt;
        float nextPoseDeltaTime = dt;
        if (_sync) {
            setSyncFrameAndComputeDeltaTime(dt, prevPoseIndex, nextPoseIndex, &prevPoseDeltaTime, &nextPoseDeltaTime, triggersOut);
        }

        evaluateAndBlendChildren(animVars, triggersOut, alpha, prevPoseIndex, nextPoseIndex, prevPoseDeltaTime, nextPoseDeltaTime);
    }
    return _poses;
}

// for AnimDebugDraw rendering
const AnimPoseVec& AnimBlendLinear::getPosesInternal() const {
    return _poses;
}

void AnimBlendLinear::evaluateAndBlendChildren(const AnimVariantMap& animVars, Triggers& triggersOut, float alpha,
                                               size_t prevPoseIndex, size_t nextPoseIndex,
                                               float prevPoseDeltaTime, float nextPoseDeltaTime) {
    if (prevPoseIndex == nextPoseIndex) {
        // this can happen if alpha is on an integer boundary
        _poses = _children[prevPoseIndex]->evaluate(animVars, prevPoseDeltaTime, triggersOut);
    } else {
        // need to eval and blend between two children.
        auto prevPoses = _children[prevPoseIndex]->evaluate(animVars, prevPoseDeltaTime, triggersOut);
        auto nextPoses = _children[nextPoseIndex]->evaluate(animVars, nextPoseDeltaTime, triggersOut);

        if (prevPoses.size() > 0 && prevPoses.size() == nextPoses.size()) {
            _poses.resize(prevPoses.size());

            ::blend(_poses.size(), &prevPoses[0], &nextPoses[0], alpha, &_poses[0]);
        }
    }
}

void AnimBlendLinear::setSyncFrameAndComputeDeltaTime(float dt, size_t prevPoseIndex, size_t nextPoseIndex,
                                                      float* prevPoseDeltaTime, float* nextPoseDeltaTime,
                                                      Triggers& triggersOut) {
    std::vector<float> offsets(_children.size(), 0.0f);
    std::vector<float> timeScales(_children.size(), 1.0f);

    float lengthSum = 0.0f;
    for (size_t i = 0; i < _children.size(); i++) {
        // abort if we find a child that is NOT a clipNode.
        if (_children[i]->getType() != AnimNode::Type::Clip) {
            // TODO: FIXME: make sync this work for other node types.
            *prevPoseDeltaTime = dt;
            *nextPoseDeltaTime = dt;
            return;
        }
        auto clipNode = std::dynamic_pointer_cast<AnimClip>(_children[i]);
        assert(clipNode);
        if (clipNode) {
            lengthSum += clipNode->getEndFrame() - clipNode->getStartFrame();
        }
    }

    float averageLength = lengthSum / (float)_children.size();

    auto prevClipNode = std::dynamic_pointer_cast<AnimClip>(_children[prevPoseIndex]);
    float prevTimeScale = (prevClipNode->getEndFrame() - prevClipNode->getStartFrame()) / averageLength;
    float prevOffset = prevClipNode->getStartFrame();
    prevClipNode->setCurrentFrame(prevOffset + prevTimeScale / _syncFrame);

    auto nextClipNode = std::dynamic_pointer_cast<AnimClip>(_children[nextPoseIndex]);
    float nextTimeScale = (nextClipNode->getEndFrame() - nextClipNode->getStartFrame()) / averageLength;
    float nextOffset = nextClipNode->getStartFrame();
    nextClipNode->setCurrentFrame(nextOffset + nextTimeScale / _syncFrame);

    const bool LOOP_FLAG = true;
    _syncFrame = ::accumulateTime(0.0f, averageLength, _timeScale, _syncFrame, dt, LOOP_FLAG, _id, triggersOut);

    *prevPoseDeltaTime = prevTimeScale;
    *nextPoseDeltaTime = nextTimeScale;
}

