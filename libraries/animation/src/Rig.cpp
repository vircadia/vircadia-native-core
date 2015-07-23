//
//  Rig.cpp
//  libraries/script-engine/src/
//
//  Created by Howard Stearns, Seth Alves, Anthony Thibault, Andrew Meadows on 7/15/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <queue>

#include "AnimationHandle.h"

#include "Rig.h"

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
    _animationHandles.insert(handle);
    return handle;
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

void Rig::deleteAnimations() {
    for (QSet<AnimationHandlePointer>::iterator it = _animationHandles.begin(); it != _animationHandles.end(); ) {
        (*it)->clearJoints();
        it = _animationHandles.erase(it);
    }
}

float Rig::initJointStates(QVector<JointState> states, glm::mat4 parentTransform, int neckJointIndex) {
    _jointStates = states;
    _neckJointIndex = neckJointIndex;
    initJointTransforms(parentTransform);

    int numStates = _jointStates.size();
    float radius = 0.0f;
    for (int i = 0; i < numStates; ++i) {
        float distance = glm::length(_jointStates[i].getPosition());
        if (distance > radius) {
            radius = distance;
        }
        _jointStates[i].buildConstraint();
    }
    for (int i = 0; i < _jointStates.size(); i++) {
        _jointStates[i].slaveVisibleTransform();
    }

    initHeadBones();

    return radius;
}


void Rig::initJointTransforms(glm::mat4 parentTransform) {
    // compute model transforms
    int numStates = _jointStates.size();
    for (int i = 0; i < numStates; ++i) {
        JointState& state = _jointStates[i];
        const FBXJoint& joint = state.getFBXJoint();
        int parentIndex = joint.parentIndex;
        if (parentIndex == -1) {
            state.initTransform(parentTransform);
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
    // return _jointStates[jointIndex];
    return maybeCauterizeHead(jointIndex);
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
        _jointStates[index]._animationPriority = 0.0f;
    }
}

float Rig::getJointAnimatinoPriority(int index) {
    if (index != -1 && index < _jointStates.size()) {
        return _jointStates[index]._animationPriority;
    }
    return 0.0f;
}

void Rig::setJointAnimatinoPriority(int index, float newPriority) {
    if (index != -1 && index < _jointStates.size()) {
        _jointStates[index]._animationPriority = newPriority;
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
    // position = translation + rotation * _jointStates[jointIndex].getPosition();
    position = translation + rotation * maybeCauterizeHead(jointIndex).getPosition();
    return true;
}

bool Rig::getJointPosition(int jointIndex, glm::vec3& position) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return false;
    }
    // position is in model-frame
    position = extractTranslation(maybeCauterizeHead(jointIndex).getTransform());
    return true;
}

bool Rig::getJointRotationInWorldFrame(int jointIndex, glm::quat& result, const glm::quat& rotation) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return false;
    }
    result = rotation * maybeCauterizeHead(jointIndex).getRotation();
    return true;
}

bool Rig::getJointRotation(int jointIndex, glm::quat& rotation) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return false;
    }
    rotation = maybeCauterizeHead(jointIndex).getRotation();
    return true;
}

bool Rig::getJointCombinedRotation(int jointIndex, glm::quat& result, const glm::quat& rotation) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return false;
    }
    result = rotation * maybeCauterizeHead(jointIndex).getRotation();
    return true;
}


bool Rig::getVisibleJointPositionInWorldFrame(int jointIndex, glm::vec3& position,
                                              glm::vec3 translation, glm::quat rotation) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return false;
    }
    // position is in world-frame
    position = translation + rotation * maybeCauterizeHead(jointIndex).getVisiblePosition();
    return true;
}

bool Rig::getVisibleJointRotationInWorldFrame(int jointIndex, glm::quat& result, glm::quat rotation) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return false;
    }
    result = rotation * maybeCauterizeHead(jointIndex).getVisibleRotation();
    return true;
}

glm::mat4 Rig::getJointTransform(int jointIndex) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return glm::mat4();
    }
    return maybeCauterizeHead(jointIndex).getTransform();
}

glm::mat4 Rig::getJointVisibleTransform(int jointIndex) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return glm::mat4();
    }
    return maybeCauterizeHead(jointIndex).getVisibleTransform();
}

void Rig::simulateInternal(float deltaTime, glm::mat4 parentTransform) {
    // update animations
    foreach (const AnimationHandlePointer& handle, _runningAnimations) {
        handle->simulate(deltaTime);
    }

    for (int i = 0; i < _jointStates.size(); i++) {
        updateJointState(i, parentTransform);
    }
    for (int i = 0; i < _jointStates.size(); i++) {
        _jointStates[i].resetTransformChanged();
    }
}

bool Rig::setJointPosition(int jointIndex, const glm::vec3& position, const glm::quat& rotation, bool useRotation,
                           int lastFreeIndex, bool allIntermediatesFree, const glm::vec3& alignment, float priority,
                           const QVector<int>& freeLineage, glm::mat4 parentTransform) {
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
            const FBXJoint& joint = state.getFBXJoint();
            if (!(joint.isFree || allIntermediatesFree)) {
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
                    updateJointState(index, parentTransform);
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
        updateJointState(freeLineage.at(j), parentTransform);
    }

    return true;
}

