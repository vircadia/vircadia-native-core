//
//  AnimClip.cpp
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimClip.h"
#include "AnimationLogging.h"

AnimClip::AnimClip(const std::string& id, const std::string& url, float startFrame, float endFrame, float timeScale, bool loopFlag) :
    AnimNode(AnimNode::ClipType, id),
    _url(url),
    _startFrame(startFrame),
    _endFrame(endFrame),
    _timeScale(timeScale),
    _frame(startFrame),
    _loopFlag(loopFlag)
{

}

AnimClip::~AnimClip() {

}

void AnimClip::setURL(const std::string& url) {
    _networkAnim = DependencyManager::get<AnimationCache>()->getAnimation(QString::fromStdString(url));
    _url = url;
}

void AnimClip::setStartFrame(float startFrame) {
    _startFrame = startFrame;
}

void AnimClip::setEndFrame(float endFrame) {
    _endFrame = endFrame;
}

void AnimClip::setLoopFlag(bool loopFlag) {
    _loopFlag = loopFlag;
}

float AnimClip::accumulateTime(float frame, float dt) const {
    const float startFrame = std::min(_startFrame, _endFrame);
    if (startFrame == _endFrame) {
        // when startFrame >= endFrame
        frame = _endFrame;
    } else if (_timeScale > 0.0f) {
        // accumulate time, keeping track of loops and end of animation events.
        const float FRAMES_PER_SECOND = 30.0f;
        float framesRemaining = (dt * _timeScale) * FRAMES_PER_SECOND;
        while (framesRemaining > 0.0f) {
            float framesTillEnd = _endFrame - _frame;
            if (framesRemaining >= framesTillEnd) {
                if (_loopFlag) {
                    // anim loop
                    // TODO: trigger onLoop event
                    framesRemaining -= framesTillEnd;
                    frame = startFrame;
                } else {
                    // anim end
                    // TODO: trigger onDone event
                    frame = _endFrame;
                    framesRemaining = 0.0f;
                }
            } else {
                frame += framesRemaining;
                framesRemaining = 0.0f;
            }
        }
    }
    return frame;
}

const std::vector<AnimBone>& AnimClip::evaluate(float dt) {
    _frame = accumulateTime(_frame, dt);

    if (!_anim && _networkAnim && _networkAnim->isLoaded() && _skeleton) {
        copyFramesFromNetworkAnim();
        _networkAnim = nullptr;
    }

    if (_anim) {
        int frameCount = _anim.size();

        int prevIndex = (int)glm::floor(_frame);
        int nextIndex = (int)glm::ceil(_frame);
        if (_loopFlag && nextIndex >= frameCount) {
            nextIndex = 0;
        }

        // It can be quite possible for the user to set _startFrame and _endFrame to
        // values before or past valid ranges.  We clamp the frames here.
        prevIndex = std::min(std::max(0, prevIndex), frameCount - 1);
        nextIndex = std::min(std::max(0, nextIndex), frameCount - 1);

        const std::vector<AnimBone>& prevFrame = _anim[prevIndex];
        const std::vector<AnimBone>& nextFrame = _anim[nextIndex];
        float alpha = glm::fract(_frame);

        for (size_t i = 0; i < _bones.size(); i++) {
            const AnimBone& prevBone = prevFrame[i];
            const AnimBone& nextBone = nextFrame[i];
            _bones[i].scale = glm::lerp(prevBone.scale, nextBone.scale, alpha);
            _bones[i].rot = glm::normalize(glm::lerp(prevBone.rot, nextBone.rot, alpha));
            _bones[i].trans = glm::lerp(prevBone.trans, nextBone.trans, alpha);
        }
    }

    return _bones;
}

void AnimClip::copyFromNetworkAnim() {
    assert(_networkAnim && _networkAnim->isLoaded() && _skeleton);
    _anim.clear();

    // build a mapping from animation joint indices to skeleton joint indices.
    // by matching joints with the same name.
    const FBXGeometry& geom = _networkAnim->getGeometry();
    const QVector<FBXJoint>& joints = geom.joints;
    std::vector<int> jointMap;
    const int animJointCount = joints.count();
    jointMap.reserve(animJointCount);
    for (int i = 0; i < animJointCount; i++) {
        int skeletonJoint _skeleton.nameToJointIndex(joints.at(i).name);
        jointMap.push_back(skeletonJoint);
    }

    const int frameCount = geom.animationFrames.size();
    const int skeletonJointCount = _skeleton.jointCount();
    _anim.resize(frameCount);
    for (int i = 0; i < frameCount; i++) {

        // init all joints in animation to bind pose
        _anim[i].reserve(skeletonJointCount);
        for (int j = 0; j < skeletonJointCount; j++) {
            _anim[i].push_back(_skeleton.getBindPose(j));
        }

        // init over all joint animations
        for (int j = 0; j < animJointCount; j++) {
            int k = jointMap[j];
            if (k >= 0 && k < skeletonJointCount) {
                // currently animations only have rotation.
                _anim[i][k].rot = geom.animationFrames[i].rotations[j];
            }
        }
    }
}
