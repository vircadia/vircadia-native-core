
#ifndef hifi_AnimStateMachine_h
#define hifi_AnimStateMachine_h

// AJT: post-pone state machine work.
#if 0
class AnimStateMachine : public AnimNode {
public:
    friend class AnimDebugDraw;

    AnimStateMachine(const std::string& id, float alpha);
    virtual ~AnimStateMachine() override;

    virtual const AnimPoseVec& evaluate(const AnimVariantMap& animVars, float dt) override;

    class AnimTransition;

    class AnimState {
    public:
        typedef std::shared_ptr<AnimState> Pointer;
        AnimState(const std::string& name, AnimNode::Pointer node, float interpFrame, float interpDuration) :
            _name(name),
            _node(node),
            _interpFrame(interpFrame),
            _interpDuration(interpDuration) {}
    protected:
        std::string _name;
        AnimNode::Pointer _node;
        float _interpFrame;
        float _interpDuration;
        std::vector<AnimTransition> _transitions;
    private:
        // no copies
        AnimState(const AnimState&) = delete;
        AnimState& operator=(const AnimState&) = delete;
    };

    class AnimTransition {
        AnimTransition(const std::string& trigger, AnimState::Pointer state) : _trigger(trigger), _state(state) {}
    protected:
        std::string _trigger;
        AnimState::Pointer _state;
    };

    void addState(AnimState::Pointer state);
    void removeState(AnimState::Pointer state);
    void setCurrentState(AnimState::Pointer state);

protected:
    // for AnimDebugDraw rendering
    virtual const AnimPoseVec& getPosesInternal() const override;

    AnimPoseVec _poses;

    std::vector<AnimState::Pointer> _states;
    AnimState::Pointer _currentState;
    AnimState::Pointer _defaultState;

private:
    // no copies
    AnimStateMachine(const AnimBlendLinear&) = delete;
    AnimStateMachine& operator=(const AnimBlendLinear&) = delete;
};
#endif

#endif // hifi_AnimBlendLinear_h
