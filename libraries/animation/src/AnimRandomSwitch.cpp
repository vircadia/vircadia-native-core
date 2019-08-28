//
//  AnimRandomSwitch.cpp
//
//  Created by Angus Antley on 4/8/2019.
//  Copyright (c) 2019 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimRandomSwitch.h"
#include "AnimUtil.h"
#include "AnimationLogging.h"

AnimRandomSwitch::AnimRandomSwitch(const QString& id) :
	AnimNode(AnimNode::Type::RandomSwitchStateMachine, id) {

}

AnimRandomSwitch::~AnimRandomSwitch() {

}

void AnimRandomSwitch::setActiveInternal(bool active) {
    _active = active;
    _triggerNewRandomState = active;
}

const AnimPoseVec& AnimRandomSwitch::evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, AnimVariantMap& triggersOut) {
    float parentDebugAlpha = context.getDebugAlpha(_id);

    AnimRandomSwitch::RandomSwitchState::Pointer desiredState = _currentState;
    if (_triggerNewRandomState || animVars.lookup(_triggerRandomSwitchVar, false)) {
        // filter states different to the last random state and with priorities.
        bool currentStateHasPriority = false;
        std::vector<RandomSwitchState::Pointer> randomStatesToConsider;
        randomStatesToConsider.reserve(_randomStates.size());
        float totalPriorities = 0.0f;
        for (size_t i = 0; i < _randomStates.size(); i++) {
            auto randState = _randomStates[i];
            if (randState->getPriority() > 0.0f) {
                bool isRepeatingClip = _children[randState->getChildIndex()]->getID() == _lastPlayedState;
                if (!isRepeatingClip) {
                    randomStatesToConsider.push_back(randState);
                    totalPriorities += randState->getPriority();
                }
                // this indicates if the curent state is one that can be selected randomly, or is one that was transitioned to by the random duration timer.
                currentStateHasPriority = currentStateHasPriority || (_currentState == randState);
            }
        }
        // get a random number and decide which motion to choose.
        float dice = randFloatInRange(0.0f, 1.0f);
        float lowerBound = 0.0f;
        for (size_t i = 0; i < randomStatesToConsider.size(); i++) {
            auto randState = randomStatesToConsider[i];
            float upperBound = lowerBound + (randState->getPriority() / totalPriorities);
            if ((dice > lowerBound) && (dice < upperBound)) {
                desiredState = randState;
                break;
            }
            lowerBound = upperBound;
        }
        if (_triggerNewRandomState) {
            switchRandomState(animVars, context, desiredState, false);
            _triggerNewRandomState = false;
        } else {
            // firing a random switch, be sure that we aren't completing a previously triggered transition
            if (currentStateHasPriority) {
                if (desiredState->getID() != _currentState->getID()) {
                    switchRandomState(animVars, context, desiredState, true);
                } else {
                    _duringInterp = false;
                }
            }
        }
        _triggerTime = randFloatInRange(_triggerTimeMin, _triggerTimeMax);
        _randomSwitchTime = randFloatInRange(_randomSwitchTimeMin, _randomSwitchTimeMax);
    } else {

        // here we are checking to see if we want a temporary movement
        // evaluate currentState transitions
        auto transitionState = evaluateTransitions(animVars);
        if (transitionState != _currentState) {
            switchRandomState(animVars, context, transitionState, true);
            _triggerTime = randFloatInRange(_triggerTimeMin, _triggerTimeMax);
            _randomSwitchTime = randFloatInRange(_randomSwitchTimeMin, _randomSwitchTimeMax);
        }
    }

    _triggerTime -= dt;
    if ((_triggerTime < 0.0f) && (_triggerTimeMin > 0.0f) && (_triggerTimeMax > 0.0f)) {
        _triggerTime = randFloatInRange(_triggerTimeMin, _triggerTimeMax);
        triggersOut.setTrigger(_transitionVar);
    }

    _randomSwitchTime -= dt;
    if ((_randomSwitchTime < 0.0f) && (_randomSwitchTimeMin > 0.0f) && (_randomSwitchTimeMax > 0.0f)) {
        _randomSwitchTime = randFloatInRange(_randomSwitchTimeMin, _randomSwitchTimeMax);
        // restart the trigger timer if it is also enabled
        _triggerTime = randFloatInRange(_triggerTimeMin, _triggerTimeMax);
        triggersOut.setTrigger(_triggerRandomSwitchVar);
    }

    assert(_currentState);
    auto currentStateNode = _children[_currentState->getChildIndex()];
    auto previousStateNode = _children[_previousState->getChildIndex()];
    assert(currentStateNode);

    if (_duringInterp) {
        _alpha += _alphaVel * dt;
        if (_alpha < 1.0f) {
            AnimPoseVec* nextPoses = nullptr;
            AnimPoseVec* prevPoses = nullptr;
            AnimPoseVec localNextPoses;
            AnimPoseVec localPrevPoses;
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
            } else if (_interpType == InterpType::EvaluateBoth) {
                localPrevPoses = previousStateNode->evaluate(animVars, context, dt, triggersOut);
                localNextPoses = currentStateNode->evaluate(animVars, context, dt, triggersOut);
                prevPoses = &localPrevPoses;
                nextPoses = &localNextPoses;
            } else {
                assert(false);
            }
            if (_poses.size() > 0 && nextPoses && prevPoses && nextPoses->size() > 0 && prevPoses->size() > 0) {
                ::blend(_poses.size(), &(prevPoses->at(0)), &(nextPoses->at(0)), easingFunc(_alpha, _easingType), &_poses[0]);
            }
            context.setDebugAlpha(_currentState->getID(), easingFunc(_alpha, _easingType) * parentDebugAlpha, _children[_currentState->getChildIndex()]->getType());
        } else {
            _duringInterp = false;
            _prevPoses.clear();
            _nextPoses.clear();
        }
    }

    if (!_duringInterp){
        context.setDebugAlpha(_currentState->getID(), parentDebugAlpha, _children[_currentState->getChildIndex()]->getType());
        _poses = currentStateNode->evaluate(animVars, context, dt, triggersOut);
    }

    processOutputJoints(triggersOut);

    context.addStateMachineInfo(_id, _currentState->getID(), _previousState->getID(), _duringInterp, _alpha);
    if (_duringInterp) {
        // hack: add previoius state to debug alpha map, with parens around it's name.
        context.setDebugAlpha(QString("(%1)").arg(_previousState->getID()), 1.0f - _alpha, AnimNodeType::Clip);
    }

    return _poses;
}

