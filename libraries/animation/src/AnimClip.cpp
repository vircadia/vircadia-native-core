//
//  AnimClip.cpp
//
//  Created by Anthony J. Thibault on 9/2/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimClip.h"

#include "GLMHelpers.h"
#include "AnimationLogging.h"
#include "AnimUtil.h"

AnimClip::AnimClip(const QString& id, const QString& url, float startFrame, float endFrame, float timeScale, bool loopFlag, bool mirrorFlag) :
    AnimNode(AnimNode::Type::Clip, id),
    _startFrame(startFrame),
    _endFrame(endFrame),
    _timeScale(timeScale),
    _loopFlag(loopFlag),
    _mirrorFlag(mirrorFlag),
    _frame(startFrame)
{
    loadURL(url);
}

AnimClip::~AnimClip() {

}

const AnimPoseVec& AnimClip::evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, AnimVariantMap& triggersOut) {

    // lookup parameters from animVars, using current instance variables as defaults.
    _startFrame = animVars.lookup(_startFrameVar, _startFrame);
    _endFrame = animVars.lookup(_endFrameVar, _endFrame);
    _timeScale = animVars.lookup(_timeScaleVar, _timeScale);
    _loopFlag = animVars.lookup(_loopFlagVar, _loopFlag);
    _mirrorFlag = animVars.lookup(_mirrorFlagVar, _mirrorFlag);
    float frame = animVars.lookup(_frameVar, _frame);

    _frame = ::accumulateTime(_startFrame, _endFrame, _timeScale, frame, dt, _loopFlag, _id, triggersOut);

    // poll network anim to see if it's finished loading yet.
    if (_networkAnim && _networkAnim->isLoaded() && _skeleton) {
        // loading is complete, copy animation frames from network animation, then throw it away.
        copyFromNetworkAnim();
        _networkAnim.reset();
    }

    if (_anim.size()) {

        // lazy creation of mirrored animation frames.
        if (_mirrorFlag && _anim.size() != _mirrorAnim.size()) {
            buildMirrorAnim();
        }

        int prevIndex = (int)glm::floor(_frame);
        int nextIndex;
        if (_loopFlag && _frame >= _endFrame) {
            nextIndex = (int)glm::ceil(_startFrame);
        } else {
            nextIndex = (int)glm::ceil(_frame);
        }

        // It can be quite possible for the user to set _startFrame and _endFrame to
        // values before or past valid ranges.  We clamp the frames here.
        int frameCount = (int)_anim.size();
        prevIndex = std::min(std::max(0, prevIndex), frameCount - 1);
        nextIndex = std::min(std::max(0, nextIndex), frameCount - 1);

        const AnimPoseVec& prevFrame = _mirrorFlag ? _mirrorAnim[prevIndex] : _anim[prevIndex];
        const AnimPoseVec& nextFrame = _mirrorFlag ? _mirrorAnim[nextIndex] : _anim[nextIndex];
        float alpha = glm::fract(_frame);

        ::blend(_poses.size(), &prevFrame[0], &nextFrame[0], alpha, &_poses[0]);
    }

    processOutputJoints(triggersOut);

    return _poses;
}

void AnimClip::loadURL(const QString& url) {
    auto animCache = DependencyManager::get<AnimationCache>();
    _networkAnim = animCache->getAnimation(url);
    _url = url;
}

void AnimClip::setCurrentFrameInternal(float frame) {
    // because dt is 0, we should not encounter any triggers
    const float dt = 0.0f;
    AnimVariantMap triggers;
    _frame = ::accumulateTime(_startFrame, _endFrame, _timeScale, frame + _startFrame, dt, _loopFlag, _id, triggers);
}

void AnimClip::copyFromNetworkAnim() {
    assert(_networkAnim && _networkAnim->isLoaded() && _skeleton);
    _anim.clear();

    auto avatarSkeleton = getSkeleton();

    // build a mapping from animation joint indices to avatar joint indices.
    // by matching joints with the same name.
    const HFMModel& animModel = _networkAnim->getHFMModel();
    AnimSkeleton animSkeleton(animModel);
    const int animJointCount = animSkeleton.getNumJoints();
    const int avatarJointCount = avatarSkeleton->getNumJoints();
    std::vector<int> animToAvatarJointIndexMap;
    animToAvatarJointIndexMap.reserve(animJointCount);
    for (int animJointIndex = 0; animJointIndex < animJointCount; animJointIndex++) {
        QString animJointName = animSkeleton.getJointName(animJointIndex);
        int avatarJointIndex = avatarSkeleton->nameToJointIndex(animJointName);
        animToAvatarJointIndexMap.push_back(avatarJointIndex);
    }

    const int animFrameCount = animModel.animationFrames.size();
    _anim.resize(animFrameCount);

    for (int frame = 0; frame < animFrameCount; frame++) {

        const HFMAnimationFrame& animFrame = animModel.animationFrames[frame];

        // init all joints in animation to default pose
        // this will give us a resonable result for bones in the avatar skeleton but not in the animation.
        _anim[frame].reserve(avatarJointCount);
        for (int avatarJointIndex = 0; avatarJointIndex < avatarJointCount; avatarJointIndex++) {
            _anim[frame].push_back(avatarSkeleton->getRelativeDefaultPose(avatarJointIndex));
        }

        for (int animJointIndex = 0; animJointIndex < animJointCount; animJointIndex++) {
            int avatarJointIndex = animToAvatarJointIndexMap[animJointIndex];

            // skip joints that are in the animation but not in the avatar.
            if (avatarJointIndex >= 0 && avatarJointIndex < avatarJointCount) {

                const glm::vec3& animTrans = animFrame.translations[animJointIndex];
                const glm::quat& animRot = animFrame.rotations[animJointIndex];

                const AnimPose& animPreRotPose = animSkeleton.getPreRotationPose(animJointIndex);
                AnimPose animPostRotPose = animSkeleton.getPostRotationPose(animJointIndex);
                AnimPose animRotPose(glm::vec3(1.0f), animRot, glm::vec3());

                // adjust anim scale to equal the scale from the avatar joint.
                // we do not support animated scale.
                const AnimPose& avatarDefaultPose = avatarSkeleton->getRelativeDefaultPose(avatarJointIndex);
                animPostRotPose.scale() = avatarDefaultPose.scale();

                // retarget translation from animation to avatar
                const glm::vec3& animZeroTrans = animModel.animationFrames[0].translations[animJointIndex];
                float boneLengthScale = 1.0f;
                const float EPSILON = 0.0001f;
                if (fabsf(glm::length(animZeroTrans)) > EPSILON) {
                    boneLengthScale = glm::length(avatarDefaultPose.trans()) / glm::length(animZeroTrans);
                }
                AnimPose animTransPose = AnimPose(glm::vec3(1.0f), glm::quat(), avatarDefaultPose.trans() + boneLengthScale * (animTrans - animZeroTrans));

                _anim[frame][avatarJointIndex] = animTransPose * animPreRotPose * animRotPose * animPostRotPose;
            }
        }
    }

    // mirrorAnim will be re-built on demand, if needed.
    _mirrorAnim.clear();

    _poses.resize(avatarJointCount);
}

void AnimClip::buildMirrorAnim() {
    assert(_skeleton);

    _mirrorAnim.clear();
    _mirrorAnim.reserve(_anim.size());
    for (auto& relPoses : _anim) {
        _mirrorAnim.push_back(relPoses);
        _skeleton->mirrorRelativePoses(_mirrorAnim.back());
    }
}

const AnimPoseVec& AnimClip::getPosesInternal() const {
    return _poses;
}
