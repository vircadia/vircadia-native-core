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

#include "AnimationHandle.h"

#include "Rig.h"

bool Rig::removeRunningAnimation(AnimationHandlePointer animationHandle) {
    return _runningAnimations.removeOne(animationHandle);
}

static void insertSorted(QList<AnimationHandlePointer>& handles, const AnimationHandlePointer& handle) {
    for (QList<AnimationHandlePointer>::iterator it = handles.begin(); it != handles.end(); it++) {
        if (handle->getPriority() > (*it)->getPriority()) {
            handles.insert(it, handle);
            return;
        }
    }
    handles.append(handle);
}

void Rig::addRunningAnimation(AnimationHandlePointer animationHandle) {
    insertSorted(_runningAnimations,  animationHandle);
}

bool Rig::isRunningAnimation(AnimationHandlePointer animationHandle) {
    return _runningAnimations.contains(animationHandle);
}

float Rig::initJointStates(glm::vec3 scale, glm::vec3 offset, QVector<JointState> states) {
    _jointStates = states;
    initJointTransforms(scale, offset);

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

    // XXX update AnimationHandles from here?
    return radius;
}

void Rig::initJointTransforms(glm::vec3 scale, glm::vec3 offset) {
    // compute model transforms
    int numStates = _jointStates.size();
    for (int i = 0; i < numStates; ++i) {
        JointState& state = _jointStates[i];
        const FBXJoint& joint = state.getFBXJoint();
        int parentIndex = joint.parentIndex;
        if (parentIndex == -1) {
            // const FBXGeometry& geometry = _geometry->getFBXGeometry();
            // NOTE: in practice geometry.offset has a non-unity scale (rather than a translation)
            glm::mat4 parentTransform = glm::scale(scale) * glm::translate(offset); // * geometry.offset; XXX
            state.initTransform(parentTransform);
        } else {
            const JointState& parentState = _jointStates.at(parentIndex);
            state.initTransform(parentState.getTransform());
        }
    }
}

void Rig::resetJoints() {
    if (_jointStates.isEmpty()) {
        return;
    }

    // const FBXGeometry& geometry = _geometry->getFBXGeometry();
    for (int i = 0; i < _jointStates.size(); i++) {
        const FBXJoint& fbxJoint = _jointStates[i].getFBXJoint();
        _jointStates[i].setRotationInConstrainedFrame(fbxJoint.rotation, 0.0f);
    }
}

bool Rig::getJointState(int index, glm::quat& rotation) const {
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

void Rig::updateVisibleJointStates() {
    for (int i = 0; i < _jointStates.size(); i++) {
        _jointStates[i].slaveVisibleTransform();
    }
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


void Rig::clearJointAnimationPriority(int index) {
    if (index != -1 && index < _jointStates.size()) {
        _jointStates[index]._animationPriority = 0.0f;
    }
}

AnimationHandlePointer Rig::createAnimationHandle() {
    AnimationHandlePointer handle(new AnimationHandle(getRigPointer()));
    _animationHandles.insert(handle);
    return handle;
}

bool Rig::getJointStateAtIndex(int jointIndex, JointState& jointState) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return false;
    }
    jointState = _jointStates[jointIndex];
    return true;
}

void Rig::updateJointStates(glm::mat4 parentTransform) {
    for (int i = 0; i < _jointStates.size(); i++) {
        updateJointState(i, parentTransform);
    }
}

void Rig::updateJointState(int index, glm::mat4 parentTransform) {
    JointState& state = _jointStates[index];
    const FBXJoint& joint = state.getFBXJoint();

    // compute model transforms
    int parentIndex = joint.parentIndex;
    if (parentIndex == -1) {
        // glm::mat4 parentTransform = glm::scale(scale) * glm::translate(offset) * geometryOffset;
        state.computeTransform(parentTransform);
    } else {
        // guard against out-of-bounds access to _jointStates
        if (joint.parentIndex >= 0 && joint.parentIndex < _jointStates.size()) {
            const JointState& parentState = _jointStates.at(parentIndex);
            state.computeTransform(parentState.getTransform(), parentState.getTransformChanged());
        }
    }
}