void Rig::inverseKinematics(int endIndex, glm::vec3 targetPosition, const glm::quat& targetRotation, float priority,
                            const QVector<int>& freeLineage, glm::mat4 parentTransform) {
    // NOTE: targetRotation is from bind- to model-frame

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
        const FBXJoint& joint = state.getFBXJoint();
        int parentIndex = joint.parentIndex;
        if (parentIndex == -1) {
            topParentTransform = parentTransform;
        } else {
            topParentTransform = _jointStates[parentIndex].getTransform();
        }
    }

    // this is a cyclic coordinate descent algorithm: see
    // http://www.ryanjuckett.com/programming/animation/21-cyclic-coordinate-descent-in-2d

    // keep track of the position of the end-effector
    JointState& endState = _jointStates[endIndex];
    glm::vec3 endPosition = endState.getPosition();
    float distanceToGo = glm::distance(targetPosition, endPosition);

    const int MAX_ITERATION_COUNT = 2;
    const float ACCEPTABLE_IK_ERROR = 0.005f; // 5mm
    int numIterations = 0;
    do {
        ++numIterations;
        // moving up, rotate each free joint to get endPosition closer to target
        for (int j = 1; j < numFree; j++) {
            int nextIndex = freeLineage.at(j);
            JointState& nextState = _jointStates[nextIndex];
            FBXJoint nextJoint = nextState.getFBXJoint();
            if (! nextJoint.isFree) {
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
                float mixFactor = 0.5f;
                if (gravityAngle < MIN_GRAVITY_ANGLE) {
                    // the final rotation is a mix of the two
                    mixFactor = 0.5f * gravityAngle / MIN_GRAVITY_ANGLE;
                }
                deltaRotation = safeMix(deltaRotation, gravityDelta, mixFactor);
            }

            // Apply the rotation, but use mixRotationDelta() which blends a bit of the default pose
            // in the process.  This provides stability to the IK solution for most models.
            glm::quat oldNextRotation = nextState.getRotation();
            float mixFactor = 0.03f;
            nextState.mixRotationDelta(deltaRotation, mixFactor, priority);

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
    } while (numIterations < MAX_ITERATION_COUNT && distanceToGo < ACCEPTABLE_IK_ERROR);

    // set final rotation of the end joint
    endState.setRotationInBindFrame(targetRotation, priority, true);
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
    return maybeCauterizeHead(jointIndex).getDefaultTranslationInConstrainedFrame();
}

glm::quat Rig::setJointRotationInConstrainedFrame(int jointIndex, glm::quat targetRotation, float priority, bool constrain) {
    glm::quat endRotation;
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return endRotation;
    }
    JointState& state = _jointStates[jointIndex];
    state.setRotationInConstrainedFrame(targetRotation, priority, constrain);
    endRotation = state.getRotationInConstrainedFrame();
    return endRotation;
}

void Rig::updateVisibleJointStates() {
    for (int i = 0; i < _jointStates.size(); i++) {
        _jointStates[i].slaveVisibleTransform();
    }
}

void Rig::setJointTransform(int jointIndex, glm::mat4 newTransform) {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return;
    }
    _jointStates[jointIndex].setTransform(newTransform);
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
    return maybeCauterizeHead(jointIndex).getDefaultRotationInParentFrame();
}

void Rig::initHeadBones() {
    if (_neckJointIndex == -1) {
        return;
    }
    _headBones.clear();
    std::queue<int> q;
    q.push(_neckJointIndex);
    _headBones.push_back(_neckJointIndex);

    // fbxJoints only hold links to parents not children, so we have to do a bit of extra work here.
    while (q.size() > 0) {
        int jointIndex = q.front();
        for (int i = 0; i < _jointStates.size(); i++) {
            const FBXJoint& fbxJoint = _jointStates[i].getFBXJoint();
            if (jointIndex == fbxJoint.parentIndex) {
                _headBones.push_back(i);
                q.push(i);
            }
        }
        q.pop();
    }
}

JointState Rig::maybeCauterizeHead(int jointIndex) const {
    // if (_headBones.contains(jointIndex)) {
    // XXX fix this... make _headBones a hash?  add a flag to JointState?
    if (_neckJointIndex != -1 &&
        _isFirstPerson &&
        std::find(_headBones.begin(), _headBones.end(), jointIndex) != _headBones.end()) {
        glm::vec4 trans = _jointStates[jointIndex].getTransform()[3];
        glm::vec4 zero(0, 0, 0, 0);
        glm::mat4 newXform(zero, zero, zero, trans);
        JointState jointStateCopy = _jointStates[jointIndex];
        jointStateCopy.setTransform(newXform);
        jointStateCopy.setVisibleTransform(newXform);
        return jointStateCopy;
    } else {
        return _jointStates[jointIndex];
    }
}

void Rig::setFirstPerson(bool isFirstPerson) {
    if (_isFirstPerson != isFirstPerson) {
        _isFirstPerson = isFirstPerson;
        _jointsAreDirty = true;
    }
}
