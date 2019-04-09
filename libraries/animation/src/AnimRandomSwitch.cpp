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

const AnimPoseVec& AnimRandomSwitch::evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, AnimVariantMap& triggersOut) {
    float parentDebugAlpha = context.getDebugAlpha(_id);

    AnimRandomSwitch::RandomSwitchState::Pointer desiredState = _currentState;
    if (abs(_framesActive - context.getFramesAnimatedThisSession()) > 1 || animVars.lookup(_triggerRandomSwitchVar, false)) {
        // get a random number and decide which motion to choose.
        float dice = randFloatInRange(0.0f, 1.0f);
        float lowerBound = 0.0f;
        for (const RandomSwitchState::Pointer& randState : _randomStates) {
            if (randState->getPriority() > 0.0f) {
                float upperBound = lowerBound + (randState->getPriority() / _totalPriorities);
                if ((dice > lowerBound) && (dice < upperBound)) {
                    desiredState = randState;
                    break;
                } else {
                    lowerBound = upperBound;
                }
            }
        }
        if (abs(_framesActive - context.getFramesAnimatedThisSession()) > 1) {
            _duringInterp = false;
            switchRandomState(animVars, context, desiredState, _duringInterp);
        } else {
            if (desiredState->getID() != _currentState->getID()) {
                _duringInterp = true;
                switchRandomState(animVars, context, desiredState, _duringInterp);
            } else {
                _duringInterp = false;
            }
        }
        _triggerTime = randFloatInRange(_triggerTimeMin, _triggerTimeMax);

    } else {

        // here we are checking to see if we want a temporary movement
        // evaluate currentState transitions
        auto desiredState = evaluateTransitions(animVars);
        if (desiredState != _currentState) {
            _duringInterp = true;
            switchRandomState(animVars, context, desiredState, _duringInterp);
            _triggerTime = randFloatInRange(_triggerTimeMin, _triggerTimeMax);
        }
    }

    _triggerTime -= dt;
    if (_triggerTime < 0.0f) {
        _triggerTime = randFloatInRange(_triggerTimeMin, _triggerTimeMax);
        triggersOut.setTrigger(_transitionVar);
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
            context.setDebugAlpha(_currentState->getID(), _alpha * parentDebugAlpha, _children[_currentState->getChildIndex()]->getType());
        } else {
            AnimPoseVec checkNodes = currentStateNode->evaluate(animVars, context, dt, triggersOut);
            _duringInterp = false;
            _prevPoses.clear();
            _nextPoses.clear();
        }
        if (_duringInterp) {
            // hack: add previoius state to debug alpha map, with parens around it's name.
            context.setDebugAlpha(QString("(%1)").arg(_previousState->getID()), 1.0f - _alpha, AnimNodeType::Clip);
        }
    }else {
        context.setDebugAlpha(_currentState->getID(), parentDebugAlpha, _children[_currentState->getChildIndex()]->getType());
        _poses = currentStateNode->evaluate(animVars, context, dt, triggersOut);
    }

    _framesActive = context.getFramesAnimatedThisSession();
    processOutputJoints(triggersOut);
    context.addStateMachineInfo(_id, _currentState->getID(), _previousState->getID(), _duringInterp, _alpha);

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

    auto nextStateNode = _children[desiredState->getChildIndex()];
    if (shouldInterp) {

        const float FRAMES_PER_SECOND = 30.0f;

        auto prevStateNode = _children[_currentState->getChildIndex()];

        _alpha = 0.0f;
        float duration = std::max(0.001f, animVars.lookup(desiredState->_interpDurationVar, desiredState->_interpDuration));
        _alphaVel = FRAMES_PER_SECOND / duration;
        _interpType = (InterpType)animVars.lookup(desiredState->_interpTypeVar, (int)desiredState->_interpType);

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
            // snapshot previoius pose
            _prevPoses = _poses;
            // no need to evaluate _nextPoses we will do it dynamically during the interp,
            // however we need to set the current frame.
            if (!desiredState->getResume()) {
                nextStateNode->setCurrentFrame(desiredState->_interpTarget - duration);
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
