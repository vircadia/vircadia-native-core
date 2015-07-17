//
//  Rig.cpp
//  libraries/script-engine/src/
//
//  Created by Howard Stearns, Seth Alves, Anthony Thibault, Andrew Meadows on 7/15/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimationHandle.h"

#include "Rig.h"

bool Rig::removeRunningAnimation(AnimationHandlePointer animationHandle) {
    return _runningAnimations.removeOne(animationHandle);
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

void Rig::addRunningAnimation(AnimationHandlePointer animationHandle) {
    insertSorted(_runningAnimations,  animationHandle);
}

bool Rig::isRunningAnimation(AnimationHandlePointer animationHandle) {
    return _runningAnimations.contains(animationHandle);
}

void Rig::initJointStates(glm::vec3 scale, glm::vec3 offset, QVector<JointState> states) {
    _jointStates = states;
    initJointTransforms(scale, offset);

    int numStates = _jointStates.size();
    float radius = 0.0f;
    for (int i = 0; i < numStates; ++i) {
        float distance = glm::length(_jointStates[i].getPosition());
        if (distance > radius) {
            radius = distance;
        }
        _jointStates[i].buildConstraint();
    }
    for (int i = 0; i < _jointStates.size(); i++) {
        _jointStates[i].slaveVisibleTransform();
    }

    // XXX update AnimationHandles from here?
}

void Rig::initJointTransforms(glm::vec3 scale, glm::vec3 offset) {
    // compute model transforms
    int numStates = _jointStates.size();
    for (int i = 0; i < numStates; ++i) {
        JointState& state = _jointStates[i];
        const FBXJoint& joint = state.getFBXJoint();
        int parentIndex = joint.parentIndex;
        if (parentIndex == -1) {
            // const FBXGeometry& geometry = _geometry->getFBXGeometry();
            // NOTE: in practice geometry.offset has a non-unity scale (rather than a translation)
            glm::mat4 parentTransform = glm::scale(scale) * glm::translate(offset); // * geometry.offset; XXX
            state.initTransform(parentTransform);
        } else {
            const JointState& parentState = _jointStates.at(parentIndex);
            state.initTransform(parentState.getTransform());
        }
    }
}

void Rig::resetJoints() {
    if (_jointStates.isEmpty()) {
        return;
    }

    // const FBXGeometry& geometry = _geometry->getFBXGeometry();
    for (int i = 0; i < _jointStates.size(); i++) {
        const FBXJoint& fbxJoint = _jointStates[i].getFBXJoint();
        _jointStates[i].setRotationInConstrainedFrame(fbxJoint.rotation, 0.0f);
    }
}

AnimationHandlePointer Rig::createAnimationHandle() {
    AnimationHandlePointer handle(new AnimationHandle(getRigPointer()));
    _animationHandles.insert(handle);
    return handle;
}
