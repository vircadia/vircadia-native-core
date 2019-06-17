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

static std::vector<int> buildJointIndexMap(const AnimSkeleton& dstSkeleton, const AnimSkeleton& srcSkeleton) {
    std::vector<int> jointIndexMap;
    int srcJointCount = srcSkeleton.getNumJoints();
    jointIndexMap.reserve(srcJointCount);
    for (int srcJointIndex = 0; srcJointIndex < srcJointCount; srcJointIndex++) {
        QString srcJointName = srcSkeleton.getJointName(srcJointIndex);
        int dstJointIndex = dstSkeleton.nameToJointIndex(srcJointName);
        jointIndexMap.push_back(dstJointIndex);
    }
    return jointIndexMap;
}

#ifdef USE_CUSTOM_ASSERT
#undef ASSERT
#define ASSERT(x)                     \
    do {                              \
        if (!(x)) {                   \
            int* bad_ptr = 0;         \
            *bad_ptr = 0x0badf00d;    \
        }                             \
    } while (0)
#endif // #ifdef USE_CUSTOM_ASSERT

void AnimClip::copyFromNetworkAnim() {
    assert(_networkAnim && _networkAnim->isLoaded() && _skeleton);
    _anim.clear();

    auto avatarSkeleton = getSkeleton();
    const HFMModel& animModel = _networkAnim->getHFMModel();
    AnimSkeleton animSkeleton(animModel);
    const int animJointCount = animSkeleton.getNumJoints();
    const int avatarJointCount = avatarSkeleton->getNumJoints();

    // build a mapping from animation joint indices to avatar joint indices by matching joints with the same name.
    std::vector<int> avatarToAnimJointIndexMap = buildJointIndexMap(animSkeleton, *avatarSkeleton);

    const int animFrameCount = animModel.animationFrames.size();
    _anim.resize(animFrameCount);

    // find the size scale factor for translation in the animation.
    float boneLengthScale = 1.0f;
    const int avatarHipsIndex = avatarSkeleton->nameToJointIndex("Hips");
    const int animHipsIndex = animSkeleton.nameToJointIndex("Hips");
    if (avatarHipsIndex != -1 && animHipsIndex != -1) {
        const int avatarHipsParentIndex = avatarSkeleton->getParentIndex(avatarHipsIndex);
        const int animHipsParentIndex = animSkeleton.getParentIndex(animHipsIndex);

        const AnimPose& avatarHipsAbsoluteDefaultPose = avatarSkeleton->getAbsoluteDefaultPose(avatarHipsIndex);
        const AnimPose& animHipsAbsoluteDefaultPose = animSkeleton.getAbsoluteDefaultPose(animHipsIndex);

        // the get the units and the heights for the animation and the avatar
        const float avatarUnitScale = extractScale(avatarSkeleton->getGeometryOffset()).y;
        const float animationUnitScale = extractScale(animModel.offset).y;
        const float avatarHeightInMeters = avatarUnitScale * avatarHipsAbsoluteDefaultPose.trans().y;
        const float animHeightInMeters = animationUnitScale * animHipsAbsoluteDefaultPose.trans().y;

        // get the parent scales for the avatar and the animation
        float avatarHipsParentScale = 1.0f;
        if (avatarHipsParentIndex != -1) {
            const AnimPose& avatarHipsParentAbsoluteDefaultPose = avatarSkeleton->getAbsoluteDefaultPose(avatarHipsParentIndex);
            avatarHipsParentScale = avatarHipsParentAbsoluteDefaultPose.scale().y;
        }
        float animHipsParentScale = 1.0f;
        if (animHipsParentIndex != -1) {
            const AnimPose& animationHipsParentAbsoluteDefaultPose = animSkeleton.getAbsoluteDefaultPose(animHipsParentIndex);
            animHipsParentScale = animationHipsParentAbsoluteDefaultPose.scale().y;
        }

        const float EPSILON = 0.0001f;
        // compute the ratios for the units, the heights in meters, and the parent scales
        if ((fabsf(animHeightInMeters) > EPSILON) && (animationUnitScale > EPSILON) && (animHipsParentScale > EPSILON)) {
            const float avatarToAnimationHeightRatio = avatarHeightInMeters / animHeightInMeters;
            const float unitsRatio = 1.0f / (avatarUnitScale / animationUnitScale);
            const float parentScaleRatio = 1.0f / (avatarHipsParentScale / animHipsParentScale);

            boneLengthScale = avatarToAnimationHeightRatio * unitsRatio * parentScaleRatio;
        }
    }

    for (int frame = 0; frame < animFrameCount; frame++) {
        const HFMAnimationFrame& animFrame = animModel.animationFrames[frame];
        ASSERT(frame >= 0 && frame < (int)animModel.animationFrames.size());

        // extract the full rotations from the animFrame (including pre and post rotations from the animModel).
        std::vector<glm::quat> animRotations;
        animRotations.reserve(animJointCount);
        for (int i = 0; i < animJointCount; i++) {
            ASSERT(i >= 0 && i < (int)animModel.joints.size());
            ASSERT(i >= 0 && i < (int)animFrame.rotations.size());
            animRotations.push_back(animModel.joints[i].preRotation * animFrame.rotations[i] * animModel.joints[i].postRotation);
        }

        // convert rotations into absolute frame
        animSkeleton.convertRelativeRotationsToAbsolute(animRotations);

        // build absolute rotations for the avatar
        std::vector<glm::quat> avatarRotations;
        avatarRotations.reserve(avatarJointCount);
        for (int avatarJointIndex = 0; avatarJointIndex < avatarJointCount; avatarJointIndex++) {
            ASSERT(avatarJointIndex >= 0 && avatarJointIndex < (int)avatarToAnimJointIndexMap.size());
            int animJointIndex = avatarToAnimJointIndexMap[avatarJointIndex];
            if (animJointIndex >= 0) {
                // This joint is in both animation and avatar.
                // Set the absolute rotation directly
                ASSERT(animJointIndex >= 0 && animJointIndex < (int)animRotations.size());
                avatarRotations.push_back(animRotations[animJointIndex]);
            } else {
                // This joint is NOT in the animation at all.
                // Set it so that the default relative rotation remains unchanged.
                glm::quat avatarRelativeDefaultRot = avatarSkeleton->getRelativeDefaultPose(avatarJointIndex).rot();
                glm::quat avatarParentAbsoluteRot;
                int avatarParentJointIndex = avatarSkeleton->getParentIndex(avatarJointIndex);
                if (avatarParentJointIndex >= 0) {
                    ASSERT(avatarParentJointIndex >= 0 && avatarParentJointIndex < (int)avatarRotations.size());
                    avatarParentAbsoluteRot = avatarRotations[avatarParentJointIndex];
                }
                avatarRotations.push_back(avatarParentAbsoluteRot * avatarRelativeDefaultRot);
            }
        }

        // convert avatar rotations into relative frame
        avatarSkeleton->convertAbsoluteRotationsToRelative(avatarRotations);

        ASSERT(frame >= 0 && frame < (int)_anim.size());
        _anim[frame].reserve(avatarJointCount);
        for (int avatarJointIndex = 0; avatarJointIndex < avatarJointCount; avatarJointIndex++) {
            const AnimPose& avatarDefaultPose = avatarSkeleton->getRelativeDefaultPose(avatarJointIndex);

            // copy scale over from avatar default pose
            glm::vec3 relativeScale = avatarDefaultPose.scale();

            glm::vec3 relativeTranslation;
            ASSERT(avatarJointIndex >= 0 && avatarJointIndex < (int)avatarToAnimJointIndexMap.size());
            int animJointIndex = avatarToAnimJointIndexMap[avatarJointIndex];
            if (animJointIndex >= 0) {
                // This joint is in both animation and avatar.
                ASSERT(animJointIndex >= 0 && animJointIndex < (int)animFrame.translations.size());
                const glm::vec3& animTrans = animFrame.translations[animJointIndex];

                // retarget translation from animation to avatar
                ASSERT(animJointIndex >= 0 && animJointIndex < (int)animModel.animationFrames[0].translations.size());
                const glm::vec3& animZeroTrans = animModel.animationFrames[0].translations[animJointIndex];
                relativeTranslation = avatarDefaultPose.trans() + boneLengthScale * (animTrans - animZeroTrans);
            } else {
                // This joint is NOT in the animation at all.
                // preserve the default translation.
                relativeTranslation = avatarDefaultPose.trans();
            }

            // build the final pose
            ASSERT(avatarJointIndex >= 0 && avatarJointIndex < (int)avatarRotations.size());
            _anim[frame].push_back(AnimPose(relativeScale, avatarRotations[avatarJointIndex], relativeTranslation));
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
