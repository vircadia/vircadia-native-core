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
#include <QScriptValueIterator>
#include <QWriteLocker>
#include <QReadLocker>

#include <GeometryUtil.h>
#include <NumericalConstants.h>
#include <DebugDraw.h>
#include <ScriptValueUtils.h>
#include <shared/NsightHelpers.h>

#include "AnimationLogging.h"
#include "AnimClip.h"
#include "AnimInverseKinematics.h"
#include "AnimSkeleton.h"
#include "IKTarget.h"

static bool isEqual(const glm::vec3& u, const glm::vec3& v) {
    const float EPSILON = 0.0001f;
    return glm::length(u - v) / glm::length(u) <= EPSILON;
}

static bool isEqual(const glm::quat& p, const glm::quat& q) {
    const float EPSILON = 0.00001f;
    return 1.0f - fabsf(glm::dot(p, q)) <= EPSILON;
}

#define ASSERT(cond) assert(cond)

// 2 meter tall dude
const glm::vec3 DEFAULT_RIGHT_EYE_POS(-0.3f, 0.9f, 0.0f);
const glm::vec3 DEFAULT_LEFT_EYE_POS(0.3f, 0.9f, 0.0f);
const glm::vec3 DEFAULT_HEAD_POS(0.0f, 0.75f, 0.0f);
const glm::vec3 DEFAULT_NECK_POS(0.0f, 0.70f, 0.0f);

void Rig::overrideAnimation(const QString& url, float fps, bool loop, float firstFrame, float lastFrame) {

    UserAnimState::ClipNodeEnum clipNodeEnum;
    if (_userAnimState.clipNodeEnum == UserAnimState::None || _userAnimState.clipNodeEnum == UserAnimState::B) {
        clipNodeEnum = UserAnimState::A;
    } else {
        clipNodeEnum = UserAnimState::B;
    }

    if (_animNode) {
        // find an unused AnimClip clipNode
        std::shared_ptr<AnimClip> clip;
        if (clipNodeEnum == UserAnimState::A) {
            clip = std::dynamic_pointer_cast<AnimClip>(_animNode->findByName("userAnimA"));
        } else {
            clip = std::dynamic_pointer_cast<AnimClip>(_animNode->findByName("userAnimB"));
        }

        if (clip) {
            // set parameters
            clip->setLoopFlag(loop);
            clip->setStartFrame(firstFrame);
            clip->setEndFrame(lastFrame);
            const float REFERENCE_FRAMES_PER_SECOND = 30.0f;
            float timeScale = fps / REFERENCE_FRAMES_PER_SECOND;
            clip->setTimeScale(timeScale);
            clip->loadURL(url);
        }
    }

    // store current user anim state.
    _userAnimState = { clipNodeEnum, url, fps, loop, firstFrame, lastFrame };

    // notify the userAnimStateMachine the desired state.
    _animVars.set("userAnimNone", false);
    _animVars.set("userAnimA", clipNodeEnum == UserAnimState::A);
    _animVars.set("userAnimB", clipNodeEnum == UserAnimState::B);
}

void Rig::restoreAnimation() {
    if (_userAnimState.clipNodeEnum != UserAnimState::None) {
        _userAnimState.clipNodeEnum = UserAnimState::None;

        // notify the userAnimStateMachine the desired state.
        _animVars.set("userAnimNone", true);
        _animVars.set("userAnimA", false);
        _animVars.set("userAnimB", false);
    }
}

QStringList Rig::getAnimationRoles() const {
    if (_animNode) {
        QStringList list;
        _animNode->traverse([&](AnimNode::Pointer node) {
            // only report clip nodes as valid roles.
            auto clipNode = std::dynamic_pointer_cast<AnimClip>(node);
            if (clipNode) {
                // filter out the userAnims, they are for internal use only.
                if (!clipNode->getID().startsWith("userAnim")) {
                    list.append(node->getID());
                }
            }
            return true;
        });
        return list;
    } else {
        return QStringList();
    }
}

void Rig::overrideRoleAnimation(const QString& role, const QString& url, float fps, bool loop, float firstFrame, float lastFrame) {
    if (_animNode) {
        AnimNode::Pointer node = _animNode->findByName(role);
        if (node) {
            _origRoleAnimations[role] = node;
            const float REFERENCE_FRAMES_PER_SECOND = 30.0f;
            float timeScale = fps / REFERENCE_FRAMES_PER_SECOND;
            auto clipNode = std::make_shared<AnimClip>(role, url, firstFrame, lastFrame, timeScale, loop, false);
            AnimNode::Pointer parent = node->getParent();
            parent->replaceChild(node, clipNode);
        } else {
            qCWarning(animation) << "Rig::overrideRoleAnimation could not find role " << role;
        }
    } else {
        qCWarning(animation) << "Rig::overrideRoleAnimation avatar not ready yet";
    }
}

void Rig::restoreRoleAnimation(const QString& role) {
    if (_animNode) {
        AnimNode::Pointer node = _animNode->findByName(role);
        if (node) {
            auto iter = _origRoleAnimations.find(role);
            if (iter != _origRoleAnimations.end()) {
                node->getParent()->replaceChild(node, iter->second);
                _origRoleAnimations.erase(iter);
            } else {
                qCWarning(animation) << "Rig::restoreRoleAnimation could not find role " << role;
            }
        }
    } else {
        qCWarning(animation) << "Rig::overrideRoleAnimation avatar not ready yet";
    }
}

void Rig::destroyAnimGraph() {
    _animSkeleton.reset();
    _animLoader.reset();
    _animNode.reset();
    _internalPoseSet._relativePoses.clear();
    _internalPoseSet._absolutePoses.clear();
    _internalPoseSet._overridePoses.clear();
    _internalPoseSet._overrideFlags.clear();
}

void Rig::initJointStates(const FBXGeometry& geometry, const glm::mat4& modelOffset) {
    _geometryOffset = AnimPose(geometry.offset);
    _invGeometryOffset = _geometryOffset.inverse();
    setModelOffset(modelOffset);

    _animSkeleton = std::make_shared<AnimSkeleton>(geometry);

    _internalPoseSet._relativePoses.clear();
    _internalPoseSet._relativePoses = _animSkeleton->getRelativeDefaultPoses();

    buildAbsoluteRigPoses(_internalPoseSet._relativePoses, _internalPoseSet._absolutePoses);

    _internalPoseSet._overridePoses.clear();
    _internalPoseSet._overridePoses = _animSkeleton->getRelativeDefaultPoses();

    _internalPoseSet._overrideFlags.clear();
    _internalPoseSet._overrideFlags.resize(_animSkeleton->getNumJoints(), false);

    buildAbsoluteRigPoses(_animSkeleton->getRelativeDefaultPoses(), _absoluteDefaultPoses);

    _rootJointIndex = geometry.rootJointIndex;
    _leftHandJointIndex = geometry.leftHandJointIndex;
    _leftElbowJointIndex = _leftHandJointIndex >= 0 ? geometry.joints.at(_leftHandJointIndex).parentIndex : -1;
    _leftShoulderJointIndex = _leftElbowJointIndex >= 0 ? geometry.joints.at(_leftElbowJointIndex).parentIndex : -1;
    _rightHandJointIndex = geometry.rightHandJointIndex;
    _rightElbowJointIndex = _rightHandJointIndex >= 0 ? geometry.joints.at(_rightHandJointIndex).parentIndex : -1;
    _rightShoulderJointIndex = _rightElbowJointIndex >= 0 ? geometry.joints.at(_rightElbowJointIndex).parentIndex : -1;
}

