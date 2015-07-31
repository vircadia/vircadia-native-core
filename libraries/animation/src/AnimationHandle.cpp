//
//  AnimationHandle.cpp
//  libraries/animation/src/
//
//  Created by Andrzej Kapolka on 10/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimationHandle.h"


void AnimationHandle::setURL(const QUrl& url) {
    if (_url != url) {
        _animation = DependencyManager::get<AnimationCache>()->getAnimation(_url = url);
        QObject::connect(_animation.data(), &Resource::onRefresh, this, &AnimationHandle::clearJoints);
        _jointMappings.clear();
    }
}

void AnimationHandle::setPriority(float priority) {
    if (_priority == priority) {
        return;
    }
    if (isRunning()) {
        _rig->removeRunningAnimation(getAnimationHandlePointer());
        if (priority < _priority) {
            replaceMatchingPriorities(priority);
        }
        _priority = priority;
        _rig->addRunningAnimation(getAnimationHandlePointer());
    } else {
        _priority = priority;
    }
}

void AnimationHandle::setStartAutomatically(bool startAutomatically) {
    if (startAutomatically && !isRunning()) {
        // Start before setting _animationLoop value so that code in setRunning() is executed
        start();
    }
    _animationLoop.setStartAutomatically(startAutomatically);
}

void AnimationHandle::setMaskedJoints(const QStringList& maskedJoints) {
    _maskedJoints = maskedJoints;
    _jointMappings.clear();
}

void AnimationHandle::setRunning(bool running) {
    if (running && isRunning()) {
        // if we're already running, this is the same as a restart
        setFrameIndex(getFirstFrame());
        return;
    }
    _animationLoop.setRunning(running);
    if (isRunning()) {
        if (!_rig->isRunningAnimation(getAnimationHandlePointer())) {
            _rig->addRunningAnimation(getAnimationHandlePointer());
        }
    } else {
        _rig->removeRunningAnimation(getAnimationHandlePointer());
        restoreJoints();
        replaceMatchingPriorities(0.0f);
    }
    emit runningChanged(isRunning());
}

AnimationHandle::AnimationHandle(RigPointer rig) :
    QObject(rig.get()),
    _rig(rig),
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


void AnimationHandle::setJointMappings(QVector<int> jointMappings) {
    _jointMappings = jointMappings;
}

QVector<int> AnimationHandle::getJointMappings() {
    if (_jointMappings.isEmpty()) {
        QVector<FBXJoint> animationJoints = _animation->getGeometry().joints;
        for (int i = 0; i < animationJoints.count(); i++) {
            _jointMappings.append(_rig->indexOfJoint(animationJoints.at(i).name));
        }
    }
    return _jointMappings;
}

void AnimationHandle::simulate(float deltaTime) {
    if (!_animation || !_animation->isLoaded()) {
        return;
    }

    _animationLoop.simulate(deltaTime);

    if (getJointMappings().isEmpty()) {
        qDebug() << "AnimationHandle::simulate -- _jointMappings.isEmpty()";
        return;
    }

    // // update the joint mappings if necessary/possible
    // if (_jointMappings.isEmpty()) {
    //     if (_model && _model->isActive()) {
    //         _jointMappings = _model->getGeometry()->getJointMappings(_animation);
    //     }
    //     if (_jointMappings.isEmpty()) {
    //         return;
    //     }
    //     if (!_maskedJoints.isEmpty()) {
    //         const FBXGeometry& geometry = _model->getGeometry()->getFBXGeometry();
    //         for (int i = 0; i < _jointMappings.size(); i++) {
    //             int& mapping = _jointMappings[i];
    //             if (mapping != -1 && _maskedJoints.contains(geometry.joints.at(mapping).name)) {
    //                 mapping = -1;
    //             }
    //         }
    //     }
    // }

    const FBXGeometry& animationGeometry = _animation->getGeometry();
    if (animationGeometry.animationFrames.isEmpty()) {
        stop();
        return;
    }

    if (_animationLoop.getMaxFrameIndexHint() != animationGeometry.animationFrames.size()) {
        _animationLoop.setMaxFrameIndexHint(animationGeometry.animationFrames.size());
    }

    // blend between the closest two frames
    applyFrame(getFrameIndex());
}

void AnimationHandle::applyFrame(float frameIndex) {
    if (!_animation || !_animation->isLoaded()) {
        return;
    }

    const FBXGeometry& animationGeometry = _animation->getGeometry();
    int frameCount = animationGeometry.animationFrames.size();
    const FBXAnimationFrame& floorFrame = animationGeometry.animationFrames.at((int)glm::floor(frameIndex) % frameCount);
    const FBXAnimationFrame& ceilFrame = animationGeometry.animationFrames.at((int)glm::ceil(frameIndex) % frameCount);
    float frameFraction = glm::fract(frameIndex);
    assert(_rig->getJointStateCount() >= _jointMappings.size());
    for (int i = 0; i < _jointMappings.size(); i++) {
        int mapping = _jointMappings.at(i);
        if (mapping != -1) {
            _rig->setJointRotationInConstrainedFrame(mapping,
                                                     safeMix(floorFrame.rotations.at(i),
                                                             ceilFrame.rotations.at(i),
                                                             frameFraction),
                                                     _priority,
                                                     false,
                                                     _mix);
        }
    }
}

void AnimationHandle::replaceMatchingPriorities(float newPriority) {
    for (int i = 0; i < _jointMappings.size(); i++) {
        int mapping = _jointMappings.at(i);
        if (mapping != -1) {
            if (_priority == _rig->getJointAnimatinoPriority(mapping)) {
                _rig->setJointAnimatinoPriority(mapping, newPriority);
            }
        }
    }
}

void AnimationHandle::restoreJoints() {
    for (int i = 0; i < _jointMappings.size(); i++) {
        int mapping = _jointMappings.at(i);
        if (mapping != -1) {
            _rig->restoreJointRotation(mapping, 1.0f, _rig->getJointAnimatinoPriority(mapping));
        }
    }
}
