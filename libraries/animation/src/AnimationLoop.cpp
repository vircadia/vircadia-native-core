//
//  AnimationLoop.cpp
//  libraries/animation
//
//  Created by Brad Hefta-Gaub on 11/12/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimationCache.h"
#include "AnimationLoop.h"

const float AnimationLoop::MAXIMUM_POSSIBLE_FRAME = 100000.0f;

AnimationLoop::AnimationLoop() :
    _fps(30.0f),
    _loop(false),
    _hold(false),
    _startAutomatically(false),
    _firstFrame(0.0f),
    _lastFrame(MAXIMUM_POSSIBLE_FRAME),
    _running(false),
    _frameIndex(0.0f),
    _maxFrameIndexHint(MAXIMUM_POSSIBLE_FRAME)
{
}

AnimationLoop::AnimationLoop(const AnimationDetails& animationDetails) :
    _fps(animationDetails.fps),
    _loop(animationDetails.loop),
    _hold(animationDetails.hold),
    _startAutomatically(animationDetails.startAutomatically),
    _firstFrame(animationDetails.firstFrame),
    _lastFrame(animationDetails.lastFrame),
    _running(animationDetails.running),
    _frameIndex(animationDetails.frameIndex)
{
}

AnimationLoop::AnimationLoop(float fps, bool loop, bool hold, bool startAutomatically, float firstFrame, 
                    float lastFrame, bool running, float frameIndex) :
    _fps(fps),
    _loop(loop),
    _hold(hold),
    _startAutomatically(startAutomatically),
    _firstFrame(firstFrame),
    _lastFrame(lastFrame),
    _running(running),
    _frameIndex(frameIndex)
{
}

void AnimationLoop::simulate(float deltaTime) {
    _frameIndex += deltaTime * _fps;

    // If we knew the number of frames from the animation, we'd consider using it here 
    // animationGeometry.animationFrames.size()
    float maxFrame = _maxFrameIndexHint;
    float endFrameIndex = qMin(_lastFrame, maxFrame - (_loop ? 0.0f : 1.0f));
    float startFrameIndex = qMin(_firstFrame, endFrameIndex);
    if ((!_loop && (_frameIndex < startFrameIndex || _frameIndex > endFrameIndex)) || startFrameIndex == endFrameIndex) {
        // passed the end; apply the last frame
        _frameIndex = glm::clamp(_frameIndex, startFrameIndex, endFrameIndex);
        if (!_hold) {
            stop();
        }
    } else {
        // wrap within the the desired range
        if (_frameIndex < startFrameIndex) {
            _frameIndex = endFrameIndex - glm::mod(endFrameIndex - _frameIndex, endFrameIndex - startFrameIndex);
    
        } else if (_frameIndex > endFrameIndex) {
            _frameIndex = startFrameIndex + glm::mod(_frameIndex - startFrameIndex, endFrameIndex - startFrameIndex);
        }
    }
}

void AnimationLoop::setStartAutomatically(bool startAutomatically) {
    if ((_startAutomatically = startAutomatically) && !isRunning()) {
        start();
    }
}
    
void AnimationLoop::setRunning(bool running) {
    // don't do anything if the new value is the same as the value we already have
    if (_running != running) {
        _running = running;
        
        // If we just set running to true, then also reset the frame to the first frame
        if (running) {
            // move back to the beginning
            _frameIndex = _firstFrame;
        }
    }
}
