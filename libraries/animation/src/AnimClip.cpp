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
    // TODO:
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

const AnimPose& AnimClip::evaluate(float dt) {
    const float startFrame = std::min(_startFrame, _endFrame);
    if (startFrame == _endFrame) {
        // when startFrame >= endFrame
        _frame = _endFrame;
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
                    _frame = startFrame;
                } else {
                    // anim end
                    // TODO: trigger onDone event
                    _frame = _endFrame;
                    framesRemaining = 0.0f;
                }
            } else {
                _frame += framesRemaining;
                framesRemaining = 0.0f;
            }
        }
    }
    // TODO: eval animation

    return _frame;
}
