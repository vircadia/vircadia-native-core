//
//  Rig.cpp
//  libraries/animation/src/
//
//  Created by Howard Stearns, Seth Alves, Anthony Thibault, Andrew Meadows on 7/15/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Rig.h"

#include <glm/gtx/vector_angle.hpp>
#include <queue>

#include "AnimationHandle.h"
#include "AnimationLogging.h"
#include "AnimSkeleton.h"

#include "Rig.h"

void Rig::HeadParameters::dump() const {
    qCDebug(animation, "HeadParameters =");
    qCDebug(animation, "    leanSideways = %0.5f", (double)leanSideways);
    qCDebug(animation, "    leanForward = %0.5f", (double)leanForward);
    qCDebug(animation, "    torsoTwist = %0.5f", (double)torsoTwist);
    glm::vec3 axis = glm::axis(localHeadOrientation);
    float theta = glm::angle(localHeadOrientation);
    qCDebug(animation, "    localHeadOrientation axis = (%.5f, %.5f, %.5f), theta = %0.5f", (double)axis.x, (double)axis.y, (double)axis.z, (double)theta);
    axis = glm::axis(worldHeadOrientation);
    theta = glm::angle(worldHeadOrientation);
    qCDebug(animation, "    worldHeadOrientation axis = (%.5f, %.5f, %.5f), theta = %0.5f", (double)axis.x, (double)axis.y, (double)axis.z, (double)theta);
    axis = glm::axis(modelRotation);
    theta = glm::angle(modelRotation);
    qCDebug(animation, "    modelRotation axis = (%.5f, %.5f, %.5f), theta = %0.5f", (double)axis.x, (double)axis.y, (double)axis.z, (double)theta);
    qCDebug(animation, "    modelTranslation = (%.5f, %.5f, %.5f)", (double)modelTranslation.x, (double)modelTranslation.y, (double)modelTranslation.z);
    qCDebug(animation, "    eyeLookAt = (%.5f, %.5f, %.5f)", (double)eyeLookAt.x, (double)eyeLookAt.y, (double)eyeLookAt.z);
    qCDebug(animation, "    eyeSaccade = (%.5f, %.5f, %.5f)", (double)eyeSaccade.x, (double)eyeSaccade.y, (double)eyeSaccade.z);
    qCDebug(animation, "    leanJointIndex = %.d", leanJointIndex);
    qCDebug(animation, "    neckJointIndex = %.d", neckJointIndex);
    qCDebug(animation, "    leftEyeJointIndex = %.d", leftEyeJointIndex);
    qCDebug(animation, "    rightEyeJointIndex = %.d", rightEyeJointIndex);
}

void insertSorted(QList<AnimationHandlePointer>& handles, const AnimationHandlePointer& handle) {
    for (QList<AnimationHandlePointer>::iterator it = handles.begin(); it != handles.end(); it++) {
        if (handle->getPriority() > (*it)->getPriority()) {
            handles.insert(it, handle);
            return;
        }
    }
    handles.append(handle);
}

AnimationHandlePointer Rig::createAnimationHandle() {
    AnimationHandlePointer handle(new AnimationHandle(getRigPointer()));
    _animationHandles.append(handle);
    return handle;
}
void Rig::removeAnimationHandle(const AnimationHandlePointer& handle) {
    handle->stop();
    // FIXME? Do we need to also animationHandle->clearJoints()? deleteAnimations(), below, was first written to do so, but did not first stop it.
    _animationHandles.removeOne(handle);
}

void Rig::startAnimation(const QString& url, float fps, float priority,
                              bool loop, bool hold, float firstFrame, float lastFrame, const QStringList& maskedJoints) {
    // This is different than startAnimationByRole, in which we use the existing values if the animation already exists.
    // Here we reuse the animation handle if possible, but in any case, we set the values to those given (or defaulted).
    AnimationHandlePointer handle = nullptr;
    foreach (const AnimationHandlePointer& candidate, _animationHandles) {
        if (candidate->getURL() == url) {
            handle = candidate;
        }
    }
    if (!handle) {
        handle = createAnimationHandle();
        handle->setURL(url);
    }
    handle->setFade(1.0f);     // If you want to fade, use the startAnimationByRole system.
    handle->setFPS(fps);
    handle->setPriority(priority);
    handle->setLoop(loop);
    handle->setHold(hold);
    handle->setFirstFrame(firstFrame);
    handle->setLastFrame(lastFrame);
    handle->setMaskedJoints(maskedJoints);
    handle->start();
}

