//
//  AnimationLoop.cpp
//  libraries/animation/src/
//
//  Created by Brad Hefta-Gaub on 11/12/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <NumericalConstants.h>

#include "AnimationCache.h"
#include "AnimationLoop.h"

const float AnimationLoop::MAXIMUM_POSSIBLE_FRAME = 100000.0f;

AnimationLoop::AnimationLoop() :
    _fps(30.0f),
    _loop(true),
    _hold(false),
    _startAutomatically(false),
    _firstFrame(0.0f),
    _lastFrame(MAXIMUM_POSSIBLE_FRAME),
    _running(false),
    _currentFrame(0.0f),
    _maxFrameIndexHint(MAXIMUM_POSSIBLE_FRAME),
    _resetOnRunning(true),
    _lastSimulated(usecTimestampNow())
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
    _currentFrame(animationDetails.currentFrame),
    _maxFrameIndexHint(MAXIMUM_POSSIBLE_FRAME),
    _resetOnRunning(true),
    _lastSimulated(usecTimestampNow())
{
}

AnimationLoop::AnimationLoop(float fps, bool loop, bool hold, bool startAutomatically, float firstFrame, 
                    float lastFrame, bool running, float currentFrame) :
    _fps(fps),
    _loop(loop),
    _hold(hold),
    _startAutomatically(startAutomatically),
    _firstFrame(firstFrame),
    _lastFrame(lastFrame),
    _running(running),
    _currentFrame(currentFrame),
    _maxFrameIndexHint(MAXIMUM_POSSIBLE_FRAME),
    _resetOnRunning(true),
    _lastSimulated(usecTimestampNow())
{
}

void AnimationLoop::simulateAtTime(quint64 now) {
    float deltaTime = (float)(now - _lastSimulated) / (float)USECS_PER_SECOND;
    _lastSimulated = now;
    simulate(deltaTime);
}

void AnimationLoop::simulate(float deltaTime) {
    _currentFrame += deltaTime * _fps;

    // If we knew the number of frames from the animation, we'd consider using it here 
    // animationGeometry.animationFrames.size()
    float maxFrame = _maxFrameIndexHint;
    float endFrameIndex = qMin(_lastFrame, maxFrame - (_loop ? 0.0f : 1.0f));
    float startFrameIndex = qMin(_firstFrame, endFrameIndex);
    if ((!_loop && (_currentFrame < startFrameIndex || _currentFrame > endFrameIndex)) || startFrameIndex == endFrameIndex) {
        // passed the end; apply the last frame
        _currentFrame = glm::clamp(_currentFrame, startFrameIndex, endFrameIndex);
        if (!_hold) {
            stop();
        }
    } else {
        // wrap within the the desired range
        if (_currentFrame < startFrameIndex) {
            _currentFrame = endFrameIndex - glm::mod(endFrameIndex - _currentFrame, endFrameIndex - startFrameIndex);
        } else if (_currentFrame > endFrameIndex) {
            _currentFrame = startFrameIndex + glm::mod(_currentFrame - startFrameIndex, endFrameIndex - startFrameIndex);
        }
    }
}

void AnimationLoop::setStartAutomatically(bool startAutomatically) {
    if ((_startAutomatically = startAutomatically) && !getRunning()) {
        start();
    }
}
    
void AnimationLoop::setRunning(bool running) {
    // don't do anything if the new value is the same as the value we already have
    if (_running != running) {
        _running = running;
        
        // If we just set running to true, then also reset the frame to the first frame
        if (running && (_resetOnRunning)) {
            // move back to the beginning
            _currentFrame = _firstFrame;
        }

        // If we just started running, set our 
        if (_running) {
            _lastSimulated = usecTimestampNow();
        }
    }
}
