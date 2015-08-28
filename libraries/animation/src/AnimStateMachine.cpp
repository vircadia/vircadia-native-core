//
//  AnimStateMachine.cpp
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimStateMachine.h"
#include "AnimationLogging.h"

AnimStateMachine::AnimStateMachine(const std::string& id) :
    AnimNode(AnimNode::Type::StateMachine, id) {

}

AnimStateMachine::~AnimStateMachine() {

}



const AnimPoseVec& AnimStateMachine::evaluate(const AnimVariantMap& animVars, float dt) {

    std::string desiredStateID = animVars.lookup(_currentStateVar, _currentState->getID());
    if (_currentState->getID() != desiredStateID) {
        // switch states
        bool foundState = false;
        for (auto& state : _states) {
            if (state->getID() == desiredStateID) {
                switchState(state);
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
        switchState(desiredState);
    }

    if (_duringInterp) {
        // TODO: do interpolation
        /*
        const float FRAMES_PER_SECOND = 30.0f;
        // blend betwen poses
        blend(_poses.size(), _prevPose, _nextPose, _alpha, &_poses[0]);
        */
    } else {
        // eval current state
        assert(_currentState);
        auto currentStateNode = _currentState->getNode();
        assert(currentStateNode);
        _poses = currentStateNode->evaluate(animVars, dt);
    }

    qCDebug(animation) << "StateMachine::evalute";

    return _poses;
}

void AnimStateMachine::setCurrentState(State::Pointer state) {
    _currentState = state;
}

void AnimStateMachine::addState(State::Pointer state) {
    _states.push_back(state);
}

void AnimStateMachine::switchState(State::Pointer desiredState) {
    qCDebug(animation) << "AnimStateMachine::switchState:" << _currentState->getID().c_str() << "->" << desiredState->getID().c_str();
    // TODO: interp.
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
