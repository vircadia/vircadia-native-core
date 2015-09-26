//
//  Rig.h
//  libraries/animation/src/
//
//  Produces animation data and hip placement for the current timestamp.
//
//  Created by Howard Stearns, Seth Alves, Anthony Thibault, Andrew Meadows on 7/15/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/*
 Things we want to be able to do, that I think we cannot do now:
 * Stop an animation at a given time so that it can be examined visually or in a test harness. (I think we can already stop animation and set frame to a computed float? But does that move the bones?)
 * Play two animations, blending between them. (Current structure just has one, under script control.)
 * Fade in an animation over another.
 * Apply IK, lean, head pointing or other overrides relative to previous position.
 All of this depends on coordinated state.

 TBD:
 - What are responsibilities of Animation/AnimationPointer/AnimationCache/AnimationDetails/AnimationObject/AnimationLoop?
    Is there common/copied code (e.g., ScriptableAvatar::update)?
 - How do attachments interact with the physics of the attached entity? E.g., do hand joints need to reflect held object
    physics?
 - Is there any current need (i.e., for initial campatability) to have multiple animations per role (e.g., idle) with the
    system choosing randomly?

 - Distribute some doc from here to the right files if it turns out to be correct:
    - AnimationDetails is a script-useable copy of animation state, analogous to EntityItemProperties, but without anything
       equivalent to editEntity.
      But what's the intended difference vs AnimationObjection? Maybe AnimationDetails is to Animation as AnimationObject
       is to AnimationPointer?
 */

#ifndef __hifi__Rig__
#define __hifi__Rig__

#include <QObject>

#include "JointState.h"  // We might want to change this (later) to something that doesn't depend on gpu, fbx and model. -HRS

#include "AnimNode.h"
#include "AnimNodeLoader.h"

class AnimationHandle;
typedef std::shared_ptr<AnimationHandle> AnimationHandlePointer;

class Rig;
typedef std::shared_ptr<Rig> RigPointer;

class Rig : public QObject, public std::enable_shared_from_this<Rig> {
public:

    struct HeadParameters {
        float leanSideways = 0.0f; // degrees
        float leanForward = 0.0f; // degrees
        float torsoTwist = 0.0f; // degrees
        bool enableLean = false;
        glm::quat modelRotation = glm::quat();
        glm::quat localHeadOrientation = glm::quat();
        float localHeadPitch = 0.0f; // degrees
        float localHeadYaw = 0.0f; // degrees
        float localHeadRoll = 0.0f; // degrees
        glm::vec3 localHeadPosition = glm::vec3();
        bool isInHMD = false;
        glm::quat worldHeadOrientation = glm::quat();
        glm::vec3 eyeLookAt = glm::vec3();  // world space
        glm::vec3 eyeSaccade = glm::vec3(); // world space
        glm::vec3 modelTranslation = glm::vec3();
        int leanJointIndex = -1;
        int neckJointIndex = -1;
        int leftEyeJointIndex = -1;
        int rightEyeJointIndex = -1;
        bool isTalking = false;

        void dump() const;
    };

    struct HandParameters {
        bool isLeftEnabled;
        bool isRightEnabled;
        glm::vec3 leftPosition = glm::vec3();
        glm::quat leftOrientation = glm::quat();
        glm::vec3 rightPosition = glm::vec3();
        glm::quat rightOrientation = glm::quat();
        float leftTrigger = 0.0f;
        float rightTrigger = 0.0f;
    };

    virtual ~Rig() {}

    RigPointer getRigPointer() { return shared_from_this(); }