void AnimRandomSwitch::setCurrentState(RandomSwitchState::Pointer randomState) {
	_previousState = _currentState ? _currentState : randomState;
	_currentState = randomState;
}

void AnimRandomSwitch::addState(RandomSwitchState::Pointer randomState) {
	_randomStates.push_back(randomState);
}

void AnimRandomSwitch::switchRandomState(const AnimVariantMap& animVars, const AnimContext& context, RandomSwitchState::Pointer desiredState, bool shouldInterp) {

    auto prevStateNode = _children[_currentState->getChildIndex()];
    auto nextStateNode = _children[desiredState->getChildIndex()];

    // activate/deactivate states
    prevStateNode->setActive(false);
    nextStateNode->setActive(true);

    _lastPlayedState = nextStateNode->getID();

    if (shouldInterp) {

        bool interpActive = _duringInterp;
        _duringInterp = true;

        const float FRAMES_PER_SECOND = 30.0f;

        auto prevStateNode = _children[_currentState->getChildIndex()];

        _alpha = 0.0f;
        float duration = std::max(0.001f, animVars.lookup(desiredState->_interpDurationVar, desiredState->_interpDuration));
        _alphaVel = FRAMES_PER_SECOND / duration;
        _interpType = (InterpType)animVars.lookup(desiredState->_interpTypeVar, (int)desiredState->_interpType);
        _easingType = desiredState->_easingType;

        // because dt is 0, we should not encounter any triggers
        const float dt = 0.0f;
        AnimVariantMap triggers;

        if (_interpType == InterpType::SnapshotBoth) {
            // snapshot previous pose.
            _prevPoses = _poses;
            // snapshot next pose at the target frame.
            if (!desiredState->getResume()) {
                nextStateNode->setCurrentFrame(desiredState->_interpTarget);
            }
            _nextPoses = nextStateNode->evaluate(animVars, context, dt, triggers);
        } else if (_interpType == InterpType::SnapshotPrev) {
            // snapshot previous pose
            _prevPoses = _poses;
            // no need to evaluate _nextPoses we will do it dynamically during the interp,
            // however we need to set the current frame.
            if (!desiredState->getResume()) {
                nextStateNode->setCurrentFrame(desiredState->_interpTarget - duration);
            }
        } else if (_interpType == InterpType::EvaluateBoth) {
            // need to set current frame in destination branch.
            nextStateNode->setCurrentFrame(desiredState->_interpTarget - duration);
            if (interpActive) {
                // snapshot previous pose
                _prevPoses = _poses;
                _interpType = InterpType::SnapshotPrev;
            }
        } else {
            assert(false);
        }
    } else {
        if (!desiredState->getResume()) {
            nextStateNode->setCurrentFrame(desiredState->_interpTarget);
        }
    }

#ifdef WANT_DEBUG
	qCDebug(animation) << "AnimRandomSwitch::switchState:" << _currentState->getID() << "->" << desiredState->getID() << "duration =" << duration << "targetFrame =" << desiredState->_interpTarget << "interpType = " << (int)_interpType;
#endif

	setCurrentState(desiredState);
}

AnimRandomSwitch::RandomSwitchState::Pointer AnimRandomSwitch::evaluateTransitions(const AnimVariantMap& animVars) const {
	assert(_currentState);
	for (auto& transition : _currentState->_transitions) {
		if (animVars.lookup(transition._var, false)) {
			return transition._randomSwitchState;
		}
	}
	return _currentState;
}

const AnimPoseVec& AnimRandomSwitch::getPosesInternal() const {
	return _poses;
}
