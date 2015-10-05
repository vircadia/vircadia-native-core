//
//  MyAvatar.cpp
//  interface/src/avatar
//
//  Created by Mark Peng on 8/16/13.
//  Copyright 2012 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <algorithm>
#include <vector>

#include <QBuffer>

#include <glm/gtx/norm.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <QtCore/QTimer>

#include <AccountManager.h>
#include <AddressManager.h>
#include <AnimationHandle.h>
#include <AudioClient.h>
#include <DependencyManager.h>
#include <display-plugins/DisplayPlugin.h>
#include <GeometryUtil.h>
#include <NodeList.h>
#include <udt/PacketHeaders.h>
#include <PathUtils.h>
#include <PerfStat.h>
#include <SharedUtil.h>
#include <TextRenderer3D.h>
#include <UserActivityLogger.h>
#include <AnimDebugDraw.h>

#include "devices/Faceshift.h"

#include "Application.h"
#include "AvatarManager.h"
#include "Environment.h"
#include "Menu.h"
#include "ModelReferential.h"
#include "MyAvatar.h"
#include "Physics.h"
#include "Recorder.h"
#include "Util.h"
#include "InterfaceLogging.h"
#include "DebugDraw.h"

using namespace std;

const glm::vec3 DEFAULT_UP_DIRECTION(0.0f, 1.0f, 0.0f);
const float YAW_SPEED = 150.0f;   // degrees/sec
const float PITCH_SPEED = 100.0f; // degrees/sec
const float DEFAULT_REAL_WORLD_FIELD_OF_VIEW_DEGREES = 30.0f;

const float MAX_WALKING_SPEED = 2.5f; // human walking speed
const float MAX_BOOST_SPEED = 0.5f * MAX_WALKING_SPEED; // keyboard motor gets additive boost below this speed
const float MIN_AVATAR_SPEED = 0.05f; // speed is set to zero below this

// TODO: normalize avatar speed for standard avatar size, then scale all motion logic
// to properly follow avatar size.
float MAX_AVATAR_SPEED = 300.0f;
float MAX_KEYBOARD_MOTOR_SPEED = MAX_AVATAR_SPEED;
float DEFAULT_KEYBOARD_MOTOR_TIMESCALE = 0.25f;
float MIN_SCRIPTED_MOTOR_TIMESCALE = 0.005f;
float DEFAULT_SCRIPTED_MOTOR_TIMESCALE = 1.0e6f;
const int SCRIPTED_MOTOR_CAMERA_FRAME = 0;
const int SCRIPTED_MOTOR_AVATAR_FRAME = 1;
const int SCRIPTED_MOTOR_WORLD_FRAME = 2;
const QString& DEFAULT_AVATAR_COLLISION_SOUND_URL = "https://hifi-public.s3.amazonaws.com/sounds/Collisions-otherorganic/Body_Hits_Impact.wav";

const float MyAvatar::ZOOM_MIN = 0.5f;
const float MyAvatar::ZOOM_MAX = 25.0f;
const float MyAvatar::ZOOM_DEFAULT = 1.5f;

MyAvatar::MyAvatar(RigPointer rig) :
    Avatar(rig),
    _gravity(0.0f, 0.0f, 0.0f),
    _wasPushing(false),
    _isPushing(false),
    _isBraking(false),
    _boomLength(ZOOM_DEFAULT),
    _trapDuration(0.0f),
    _thrust(0.0f),
    _keyboardMotorVelocity(0.0f),
    _keyboardMotorTimescale(DEFAULT_KEYBOARD_MOTOR_TIMESCALE),
    _scriptedMotorVelocity(0.0f),
    _scriptedMotorTimescale(DEFAULT_SCRIPTED_MOTOR_TIMESCALE),
    _scriptedMotorFrame(SCRIPTED_MOTOR_CAMERA_FRAME),
    _motionBehaviors(AVATAR_MOTION_DEFAULTS),
    _collisionSoundURL(""),
    _characterController(this),
    _lookAtTargetAvatar(),
    _shouldRender(true),
    _billboardValid(false),
    _eyeContactTarget(LEFT_EYE),
    _realWorldFieldOfView("realWorldFieldOfView",
                          DEFAULT_REAL_WORLD_FIELD_OF_VIEW_DEGREES),
    _hmdSensorMatrix(),
    _hmdSensorOrientation(),
    _hmdSensorPosition(),
    _bodySensorMatrix(),
    _sensorToWorldMatrix(),
    _goToPending(false),
    _goToPosition(),
    _goToOrientation(),
    _rig(rig),
    _prevShouldDrawHead(true),
    _audioListenerMode(FROM_HEAD)
{
    for (int i = 0; i < MAX_DRIVE_KEYS; i++) {
        _driveKeys[i] = 0.0f;
    }

    // connect to AddressManager signal for location jumps
    connect(DependencyManager::get<AddressManager>().data(), &AddressManager::locationChangeRequired,
            this, &MyAvatar::goToLocation);
    _characterController.setEnabled(true);
}

MyAvatar::~MyAvatar() {
    _lookAtTargetAvatar.reset();
}

QByteArray MyAvatar::toByteArray(bool cullSmallChanges, bool sendAll) {
    CameraMode mode = Application::getInstance()->getCamera()->getMode();
    if (mode == CAMERA_MODE_THIRD_PERSON || mode == CAMERA_MODE_INDEPENDENT) {
        // fake the avatar position that is sent up to the AvatarMixer
        glm::vec3 oldPosition = _position;
        _position = getSkeletonPosition();
        QByteArray array = AvatarData::toByteArray(cullSmallChanges, sendAll);
        // copy the correct position back
        _position = oldPosition;
        return array;
    }
    return AvatarData::toByteArray(cullSmallChanges, sendAll);
}

void MyAvatar::reset() {
    _skeletonModel.reset();
    getHead()->reset();

    _targetVelocity = glm::vec3(0.0f);
    setThrust(glm::vec3(0.0f));
    //  Reset the pitch and roll components of the avatar's orientation, preserve yaw direction
    glm::vec3 eulers = safeEulerAngles(getOrientation());
    eulers.x = 0.0f;
    eulers.z = 0.0f;
    setOrientation(glm::quat(eulers));

    // This should be simpler when we have only graph animations always on.
    bool isRig = _rig->getEnableRig();
    // seting rig animation to true, below, will clear the graph animation menu item, so grab it now.
    bool isGraph = _rig->getEnableAnimGraph() || Menu::getInstance()->isOptionChecked(MenuOption::EnableAnimGraph);
    qApp->setRawAvatarUpdateThreading(false);
    _rig->disableHands = true;
    setEnableRigAnimations(true);
    _skeletonModel.simulate(0.1f);  // non-zero
    setEnableRigAnimations(false);
    _skeletonModel.simulate(0.1f);
    if (isRig) {
        setEnableRigAnimations(true);
        Menu::getInstance()->setIsOptionChecked(MenuOption::EnableRigAnimations, true);
    } else if (isGraph) {
        setEnableAnimGraph(true);
        Menu::getInstance()->setIsOptionChecked(MenuOption::EnableAnimGraph, true);
    }
    _rig->disableHands = false;
    qApp->setRawAvatarUpdateThreading();
}

void MyAvatar::update(float deltaTime) {

    if (_goToPending) {
        setPosition(_goToPosition);
        setOrientation(_goToOrientation);
        _goToPending = false;
    }

    if (_referential) {
        _referential->update();
    }

    Head* head = getHead();
    head->relaxLean(deltaTime);
    updateFromTrackers(deltaTime);

    //  Get audio loudness data from audio input device
    auto audio = DependencyManager::get<AudioClient>();
    head->setAudioLoudness(audio->getLastInputLoudness());
    head->setAudioAverageLoudness(audio->getAudioAverageInputLoudness());

    simulate(deltaTime);
}

void MyAvatar::simulate(float deltaTime) {
    PerformanceTimer perfTimer("simulate");

    // Play back recording
    if (_player && _player->isPlaying()) {
        _player->play();
    }

    if (_scale != _targetScale) {
        float scale = (1.0f - SMOOTHING_RATIO) * _scale + SMOOTHING_RATIO * _targetScale;
        setScale(scale);
    }

    {
        PerformanceTimer perfTimer("transform");
        updateOrientation(deltaTime);
        updatePosition(deltaTime);
    }

    {
        PerformanceTimer perfTimer("hand");
        // update avatar skeleton and simulate hand and head
        getHand()->simulate(deltaTime, true);
    }

    {
        PerformanceTimer perfTimer("skeleton");
        _skeletonModel.simulate(deltaTime);
    }

    if (!_skeletonModel.hasSkeleton()) {
        // All the simulation that can be done has been done
        return;
    }

    {
        PerformanceTimer perfTimer("attachments");
        simulateAttachments(deltaTime);
    }

    {
        PerformanceTimer perfTimer("joints");
        // copy out the skeleton joints from the model
        _jointData.resize(_rig->getJointStateCount());

        for (int i = 0; i < _jointData.size(); i++) {
            JointData& data = _jointData[i];
            data.rotationSet |= _rig->getJointStateRotation(i, data.rotation);
            data.translationSet |= _rig->getJointStateTranslation(i, data.translation);
        }
    }

    {
        PerformanceTimer perfTimer("head");
        Head* head = getHead();
        glm::vec3 headPosition;
        if (!_skeletonModel.getHeadPosition(headPosition)) {
            headPosition = _position;
        }
        head->setPosition(headPosition);
        head->setScale(_scale);
        head->simulate(deltaTime, true);
    }

    // Record avatars movements.
    if (_recorder && _recorder->isRecording()) {
        _recorder->record();
    }

    // consider updating our billboard
    maybeUpdateBillboard();
}

glm::mat4 MyAvatar::getSensorToWorldMatrix() const {
    return _sensorToWorldMatrix;
}

// returns true if pos is OUTSIDE of the vertical capsule
// where the middle cylinder length is defined by capsuleLen and the radius by capsuleRad.
static bool capsuleCheck(const glm::vec3& pos, float capsuleLen, float capsuleRad) {
    const float halfCapsuleLen = capsuleLen / 2.0f;
    if (fabs(pos.y) <= halfCapsuleLen) {
        // cylinder check for middle capsule
        glm::vec2 horizPos(pos.x, pos.z);
        return glm::length(horizPos) > capsuleRad;
    } else if (pos.y > halfCapsuleLen) {
        glm::vec3 center(0.0f, halfCapsuleLen, 0.0f);
        return glm::length(center - pos) > capsuleRad;
    } else if (pos.y < halfCapsuleLen) {
        glm::vec3 center(0.0f, -halfCapsuleLen, 0.0f);
        return glm::length(center - pos) > capsuleRad;
    } else {
        return false;
    }
}

