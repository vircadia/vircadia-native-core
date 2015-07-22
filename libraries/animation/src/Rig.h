//
//  Rig.h
//  libraries/script-engine/src/
//
//  Produces animation data and hip placement for the current timestamp.
//
//  Created by Howard Stearns, Seth Alves, Anthony Thibault, Andrew Meadows on 7/15/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/* TBD:
 - What is responsibilities of Animation/AnimationPointer/AnimationCache/AnimationDetails?  Is there common/copied code (e.g., ScriptableAvatar::update)?
 - How do attachments interact with the physics of the attached entity? E.g., do hand joints need to reflect held object physics?
 - Is there any current need (i.e., for initial campatability) to have multiple animations per role (e.g., idle) with the system choosing randomly?
 
 - Distribute some doc from here to the right files if it turns out to be correct:
    - AnimationDetails is a script-useable copy of animation state, analogous to EntityItemProperties, but without anything equivalent to editEntity.
 */

#ifndef __hifi__Rig__
#define __hifi__Rig__

#include <QObject>

#include "JointState.h"  // We might want to change this (later) to something that doesn't depend on gpu, fbx and model. -HRS

class AnimationHandle;
typedef std::shared_ptr<AnimationHandle> AnimationHandlePointer;
// typedef QWeakPointer<AnimationHandle> WeakAnimationHandlePointer;

class Rig;
typedef std::shared_ptr<Rig> RigPointer;


class Rig : public QObject, public std::enable_shared_from_this<Rig> {

public:
    RigPointer getRigPointer() { return shared_from_this(); }

    bool removeRunningAnimation(AnimationHandlePointer animationHandle);
    void addRunningAnimation(AnimationHandlePointer animationHandle);
    bool isRunningAnimation(AnimationHandlePointer animationHandle);
    const QList<AnimationHandlePointer>& getRunningAnimations() const { return _runningAnimations; }

    float initJointStates(glm::vec3 scale, glm::vec3 offset, QVector<JointState> states);
    void initJointTransforms(glm::vec3 scale, glm::vec3 offset);
    void resetJoints();
    bool jointStatesEmpty() { return _jointStates.isEmpty(); };
    int jointStateCount() const { return _jointStates.size(); }
    bool getJointStateAtIndex(int jointIndex, JointState& jointState) const;

    void updateJointStates(glm::mat4 parentTransform);
    void updateJointState(int index, glm::mat4 parentTransform);
    void resetAllTransformsChanged();

    bool getJointState(int index, glm::quat& rotation) const;
    bool getVisibleJointState(int index, glm::quat& rotation) const;
    void updateVisibleJointStates();
    void clearJointState(int index);
    void clearJointStates();
    void setJointState(int index, bool valid, const glm::quat& rotation, float priority);
    void clearJointAnimationPriority(int index);
    glm::quat setJointRotationInBindFrame(int jointIndex, const glm::quat& rotation, float priority, bool constrain = false);
    glm::quat setJointRotationInConstrainedFrame(int jointIndex, glm::quat targetRotation,
                                                 float priority, bool constrain = false);
    void applyJointRotationDelta(int jointIndex, const glm::quat& delta, bool constrain, float priority);

    QVector<JointState> getJointStates() { return _jointStates; }

    AnimationHandlePointer createAnimationHandle();

    bool setJointPosition(int jointIndex, const glm::vec3& position, const glm::quat& rotation,
                          bool useRotation, int lastFreeIndex, bool allIntermediatesFree, const glm::vec3& alignment,
                          float priority, glm::mat4 parentTransform);
    void inverseKinematics(int endIndex, glm::vec3 targetPosition,
                           const glm::quat& targetRotation, float priority, glm::mat4 parentTransform);
    bool restoreJointPosition(int jointIndex, float fraction, float priority);
    float getLimbLength(int jointIndex, glm::vec3 scale) const;

protected:
    QVector<JointState> _jointStates;

    QSet<AnimationHandlePointer> _animationHandles;
    QList<AnimationHandlePointer> _runningAnimations;
};

#endif /* defined(__hifi__Rig__) */
