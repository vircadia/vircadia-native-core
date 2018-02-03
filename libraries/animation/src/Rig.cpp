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
#include <PerfStat.h>
#include <ScriptValueUtils.h>
#include <shared/NsightHelpers.h>

#include "AnimationLogging.h"
#include "AnimClip.h"
#include "AnimInverseKinematics.h"
#include "AnimOverlay.h"
#include "AnimSkeleton.h"
#include "AnimUtil.h"
#include "IKTarget.h"


static int nextRigId = 1;
static std::map<int, Rig*> rigRegistry;
static std::mutex rigRegistryMutex;

static bool isEqual(const glm::vec3& u, const glm::vec3& v) {
    const float EPSILON = 0.0001f;
    float uLen = glm::length(u);
    if (uLen == 0.0f) {
        return glm::length(v) <= EPSILON;
    } else {
        return (glm::length(u - v) / uLen) <= EPSILON;
    }
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

Rig::Rig() {
    // Ensure thread-safe access to the rigRegistry.
    std::lock_guard<std::mutex> guard(rigRegistryMutex);

    // Insert this newly allocated rig into the rig registry
    _rigId = nextRigId;
    rigRegistry[_rigId] = this;
    nextRigId++;
}

Rig::~Rig() {
    // Ensure thread-safe access to the rigRegstry, but also prevent the rig from being deleted
    // while Rig::animationStateHandlerResult is being invoked on a script thread.
    std::lock_guard<std::mutex> guard(rigRegistryMutex);
    auto iter = rigRegistry.find(_rigId);
    if (iter != rigRegistry.end()) {
        rigRegistry.erase(iter);
    }
}

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
            _roleAnimStates[role] = { role, url, fps, loop, firstFrame, lastFrame };
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

            auto statesIter = _roleAnimStates.find(role);
            if (statesIter != _roleAnimStates.end()) {
                _roleAnimStates.erase(statesIter);
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
    _numOverrides = 0;
}

void Rig::initJointStates(const FBXGeometry& geometry, const glm::mat4& modelOffset) {
    _geometryOffset = AnimPose(geometry.offset);
    _invGeometryOffset = _geometryOffset.inverse();
    _geometryToRigTransform = modelOffset * geometry.offset;
    _rigToGeometryTransform = glm::inverse(_geometryToRigTransform);
    setModelOffset(modelOffset);

    _animSkeleton = std::make_shared<AnimSkeleton>(geometry);

    _internalPoseSet._relativePoses.clear();
    _internalPoseSet._relativePoses = _animSkeleton->getRelativeDefaultPoses();

    buildAbsoluteRigPoses(_internalPoseSet._relativePoses, _internalPoseSet._absolutePoses);

    _internalPoseSet._overridePoses.clear();
    _internalPoseSet._overridePoses = _animSkeleton->getRelativeDefaultPoses();

    _internalPoseSet._overrideFlags.clear();
    _internalPoseSet._overrideFlags.resize(_animSkeleton->getNumJoints(), false);
    _numOverrides = 0;

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
    _numOverrides = 0;

    buildAbsoluteRigPoses(_animSkeleton->getRelativeDefaultPoses(), _absoluteDefaultPoses);

    _rootJointIndex = geometry.rootJointIndex;
    _leftHandJointIndex = geometry.leftHandJointIndex;
    _leftElbowJointIndex = _leftHandJointIndex >= 0 ? geometry.joints.at(_leftHandJointIndex).parentIndex : -1;
    _leftShoulderJointIndex = _leftElbowJointIndex >= 0 ? geometry.joints.at(_leftElbowJointIndex).parentIndex : -1;
    _rightHandJointIndex = geometry.rightHandJointIndex;
    _rightElbowJointIndex = _rightHandJointIndex >= 0 ? geometry.joints.at(_rightHandJointIndex).parentIndex : -1;
    _rightShoulderJointIndex = _rightElbowJointIndex >= 0 ? geometry.joints.at(_rightElbowJointIndex).parentIndex : -1;

    if (!_animGraphURL.isEmpty()) {
        _animNode.reset();
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
    if (!isEqual(_modelOffset.trans(), newModelOffset.trans()) ||
        !isEqual(_modelOffset.rot(), newModelOffset.rot()) ||
        !isEqual(_modelOffset.scale(), newModelOffset.scale())) {

        _modelOffset = newModelOffset;

        // compute geometryToAvatarTransforms
        _geometryToRigTransform = _modelOffset * _geometryOffset;
        _rigToGeometryTransform = glm::inverse(_geometryToRigTransform);

        // rebuild cached default poses
        if (_animSkeleton) {
            buildAbsoluteRigPoses(_animSkeleton->getRelativeDefaultPoses(), _absoluteDefaultPoses);
        }
    }
}

void Rig::clearJointState(int index) {
    if (isIndexValid(index)) {
        if (_internalPoseSet._overrideFlags[index]) {
            _internalPoseSet._overrideFlags[index] = false;
            --_numOverrides;
        }
        _internalPoseSet._overridePoses[index] = _animSkeleton->getRelativeDefaultPose(index);
    }
}

void Rig::clearJointStates() {
    _internalPoseSet._overrideFlags.clear();
    _numOverrides = 0;
    if (_animSkeleton) {
        _internalPoseSet._overrideFlags.resize(_animSkeleton->getNumJoints());
        _internalPoseSet._overridePoses = _animSkeleton->getRelativeDefaultPoses();
    }
}

void Rig::clearJointAnimationPriority(int index) {
    if (isIndexValid(index)) {
        if (_internalPoseSet._overrideFlags[index]) {
            _internalPoseSet._overrideFlags[index] = false;
            --_numOverrides;
        }
        _internalPoseSet._overridePoses[index] = _animSkeleton->getRelativeDefaultPose(index);
    }
}

std::shared_ptr<AnimInverseKinematics> Rig::getAnimInverseKinematicsNode() const {
    std::shared_ptr<AnimInverseKinematics> result;
    if (_animNode) {
        _animNode->traverse([&](AnimNode::Pointer node) {
            // only report clip nodes as valid roles.
            auto ikNode = std::dynamic_pointer_cast<AnimInverseKinematics>(node);
            if (ikNode) {
                result = ikNode;
                return false;
            } else {
                return true;
            }
        });
    }
    return result;
}

void Rig::clearIKJointLimitHistory() {
    auto ikNode = getAnimInverseKinematicsNode();
    if (ikNode) {
        ikNode->clearIKJointLimitHistory();
    }
}

float Rig::getIKErrorOnLastSolve() const {
    float result = 0.0f;

    if (_animNode) {
        _animNode->traverse([&](AnimNode::Pointer node) {
            auto ikNode = std::dynamic_pointer_cast<AnimInverseKinematics>(node);
            if (ikNode) {
                result = ikNode->getMaxErrorOnLastSolve();
            }
            return true;
        });
    }
    return result;
}

int Rig::getJointParentIndex(int childIndex) const {
    if (_animSkeleton && isIndexValid(childIndex)) {
        return _animSkeleton->getParentIndex(childIndex);
    }
    return -1;
}

void Rig::setJointTranslation(int index, bool valid, const glm::vec3& translation, float priority) {
    if (isIndexValid(index)) {
        if (valid) {
            assert(_internalPoseSet._overrideFlags.size() == _internalPoseSet._overridePoses.size());
            if (!_internalPoseSet._overrideFlags[index]) {
                _internalPoseSet._overrideFlags[index] = true;
                ++_numOverrides;
            }
            _internalPoseSet._overridePoses[index].trans() = translation;
        }
    }
}

void Rig::setJointState(int index, bool valid, const glm::quat& rotation, const glm::vec3& translation, float priority) {
    if (isIndexValid(index)) {
        assert(_internalPoseSet._overrideFlags.size() == _internalPoseSet._overridePoses.size());
        if (!_internalPoseSet._overrideFlags[index]) {
            _internalPoseSet._overrideFlags[index] = true;
            ++_numOverrides;
        }
        _internalPoseSet._overridePoses[index].rot() = rotation;
        _internalPoseSet._overridePoses[index].trans() = translation;
    }
}

void Rig::setJointRotation(int index, bool valid, const glm::quat& rotation, float priority) {
    if (isIndexValid(index)) {
        if (valid) {
            ASSERT(_internalPoseSet._overrideFlags.size() == _internalPoseSet._overridePoses.size());
            if (!_internalPoseSet._overrideFlags[index]) {
                _internalPoseSet._overrideFlags[index] = true;
                ++_numOverrides;
            }
            _internalPoseSet._overridePoses[index].rot() = rotation;
        }
    }
}

bool Rig::getJointPositionInWorldFrame(int jointIndex, glm::vec3& position, glm::vec3 translation, glm::quat rotation) const {
    bool success { false };
    if (QThread::currentThread() == thread()) {
        if (isIndexValid(jointIndex)) {
            position = (rotation * _internalPoseSet._absolutePoses[jointIndex].trans()) + translation;
            success = true;
        } else {
            success = false;
        }
    } else {
        QReadLocker readLock(&_externalPoseSetLock);
        if (jointIndex >= 0 && jointIndex < (int)_externalPoseSet._absolutePoses.size()) {
            position = (rotation * _externalPoseSet._absolutePoses[jointIndex].trans()) + translation;
            success = true;
        } else {
            success = false;
        }
    }

    if (isNaN(position)) {
        qCWarning(animation) << "Rig::getJointPositionInWorldFrame produces NaN";
        success = false;
        position = glm::vec3(0.0f);
    }

    return success;
}

bool Rig::getJointPosition(int jointIndex, glm::vec3& position) const {
    if (QThread::currentThread() == thread()) {
        if (isIndexValid(jointIndex)) {
            position = _internalPoseSet._absolutePoses[jointIndex].trans();
            return true;
        } else {
            return false;
        }
    } else {
        return getAbsoluteJointTranslationInRigFrame(jointIndex, position);
    }
}

bool Rig::getJointRotationInWorldFrame(int jointIndex, glm::quat& result, const glm::quat& rotation) const {
    if (QThread::currentThread() == thread()) {
        if (isIndexValid(jointIndex)) {
            result = rotation * _internalPoseSet._absolutePoses[jointIndex].rot();
            return true;
        } else {
            return false;
        }
    }

    QReadLocker readLock(&_externalPoseSetLock);
    if (jointIndex >= 0 && jointIndex < (int)_externalPoseSet._absolutePoses.size()) {
        result = rotation * _externalPoseSet._absolutePoses[jointIndex].rot();
        return true;
    } else {
        return false;
    }
}

bool Rig::getJointRotation(int jointIndex, glm::quat& rotation) const {
    if (QThread::currentThread() == thread()) {
        if (isIndexValid(jointIndex)) {
            rotation = _internalPoseSet._relativePoses[jointIndex].rot();
            return true;
        } else {
            return false;
        }
    }

    QReadLocker readLock(&_externalPoseSetLock);
    if (jointIndex >= 0 && jointIndex < (int)_externalPoseSet._relativePoses.size()) {
        rotation = _externalPoseSet._relativePoses[jointIndex].rot();
        return true;
    } else {
        return false;
    }
}

bool Rig::getAbsoluteJointRotationInRigFrame(int jointIndex, glm::quat& rotation) const {
    QReadLocker readLock(&_externalPoseSetLock);
    if (jointIndex >= 0 && jointIndex < (int)_externalPoseSet._absolutePoses.size()) {
        rotation = _externalPoseSet._absolutePoses[jointIndex].rot();
        return true;
    } else {
        return false;
    }
}

bool Rig::getJointTranslation(int jointIndex, glm::vec3& translation) const {
    QReadLocker readLock(&_externalPoseSetLock);
    if (jointIndex >= 0 && jointIndex < (int)_externalPoseSet._relativePoses.size()) {
        translation = _externalPoseSet._relativePoses[jointIndex].trans();
        return true;
    } else {
        return false;
    }
}

bool Rig::getAbsoluteJointTranslationInRigFrame(int jointIndex, glm::vec3& translation) const {
    QReadLocker readLock(&_externalPoseSetLock);
    if (jointIndex >= 0 && jointIndex < (int)_externalPoseSet._absolutePoses.size()) {
        translation = _externalPoseSet._absolutePoses[jointIndex].trans();
        return true;
    } else {
        return false;
    }
}

bool Rig::getAbsoluteJointPoseInRigFrame(int jointIndex, AnimPose& returnPose) const {
    QReadLocker readLock(&_externalPoseSetLock);
    if (jointIndex >= 0 && jointIndex < (int)_externalPoseSet._absolutePoses.size()) {
        returnPose = _externalPoseSet._absolutePoses[jointIndex];
        return true;
    } else {
        return false;
    }
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

void Rig::setEnableAnimations(bool enable) {
    _enabledAnimations = enable;
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
        rotationOut = _animSkeleton->getRelativeDefaultPose(index).rot();
        return true;
    } else {
        return false;
    }
}

bool Rig::getRelativeDefaultJointTranslation(int index, glm::vec3& translationOut) const {
    if (_animSkeleton && index >= 0 && index < _animSkeleton->getNumJoints()) {
        translationOut = _animSkeleton->getRelativeDefaultPose(index).trans();
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

    glm::vec3 forward = worldRotation * IDENTITY_FORWARD;
    glm::vec3 workingVelocity = worldVelocity;

    {
        glm::vec3 localVel = glm::inverse(worldRotation) * workingVelocity;

        float forwardSpeed = glm::dot(localVel, IDENTITY_FORWARD);
        float lateralSpeed = glm::dot(localVel, IDENTITY_RIGHT);
        float turningSpeed = glm::orientedAngle(forward, _lastForward, IDENTITY_UP) / deltaTime;

        // filter speeds using a simple moving average.
        _averageForwardSpeed.updateAverage(forwardSpeed);
        _averageLateralSpeed.updateAverage(lateralSpeed);

        // sine wave LFO var for testing.
        static float t = 0.0f;
        _animVars.set("sine", 2.0f * 0.5f * sinf(t) + 0.5f);

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

    _lastForward = forward;
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
        int rigId = _rigId;
        auto handleResult = [rigId, identifier](QScriptValue result) { // called in script thread to get the result back to us.
            // Hold the rigRegistryMutex to ensure thread-safe access to the rigRegistry, but
            // also to prevent the rig from being deleted while this lambda is being executed.
            std::lock_guard<std::mutex> guard(rigRegistryMutex);

            // if the rig pointer is in the registry, it has not been deleted yet.
            auto iter = rigRegistry.find(rigId);
            if (iter != rigRegistry.end()) {
                Rig* rig = iter->second;
                assert(rig);
                rig->animationStateHandlerResult(identifier, result);
            }
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

void Rig::updateAnimations(float deltaTime, const glm::mat4& rootTransform, const glm::mat4& rigToWorldTransform) {
    DETAILED_PROFILE_RANGE_EX(simulation_animation_detail, __FUNCTION__, 0xffff00ff, 0);
    DETAILED_PERFORMANCE_TIMER("updateAnimations");

    setModelOffset(rootTransform);

    if (_animNode && _enabledAnimations) {
        DETAILED_PERFORMANCE_TIMER("handleTriggers");

        updateAnimationStateHandlers();
        _animVars.setRigToGeometryTransform(_rigToGeometryTransform);

        AnimContext context(_enableDebugDrawIKTargets, _enableDebugDrawIKConstraints, _enableDebugDrawIKChains,
                            getGeometryToRigTransform(), rigToWorldTransform);

        // evaluate the animation
        AnimNode::Triggers triggersOut;

        _internalPoseSet._relativePoses = _animNode->evaluate(_animVars, context, deltaTime, triggersOut);
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

void Rig::updateFromEyeParameters(const EyeParameters& params) {
    updateEyeJoint(params.leftEyeJointIndex, params.modelTranslation, params.modelRotation, params.eyeLookAt, params.eyeSaccade);
    updateEyeJoint(params.rightEyeJointIndex, params.modelTranslation, params.modelRotation, params.eyeLookAt, params.eyeSaccade);
}

void Rig::computeHeadFromHMD(const AnimPose& hmdPose, glm::vec3& headPositionOut, glm::quat& headOrientationOut) const {

    // the input hmd values are in avatar/rig space
    const glm::vec3& hmdPosition = hmdPose.trans();

    // the HMD looks down the negative z axis, but the head bone looks down the z axis, so apply a 180 degree rotation.
    const glm::quat& hmdOrientation = hmdPose.rot() * Quaternions::Y_180;

    // TODO: cache jointIndices
    int rightEyeIndex = indexOfJoint("RightEye");
    int leftEyeIndex = indexOfJoint("LeftEye");
    int headIndex = indexOfJoint("Head");

    glm::vec3 absRightEyePos = rightEyeIndex != -1 ? getAbsoluteDefaultPose(rightEyeIndex).trans() : DEFAULT_RIGHT_EYE_POS;
    glm::vec3 absLeftEyePos = leftEyeIndex != -1 ? getAbsoluteDefaultPose(leftEyeIndex).trans() : DEFAULT_LEFT_EYE_POS;
    glm::vec3 absHeadPos = headIndex != -1 ? getAbsoluteDefaultPose(headIndex).trans() : DEFAULT_HEAD_POS;

    glm::vec3 absCenterEyePos = (absRightEyePos + absLeftEyePos) / 2.0f;
    glm::vec3 eyeOffset = absCenterEyePos - absHeadPos;

    headPositionOut = hmdPosition - hmdOrientation * eyeOffset;

    headOrientationOut = hmdOrientation;
}

void Rig::updateHead(bool headEnabled, bool hipsEnabled, const AnimPose& headPose) {
    if (_animSkeleton) {
        if (headEnabled) {
            _animVars.set("headPosition", headPose.trans());
            _animVars.set("headRotation", headPose.rot());
            if (hipsEnabled) {
                // Since there is an explicit hips ik target, switch the head to use the more flexible Spline IK chain type.
                // this will allow the spine to compress/expand and bend more natrually, ensuring that it can reach the head target position.
                _animVars.set("headType", (int)IKTarget::Type::Spline);
                _animVars.unset("headWeight");  // use the default weight for this target.
            } else {
                // When there is no hips IK target, use the HmdHead IK chain type.  This will make the spine very stiff,
                // but because the IK _hipsOffset is enabled, the hips will naturally follow underneath the head.
                _animVars.set("headType", (int)IKTarget::Type::HmdHead);
                _animVars.set("headWeight", 8.0f);
            }
        } else {
            _animVars.unset("headPosition");
            _animVars.set("headRotation", headPose.rot());
            _animVars.set("headType", (int)IKTarget::Type::RotationOnly);
        }
    }
}

const float INV_SQRT_3 = 1.0f / sqrtf(3.0f);
const int DOP14_COUNT = 14;
const glm::vec3 DOP14_NORMALS[DOP14_COUNT] = {
    Vectors::UNIT_X,
    -Vectors::UNIT_X,
    Vectors::UNIT_Y,
    -Vectors::UNIT_Y,
    Vectors::UNIT_Z,
    -Vectors::UNIT_Z,
    glm::vec3(INV_SQRT_3, INV_SQRT_3, INV_SQRT_3),
    -glm::vec3(INV_SQRT_3, INV_SQRT_3, INV_SQRT_3),
    glm::vec3(INV_SQRT_3, -INV_SQRT_3, INV_SQRT_3),
    -glm::vec3(INV_SQRT_3, -INV_SQRT_3, INV_SQRT_3),
    glm::vec3(INV_SQRT_3, INV_SQRT_3, -INV_SQRT_3),
    -glm::vec3(INV_SQRT_3, INV_SQRT_3, -INV_SQRT_3),
    glm::vec3(INV_SQRT_3, -INV_SQRT_3, -INV_SQRT_3),
    -glm::vec3(INV_SQRT_3, -INV_SQRT_3, -INV_SQRT_3)
};

// returns true if the given point lies inside of the k-dop, specified by shapeInfo & shapePose.
// if the given point does lie within the k-dop, it also returns the amount of displacement necessary to push that point outward
// such that it lies on the surface of the kdop.
static bool findPointKDopDisplacement(const glm::vec3& point, const AnimPose& shapePose, const FBXJointShapeInfo& shapeInfo, glm::vec3& displacementOut) {

    // transform point into local space of jointShape.
    glm::vec3 localPoint = shapePose.inverse().xformPoint(point);

    // Only works for 14-dop shape infos.
    if (shapeInfo.dots.size() != DOP14_COUNT) {
        return false;
    }

    glm::vec3 minDisplacement(FLT_MAX);
    float minDisplacementLen = FLT_MAX;
    glm::vec3 p = localPoint - shapeInfo.avgPoint;
    float pLen = glm::length(p);
    if (pLen > 0.0f) {
        int slabCount = 0;
        for (int i = 0; i < DOP14_COUNT; i++) {
            float dot = glm::dot(p, DOP14_NORMALS[i]);
            if (dot > 0.0f && dot < shapeInfo.dots[i]) {
                slabCount++;
                float distToPlane = pLen * (shapeInfo.dots[i] / dot);
                float displacementLen = distToPlane - pLen;

                // keep track of the smallest displacement
                if (displacementLen < minDisplacementLen) {
                    minDisplacementLen = displacementLen;
                    minDisplacement = (p / pLen) * displacementLen;
                }
            }
        }
        if (slabCount == (DOP14_COUNT / 2) && minDisplacementLen != FLT_MAX) {
            // we are within the k-dop so push the point along the minimum displacement found
            displacementOut = shapePose.xformVectorFast(minDisplacement);
            return true;
        } else {
            // point is outside of kdop
            return false;
        }
    } else {
        // point is directly on top of shapeInfo.avgPoint.
        // push the point out along the x axis.
        displacementOut = shapePose.xformVectorFast(shapeInfo.points[0]);
        return true;
    }
}

glm::vec3 Rig::deflectHandFromTorso(const glm::vec3& handPosition, const FBXJointShapeInfo& hipsShapeInfo, const FBXJointShapeInfo& spineShapeInfo,
                                    const FBXJointShapeInfo& spine1ShapeInfo, const FBXJointShapeInfo& spine2ShapeInfo) const {
    glm::vec3 position = handPosition;
    glm::vec3 displacement;
    int hipsJoint = indexOfJoint("Hips");
    if (hipsJoint >= 0) {
        AnimPose hipsPose;
        if (getAbsoluteJointPoseInRigFrame(hipsJoint, hipsPose)) {
            if (findPointKDopDisplacement(position, hipsPose, hipsShapeInfo, displacement)) {
                position += displacement;
            }
        }
    }

    int spineJoint = indexOfJoint("Spine");
    if (spineJoint >= 0) {
        AnimPose spinePose;
        if (getAbsoluteJointPoseInRigFrame(spineJoint, spinePose)) {
            if (findPointKDopDisplacement(position, spinePose, spineShapeInfo, displacement)) {
                position += displacement;
            }
        }
    }

    int spine1Joint = indexOfJoint("Spine1");
    if (spine1Joint >= 0) {
        AnimPose spine1Pose;
        if (getAbsoluteJointPoseInRigFrame(spine1Joint, spine1Pose)) {
            if (findPointKDopDisplacement(position, spine1Pose, spine1ShapeInfo, displacement)) {
                position += displacement;
            }
        }
    }

    int spine2Joint = indexOfJoint("Spine2");
    if (spine2Joint >= 0) {
        AnimPose spine2Pose;
        if (getAbsoluteJointPoseInRigFrame(spine2Joint, spine2Pose)) {
            if (findPointKDopDisplacement(position, spine2Pose, spine2ShapeInfo, displacement)) {
                position += displacement;
            }
        }
    }

    return position;
}

void Rig::updateHands(bool leftHandEnabled, bool rightHandEnabled, bool hipsEnabled, bool hipsEstimated,
                      bool leftArmEnabled, bool rightArmEnabled, float dt,
                      const AnimPose& leftHandPose, const AnimPose& rightHandPose,
                      const FBXJointShapeInfo& hipsShapeInfo, const FBXJointShapeInfo& spineShapeInfo,
                      const FBXJointShapeInfo& spine1ShapeInfo, const FBXJointShapeInfo& spine2ShapeInfo) {

    const float ELBOW_POLE_VECTOR_BLEND_FACTOR = 0.95f;

    int hipsIndex = indexOfJoint("Hips");

    if (leftHandEnabled) {

        glm::vec3 handPosition = leftHandPose.trans();
        glm::quat handRotation = leftHandPose.rot();

        if (!hipsEnabled || hipsEstimated) {
            // prevent the hand IK targets from intersecting the torso
            handPosition = deflectHandFromTorso(handPosition, hipsShapeInfo, spineShapeInfo, spine1ShapeInfo, spine2ShapeInfo);
        }

        _animVars.set("leftHandPosition", handPosition);
        _animVars.set("leftHandRotation", handRotation);
        _animVars.set("leftHandType", (int)IKTarget::Type::RotationAndPosition);

        // compute pole vector
        int handJointIndex = _animSkeleton->nameToJointIndex("LeftHand");
        int armJointIndex = _animSkeleton->nameToJointIndex("LeftArm");
        int elbowJointIndex = _animSkeleton->nameToJointIndex("LeftForeArm");
        if (!leftArmEnabled && elbowJointIndex >= 0 && armJointIndex >= 0 && elbowJointIndex >= 0) {
            glm::vec3 poleVector = calculateElbowPoleVector(handJointIndex, elbowJointIndex, armJointIndex, hipsIndex, true);

            // smooth toward desired pole vector from previous pole vector...  to reduce jitter
            if (!_prevLeftHandPoleVectorValid) {
                _prevLeftHandPoleVectorValid = true;
                _prevLeftHandPoleVector = poleVector;
            }
            glm::quat deltaRot = rotationBetween(_prevLeftHandPoleVector, poleVector);
            glm::quat smoothDeltaRot = safeMix(deltaRot, Quaternions::IDENTITY, ELBOW_POLE_VECTOR_BLEND_FACTOR);
            _prevLeftHandPoleVector = smoothDeltaRot * _prevLeftHandPoleVector;

            _animVars.set("leftHandPoleVectorEnabled", true);
            _animVars.set("leftHandPoleReferenceVector", Vectors::UNIT_X);
            _animVars.set("leftHandPoleVector", _prevLeftHandPoleVector);
        } else {
            _prevLeftHandPoleVectorValid = false;
            _animVars.set("leftHandPoleVectorEnabled", false);
        }
    } else {
        _prevLeftHandPoleVectorValid = false;
        _animVars.set("leftHandPoleVectorEnabled", false);

        _animVars.unset("leftHandPosition");
        _animVars.unset("leftHandRotation");
        _animVars.set("leftHandType", (int)IKTarget::Type::HipsRelativeRotationAndPosition);

    }

    if (rightHandEnabled) {

        glm::vec3 handPosition = rightHandPose.trans();
        glm::quat handRotation = rightHandPose.rot();

        if (!hipsEnabled || hipsEstimated) {
            // prevent the hand IK targets from intersecting the torso
            handPosition = deflectHandFromTorso(handPosition, hipsShapeInfo, spineShapeInfo, spine1ShapeInfo, spine2ShapeInfo);
        }

        _animVars.set("rightHandPosition", handPosition);
        _animVars.set("rightHandRotation", handRotation);
        _animVars.set("rightHandType", (int)IKTarget::Type::RotationAndPosition);

        // compute pole vector
        int handJointIndex = _animSkeleton->nameToJointIndex("RightHand");
        int armJointIndex = _animSkeleton->nameToJointIndex("RightArm");
        int elbowJointIndex = _animSkeleton->nameToJointIndex("RightForeArm");
        if (!rightArmEnabled && elbowJointIndex >= 0 && armJointIndex >= 0 && elbowJointIndex >= 0) {
            glm::vec3 poleVector = calculateElbowPoleVector(handJointIndex, elbowJointIndex, armJointIndex, hipsIndex, false);

            // smooth toward desired pole vector from previous pole vector...  to reduce jitter
            if (!_prevRightHandPoleVectorValid) {
                _prevRightHandPoleVectorValid = true;
                _prevRightHandPoleVector = poleVector;
            }
            glm::quat deltaRot = rotationBetween(_prevRightHandPoleVector, poleVector);
            glm::quat smoothDeltaRot = safeMix(deltaRot, Quaternions::IDENTITY, ELBOW_POLE_VECTOR_BLEND_FACTOR);
            _prevRightHandPoleVector = smoothDeltaRot * _prevRightHandPoleVector;

            _animVars.set("rightHandPoleVectorEnabled", true);
            _animVars.set("rightHandPoleReferenceVector", -Vectors::UNIT_X);
            _animVars.set("rightHandPoleVector", _prevRightHandPoleVector);
        } else {
            _prevRightHandPoleVectorValid = false;
            _animVars.set("rightHandPoleVectorEnabled", false);
        }
    } else {
        _prevRightHandPoleVectorValid = false;
        _animVars.set("rightHandPoleVectorEnabled", false);

        _animVars.unset("rightHandPosition");
        _animVars.unset("rightHandRotation");
        _animVars.set("rightHandType", (int)IKTarget::Type::HipsRelativeRotationAndPosition);
    }
}

void Rig::updateFeet(bool leftFootEnabled, bool rightFootEnabled, const AnimPose& leftFootPose, const AnimPose& rightFootPose) {

    const float KNEE_POLE_VECTOR_BLEND_FACTOR = 0.95f;

    int hipsIndex = indexOfJoint("Hips");

    if (leftFootEnabled) {
        _animVars.set("leftFootPosition", leftFootPose.trans());
        _animVars.set("leftFootRotation", leftFootPose.rot());
        _animVars.set("leftFootType", (int)IKTarget::Type::RotationAndPosition);

        int footJointIndex = _animSkeleton->nameToJointIndex("LeftFoot");
        int kneeJointIndex = _animSkeleton->nameToJointIndex("LeftLeg");
        int upLegJointIndex = _animSkeleton->nameToJointIndex("LeftUpLeg");
        glm::vec3 poleVector = calculateKneePoleVector(footJointIndex, kneeJointIndex, upLegJointIndex, hipsIndex, leftFootPose);

        // smooth toward desired pole vector from previous pole vector...  to reduce jitter
        if (!_prevLeftFootPoleVectorValid) {
            _prevLeftFootPoleVectorValid = true;
            _prevLeftFootPoleVector = poleVector;
        }
        glm::quat deltaRot = rotationBetween(_prevLeftFootPoleVector, poleVector);
        glm::quat smoothDeltaRot = safeMix(deltaRot, Quaternions::IDENTITY, KNEE_POLE_VECTOR_BLEND_FACTOR);
        _prevLeftFootPoleVector = smoothDeltaRot * _prevLeftFootPoleVector;

        _animVars.set("leftFootPoleVectorEnabled", true);
        _animVars.set("leftFootPoleReferenceVector", Vectors::UNIT_Z);
        _animVars.set("leftFootPoleVector", _prevLeftFootPoleVector);
    } else {
        _animVars.unset("leftFootPosition");
        _animVars.unset("leftFootRotation");
        _animVars.set("leftFootType", (int)IKTarget::Type::RotationAndPosition);
        _animVars.set("leftFootPoleVectorEnabled", false);
        _prevLeftFootPoleVectorValid = false;
    }

    if (rightFootEnabled) {
        _animVars.set("rightFootPosition", rightFootPose.trans());
        _animVars.set("rightFootRotation", rightFootPose.rot());
        _animVars.set("rightFootType", (int)IKTarget::Type::RotationAndPosition);

        int footJointIndex = _animSkeleton->nameToJointIndex("RightFoot");
        int kneeJointIndex = _animSkeleton->nameToJointIndex("RightLeg");
        int upLegJointIndex = _animSkeleton->nameToJointIndex("RightUpLeg");
        glm::vec3 poleVector = calculateKneePoleVector(footJointIndex, kneeJointIndex, upLegJointIndex, hipsIndex, rightFootPose);

        // smooth toward desired pole vector from previous pole vector...  to reduce jitter
        if (!_prevRightFootPoleVectorValid) {
            _prevRightFootPoleVectorValid = true;
            _prevRightFootPoleVector = poleVector;
        }
        glm::quat deltaRot = rotationBetween(_prevRightFootPoleVector, poleVector);
        glm::quat smoothDeltaRot = safeMix(deltaRot, Quaternions::IDENTITY, KNEE_POLE_VECTOR_BLEND_FACTOR);
        _prevRightFootPoleVector = smoothDeltaRot * _prevRightFootPoleVector;

        _animVars.set("rightFootPoleVectorEnabled", true);
        _animVars.set("rightFootPoleReferenceVector", Vectors::UNIT_Z);
        _animVars.set("rightFootPoleVector", _prevRightFootPoleVector);
    } else {
        _animVars.unset("rightFootPosition");
        _animVars.unset("rightFootRotation");
        _animVars.set("rightFootPoleVectorEnabled", false);
        _animVars.set("rightFootType", (int)IKTarget::Type::RotationAndPosition);
    }
}

void Rig::updateEyeJoint(int index, const glm::vec3& modelTranslation, const glm::quat& modelRotation, const glm::vec3& lookAtSpot, const glm::vec3& saccade) {

    // TODO: does not properly handle avatar scale.

    if (isIndexValid(index)) {
        const glm::mat4 rigToWorld = createMatFromQuatAndPos(modelRotation, modelTranslation);
        const glm::mat4 worldToRig = glm::inverse(rigToWorld);
        const glm::vec3 lookAtVector = glm::normalize(transformPoint(worldToRig, lookAtSpot) - _internalPoseSet._absolutePoses[index].trans());

        int headIndex = indexOfJoint("Head");
        glm::quat headQuat;
        if (headIndex >= 0) {
            headQuat = _internalPoseSet._absolutePoses[headIndex].rot();
        }

        glm::vec3 headUp = headQuat * Vectors::UNIT_Y;
        glm::vec3 z, y, zCrossY;
        generateBasisVectors(lookAtVector, headUp, z, y, zCrossY);
        glm::mat3 m(-zCrossY, y, z);
        glm::quat desiredQuat = glm::normalize(glm::quat_cast(m));

        glm::quat deltaQuat = desiredQuat * glm::inverse(headQuat);

        // limit swing rotation of the deltaQuat by a 30 degree cone.
        // TODO: use swing twist decomposition constraint instead, for off axis rotation clamping.
        const float MAX_ANGLE = 30.0f * RADIANS_PER_DEGREE;
        if (fabsf(glm::angle(deltaQuat)) > MAX_ANGLE) {
            deltaQuat = glm::angleAxis(glm::clamp(glm::angle(deltaQuat), -MAX_ANGLE, MAX_ANGLE), glm::axis(deltaQuat));
        }

        // directly set absolutePose rotation
        _internalPoseSet._absolutePoses[index].rot() = deltaQuat * headQuat;
    }
}

static glm::quat quatLerp(const glm::quat& q1, const glm::quat& q2, float alpha) {
    float dot = glm::dot(q1, q2);
    glm::quat temp;
    if (dot < 0.0f) {
        temp = -q2;
    } else {
        temp = q2;
    }
    return glm::normalize(glm::lerp(q1, temp, alpha));
}

glm::vec3 Rig::calculateElbowPoleVector(int handIndex, int elbowIndex, int armIndex, int hipsIndex, bool isLeft) const {
    AnimPose hipsPose = _externalPoseSet._absolutePoses[hipsIndex];
    AnimPose handPose = _externalPoseSet._absolutePoses[handIndex];
    AnimPose elbowPose = _externalPoseSet._absolutePoses[elbowIndex];
    AnimPose armPose = _externalPoseSet._absolutePoses[armIndex];

    // ray from hand to arm.
    glm::vec3 d = glm::normalize(handPose.trans() - armPose.trans());

    float sign = isLeft ? 1.0f : -1.0f;

    // form a plane normal to the hips x-axis.
    glm::vec3 n = hipsPose.rot() * Vectors::UNIT_X;
    glm::vec3 y = hipsPose.rot() * Vectors::UNIT_Y;

    // project d onto this plane
    glm::vec3 dProj = d - glm::dot(d, n) * n;

    // give dProj a bit of offset away from the body.
    float avatarScale = extractUniformScale(_modelOffset);
    const float LATERAL_OFFSET = 1.0f * avatarScale;
    const float VERTICAL_OFFSET = -0.333f * avatarScale;
    glm::vec3 dProjWithOffset = dProj + sign * LATERAL_OFFSET * n + y * VERTICAL_OFFSET;

    // rotate dProj by 90 degrees to get the poleVector.
    glm::vec3 poleVector = glm::angleAxis(PI / 2.0f, n) * dProjWithOffset;

    // blend the wrist oreintation into the pole vector to reduce the painfully bent wrist problem.
    glm::quat elbowToHandDelta = handPose.rot() * glm::inverse(elbowPose.rot());
    const float WRIST_POLE_ADJUST_FACTOR = 0.5f;
    glm::quat poleAdjust = quatLerp(Quaternions::IDENTITY, elbowToHandDelta, WRIST_POLE_ADJUST_FACTOR);

    return glm::normalize(poleAdjust * poleVector);
}

glm::vec3 Rig::calculateKneePoleVector(int footJointIndex, int kneeIndex, int upLegIndex, int hipsIndex, const AnimPose& targetFootPose) const {

    AnimPose hipsPose = _externalPoseSet._absolutePoses[hipsIndex];
    AnimPose footPose = targetFootPose;
    AnimPose kneePose = _externalPoseSet._absolutePoses[kneeIndex];
    AnimPose upLegPose = _externalPoseSet._absolutePoses[upLegIndex];

    // ray from foot to upLeg
    glm::vec3 d = glm::normalize(footPose.trans() - upLegPose.trans());

    // form a plane normal to the hips x-axis
    glm::vec3 n = hipsPose.rot() * Vectors::UNIT_X;

    // project d onto this plane
    glm::vec3 dProj = d - glm::dot(d, n) * n;

    // rotate dProj by 90 degrees to get the poleVector.
    glm::vec3 poleVector = glm::angleAxis(-PI / 2.0f, n) * dProj;

    // blend the foot oreintation into the pole vector
    glm::quat kneeToFootDelta = footPose.rot() * glm::inverse(kneePose.rot());
    const float WRIST_POLE_ADJUST_FACTOR = 0.5f;
    glm::quat poleAdjust = quatLerp(Quaternions::IDENTITY, kneeToFootDelta, WRIST_POLE_ADJUST_FACTOR);

    return glm::normalize(poleAdjust * poleVector);
}

void Rig::updateFromControllerParameters(const ControllerParameters& params, float dt) {
    if (!_animSkeleton || !_animNode) {
        return;
    }

    _animVars.set("isTalking", params.isTalking);
    _animVars.set("notIsTalking", !params.isTalking);

    bool headEnabled = params.primaryControllerFlags[PrimaryControllerType_Head] & (uint8_t)ControllerFlags::Enabled;
    bool leftHandEnabled = params.primaryControllerFlags[PrimaryControllerType_LeftHand] & (uint8_t)ControllerFlags::Enabled;
    bool rightHandEnabled = params.primaryControllerFlags[PrimaryControllerType_RightHand] & (uint8_t)ControllerFlags::Enabled;
    bool hipsEnabled = params.primaryControllerFlags[PrimaryControllerType_Hips] & (uint8_t)ControllerFlags::Enabled;
    bool hipsEstimated = params.primaryControllerFlags[PrimaryControllerType_Hips] & (uint8_t)ControllerFlags::Estimated;
    bool leftFootEnabled = params.primaryControllerFlags[PrimaryControllerType_LeftFoot] & (uint8_t)ControllerFlags::Enabled;
    bool rightFootEnabled = params.primaryControllerFlags[PrimaryControllerType_RightFoot] & (uint8_t)ControllerFlags::Enabled;
    bool spine2Enabled = params.primaryControllerFlags[PrimaryControllerType_Spine2] & (uint8_t)ControllerFlags::Enabled;
    bool leftArmEnabled = params.secondaryControllerFlags[SecondaryControllerType_LeftArm] & (uint8_t)ControllerFlags::Enabled;
    bool rightArmEnabled = params.secondaryControllerFlags[SecondaryControllerType_RightArm] & (uint8_t)ControllerFlags::Enabled;

    updateHead(headEnabled, hipsEnabled, params.primaryControllerPoses[PrimaryControllerType_Head]);

    updateHands(leftHandEnabled, rightHandEnabled, hipsEnabled, hipsEstimated, leftArmEnabled, rightArmEnabled, dt,
                params.primaryControllerPoses[PrimaryControllerType_LeftHand], params.primaryControllerPoses[PrimaryControllerType_RightHand],
                params.hipsShapeInfo, params.spineShapeInfo, params.spine1ShapeInfo, params.spine2ShapeInfo);

    updateFeet(leftFootEnabled, rightFootEnabled,
               params.primaryControllerPoses[PrimaryControllerType_LeftFoot], params.primaryControllerPoses[PrimaryControllerType_RightFoot]);

    // if the hips or the feet are being controlled.
    if (hipsEnabled || rightFootEnabled || leftFootEnabled) {
        // for more predictable IK solve from the center of the joint limits, not from the underpose
        _animVars.set("solutionSource", (int)AnimInverseKinematics::SolutionSource::RelaxToLimitCenterPoses);

        // replace the feet animation with the default pose, this is to prevent unexpected toe wiggling.
        _animVars.set("defaultPoseOverlayAlpha", 1.0f);
        _animVars.set("defaultPoseOverlayBoneSet", (int)AnimOverlay::BothFeetBoneSet);
    } else {
        // augment the IK with the underPose.
        _animVars.set("solutionSource", (int)AnimInverseKinematics::SolutionSource::RelaxToUnderPoses);

        // feet should follow source animation
        _animVars.unset("defaultPoseOverlayAlpha");
        _animVars.unset("defaultPoseOverlayBoneSet");
    }

    if (hipsEnabled) {
        _animVars.set("hipsType", (int)IKTarget::Type::RotationAndPosition);
        _animVars.set("hipsPosition", params.primaryControllerPoses[PrimaryControllerType_Hips].trans());
        _animVars.set("hipsRotation", params.primaryControllerPoses[PrimaryControllerType_Hips].rot());
    } else {
        _animVars.set("hipsType", (int)IKTarget::Type::Unknown);
    }

    if (hipsEnabled && spine2Enabled) {
        _animVars.set("spine2Type", (int)IKTarget::Type::Spline);
        _animVars.set("spine2Position", params.primaryControllerPoses[PrimaryControllerType_Spine2].trans());
        _animVars.set("spine2Rotation", params.primaryControllerPoses[PrimaryControllerType_Spine2].rot());
    } else {
        _animVars.set("spine2Type", (int)IKTarget::Type::Unknown);
    }

    // set secondary targets
    static const std::vector<QString> secondaryControllerJointNames = {
        "LeftShoulder",
        "RightShoulder",
        "LeftArm",
        "RightArm",
        "LeftForeArm",
        "RightForeArm",
        "LeftUpLeg",
        "RightUpLeg",
        "LeftLeg",
        "RightLeg",
        "LeftToeBase",
        "RightToeBase"
    };

    std::shared_ptr<AnimInverseKinematics> ikNode = getAnimInverseKinematicsNode();
    for (int i = 0; i < (int)NumSecondaryControllerTypes; i++) {
        int index = indexOfJoint(secondaryControllerJointNames[i]);
        if (index >= 0) {
            if (params.secondaryControllerFlags[i] & (uint8_t)ControllerFlags::Enabled) {
                ikNode->setSecondaryTargetInRigFrame(index, params.secondaryControllerPoses[i]);
            } else {
                ikNode->clearSecondaryTarget(index);
            }
        }
    }
}

void Rig::initAnimGraph(const QUrl& url) {
    if (_animGraphURL != url || (!_animNode && !_animLoading)) {
        _animGraphURL = url;

        _animNode.reset();

        // load the anim graph
        _animLoader.reset(new AnimNodeLoader(url));
        _animLoading = true;
        std::weak_ptr<AnimSkeleton> weakSkeletonPtr = _animSkeleton;
        connect(_animLoader.get(), &AnimNodeLoader::success, [this, weakSkeletonPtr](AnimNode::Pointer nodeIn) {
            _animNode = nodeIn;

            // abort load if the previous skeleton was deleted.
            auto sharedSkeletonPtr = weakSkeletonPtr.lock();
            if (!sharedSkeletonPtr) {
                return;
            }

            _animNode->setSkeleton(sharedSkeletonPtr);

            if (_userAnimState.clipNodeEnum != UserAnimState::None) {
                // restore the user animation we had before reset.
                UserAnimState origState = _userAnimState;
                _userAnimState = { UserAnimState::None, "", 30.0f, false, 0.0f, 0.0f };
                overrideAnimation(origState.url, origState.fps, origState.loop, origState.firstFrame, origState.lastFrame);
            }

            // restore the role animations we had before reset.
            for (auto& roleAnimState : _roleAnimStates) {
                auto roleState = roleAnimState.second;
                overrideRoleAnimation(roleState.role, roleState.url, roleState.fps, roleState.loop, roleState.firstFrame, roleState.lastFrame);
            }
            _animLoading = false;

            emit onLoadComplete();
        });
        connect(_animLoader.get(), &AnimNodeLoader::error, [url](int error, QString str) {
            qCCritical(animation) << "Error loading" << url.toDisplayString() << "code = " << error << "str =" << str;
        });
    }
}

bool Rig::getModelRegistrationPoint(glm::vec3& modelRegistrationPointOut) const {
    if (_animSkeleton && _rootJointIndex >= 0) {
        modelRegistrationPointOut = _geometryOffset * -_animSkeleton->getAbsoluteDefaultPose(_rootJointIndex).trans();
        return true;
    } else {
        return false;
    }
}

void Rig::applyOverridePoses() {
    DETAILED_PERFORMANCE_TIMER("override");
    if (_numOverrides == 0 || !_animSkeleton) {
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
    DETAILED_PERFORMANCE_TIMER("buildAbsolute");
    if (!_animSkeleton) {
        return;
    }

    ASSERT(_animSkeleton->getNumJoints() == (int)relativePoses.size());

    absolutePosesOut.resize(relativePoses.size());
    AnimPose geometryToRigTransform(_geometryToRigTransform);
    for (int i = 0; i < (int)relativePoses.size(); i++) {
        int parentIndex = _animSkeleton->getParentIndex(i);
        if (parentIndex == -1) {
            // transform all root absolute poses into rig space
            absolutePosesOut[i] = geometryToRigTransform * relativePoses[i];
        } else {
            absolutePosesOut[i] = absolutePosesOut[parentIndex] * relativePoses[i];
        }
    }
}

glm::mat4 Rig::getJointTransform(int jointIndex) const {
    static const glm::mat4 IDENTITY;
    if (isIndexValid(jointIndex)) {
        return _internalPoseSet._absolutePoses[jointIndex];
    } else {
        return IDENTITY;
    }
}

AnimPose Rig::getJointPose(int jointIndex) const {
    if (isIndexValid(jointIndex)) {
        return _internalPoseSet._absolutePoses[jointIndex];
    } else {
        return AnimPose::identity;
    }
}

void Rig::copyJointsIntoJointData(QVector<JointData>& jointDataVec) const {

    const AnimPose geometryToRigPose(_geometryToRigTransform);

    jointDataVec.resize((int)getJointStateCount());
    for (auto i = 0; i < jointDataVec.size(); i++) {
        JointData& data = jointDataVec[i];
        if (isIndexValid(i)) {
            // rotations are in absolute rig frame.
            glm::quat defaultAbsRot = geometryToRigPose.rot() * _animSkeleton->getAbsoluteDefaultPose(i).rot();
            data.rotation = _internalPoseSet._absolutePoses[i].rot();
            data.rotationIsDefaultPose = isEqual(data.rotation, defaultAbsRot);

            // translations are in relative frame but scaled so that they are in meters,
            // instead of geometry units.
            glm::vec3 defaultRelTrans = _geometryOffset.scale() * _animSkeleton->getRelativeDefaultPose(i).trans();
            data.translation = _geometryOffset.scale() * _internalPoseSet._relativePoses[i].trans();
            data.translationIsDefaultPose = isEqual(data.translation, defaultRelTrans);
        } else {
            data.translationIsDefaultPose = true;
            data.rotationIsDefaultPose = true;
        }
    }
}

void Rig::copyJointsFromJointData(const QVector<JointData>& jointDataVec) {
    DETAILED_PROFILE_RANGE(simulation_animation_detail, "copyJoints");
    DETAILED_PERFORMANCE_TIMER("copyJoints");

    if (!_animSkeleton) {
        return;
    }
    int numJoints = jointDataVec.size();
    const AnimPoseVec& absoluteDefaultPoses = _animSkeleton->getAbsoluteDefaultPoses();
    if (numJoints != (int)absoluteDefaultPoses.size()) {
        // jointData is incompatible
        return;
    }

    // make a vector of rotations in absolute-geometry-frame
    std::vector<glm::quat> rotations;
    rotations.reserve(numJoints);
    const glm::quat rigToGeometryRot(glmExtractRotation(_rigToGeometryTransform));
    for (int i = 0; i < numJoints; i++) {
        const JointData& data = jointDataVec.at(i);
        if (data.rotationIsDefaultPose) {
            rotations.push_back(absoluteDefaultPoses[i].rot());
        } else {
            // JointData rotations are in absolute rig-frame so we rotate them to absolute geometry-frame
            rotations.push_back(rigToGeometryRot * data.rotation);
        }
    }

    // convert rotations from absolute to parent relative.
    _animSkeleton->convertAbsoluteRotationsToRelative(rotations);

    // store new relative poses
    if (numJoints != (int)_internalPoseSet._relativePoses.size()) {
        _internalPoseSet._relativePoses = _animSkeleton->getRelativeDefaultPoses();
    }
    const AnimPoseVec& relativeDefaultPoses = _animSkeleton->getRelativeDefaultPoses();
    for (int i = 0; i < numJoints; i++) {
        const JointData& data = jointDataVec.at(i);
        _internalPoseSet._relativePoses[i].scale() = Vectors::ONE;
        _internalPoseSet._relativePoses[i].rot() = rotations[i];
        if (data.translationIsDefaultPose) {
            _internalPoseSet._relativePoses[i].trans() = relativeDefaultPoses[i].trans();
        } else {
            // JointData translations are in scaled relative-frame so we scale back to regular relative-frame
            _internalPoseSet._relativePoses[i].trans() = _invGeometryOffset.scale() * data.translation;
        }
    }
}

void Rig::computeExternalPoses(const glm::mat4& modelOffsetMat) {
    _modelOffset = AnimPose(modelOffsetMat);
    _geometryToRigTransform = _modelOffset * _geometryOffset;
    _rigToGeometryTransform = glm::inverse(_geometryToRigTransform);

    buildAbsoluteRigPoses(_internalPoseSet._relativePoses, _internalPoseSet._absolutePoses);
    QWriteLocker writeLock(&_externalPoseSetLock);
    _externalPoseSet = _internalPoseSet;
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

    glm::vec3 hipsPosition(0.0f);
    int hipsIndex = indexOfJoint("Hips");
    if (hipsIndex >= 0) {
        hipsPosition = transformPoint(_geometryToRigTransform, _animSkeleton->getAbsoluteDefaultPose(hipsIndex).trans());
    }

    // compute bounding box that encloses all points
    Extents totalExtents;
    totalExtents.reset();

    // HACK by convention our Avatars are always modeled such that y=0 is the ground plane.
    // add the zero point so that our avatars will always have bounding volumes that are flush with the ground
    // even if they do not have legs (default robot)
    totalExtents.addPoint(glm::vec3(0.0f));

    // To reduce the radius of the bounding capsule to be tight with the torso, we only consider joints
    // from the head to the hips when computing the rest of the bounding capsule.
    int index = indexOfJoint("Head");
    while (index != -1) {
        const FBXJointShapeInfo& shapeInfo = geometry.joints.at(index).shapeInfo;
        AnimPose pose = _animSkeleton->getAbsoluteDefaultPose(index);
        if (shapeInfo.points.size() > 0) {
            for (auto& point : shapeInfo.points) {
                totalExtents.addPoint((pose * point));
            }
        }
        index = _animSkeleton->getParentIndex(index);
    }

    // compute bounding shape parameters
    // NOTE: we assume that the longest side of totalExtents is the yAxis...
    glm::vec3 diagonal = (transformPoint(_geometryToRigTransform, totalExtents.maximum) -
                          transformPoint(_geometryToRigTransform, totalExtents.minimum));
    // ... and assume the radiusOut is half the RMS of the X and Z sides:
    radiusOut = 0.5f * sqrtf(0.5f * (diagonal.x * diagonal.x + diagonal.z * diagonal.z));
    heightOut = diagonal.y - 2.0f * radiusOut;

    glm::vec3 capsuleCenter = transformPoint(_geometryToRigTransform, (0.5f * (totalExtents.maximum + totalExtents.minimum)));
    localOffsetOut = capsuleCenter - hipsPosition;
}