void Rig::reset(const FBXGeometry& geometry) {
    _geometryOffset = AnimPose(geometry.offset);
    _invGeometryOffset = _geometryOffset.inverse();
    _animSkeleton = std::make_shared<AnimSkeleton>(geometry);

    _internalPoseSet._relativePoses.clear();
    _internalPoseSet._relativePoses = _animSkeleton->getRelativeDefaultPoses();

    buildAbsoluteRigPoses(_internalPoseSet._relativePoses, _internalPoseSet._absolutePoses);

    _internalPoseSet._overridePoses.clear();
    _internalPoseSet._overridePoses = _animSkeleton->getRelativeDefaultPoses();

    _internalPoseSet._overrideFlags.clear();
    _internalPoseSet._overrideFlags.resize(_animSkeleton->getNumJoints(), false);

    buildAbsoluteRigPoses(_animSkeleton->getRelativeDefaultPoses(), _absoluteDefaultPoses);

    _rootJointIndex = geometry.rootJointIndex;
    _leftHandJointIndex = geometry.leftHandJointIndex;
    _leftElbowJointIndex = _leftHandJointIndex >= 0 ? geometry.joints.at(_leftHandJointIndex).parentIndex : -1;
    _leftShoulderJointIndex = _leftElbowJointIndex >= 0 ? geometry.joints.at(_leftElbowJointIndex).parentIndex : -1;
    _rightHandJointIndex = geometry.rightHandJointIndex;
    _rightElbowJointIndex = _rightHandJointIndex >= 0 ? geometry.joints.at(_rightHandJointIndex).parentIndex : -1;
    _rightShoulderJointIndex = _rightElbowJointIndex >= 0 ? geometry.joints.at(_rightElbowJointIndex).parentIndex : -1;

    if (!_animGraphURL.isEmpty()) {
        initAnimGraph(_animGraphURL);
    }
}

bool Rig::jointStatesEmpty() {
    return _internalPoseSet._relativePoses.empty();
}

int Rig::getJointStateCount() const {
    return (int)_internalPoseSet._relativePoses.size();
}

static const uint32_t MAX_JOINT_NAME_WARNING_COUNT = 100;

int Rig::indexOfJoint(const QString& jointName) const {
    if (_animSkeleton) {
        int result = _animSkeleton->nameToJointIndex(jointName);

        // This is a content error, so we should issue a warning.
        if (result < 0 && _jointNameWarningCount < MAX_JOINT_NAME_WARNING_COUNT) {
            qCWarning(animation) << "Rig: Missing joint" << jointName << "in avatar model";
            _jointNameWarningCount++;
        }
        return result;
    } else {
        // This is normal and can happen when the avatar model has not been dowloaded/loaded yet.
        return -1;
    }
}

QString Rig::nameOfJoint(int jointIndex) const {
    if (_animSkeleton) {
        return _animSkeleton->getJointName(jointIndex);
    } else {
        return "";
    }
}

void Rig::setModelOffset(const glm::mat4& modelOffsetMat) {
    AnimPose newModelOffset = AnimPose(modelOffsetMat);
    if (!isEqual(_modelOffset.trans, newModelOffset.trans) ||
        !isEqual(_modelOffset.rot, newModelOffset.rot) ||
        !isEqual(_modelOffset.scale, newModelOffset.scale)) {

        _modelOffset = newModelOffset;

        // compute geometryToAvatarTransforms
        _geometryToRigTransform = _modelOffset * _geometryOffset;
        _rigToGeometryTransform = glm::inverse(_geometryToRigTransform);

        // rebuild cached default poses
        buildAbsoluteRigPoses(_animSkeleton->getRelativeDefaultPoses(), _absoluteDefaultPoses);
    }
}

void Rig::clearJointState(int index) {
    if (isIndexValid(index)) {
        _internalPoseSet._overrideFlags[index] = false;
        _internalPoseSet._overridePoses[index] = _animSkeleton->getRelativeDefaultPose(index);
    }
}

void Rig::clearJointStates() {
    _internalPoseSet._overrideFlags.clear();
    if (_animSkeleton) {
        _internalPoseSet._overrideFlags.resize(_animSkeleton->getNumJoints());
        _internalPoseSet._overridePoses = _animSkeleton->getRelativeDefaultPoses();
    }
}

void Rig::clearJointAnimationPriority(int index) {
    if (isIndexValid(index)) {
        _internalPoseSet._overrideFlags[index] = false;
        _internalPoseSet._overridePoses[index] = _animSkeleton->getRelativeDefaultPose(index);
    }
}

void Rig::clearIKJointLimitHistory() {
    if (_animNode) {
        _animNode->traverse([&](AnimNode::Pointer node) {
            // only report clip nodes as valid roles.
            auto ikNode = std::dynamic_pointer_cast<AnimInverseKinematics>(node);
            if (ikNode) {
                ikNode->clearIKJointLimitHistory();
            }
            return true;
        });
    }
}

void Rig::setJointTranslation(int index, bool valid, const glm::vec3& translation, float priority) {
    if (isIndexValid(index)) {
        if (valid) {
            assert(_internalPoseSet._overrideFlags.size() == _internalPoseSet._overridePoses.size());
            _internalPoseSet._overrideFlags[index] = true;
            _internalPoseSet._overridePoses[index].trans = translation;
        }
    }
}

void Rig::setJointState(int index, bool valid, const glm::quat& rotation, const glm::vec3& translation, float priority) {
    if (isIndexValid(index)) {
        assert(_internalPoseSet._overrideFlags.size() == _internalPoseSet._overridePoses.size());
        _internalPoseSet._overrideFlags[index] = true;
        _internalPoseSet._overridePoses[index].rot = rotation;
        _internalPoseSet._overridePoses[index].trans = translation;
    }
}

void Rig::setJointRotation(int index, bool valid, const glm::quat& rotation, float priority) {
    if (isIndexValid(index)) {
        if (valid) {
            ASSERT(_internalPoseSet._overrideFlags.size() == _internalPoseSet._overridePoses.size());
            _internalPoseSet._overrideFlags[index] = true;
            _internalPoseSet._overridePoses[index].rot = rotation;
        }
    }
}

void Rig::restoreJointRotation(int index, float fraction, float priority) {
    // AJT: DEAD CODE?
    ASSERT(false);
}

void Rig::restoreJointTranslation(int index, float fraction, float priority) {
    // AJT: DEAD CODE?
    ASSERT(false);
}

bool Rig::getJointPositionInWorldFrame(int jointIndex, glm::vec3& position, glm::vec3 translation, glm::quat rotation) const {
    if (isIndexValid(jointIndex)) {
        position = (rotation * _internalPoseSet._absolutePoses[jointIndex].trans) + translation;
        return true;
    } else {
        return false;
    }
}

bool Rig::getJointPosition(int jointIndex, glm::vec3& position) const {
    if (isIndexValid(jointIndex)) {
        position = _internalPoseSet._absolutePoses[jointIndex].trans;
        return true;
    } else {
        return false;
    }
}

bool Rig::getJointRotationInWorldFrame(int jointIndex, glm::quat& result, const glm::quat& rotation) const {
    if (isIndexValid(jointIndex)) {
        result = rotation * _internalPoseSet._absolutePoses[jointIndex].rot;
        return true;
    } else {
        return false;
    }
}

bool Rig::getJointRotation(int jointIndex, glm::quat& rotation) const {
    QReadLocker readLock(&_externalPoseSetLock);
    if (jointIndex >= 0 && jointIndex < (int)_externalPoseSet._relativePoses.size()) {
        rotation = _externalPoseSet._relativePoses[jointIndex].rot;
        return true;
    } else {
        return false;
    }
}