// Pass a recent sample of the HMD to the avatar.
// This can also update the avatar's position to follow the HMD
// as it moves through the world.
void MyAvatar::updateFromHMDSensorMatrix(const glm::mat4& hmdSensorMatrix) {

    auto now = usecTimestampNow();
    auto deltaUsecs = now - _lastUpdateFromHMDTime;
    _lastUpdateFromHMDTime = now;
    double actualDeltaTime = (double)deltaUsecs / (double)USECS_PER_SECOND;
    const float BIGGEST_DELTA_TIME_SECS = 0.25f;
    float deltaTime = glm::clamp((float)actualDeltaTime, 0.0f, BIGGEST_DELTA_TIME_SECS);

    // update the sensorMatrices based on the new hmd pose
    _hmdSensorMatrix = hmdSensorMatrix;
    _hmdSensorPosition = extractTranslation(hmdSensorMatrix);
    _hmdSensorOrientation = glm::quat_cast(hmdSensorMatrix);

    const float STRAIGHTING_LEAN_DURATION = 0.5f;  // seconds

    // define a vertical capsule
    const float STRAIGHTING_LEAN_CAPSULE_RADIUS = 0.2f;  // meters
    const float STRAIGHTING_LEAN_CAPSULE_LENGTH = 0.05f;  // length of the cylinder part of the capsule in meters.

    auto newBodySensorMatrix = deriveBodyFromHMDSensor();
    glm::vec3 diff = extractTranslation(newBodySensorMatrix) - extractTranslation(_bodySensorMatrix);
    if (!_straightingLean && capsuleCheck(diff, STRAIGHTING_LEAN_CAPSULE_LENGTH, STRAIGHTING_LEAN_CAPSULE_RADIUS)) {

        // begin homing toward derived body position.
        _straightingLean = true;
        _straightingLeanAlpha = 0.0f;

    } else if (_straightingLean) {

        auto newBodySensorMatrix = deriveBodyFromHMDSensor();
        auto worldBodyMatrix = _sensorToWorldMatrix * newBodySensorMatrix;
        glm::vec3 worldBodyPos = extractTranslation(worldBodyMatrix);
        glm::quat worldBodyRot = glm::normalize(glm::quat_cast(worldBodyMatrix));

        _straightingLeanAlpha += (1.0f / STRAIGHTING_LEAN_DURATION) * deltaTime;

        if (_straightingLeanAlpha >= 1.0f) {
            _straightingLean = false;
            nextAttitude(worldBodyPos, worldBodyRot);
            _bodySensorMatrix = newBodySensorMatrix;
        } else {
            // interp position toward the desired pos
            glm::vec3 pos = lerp(getPosition(), worldBodyPos, _straightingLeanAlpha);
            glm::quat rot = glm::normalize(safeMix(getOrientation(), worldBodyRot, _straightingLeanAlpha));
            nextAttitude(pos, rot);

            // interp sensor matrix toward desired
            glm::vec3 nextBodyPos = extractTranslation(newBodySensorMatrix);
            glm::quat nextBodyRot = glm::normalize(glm::quat_cast(newBodySensorMatrix));
            glm::vec3 prevBodyPos = extractTranslation(_bodySensorMatrix);
            glm::quat prevBodyRot = glm::normalize(glm::quat_cast(_bodySensorMatrix));
            pos = lerp(prevBodyPos, nextBodyPos, _straightingLeanAlpha);
            rot = glm::normalize(safeMix(prevBodyRot, nextBodyRot, _straightingLeanAlpha));
            _bodySensorMatrix = createMatFromQuatAndPos(rot, pos);
        }
    }
}
// 
// best called at end of main loop, just before rendering.
// update sensor to world matrix from current body position and hmd sensor.
// This is so the correct camera can be used for rendering.
void MyAvatar::updateSensorToWorldMatrix() {
    // update the sensor mat so that the body position will end up in the desired
    // position when driven from the head.
    glm::mat4 desiredMat = createMatFromQuatAndPos(getOrientation(), getPosition());
    _sensorToWorldMatrix = desiredMat * glm::inverse(_bodySensorMatrix);
}

//  Update avatar head rotation with sensor data
void MyAvatar::updateFromTrackers(float deltaTime) {
    glm::vec3 estimatedPosition, estimatedRotation;

    bool inHmd = qApp->getAvatarUpdater()->isHMDMode();

    if (isPlaying() && inHmd) {
        return;
    }

    FaceTracker* tracker = Application::getInstance()->getActiveFaceTracker();
    bool inFacetracker = tracker && !tracker->isMuted();

    if (inHmd) {
        estimatedPosition = extractTranslation(getHMDSensorMatrix());
        estimatedPosition.x *= -1.0f;
        _trackedHeadPosition = estimatedPosition;

        const float OCULUS_LEAN_SCALE = 0.05f;
        estimatedPosition /= OCULUS_LEAN_SCALE;
    } else if (inFacetracker) {
        estimatedPosition = tracker->getHeadTranslation();
        _trackedHeadPosition = estimatedPosition;
        estimatedRotation = glm::degrees(safeEulerAngles(tracker->getHeadRotation()));
        if (Application::getInstance()->getCamera()->getMode() == CAMERA_MODE_MIRROR) {
            // Invert yaw and roll when in mirror mode
            // NOTE: this is kinda a hack, it's the same hack we use to make the head tilt. But it's not really a mirror
            // it just makes you feel like you're looking in a mirror because the body movements of the avatar appear to
            // match your body movements.
            YAW(estimatedRotation) *= -1.0f;
            ROLL(estimatedRotation) *= -1.0f;
        }
    }

    //  Rotate the body if the head is turned beyond the screen
    if (Menu::getInstance()->isOptionChecked(MenuOption::TurnWithHead)) {
        const float TRACKER_YAW_TURN_SENSITIVITY = 0.5f;
        const float TRACKER_MIN_YAW_TURN = 15.0f;
        const float TRACKER_MAX_YAW_TURN = 50.0f;
        if ( (fabs(estimatedRotation.y) > TRACKER_MIN_YAW_TURN) &&
             (fabs(estimatedRotation.y) < TRACKER_MAX_YAW_TURN) ) {
            if (estimatedRotation.y > 0.0f) {
                _bodyYawDelta += (estimatedRotation.y - TRACKER_MIN_YAW_TURN) * TRACKER_YAW_TURN_SENSITIVITY;
            } else {
                _bodyYawDelta += (estimatedRotation.y + TRACKER_MIN_YAW_TURN) * TRACKER_YAW_TURN_SENSITIVITY;
            }
        }
    }

    // Set the rotation of the avatar's head (as seen by others, not affecting view frustum)
    // to be scaled such that when the user's physical head is pointing at edge of screen, the
    // avatar head is at the edge of the in-world view frustum.  So while a real person may move
    // their head only 30 degrees or so, this may correspond to a 90 degree field of view.
    // Note that roll is magnified by a constant because it is not related to field of view.


    Head* head = getHead();
    if (inHmd || isPlaying()) {
        head->setDeltaPitch(estimatedRotation.x);
        head->setDeltaYaw(estimatedRotation.y);
        head->setDeltaRoll(estimatedRotation.z);
    } else {
        float magnifyFieldOfView = qApp->getFieldOfView() /
                                   _realWorldFieldOfView.get();
        head->setDeltaPitch(estimatedRotation.x * magnifyFieldOfView);
        head->setDeltaYaw(estimatedRotation.y * magnifyFieldOfView);
        head->setDeltaRoll(estimatedRotation.z);
    }

    //  Update torso lean distance based on accelerometer data
    const float TORSO_LENGTH = 0.5f;
    glm::vec3 relativePosition = estimatedPosition - glm::vec3(0.0f, -TORSO_LENGTH, 0.0f);
    const float MAX_LEAN = 45.0f;

    // Invert left/right lean when in mirror mode
    // NOTE: this is kinda a hack, it's the same hack we use to make the head tilt. But it's not really a mirror
    // it just makes you feel like you're looking in a mirror because the body movements of the avatar appear to
    // match your body movements.
    if ((inHmd || inFacetracker) && Application::getInstance()->getCamera()->getMode() == CAMERA_MODE_MIRROR) {
        relativePosition.x = -relativePosition.x;
    }

    head->setLeanSideways(glm::clamp(glm::degrees(atanf(relativePosition.x * _leanScale / TORSO_LENGTH)),
                                     -MAX_LEAN, MAX_LEAN));
    head->setLeanForward(glm::clamp(glm::degrees(atanf(relativePosition.z * _leanScale / TORSO_LENGTH)),
                                    -MAX_LEAN, MAX_LEAN));
}


// virtual
void MyAvatar::render(RenderArgs* renderArgs, const glm::vec3& cameraPosition) {
    // don't render if we've been asked to disable local rendering
    if (!_shouldRender) {
        return; // exit early
    }

    Avatar::render(renderArgs, cameraPosition);

    // don't display IK constraints in shadow mode
    if (Menu::getInstance()->isOptionChecked(MenuOption::ShowIKConstraints) &&
        renderArgs && renderArgs->_batch) {
        _skeletonModel.renderIKConstraints(*renderArgs->_batch);
    }
}

const glm::vec3 HAND_TO_PALM_OFFSET(0.0f, 0.12f, 0.08f);

glm::vec3 MyAvatar::getLeftPalmPosition() {
    glm::vec3 leftHandPosition;
    getSkeletonModel().getLeftHandPosition(leftHandPosition);
    glm::quat leftRotation;
    getSkeletonModel().getJointRotationInWorldFrame(getSkeletonModel().getLeftHandJointIndex(), leftRotation);
    leftHandPosition += HAND_TO_PALM_OFFSET * glm::inverse(leftRotation);
    return leftHandPosition;
}

glm::vec3 MyAvatar::getLeftPalmVelocity() {
    const PalmData* palm = getHand()->getPalm(LEFT_HAND_INDEX);
    if (palm != NULL) {
        return palm->getVelocity();
    }
    return glm::vec3(0.0f);
}

glm::vec3 MyAvatar::getLeftPalmAngularVelocity() {
    const PalmData* palm = getHand()->getPalm(LEFT_HAND_INDEX);
    if (palm != NULL) {
        return palm->getRawAngularVelocity();
    }
    return glm::vec3(0.0f);
}

glm::quat MyAvatar::getLeftPalmRotation() {
    glm::quat leftRotation;
    getSkeletonModel().getJointRotationInWorldFrame(getSkeletonModel().getLeftHandJointIndex(), leftRotation);
    return leftRotation;
}

glm::vec3 MyAvatar::getRightPalmPosition() {
    glm::vec3 rightHandPosition;
    getSkeletonModel().getRightHandPosition(rightHandPosition);
    glm::quat rightRotation;
    getSkeletonModel().getJointRotationInWorldFrame(getSkeletonModel().getRightHandJointIndex(), rightRotation);
    rightHandPosition += HAND_TO_PALM_OFFSET * glm::inverse(rightRotation);
    return rightHandPosition;
}

