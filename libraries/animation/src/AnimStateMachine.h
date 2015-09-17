//
//  AnimStateMachine.h
//
//  Created by Anthony J. Thibault on 9/2/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimStateMachine_h
#define hifi_AnimStateMachine_h

#include <string>
#include <vector>
#include "AnimNode.h"

// State Machine for transitioning between children AnimNodes
//
// This is mechinisim for playing animations and smoothly interpolating/fading
// between them.  A StateMachine has a set of States, which typically reference
// child AnimNodes.  Each State has a list of Transitions, which are evaluated
// to determine when we should switch to a new State.  Parameters for the smooth
// interpolation/fading are read from the State that you are transitioning to.
//
// The currentState can be set directly via the setCurrentStateVar() and will override
// any State transitions.
//
// Each State has two parameters that can be changed via AnimVars,
// * interpTarget - (frames) The destination frame of the interpolation. i.e. the first frame of the animation that will
//   visible after interpolation is complete.
// * interpDuration - (frames) The total length of time it will take to interp between the current pose and the
//   interpTarget frame.

class AnimStateMachine : public AnimNode {
public:
    friend class AnimNodeLoader;
    friend bool processStateMachineNode(AnimNode::Pointer node, const QJsonObject& jsonObj, const QString& nodeId, const QUrl& jsonUrl);

protected:
    class State {
    public:
        friend AnimStateMachine;
        friend bool processStateMachineNode(AnimNode::Pointer node, const QJsonObject& jsonObj, const QString& nodeId, const QUrl& jsonUrl);

        using Pointer = std::shared_ptr<State>;
        using ConstPointer = std::shared_ptr<const State>;

        class Transition {
        public:
            friend AnimStateMachine;
            Transition(const QString& var, State::Pointer state) : _var(var), _state(state) {}
        protected:
            QString _var;
            State::Pointer _state;
        };

        State(const QString& id, AnimNode::Pointer node, float interpTarget, float interpDuration) :
            _id(id),
            _node(node),
            _interpTarget(interpTarget),
            _interpDuration(interpDuration) {}

        void setInterpTargetVar(const QString& interpTargetVar) { _interpTargetVar = interpTargetVar; }
        void setInterpDurationVar(const QString& interpDurationVar) { _interpDurationVar = interpDurationVar; }

        AnimNode::Pointer getNode() const { return _node; }
        const QString& getID() const { return _id; }

    protected:

        void setInterpTarget(float interpTarget) { _interpTarget = interpTarget; }
        void setInterpDuration(float interpDuration) { _interpDuration = interpDuration; }

        void addTransition(const Transition& transition) { _transitions.push_back(transition); }

        QString _id;
        AnimNode::Pointer _node;
        float _interpTarget;  // frames
        float _interpDuration; // frames

        QString _interpTargetVar;
        QString _interpDurationVar;

        std::vector<Transition> _transitions;

    private:
        // no copies
        State(const State&) = delete;
        State& operator=(const State&) = delete;
    };

public:

    AnimStateMachine(const QString& id);
    virtual ~AnimStateMachine() override;

    virtual const AnimPoseVec& evaluate(const AnimVariantMap& animVars, float dt, Triggers& triggersOut) override;

    void setCurrentStateVar(QString& currentStateVar) { _currentStateVar = currentStateVar; }

protected:

    void setCurrentState(State::Pointer state);

    void addState(State::Pointer state);

    void switchState(const AnimVariantMap& animVars, State::Pointer desiredState);
    State::Pointer evaluateTransitions(const AnimVariantMap& animVars) const;

    // for AnimDebugDraw rendering
    virtual const AnimPoseVec& getPosesInternal() const override;

    AnimPoseVec _poses;

    // interpolation state
    bool _duringInterp = false;
    float _alphaVel = 0.0f;
    float _alpha = 0.0f;
    AnimPoseVec _prevPoses;
    AnimPoseVec _nextPoses;

    State::Pointer _currentState;
    std::vector<State::Pointer> _states;

    QString _currentStateVar;

private:
    // no copies
    AnimStateMachine(const AnimStateMachine&) = delete;
    AnimStateMachine& operator=(const AnimStateMachine&) = delete;
};

#endif // hifi_AnimStateMachine_h
