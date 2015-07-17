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

#ifndef __hifi__Rig__
#define __hifi__Rig__

#include <QObject>

#include "JointState.h"

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

    void initJointStates(glm::vec3 scale, glm::vec3 offset, QVector<JointState> states);
    void initJointTransforms(glm::vec3 scale, glm::vec3 offset);
    void resetJoints();

    QVector<JointState> getJointStates() { return _jointStates; }

    AnimationHandlePointer createAnimationHandle();

protected:
    QVector<JointState> _jointStates;

    QSet<AnimationHandlePointer> _animationHandles;
    QList<AnimationHandlePointer> _runningAnimations;
};

#endif /* defined(__hifi__Rig__) */
