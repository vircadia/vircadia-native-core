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

#ifndef __hifi__Rig__
#define __hifi__Rig__

#include <QObject>
#include <QMutex>
#include <QScriptValue>
#include <vector>
#include <JointData.h>

#include "AnimNode.h"
#include "AnimNodeLoader.h"
#include "SimpleMovingAverage.h"

class Rig;
typedef std::shared_ptr<Rig> RigPointer;

class Rig : public QObject, public std::enable_shared_from_this<Rig> {
public:
    struct StateHandler {
        AnimVariantMap results;
        QStringList propertyNames;
        QScriptValue function;
        bool useNames;
    };

    struct HeadParameters {
        float leanSideways = 0.0f; // degrees
        float leanForward = 0.0f; // degrees
        float torsoTwist = 0.0f; // degrees
        bool enableLean = false;
        glm::quat worldHeadOrientation = glm::quat();  // world space (-z forward)
        glm::quat localHeadOrientation = glm::quat();  // avatar space (-z forward)
        glm::vec3 localHeadPosition = glm::vec3();     // avatar space
        bool isInHMD = false;
        int leanJointIndex = -1;
        int neckJointIndex = -1;
        bool isTalking = false;
    };

    struct EyeParameters {
        glm::quat worldHeadOrientation = glm::quat();
        glm::vec3 eyeLookAt = glm::vec3();  // world space
        glm::vec3 eyeSaccade = glm::vec3(); // world space
        glm::vec3 modelTranslation = glm::vec3();
        glm::quat modelRotation = glm::quat();
        int leftEyeJointIndex = -1;
        int rightEyeJointIndex = -1;
    };

    struct HandParameters {
        bool isLeftEnabled;
        bool isRightEnabled;
        glm::vec3 leftPosition = glm::vec3();     // avatar space
        glm::quat leftOrientation = glm::quat();  // avatar space (z forward)
        glm::vec3 rightPosition = glm::vec3();    // avatar space
        glm::quat rightOrientation = glm::quat(); // avatar space (z forward)
        float leftTrigger = 0.0f;
        float rightTrigger = 0.0f;
    };

    virtual ~Rig() {}

    void destroyAnimGraph();
    void overrideAnimation(const QString& url, float fps, bool loop, float firstFrame, float lastFrame);
    void restoreAnimation();
    QStringList getAnimationRoles() const;
    void overrideRoleAnimation(const QString& role, const QString& url, float fps, bool loop, float firstFrame, float lastFrame);
    void restoreRoleAnimation(const QString& role);
    void prefetchAnimation(const QString& url);

    void initJointStates(const FBXGeometry& geometry, const glm::mat4& modelOffset);
    void reset(const FBXGeometry& geometry);
    bool jointStatesEmpty();
    int getJointStateCount() const;
    int indexOfJoint(const QString& jointName) const;

    void setModelOffset(const glm::mat4& modelOffset);

    bool getJointStateRotation(int index, glm::quat& rotation) const;
    bool getJointStateTranslation(int index, glm::vec3& translation) const;

    void clearJointState(int index);
    void clearJointStates();
    void clearJointAnimationPriority(int index);

    void setJointState(int index, bool valid, const glm::quat& rotation, const glm::vec3& translation, float priority);
    void setJointTranslation(int index, bool valid, const glm::vec3& translation, float priority);
    void setJointRotation(int index, bool valid, const glm::quat& rotation, float priority);

    void restoreJointRotation(int index, float fraction, float priority);
    void restoreJointTranslation(int index, float fraction, float priority);
    bool getJointPositionInWorldFrame(int jointIndex, glm::vec3& position,
                                      glm::vec3 translation, glm::quat rotation) const;

    bool getJointPosition(int jointIndex, glm::vec3& position) const;
    bool getJointRotationInWorldFrame(int jointIndex, glm::quat& result, const glm::quat& rotation) const;
    bool getJointRotation(int jointIndex, glm::quat& rotation) const;
    bool getJointTranslation(int jointIndex, glm::vec3& translation) const;
    bool getJointCombinedRotation(int jointIndex, glm::quat& result, const glm::quat& rotation) const;
    glm::mat4 getJointTransform(int jointIndex) const;
    // Start or stop animations as needed.
    void computeMotionAnimationState(float deltaTime, const glm::vec3& worldPosition, const glm::vec3& worldVelocity, const glm::quat& worldRotation);
    // Regardless of who started the animations or how many, update the joints.
    void updateAnimations(float deltaTime, glm::mat4 rootTransform);
    void inverseKinematics(int endIndex, glm::vec3 targetPosition, const glm::quat& targetRotation, float priority,
                           const QVector<int>& freeLineage, glm::mat4 rootTransform);
    bool restoreJointPosition(int jointIndex, float fraction, float priority, const QVector<int>& freeLineage);
    float getLimbLength(int jointIndex, const QVector<int>& freeLineage,
                        const glm::vec3 scale, const QVector<FBXJoint>& fbxJoints) const;