AnimationHandlePointer Rig::addAnimationByRole(const QString& role, const QString& url, float fps, float priority,
                                               bool loop, bool hold, float firstFrame, float lastFrame, const QStringList& maskedJoints, bool startAutomatically) {
    // check for a configured animation for the role
    //qCDebug(animation) << "addAnimationByRole" << role << url << fps << priority << loop << hold << firstFrame << lastFrame << maskedJoints << startAutomatically;
    foreach (const AnimationHandlePointer& candidate, _animationHandles) {
        if (candidate->getRole() == role) {
            if (startAutomatically) {
                candidate->start();
            }
            return candidate;
        }
    }
    AnimationHandlePointer handle = createAnimationHandle();
    QString standard = "";
    if (url.isEmpty()) {  // Default animations for fight club
        const QString& base = "https://hifi-public.s3.amazonaws.com/ozan/anim/standard_anims/";
        if (role == "walk") {
            standard = base + "walk_fwd.fbx";
        } else if (role == "backup") {
            standard = base + "walk_bwd.fbx";
        } else if (role == "leftTurn") {
            standard = base + "turn_left.fbx";
        } else if (role == "rightTurn") {
            standard = base + "turn_right.fbx";
        } else if (role == "leftStrafe") {
            standard = base + "strafe_left.fbx";
        } else if (role == "rightStrafe") {
            standard = base + "strafe_right.fbx";
        } else if (role == "idle") {
            standard = base + "idle.fbx";
            fps = 25.0f;
        }
        if (!standard.isEmpty()) {
            loop = true;
        }
    }
    handle->setRole(role);
    handle->setURL(url.isEmpty() ? standard : url);
    handle->setFPS(fps);
    handle->setPriority(priority);
    handle->setLoop(loop);
    handle->setHold(hold);
    handle->setFirstFrame(firstFrame);
    handle->setLastFrame(lastFrame);
    handle->setMaskedJoints(maskedJoints);
    if (startAutomatically) {
        handle->start();
    }
    return handle;
}
void Rig::startAnimationByRole(const QString& role, const QString& url, float fps, float priority,
                               bool loop, bool hold, float firstFrame, float lastFrame, const QStringList& maskedJoints) {
    AnimationHandlePointer handle = addAnimationByRole(role, url, fps, priority, loop, hold, firstFrame, lastFrame, maskedJoints, true);
    handle->setFadePerSecond(1.0f); // For now. Could be individualized later.
}

void Rig::stopAnimationByRole(const QString& role) {
    foreach (const AnimationHandlePointer& handle, getRunningAnimations()) {
        if (handle->getRole() == role) {
            handle->setFadePerSecond(-1.0f); // For now. Could be individualized later.
        }
    }
}

void Rig::stopAnimation(const QString& url) {
     foreach (const AnimationHandlePointer& handle, getRunningAnimations()) {
        if (handle->getURL() == url) {
            handle->setFade(0.0f); // right away. Will be remove during updateAnimations, without locking
            handle->setFadePerSecond(-1.0f); // so that the updateAnimation code notices
        }
    }
}

bool Rig::removeRunningAnimation(AnimationHandlePointer animationHandle) {
    return _runningAnimations.removeOne(animationHandle);
}

void Rig::addRunningAnimation(AnimationHandlePointer animationHandle) {
    insertSorted(_runningAnimations, animationHandle);
}

bool Rig::isRunningAnimation(AnimationHandlePointer animationHandle) {
    return _runningAnimations.contains(animationHandle);
}
bool Rig::isRunningRole(const QString& role) {  //obviously, there are more efficient ways to do this
    for (auto animation : _runningAnimations) {
        if ((animation->getRole() == role) && (animation->getFadePerSecond() >= 0.0f)) { // Don't count those being faded out
            return true;
        }
    }
    return false;
}

void Rig::deleteAnimations() {
    for (auto animation : _animationHandles) {
        removeAnimationHandle(animation);
    }
    _animationHandles.clear();
}

void Rig::destroyAnimGraph() {
    _animSkeleton = nullptr;
    _animLoader = nullptr;
    _animNode = nullptr;
}

void Rig::initJointStates(QVector<JointState> states, glm::mat4 rootTransform,
                          int rootJointIndex,
                          int leftHandJointIndex,
                          int leftElbowJointIndex,
                          int leftShoulderJointIndex,
                          int rightHandJointIndex,
                          int rightElbowJointIndex,
                          int rightShoulderJointIndex) {
    _jointStates = states;

    _rootJointIndex = rootJointIndex;
    _leftHandJointIndex = leftHandJointIndex;
    _leftElbowJointIndex = leftElbowJointIndex;
    _leftShoulderJointIndex = leftShoulderJointIndex;
    _rightHandJointIndex = rightHandJointIndex;
    _rightElbowJointIndex = rightElbowJointIndex;
    _rightShoulderJointIndex = rightShoulderJointIndex;

    initJointTransforms(rootTransform);

    int numStates = _jointStates.size();
    for (int i = 0; i < numStates; ++i) {
        _jointStates[i].buildConstraint();
    }
    for (int i = 0; i < _jointStates.size(); i++) {
        _jointStates[i].slaveVisibleTransform();
    }
}

// We could build and cache a dictionary, too....
// Should we be using .fst mapping instead/also?
int Rig::indexOfJoint(const QString& jointName) {
    for (int i = 0; i < _jointStates.count(); i++) {
        if (_jointStates[i].getName() == jointName) {
            return i;
        }
    }
    return -1;
}


void Rig::initJointTransforms(glm::mat4 rootTransform) {
    // compute model transforms
    int numStates = _jointStates.size();
    for (int i = 0; i < numStates; ++i) {
        JointState& state = _jointStates[i];
        int parentIndex = state.getParentIndex();
        if (parentIndex == -1) {
            state.initTransform(rootTransform);
        } else {
            const JointState& parentState = _jointStates.at(parentIndex);
            state.initTransform(parentState.getTransform());
        }
    }
}

void Rig::clearJointTransformTranslation(int jointIndex) {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return;
    }
    _jointStates[jointIndex].clearTransformTranslation();
}

