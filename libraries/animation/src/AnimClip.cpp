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

AnimClip::AnimClip(const QString& id, const QString& url, float startFrame, float endFrame, float timeScale, bool loopFlag) :
    AnimNode(AnimNode::Type::Clip, id),
    _startFrame(startFrame),
    _endFrame(endFrame),
    _timeScale(timeScale),
    _loopFlag(loopFlag),
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
    float frame = animVars.lookup(_frameVar, _frame);

    _frame = ::accumulateTime(_startFrame, _endFrame, _timeScale, frame, dt, _loopFlag, _id, triggersOut);

    // poll network anim to see if it's finished loading yet.
    if (_networkAnim && _networkAnim->isLoaded() && _skeleton) {
        // loading is complete, copy animation frames from network animation, then throw it away.
        copyFromNetworkAnim();
        _networkAnim.reset();
    }

    if (_anim.size()) {
        int prevIndex = (int)glm::floor(_frame);
        int nextIndex;
        if (_loopFlag && _frame >= _endFrame) {
            nextIndex = (int)glm::ceil(_startFrame);
        } else {
            nextIndex = (int)glm::ceil(_frame);
        }

        // It can be quite possible for the user to set _startFrame and _endFrame to
        // values before or past valid ranges.  We clamp the frames here.
        int frameCount = _anim.size();
        prevIndex = std::min(std::max(0, prevIndex), frameCount - 1);
        nextIndex = std::min(std::max(0, nextIndex), frameCount - 1);

        const AnimPoseVec& prevFrame = _anim[prevIndex];
        const AnimPoseVec& nextFrame = _anim[nextIndex];
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
    const int animJointCount = animSkeleton.getNumJoints();
    const int skeletonJointCount = _skeleton->getNumJoints();
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

        // init all joints in animation to default pose
        // this will give us a resonable result for bones in the model skeleton but not in the animation.
        _anim[frame].reserve(skeletonJointCount);
        for (int skeletonJoint = 0; skeletonJoint < skeletonJointCount; skeletonJoint++) {
            _anim[frame].push_back(_skeleton->getRelativeDefaultPose(skeletonJoint));
        }

        for (int animJoint = 0; animJoint < animJointCount; animJoint++) {
            int skeletonJoint = jointMap[animJoint];

            // skip joints that are in the animation but not in the skeleton.
            if (skeletonJoint >= 0 && skeletonJoint < skeletonJointCount) {

                const glm::vec3& fbxZeroTrans = geom.animationFrames[0].translations[animJoint];
#ifdef USE_PRE_ROT_FROM_ANIM
                // TODO: This is the correct way to apply the pre rotations from maya, however
                // the current animation set has incorrect preRotations for the left wrist and thumb
                // so it looks wrong if we enable this code.
                glm::quat preRotation = animSkeleton.getPreRotation(animJoint);
#else
                // TODO: This is the legacy approach, this does not work when animations and models do not
                // have the same set of pre rotations.  For example when mixing maya models with blender animations.
                glm::quat preRotation = _skeleton->getRelativeBindPose(skeletonJoint).rot;
#endif
                const AnimPose& relDefaultPose = _skeleton->getRelativeDefaultPose(skeletonJoint);

                // used to adjust translation offsets, so large translation animatons on the reference skeleton
                // will be adjusted when played on a skeleton with short limbs.
                float limbLengthScale = fabsf(glm::length(fbxZeroTrans)) <= 0.0001f ? 1.0f : (glm::length(relDefaultPose.trans) / glm::length(fbxZeroTrans));

                AnimPose& pose = _anim[frame][skeletonJoint];
                const FBXAnimationFrame& fbxAnimFrame = geom.animationFrames[frame];

                // rotation in fbxAnimationFrame is a delta from its preRotation.
                pose.rot = preRotation * fbxAnimFrame.rotations[animJoint];

                // translation in fbxAnimationFrame is not a delta.
                // convert it into a delta by subtracting from the first frame.
                const glm::vec3& fbxTrans = fbxAnimFrame.translations[animJoint];
                pose.trans = relDefaultPose.trans + limbLengthScale * (fbxTrans - fbxZeroTrans);
            }
        }
    }

    _poses.resize(skeletonJointCount);
}


const AnimPoseVec& AnimClip::getPosesInternal() const {
    return _poses;
}
