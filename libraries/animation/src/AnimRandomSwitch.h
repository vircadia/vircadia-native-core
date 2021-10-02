//
//  AnimRandomSwitch.h
//
//  Created by Angus Antley on 4/8/19.
//  Copyright (c) 2019 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimRandomSwitch_h
#define hifi_AnimRandomSwitch_h

#include <string>
#include <vector>
#include "AnimNode.h"
#include "AnimUtil.h"

// Random Switch State Machine for random transitioning between children AnimNodes
//
// This is mechanisim for choosing and playing a random animation and smoothly interpolating/fading
// between them.  A RandomSwitch has a set of States, which typically reference
// child AnimNodes.  Each Random Switch State has a list of Transitions, which are evaluated
// to determine when we should switch to a new State.  Parameters for the smooth
// interpolation/fading are read from the Random Switch State that you are transitioning to.
//
// The currentState can be set directly via the setCurrentStateVar() and will override
// any State transitions.
//
// Each Random Switch State has two parameters that can be changed via AnimVars,
// * interpTarget - (frames) The destination frame of the interpolation. i.e. the first frame of the animation that will
//   visible after interpolation is complete.
// * interpDuration - (frames) The total length of time it will take to interp between the current pose and the
//   interpTarget frame.
// * interpType - How the interpolation is performed.
// * priority - this number represents how likely this Random Switch State will be chosen.
//   the priority for each Random Switch State will be normalized, so their relative size is what is important
// * resume - if resume is false then if this state is chosen twice in a row it will remember what frame it was playing on.
// * SnapshotBoth: Stores two snapshots, the previous animation before interpolation begins and the target state at the
//   interTarget frame.  Then during the interpolation period the two snapshots are interpolated to produce smooth motion between them.
// * SnapshotPrev: Stores a snapshot of the previous animation before interpolation begins.  However the target state is
//   evaluated dynamically.  During the interpolation period the previous snapshot is interpolated with the target pose
//   to produce smooth motion between them.  This mode is useful for interping into a blended animation where the actual
//   blend factor is not known at the start of the interp or is might change dramatically during the interp.
//

class AnimRandomSwitch : public AnimNode {
public:
    friend class AnimNodeLoader;
    friend bool processRandomSwitchStateMachineNode(AnimNode::Pointer node, const QJsonObject& jsonObj, const QString& nodeId, const QUrl& jsonUrl);

    enum class InterpType {
        SnapshotBoth = 0,
        SnapshotPrev,
        EvaluateBoth,
        NumTypes
    };

protected:

    class RandomSwitchState {
    public:
        friend AnimRandomSwitch;
        friend bool processRandomSwitchStateMachineNode(AnimNode::Pointer node, const QJsonObject& jsonObj, const QString& nodeId, const QUrl& jsonUrl);

        using Pointer = std::shared_ptr<RandomSwitchState>;
        using ConstPointer = std::shared_ptr<const RandomSwitchState>;

        class Transition {
        public:
            friend AnimRandomSwitch;
            Transition(const QString& var, RandomSwitchState::Pointer randomState) : _var(var), _randomSwitchState(randomState) {}
        protected:
            QString _var;
            RandomSwitchState::Pointer _randomSwitchState;
        };

        RandomSwitchState(const QString& id, int childIndex, float interpTarget, float interpDuration, InterpType interpType, EasingType easingType, float priority, bool resume) :
            _id(id),
            _childIndex(childIndex),
            _interpTarget(interpTarget),
            _interpDuration(interpDuration),
            _interpType(interpType),
            _easingType(easingType),
            _priority(priority),
            _resume(resume){
        }

        void setInterpTargetVar(const QString& interpTargetVar) { _interpTargetVar = interpTargetVar; }
        void setInterpDurationVar(const QString& interpDurationVar) { _interpDurationVar = interpDurationVar; }
        void setInterpTypeVar(const QString& interpTypeVar) { _interpTypeVar = interpTypeVar; }

        int getChildIndex() const { return _childIndex; }
        float getPriority() const { return _priority; }
        bool getResume() const { return _resume; }
        const QString& getID() const { return _id; }

    protected:

        void setInterpTarget(float interpTarget) { _interpTarget = interpTarget; }
        void setInterpDuration(float interpDuration) { _interpDuration = interpDuration; }
        void setPriority(float priority) { _priority = priority; }
        void setResumeFlag(bool resume) { _resume = resume; }

        void addTransition(const Transition& transition) { _transitions.push_back(transition); }

        QString _id;
        int _childIndex;
        float _interpTarget;  // frames
        float _interpDuration; // frames
        InterpType _interpType;
        EasingType _easingType;
        float _priority {0.0f};
        bool _resume {false};

        QString _interpTargetVar;
        QString _interpDurationVar;
        QString _interpTypeVar;

        std::vector<Transition> _transitions;

    private:
        Q_DISABLE_COPY(RandomSwitchState)
    };

public:

    explicit AnimRandomSwitch(const QString& id);
    virtual ~AnimRandomSwitch() override;

    virtual const AnimPoseVec& evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, AnimVariantMap& triggersOut) override;

    void setCurrentStateVar(QString& currentStateVar) { _currentStateVar = currentStateVar; }

protected:

    void setCurrentState(RandomSwitchState::Pointer randomState);
    void setTriggerRandomSwitchVar(const QString& triggerRandomSwitchVar) { _triggerRandomSwitchVar = triggerRandomSwitchVar; }
    void setRandomSwitchTimeMin(float randomSwitchTimeMin) { _randomSwitchTimeMin = randomSwitchTimeMin; }
    void setRandomSwitchTimeMax(float randomSwitchTimeMax) { _randomSwitchTimeMax = randomSwitchTimeMax; }
    void setTransitionVar(const QString& transitionVar) { _transitionVar = transitionVar; }
    void setTriggerTimeMin(float triggerTimeMin) { _triggerTimeMin = triggerTimeMin; }
    void setTriggerTimeMax(float triggerTimeMax) { _triggerTimeMax = triggerTimeMax; }

    void addState(RandomSwitchState::Pointer randomState);

    void switchRandomState(const AnimVariantMap& animVars, const AnimContext& context, RandomSwitchState::Pointer desiredState, bool shouldInterp);
    RandomSwitchState::Pointer evaluateTransitions(const AnimVariantMap& animVars) const;

    // for AnimDebugDraw rendering
    virtual const AnimPoseVec& getPosesInternal() const override;
    virtual void setActiveInternal(bool active) override;

    AnimPoseVec _poses;

    bool _triggerNewRandomState = false;
    // interpolation state
    bool _duringInterp = false;
    InterpType _interpType { InterpType::SnapshotPrev };
    EasingType _easingType { EasingType_Linear };
    float _alphaVel = 0.0f;
    float _alpha = 0.0f;
    AnimPoseVec _prevPoses;
    AnimPoseVec _nextPoses;

    RandomSwitchState::Pointer _currentState;
    RandomSwitchState::Pointer _previousState;
    std::vector<RandomSwitchState::Pointer> _randomStates;

    QString _currentStateVar;
    QString _triggerRandomSwitchVar;
    QString _transitionVar;
    float _triggerTimeMin { 10.0f };
    float _triggerTimeMax { 20.0f };
    float _triggerTime { 0.0f };
    float _randomSwitchTimeMin { 10.0f };
    float _randomSwitchTimeMax { 20.0f };
    float _randomSwitchTime { 0.0f };
    QString _lastPlayedState;

private:
    Q_DISABLE_COPY(AnimRandomSwitch)
};

#endif // hifi_AnimRandomSwitch_h
