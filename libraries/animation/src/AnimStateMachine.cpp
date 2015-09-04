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

AnimStateMachine::AnimStateMachine(const std::string& id) :
    AnimNode(AnimNode::Type::StateMachine, id) {

}

AnimStateMachine::~AnimStateMachine() {

}

const AnimPoseVec& AnimStateMachine::evaluate(const AnimVariantMap& animVars, float dt, Triggers& triggersOut) {

    std::string desiredStateID = animVars.lookup(_currentStateVar, _currentState->getID());
    if (_currentState->getID() != desiredStateID) {
        // switch states
        bool foundState = false;
        for (auto& state : _states) {
            if (state->getID() == desiredStateID) {
                switchState(animVars, state);
                foundState = true;
                break;
            }
        }
        if (!foundState) {
            qCCritical(animation) << "AnimStateMachine could not find state =" << desiredStateID.c_str() << ", referenced by _currentStateVar =" << _currentStateVar.c_str();
        }
    }

    // evaluate currentState transitions
    auto desiredState = evaluateTransitions(animVars);
    if (desiredState != _currentState) {
        switchState(animVars, desiredState);
    }

    assert(_currentState);
    auto currentStateNode = _currentState->getNode();
    assert(currentStateNode);

    if (_duringInterp) {
        _alpha += _alphaVel * dt;
        if (_alpha < 1.0f) {
            if (_poses.size() > 0) {
                ::blend(_poses.size(), &_prevPoses[0], &_nextPoses[0], _alpha, &_poses[0]);
            }
        } else {
            _duringInterp = false;
            _prevPoses.clear();
            _nextPoses.clear();
        }
    }
    if (!_duringInterp) {
        _poses = currentStateNode->evaluate(animVars, dt, triggersOut);
    }
    return _poses;
}

void AnimStateMachine::setCurrentState(State::Pointer state) {
    _currentState = state;
}

void AnimStateMachine::addState(State::Pointer state) {
    _states.push_back(state);
}

void AnimStateMachine::switchState(const AnimVariantMap& animVars, State::Pointer desiredState) {

    qCDebug(animation) << "AnimStateMachine::switchState:" << _currentState->getID().c_str() << "->" << desiredState->getID().c_str();

    const float FRAMES_PER_SECOND = 30.0f;

    auto prevStateNode = _currentState->getNode();
    auto nextStateNode = desiredState->getNode();

    _duringInterp = true;
    _alpha = 0.0f;
    float duration = std::max(0.001f, animVars.lookup(desiredState->_interpDurationVar, desiredState->_interpDuration));
    _alphaVel = FRAMES_PER_SECOND / duration;
    _prevPoses = _poses;
    nextStateNode->setCurrentFrame(desiredState->_interpTarget);

    // because dt is 0, we should not encounter any triggers
    const float dt = 0.0f;
    Triggers triggers;
    _nextPoses = nextStateNode->evaluate(animVars, dt, triggers);

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