    glm::quat setJointRotationInBindFrame(int jointIndex, const glm::quat& rotation, float priority);
    glm::vec3 getJointDefaultTranslationInConstrainedFrame(int jointIndex);
    glm::quat setJointRotationInConstrainedFrame(int jointIndex, glm::quat targetRotation,
                                                 float priority, float mix = 1.0f);
    bool getJointRotationInConstrainedFrame(int jointIndex, glm::quat& rotOut) const;
    glm::quat getJointDefaultRotationInParentFrame(int jointIndex);
    void clearJointStatePriorities();

    void updateFromHeadParameters(const HeadParameters& params, float dt);
    void updateFromEyeParameters(const EyeParameters& params);
    void updateFromHandParameters(const HandParameters& params, float dt);

    void initAnimGraph(const QUrl& url);

    AnimNode::ConstPointer getAnimNode() const { return _animNode; }
    AnimSkeleton::ConstPointer getAnimSkeleton() const { return _animSkeleton; }
    QScriptValue addAnimationStateHandler(QScriptValue handler, QScriptValue propertiesList);
    void removeAnimationStateHandler(QScriptValue handler);
    void animationStateHandlerResult(int identifier, QScriptValue result);

    bool getModelRegistrationPoint(glm::vec3& modelRegistrationPointOut) const;

    const glm::vec3& getEyesInRootFrame() const { return _eyesInRootFrame; }

    AnimPose getAbsoluteDefaultPose(int index) const;  // avatar space

    void copyJointsIntoJointData(QVector<JointData>& jointDataVec) const;
    void copyJointsFromJointData(const QVector<JointData>& jointDataVec);

 protected:
    bool isValid(int index) const { return _animSkeleton && index >= 0 && index < _animSkeleton->getNumJoints(); }
    void updateAnimationStateHandlers();
    void applyOverridePoses();
    void buildAbsolutePoses();

    void updateLeanJoint(int index, float leanSideways, float leanForward, float torsoTwist);
    void updateNeckJoint(int index, const HeadParameters& params);
    void computeHeadNeckAnimVars(const AnimPose& hmdPose, glm::vec3& headPositionOut, glm::quat& headOrientationOut,
                                 glm::vec3& neckPositionOut, glm::quat& neckOrientationOut) const;
    void updateEyeJoint(int index, const glm::vec3& modelTranslation, const glm::quat& modelRotation, const glm::quat& worldHeadOrientation, const glm::vec3& lookAt, const glm::vec3& saccade);
    void calcAnimAlpha(float speed, const std::vector<float>& referenceSpeeds, float* alphaOut) const;

    void computeEyesInRootFrame(const AnimPoseVec& poses);

    AnimPose _modelOffset;  // model to avatar space (without 180 flip)
    AnimPose _geometryOffset; // geometry to model space (includes unit offset & fst offsets)
    AnimPoseVec _relativePoses; // geometry space relative to parent.
    AnimPoseVec _absolutePoses; // avatar space, not relative to parent.
    AnimPoseVec _overridePoses; // geometry space relative to parent.
    std::vector<bool> _overrideFlags;

    glm::mat4 _geometryToRigTransform;
    glm::mat4 _rigToGeometryTransform;

    int _rootJointIndex { -1 };

    int _leftHandJointIndex { -1 };
    int _leftElbowJointIndex { -1 };
    int _leftShoulderJointIndex { -1 };

    int _rightHandJointIndex { -1 };
    int _rightElbowJointIndex { -1 };
    int _rightShoulderJointIndex { -1 };

    glm::vec3 _lastFront;
    glm::vec3 _lastPosition;
    glm::vec3 _lastVelocity;
    glm::vec3 _eyesInRootFrame { Vectors::ZERO };

    QUrl _animGraphURL;
    std::shared_ptr<AnimNode> _animNode;
    std::shared_ptr<AnimSkeleton> _animSkeleton;
    std::unique_ptr<AnimNodeLoader> _animLoader;
    AnimVariantMap _animVars;
    enum class RigRole {
        Idle = 0,
        Turn,
        Move
    };
    RigRole _state { RigRole::Idle };
    RigRole _desiredState { RigRole::Idle };
    float _desiredStateAge { 0.0f };
    enum class UserAnimState {
        None = 0,
        A,
        B
    };
    UserAnimState _userAnimState { UserAnimState::None };
    QString _currentUserAnimURL;
    float _leftHandOverlayAlpha { 0.0f };
    float _rightHandOverlayAlpha { 0.0f };

    SimpleMovingAverage _averageForwardSpeed { 10 };
    SimpleMovingAverage _averageLateralSpeed { 10 };

    std::map<QString, AnimNode::Pointer> _origRoleAnimations;
    std::vector<AnimNode::Pointer> _prefetchedAnimations;

private:
    QMap<int, StateHandler> _stateHandlers;
    int _nextStateHandlerId { 0 };
    QMutex _stateMutex;
};

#endif /* defined(__hifi__Rig__) */