bool Rig::getAbsoluteJointRotationInRigFrame(int jointIndex, glm::quat& rotation) const {
    QReadLocker readLock(&_externalPoseSetLock);
    if (jointIndex >= 0 && jointIndex < (int)_externalPoseSet._absolutePoses.size()) {
        rotation = _externalPoseSet._absolutePoses[jointIndex].rot;
        return true;
    } else {
        return false;
    }
}

bool Rig::getJointTranslation(int jointIndex, glm::vec3& translation) const {
    QReadLocker readLock(&_externalPoseSetLock);
    if (jointIndex >= 0 && jointIndex < (int)_externalPoseSet._relativePoses.size()) {
        translation = _externalPoseSet._relativePoses[jointIndex].trans;
        return true;
    } else {
        return false;
    }
}

bool Rig::getAbsoluteJointTranslationInRigFrame(int jointIndex, glm::vec3& translation) const {
    QReadLocker readLock(&_externalPoseSetLock);
    if (jointIndex >= 0 && jointIndex < (int)_externalPoseSet._absolutePoses.size()) {
        translation = _externalPoseSet._absolutePoses[jointIndex].trans;
        return true;
    } else {
        return false;
    }
}

bool Rig::getJointCombinedRotation(int jointIndex, glm::quat& result, const glm::quat& rotation) const {
    // AJT: TODO: used by attachments
    ASSERT(false);
    return false;
}

void Rig::calcAnimAlpha(float speed, const std::vector<float>& referenceSpeeds, float* alphaOut) const {

    ASSERT(referenceSpeeds.size() > 0);

    // calculate alpha from linear combination of referenceSpeeds.
    float alpha = 0.0f;
    if (speed <= referenceSpeeds.front()) {
        alpha = 0.0f;
    } else if (speed > referenceSpeeds.back()) {
        alpha = (float)(referenceSpeeds.size() - 1);
    } else {
        for (size_t i = 0; i < referenceSpeeds.size() - 1; i++) {
            if (referenceSpeeds[i] < speed && speed < referenceSpeeds[i + 1]) {
                alpha = (float)i + ((speed - referenceSpeeds[i]) / (referenceSpeeds[i + 1] - referenceSpeeds[i]));
                break;
            }
        }
    }

    *alphaOut = alpha;
}

void Rig::setEnableInverseKinematics(bool enable) {
    _enableInverseKinematics = enable;
}

AnimPose Rig::getAbsoluteDefaultPose(int index) const {
    if (_animSkeleton && index >= 0 && index < _animSkeleton->getNumJoints()) {
        return _absoluteDefaultPoses[index];
    } else {
        return AnimPose::identity;
    }
}

const AnimPoseVec& Rig::getAbsoluteDefaultPoses() const {
    return _absoluteDefaultPoses;
}


bool Rig::getRelativeDefaultJointRotation(int index, glm::quat& rotationOut) const {
    if (_animSkeleton && index >= 0 && index < _animSkeleton->getNumJoints()) {
        rotationOut = _animSkeleton->getRelativeDefaultPose(index).rot;
        return true;
    } else {
        return false;
    }
}

bool Rig::getRelativeDefaultJointTranslation(int index, glm::vec3& translationOut) const {
    if (_animSkeleton && index >= 0 && index < _animSkeleton->getNumJoints()) {
        translationOut = _animSkeleton->getRelativeDefaultPose(index).trans;
        return true;
    } else {
        return false;
    }
}

// animation reference speeds.
static const std::vector<float> FORWARD_SPEEDS = { 0.4f, 1.4f, 4.5f }; // m/s
static const std::vector<float> BACKWARD_SPEEDS = { 0.6f, 1.45f }; // m/s
static const std::vector<float> LATERAL_SPEEDS = { 0.2f, 0.65f }; // m/s