glm::vec3 MyAvatar::getRightPalmVelocity() {
    const PalmData* palm = getHand()->getPalm(RIGHT_HAND_INDEX);
    if (palm != NULL) {
        return palm->getVelocity();
    }
    return glm::vec3(0.0f);
}

glm::vec3 MyAvatar::getRightPalmAngularVelocity() {
    const PalmData* palm = getHand()->getPalm(RIGHT_HAND_INDEX);
    if (palm != NULL) {
        return palm->getRawAngularVelocity();
    }
    return glm::vec3(0.0f);
}

glm::quat MyAvatar::getRightPalmRotation() {
    glm::quat rightRotation;
    getSkeletonModel().getJointRotationInWorldFrame(getSkeletonModel().getRightHandJointIndex(), rightRotation);
    return rightRotation;
}

void MyAvatar::clearReferential() {
    changeReferential(NULL);
}

bool MyAvatar::setModelReferential(const QUuid& id) {
    EntityTreePointer tree = Application::getInstance()->getEntities()->getTree();
    changeReferential(new ModelReferential(id, tree, this));
    if (_referential->isValid()) {
        return true;
    } else {
        changeReferential(NULL);
        return false;
    }
}

bool MyAvatar::setJointReferential(const QUuid& id, int jointIndex) {
    EntityTreePointer tree = Application::getInstance()->getEntities()->getTree();
    changeReferential(new JointReferential(jointIndex, id, tree, this));
    if (!_referential->isValid()) {
        return true;
    } else {
        changeReferential(NULL);
        return false;
    }
}

bool MyAvatar::isRecording() {
    if (!_recorder) {
        return false;
    }
    if (QThread::currentThread() != thread()) {
        bool result;
        QMetaObject::invokeMethod(this, "isRecording", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(bool, result));
        return result;
    }
    return _recorder && _recorder->isRecording();
}

qint64 MyAvatar::recorderElapsed() {
    if (!_recorder) {
        return 0;
    }
    if (QThread::currentThread() != thread()) {
        qint64 result;
        QMetaObject::invokeMethod(this, "recorderElapsed", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(qint64, result));
        return result;
    }
    return _recorder->elapsed();
}

void MyAvatar::startRecording() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "startRecording", Qt::BlockingQueuedConnection);
        return;
    }
    if (!_recorder) {
        _recorder = QSharedPointer<Recorder>::create(this);
    }
    // connect to AudioClient's signal so we get input audio
    auto audioClient = DependencyManager::get<AudioClient>();
    connect(audioClient.data(), &AudioClient::inputReceived, _recorder.data(),
            &Recorder::recordAudio, Qt::BlockingQueuedConnection);

    _recorder->startRecording();
}

void MyAvatar::stopRecording() {
    if (!_recorder) {
        return;
    }
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "stopRecording", Qt::BlockingQueuedConnection);
        return;
    }
    if (_recorder) {
        // stop grabbing audio from the AudioClient
        auto audioClient = DependencyManager::get<AudioClient>();
        disconnect(audioClient.data(), 0, _recorder.data(), 0);

        _recorder->stopRecording();
    }
}

void MyAvatar::saveRecording(QString filename) {
    if (!_recorder) {
        qCDebug(interfaceapp) << "There is no recording to save";
        return;
    }
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "saveRecording", Qt::BlockingQueuedConnection,
                                  Q_ARG(QString, filename));
        return;
    }
    if (_recorder) {
        _recorder->saveToFile(filename);
    }
}

void MyAvatar::loadLastRecording() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "loadLastRecording", Qt::BlockingQueuedConnection);
        return;
    }
    if (!_recorder) {
        qCDebug(interfaceapp) << "There is no recording to load";
        return;
    }
    if (!_player) {
        _player = QSharedPointer<Player>::create(this);
    }

    _player->loadRecording(_recorder->getRecording());
}

void MyAvatar::startAnimation(const QString& url, float fps, float priority,
        bool loop, bool hold, float firstFrame, float lastFrame, const QStringList& maskedJoints) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "startAnimation", Q_ARG(const QString&, url), Q_ARG(float, fps),
            Q_ARG(float, priority), Q_ARG(bool, loop), Q_ARG(bool, hold), Q_ARG(float, firstFrame),
            Q_ARG(float, lastFrame), Q_ARG(const QStringList&, maskedJoints));
        return;
    }
    _rig->startAnimation(url, fps, priority, loop, hold, firstFrame, lastFrame, maskedJoints);
}

void MyAvatar::startAnimationByRole(const QString& role, const QString& url, float fps, float priority,
        bool loop, bool hold, float firstFrame, float lastFrame, const QStringList& maskedJoints) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "startAnimationByRole", Q_ARG(const QString&, role), Q_ARG(const QString&, url),
            Q_ARG(float, fps), Q_ARG(float, priority), Q_ARG(bool, loop), Q_ARG(bool, hold), Q_ARG(float, firstFrame),
            Q_ARG(float, lastFrame), Q_ARG(const QStringList&, maskedJoints));
        return;
    }
    _rig->startAnimationByRole(role, url, fps, priority, loop, hold, firstFrame, lastFrame, maskedJoints);
}

void MyAvatar::stopAnimationByRole(const QString& role) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "stopAnimationByRole", Q_ARG(const QString&, role));
        return;
    }
    _rig->stopAnimationByRole(role);
}

void MyAvatar::stopAnimation(const QString& url) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "stopAnimation", Q_ARG(const QString&, url));
        return;
    }
    _rig->stopAnimation(url);
}

AnimationDetails MyAvatar::getAnimationDetailsByRole(const QString& role) {
    AnimationDetails result;
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "getAnimationDetailsByRole", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(AnimationDetails, result),
            Q_ARG(const QString&, role));
        return result;
    }
    foreach (const AnimationHandlePointer& handle, _rig->getRunningAnimations()) {
        if (handle->getRole() == role) {
            result = handle->getAnimationDetails();
            break;
        }
    }
    return result;
}

AnimationDetails MyAvatar::getAnimationDetails(const QString& url) {
    AnimationDetails result;
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "getAnimationDetails", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(AnimationDetails, result),
            Q_ARG(const QString&, url));
        return result;
    }
    foreach (const AnimationHandlePointer& handle, _rig->getRunningAnimations()) {
        if (handle->getURL() == url) {
            result = handle->getAnimationDetails();
            break;
        }
    }
    return result;
}

void MyAvatar::saveData() {
    Settings settings;
    settings.beginGroup("Avatar");

    settings.setValue("headPitch", getHead()->getBasePitch());

    settings.setValue("pupilDilation", getHead()->getPupilDilation());

    settings.setValue("leanScale", _leanScale);
    settings.setValue("scale", _targetScale);

    settings.setValue("fullAvatarURL", _fullAvatarURLFromPreferences);
    settings.setValue("fullAvatarModelName", _fullAvatarModelName);
    settings.setValue("animGraphURL", _animGraphUrl);

    settings.beginWriteArray("attachmentData");
    for (int i = 0; i < _attachmentData.size(); i++) {
        settings.setArrayIndex(i);
        const AttachmentData& attachment = _attachmentData.at(i);
        settings.setValue("modelURL", attachment.modelURL);
        settings.setValue("jointName", attachment.jointName);
        settings.setValue("translation_x", attachment.translation.x);
        settings.setValue("translation_y", attachment.translation.y);
        settings.setValue("translation_z", attachment.translation.z);
        glm::vec3 eulers = safeEulerAngles(attachment.rotation);
        settings.setValue("rotation_x", eulers.x);
        settings.setValue("rotation_y", eulers.y);
        settings.setValue("rotation_z", eulers.z);
        settings.setValue("scale", attachment.scale);
    }
    settings.endArray();

    settings.beginWriteArray("animationHandles");
    auto animationHandles = _rig->getAnimationHandles();
    for (int i = 0; i < animationHandles.size(); i++) {
        settings.setArrayIndex(i);
        const AnimationHandlePointer& pointer = animationHandles.at(i);
        settings.setValue("role", pointer->getRole());
        settings.setValue("url", pointer->getURL());
        settings.setValue("fps", pointer->getFPS());
        settings.setValue("priority", pointer->getPriority());
        settings.setValue("loop", pointer->getLoop());
        settings.setValue("hold", pointer->getHold());
        settings.setValue("startAutomatically", pointer->getStartAutomatically());
        settings.setValue("firstFrame", pointer->getFirstFrame());
        settings.setValue("lastFrame", pointer->getLastFrame());
        settings.setValue("maskedJoints", pointer->getMaskedJoints());
    }
    settings.endArray();

    settings.setValue("displayName", _displayName);
    settings.setValue("collisionSoundURL", _collisionSoundURL);

    settings.endGroup();
}

float loadSetting(QSettings& settings, const char* name, float defaultValue) {
    float value = settings.value(name, defaultValue).toFloat();
    if (glm::isnan(value)) {
        value = defaultValue;
    }
    return value;
}

// Resource loading is not yet thread safe. If an animation is not loaded when requested by other than tha main thread,
// we block in AnimationHandle::setURL => AnimationCache::getAnimation.
// Meanwhile, the main thread will also eventually lock as it tries to render us.
// If we demand the animation from the update thread while we're locked, we'll deadlock.
// Until we untangle this, code puts the updates back on the main thread temporarilly and starts all the loading.
void MyAvatar::safelyLoadAnimations() {
    _rig->addAnimationByRole("idle");
    _rig->addAnimationByRole("walk");
    _rig->addAnimationByRole("backup");
    _rig->addAnimationByRole("leftTurn");
    _rig->addAnimationByRole("rightTurn");
    _rig->addAnimationByRole("leftStrafe");
    _rig->addAnimationByRole("rightStrafe");
}

void MyAvatar::setEnableRigAnimations(bool isEnabled) {
    if (_rig->getEnableRig() == isEnabled) {
        return;
    }
    if (isEnabled) {
        qApp->setRawAvatarUpdateThreading(false);
        setEnableAnimGraph(false);
        Menu::getInstance()->setIsOptionChecked(MenuOption::EnableAnimGraph, false);
        safelyLoadAnimations();
        qApp->setRawAvatarUpdateThreading();
        _rig->setEnableRig(true);
    } else {
        _rig->setEnableRig(false);
        _rig->deleteAnimations();
    }
}

void MyAvatar::setEnableAnimGraph(bool isEnabled) {
    if (_rig->getEnableAnimGraph() == isEnabled) {
        return;
    }
    if (isEnabled) {
        qApp->setRawAvatarUpdateThreading(false);
        setEnableRigAnimations(false);
        Menu::getInstance()->setIsOptionChecked(MenuOption::EnableRigAnimations, false);
        safelyLoadAnimations();
        if (_skeletonModel.readyToAddToScene()) {
            _rig->setEnableAnimGraph(true);
           initAnimGraph(); // must be enabled for the init to happen
            _rig->setEnableAnimGraph(false); // must be disable to safely reset threading
       }
        qApp->setRawAvatarUpdateThreading();
        _rig->setEnableAnimGraph(true);
    } else {
        _rig->setEnableAnimGraph(false);
        destroyAnimGraph();
    }
}

