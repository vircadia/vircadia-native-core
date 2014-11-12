//
//  Model.cpp
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 10/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimationHandle.h"
#include "Application.h"

void AnimationHandle::setURL(const QUrl& url) {
    if (_url != url) {
        _animation = Application::getInstance()->getAnimationCache()->getAnimation(_url = url);
        _jointMappings.clear();
    }
}

static void insertSorted(QList<AnimationHandlePointer>& handles, const AnimationHandlePointer& handle) {
    for (QList<AnimationHandlePointer>::iterator it = handles.begin(); it != handles.end(); it++) {
        if (handle->getPriority() > (*it)->getPriority()) {
            handles.insert(it, handle);
            return;
        } 
    }
    handles.append(handle);
}

void AnimationHandle::setPriority(float priority) {
    if (_priority == priority) {
        return;
    }
    if (_running) {
        _model->_runningAnimations.removeOne(_self);
        if (priority < _priority) {
            replaceMatchingPriorities(priority);
        }
        _priority = priority;
        insertSorted(_model->_runningAnimations, _self);
        
    } else {
        _priority = priority;
    }
}

void AnimationHandle::setStartAutomatically(bool startAutomatically) {
    if ((_startAutomatically = startAutomatically) && !_running) {
        start();
    }
}

void AnimationHandle::setMaskedJoints(const QStringList& maskedJoints) {
    _maskedJoints = maskedJoints;
    _jointMappings.clear();
}

void AnimationHandle::setRunning(bool running) {
    if (_running == running) {
        if (running) {
            // move back to the beginning
            _frameIndex = _firstFrame;
        }
        return;
    }
    if ((_running = running)) {
        if (!_model->_runningAnimations.contains(_self)) {
            insertSorted(_model->_runningAnimations, _self);
        }
        _frameIndex = _firstFrame;
          
    } else {
        _model->_runningAnimations.removeOne(_self);
        replaceMatchingPriorities(0.0f);
    }
    emit runningChanged(_running);
}

AnimationHandle::AnimationHandle(Model* model) :
    QObject(model),
    _model(model),
    _fps(30.0f),
    _priority(1.0f),
    _loop(false),
    _hold(false),
    _startAutomatically(false),
    _firstFrame(0.0f),
    _lastFrame(FLT_MAX),
    _running(false) {
}

AnimationDetails AnimationHandle::getAnimationDetails() const {
    AnimationDetails details(_role, _url, _fps, _priority, _loop, _hold,
                        _startAutomatically, _firstFrame, _lastFrame, _running, _frameIndex);
    return details;
}


void AnimationHandle::simulate(float deltaTime) {
    _frameIndex += deltaTime * _fps;
    
    // update the joint mappings if necessary/possible
    if (_jointMappings.isEmpty()) {
        if (_model->isActive()) {
            _jointMappings = _model->getGeometry()->getJointMappings(_animation);
        }
        if (_jointMappings.isEmpty()) {
            return;
        }
        if (!_maskedJoints.isEmpty()) {
            const FBXGeometry& geometry = _model->getGeometry()->getFBXGeometry();
            for (int i = 0; i < _jointMappings.size(); i++) {
                int& mapping = _jointMappings[i];
                if (mapping != -1 && _maskedJoints.contains(geometry.joints.at(mapping).name)) {
                    mapping = -1;
                }
            }
        }
    }
    
    const FBXGeometry& animationGeometry = _animation->getGeometry();
    if (animationGeometry.animationFrames.isEmpty()) {
        stop();
        return;
    }
    float endFrameIndex = qMin(_lastFrame, animationGeometry.animationFrames.size() - (_loop ? 0.0f : 1.0f));
    float startFrameIndex = qMin(_firstFrame, endFrameIndex);
    if ((!_loop && (_frameIndex < startFrameIndex || _frameIndex > endFrameIndex)) || startFrameIndex == endFrameIndex) {
        // passed the end; apply the last frame
        applyFrame(glm::clamp(_frameIndex, startFrameIndex, endFrameIndex));
        if (!_hold) {
            stop();
        }
        return;
    }
    // wrap within the the desired range
    if (_frameIndex < startFrameIndex) {
        _frameIndex = endFrameIndex - glm::mod(endFrameIndex - _frameIndex, endFrameIndex - startFrameIndex);
    
    } else if (_frameIndex > endFrameIndex) {
        _frameIndex = startFrameIndex + glm::mod(_frameIndex - startFrameIndex, endFrameIndex - startFrameIndex);
    }
    
    // blend between the closest two frames
    applyFrame(_frameIndex);
}

void AnimationHandle::applyFrame(float frameIndex) {
    const FBXGeometry& animationGeometry = _animation->getGeometry();
    int frameCount = animationGeometry.animationFrames.size();
    const FBXAnimationFrame& floorFrame = animationGeometry.animationFrames.at((int)glm::floor(frameIndex) % frameCount);
    const FBXAnimationFrame& ceilFrame = animationGeometry.animationFrames.at((int)glm::ceil(frameIndex) % frameCount);
    float frameFraction = glm::fract(frameIndex);
    for (int i = 0; i < _jointMappings.size(); i++) {
        int mapping = _jointMappings.at(i);
        if (mapping != -1) {
            JointState& state = _model->_jointStates[mapping];
            state.setRotationInConstrainedFrame(safeMix(floorFrame.rotations.at(i), ceilFrame.rotations.at(i), frameFraction), _priority);
        }
    }
}

void AnimationHandle::replaceMatchingPriorities(float newPriority) {
    for (int i = 0; i < _jointMappings.size(); i++) {
        int mapping = _jointMappings.at(i);
        if (mapping != -1) {
            JointState& state = _model->_jointStates[mapping];
            if (_priority == state._animationPriority) {
                state._animationPriority = newPriority;
            }
        }
    }
}