void Rig::computeMotionAnimationState(float deltaTime, const glm::vec3& worldPosition, const glm::vec3& worldVelocity, const glm::quat& worldRotation, CharacterControllerState ccState) {

    glm::vec3 front = worldRotation * IDENTITY_FRONT;
    glm::vec3 workingVelocity = worldVelocity;

    {
        glm::vec3 localVel = glm::inverse(worldRotation) * workingVelocity;

        float forwardSpeed = glm::dot(localVel, IDENTITY_FRONT);
        float lateralSpeed = glm::dot(localVel, IDENTITY_RIGHT);
        float turningSpeed = glm::orientedAngle(front, _lastFront, IDENTITY_UP) / deltaTime;

        // filter speeds using a simple moving average.
        _averageForwardSpeed.updateAverage(forwardSpeed);
        _averageLateralSpeed.updateAverage(lateralSpeed);

        // sine wave LFO var for testing.
        static float t = 0.0f;
        _animVars.set("sine", 2.0f * static_cast<float>(0.5 * sin(t) + 0.5));

        float moveForwardAlpha = 0.0f;
        float moveBackwardAlpha = 0.0f;
        float moveLateralAlpha = 0.0f;

        // calcuate the animation alpha and timeScale values based on current speeds and animation reference speeds.
        calcAnimAlpha(_averageForwardSpeed.getAverage(), FORWARD_SPEEDS, &moveForwardAlpha);
        calcAnimAlpha(-_averageForwardSpeed.getAverage(), BACKWARD_SPEEDS, &moveBackwardAlpha);
        calcAnimAlpha(fabsf(_averageLateralSpeed.getAverage()), LATERAL_SPEEDS, &moveLateralAlpha);

        _animVars.set("moveForwardSpeed", _averageForwardSpeed.getAverage());
        _animVars.set("moveForwardAlpha", moveForwardAlpha);

        _animVars.set("moveBackwardSpeed", -_averageForwardSpeed.getAverage());
        _animVars.set("moveBackwardAlpha", moveBackwardAlpha);

        _animVars.set("moveLateralSpeed", fabsf(_averageLateralSpeed.getAverage()));
        _animVars.set("moveLateralAlpha", moveLateralAlpha);

        const float MOVE_ENTER_SPEED_THRESHOLD = 0.2f; // m/sec
        const float MOVE_EXIT_SPEED_THRESHOLD = 0.07f;  // m/sec
        const float TURN_ENTER_SPEED_THRESHOLD = 0.5f; // rad/sec
        const float TURN_EXIT_SPEED_THRESHOLD = 0.2f; // rad/sec

        if (ccState == CharacterControllerState::Hover) {
            if (_desiredState != RigRole::Hover) {
                _desiredStateAge = 0.0f;
            }
            _desiredState = RigRole::Hover;
        } else if (ccState == CharacterControllerState::InAir) {
            if (_desiredState != RigRole::InAir) {
                _desiredStateAge = 0.0f;
            }
            _desiredState = RigRole::InAir;
        } else if (ccState == CharacterControllerState::Takeoff) {
            if (_desiredState != RigRole::Takeoff) {
                _desiredStateAge = 0.0f;
            }
            _desiredState = RigRole::Takeoff;
        } else {
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
                if (_desiredState != RigRole::Move) {
                    _desiredStateAge = 0.0f;
                }
                _desiredState = RigRole::Move;
            } else {
                if (fabsf(turningSpeed) > turnThresh) {
                    if (_desiredState != RigRole::Turn) {
                        _desiredStateAge = 0.0f;
                    }
                    _desiredState = RigRole::Turn;
                } else { // idle
                    if (_desiredState != RigRole::Idle) {
                        _desiredStateAge = 0.0f;
                    }
                    _desiredState = RigRole::Idle;
                }
            }
        }

        const float STATE_CHANGE_HYSTERESIS_TIMER = 0.1f;

        // Skip hystersis timer for jump transitions.
        if (_desiredState == RigRole::Takeoff) {
            _desiredStateAge = STATE_CHANGE_HYSTERESIS_TIMER;
        } else if (_state == RigRole::Takeoff && _desiredState == RigRole::InAir) {
            _desiredStateAge = STATE_CHANGE_HYSTERESIS_TIMER;
        } else if (_state == RigRole::InAir && _desiredState != RigRole::InAir) {
            _desiredStateAge = STATE_CHANGE_HYSTERESIS_TIMER;
        }

        if ((_desiredStateAge >= STATE_CHANGE_HYSTERESIS_TIMER) && _desiredState != _state) {
            _state = _desiredState;
            _desiredStateAge = 0.0f;
        }

        _desiredStateAge += deltaTime;

        if (_state == RigRole::Move) {
            glm::vec3 horizontalVel = localVel - glm::vec3(0.0f, localVel.y, 0.0f);
            if (glm::length(horizontalVel) > MOVE_ENTER_SPEED_THRESHOLD) {
                if (fabsf(forwardSpeed) > 0.5f * fabsf(lateralSpeed)) {
                    if (forwardSpeed > 0.0f) {
                        // forward
                        _animVars.set("isMovingForward", true);
                        _animVars.set("isMovingBackward", false);
                        _animVars.set("isMovingRight", false);
                        _animVars.set("isMovingLeft", false);
                        _animVars.set("isNotMoving", false);

                    } else {
                        // backward
                        _animVars.set("isMovingBackward", true);
                        _animVars.set("isMovingForward", false);
                        _animVars.set("isMovingRight", false);
                        _animVars.set("isMovingLeft", false);
                        _animVars.set("isNotMoving", false);
                    }
                } else {
                    if (lateralSpeed > 0.0f) {
                        // right
                        _animVars.set("isMovingRight", true);
                        _animVars.set("isMovingLeft", false);
                        _animVars.set("isMovingForward", false);
                        _animVars.set("isMovingBackward", false);
                        _animVars.set("isNotMoving", false);
                    } else {
                        // left
                        _animVars.set("isMovingLeft", true);
                        _animVars.set("isMovingRight", false);
                        _animVars.set("isMovingForward", false);
                        _animVars.set("isMovingBackward", false);
                        _animVars.set("isNotMoving", false);
                    }
                }
            }
            _animVars.set("isTurningLeft", false);
            _animVars.set("isTurningRight", false);
            _animVars.set("isNotTurning", true);
            _animVars.set("isFlying", false);
            _animVars.set("isNotFlying", true);
            _animVars.set("isTakeoffStand", false);
            _animVars.set("isTakeoffRun", false);
            _animVars.set("isNotTakeoff", true);
            _animVars.set("isInAirStand", false);
            _animVars.set("isInAirRun", false);
            _animVars.set("isNotInAir", true);

        } else if (_state == RigRole::Turn) {
            if (turningSpeed > 0.0f) {
                // turning right
                _animVars.set("isTurningRight", true);
                _animVars.set("isTurningLeft", false);
                _animVars.set("isNotTurning", false);
            } else {
                // turning left
                _animVars.set("isTurningLeft", true);
                _animVars.set("isTurningRight", false);
                _animVars.set("isNotTurning", false);
            }
            _animVars.set("isMovingForward", false);
            _animVars.set("isMovingBackward", false);
            _animVars.set("isMovingRight", false);
            _animVars.set("isMovingLeft", false);
            _animVars.set("isNotMoving", true);
            _animVars.set("isFlying", false);
            _animVars.set("isNotFlying", true);
            _animVars.set("isTakeoffStand", false);
            _animVars.set("isTakeoffRun", false);
            _animVars.set("isNotTakeoff", true);
            _animVars.set("isInAirStand", false);
            _animVars.set("isInAirRun", false);
            _animVars.set("isNotInAir", true);

        } else if (_state == RigRole::Idle ) {
            // default anim vars to notMoving and notTurning
            _animVars.set("isMovingForward", false);
            _animVars.set("isMovingBackward", false);
            _animVars.set("isMovingLeft", false);
            _animVars.set("isMovingRight", false);
            _animVars.set("isNotMoving", true);
            _animVars.set("isTurningLeft", false);
            _animVars.set("isTurningRight", false);
            _animVars.set("isNotTurning", true);
            _animVars.set("isFlying", false);
            _animVars.set("isNotFlying", true);
            _animVars.set("isTakeoffStand", false);
            _animVars.set("isTakeoffRun", false);
            _animVars.set("isNotTakeoff", true);
            _animVars.set("isInAirStand", false);
            _animVars.set("isInAirRun", false);
            _animVars.set("isNotInAir", true);

        } else if (_state == RigRole::Hover) {
            // flying.
            _animVars.set("isMovingForward", false);
            _animVars.set("isMovingBackward", false);
            _animVars.set("isMovingLeft", false);
            _animVars.set("isMovingRight", false);
            _animVars.set("isNotMoving", true);
            _animVars.set("isTurningLeft", false);
            _animVars.set("isTurningRight", false);
            _animVars.set("isNotTurning", true);
            _animVars.set("isFlying", true);
            _animVars.set("isNotFlying", false);
            _animVars.set("isTakeoffStand", false);
            _animVars.set("isTakeoffRun", false);
            _animVars.set("isNotTakeoff", true);
            _animVars.set("isInAirStand", false);
            _animVars.set("isInAirRun", false);
            _animVars.set("isNotInAir", true);

        } else if (_state == RigRole::Takeoff) {
            // jumping in-air
            _animVars.set("isMovingForward", false);
            _animVars.set("isMovingBackward", false);
            _animVars.set("isMovingLeft", false);
            _animVars.set("isMovingRight", false);
            _animVars.set("isNotMoving", true);
            _animVars.set("isTurningLeft", false);
            _animVars.set("isTurningRight", false);
            _animVars.set("isNotTurning", true);
            _animVars.set("isFlying", false);
            _animVars.set("isNotFlying", true);

            bool takeOffRun = forwardSpeed > 0.1f;
            if (takeOffRun) {
                _animVars.set("isTakeoffStand", false);
                _animVars.set("isTakeoffRun", true);
            } else {
                _animVars.set("isTakeoffStand", true);
                _animVars.set("isTakeoffRun", false);
            }

            _animVars.set("isNotTakeoff", false);
            _animVars.set("isInAirStand", false);
            _animVars.set("isInAirRun", false);
            _animVars.set("isNotInAir", false);

        } else if (_state == RigRole::InAir) {
            // jumping in-air
            _animVars.set("isMovingForward", false);
            _animVars.set("isMovingBackward", false);
            _animVars.set("isMovingLeft", false);
            _animVars.set("isMovingRight", false);
            _animVars.set("isNotMoving", true);
            _animVars.set("isTurningLeft", false);
            _animVars.set("isTurningRight", false);
            _animVars.set("isNotTurning", true);
            _animVars.set("isFlying", false);
            _animVars.set("isNotFlying", true);
            _animVars.set("isTakeoffStand", false);
            _animVars.set("isTakeoffRun", false);
            _animVars.set("isNotTakeoff", true);

            bool inAirRun = forwardSpeed > 0.1f;
            if (inAirRun) {
                _animVars.set("isInAirStand", false);
                _animVars.set("isInAirRun", true);
            } else {
                _animVars.set("isInAirStand", true);
                _animVars.set("isInAirRun", false);
            }
            _animVars.set("isNotInAir", false);

            // compute blend based on velocity
            const float JUMP_SPEED = 3.5f;
            float alpha = glm::clamp(-_lastVelocity.y / JUMP_SPEED, -1.0f, 1.0f) + 1.0f;
            _animVars.set("inAirAlpha", alpha);
        }

        t += deltaTime;

        if (_enableInverseKinematics != _lastEnableInverseKinematics) {
            if (_enableInverseKinematics) {
                _animVars.set("ikOverlayAlpha", 1.0f);
            } else {
                _animVars.set("ikOverlayAlpha", 0.0f);
            }
        }
        _lastEnableInverseKinematics = _enableInverseKinematics;
    }

    _lastFront = front;
    _lastPosition = worldPosition;
    _lastVelocity = workingVelocity;
}