void MyAvatar::setEnableDebugDrawBindPose(bool isEnabled) {
    _enableDebugDrawBindPose = isEnabled;

    if (!isEnabled) {
        AnimDebugDraw::getInstance().removeSkeleton("myAvatar");
    }
}

void MyAvatar::setEnableDebugDrawAnimPose(bool isEnabled) {
    _enableDebugDrawAnimPose = isEnabled;

    if (!isEnabled) {
        AnimDebugDraw::getInstance().removePoses("myAvatar");
    }
}

void MyAvatar::setEnableMeshVisible(bool isEnabled) {
    render::ScenePointer scene = Application::getInstance()->getMain3DScene();
    _skeletonModel.setVisibleInScene(isEnabled, scene);
}

void MyAvatar::loadData() {
    Settings settings;
    settings.beginGroup("Avatar");

    getHead()->setBasePitch(loadSetting(settings, "headPitch", 0.0f));

    getHead()->setPupilDilation(loadSetting(settings, "pupilDilation", 0.0f));

    _leanScale = loadSetting(settings, "leanScale", 0.05f);
    _targetScale = loadSetting(settings, "scale", 1.0f);
    setScale(_scale);

    _animGraphUrl = settings.value("animGraphURL", "").toString();
    _fullAvatarURLFromPreferences = settings.value("fullAvatarURL", AvatarData::defaultFullAvatarModelUrl()).toUrl();
    _fullAvatarModelName = settings.value("fullAvatarModelName", DEFAULT_FULL_AVATAR_MODEL_NAME).toString();

    useFullAvatarURL(_fullAvatarURLFromPreferences, _fullAvatarModelName);

    QVector<AttachmentData> attachmentData;
    int attachmentCount = settings.beginReadArray("attachmentData");
    for (int i = 0; i < attachmentCount; i++) {
        settings.setArrayIndex(i);
        AttachmentData attachment;
        attachment.modelURL = settings.value("modelURL").toUrl();
        attachment.jointName = settings.value("jointName").toString();
        attachment.translation.x = loadSetting(settings, "translation_x", 0.0f);
        attachment.translation.y = loadSetting(settings, "translation_y", 0.0f);
        attachment.translation.z = loadSetting(settings, "translation_z", 0.0f);
        glm::vec3 eulers;
        eulers.x = loadSetting(settings, "rotation_x", 0.0f);
        eulers.y = loadSetting(settings, "rotation_y", 0.0f);
        eulers.z = loadSetting(settings, "rotation_z", 0.0f);
        attachment.rotation = glm::quat(eulers);
        attachment.scale = loadSetting(settings, "scale", 1.0f);
        attachmentData.append(attachment);
    }
    settings.endArray();
    setAttachmentData(attachmentData);

    int animationCount = settings.beginReadArray("animationHandles");
    _rig->deleteAnimations();
    for (int i = 0; i < animationCount; i++) {
        settings.setArrayIndex(i);
        _rig->addAnimationByRole(settings.value("role", "idle").toString(),
                                 settings.value("url").toString(),
                                 loadSetting(settings, "fps", 30.0f),
                                 loadSetting(settings, "priority", 1.0f),
                                 settings.value("loop", true).toBool(),
                                 settings.value("hold", false).toBool(),
                                 settings.value("firstFrame", 0.0f).toFloat(),
                                 settings.value("lastFrame", INT_MAX).toFloat(),
                                 settings.value("maskedJoints").toStringList(),
                                 settings.value("startAutomatically", true).toBool());
    }
    settings.endArray();

    setDisplayName(settings.value("displayName").toString());
    setCollisionSoundURL(settings.value("collisionSoundURL", DEFAULT_AVATAR_COLLISION_SOUND_URL).toString());

    settings.endGroup();
    _rig->setEnableRig(Menu::getInstance()->isOptionChecked(MenuOption::EnableRigAnimations));
    setEnableMeshVisible(Menu::getInstance()->isOptionChecked(MenuOption::MeshVisible));
    setEnableDebugDrawBindPose(Menu::getInstance()->isOptionChecked(MenuOption::AnimDebugDrawBindPose));
    setEnableDebugDrawAnimPose(Menu::getInstance()->isOptionChecked(MenuOption::AnimDebugDrawAnimPose));
}

void MyAvatar::saveAttachmentData(const AttachmentData& attachment) const {
    Settings settings;
    settings.beginGroup("savedAttachmentData");
    settings.beginGroup(_skeletonModel.getURL().toString());
    settings.beginGroup(attachment.modelURL.toString());
    settings.setValue("jointName", attachment.jointName);

    settings.beginGroup(attachment.jointName);
    settings.setValue("translation_x", attachment.translation.x);
    settings.setValue("translation_y", attachment.translation.y);
    settings.setValue("translation_z", attachment.translation.z);
    glm::vec3 eulers = safeEulerAngles(attachment.rotation);
    settings.setValue("rotation_x", eulers.x);
    settings.setValue("rotation_y", eulers.y);
    settings.setValue("rotation_z", eulers.z);
    settings.setValue("scale", attachment.scale);

    settings.endGroup();
    settings.endGroup();
    settings.endGroup();
    settings.endGroup();
}

AttachmentData MyAvatar::loadAttachmentData(const QUrl& modelURL, const QString& jointName) const {
    Settings settings;
    settings.beginGroup("savedAttachmentData");
    settings.beginGroup(_skeletonModel.getURL().toString());
    settings.beginGroup(modelURL.toString());

    AttachmentData attachment;
    attachment.modelURL = modelURL;
    if (jointName.isEmpty()) {
        attachment.jointName = settings.value("jointName").toString();
    } else {
        attachment.jointName = jointName;
    }
    settings.beginGroup(attachment.jointName);
    if (settings.contains("translation_x")) {
        attachment.translation.x = loadSetting(settings, "translation_x", 0.0f);
        attachment.translation.y = loadSetting(settings, "translation_y", 0.0f);
        attachment.translation.z = loadSetting(settings, "translation_z", 0.0f);
        glm::vec3 eulers;
        eulers.x = loadSetting(settings, "rotation_x", 0.0f);
        eulers.y = loadSetting(settings, "rotation_y", 0.0f);
        eulers.z = loadSetting(settings, "rotation_z", 0.0f);
        attachment.rotation = glm::quat(eulers);
        attachment.scale = loadSetting(settings, "scale", 1.0f);
    } else {
        attachment = AttachmentData();
    }

    settings.endGroup();
    settings.endGroup();
    settings.endGroup();
    settings.endGroup();

    return attachment;
}

int MyAvatar::parseDataFromBuffer(const QByteArray& buffer) {
    qCDebug(interfaceapp) << "Error: ignoring update packet for MyAvatar"
        << " packetLength = " << buffer.size();
    // this packet is just bad, so we pretend that we unpacked it ALL
    return buffer.size();
}

void MyAvatar::sendKillAvatar() {
    auto killPacket = NLPacket::create(PacketType::KillAvatar, 0);
    DependencyManager::get<NodeList>()->broadcastToNodes(std::move(killPacket), NodeSet() << NodeType::AvatarMixer);
}

void MyAvatar::updateLookAtTargetAvatar() {
    //
    //  Look at the avatar whose eyes are closest to the ray in direction of my avatar's head
    //  And set the correctedLookAt for all (nearby) avatars that are looking at me.
    _lookAtTargetAvatar.reset();
    _targetAvatarPosition = glm::vec3(0.0f);

    glm::vec3 lookForward = getHead()->getFinalOrientationInWorldFrame() * IDENTITY_FRONT;
    glm::vec3 cameraPosition = Application::getInstance()->getCamera()->getPosition();

    float smallestAngleTo = glm::radians(DEFAULT_FIELD_OF_VIEW_DEGREES) / 2.0f;
    const float KEEP_LOOKING_AT_CURRENT_ANGLE_FACTOR = 1.3f;
    const float GREATEST_LOOKING_AT_DISTANCE = 10.0f;

    foreach (const AvatarSharedPointer& avatarPointer, DependencyManager::get<AvatarManager>()->getAvatarHash()) {
        auto avatar = static_pointer_cast<Avatar>(avatarPointer);
        bool isCurrentTarget = avatar->getIsLookAtTarget();
        float distanceTo = glm::length(avatar->getHead()->getEyePosition() - cameraPosition);
        avatar->setIsLookAtTarget(false);
        if (!avatar->isMyAvatar() && avatar->isInitialized() && (distanceTo < GREATEST_LOOKING_AT_DISTANCE * getScale())) {
            float angleTo = glm::angle(lookForward, glm::normalize(avatar->getHead()->getEyePosition() - cameraPosition));
            if (angleTo < (smallestAngleTo * (isCurrentTarget ? KEEP_LOOKING_AT_CURRENT_ANGLE_FACTOR : 1.0f))) {
                _lookAtTargetAvatar = avatarPointer;
                _targetAvatarPosition = avatarPointer->getPosition();
                smallestAngleTo = angleTo;
            }
            if (Application::getInstance()->isLookingAtMyAvatar(avatar)) {

                // Alter their gaze to look directly at my camera; this looks more natural than looking at my avatar's face.
                glm::vec3 lookAtPosition = avatar->getHead()->getLookAtPosition(); // A position, in world space, on my avatar.

                // The camera isn't at the point midway between the avatar eyes. (Even without an HMD, the head can be offset a bit.)
                // Let's get everything to world space:
                glm::vec3 avatarLeftEye = getHead()->getLeftEyePosition();
                glm::vec3 avatarRightEye = getHead()->getRightEyePosition();
                // When not in HMD, these might both answer identity (i.e., the bridge of the nose). That's ok.
                // By my inpsection of the code and live testing, getEyeOffset and getEyePose are the same. (Application hands identity as offset matrix.)
                // This might be more work than needed for any given use, but as we explore different formulations, we go mad if we don't work in world space.
                glm::mat4 leftEye = Application::getInstance()->getEyeOffset(Eye::Left);
                glm::mat4 rightEye = Application::getInstance()->getEyeOffset(Eye::Right);
                glm::vec3 leftEyeHeadLocal = glm::vec3(leftEye[3]);
                glm::vec3 rightEyeHeadLocal = glm::vec3(rightEye[3]);
                auto humanSystem = Application::getInstance()->getViewFrustum();
                glm::vec3 humanLeftEye = humanSystem->getPosition() + (humanSystem->getOrientation() * leftEyeHeadLocal);
                glm::vec3 humanRightEye = humanSystem->getPosition() + (humanSystem->getOrientation() * rightEyeHeadLocal);


                // First find out where (in world space) the person is looking relative to that bridge-of-the-avatar point.
                // (We will be adding that offset to the camera position, after making some other adjustments.)
                glm::vec3 gazeOffset = lookAtPosition - getHead()->getEyePosition();

                // Scale by proportional differences between avatar and human.
                float humanEyeSeparationInModelSpace = glm::length(humanLeftEye - humanRightEye);
                float avatarEyeSeparation = glm::length(avatarLeftEye - avatarRightEye);
                gazeOffset = gazeOffset * humanEyeSeparationInModelSpace / avatarEyeSeparation;

                // If the camera is also not oriented with the head, adjust by getting the offset in head-space...
                /* Not needed (i.e., code is a no-op), but I'm leaving the example code here in case something like this is needed someday.
                glm::quat avatarHeadOrientation = getHead()->getOrientation();
                glm::vec3 gazeOffsetLocalToHead = glm::inverse(avatarHeadOrientation) * gazeOffset;
                // ... and treat that as though it were in camera space, bringing it back to world space.
                // But camera is fudged to make the picture feel like the avatar's orientation.
                glm::quat humanOrientation = humanSystem->getOrientation(); // or just avatar getOrienation() ?
                gazeOffset = humanOrientation * gazeOffsetLocalToHead;
                glm::vec3 corrected = humanSystem->getPosition() + gazeOffset;
               */

                // And now we can finally add that offset to the camera.
                glm::vec3 corrected = Application::getInstance()->getViewFrustum()->getPosition() + gazeOffset;

                avatar->getHead()->setCorrectedLookAtPosition(corrected);

            } else {
                avatar->getHead()->clearCorrectedLookAtPosition();
            }
        } else {
            avatar->getHead()->clearCorrectedLookAtPosition();
        }
    }
    auto avatarPointer = _lookAtTargetAvatar.lock();
    if (avatarPointer) {
        static_pointer_cast<Avatar>(avatarPointer)->setIsLookAtTarget(true);
    }
}

