//
//  AnimStateMachine.cpp
//
//  Created by Anthony J. Thibault on 9/2/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimStateMachine.h"
#include "AnimUtil.h"
#include "AnimationLogging.h"

AnimStateMachine::AnimStateMachine(const QString& id) :
    AnimNode(AnimNode::Type::StateMachine, id) {

}

AnimStateMachine::~AnimStateMachine() {

}

const AnimPoseVec& AnimStateMachine::evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, Triggers& triggersOut) {

    QString desiredStateID = animVars.lookup(_currentStateVar, _currentState->getID());
    if (_currentState->getID() != desiredStateID) {
        // switch states
        bool foundState = false;
        for (auto& state : _states) {
            if (state->getID() == desiredStateID) {
                switchState(animVars, context, state);
                foundState = true;
                break;
            }
        }
        if (!foundState) {
            qCCritical(animation) << "AnimStateMachine could not find state =" << desiredStateID << ", referenced by _currentStateVar =" << _currentStateVar;
        }
    }

    // evaluate currentState transitions
    auto desiredState = evaluateTransitions(animVars);
    if (desiredState != _currentState) {
        switchState(animVars, context, desiredState);
    }

    assert(_currentState);
    auto currentStateNode = _children[_currentState->getChildIndex()];
    assert(currentStateNode);

    if (_duringInterp) {
        _alpha += _alphaVel * dt;
        if (_alpha < 1.0f) {
            AnimPoseVec* nextPoses = nullptr;
            AnimPoseVec* prevPoses = nullptr;
            AnimPoseVec localNextPoses;
            if (_interpType == InterpType::SnapshotBoth) {
                // interp between both snapshots
                prevPoses = &_prevPoses;
                nextPoses = &_nextPoses;
            } else if (_interpType == InterpType::SnapshotPrev) {
                // interp between the prev snapshot and evaluated next target.
                // this is useful for interping into a blend
                localNextPoses = currentStateNode->evaluate(animVars, context, dt, triggersOut);
                prevPoses = &_prevPoses;
                nextPoses = &localNextPoses;
            } else {
                assert(false);
            }

            if (_poses.size() > 0 && nextPoses && prevPoses && nextPoses->size() > 0 && prevPoses->size() > 0) {
                ::blend(_poses.size(), &(prevPoses->at(0)), &(nextPoses->at(0)), _alpha, &_poses[0]);
            }
        } else {
            _duringInterp = false;
            _prevPoses.clear();
            _nextPoses.clear();
        }
    }
    if (!_duringInterp) {
        _poses = currentStateNode->evaluate(animVars, context, dt, triggersOut);
    }
    return _poses;
}

void AnimStateMachine::setCurrentState(State::Pointer state) {
    _currentState = state;
}

void AnimStateMachine::addState(State::Pointer state) {
    _states.push_back(state);
}

void AnimStateMachine::switchState(const AnimVariantMap& animVars, const AnimContext& context, State::Pointer desiredState) {

    const float FRAMES_PER_SECOND = 30.0f;

    auto prevStateNode = _children[_currentState->getChildIndex()];
    auto nextStateNode = _children[desiredState->getChildIndex()];

    _duringInterp = true;
    _alpha = 0.0f;
    float duration = std::max(0.001f, animVars.lookup(desiredState->_interpDurationVar, desiredState->_interpDuration));
    _alphaVel = FRAMES_PER_SECOND / duration;
    _interpType = (InterpType)animVars.lookup(desiredState->_interpTypeVar, (int)desiredState->_interpType);

    // because dt is 0, we should not encounter any triggers
    const float dt = 0.0f;
    Triggers triggers;

    if (_interpType == InterpType::SnapshotBoth) {
        // snapshot previous pose.
        _prevPoses = _poses;
        // snapshot next pose at the target frame.
        nextStateNode->setCurrentFrame(desiredState->_interpTarget);
        _nextPoses = nextStateNode->evaluate(animVars, context, dt, triggers);
    } else if (_interpType == InterpType::SnapshotPrev) {
        // snapshot previoius pose
        _prevPoses = _poses;
        // no need to evaluate _nextPoses we will do it dynamically during the interp,
        // however we need to set the current frame.
        nextStateNode->setCurrentFrame(desiredState->_interpTarget - duration);
    } else {
        assert(false);
    }

#ifdef WANT_DEBUG
    qCDebug(animation) << "AnimStateMachine::switchState:" << _currentState->getID() << "->" << desiredState->getID() << "duration =" << duration << "targetFrame =" << desiredState->_interpTarget << "interpType = " << (int)_interpType;
#endif

    _currentState = desiredState;
}

AnimStateMachine::State::Pointer AnimStateMachine::evaluateTransitions(const AnimVariantMap& animVars) const {
    assert(_currentState);
    for (auto& transition : _currentState->_transitions) {
        if (animVars.lookup(transition._var, false)) {
            return transition._state;
        }
    }
    return _currentState;
}

const AnimPoseVec& AnimStateMachine::getPosesInternal() const {
    return _poses;
}