// Allow script to add/remove handlers and report results, from within their thread.
QScriptValue Rig::addAnimationStateHandler(QScriptValue handler, QScriptValue propertiesList) { // called in script thread

    // validate argument types
    if (handler.isFunction() && (isListOfStrings(propertiesList) || propertiesList.isUndefined() || propertiesList.isNull())) {
        QMutexLocker locker(&_stateMutex);
        // Find a safe id, even if there are lots of many scripts add and remove handlers repeatedly.
        while (!_nextStateHandlerId || _stateHandlers.contains(_nextStateHandlerId)) { // 0 is unused, and don't reuse existing after wrap.
            _nextStateHandlerId++;
        }
        StateHandler& data = _stateHandlers[_nextStateHandlerId];
        data.function = handler;
        data.useNames = propertiesList.isArray();
        if (data.useNames) {
            data.propertyNames = propertiesList.toVariant().toStringList();
        }
        return QScriptValue(_nextStateHandlerId); // suitable for giving to removeAnimationStateHandler
    } else {
        qCWarning(animation) << "Rig::addAnimationStateHandler invalid arguments, expected (function, string[])";
        return QScriptValue(QScriptValue::UndefinedValue);
    }
}

void Rig::removeAnimationStateHandler(QScriptValue identifier) { // called in script thread
    // validate arguments
    if (identifier.isNumber()) {
        QMutexLocker locker(&_stateMutex);
        _stateHandlers.remove(identifier.toInt32()); // silently continues if handler not present. 0 is unused
    } else {
        qCWarning(animation) << "Rig::removeAnimationStateHandler invalid argument, expected a number";
    }
}

void Rig::animationStateHandlerResult(int identifier, QScriptValue result) { // called synchronously from script
    QMutexLocker locker(&_stateMutex);
    auto found = _stateHandlers.find(identifier);
    if (found == _stateHandlers.end()) {
        return; // Don't use late-breaking results that got reported after the handler was removed.
    }
    found.value().results.animVariantMapFromScriptValue(result); // Into our own copy.
}

void Rig::updateAnimationStateHandlers() { // called on avatar update thread (which may be main thread)
    QMutexLocker locker(&_stateMutex);
    // It might pay to produce just one AnimVariantMap copy here, with a union of all the requested propertyNames,
    // rather than having each callAnimationStateHandler invocation make its own copy.
    // However, that copying is done on the script's own time rather than ours, so even if it's less cpu, it would be more
    // work on the avatar update thread (which is possibly the main thread).
    for (auto data = _stateHandlers.begin(); data != _stateHandlers.end(); data++) {
        // call out:
        int identifier = data.key();
        StateHandler& value = data.value();
        QScriptValue& function = value.function;
        auto handleResult = [this, identifier](QScriptValue result) { // called in script thread to get the result back to us.
            animationStateHandlerResult(identifier, result);
        };
        // invokeMethod makes a copy of the args, and copies of AnimVariantMap do copy the underlying map, so this will correctly capture
        // the state of _animVars and allow continued changes to _animVars in this thread without conflict.
        QMetaObject::invokeMethod(function.engine(), "callAnimationStateHandler",  Qt::QueuedConnection,
                                  Q_ARG(QScriptValue, function),
                                  Q_ARG(AnimVariantMap, _animVars),
                                  Q_ARG(QStringList, value.propertyNames),
                                  Q_ARG(bool, value.useNames),
                                  Q_ARG(AnimVariantResultHandler, handleResult));
        // It turns out that, for thread-safety reasons, ScriptEngine::callAnimationStateHandler will invoke itself if called from other
        // than the script thread. Thus the above _could_ be replaced with an ordinary call, which will then trigger the same
        // invokeMethod as is done explicitly above. However, the script-engine library depends on this animation library, not vice versa.
        // We could create an AnimVariantCallingMixin class in shared, with an abstract virtual slot
        // AnimVariantCallingMixin::callAnimationStateHandler (and move AnimVariantMap/AnimVaraintResultHandler to shared), but the
        // call site here would look like this instead of the above:
        //   dynamic_cast<AnimVariantCallingMixin*>(function.engine())->callAnimationStateHandler(function, ..., handleResult);
        // This works (I tried it), but the result would be that we would still have same runtime type checks as the invokeMethod above
        // (occuring within the ScriptEngine::callAnimationStateHandler invokeMethod trampoline), _plus_ another runtime check for the dynamic_cast.

        // Gather results in (likely from an earlier update).
        // Note: the behavior is undefined if a handler (re-)sets a trigger. Scripts should not be doing that.
        _animVars.copyVariantsFrom(value.results); // If multiple handlers write the same anim var, the last registgered wins. (_map preserves order).
    }
}

void Rig::updateAnimations(float deltaTime, glm::mat4 rootTransform) {

    PROFILE_RANGE_EX(__FUNCTION__, 0xffff00ff, 0);

    setModelOffset(rootTransform);

    if (_animNode) {

        updateAnimationStateHandlers();
        _animVars.setRigToGeometryTransform(_rigToGeometryTransform);

        // evaluate the animation
        AnimNode::Triggers triggersOut;
        _internalPoseSet._relativePoses = _animNode->evaluate(_animVars, deltaTime, triggersOut);
        if ((int)_internalPoseSet._relativePoses.size() != _animSkeleton->getNumJoints()) {
            // animations haven't fully loaded yet.
            _internalPoseSet._relativePoses = _animSkeleton->getRelativeDefaultPoses();
        }
        _animVars.clearTriggers();
        for (auto& trigger : triggersOut) {
            _animVars.setTrigger(trigger);
        }
    }

    applyOverridePoses();
    buildAbsoluteRigPoses(_internalPoseSet._relativePoses, _internalPoseSet._absolutePoses);

    // copy internal poses to external poses
    {
        QWriteLocker writeLock(&_externalPoseSetLock);
        _externalPoseSet = _internalPoseSet;
    }
}

void Rig::inverseKinematics(int endIndex, glm::vec3 targetPosition, const glm::quat& targetRotation, float priority,
                            const QVector<int>& freeLineage, glm::mat4 rootTransform) {
    ASSERT(false);
}

bool Rig::restoreJointPosition(int jointIndex, float fraction, float priority, const QVector<int>& freeLineage) {
    ASSERT(false);
    return false;
}

float Rig::getLimbLength(int jointIndex, const QVector<int>& freeLineage,
                         const glm::vec3 scale, const QVector<FBXJoint>& fbxJoints) const {
    ASSERT(false);
    return 1.0f;
}

glm::quat Rig::setJointRotationInBindFrame(int jointIndex, const glm::quat& rotation, float priority) {
    ASSERT(false);
    return glm::quat();
}