void MyAvatar::clearLookAtTargetAvatar() {
    _lookAtTargetAvatar.reset();
}

eyeContactTarget MyAvatar::getEyeContactTarget() {
    float const CHANCE_OF_CHANGING_TARGET = 0.01f;
    if (randFloat() < CHANCE_OF_CHANGING_TARGET) {
        float const FIFTY_FIFTY_CHANCE = 0.5f;
        switch (_eyeContactTarget) {
            case LEFT_EYE:
                _eyeContactTarget = (randFloat() < FIFTY_FIFTY_CHANCE) ? MOUTH : RIGHT_EYE;
                break;
            case RIGHT_EYE:
                _eyeContactTarget = (randFloat() < FIFTY_FIFTY_CHANCE) ? LEFT_EYE : MOUTH;
                break;
            case MOUTH:
                _eyeContactTarget = (randFloat() < FIFTY_FIFTY_CHANCE) ? RIGHT_EYE : LEFT_EYE;
                break;
        }
    }

    return _eyeContactTarget;
}

glm::vec3 MyAvatar::getDefaultEyePosition() const {
    return getPosition() + getWorldAlignedOrientation() * _skeletonModel.getDefaultEyeModelPosition();
}

const float SCRIPT_PRIORITY = DEFAULT_PRIORITY + 1.0f;
const float RECORDER_PRIORITY = SCRIPT_PRIORITY + 1.0f;

void MyAvatar::setJointRotations(QVector<glm::quat> jointRotations) {
    int numStates = glm::min(_skeletonModel.getJointStateCount(), jointRotations.size());
    for (int i = 0; i < numStates; ++i) {
        // HACK: ATM only Recorder calls setJointRotations() so we hardcode its priority here
        _skeletonModel.setJointRotation(i, true, jointRotations[i], RECORDER_PRIORITY);
    }
}

void MyAvatar::setJointTranslations(QVector<glm::vec3> jointTranslations) {
    int numStates = glm::min(_skeletonModel.getJointStateCount(), jointTranslations.size());
    for (int i = 0; i < numStates; ++i) {
        // HACK: ATM only Recorder calls setJointTranslations() so we hardcode its priority here
        _skeletonModel.setJointTranslation(i, true, jointTranslations[i], RECORDER_PRIORITY);
    }
}

void MyAvatar::setJointData(int index, const glm::quat& rotation, const glm::vec3& translation) {
    if (QThread::currentThread() == thread()) {
        // HACK: ATM only JS scripts call setJointData() on MyAvatar so we hardcode the priority
        _rig->setJointState(index, true, rotation, translation, SCRIPT_PRIORITY);
    }
}

void MyAvatar::setJointRotation(int index, const glm::quat& rotation) {
    if (QThread::currentThread() == thread()) {
        // HACK: ATM only JS scripts call setJointData() on MyAvatar so we hardcode the priority
        _rig->setJointRotation(index, true, rotation, SCRIPT_PRIORITY);
    }
}

void MyAvatar::setJointTranslation(int index, const glm::vec3& translation) {
    if (QThread::currentThread() == thread()) {
        // HACK: ATM only JS scripts call setJointData() on MyAvatar so we hardcode the priority
        _rig->setJointTranslation(index, true, translation, SCRIPT_PRIORITY);
    }
}

void MyAvatar::clearJointData(int index) {
    if (QThread::currentThread() == thread()) {
        // HACK: ATM only JS scripts call clearJointData() on MyAvatar so we hardcode the priority
        _rig->setJointState(index, false, glm::quat(), glm::vec3(), 0.0f);
        _rig->clearJointAnimationPriority(index);
    }
}

void MyAvatar::clearJointsData() {
    clearJointAnimationPriorities();
}

void MyAvatar::clearJointAnimationPriorities() {
    int numStates = _skeletonModel.getJointStateCount();
    for (int i = 0; i < numStates; ++i) {
        _rig->clearJointAnimationPriority(i);
    }
}

void MyAvatar::setFaceModelURL(const QUrl& faceModelURL) {

    Avatar::setFaceModelURL(faceModelURL);
    render::ScenePointer scene = Application::getInstance()->getMain3DScene();
    getHead()->getFaceModel().setVisibleInScene(_prevShouldDrawHead, scene);
    _billboardValid = false;
}

void MyAvatar::setSkeletonModelURL(const QUrl& skeletonModelURL) {

    Avatar::setSkeletonModelURL(skeletonModelURL);
    render::ScenePointer scene = Application::getInstance()->getMain3DScene();
    _billboardValid = false;
    _skeletonModel.setVisibleInScene(true, scene);
    _headBoneSet.clear();
}

void MyAvatar::useFullAvatarURL(const QUrl& fullAvatarURL, const QString& modelName) {

    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "useFullAvatarURL", Qt::BlockingQueuedConnection,
                                  Q_ARG(const QUrl&, fullAvatarURL),
                                  Q_ARG(const QString&, modelName));
        return;
    }

    if (_fullAvatarURLFromPreferences != fullAvatarURL) {
        _fullAvatarURLFromPreferences = fullAvatarURL;
        if (modelName.isEmpty()) {
            QVariantHash fullAvatarFST = FSTReader::downloadMapping(_fullAvatarURLFromPreferences.toString());
            _fullAvatarModelName = fullAvatarFST["name"].toString();
        } else {
            _fullAvatarModelName = modelName;
        }
    }

    if (!getFaceModelURLString().isEmpty()) {
        setFaceModelURL(QString());
    }

    const QString& urlString = fullAvatarURL.toString();
    if (urlString.isEmpty() || (fullAvatarURL != getSkeletonModelURL())) {
        bool isRigEnabled = getEnableRigAnimations();
        bool isGraphEnabled = getEnableAnimGraph();
        qApp->setRawAvatarUpdateThreading(false);
        setEnableRigAnimations(false);
        setEnableAnimGraph(false);

        setSkeletonModelURL(fullAvatarURL);

        setEnableRigAnimations(isRigEnabled);
        setEnableAnimGraph(isGraphEnabled);
        qApp->setRawAvatarUpdateThreading();
        UserActivityLogger::getInstance().changedModel("skeleton", urlString);
    }
    sendIdentityPacket();
}

void MyAvatar::setAttachmentData(const QVector<AttachmentData>& attachmentData) {
    Avatar::setAttachmentData(attachmentData);
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setAttachmentData", Qt::DirectConnection,
                                  Q_ARG(const QVector<AttachmentData>, attachmentData));
        return;
    }
    _billboardValid = false;
}

glm::vec3 MyAvatar::getSkeletonPosition() const {
    CameraMode mode = Application::getInstance()->getCamera()->getMode();
    if (mode == CAMERA_MODE_THIRD_PERSON || mode == CAMERA_MODE_INDEPENDENT) {
        // The avatar is rotated PI about the yAxis, so we have to correct for it
        // to get the skeleton offset contribution in the world-frame.
        const glm::quat FLIP = glm::angleAxis(PI, glm::vec3(0.0f, 1.0f, 0.0f));
        return _position + getOrientation() * FLIP * _skeletonOffset;
    }
    return Avatar::getPosition();
}

void MyAvatar::rebuildSkeletonBody() {
    // compute localAABox
    float radius = _skeletonModel.getBoundingCapsuleRadius();
    float height = _skeletonModel.getBoundingCapsuleHeight() + 2.0f * radius;
    glm::vec3 corner(-radius, -0.5f * height, -radius);
    corner += _skeletonModel.getBoundingCapsuleOffset();
    glm::vec3 scale(2.0f * radius, height, 2.0f * radius);
    _characterController.setLocalBoundingBox(corner, scale);
}

QString MyAvatar::getScriptedMotorFrame() const {
    QString frame = "avatar";
    if (_scriptedMotorFrame == SCRIPTED_MOTOR_CAMERA_FRAME) {
        frame = "camera";
    } else if (_scriptedMotorFrame == SCRIPTED_MOTOR_WORLD_FRAME) {
        frame = "world";
    }
    return frame;
}

void MyAvatar::setScriptedMotorVelocity(const glm::vec3& velocity) {
    float MAX_SCRIPTED_MOTOR_SPEED = 500.0f;
    _scriptedMotorVelocity = velocity;
    float speed = glm::length(_scriptedMotorVelocity);
    if (speed > MAX_SCRIPTED_MOTOR_SPEED) {
        _scriptedMotorVelocity *= MAX_SCRIPTED_MOTOR_SPEED / speed;
    }
}

void MyAvatar::setScriptedMotorTimescale(float timescale) {
    // we clamp the timescale on the large side (instead of just the low side) to prevent
    // obnoxiously large values from introducing NaN into avatar's velocity
    _scriptedMotorTimescale = glm::clamp(timescale, MIN_SCRIPTED_MOTOR_TIMESCALE,
            DEFAULT_SCRIPTED_MOTOR_TIMESCALE);
}