    AnimationHandlePointer createAnimationHandle();
    void removeAnimationHandle(const AnimationHandlePointer& handle);
    bool removeRunningAnimation(AnimationHandlePointer animationHandle);
    void addRunningAnimation(AnimationHandlePointer animationHandle);
    bool isRunningAnimation(AnimationHandlePointer animationHandle);
    bool isRunningRole(const QString& role); // There can be multiple animations per role, so this is more general than isRunningAnimation.
    const QList<AnimationHandlePointer>& getRunningAnimations() const { return _runningAnimations; }
    void deleteAnimations();
    void destroyAnimGraph();
    const QList<AnimationHandlePointer>& getAnimationHandles() const { return _animationHandles; }
    void startAnimation(const QString& url, float fps = 30.0f, float priority = 1.0f, bool loop = false,
                        bool hold = false, float firstFrame = 0.0f, float lastFrame = FLT_MAX, const QStringList& maskedJoints = QStringList());
    void stopAnimation(const QString& url);
    void startAnimationByRole(const QString& role, const QString& url = QString(), float fps = 30.0f,
                              float priority = 1.0f, bool loop = false, bool hold = false, float firstFrame = 0.0f,
                              float lastFrame = FLT_MAX, const QStringList& maskedJoints = QStringList());
    void stopAnimationByRole(const QString& role);
    AnimationHandlePointer addAnimationByRole(const QString& role, const QString& url = QString(), float fps = 30.0f,
                                              float priority = 1.0f, bool loop = false, bool hold = false, float firstFrame = 0.0f,
                                              float lastFrame = FLT_MAX, const QStringList& maskedJoints = QStringList(), bool startAutomatically = false);

    void initJointStates(QVector<JointState> states, glm::mat4 rootTransform,
                         int rootJointIndex,
                         int leftHandJointIndex,
                         int leftElbowJointIndex,
                         int leftShoulderJointIndex,
                         int rightHandJointIndex,
                         int rightElbowJointIndex,
                         int rightShoulderJointIndex);
    bool jointStatesEmpty() { return _jointStates.isEmpty(); };
    int getJointStateCount() const { return _jointStates.size(); }
    int indexOfJoint(const QString& jointName) ;

    void initJointTransforms(glm::mat4 rootTransform);
    void clearJointTransformTranslation(int jointIndex);
    void reset(const QVector<FBXJoint>& fbxJoints);
    bool getJointStateRotation(int index, glm::quat& rotation) const;
    void applyJointRotationDelta(int jointIndex, const glm::quat& delta, bool constrain, float priority);
    JointState getJointState(int jointIndex) const; // XXX
    bool getVisibleJointState(int index, glm::quat& rotation) const;
    void clearJointState(int index);
    void clearJointStates();
    void clearJointAnimationPriority(int index);
    float getJointAnimatinoPriority(int index);
    void setJointAnimatinoPriority(int index, float newPriority);
    void setJointState(int index, bool valid, const glm::quat& rotation, float priority);
    void restoreJointRotation(int index, float fraction, float priority);
    bool getJointPositionInWorldFrame(int jointIndex, glm::vec3& position,
                                      glm::vec3 translation, glm::quat rotation) const;

    bool getJointPosition(int jointIndex, glm::vec3& position) const;
    bool getJointRotationInWorldFrame(int jointIndex, glm::quat& result, const glm::quat& rotation) const;
    bool getJointRotation(int jointIndex, glm::quat& rotation) const;
    bool getJointCombinedRotation(int jointIndex, glm::quat& result, const glm::quat& rotation) const;
    bool getVisibleJointPositionInWorldFrame(int jointIndex, glm::vec3& position,
                                             glm::vec3 translation, glm::quat rotation) const;
    bool getVisibleJointRotationInWorldFrame(int jointIndex, glm::quat& result, glm::quat rotation) const;
    glm::mat4 getJointTransform(int jointIndex) const;
    glm::mat4 getJointVisibleTransform(int jointIndex) const;
    void setJointVisibleTransform(int jointIndex, glm::mat4 newTransform);
    // Start or stop animations as needed.
    void computeMotionAnimationState(float deltaTime, const glm::vec3& worldPosition, const glm::vec3& worldVelocity, const glm::quat& worldRotation);
    // Regardless of who started the animations or how many, update the joints.
    void updateAnimations(float deltaTime, glm::mat4 rootTransform);
    bool setJointPosition(int jointIndex, const glm::vec3& position, const glm::quat& rotation, bool useRotation,
                          int lastFreeIndex, bool allIntermediatesFree, const glm::vec3& alignment, float priority,
                          const QVector<int>& freeLineage, glm::mat4 rootTransform);
    void inverseKinematics(int endIndex, glm::vec3 targetPosition, const glm::quat& targetRotation, float priority,
                           const QVector<int>& freeLineage, glm::mat4 rootTransform);
    bool restoreJointPosition(int jointIndex, float fraction, float priority, const QVector<int>& freeLineage);
    float getLimbLength(int jointIndex, const QVector<int>& freeLineage,
                        const glm::vec3 scale, const QVector<FBXJoint>& fbxJoints) const;