glm::vec3 Rig::getJointDefaultTranslationInConstrainedFrame(int jointIndex) {
    ASSERT(false);
    return glm::vec3();
}

glm::quat Rig::setJointRotationInConstrainedFrame(int jointIndex, glm::quat targetRotation, float priority, float mix) {
    ASSERT(false);
    return glm::quat();
}

bool Rig::getJointRotationInConstrainedFrame(int jointIndex, glm::quat& quatOut) const {
    ASSERT(false);
    return false;
}

void Rig::clearJointStatePriorities() {
    ASSERT(false);
}

glm::quat Rig::getJointDefaultRotationInParentFrame(int jointIndex) {
    ASSERT(false);
    return glm::quat();
}

void Rig::updateFromHeadParameters(const HeadParameters& params, float dt) {
    updateNeckJoint(params.neckJointIndex, params);

    _animVars.set("isTalking", params.isTalking);
    _animVars.set("notIsTalking", !params.isTalking);
}

void Rig::updateFromEyeParameters(const EyeParameters& params) {
    updateEyeJoint(params.leftEyeJointIndex, params.modelTranslation, params.modelRotation,
                   params.worldHeadOrientation, params.eyeLookAt, params.eyeSaccade);
    updateEyeJoint(params.rightEyeJointIndex, params.modelTranslation, params.modelRotation,
                   params.worldHeadOrientation, params.eyeLookAt, params.eyeSaccade);
}

void Rig::computeHeadNeckAnimVars(const AnimPose& hmdPose, glm::vec3& headPositionOut, glm::quat& headOrientationOut,
                                  glm::vec3& neckPositionOut, glm::quat& neckOrientationOut) const {

    // the input hmd values are in avatar/rig space
    const glm::vec3 hmdPosition = hmdPose.trans;
    const glm::quat hmdOrientation = hmdPose.rot;

    // TODO: cache jointIndices
    int rightEyeIndex = indexOfJoint("RightEye");
    int leftEyeIndex = indexOfJoint("LeftEye");
    int headIndex = indexOfJoint("Head");
    int neckIndex = indexOfJoint("Neck");

    glm::vec3 absRightEyePos = rightEyeIndex != -1 ? getAbsoluteDefaultPose(rightEyeIndex).trans : DEFAULT_RIGHT_EYE_POS;
    glm::vec3 absLeftEyePos = leftEyeIndex != -1 ? getAbsoluteDefaultPose(leftEyeIndex).trans : DEFAULT_LEFT_EYE_POS;
    glm::vec3 absHeadPos = headIndex != -1 ? getAbsoluteDefaultPose(headIndex).trans : DEFAULT_HEAD_POS;
    glm::vec3 absNeckPos = neckIndex != -1 ? getAbsoluteDefaultPose(neckIndex).trans : DEFAULT_NECK_POS;

    glm::vec3 absCenterEyePos = (absRightEyePos + absLeftEyePos) / 2.0f;
    glm::vec3 eyeOffset = absCenterEyePos - absHeadPos;
    glm::vec3 headOffset = absHeadPos - absNeckPos;

    // apply simplistic head/neck model

    // head
    headPositionOut = hmdPosition - hmdOrientation * eyeOffset;
    headOrientationOut = hmdOrientation;

    // neck
    neckPositionOut = hmdPosition - hmdOrientation * (headOffset + eyeOffset);

    // slerp between default orientation and hmdOrientation
    neckOrientationOut = safeMix(hmdOrientation, _animSkeleton->getRelativeDefaultPose(neckIndex).rot, 0.5f);
}

void Rig::updateNeckJoint(int index, const HeadParameters& params) {
    if (_animSkeleton && index >= 0 && index < _animSkeleton->getNumJoints()) {
        glm::quat yFlip180 = glm::angleAxis(PI, glm::vec3(0.0f, 1.0f, 0.0f));
        if (params.isInHMD) {
            glm::vec3 headPos, neckPos;
            glm::quat headRot, neckRot;

            AnimPose hmdPose(glm::vec3(1.0f), params.rigHeadOrientation * yFlip180, params.rigHeadPosition);
            computeHeadNeckAnimVars(hmdPose, headPos, headRot, neckPos, neckRot);

            // debug rendering
#ifdef DEBUG_RENDERING
            const glm::vec4 red(1.0f, 0.0f, 0.0f, 1.0f);
            const glm::vec4 green(0.0f, 1.0f, 0.0f, 1.0f);

            // transform from bone into avatar space
            AnimPose headPose(glm::vec3(1), headRot, headPos);
            DebugDraw::getInstance().addMyAvatarMarker("headTarget", headPose.rot, headPose.trans, red);

            // transform from bone into avatar space
            AnimPose neckPose(glm::vec3(1), neckRot, neckPos);
            DebugDraw::getInstance().addMyAvatarMarker("neckTarget", neckPose.rot, neckPose.trans, green);
#endif

            _animVars.set("headPosition", headPos);
            _animVars.set("headRotation", headRot);
            _animVars.set("headType", (int)IKTarget::Type::HmdHead);
            _animVars.set("neckPosition", neckPos);
            _animVars.set("neckRotation", neckRot);
            _animVars.set("neckType", (int)IKTarget::Type::Unknown); // 'Unknown' disables the target

        } else {
            _animVars.unset("headPosition");
            _animVars.set("headRotation", params.rigHeadOrientation * yFlip180);
            _animVars.set("headAndNeckType", (int)IKTarget::Type::RotationOnly);
            _animVars.set("headType", (int)IKTarget::Type::RotationOnly);
            _animVars.unset("neckPosition");
            _animVars.unset("neckRotation");
            _animVars.set("neckType", (int)IKTarget::Type::RotationOnly);
        }
    }
}

void Rig::updateEyeJoint(int index, const glm::vec3& modelTranslation, const glm::quat& modelRotation, const glm::quat& worldHeadOrientation, const glm::vec3& lookAtSpot, const glm::vec3& saccade) {

    // TODO: does not properly handle avatar scale.

    if (isIndexValid(index)) {
        glm::mat4 rigToWorld = createMatFromQuatAndPos(modelRotation, modelTranslation);
        glm::mat4 worldToRig = glm::inverse(rigToWorld);
        glm::vec3 lookAtVector = glm::normalize(transformPoint(worldToRig, lookAtSpot) - _internalPoseSet._absolutePoses[index].trans);

        int headIndex = indexOfJoint("Head");
        glm::quat headQuat;
        if (headIndex >= 0) {
            headQuat = _internalPoseSet._absolutePoses[headIndex].rot;
        }

        glm::vec3 headUp = headQuat * Vectors::UNIT_Y;
        glm::vec3 z, y, x;
        generateBasisVectors(lookAtVector, headUp, z, y, x);
        glm::mat3 m(glm::cross(y, z), y, z);
        glm::quat desiredQuat = glm::normalize(glm::quat_cast(m));

        glm::quat deltaQuat = desiredQuat * glm::inverse(headQuat);

        // limit swing rotation of the deltaQuat by a 30 degree cone.
        // TODO: use swing twist decomposition constraint instead, for off axis rotation clamping.
        const float MAX_ANGLE = 30.0f * RADIANS_PER_DEGREE;
        if (fabsf(glm::angle(deltaQuat)) > MAX_ANGLE) {
            deltaQuat = glm::angleAxis(glm::clamp(glm::angle(deltaQuat), -MAX_ANGLE, MAX_ANGLE), glm::axis(deltaQuat));
        }

        // directly set absolutePose rotation
        _internalPoseSet._absolutePoses[index].rot = deltaQuat * headQuat;
    }
}