void MyAvatar::setScriptedMotorFrame(QString frame) {
    if (frame.toLower() == "camera") {
        _scriptedMotorFrame = SCRIPTED_MOTOR_CAMERA_FRAME;
    } else if (frame.toLower() == "avatar") {
        _scriptedMotorFrame = SCRIPTED_MOTOR_AVATAR_FRAME;
    } else if (frame.toLower() == "world") {
        _scriptedMotorFrame = SCRIPTED_MOTOR_WORLD_FRAME;
    }
}

void MyAvatar::clearScriptableSettings() {
    clearJointAnimationPriorities();
    _scriptedMotorVelocity = glm::vec3(0.0f);
    _scriptedMotorTimescale = DEFAULT_SCRIPTED_MOTOR_TIMESCALE;
}

void MyAvatar::setCollisionSoundURL(const QString& url) {
    _collisionSoundURL = url;
    if (!url.isEmpty() && (url != _collisionSoundURL)) {
        emit newCollisionSoundURL(QUrl(url));
    }
}

void MyAvatar::attach(const QString& modelURL, const QString& jointName, const glm::vec3& translation,
        const glm::quat& rotation, float scale, bool allowDuplicates, bool useSaved) {
    if (QThread::currentThread() != thread()) {
        Avatar::attach(modelURL, jointName, translation, rotation, scale, allowDuplicates, useSaved);
        return;
    }
    if (useSaved) {
        AttachmentData attachment = loadAttachmentData(modelURL, jointName);
        if (attachment.isValid()) {
            Avatar::attach(modelURL, attachment.jointName, attachment.translation,
                attachment.rotation, attachment.scale, allowDuplicates, useSaved);
            return;
        }
    }
    Avatar::attach(modelURL, jointName, translation, rotation, scale, allowDuplicates, useSaved);
}

void MyAvatar::renderBody(RenderArgs* renderArgs, ViewFrustum* renderFrustum, float glowLevel) {

    if (!_skeletonModel.isRenderable()) {
        return; // wait until all models are loaded
    }

    fixupModelsInScene();

    //  Render head so long as the camera isn't inside it
    if (shouldRenderHead(renderArgs)) {
        getHead()->render(renderArgs, 1.0f, renderFrustum);
    }

    // This is drawing the lookat vectors from our avatar to wherever we're looking.
    if (qApp->isHMDMode()) {
        glm::vec3 cameraPosition = Application::getInstance()->getCamera()->getPosition();

        glm::mat4 leftEyePose = Application::getInstance()->getActiveDisplayPlugin()->getEyePose(Eye::Left);
        glm::vec3 leftEyePosition = glm::vec3(leftEyePose[3]);
        glm::mat4 rightEyePose = Application::getInstance()->getActiveDisplayPlugin()->getEyePose(Eye::Right);
        glm::vec3 rightEyePosition = glm::vec3(rightEyePose[3]);
        glm::mat4 headPose = Application::getInstance()->getActiveDisplayPlugin()->getHeadPose();
        glm::vec3 headPosition = glm::vec3(headPose[3]);

        getHead()->renderLookAts(renderArgs,
            cameraPosition + getOrientation() * (leftEyePosition - headPosition),
            cameraPosition + getOrientation() * (rightEyePosition - headPosition));
    } else {
        getHead()->renderLookAts(renderArgs);
    }

    if (renderArgs->_renderMode != RenderArgs::SHADOW_RENDER_MODE && 
            Menu::getInstance()->isOptionChecked(MenuOption::DisplayHandTargets)) {
        getHand()->renderHandTargets(renderArgs, true);
    }
}

void MyAvatar::setVisibleInSceneIfReady(Model* model, render::ScenePointer scene, bool visible) {
    if (model->isActive() && model->isRenderable()) {
        model->setVisibleInScene(visible, scene);
    }
}

void MyAvatar::initHeadBones() {
    int neckJointIndex = -1;
    if (_skeletonModel.getGeometry()) {
        neckJointIndex = _skeletonModel.getGeometry()->getFBXGeometry().neckJointIndex;
    }
    if (neckJointIndex == -1) {
        return;
    }
    _headBoneSet.clear();
    std::queue<int> q;
    q.push(neckJointIndex);
    _headBoneSet.insert(neckJointIndex);

    // fbxJoints only hold links to parents not children, so we have to do a bit of extra work here.
    while (q.size() > 0) {
        int jointIndex = q.front();
        for (int i = 0; i < _skeletonModel.getJointStateCount(); i++) {
            if (jointIndex == _skeletonModel.getParentJointIndex(i)) {
                _headBoneSet.insert(i);
                q.push(i);
            }
        }
        q.pop();
    }
}

void MyAvatar::initAnimGraph() {
    // avatar.json
    // https://gist.github.com/hyperlogic/7d6a0892a7319c69e2b9
    //
    // ik-avatar.json
    // https://gist.github.com/hyperlogic/e58e0a24cc341ad5d060
    //
    // ik-avatar-hands.json
    // https://gist.githubusercontent.com/hyperlogic/04a02c47eb56d8bfaebb
    //
    // ik-avatar-hands-idle.json
    // https://gist.githubusercontent.com/hyperlogic/d951c78532e7a20557ad
    //
    // or run a local web-server
    // python -m SimpleHTTPServer&
    //auto graphUrl = QUrl("http://localhost:8000/avatar.json");
    auto graphUrl = QUrl(_animGraphUrl.isEmpty() ?
                         QUrl::fromLocalFile(PathUtils::resourcesPath() + "meshes/defaultAvatar_full/avatar-animation.json") :
                         _animGraphUrl);
    _rig->initAnimGraph(graphUrl, _skeletonModel.getGeometry()->getFBXGeometry());
}

void MyAvatar::destroyAnimGraph() {
    _rig->destroyAnimGraph();
}

void MyAvatar::preRender(RenderArgs* renderArgs) {

    render::ScenePointer scene = Application::getInstance()->getMain3DScene();
    const bool shouldDrawHead = shouldRenderHead(renderArgs);

    if (_skeletonModel.initWhenReady(scene)) {
        initHeadBones();
        _skeletonModel.setCauterizeBoneSet(_headBoneSet);
        initAnimGraph();
        _debugDrawSkeleton = std::make_shared<AnimSkeleton>(_skeletonModel.getGeometry()->getFBXGeometry());
    }

    if (_enableDebugDrawBindPose || _enableDebugDrawAnimPose) {

        // bones are aligned such that z is forward, not -z.
        glm::quat rotY180 = glm::angleAxis((float)M_PI, glm::vec3(0.0f, 1.0f, 0.0f));
        AnimPose xform(glm::vec3(1), getOrientation() * rotY180, getPosition());

        if (_enableDebugDrawBindPose && _debugDrawSkeleton) {
            glm::vec4 gray(0.2f, 0.2f, 0.2f, 0.2f);
            AnimDebugDraw::getInstance().addSkeleton("myAvatar", _debugDrawSkeleton, xform, gray);
        }

        if (_enableDebugDrawAnimPose && _debugDrawSkeleton) {
            glm::vec4 cyan(0.1f, 0.6f, 0.6f, 1.0f);

            // build AnimPoseVec from JointStates.
            AnimPoseVec poses;
            poses.reserve(_debugDrawSkeleton->getNumJoints());
            for (int i = 0; i < _debugDrawSkeleton->getNumJoints(); i++) {
                AnimPose pose = _debugDrawSkeleton->getRelativeBindPose(i);
                glm::quat jointRot;
                _rig->getJointRotationInConstrainedFrame(i, jointRot);
                glm::vec3 jointTrans;
                _rig->getJointTranslation(i, jointTrans);
                pose.rot = pose.rot * jointRot;
                pose.trans = jointTrans;
                poses.push_back(pose);
            }

            AnimDebugDraw::getInstance().addPoses("myAvatar", _debugDrawSkeleton, poses, xform, cyan);
        }
    }

    DebugDraw::getInstance().updateMyAvatarPos(getPosition());
    DebugDraw::getInstance().updateMyAvatarRot(getOrientation());

    if (shouldDrawHead != _prevShouldDrawHead) {
        _skeletonModel.setCauterizeBones(!shouldDrawHead);
    }
    _prevShouldDrawHead = shouldDrawHead;
}

const float RENDER_HEAD_CUTOFF_DISTANCE = 0.50f;

bool MyAvatar::cameraInsideHead() const {
    const Head* head = getHead();
    const glm::vec3 cameraPosition = Application::getInstance()->getCamera()->getPosition();
    return glm::length(cameraPosition - head->getEyePosition()) < (RENDER_HEAD_CUTOFF_DISTANCE * _scale);
}

bool MyAvatar::shouldRenderHead(const RenderArgs* renderArgs) const {
    return ((renderArgs->_renderMode != RenderArgs::DEFAULT_RENDER_MODE) ||
            (Application::getInstance()->getCamera()->getMode() != CAMERA_MODE_FIRST_PERSON) ||
            !cameraInsideHead());
}

void MyAvatar::updateOrientation(float deltaTime) {
    //  Smoothly rotate body with arrow keys
    float targetSpeed = (_driveKeys[ROT_LEFT] - _driveKeys[ROT_RIGHT]) * YAW_SPEED;
    if (targetSpeed != 0.0f) {
        const float ROTATION_RAMP_TIMESCALE = 0.1f;
        float blend = deltaTime / ROTATION_RAMP_TIMESCALE;
        if (blend > 1.0f) {
            blend = 1.0f;
        }
        _bodyYawDelta = (1.0f - blend) * _bodyYawDelta + blend * targetSpeed;
    } else if (_bodyYawDelta != 0.0f) {
        // attenuate body rotation speed
        const float ROTATION_DECAY_TIMESCALE = 0.05f;
        float attenuation = 1.0f - deltaTime / ROTATION_DECAY_TIMESCALE;
        if (attenuation < 0.0f) {
            attenuation = 0.0f;
        }
        _bodyYawDelta *= attenuation;

        float MINIMUM_ROTATION_RATE = 2.0f;
        if (fabsf(_bodyYawDelta) < MINIMUM_ROTATION_RATE) {
            _bodyYawDelta = 0.0f;
        }
    }

    getHead()->setBasePitch(getHead()->getBasePitch() + (_driveKeys[ROT_UP] - _driveKeys[ROT_DOWN]) * PITCH_SPEED * deltaTime);
    // update body orientation by movement inputs
    setOrientation(getOrientation() *
                   glm::quat(glm::radians(glm::vec3(0.0f, _bodyYawDelta * deltaTime, 0.0f))));

    if (qApp->getAvatarUpdater()->isHMDMode()) {
        glm::quat orientation = glm::quat_cast(getSensorToWorldMatrix()) * getHMDSensorOrientation();
        glm::quat bodyOrientation = getWorldBodyOrientation();
        glm::quat localOrientation = glm::inverse(bodyOrientation) * orientation;

        // these angles will be in radians
        // ... so they need to be converted to degrees before we do math...
        glm::vec3 euler = glm::eulerAngles(localOrientation) * DEGREES_PER_RADIAN;

        //Invert yaw and roll when in mirror mode
        if (Application::getInstance()->getCamera()->getMode() == CAMERA_MODE_MIRROR) {
            YAW(euler) *= -1.0f;
            ROLL(euler) *= -1.0f;
        }

        Head* head = getHead();
        head->setBaseYaw(YAW(euler));
        head->setBasePitch(PITCH(euler));
        head->setBaseRoll(ROLL(euler));
    }
}

