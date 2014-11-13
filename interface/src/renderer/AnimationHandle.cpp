//
//  AnimationHandle.cpp
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
    if (isRunning()) {
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
    _animationLoop.setStartAutomatically(startAutomatically);
    if (getStartAutomatically() && !isRunning()) {
        start();
    }
}

void AnimationHandle::setMaskedJoints(const QStringList& maskedJoints) {
    _maskedJoints = maskedJoints;
    _jointMappings.clear();
}

void AnimationHandle::setRunning(bool running) {
    if (isRunning() == running) {
        // if we're already running, this is the same as a restart
        if (running) {
            // move back to the beginning
            setFrameIndex(getFirstFrame());
        }
        return;
    }
    _animationLoop.setRunning(running);
    if (isRunning()) {
        if (!_model->_runningAnimations.contains(_self)) {
            insertSorted(_model->_runningAnimations, _self);
        }
    } else {
        _model->_runningAnimations.removeOne(_self);
        replaceMatchingPriorities(0.0f);
    }
    emit runningChanged(isRunning());
}

AnimationHandle::AnimationHandle(Model* model) :
    QObject(model),
    _model(model),
    _priority(1.0f) 
{
}

AnimationDetails AnimationHandle::getAnimationDetails() const {
    AnimationDetails details(_role, _url, getFPS(), _priority, getLoop(), getHold(),
                        getStartAutomatically(), getFirstFrame(), getLastFrame(), isRunning(), getFrameIndex());
    return details;
}

void AnimationHandle::setAnimationDetails(const AnimationDetails& details) {
    setRole(details.role);
    setURL(details.url);
    setFPS(details.fps);
    setPriority(details.priority);
    setLoop(details.loop);
    setHold(details.hold);
    setStartAutomatically(details.startAutomatically);
    setFirstFrame(details.firstFrame);
    setLastFrame(details.lastFrame);
    setRunning(details.running);
    setFrameIndex(details.frameIndex);

    // NOTE: AnimationDetails doesn't support maskedJoints
    //setMaskedJoints(const QStringList& maskedJoints);
}


void AnimationHandle::simulate(float deltaTime) {
    _animationLoop.simulate(deltaTime);
    
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
    
    // TODO: When moving the loop/frame calculations to AnimationLoop class, we changed this behavior
    // see AnimationLoop class for more details. Do we need to support clamping the endFrameIndex to
    // the max number of frames in the geometry???
    //
    // float endFrameIndex = qMin(_lastFrame, animationGeometry.animationFrames.size() - (_loop ? 0.0f : 1.0f));
        
    // blend between the closest two frames
    applyFrame(getFrameIndex());
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