void Rig::updateFromHandParameters(const HandParameters& params, float dt) {
    if (_animSkeleton && _animNode) {

        const float HAND_RADIUS = 0.05f;
        int hipsIndex = indexOfJoint("Hips");
        glm::vec3 hipsTrans;
        if (hipsIndex >= 0) {
            hipsTrans = _internalPoseSet._absolutePoses[hipsIndex].trans;
        }

        // Use this capsule to represent the avatar body.
        const float bodyCapsuleRadius = params.bodyCapsuleRadius;
        const glm::vec3 bodyCapsuleCenter = hipsTrans - params.bodyCapsuleLocalOffset;
        const glm::vec3 bodyCapsuleStart = bodyCapsuleCenter - glm::vec3(0, params.bodyCapsuleHalfHeight, 0);
        const glm::vec3 bodyCapsuleEnd = bodyCapsuleCenter + glm::vec3(0, params.bodyCapsuleHalfHeight, 0);

        if (params.isLeftEnabled) {

            // prevent the hand IK targets from intersecting the body capsule
            glm::vec3 handPosition = params.leftPosition;
            glm::vec3 displacement;
            if (findSphereCapsulePenetration(handPosition, HAND_RADIUS, bodyCapsuleStart, bodyCapsuleEnd, bodyCapsuleRadius, displacement)) {
                handPosition -= displacement;
            }

            _animVars.set("leftHandPosition", handPosition);
            _animVars.set("leftHandRotation", params.leftOrientation);
            _animVars.set("leftHandType", (int)IKTarget::Type::RotationAndPosition);
        } else {
            _animVars.unset("leftHandPosition");
            _animVars.unset("leftHandRotation");
            _animVars.set("leftHandType", (int)IKTarget::Type::HipsRelativeRotationAndPosition);
        }

        if (params.isRightEnabled) {

            // prevent the hand IK targets from intersecting the body capsule
            glm::vec3 handPosition = params.rightPosition;
            glm::vec3 displacement;
            if (findSphereCapsulePenetration(handPosition, HAND_RADIUS, bodyCapsuleStart, bodyCapsuleEnd, bodyCapsuleRadius, displacement)) {
                handPosition -= displacement;
            }

            _animVars.set("rightHandPosition", handPosition);
            _animVars.set("rightHandRotation", params.rightOrientation);
            _animVars.set("rightHandType", (int)IKTarget::Type::RotationAndPosition);
        } else {
            _animVars.unset("rightHandPosition");
            _animVars.unset("rightHandRotation");
            _animVars.set("rightHandType", (int)IKTarget::Type::HipsRelativeRotationAndPosition);
        }
    }
}

void Rig::initAnimGraph(const QUrl& url) {
    _animGraphURL = url;

    _animNode.reset();

    // load the anim graph
    _animLoader.reset(new AnimNodeLoader(url));
    connect(_animLoader.get(), &AnimNodeLoader::success, [this](AnimNode::Pointer nodeIn) {
        _animNode = nodeIn;
        _animNode->setSkeleton(_animSkeleton);

        if (_userAnimState.clipNodeEnum != UserAnimState::None) {
            // restore the user animation we had before reset.
            UserAnimState origState = _userAnimState;
            _userAnimState = { UserAnimState::None, "", 30.0f, false, 0.0f, 0.0f };
            overrideAnimation(origState.url, origState.fps, origState.loop, origState.firstFrame, origState.lastFrame);
        }

        emit onLoadComplete();
    });
    connect(_animLoader.get(), &AnimNodeLoader::error, [url](int error, QString str) {
        qCCritical(animation) << "Error loading" << url.toDisplayString() << "code = " << error << "str =" << str;
    });
}

bool Rig::getModelRegistrationPoint(glm::vec3& modelRegistrationPointOut) const {
    if (_animSkeleton && _rootJointIndex >= 0) {
        modelRegistrationPointOut = _geometryOffset * -_animSkeleton->getAbsoluteDefaultPose(_rootJointIndex).trans;
        return true;
    } else {
        return false;
    }
}

void Rig::applyOverridePoses() {
    if (!_animSkeleton) {
        return;
    }

    ASSERT(_animSkeleton->getNumJoints() == (int)_internalPoseSet._relativePoses.size());
    ASSERT(_animSkeleton->getNumJoints() == (int)_internalPoseSet._overrideFlags.size());
    ASSERT(_animSkeleton->getNumJoints() == (int)_internalPoseSet._overridePoses.size());

    for (size_t i = 0; i < _internalPoseSet._overrideFlags.size(); i++) {
        if (_internalPoseSet._overrideFlags[i]) {
            _internalPoseSet._relativePoses[i] = _internalPoseSet._overridePoses[i];
        }
    }
}

void Rig::buildAbsoluteRigPoses(const AnimPoseVec& relativePoses, AnimPoseVec& absolutePosesOut) {
    if (!_animSkeleton) {
        return;
    }

    ASSERT(_animSkeleton->getNumJoints() == (int)relativePoses.size());

    // flatten all poses out so they are absolute not relative
    absolutePosesOut.resize(relativePoses.size());
    for (int i = 0; i < (int)relativePoses.size(); i++) {
        int parentIndex = _animSkeleton->getParentIndex(i);
        if (parentIndex == -1) {
            absolutePosesOut[i] = relativePoses[i];
        } else {
            absolutePosesOut[i] = absolutePosesOut[parentIndex] * relativePoses[i];
        }
    }

    // transform all absolute poses into rig space.
    AnimPose geometryToRigTransform(_geometryToRigTransform);
    for (int i = 0; i < (int)absolutePosesOut.size(); i++) {
        absolutePosesOut[i] = geometryToRigTransform * absolutePosesOut[i];
    }
}

glm::mat4 Rig::getJointTransform(int jointIndex) const {
    if (isIndexValid(jointIndex)) {
        return _internalPoseSet._absolutePoses[jointIndex];
    } else {
        return glm::mat4();
    }
}

void Rig::copyJointsIntoJointData(QVector<JointData>& jointDataVec) const {

    const AnimPose geometryToRigPose(_geometryToRigTransform);

    jointDataVec.resize((int)getJointStateCount());
    for (auto i = 0; i < jointDataVec.size(); i++) {
        JointData& data = jointDataVec[i];
        if (isIndexValid(i)) {
            // rotations are in absolute rig frame.
            glm::quat defaultAbsRot = geometryToRigPose.rot * _animSkeleton->getAbsoluteDefaultPose(i).rot;
            data.rotation = _internalPoseSet._absolutePoses[i].rot;
            data.rotationSet = !isEqual(data.rotation, defaultAbsRot);

            // translations are in relative frame but scaled so that they are in meters,
            // instead of geometry units.
            glm::vec3 defaultRelTrans = _geometryOffset.scale * _animSkeleton->getRelativeDefaultPose(i).trans;
            data.translation = _geometryOffset.scale * _internalPoseSet._relativePoses[i].trans;
            data.translationSet = !isEqual(data.translation, defaultRelTrans);
        } else {
            data.translationSet = false;
            data.rotationSet = false;
        }
    }
}