void Rig::reset(const QVector<FBXJoint>& fbxJoints) {
    if (_jointStates.isEmpty()) {
        return;
    }
    for (int i = 0; i < _jointStates.size(); i++) {
        _jointStates[i].setRotationInConstrainedFrame(fbxJoints.at(i).rotation, 0.0f);
    }
}

JointState Rig::getJointState(int jointIndex) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return JointState();
    }
    return _jointStates[jointIndex];
}

bool Rig::getJointStateRotation(int index, glm::quat& rotation) const {
    if (index == -1 || index >= _jointStates.size()) {
        return false;
    }
    const JointState& state = _jointStates.at(index);
    rotation = state.getRotationInConstrainedFrame();
    return !state.rotationIsDefault(rotation);
}

bool Rig::getVisibleJointState(int index, glm::quat& rotation) const {
    if (index == -1 || index >= _jointStates.size()) {
        return false;
    }
    const JointState& state = _jointStates.at(index);
    rotation = state.getVisibleRotationInConstrainedFrame();
    return !state.rotationIsDefault(rotation);
}

void Rig::clearJointState(int index) {
    if (index != -1 && index < _jointStates.size()) {
        JointState& state = _jointStates[index];
        state.setRotationInConstrainedFrame(glm::quat(), 0.0f);
    }
}

void Rig::clearJointStates() {
    _jointStates.clear();
}

void Rig::clearJointAnimationPriority(int index) {
    if (index != -1 && index < _jointStates.size()) {
        _jointStates[index].setAnimationPriority(0.0f);
    }
}

float Rig::getJointAnimatinoPriority(int index) {
    if (index != -1 && index < _jointStates.size()) {
        return _jointStates[index].getAnimationPriority();
    }
    return 0.0f;
}

void Rig::setJointAnimatinoPriority(int index, float newPriority) {
    if (index != -1 && index < _jointStates.size()) {
        _jointStates[index].setAnimationPriority(newPriority);
    }
}

void Rig::setJointState(int index, bool valid, const glm::quat& rotation, float priority) {
    if (index != -1 && index < _jointStates.size()) {
        JointState& state = _jointStates[index];
        if (valid) {
            state.setRotationInConstrainedFrame(rotation, priority);
        } else {
            state.restoreRotation(1.0f, priority);
        }
    }
}

void Rig::restoreJointRotation(int index, float fraction, float priority) {
    if (index != -1 && index < _jointStates.size()) {
        _jointStates[index].restoreRotation(fraction, priority);
    }
}

bool Rig::getJointPositionInWorldFrame(int jointIndex, glm::vec3& position,
                                       glm::vec3 translation, glm::quat rotation) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return false;
    }
    // position is in world-frame
    position = translation + rotation * _jointStates[jointIndex].getPosition();
    return true;
}

bool Rig::getJointPosition(int jointIndex, glm::vec3& position) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return false;
    }
    // position is in model-frame
    position = extractTranslation(_jointStates[jointIndex].getTransform());
    return true;
}

bool Rig::getJointRotationInWorldFrame(int jointIndex, glm::quat& result, const glm::quat& rotation) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return false;
    }
    result = rotation * _jointStates[jointIndex].getRotation();
    return true;
}

bool Rig::getJointRotation(int jointIndex, glm::quat& rotation) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return false;
    }
    rotation = _jointStates[jointIndex].getRotation();
    return true;
}

bool Rig::getJointCombinedRotation(int jointIndex, glm::quat& result, const glm::quat& rotation) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return false;
    }
    result = rotation * _jointStates[jointIndex].getRotation();
    return true;
}


bool Rig::getVisibleJointPositionInWorldFrame(int jointIndex, glm::vec3& position,
                                              glm::vec3 translation, glm::quat rotation) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return false;
    }
    // position is in world-frame
    position = translation + rotation * _jointStates[jointIndex].getVisiblePosition();
    return true;
}

bool Rig::getVisibleJointRotationInWorldFrame(int jointIndex, glm::quat& result, glm::quat rotation) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return false;
    }
    result = rotation * _jointStates[jointIndex].getVisibleRotation();
    return true;
}

glm::mat4 Rig::getJointTransform(int jointIndex) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return glm::mat4();
    }
    return _jointStates[jointIndex].getTransform();
}

glm::mat4 Rig::getJointVisibleTransform(int jointIndex) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return glm::mat4();
    }
    return _jointStates[jointIndex].getVisibleTransform();
}