    glm::quat setJointRotationInBindFrame(int jointIndex, const glm::quat& rotation, float priority, bool constrain = false);
    glm::vec3 getJointDefaultTranslationInConstrainedFrame(int jointIndex);
    glm::quat setJointRotationInConstrainedFrame(int jointIndex, glm::quat targetRotation,
                                                 float priority, bool constrain = false, float mix = 1.0f);
    bool getJointRotationInConstrainedFrame(int jointIndex, glm::quat& rotOut) const;
    glm::quat getJointDefaultRotationInParentFrame(int jointIndex);
    void updateVisibleJointStates();

    virtual void updateJointState(int index, glm::mat4 rootTransform) = 0;

    void setEnableRig(bool isEnabled) { _enableRig = isEnabled; }
    bool getEnableRig() const { return _enableRig; }
    void setEnableAnimGraph(bool isEnabled) { _enableAnimGraph = isEnabled; }
    bool getEnableAnimGraph() const { return _enableAnimGraph; }

    void updateFromHeadParameters(const HeadParameters& params, float dt);
    void updateEyeJoints(int leftEyeIndex, int rightEyeIndex, const glm::vec3& modelTranslation, const glm::quat& modelRotation,
                         const glm::quat& worldHeadOrientation, const glm::vec3& lookAtSpot, const glm::vec3& saccade = glm::vec3(0.0f));

    void updateFromHandParameters(const HandParameters& params, float dt);

    virtual void setHandPosition(int jointIndex, const glm::vec3& position, const glm::quat& rotation,
                                 float scale, float priority) = 0;

    void initAnimGraph(const QUrl& url, const FBXGeometry& fbxGeometry);

    AnimNode::ConstPointer getAnimNode() const { return _animNode; }
    AnimSkeleton::ConstPointer getAnimSkeleton() const { return _animSkeleton; }
    bool disableHands {false}; // should go away with rig animation (and Rig::inverseKinematics)

 protected:

    void updateLeanJoint(int index, float leanSideways, float leanForward, float torsoTwist);
    void updateNeckJoint(int index, const HeadParameters& params);
    void updateEyeJoint(int index, const glm::vec3& modelTranslation, const glm::quat& modelRotation, const glm::quat& worldHeadOrientation, const glm::vec3& lookAt, const glm::vec3& saccade);

    QVector<JointState> _jointStates;
    int _rootJointIndex = -1;

    int _leftHandJointIndex = -1;
    int _leftElbowJointIndex = -1;
    int _leftShoulderJointIndex = -1;

    int _rightHandJointIndex = -1;
    int _rightElbowJointIndex = -1;
    int _rightShoulderJointIndex = -1;

    QList<AnimationHandlePointer> _animationHandles;
    QList<AnimationHandlePointer> _runningAnimations;

    bool _enableRig = false;
    bool _enableAnimGraph = false;
    glm::vec3 _lastFront;
    glm::vec3 _lastPosition;

    std::shared_ptr<AnimNode> _animNode;
    std::shared_ptr<AnimSkeleton> _animSkeleton;
    std::unique_ptr<AnimNodeLoader> _animLoader;
    AnimVariantMap _animVars;
    enum class RigRole {
        Idle = 0,
        Turn,
        Move
    };
    RigRole _state = RigRole::Idle;
    float _leftHandOverlayAlpha = 0.0f;
    float _rightHandOverlayAlpha = 0.0f;
};

#endif /* defined(__hifi__Rig__) */