glm::vec3 MyAvatar::applyKeyboardMotor(float deltaTime, const glm::vec3& localVelocity, bool isHovering) {
    if (! (_motionBehaviors & AVATAR_MOTION_KEYBOARD_MOTOR_ENABLED)) {
        return localVelocity;
    }
    // compute motor efficiency
    // The timescale of the motor is the approximate time it takes for the motor to
    // accomplish its intended localVelocity.  A short timescale makes the motor strong,
    // and a long timescale makes it weak.  The value of timescale to use depends
    // on what the motor is doing:
    //
    // (1) braking --> short timescale (aggressive motor assertion)
    // (2) pushing --> medium timescale (mild motor assertion)
    // (3) inactive --> long timescale (gentle friction for low speeds)
    float MIN_KEYBOARD_MOTOR_TIMESCALE = 0.125f;
    float MAX_KEYBOARD_MOTOR_TIMESCALE = 0.4f;
    float MIN_KEYBOARD_BRAKE_SPEED = 0.3f;
    float timescale = MAX_KEYBOARD_MOTOR_TIMESCALE;
    bool isThrust = (glm::length2(_thrust) > EPSILON);
    if (_isPushing || isThrust ||
            (_scriptedMotorTimescale < MAX_KEYBOARD_MOTOR_TIMESCALE &&
            (_motionBehaviors & AVATAR_MOTION_SCRIPTED_MOTOR_ENABLED))) {
        // we don't want to brake if something is pushing the avatar around
        timescale = _keyboardMotorTimescale;
        _isBraking = false;
    } else {
        float speed = glm::length(localVelocity);
        _isBraking = _wasPushing || (_isBraking && speed > MIN_KEYBOARD_BRAKE_SPEED);
        if (_isBraking) {
            timescale = MIN_KEYBOARD_MOTOR_TIMESCALE;
        }
    }
    _wasPushing = _isPushing || isThrust;
    _isPushing = false;
    float motorEfficiency = glm::clamp(deltaTime / timescale, 0.0f, 1.0f);

    glm::vec3 newLocalVelocity = localVelocity;
    float keyboardInput = fabsf(_driveKeys[FWD] - _driveKeys[BACK]) +
        (fabsf(_driveKeys[RIGHT] - _driveKeys[LEFT])) +
        fabsf(_driveKeys[UP] - _driveKeys[DOWN]);
    if (keyboardInput) {
        // Compute keyboard input
        glm::vec3 front = (_driveKeys[FWD] - _driveKeys[BACK]) * IDENTITY_FRONT;
        glm::vec3 right = (_driveKeys[RIGHT] - _driveKeys[LEFT]) * IDENTITY_RIGHT;
        glm::vec3 up = (_driveKeys[UP] - _driveKeys[DOWN]) * IDENTITY_UP;

        glm::vec3 direction = front + right + up;
        float directionLength = glm::length(direction);

        //qCDebug(interfaceapp, "direction = (%.5f, %.5f, %.5f)", direction.x, direction.y, direction.z);

        // Compute motor magnitude
        if (directionLength > EPSILON) {
            direction /= directionLength;

            if (isHovering) {
                // we're flying --> complex acceleration curve with high max speed
                float motorSpeed = glm::length(_keyboardMotorVelocity);
                float finalMaxMotorSpeed = _scale * MAX_KEYBOARD_MOTOR_SPEED;
                float speedGrowthTimescale  = 2.0f;
                float speedIncreaseFactor = 1.8f;
                motorSpeed *= 1.0f + glm::clamp(deltaTime / speedGrowthTimescale , 0.0f, 1.0f) * speedIncreaseFactor;
                const float maxBoostSpeed = _scale * MAX_BOOST_SPEED;
                if (motorSpeed < maxBoostSpeed) {
                    // an active keyboard motor should never be slower than this
                    float boostCoefficient = (maxBoostSpeed - motorSpeed) / maxBoostSpeed;
                    motorSpeed += MIN_AVATAR_SPEED * boostCoefficient;
                    motorEfficiency += (1.0f - motorEfficiency) * boostCoefficient;
                } else if (motorSpeed > finalMaxMotorSpeed) {
                    motorSpeed = finalMaxMotorSpeed;
                }
                _keyboardMotorVelocity = motorSpeed * direction;

            } else {
                // we're using a floor --> simple exponential decay toward target walk speed
                const float WALK_ACCELERATION_TIMESCALE = 0.7f;  // seconds to decrease delta to 1/e
                _keyboardMotorVelocity = MAX_WALKING_SPEED * direction;
                motorEfficiency = glm::clamp(deltaTime / WALK_ACCELERATION_TIMESCALE, 0.0f, 1.0f);
            }
            _isPushing = true;
        }
        newLocalVelocity = localVelocity + motorEfficiency * (_keyboardMotorVelocity - localVelocity);
    } else {
        _keyboardMotorVelocity = glm::vec3(0.0f);
        newLocalVelocity = (1.0f - motorEfficiency) * localVelocity;
        if (!isHovering && !_wasPushing) {
            float speed = glm::length(newLocalVelocity);
            if (speed > MIN_AVATAR_SPEED) {
                // add small constant friction to help avatar drift to a stop sooner at low speeds
                const float CONSTANT_FRICTION_DECELERATION = MIN_AVATAR_SPEED / 0.20f;
                newLocalVelocity *= (speed - timescale * CONSTANT_FRICTION_DECELERATION) / speed;
            }
        }
    }

    float boomChange = _driveKeys[BOOM_OUT] - _driveKeys[BOOM_IN];
    _boomLength += 2.0f * _boomLength * boomChange + boomChange * boomChange;
    _boomLength = glm::clamp<float>(_boomLength, ZOOM_MIN, ZOOM_MAX);

    return newLocalVelocity;
}

glm::vec3 MyAvatar::applyScriptedMotor(float deltaTime, const glm::vec3& localVelocity) {
    // NOTE: localVelocity is in camera-frame because that's the frame of the default avatar motor
    if (! (_motionBehaviors & AVATAR_MOTION_SCRIPTED_MOTOR_ENABLED)) {
        return localVelocity;
    }
    glm::vec3 deltaVelocity(0.0f);
    if (_scriptedMotorFrame == SCRIPTED_MOTOR_CAMERA_FRAME) {
        // camera frame
        deltaVelocity = _scriptedMotorVelocity - localVelocity;
    } else if (_scriptedMotorFrame == SCRIPTED_MOTOR_AVATAR_FRAME) {
        // avatar frame
        glm::quat rotation = glm::inverse(getHead()->getCameraOrientation()) * getOrientation();
        deltaVelocity = rotation * _scriptedMotorVelocity - localVelocity;
    } else {
        // world-frame
        glm::quat rotation = glm::inverse(getHead()->getCameraOrientation());
        deltaVelocity = rotation * _scriptedMotorVelocity - localVelocity;
    }
    float motorEfficiency = glm::clamp(deltaTime / _scriptedMotorTimescale, 0.0f, 1.0f);
    return localVelocity + motorEfficiency * deltaVelocity;
}

void MyAvatar::updatePosition(float deltaTime) {
    // rotate velocity into camera frame
    glm::quat rotation = getHead()->getCameraOrientation();
    glm::vec3 localVelocity = glm::inverse(rotation) * _targetVelocity;

    bool isHovering = _characterController.isHovering();
    glm::vec3 newLocalVelocity = applyKeyboardMotor(deltaTime, localVelocity, isHovering);
    newLocalVelocity = applyScriptedMotor(deltaTime, newLocalVelocity);

    // rotate back into world-frame
    _targetVelocity = rotation * newLocalVelocity;

    _targetVelocity += _thrust * deltaTime;
    _thrust = glm::vec3(0.0f);

    // cap avatar speed
    float speed = glm::length(_targetVelocity);
    if (speed > MAX_AVATAR_SPEED) {
        _targetVelocity *= MAX_AVATAR_SPEED / speed;
        speed = MAX_AVATAR_SPEED;
    }

    if (speed > MIN_AVATAR_SPEED && !_characterController.isEnabled()) {
        // update position ourselves
        applyPositionDelta(deltaTime * _targetVelocity);
        measureMotionDerivatives(deltaTime);
    } // else physics will move avatar later

    // update _moving flag based on speed
    const float MOVING_SPEED_THRESHOLD = 0.01f;
    _moving = speed > MOVING_SPEED_THRESHOLD;
}

void MyAvatar::updateCollisionSound(const glm::vec3 &penetration, float deltaTime, float frequency) {
    // COLLISION SOUND API in Audio has been removed
}

bool findAvatarAvatarPenetration(const glm::vec3 positionA, float radiusA, float heightA,
        const glm::vec3 positionB, float radiusB, float heightB, glm::vec3& penetration) {
    glm::vec3 positionBA = positionB - positionA;
    float xzDistance = sqrt(positionBA.x * positionBA.x + positionBA.z * positionBA.z);
    if (xzDistance < (radiusA + radiusB)) {
        float yDistance = fabs(positionBA.y);
        float halfHeights = 0.5f * (heightA + heightB);
        if (yDistance < halfHeights) {
            // cylinders collide
            if (xzDistance > 0.0f) {
                positionBA.y = 0.0f;
                // note, penetration should point from A into B
                penetration = positionBA * ((radiusA + radiusB - xzDistance) / xzDistance);
                return true;
            } else {
                // exactly coaxial -- we'll return false for this case
                return false;
            }
        } else if (yDistance < halfHeights + radiusA + radiusB) {
            // caps collide
            if (positionBA.y < 0.0f) {
                // A is above B
                positionBA.y += halfHeights;
                float BA = glm::length(positionBA);
                penetration = positionBA * (radiusA + radiusB - BA) / BA;
                return true;
            } else {
                // A is below B
                positionBA.y -= halfHeights;
                float BA = glm::length(positionBA);
                penetration = positionBA * (radiusA + radiusB - BA) / BA;
                return true;
            }
        }
    }
    return false;
}