void Rig::computeMotionAnimationState(float deltaTime, const glm::vec3& worldPosition, const glm::vec3& worldVelocity, const glm::quat& worldRotation) {

    glm::vec3 front = worldRotation * IDENTITY_FRONT;

    // It can be more accurate/smooth to use velocity rather than position,
    // but some modes (e.g., hmd standing) update position without updating velocity.
    // It's very hard to debug hmd standing. (Look down at yourself, or have a second person observe. HMD third person is a bit undefined...)
    // So, let's create our own workingVelocity from the worldPosition...
    glm::vec3 positionDelta = worldPosition - _lastPosition;
    glm::vec3 workingVelocity = positionDelta / deltaTime;

#if !WANT_DEBUG
    // But for smoothest (non-hmd standing) results, go ahead and use velocity:
    if (!positionDelta.x && !positionDelta.y && !positionDelta.z) {
        workingVelocity = worldVelocity;
    }
#endif

    if (_enableAnimGraph) {

        glm::vec3 localVel = glm::inverse(worldRotation) * workingVelocity;
        float forwardSpeed = glm::dot(localVel, IDENTITY_FRONT);
        float lateralSpeed = glm::dot(localVel, IDENTITY_RIGHT);
        float turningSpeed = glm::orientedAngle(front, _lastFront, IDENTITY_UP) / deltaTime;

        // sine wave LFO var for testing.
        static float t = 0.0f;
        _animVars.set("sine", static_cast<float>(0.5 * sin(t) + 0.5));

        // default anim vars to notMoving and notTurning
        _animVars.set("isMovingForward", false);
        _animVars.set("isMovingBackward", false);
        _animVars.set("isMovingLeft", false);
        _animVars.set("isMovingRight", false);
        _animVars.set("isNotMoving", true);
        _animVars.set("isTurningLeft", false);
        _animVars.set("isTurningRight", false);
        _animVars.set("isNotTurning", true);

        const float ANIM_WALK_SPEED = 1.4f; // m/s
        _animVars.set("walkTimeScale", glm::clamp(0.5f, 2.0f, glm::length(localVel) / ANIM_WALK_SPEED));

        const float MOVE_ENTER_SPEED_THRESHOLD = 0.2f; // m/sec
        const float MOVE_EXIT_SPEED_THRESHOLD = 0.07f;  // m/sec
        const float TURN_ENTER_SPEED_THRESHOLD = 0.5f; // rad/sec
        const float TURN_EXIT_SPEED_THRESHOLD = 0.2f; // rad/sec

        float moveThresh;
        if (_state != RigRole::Move) {
            moveThresh = MOVE_ENTER_SPEED_THRESHOLD;
        } else {
            moveThresh = MOVE_EXIT_SPEED_THRESHOLD;
        }

        float turnThresh;
        if (_state != RigRole::Turn) {
            turnThresh = TURN_ENTER_SPEED_THRESHOLD;
        } else {
            turnThresh = TURN_EXIT_SPEED_THRESHOLD;
        }

        if (glm::length(localVel) > moveThresh) {
            if (fabsf(forwardSpeed) > 0.5f * fabsf(lateralSpeed)) {
                if (forwardSpeed > 0.0f) {
                    // forward
                    _animVars.set("isMovingForward", true);
                    _animVars.set("isNotMoving", false);

                } else {
                    // backward
                    _animVars.set("isMovingBackward", true);
                    _animVars.set("isNotMoving", false);
                }
            } else {
                if (lateralSpeed > 0.0f) {
                    // right
                    _animVars.set("isMovingRight", true);
                    _animVars.set("isNotMoving", false);
                } else {
                    // left
                    _animVars.set("isMovingLeft", true);
                    _animVars.set("isNotMoving", false);
                }
            }
            _state = RigRole::Move;
        } else {
            if (fabsf(turningSpeed) > turnThresh) {
                if (turningSpeed > 0.0f) {
                    // turning right
                    _animVars.set("isTurningRight", true);
                    _animVars.set("isNotTurning", false);
                } else {
                    // turning left
                    _animVars.set("isTurningLeft", true);
                    _animVars.set("isNotTurning", false);
                }
                _state = RigRole::Turn;
            } else {
                // idle
                _state = RigRole::Idle;
            }
        }

        t += deltaTime;
    }

    if (_enableRig) {
        bool isMoving = false;

        glm::vec3 right = worldRotation * IDENTITY_RIGHT;
        const float PERCEPTIBLE_DELTA = 0.001f;
        const float PERCEPTIBLE_SPEED = 0.1f;

        // Note: Separately, we've arranged for starting/stopping animations by role (as we've done here) to pick up where they've left off when fading,
        // so that you wouldn't notice the start/stop if it happens fast enough (e.g., one frame). But the print below would still be noisy.

        float forwardSpeed = glm::dot(workingVelocity, front);
        float rightLateralSpeed = glm::dot(workingVelocity, right);
        float rightTurningDelta = glm::orientedAngle(front, _lastFront, IDENTITY_UP);
        float rightTurningSpeed = rightTurningDelta / deltaTime;
        bool isTurning = (std::abs(rightTurningDelta) > PERCEPTIBLE_DELTA) && (std::abs(rightTurningSpeed) > PERCEPTIBLE_SPEED);
        bool isStrafing = std::abs(rightLateralSpeed) > PERCEPTIBLE_SPEED;
        auto updateRole = [&](const QString& role, bool isOn) {
            isMoving = isMoving || isOn;
            if (isOn) {
                if (!isRunningRole(role)) {
                    qCDebug(animation) << "Rig STARTING" << role;
                    startAnimationByRole(role);

                }
            } else {
                if (isRunningRole(role)) {
                    qCDebug(animation) << "Rig stopping" << role;
                    stopAnimationByRole(role);
                }
            }
        };
        updateRole("walk",   forwardSpeed > PERCEPTIBLE_SPEED);
        updateRole("backup", forwardSpeed < -PERCEPTIBLE_SPEED);
        updateRole("rightTurn", isTurning && (rightTurningSpeed > 0.0f));
        updateRole("leftTurn",  isTurning && (rightTurningSpeed < 0.0f));
        isStrafing = isStrafing && !isMoving;
        updateRole("rightStrafe", isStrafing && (rightLateralSpeed > 0.0f));
        updateRole("leftStrafe",  isStrafing && (rightLateralSpeed < 0.0f));
        updateRole("idle", !isMoving); // Must be last, as it makes isMoving bogus.
    }

    _lastFront = front;
    _lastPosition = worldPosition;
}