void Rig::copyJointsFromJointData(const QVector<JointData>& jointDataVec) {
    if (_animSkeleton && jointDataVec.size() == (int)_internalPoseSet._overrideFlags.size()) {

        // transform all the default poses into rig space.
        const AnimPose geometryToRigPose(_geometryToRigTransform);
        std::vector<bool> overrideFlags(_internalPoseSet._overridePoses.size(), false);

        // start with the default rotations in absolute rig frame
        std::vector<glm::quat> rotations;
        rotations.reserve(_animSkeleton->getAbsoluteDefaultPoses().size());
        for (auto& pose : _animSkeleton->getAbsoluteDefaultPoses()) {
            rotations.push_back(geometryToRigPose.rot * pose.rot);
        }

        // start translations in relative frame but scaled to meters.
        std::vector<glm::vec3> translations;
        translations.reserve(_animSkeleton->getRelativeDefaultPoses().size());
        for (auto& pose : _animSkeleton->getRelativeDefaultPoses()) {
            translations.push_back(_geometryOffset.scale * pose.trans);
        }

        ASSERT(overrideFlags.size() == rotations.size());

        // copy over rotations from the jointDataVec, which is also in absolute rig frame
        for (int i = 0; i < jointDataVec.size(); i++) {
            if (isIndexValid(i)) {
                const JointData& data = jointDataVec.at(i);
                if (data.rotationSet) {
                    overrideFlags[i] = true;
                    rotations[i] = data.rotation;
                }
                if (data.translationSet) {
                    overrideFlags[i] = true;
                    translations[i] = data.translation;
                }
            }
        }

        ASSERT(_internalPoseSet._overrideFlags.size() == _internalPoseSet._overridePoses.size());

        // convert resulting rotations into geometry space.
        const glm::quat rigToGeometryRot(glmExtractRotation(_rigToGeometryTransform));
        for (auto& rot : rotations) {
            rot = rigToGeometryRot * rot;
        }

        // convert all rotations from absolute to parent relative.
        _animSkeleton->convertAbsoluteRotationsToRelative(rotations);

        // copy the geometry space parent relative poses into _overridePoses
        for (int i = 0; i < jointDataVec.size(); i++) {
            if (overrideFlags[i]) {
                _internalPoseSet._overrideFlags[i] = true;
                _internalPoseSet._overridePoses[i].scale = Vectors::ONE;
                _internalPoseSet._overridePoses[i].rot = rotations[i];
                // scale translations from meters back into geometry units.
                _internalPoseSet._overridePoses[i].trans = _invGeometryOffset.scale * translations[i];
            }
        }
    }
}

void Rig::computeAvatarBoundingCapsule(
        const FBXGeometry& geometry,
        float& radiusOut,
        float& heightOut,
        glm::vec3& localOffsetOut) const {
    if (! _animSkeleton) {
        const float DEFAULT_AVATAR_CAPSULE_RADIUS = 0.3f;
        const float DEFAULT_AVATAR_CAPSULE_HEIGHT = 1.3f;
        const glm::vec3 DEFAULT_AVATAR_CAPSULE_LOCAL_OFFSET = glm::vec3(0.0f, -0.25f, 0.0f);
        radiusOut = DEFAULT_AVATAR_CAPSULE_RADIUS;
        heightOut = DEFAULT_AVATAR_CAPSULE_HEIGHT;
        localOffsetOut = DEFAULT_AVATAR_CAPSULE_LOCAL_OFFSET;
        return;
    }

    AnimInverseKinematics ikNode("boundingShape");
    ikNode.setSkeleton(_animSkeleton);
    ikNode.setTargetVars("LeftHand",
                         "leftHandPosition",
                         "leftHandRotation",
                         "leftHandType");
    ikNode.setTargetVars("RightHand",
                         "rightHandPosition",
                         "rightHandRotation",
                         "rightHandType");
    ikNode.setTargetVars("LeftFoot",
                         "leftFootPosition",
                         "leftFootRotation",
                         "leftFootType");
    ikNode.setTargetVars("RightFoot",
                         "rightFootPosition",
                         "rightFootRotation",
                         "rightFootType");

    AnimPose geometryToRig = _modelOffset * _geometryOffset;

    AnimPose hips(glm::vec3(1), glm::quat(), glm::vec3());
    int hipsIndex = indexOfJoint("Hips");
    if (hipsIndex >= 0) {
        hips = geometryToRig * _animSkeleton->getAbsoluteBindPose(hipsIndex);
    }
    AnimVariantMap animVars;
    glm::quat handRotation = glm::angleAxis(PI, Vectors::UNIT_X);
    animVars.set("leftHandPosition", hips.trans);
    animVars.set("leftHandRotation", handRotation);
    animVars.set("leftHandType", (int)IKTarget::Type::RotationAndPosition);
    animVars.set("rightHandPosition", hips.trans);
    animVars.set("rightHandRotation", handRotation);
    animVars.set("rightHandType", (int)IKTarget::Type::RotationAndPosition);

    int rightFootIndex = indexOfJoint("RightFoot");
    int leftFootIndex = indexOfJoint("LeftFoot");
    if (rightFootIndex != -1 && leftFootIndex != -1) {
        glm::vec3 foot = Vectors::ZERO;
        glm::quat footRotation = glm::angleAxis(0.5f * PI, Vectors::UNIT_X);
        animVars.set("leftFootPosition", foot);
        animVars.set("leftFootRotation", footRotation);
        animVars.set("leftFootType", (int)IKTarget::Type::RotationAndPosition);
        animVars.set("rightFootPosition", foot);
        animVars.set("rightFootRotation", footRotation);
        animVars.set("rightFootType", (int)IKTarget::Type::RotationAndPosition);
    }

    // call overlay twice: once to verify AnimPoseVec joints and again to do the IK
    AnimNode::Triggers triggersOut;
    float dt = 1.0f; // the value of this does not matter
    ikNode.overlay(animVars, dt, triggersOut, _animSkeleton->getRelativeBindPoses());
    AnimPoseVec finalPoses =  ikNode.overlay(animVars, dt, triggersOut, _animSkeleton->getRelativeBindPoses());

    // convert relative poses to absolute
    _animSkeleton->convertRelativePosesToAbsolute(finalPoses);

    // compute bounding box that encloses all points
    Extents totalExtents;
    totalExtents.reset();

    // HACK by convention our Avatars are always modeled such that y=0 is the ground plane.
    // add the zero point so that our avatars will always have bounding volumes that are flush with the ground
    // even if they do not have legs (default robot)
    totalExtents.addPoint(glm::vec3(0.0f));

    // HACK to reduce the radius of the bounding capsule to be tight with the torso, we only consider joints
    // from the head to the hips when computing the rest of the bounding capsule.
    int index = indexOfJoint("Head");
    while (index != -1) {
        const FBXJointShapeInfo& shapeInfo = geometry.joints.at(index).shapeInfo;
        AnimPose pose = finalPoses[index];
        if (shapeInfo.points.size() > 0) {
            for (int j = 0; j < shapeInfo.points.size(); ++j) {
                totalExtents.addPoint((pose * shapeInfo.points[j]));
            }
        }
        index = _animSkeleton->getParentIndex(index);
    }

    // compute bounding shape parameters
    // NOTE: we assume that the longest side of totalExtents is the yAxis...
    glm::vec3 diagonal = (geometryToRig * totalExtents.maximum) - (geometryToRig * totalExtents.minimum);
    // ... and assume the radiusOut is half the RMS of the X and Z sides:
    radiusOut = 0.5f * sqrtf(0.5f * (diagonal.x * diagonal.x + diagonal.z * diagonal.z));
    heightOut = diagonal.y - 2.0f * radiusOut;

    glm::vec3 rootPosition = finalPoses[geometry.rootJointIndex].trans;
    glm::vec3 rigCenter = (geometryToRig * (0.5f * (totalExtents.maximum + totalExtents.minimum)));
    localOffsetOut = rigCenter - (geometryToRig * rootPosition);
}