void MyAvatar::maybeUpdateBillboard() {
    qApp->getAvatarUpdater()->setRequestBillboardUpdate(false);
    if (_billboardValid || !(_skeletonModel.isLoadedWithTextures() && getHead()->getFaceModel().isLoadedWithTextures())) {
        return;
    }
    foreach (Model* model, _attachmentModels) {
        if (!model->isLoadedWithTextures()) {
            return;
        }
    }
    qApp->getAvatarUpdater()->setRequestBillboardUpdate(true);
}
void MyAvatar::doUpdateBillboard() {
    RenderArgs renderArgs(qApp->getGPUContext());
    QImage image = qApp->renderAvatarBillboard(&renderArgs);
    _billboard.clear();
    QBuffer buffer(&_billboard);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
#ifdef DEBUG
    image.save("billboard.png", "PNG");
#endif
    _billboardValid = true;

    sendBillboardPacket();
}

void MyAvatar::increaseSize() {
    if ((1.0f + SCALING_RATIO) * _targetScale < MAX_AVATAR_SCALE) {
        _targetScale *= (1.0f + SCALING_RATIO);
        qCDebug(interfaceapp, "Changed scale to %f", (double)_targetScale);
    }
}

void MyAvatar::decreaseSize() {
    if (MIN_AVATAR_SCALE < (1.0f - SCALING_RATIO) * _targetScale) {
        _targetScale *= (1.0f - SCALING_RATIO);
        qCDebug(interfaceapp, "Changed scale to %f", (double)_targetScale);
    }
}

void MyAvatar::resetSize() {
    _targetScale = 1.0f;
    qCDebug(interfaceapp, "Reseted scale to %f", (double)_targetScale);
}

void MyAvatar::goToLocation(const glm::vec3& newPosition,
                            bool hasOrientation, const glm::quat& newOrientation,
                            bool shouldFaceLocation) {

    qCDebug(interfaceapp).nospace() << "MyAvatar goToLocation - moving to " << newPosition.x << ", "
        << newPosition.y << ", " << newPosition.z;

    _goToPending = true;
    _goToPosition = newPosition;
    _goToOrientation = getOrientation();
    if (hasOrientation) {
        qCDebug(interfaceapp).nospace() << "MyAvatar goToLocation - new orientation is "
                                        << newOrientation.x << ", " << newOrientation.y << ", " << newOrientation.z << ", " << newOrientation.w;

        // orient the user to face the target
        glm::quat quatOrientation = newOrientation;

        if (shouldFaceLocation) {
            quatOrientation = newOrientation * glm::angleAxis(PI, glm::vec3(0.0f, 1.0f, 0.0f));

            // move the user a couple units away
            const float DISTANCE_TO_USER = 2.0f;
            _goToPosition = newPosition - quatOrientation * IDENTITY_FRONT * DISTANCE_TO_USER;
        }

        _goToOrientation = quatOrientation;
    }

    emit transformChanged();
}

void MyAvatar::updateMotionBehaviorFromMenu() {

    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "updateMotionBehaviorFromMenu");
        return;
    }
    
    Menu* menu = Menu::getInstance();
    if (menu->isOptionChecked(MenuOption::KeyboardMotorControl)) {
        _motionBehaviors |= AVATAR_MOTION_KEYBOARD_MOTOR_ENABLED;
    } else {
        _motionBehaviors &= ~AVATAR_MOTION_KEYBOARD_MOTOR_ENABLED;
    }
    if (menu->isOptionChecked(MenuOption::ScriptedMotorControl)) {
        _motionBehaviors |= AVATAR_MOTION_SCRIPTED_MOTOR_ENABLED;
    } else {
        _motionBehaviors &= ~AVATAR_MOTION_SCRIPTED_MOTOR_ENABLED;
    }
    _characterController.setEnabled(menu->isOptionChecked(MenuOption::EnableCharacterController));
}

//Renders sixense laser pointers for UI selection with controllers
void MyAvatar::renderLaserPointers(gpu::Batch& batch) {
    const float PALM_TIP_ROD_RADIUS = 0.002f;

    //If the Oculus is enabled, we will draw a blue cursor ray

    for (size_t i = 0; i < getHand()->getNumPalms(); ++i) {
        PalmData& palm = getHand()->getPalms()[i];
        if (palm.isActive()) {
            glm::vec3 tip = getLaserPointerTipPosition(&palm);
            glm::vec3 root = palm.getPosition();

            //Scale the root vector with the avatar scale
            scaleVectorRelativeToPosition(root);
            Transform transform = Transform();
            transform.setTranslation(glm::vec3());
            batch.setModelTransform(transform);
            Avatar::renderJointConnectingCone(batch, root, tip, PALM_TIP_ROD_RADIUS, PALM_TIP_ROD_RADIUS, glm::vec4(0, 1, 1, 1));
        }
    }
}

//Gets the tip position for the laser pointer
glm::vec3 MyAvatar::getLaserPointerTipPosition(const PalmData* palm) {
    glm::vec3 direction = glm::normalize(palm->getTipPosition() - palm->getPosition());

    glm::vec3 position = palm->getPosition();
    //scale the position with the avatar
    scaleVectorRelativeToPosition(position);


    glm::vec3 result;
    const auto& compositor = qApp->getApplicationCompositor();
    if (compositor.calculateRayUICollisionPoint(position, direction, result)) {
        return result;
    }

    return palm->getPosition();
}

void MyAvatar::clearDriveKeys() {
    for (int i = 0; i < MAX_DRIVE_KEYS; ++i) {
        _driveKeys[i] = 0.0f;
    }
}

void MyAvatar::relayDriveKeysToCharacterController() {
    if (_driveKeys[UP] > 0.0f) {
        _characterController.jump();
    }
}

glm::vec3 MyAvatar::getWorldBodyPosition() const {
    return transformPoint(_sensorToWorldMatrix, extractTranslation(_bodySensorMatrix));
}

glm::quat MyAvatar::getWorldBodyOrientation() const {
    return glm::quat_cast(_sensorToWorldMatrix * _bodySensorMatrix);
}

// derive avatar body position and orientation from the current HMD Sensor location.
// results are in sensor space
glm::mat4 MyAvatar::deriveBodyFromHMDSensor() const {

    // HMD is in sensor space.
    const glm::vec3 hmdPosition = getHMDSensorPosition();
    const glm::quat hmdOrientation = getHMDSensorOrientation();
    const glm::quat hmdOrientationYawOnly = cancelOutRollAndPitch(hmdOrientation);

    const glm::vec3 DEFAULT_RIGHT_EYE_POS(-0.3f, 1.6f, 0.0f);
    const glm::vec3 DEFAULT_LEFT_EYE_POS(0.3f, 1.6f, 0.0f);
    const glm::vec3 DEFAULT_NECK_POS(0.0f, 1.5f, 0.0f);
    const glm::vec3 DEFAULT_HIPS_POS(0.0f, 1.0f, 0.0f);

    vec3 localEyes, localNeck;
    if (!_debugDrawSkeleton) {
        const glm::quat rotY180 = glm::angleAxis((float)PI, glm::vec3(0.0f, 1.0f, 0.0f));
        localEyes = rotY180 * (((DEFAULT_RIGHT_EYE_POS + DEFAULT_LEFT_EYE_POS) / 2.0f) - DEFAULT_HIPS_POS);
        localNeck = rotY180 * (DEFAULT_NECK_POS - DEFAULT_HIPS_POS);
    } else {
        // TODO: At the moment MyAvatar does not have access to the rig, which has the skeleton, which has the bind poses.
        // for now use the _debugDrawSkeleton, which is initialized with the same FBX model as the rig.

        // TODO: cache these indices.
        int rightEyeIndex = _debugDrawSkeleton->nameToJointIndex("RightEye");
        int leftEyeIndex = _debugDrawSkeleton->nameToJointIndex("LeftEye");
        int neckIndex = _debugDrawSkeleton->nameToJointIndex("Neck");
        int hipsIndex = _debugDrawSkeleton->nameToJointIndex("Hips");

        glm::vec3 absRightEyePos = rightEyeIndex != -1 ? _debugDrawSkeleton->getAbsoluteBindPose(rightEyeIndex).trans : DEFAULT_RIGHT_EYE_POS;
        glm::vec3 absLeftEyePos = leftEyeIndex != -1 ? _debugDrawSkeleton->getAbsoluteBindPose(leftEyeIndex).trans : DEFAULT_LEFT_EYE_POS;
        glm::vec3 absNeckPos = neckIndex != -1 ? _debugDrawSkeleton->getAbsoluteBindPose(neckIndex).trans : DEFAULT_NECK_POS;
        glm::vec3 absHipsPos = neckIndex != -1 ? _debugDrawSkeleton->getAbsoluteBindPose(hipsIndex).trans : DEFAULT_HIPS_POS;

        const glm::quat rotY180 = glm::angleAxis((float)PI, glm::vec3(0.0f, 1.0f, 0.0f));
        localEyes = rotY180 * (((absRightEyePos + absLeftEyePos) / 2.0f) - absHipsPos);
        localNeck = rotY180 * (absNeckPos - absHipsPos);
    }

    // apply simplistic head/neck model
    // figure out where the avatar body should be by applying offsets from the avatar's neck & head joints.

    // eyeToNeck offset is relative full HMD orientation.
    // while neckToRoot offset is only relative to HMDs yaw.
    glm::vec3 eyeToNeck = hmdOrientation * (localNeck - localEyes);
    glm::vec3 neckToRoot = hmdOrientationYawOnly * -localNeck;
    glm::vec3 bodyPos = hmdPosition + eyeToNeck + neckToRoot;

    // avatar facing is determined solely by hmd orientation.
    return createMatFromQuatAndPos(hmdOrientationYawOnly, bodyPos);
}

glm::vec3 MyAvatar::getPositionForAudio() {
    switch (_audioListenerMode) {
        case AudioListenerMode::FROM_HEAD:
            return getHead()->getPosition();
        case AudioListenerMode::FROM_CAMERA:
            return Application::getInstance()->getCamera()->getPosition();
        case AudioListenerMode::CUSTOM:
            return _customListenPosition;
    }
    return vec3();
}

glm::quat MyAvatar::getOrientationForAudio() {
    switch (_audioListenerMode) {
        case AudioListenerMode::FROM_HEAD:
            return getHead()->getFinalOrientationInWorldFrame();
        case AudioListenerMode::FROM_CAMERA:
            return Application::getInstance()->getCamera()->getOrientation();
        case AudioListenerMode::CUSTOM:
            return _customListenOrientation;
    }
    return quat();
}

void MyAvatar::setAudioListenerMode(AudioListenerMode audioListenerMode) {
    if (_audioListenerMode != audioListenerMode) {
        _audioListenerMode = audioListenerMode;
        emit audioListenerModeChanged();
    }
}

QScriptValue audioListenModeToScriptValue(QScriptEngine* engine, const AudioListenerMode& audioListenerMode) {
    return audioListenerMode;
}

void audioListenModeFromScriptValue(const QScriptValue& object, AudioListenerMode& audioListenerMode) {
    audioListenerMode = (AudioListenerMode)object.toUInt16();
}