void Rig::updateAnimations(float deltaTime, glm::mat4 rootTransform) {

    if (_enableAnimGraph) {
        if (!_animNode) {
            return;
        }

        // evaluate the animation
        AnimNode::Triggers triggersOut;
        AnimPoseVec poses = _animNode->evaluate(_animVars, deltaTime, triggersOut);
        _animVars.clearTriggers();
        for (auto& trigger : triggersOut) {
            _animVars.setTrigger(trigger);
        }

        // copy poses into jointStates
        const float PRIORITY = 1.0f;
        for (size_t i = 0; i < poses.size(); i++) {
            setJointRotationInConstrainedFrame((int)i, glm::inverse(_animSkeleton->getRelativeBindPose(i).rot) * poses[i].rot, PRIORITY, false);
        }

    } else {

        // First normalize the fades so that they sum to 1.0.
        // update the fade data in each animation (not normalized as they are an independent propert of animation)
        foreach (const AnimationHandlePointer& handle, _runningAnimations) {
            float fadePerSecond = handle->getFadePerSecond();
            float fade = handle->getFade();
            if (fadePerSecond != 0.0f) {
                fade += fadePerSecond * deltaTime;
                if ((0.0f >= fade) || (fade >= 1.0f)) {
                    fade = glm::clamp(fade, 0.0f, 1.0f);
                    handle->setFadePerSecond(0.0f);
                }
                handle->setFade(fade);
                if (fade <= 0.0f) { // stop any finished animations now
                    handle->setRunning(false, false); // but do not restore joints as it causes a flicker
                }
            }
        }
        // sum the remaining fade data
        float fadeTotal = 0.0f;
        foreach (const AnimationHandlePointer& handle, _runningAnimations) {
            fadeTotal += handle->getFade();
        }
        float fadeSumSoFar = 0.0f;
        foreach (const AnimationHandlePointer& handle, _runningAnimations) {
            handle->setPriority(1.0f);
            // if no fadeTotal, everyone's (typically just one running) is starting at zero. In that case, blend equally.
            float normalizedFade = (fadeTotal != 0.0f) ? (handle->getFade() / fadeTotal) : (1.0f / _runningAnimations.count());
            assert(normalizedFade != 0.0f);
            // simulate() will blend each animation result into the result so far, based on the pairwise mix at at each step.
            // i.e., slerp the 'mix' distance from the result so far towards this iteration's animation result.
            // The formula here for mix is based on the idea that, at each step:
            // fadeSum is to normalizedFade, as (1 - mix) is to mix
            // i.e., fadeSumSoFar/normalizedFade = (1 - mix)/mix
            // Then we solve for mix.
            // Sanity check: For the first animation, fadeSum = 0, and the mix will always be 1.
            // Sanity check: For equal blending, the formula is equivalent to mix = 1 / nAnimationsSoFar++
            float mix = 1.0f / ((fadeSumSoFar / normalizedFade) + 1.0f);
            assert((0.0f <= mix) && (mix <= 1.0f));
            fadeSumSoFar += normalizedFade;
            handle->setMix(mix);
            handle->simulate(deltaTime);
        }
    }

    for (int i = 0; i < _jointStates.size(); i++) {
        updateJointState(i, rootTransform);
    }
    for (int i = 0; i < _jointStates.size(); i++) {
        _jointStates[i].resetTransformChanged();
    }
}

