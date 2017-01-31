//
//  AnimClip.cpp
//
//  Created by Anthony J. Thibault on 9/2/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLMHelpers.h"
#include "AnimClip.h"
#include "AnimationLogging.h"
#include "AnimUtil.h"

bool AnimClip::usePreAndPostPoseFromAnim = true;

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

const AnimPoseVec& AnimClip::evaluate(const AnimVariantMap& animVars, float dt, Triggers& triggersOut) {

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
    Triggers triggers;
    _frame = ::accumulateTime(_startFrame, _endFrame, _timeScale, frame + _startFrame, dt, _loopFlag, _id, triggers);
}

void AnimClip::copyFromNetworkAnim() {
    assert(_networkAnim && _networkAnim->isLoaded() && _skeleton);
    _anim.clear();

    // build a mapping from animation joint indices to skeleton joint indices.
    // by matching joints with the same name.
    const FBXGeometry& geom = _networkAnim->getGeometry();
    AnimSkeleton animSkeleton(geom);
    const auto animJointCount = animSkeleton.getNumJoints();
    const auto skeletonJointCount = _skeleton->getNumJoints();
    std::vector<int> jointMap;
    jointMap.reserve(animJointCount);
    for (int i = 0; i < animJointCount; i++) {
        int skeletonJoint = _skeleton->nameToJointIndex(animSkeleton.getJointName(i));
        if (skeletonJoint == -1) {
            qCWarning(animation) << "animation contains joint =" << animSkeleton.getJointName(i) << " which is not in the skeleton, url =" << _url;
        }
        jointMap.push_back(skeletonJoint);
    }

    const int frameCount = geom.animationFrames.size();
    _anim.resize(frameCount);

    for (int frame = 0; frame < frameCount; frame++) {

        const FBXAnimationFrame& fbxAnimFrame = geom.animationFrames[frame];

        // init all joints in animation to default pose
        // this will give us a resonable result for bones in the model skeleton but not in the animation.
        _anim[frame].reserve(skeletonJointCount);
        for (int skeletonJoint = 0; skeletonJoint < skeletonJointCount; skeletonJoint++) {
            _anim[frame].push_back(_skeleton->getRelativeDefaultPose(skeletonJoint));
        }

        for (int animJoint = 0; animJoint < animJointCount; animJoint++) {
            int skeletonJoint = jointMap[animJoint];

            const glm::vec3& fbxAnimTrans = fbxAnimFrame.translations[animJoint];
            const glm::quat& fbxAnimRot = fbxAnimFrame.rotations[animJoint];

            // skip joints that are in the animation but not in the skeleton.
            if (skeletonJoint >= 0 && skeletonJoint < skeletonJointCount) {

                AnimPose preRot, postRot;
                if (usePreAndPostPoseFromAnim) {
                    preRot = animSkeleton.getPreRotationPose(animJoint);
                    postRot = animSkeleton.getPostRotationPose(animJoint);
                } else {
                    // In order to support Blender, which does not have preRotation FBX support, we use the models defaultPose as the reference frame for the animations.
                    preRot = AnimPose(glm::vec3(1.0f), _skeleton->getRelativeBindPose(skeletonJoint).rot(), glm::vec3());
                    postRot = AnimPose::identity;
                }

                // cancel out scale
                preRot.scale() = glm::vec3(1.0f);
                postRot.scale() = glm::vec3(1.0f);

                AnimPose rot(glm::vec3(1.0f), fbxAnimRot, glm::vec3());

                // adjust translation offsets, so large translation animatons on the reference skeleton
                // will be adjusted when played on a skeleton with short limbs.
                const glm::vec3& fbxZeroTrans = geom.animationFrames[0].translations[animJoint];
                const AnimPose& relDefaultPose = _skeleton->getRelativeDefaultPose(skeletonJoint);
                float boneLengthScale = 1.0f;
                const float EPSILON = 0.0001f;
                if (fabsf(glm::length(fbxZeroTrans)) > EPSILON) {
                    boneLengthScale = glm::length(relDefaultPose.trans()) / glm::length(fbxZeroTrans);
                }

                AnimPose trans = AnimPose(glm::vec3(1.0f), glm::quat(), relDefaultPose.trans() + boneLengthScale * (fbxAnimTrans - fbxZeroTrans));

                _anim[frame][skeletonJoint] = trans * preRot * rot * postRot;
            }
        }
    }

    // mirrorAnim will be re-built on demand, if needed.
    _mirrorAnim.clear();

    _poses.resize(skeletonJointCount);
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