void Rig::resetAllTransformsChanged() {
    for (int i = 0; i < _jointStates.size(); i++) {
        _jointStates[i].resetTransformChanged();
    }
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

void Rig::applyJointRotationDelta(int jointIndex, const glm::quat& delta, bool constrain, float priority) {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return;
    }
    _jointStates[jointIndex].applyRotationDelta(delta, constrain, priority);
}

bool Rig::setJointPosition(int jointIndex, const glm::vec3& position, const glm::quat& rotation,
                           bool useRotation, int lastFreeIndex, bool allIntermediatesFree, const glm::vec3& alignment,
                           float priority, glm::mat4 parentTransform) {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return false;
    }

    // const FBXGeometry& geometry = _geometry->getFBXGeometry();
    // const QVector<int>& freeLineage = geometry.joints.at(jointIndex).freeLineage;
    const FBXJoint& fbxJoint = _jointStates[jointIndex].getFBXJoint();
    const QVector<int>& freeLineage = fbxJoint.freeLineage;


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

void Rig::inverseKinematics(int endIndex, glm::vec3 targetPosition,
                            const glm::quat& targetRotation, float priority, glm::mat4 parentTransform) {
    // NOTE: targetRotation is from bind- to model-frame

    if (endIndex == -1 || _jointStates.isEmpty()) {
        return;
    }

    // const FBXGeometry& geometry = _geometry->getFBXGeometry();
    // const QVector<int>& freeLineage = geometry.joints.at(endIndex).freeLineage;
    const FBXJoint& fbxJoint = _jointStates[endIndex].getFBXJoint();
    const QVector<int>& freeLineage = fbxJoint.freeLineage;

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
        glm::mat4 parentTransform = topParentTransform;
        for (int j = numFree - 1; j >= 0; --j) {
            JointState& freeState = _jointStates[freeLineage.at(j)];
            freeState.computeTransform(parentTransform);
            parentTransform = freeState.getTransform();
        }

        // measure our success
        endPosition = endState.getPosition();
        distanceToGo = glm::distance(targetPosition, endPosition);
    } while (numIterations < MAX_ITERATION_COUNT && distanceToGo < ACCEPTABLE_IK_ERROR);

    // set final rotation of the end joint
    endState.setRotationInBindFrame(targetRotation, priority, true);
}

bool Rig::restoreJointPosition(int jointIndex, float fraction, float priority) {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return false;
    }
    // const FBXGeometry& geometry = _geometry->getFBXGeometry();
    // const QVector<int>& freeLineage = geometry.joints.at(jointIndex).freeLineage;
    const FBXJoint& fbxJoint = _jointStates[jointIndex].getFBXJoint();
    const QVector<int>& freeLineage = fbxJoint.freeLineage;

    foreach (int index, freeLineage) {
        JointState& state = _jointStates[index];
        state.restoreRotation(fraction, priority);
    }
    return true;
}

float Rig::getLimbLength(int jointIndex, glm::vec3 scale) const {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return 0.0f;
    }

    // const FBXGeometry& geometry = _geometry->getFBXGeometry();
    // const QVector<int>& freeLineage = geometry.joints.at(jointIndex).freeLineage;
    const FBXJoint& fbxJoint = _jointStates[jointIndex].getFBXJoint();
    const QVector<int>& freeLineage = fbxJoint.freeLineage;

    float length = 0.0f;
    float lengthScale = (scale.x + scale.y + scale.z) / 3.0f;
    for (int i = freeLineage.size() - 2; i >= 0; i--) {
        int something = freeLineage.at(i);
        const FBXJoint& fbxJointI = _jointStates[something].getFBXJoint();
        length += fbxJointI.distanceToParent * lengthScale;
    }
    return length;
}