bool Rig::setJointPosition(int jointIndex, const glm::vec3& position, const glm::quat& rotation, bool useRotation,
                           int lastFreeIndex, bool allIntermediatesFree, const glm::vec3& alignment, float priority,
                           const QVector<int>& freeLineage, glm::mat4 rootTransform) {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return false;
    }
    if (freeLineage.isEmpty()) {
        return false;
    }
    if (lastFreeIndex == -1) {
        lastFreeIndex = freeLineage.last();
    }

    // this is a cyclic coordinate descent algorithm: see
    // http://www.ryanjuckett.com/programming/animation/21-cyclic-coordinate-descent-in-2d
    const int ITERATION_COUNT = 1;
    glm::vec3 worldAlignment = alignment;
    for (int i = 0; i < ITERATION_COUNT; i++) {
        // first, try to rotate the end effector as close as possible to the target rotation, if any
        glm::quat endRotation;
        if (useRotation) {
            JointState& state = _jointStates[jointIndex];

            state.setRotationInBindFrame(rotation, priority);
            endRotation = state.getRotationInBindFrame();
        }

        // then, we go from the joint upwards, rotating the end as close as possible to the target
        glm::vec3 endPosition = extractTranslation(_jointStates[jointIndex].getTransform());
        for (int j = 1; freeLineage.at(j - 1) != lastFreeIndex; j++) {
            int index = freeLineage.at(j);
            JointState& state = _jointStates[index];
            if (!(state.getIsFree() || allIntermediatesFree)) {
                continue;
            }
            glm::vec3 jointPosition = extractTranslation(state.getTransform());
            glm::vec3 jointVector = endPosition - jointPosition;
            glm::quat oldCombinedRotation = state.getRotation();
            glm::quat combinedDelta;
            float combinedWeight;
            if (useRotation) {
                combinedDelta = safeMix(rotation * glm::inverse(endRotation),
                    rotationBetween(jointVector, position - jointPosition), 0.5f);
                combinedWeight = 2.0f;

            } else {
                combinedDelta = rotationBetween(jointVector, position - jointPosition);
                combinedWeight = 1.0f;
            }
            if (alignment != glm::vec3() && j > 1) {
                jointVector = endPosition - jointPosition;
                glm::vec3 positionSum;
                for (int k = j - 1; k > 0; k--) {
                    int index = freeLineage.at(k);
                    updateJointState(index, rootTransform);
                    positionSum += extractTranslation(_jointStates.at(index).getTransform());
                }
                glm::vec3 projectedCenterOfMass = glm::cross(jointVector,
                    glm::cross(positionSum / (j - 1.0f) - jointPosition, jointVector));
                glm::vec3 projectedAlignment = glm::cross(jointVector, glm::cross(worldAlignment, jointVector));
                const float LENGTH_EPSILON = 0.001f;
                if (glm::length(projectedCenterOfMass) > LENGTH_EPSILON && glm::length(projectedAlignment) > LENGTH_EPSILON) {
                    combinedDelta = safeMix(combinedDelta, rotationBetween(projectedCenterOfMass, projectedAlignment),
                        1.0f / (combinedWeight + 1.0f));
                }
            }
            state.applyRotationDelta(combinedDelta, true, priority);
            glm::quat actualDelta = state.getRotation() * glm::inverse(oldCombinedRotation);
            endPosition = actualDelta * jointVector + jointPosition;
            if (useRotation) {
                endRotation = actualDelta * endRotation;
            }
        }
    }

    // now update the joint states from the top
    for (int j = freeLineage.size() - 1; j >= 0; j--) {
        updateJointState(freeLineage.at(j), rootTransform);
    }

    return true;
}

void Rig::inverseKinematics(int endIndex, glm::vec3 targetPosition, const glm::quat& targetRotation, float priority,
                            const QVector<int>& freeLineage, glm::mat4 rootTransform) {
    // NOTE: targetRotation is from in model-frame

    if (endIndex == -1 || _jointStates.isEmpty()) {
        return;
    }

    if (freeLineage.isEmpty()) {
        return;
    }
    int numFree = freeLineage.size();

    // store and remember topmost parent transform
    glm::mat4 topParentTransform;
    {
        int index = freeLineage.last();
        const JointState& state = _jointStates.at(index);
        int parentIndex = state.getParentIndex();
        if (parentIndex == -1) {
            topParentTransform = rootTransform;
        } else {
            topParentTransform = _jointStates[parentIndex].getTransform();
        }
    }

    // relax toward default rotation
    // NOTE: ideally this should use dt and a relaxation timescale to compute how much to relax
    for (int j = 0; j < numFree; j++) {
        int nextIndex = freeLineage.at(j);
        JointState& nextState = _jointStates[nextIndex];
        if (! nextState.getIsFree()) {
            continue;
        }

        // Apply the zero rotationDelta, but use mixRotationDelta() which blends a bit of the default pose
        // in the process.  This provides stability to the IK solution for most models.
        float mixFactor = 0.08f;
        nextState.mixRotationDelta(glm::quat(), mixFactor, priority);
    }

    // this is a cyclic coordinate descent algorithm: see
    // http://www.ryanjuckett.com/programming/animation/21-cyclic-coordinate-descent-in-2d

    // keep track of the position of the end-effector
    JointState& endState = _jointStates[endIndex];
    glm::vec3 endPosition = endState.getPosition();
    float distanceToGo = glm::distance(targetPosition, endPosition);

    const int MAX_ITERATION_COUNT = 3;
    const float ACCEPTABLE_IK_ERROR = 0.005f; // 5mm
    int numIterations = 0;
    do {
        ++numIterations;
        // moving up, rotate each free joint to get endPosition closer to target
        for (int j = 1; j < numFree; j++) {
            int nextIndex = freeLineage.at(j);
            JointState& nextState = _jointStates[nextIndex];
            if (! nextState.getIsFree()) {
                continue;
            }

            glm::vec3 pivot = nextState.getPosition();
            glm::vec3 leverArm = endPosition - pivot;
            float leverLength = glm::length(leverArm);
            if (leverLength < EPSILON) {
                continue;
            }
            glm::quat deltaRotation = rotationBetween(leverArm, targetPosition - pivot);

            // We want to mix the shortest rotation with one that will pull the system down with gravity
            // so that limbs don't float unrealistically.  To do this we compute a simplified center of mass
            // where each joint has unit mass and we don't bother averaging it because we only need direction.
            if (j > 1) {

                glm::vec3 centerOfMass(0.0f);
                for (int k = 0; k < j; ++k) {
                    int massIndex = freeLineage.at(k);
                    centerOfMass += _jointStates[massIndex].getPosition() - pivot;
                }
                // the gravitational effect is a rotation that tends to align the two cross products
                const glm::vec3 worldAlignment = glm::vec3(0.0f, -1.0f, 0.0f);
                glm::quat gravityDelta = rotationBetween(glm::cross(centerOfMass, leverArm),
                    glm::cross(worldAlignment, leverArm));

                float gravityAngle = glm::angle(gravityDelta);
                const float MIN_GRAVITY_ANGLE = 0.1f;
                float mixFactor = 0.1f;
                if (gravityAngle < MIN_GRAVITY_ANGLE) {
                    // the final rotation is a mix of the two
                    mixFactor = 0.5f * gravityAngle / MIN_GRAVITY_ANGLE;
                }
                deltaRotation = safeMix(deltaRotation, gravityDelta, mixFactor);
            }

            // Apply the rotation delta.
            glm::quat oldNextRotation = nextState.getRotation();
            float mixFactor = 0.05f;
            nextState.applyRotationDelta(deltaRotation, mixFactor, priority);

            // measure the result of the rotation which may have been modified by
            // blending and constraints
            glm::quat actualDelta = nextState.getRotation() * glm::inverse(oldNextRotation);
            endPosition = pivot + actualDelta * leverArm;
        }

        // recompute transforms from the top down
        glm::mat4 currentParentTransform = topParentTransform;
        for (int j = numFree - 1; j >= 0; --j) {
            JointState& freeState = _jointStates[freeLineage.at(j)];
            freeState.computeTransform(currentParentTransform);
            currentParentTransform = freeState.getTransform();
        }

        // measure our success
        endPosition = endState.getPosition();
        distanceToGo = glm::distance(targetPosition, endPosition);
    } while (numIterations < MAX_ITERATION_COUNT && distanceToGo > ACCEPTABLE_IK_ERROR);

    // set final rotation of the end joint
    endState.setRotationInModelFrame(targetRotation, priority, true);
}

