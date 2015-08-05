//
//  AnimClip.cpp
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLMHelpers.h"
#include "AnimClip.h"
#include "AnimationLogging.h"

AnimClip::AnimClip(const std::string& id, const std::string& url, float startFrame, float endFrame, float timeScale, bool loopFlag) :
    AnimNode(AnimNode::ClipType, id),
    _startFrame(startFrame),
    _endFrame(endFrame),
    _timeScale(timeScale),
    _loopFlag(loopFlag),
    _frame(startFrame)
{
    setURL(url);
}

AnimClip::~AnimClip() {

}

void AnimClip::setURL(const std::string& url) {
    auto animCache = DependencyManager::get<AnimationCache>();
    _networkAnim = animCache->getAnimation(QString::fromStdString(url));
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

const std::vector<AnimPose>& AnimClip::evaluate(float dt) {
    _frame = accumulateTime(_frame, dt);

    if (_networkAnim && _networkAnim->isLoaded() && _skeleton) {
        copyFromNetworkAnim();
        _networkAnim.reset();
    }

    if (_anim.size()) {
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

        const std::vector<AnimPose>& prevFrame = _anim[prevIndex];
        const std::vector<AnimPose>& nextFrame = _anim[nextIndex];
        float alpha = glm::fract(_frame);

        for (size_t i = 0; i < _poses.size(); i++) {
            const AnimPose& prevPose = prevFrame[i];
            const AnimPose& nextPose = nextFrame[i];
            _poses[i].scale = lerp(prevPose.scale, nextPose.scale, alpha);
            _poses[i].rot = glm::normalize(glm::lerp(prevPose.rot, nextPose.rot, alpha));
            _poses[i].trans = lerp(prevPose.trans, nextPose.trans, alpha);
        }
    }

    return _poses;
}

void AnimClip::copyFromNetworkAnim() {
    assert(_networkAnim && _networkAnim->isLoaded() && _skeleton);
    _anim.clear();

    // build a mapping from animation joint indices to skeleton joint indices.
    // by matching joints with the same name.
    const FBXGeometry& geom = _networkAnim->getGeometry();
    const QVector<FBXJoint>& animJoints = geom.joints;
    std::vector<int> jointMap;
    const int animJointCount = animJoints.count();
    jointMap.reserve(animJointCount);
    for (int i = 0; i < animJointCount; i++) {
        int skeletonJoint = _skeleton->nameToJointIndex(animJoints.at(i).name);
        if (skeletonJoint == -1) {
            qCWarning(animation) << "animation contains joint =" << animJoints.at(i).name << " which is not in the skeleton, url =" << _url.c_str();
        }
        jointMap.push_back(skeletonJoint);
    }

    const int frameCount = geom.animationFrames.size();
    const int skeletonJointCount = _skeleton->getNumJoints();
    _anim.resize(frameCount);
    for (int i = 0; i < frameCount; i++) {

        // init all joints in animation to bind pose
        _anim[i].reserve(skeletonJointCount);
        for (int j = 0; j < skeletonJointCount; j++) {
            _anim[i].push_back(_skeleton->getRelativeBindPose(j));
        }

        // init over all joint animations
        for (int j = 0; j < animJointCount; j++) {
            int k = jointMap[j];
            if (k >= 0 && k < skeletonJointCount) {
                // currently FBX animations only have rotation.
                _anim[i][k].rot = _skeleton->getRelativeBindPose(j).rot * geom.animationFrames[i].rotations[j];
            }
        }
    }
    _poses.resize(skeletonJointCount);
}


const std::vector<AnimPose>& AnimClip::getPosesInternal() const {
    return _poses;
}