bool Rig::restoreJointPosition(int jointIndex, float fraction, float priority, const QVector<int>& freeLineage) {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return false;
    }

    foreach (int index, freeLineage) {
        JointState& state = _jointStates[index];
        state.restoreRotation(fraction, priority);
    }
    return true;
}

float Rig::getLimbLength(int jointIndex, const QVector<int>& freeLineage,
                         const glm::vec3 scale, const QVector<FBXJoint>& fbxJoints) const {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return 0.0f;
    }
    float length = 0.0f;
    float lengthScale = (scale.x + scale.y + scale.z) / 3.0f;
    for (int i = freeLineage.size() - 2; i >= 0; i--) {
        length += fbxJoints.at(freeLineage.at(i)).distanceToParent * lengthScale;
    }
    return length;
}

glm::quat Rig::setJointRotationInBindFrame(int jointIndex, const glm::quat& rotation, float priority, bool constrain) {
    glm::quat endRotation;
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return endRotation;
    }
    JointState& state = _jointStates[jointIndex];
    state.setRotationInBindFrame(rotation, priority, constrain);
    endRotation = state.getRotationInBindFrame();
    return endRotation;
}

glm::vec3 Rig::getJointDefaultTranslationInConstrainedFrame(int jointIndex) {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return glm::vec3();
    }
    return _jointStates[jointIndex].getDefaultTranslationInConstrainedFrame();
}

glm::quat Rig::setJointRotationInConstrainedFrame(int jointIndex, glm::quat targetRotation, float priority, bool constrain, float mix) {
    glm::quat endRotation;
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return endRotation;
    }
    JointState& state = _jointStates[jointIndex];
    state.setRotationInConstrainedFrame(targetRotation, priority, constrain, mix);
    endRotation = state.getRotationInConstrainedFrame();
    return endRotation;
}

bool Rig::getJointRotationInConstrainedFrame(int jointIndex, glm::quat& quatOut) const {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return false;
    }
    quatOut = _jointStates[jointIndex].getRotationInConstrainedFrame();
    return true;
}

void Rig::updateVisibleJointStates() {
    for (int i = 0; i < _jointStates.size(); i++) {
        _jointStates[i].slaveVisibleTransform();
    }
}

void Rig::setJointVisibleTransform(int jointIndex, glm::mat4 newTransform) {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return;
    }
    _jointStates[jointIndex].setVisibleTransform(newTransform);
}

void Rig::applyJointRotationDelta(int jointIndex, const glm::quat& delta, bool constrain, float priority) {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return;
    }
    _jointStates[jointIndex].applyRotationDelta(delta, constrain, priority);
}

glm::quat Rig::getJointDefaultRotationInParentFrame(int jointIndex) {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return glm::quat();
    }
    return _jointStates[jointIndex].getDefaultRotationInParentFrame();
}

void Rig::updateFromHeadParameters(const HeadParameters& params) {
    updateLeanJoint(params.leanJointIndex, params.leanSideways, params.leanForward, params.torsoTwist);
    updateNeckJoint(params.neckJointIndex, params.localHeadOrientation, params.leanSideways, params.leanForward, params.torsoTwist);
    updateEyeJoints(params.leftEyeJointIndex, params.rightEyeJointIndex, params.modelTranslation, params.modelRotation,
                    params.worldHeadOrientation, params.eyeLookAt, params.eyeSaccade);
}

void Rig::updateLeanJoint(int index, float leanSideways, float leanForward, float torsoTwist) {
    if (index >= 0 && _jointStates[index].getParentIndex() >= 0) {
        auto& parentState = _jointStates[_jointStates[index].getParentIndex()];

        // get the rotation axes in joint space and use them to adjust the rotation
        glm::vec3 xAxis(1.0f, 0.0f, 0.0f);
        glm::vec3 yAxis(0.0f, 1.0f, 0.0f);
        glm::vec3 zAxis(0.0f, 0.0f, 1.0f);
        glm::quat inverse = glm::inverse(parentState.getRotation() * getJointDefaultRotationInParentFrame(index));
        setJointRotationInConstrainedFrame(index,
                                           glm::angleAxis(- RADIANS_PER_DEGREE * leanSideways, inverse * zAxis) *
                                           glm::angleAxis(- RADIANS_PER_DEGREE * leanForward, inverse * xAxis) *
                                           glm::angleAxis(RADIANS_PER_DEGREE * torsoTwist, inverse * yAxis) *
                                           getJointState(index).getDefaultRotation(), DEFAULT_PRIORITY);
    }
}

void Rig::updateNeckJoint(int index, const glm::quat& localHeadOrientation, float leanSideways, float leanForward, float torsoTwist) {
    if (index >= 0 && _jointStates[index].getParentIndex() >= 0) {
        auto& state = _jointStates[index];
        auto& parentState = _jointStates[state.getParentIndex()];

        // get the rotation axes in joint space and use them to adjust the rotation
        glm::mat3 axes = glm::mat3_cast(glm::quat());
        glm::mat3 inverse = glm::mat3(glm::inverse(parentState.getTransform() *
                                                   glm::translate(getJointDefaultTranslationInConstrainedFrame(index)) *
                                                   state.getPreTransform() * glm::mat4_cast(state.getPreRotation())));
        glm::vec3 pitchYawRoll = safeEulerAngles(localHeadOrientation);
        glm::vec3 lean = glm::radians(glm::vec3(leanForward, torsoTwist, leanSideways));
        pitchYawRoll -= lean;
        setJointRotationInConstrainedFrame(index,
                                           glm::angleAxis(-pitchYawRoll.z, glm::normalize(inverse * axes[2])) *
                                           glm::angleAxis(pitchYawRoll.y, glm::normalize(inverse * axes[1])) *
                                           glm::angleAxis(-pitchYawRoll.x, glm::normalize(inverse * axes[0])) *
                                           state.getDefaultRotation(), DEFAULT_PRIORITY);
    }
}

void Rig::updateEyeJoints(int leftEyeIndex, int rightEyeIndex, const glm::vec3& modelTranslation, const glm::quat& modelRotation,
                          const glm::quat& worldHeadOrientation, const glm::vec3& lookAtSpot, const glm::vec3& saccade) {
    updateEyeJoint(leftEyeIndex, modelTranslation, modelRotation, worldHeadOrientation, lookAtSpot, saccade);
    updateEyeJoint(rightEyeIndex, modelTranslation, modelRotation, worldHeadOrientation, lookAtSpot, saccade);
}

void Rig::updateEyeJoint(int index, const glm::vec3& modelTranslation, const glm::quat& modelRotation, const glm::quat& worldHeadOrientation, const glm::vec3& lookAtSpot, const glm::vec3& saccade) {
    if (index >= 0 && _jointStates[index].getParentIndex() >= 0) {
        auto& state = _jointStates[index];
        auto& parentState = _jointStates[state.getParentIndex()];

        // NOTE: at the moment we do the math in the world-frame, hence the inverse transform is more complex than usual.
        glm::mat4 inverse = glm::inverse(glm::mat4_cast(modelRotation) * parentState.getTransform() *
                                         glm::translate(state.getDefaultTranslationInConstrainedFrame()) *
                                         state.getPreTransform() * glm::mat4_cast(state.getPreRotation() * state.getDefaultRotation()));
        glm::vec3 front = glm::vec3(inverse * glm::vec4(worldHeadOrientation * IDENTITY_FRONT, 0.0f));
        glm::vec3 lookAtDelta = lookAtSpot - modelTranslation;
        glm::vec3 lookAt = glm::vec3(inverse * glm::vec4(lookAtDelta + glm::length(lookAtDelta) * saccade, 1.0f));
        glm::quat between = rotationBetween(front, lookAt);
        const float MAX_ANGLE = 30.0f * RADIANS_PER_DEGREE;
        state.setRotationInConstrainedFrame(glm::angleAxis(glm::clamp(glm::angle(between), -MAX_ANGLE, MAX_ANGLE), glm::axis(between)) *
                                            state.getDefaultRotation(), DEFAULT_PRIORITY);
    }
}

void Rig::initAnimGraph(const QUrl& url, const FBXGeometry& fbxGeometry) {
    if (!_enableAnimGraph) {
        return;
    }

    _animSkeleton = std::make_shared<AnimSkeleton>(fbxGeometry);

    // load the anim graph
    _animLoader.reset(new AnimNodeLoader(url));
    connect(_animLoader.get(), &AnimNodeLoader::success, [this](AnimNode::Pointer nodeIn) {
        _animNode = nodeIn;
        _animNode->setSkeleton(_animSkeleton);
    });
    connect(_animLoader.get(), &AnimNodeLoader::error, [this, url](int error, QString str) {
        qCCritical(animation) << "Error loading" << url.toDisplayString() << "code = " << error << "str =" << str;
    });
}
