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
#include <GeometryUtil.h>
#include <NodeList.h>
#include <udt/PacketHeaders.h>
#include <PathUtils.h>
#include <PerfStat.h>
#include <ShapeCollider.h>
#include <SharedUtil.h>
#include <TextRenderer3D.h>
#include <UserActivityLogger.h>

#include "devices/Faceshift.h"
#include "devices/OculusManager.h"

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

#include "gpu/GLBackend.h"


using namespace std;

const glm::vec3 DEFAULT_UP_DIRECTION(0.0f, 1.0f, 0.0f);
const float YAW_SPEED = 500.0f;   // degrees/sec
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
const QString& DEFAULT_AVATAR_COLLISION_SOUND_URL = "https://s3.amazonaws.com/hifi-public/sounds/Collisions-hitsandslaps/airhockey_hit1.wav";

const float MyAvatar::ZOOM_MIN = 0.5f;
const float MyAvatar::ZOOM_MAX = 25.0f;
const float MyAvatar::ZOOM_DEFAULT = 1.5f;

MyAvatar::MyAvatar() :
	Avatar(),
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
    _feetTouchFloor(true),
    _eyeContactTarget(LEFT_EYE),
    _realWorldFieldOfView("realWorldFieldOfView",
                          DEFAULT_REAL_WORLD_FIELD_OF_VIEW_DEGREES),
    _firstPersonSkeletonModel(this),
    _prevShouldDrawHead(true)
{
    _firstPersonSkeletonModel.setIsFirstPerson(true);

    ShapeCollider::initDispatchTable();
    for (int i = 0; i < MAX_DRIVE_KEYS; i++) {
        _driveKeys[i] = 0.0f;
    }

    _skeletonModel.setEnableShapes(true);

    // connect to AddressManager signal for location jumps
    connect(DependencyManager::get<AddressManager>().data(), &AddressManager::locationChangeRequired,
            this, &MyAvatar::goToLocation);
    _characterController.setEnabled(true);
}

MyAvatar::~MyAvatar() {
    _lookAtTargetAvatar.reset();
}

QByteArray MyAvatar::toByteArray() {
    CameraMode mode = Application::getInstance()->getCamera()->getMode();
    if (mode == CAMERA_MODE_THIRD_PERSON || mode == CAMERA_MODE_INDEPENDENT) {
        // fake the avatar position that is sent up to the AvatarMixer
        glm::vec3 oldPosition = _position;
        _position = getSkeletonPosition();
        QByteArray array = AvatarData::toByteArray();
        // copy the correct position back
        _position = oldPosition;
        return array;
    }
    return AvatarData::toByteArray();
}

void MyAvatar::reset() {
    _skeletonModel.reset();
    _firstPersonSkeletonModel.reset();
    getHead()->reset();

    _targetVelocity = glm::vec3(0.0f);
    setThrust(glm::vec3(0.0f));
    //  Reset the pitch and roll components of the avatar's orientation, preserve yaw direction
    glm::vec3 eulers = safeEulerAngles(getOrientation());
    eulers.x = 0.0f;
    eulers.z = 0.0f;
    setOrientation(glm::quat(eulers));
}

void MyAvatar::update(float deltaTime) {
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
    if (_feetTouchFloor) {
        _skeletonModel.updateStandingFoot();
    }
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
        _firstPersonSkeletonModel.simulate(deltaTime);
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
        _jointData.resize(_skeletonModel.getJointStateCount());
        for (int i = 0; i < _jointData.size(); i++) {
            JointData& data = _jointData[i];
            data.valid = _skeletonModel.getJointState(i, data.rotation);
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

//  Update avatar head rotation with sensor data
void MyAvatar::updateFromTrackers(float deltaTime) {
    glm::vec3 estimatedPosition, estimatedRotation;

    bool inHmd = qApp->isHMDMode();

    if (isPlaying() && inHmd) {
        return;
    }

    if (inHmd) {
        estimatedPosition = qApp->getHeadPosition();
        estimatedPosition.x *= -1.0f;
        _trackedHeadPosition = estimatedPosition;

        const float OCULUS_LEAN_SCALE = 0.05f;
        estimatedPosition /= OCULUS_LEAN_SCALE;
    } else {
        FaceTracker* tracker = Application::getInstance()->getActiveFaceTracker();
        if (tracker && !tracker->isMuted()) {
            estimatedPosition = tracker->getHeadTranslation();
            _trackedHeadPosition = estimatedPosition;
            estimatedRotation = glm::degrees(safeEulerAngles(tracker->getHeadRotation()));
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
    } else {
        float magnifyFieldOfView = qApp->getFieldOfView() /
                                   _realWorldFieldOfView.get();
        head->setDeltaPitch(estimatedRotation.x * magnifyFieldOfView);
        head->setDeltaYaw(estimatedRotation.y * magnifyFieldOfView);
    }
    head->setDeltaRoll(estimatedRotation.z);

    //  Update torso lean distance based on accelerometer data
    const float TORSO_LENGTH = 0.5f;
    glm::vec3 relativePosition = estimatedPosition - glm::vec3(0.0f, -TORSO_LENGTH, 0.0f);
    const float MAX_LEAN = 45.0f;

    // Invert left/right lean when in mirror mode
    // NOTE: this is kinda a hack, it's the same hack we use to make the head tilt. But it's not really a mirror
    // it just makes you feel like you're looking in a mirror because the body movements of the avatar appear to
    // match your body movements.
    if (inHmd && Application::getInstance()->getCamera()->getMode() == CAMERA_MODE_MIRROR) {
        relativePosition.x = -relativePosition.x;
    }

    head->setLeanSideways(glm::clamp(glm::degrees(atanf(relativePosition.x * _leanScale / TORSO_LENGTH)),
        -MAX_LEAN, MAX_LEAN));
    head->setLeanForward(glm::clamp(glm::degrees(atanf(relativePosition.z * _leanScale / TORSO_LENGTH)),
        -MAX_LEAN, MAX_LEAN));
}


// virtual
void MyAvatar::render(RenderArgs* renderArgs, const glm::vec3& cameraPosition, bool postLighting) {
    // don't render if we've been asked to disable local rendering
    if (!_shouldRender) {
        return; // exit early
    }

    Avatar::render(renderArgs, cameraPosition, postLighting);

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

glm::quat MyAvatar::getRightPalmRotation() {
    glm::quat rightRotation;
    getSkeletonModel().getJointRotationInWorldFrame(getSkeletonModel().getRightHandJointIndex(), rightRotation);
    return rightRotation;
}

void MyAvatar::clearReferential() {
    changeReferential(NULL);
}

bool MyAvatar::setModelReferential(const QUuid& id) {
    EntityTree* tree = Application::getInstance()->getEntities()->getTree();
    changeReferential(new ModelReferential(id, tree, this));
    if (_referential->isValid()) {
        return true;
    } else {
        changeReferential(NULL);
        return false;
    }
}

bool MyAvatar::setJointReferential(const QUuid& id, int jointIndex) {
    EntityTree* tree = Application::getInstance()->getEntities()->getTree();
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

AnimationHandlePointer MyAvatar::addAnimationHandle() {
    AnimationHandlePointer handle = _skeletonModel.createAnimationHandle();
    _animationHandles.append(handle);
    return handle;
}

void MyAvatar::removeAnimationHandle(const AnimationHandlePointer& handle) {
    handle->stop();
    _animationHandles.removeOne(handle);
}

void MyAvatar::startAnimation(const QString& url, float fps, float priority,
        bool loop, bool hold, float firstFrame, float lastFrame, const QStringList& maskedJoints) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "startAnimation", Q_ARG(const QString&, url), Q_ARG(float, fps),
            Q_ARG(float, priority), Q_ARG(bool, loop), Q_ARG(bool, hold), Q_ARG(float, firstFrame),
            Q_ARG(float, lastFrame), Q_ARG(const QStringList&, maskedJoints));
        return;
    }
    AnimationHandlePointer handle = _skeletonModel.createAnimationHandle();
    handle->setURL(url);
    handle->setFPS(fps);
    handle->setPriority(priority);
    handle->setLoop(loop);
    handle->setHold(hold);
    handle->setFirstFrame(firstFrame);
    handle->setLastFrame(lastFrame);
    handle->setMaskedJoints(maskedJoints);
    handle->start();
}

void MyAvatar::startAnimationByRole(const QString& role, const QString& url, float fps, float priority,
        bool loop, bool hold, float firstFrame, float lastFrame, const QStringList& maskedJoints) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "startAnimationByRole", Q_ARG(const QString&, role), Q_ARG(const QString&, url),
            Q_ARG(float, fps), Q_ARG(float, priority), Q_ARG(bool, loop), Q_ARG(bool, hold), Q_ARG(float, firstFrame),
            Q_ARG(float, lastFrame), Q_ARG(const QStringList&, maskedJoints));
        return;
    }
    // check for a configured animation for the role
    foreach (const AnimationHandlePointer& handle, _animationHandles) {
        if (handle->getRole() == role) {
            handle->start();
            return;
        }
    }
    // no joy; use the parameters provided
    AnimationHandlePointer handle = _skeletonModel.createAnimationHandle();
    handle->setRole(role);
    handle->setURL(url);
    handle->setFPS(fps);
    handle->setPriority(priority);
    handle->setLoop(loop);
    handle->setHold(hold);
    handle->setFirstFrame(firstFrame);
    handle->setLastFrame(lastFrame);
    handle->setMaskedJoints(maskedJoints);
    handle->start();
}

void MyAvatar::stopAnimationByRole(const QString& role) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "stopAnimationByRole", Q_ARG(const QString&, role));
        return;
    }
    foreach (const AnimationHandlePointer& handle, _skeletonModel.getRunningAnimations()) {
        if (handle->getRole() == role) {
            handle->stop();
        }
    }
}

void MyAvatar::stopAnimation(const QString& url) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "stopAnimation", Q_ARG(const QString&, url));
        return;
    }
    foreach (const AnimationHandlePointer& handle, _skeletonModel.getRunningAnimations()) {
        if (handle->getURL() == url) {
            handle->stop();
        }
    }
}

AnimationDetails MyAvatar::getAnimationDetailsByRole(const QString& role) {
    AnimationDetails result;
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "getAnimationDetailsByRole", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(AnimationDetails, result),
            Q_ARG(const QString&, role));
        return result;
    }
    foreach (const AnimationHandlePointer& handle, _skeletonModel.getRunningAnimations()) {
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
    foreach (const AnimationHandlePointer& handle, _skeletonModel.getRunningAnimations()) {
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

    settings.setValue("useFullAvatar", _useFullAvatar);
    settings.setValue("fullAvatarURL", _fullAvatarURLFromPreferences);
    settings.setValue("faceModelURL", _headURLFromPreferences);
    settings.setValue("skeletonModelURL", _skeletonURLFromPreferences);
    settings.setValue("headModelName", _headModelName);
    settings.setValue("bodyModelName", _bodyModelName);
    settings.setValue("fullAvatarModelName", _fullAvatarModelName);

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
    for (int i = 0; i < _animationHandles.size(); i++) {
        settings.setArrayIndex(i);
        const AnimationHandlePointer& pointer = _animationHandles.at(i);
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

void MyAvatar::loadData() {
    Settings settings;
    settings.beginGroup("Avatar");

    getHead()->setBasePitch(loadSetting(settings, "headPitch", 0.0f));

    getHead()->setPupilDilation(loadSetting(settings, "pupilDilation", 0.0f));

    _leanScale = loadSetting(settings, "leanScale", 0.05f);
    _targetScale = loadSetting(settings, "scale", 1.0f);
    setScale(_scale);

    // The old preferences only stored the face and skeleton URLs, we didn't track if the user wanted to use 1 or 2 urls
    // for their avatar, So we need to attempt to detect this old case and set our new preferences accordingly. If
    // the head URL is empty, then we will assume they are using a full url...
    bool isOldSettings = !(settings.contains("useFullAvatar") || settings.contains("fullAvatarURL"));

    _useFullAvatar = settings.value("useFullAvatar").toBool();
    _headURLFromPreferences = settings.value("faceModelURL", DEFAULT_HEAD_MODEL_URL).toUrl();
    _fullAvatarURLFromPreferences = settings.value("fullAvatarURL", DEFAULT_FULL_AVATAR_MODEL_URL).toUrl();
    _skeletonURLFromPreferences = settings.value("skeletonModelURL", DEFAULT_BODY_MODEL_URL).toUrl();
    _headModelName = settings.value("headModelName", DEFAULT_HEAD_MODEL_NAME).toString();
    _bodyModelName = settings.value("bodyModelName", DEFAULT_BODY_MODEL_NAME).toString();
    _fullAvatarModelName = settings.value("fullAvatarModelName", DEFAULT_FULL_AVATAR_MODEL_NAME).toString();

    if (isOldSettings) {
        bool assumeFullAvatar = _headURLFromPreferences.isEmpty();
        _useFullAvatar = assumeFullAvatar;

        if (_useFullAvatar) {
            _fullAvatarURLFromPreferences = settings.value("skeletonModelURL").toUrl();
            _headURLFromPreferences = DEFAULT_HEAD_MODEL_URL;
            _skeletonURLFromPreferences = DEFAULT_BODY_MODEL_URL;

            QVariantHash fullAvatarFST = FSTReader::downloadMapping(_fullAvatarURLFromPreferences.toString());

            _headModelName = "Default";
            _bodyModelName = "Default";
            _fullAvatarModelName = fullAvatarFST["name"].toString();

        } else {
            _fullAvatarURLFromPreferences = DEFAULT_FULL_AVATAR_MODEL_URL;
            _skeletonURLFromPreferences = settings.value("skeletonModelURL", DEFAULT_BODY_MODEL_URL).toUrl();

            if (_skeletonURLFromPreferences == DEFAULT_BODY_MODEL_URL) {
                _bodyModelName = DEFAULT_BODY_MODEL_NAME;
            } else {
                QVariantHash bodyFST = FSTReader::downloadMapping(_skeletonURLFromPreferences.toString());
                _bodyModelName = bodyFST["name"].toString();
            }

            if (_headURLFromPreferences == DEFAULT_HEAD_MODEL_URL) {
                _headModelName = DEFAULT_HEAD_MODEL_NAME;
            } else {
                QVariantHash headFST = FSTReader::downloadMapping(_headURLFromPreferences.toString());
                _headModelName = headFST["name"].toString();
            }

            _fullAvatarModelName = "Default";
        }
    }

    if (_useFullAvatar) {
        useFullAvatarURL(_fullAvatarURLFromPreferences, _fullAvatarModelName);
    } else {
        useHeadAndBodyURLs(_headURLFromPreferences, _skeletonURLFromPreferences, _headModelName, _bodyModelName);
    }

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
    while (_animationHandles.size() > animationCount) {
        _animationHandles.takeLast()->stop();
    }
    while (_animationHandles.size() < animationCount) {
        addAnimationHandle();
    }
    for (int i = 0; i < animationCount; i++) {
        settings.setArrayIndex(i);
        const AnimationHandlePointer& handle = _animationHandles.at(i);
        handle->setRole(settings.value("role", "idle").toString());
        handle->setURL(settings.value("url").toUrl());
        handle->setFPS(loadSetting(settings, "fps", 30.0f));
        handle->setPriority(loadSetting(settings, "priority", 1.0f));
        handle->setLoop(settings.value("loop", true).toBool());
        handle->setHold(settings.value("hold", false).toBool());
        handle->setFirstFrame(settings.value("firstFrame", 0.0f).toFloat());
        handle->setLastFrame(settings.value("lastFrame", INT_MAX).toFloat());
        handle->setMaskedJoints(settings.value("maskedJoints").toStringList());
        handle->setStartAutomatically(settings.value("startAutomatically", true).toBool());
    }
    settings.endArray();

    setDisplayName(settings.value("displayName").toString());
    setCollisionSoundURL(settings.value("collisionSoundURL", DEFAULT_AVATAR_COLLISION_SOUND_URL).toString());

    settings.endGroup();
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
    //
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
                // Offset their gaze according to whether they're looking at one of my eyes or my mouth.
                glm::vec3 gazeOffset = avatar->getHead()->getLookAtPosition() - getHead()->getEyePosition();
                const float HUMAN_EYE_SEPARATION = 0.065f;
                float myEyeSeparation = glm::length(getHead()->getLeftEyePosition() - getHead()->getRightEyePosition());
                gazeOffset = gazeOffset * HUMAN_EYE_SEPARATION / myEyeSeparation;

                if (Application::getInstance()->isHMDMode()) {
                    //avatar->getHead()->setCorrectedLookAtPosition(Application::getInstance()->getCamera()->getPosition()
                    //    + OculusManager::getMidEyePosition() + gazeOffset);
                    avatar->getHead()->setCorrectedLookAtPosition(Application::getInstance()->getViewFrustum()->getPosition()
                        + OculusManager::getMidEyePosition() + gazeOffset);
                } else {
                    avatar->getHead()->setCorrectedLookAtPosition(Application::getInstance()->getViewFrustum()->getPosition()
                        + gazeOffset);
                }
            } else {
                avatar->getHead()->clearCorrectedLookAtPosition();
            }
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
    return _position + getWorldAlignedOrientation() * _skeletonModel.getDefaultEyeModelPosition();
}

const float SCRIPT_PRIORITY = DEFAULT_PRIORITY + 1.0f;
const float RECORDER_PRIORITY = SCRIPT_PRIORITY + 1.0f;

void MyAvatar::setJointRotations(QVector<glm::quat> jointRotations) {
    int numStates = glm::min(_skeletonModel.getJointStateCount(), jointRotations.size());
    for (int i = 0; i < numStates; ++i) {
        // HACK: ATM only Recorder calls setJointRotations() so we hardcode its priority here
        _skeletonModel.setJointState(i, true, jointRotations[i], RECORDER_PRIORITY);
    }
}

void MyAvatar::setJointData(int index, const glm::quat& rotation) {
    if (QThread::currentThread() == thread()) {
        // HACK: ATM only JS scripts call setJointData() on MyAvatar so we hardcode the priority
        _skeletonModel.setJointState(index, true, rotation, SCRIPT_PRIORITY);
    }
}

void MyAvatar::clearJointData(int index) {
    if (QThread::currentThread() == thread()) {
        // HACK: ATM only JS scripts call clearJointData() on MyAvatar so we hardcode the priority
        _skeletonModel.setJointState(index, false, glm::quat(), 0.0f);
        _skeletonModel.clearJointAnimationPriority(index);
    }
}

void MyAvatar::clearJointsData() {
    clearJointAnimationPriorities();
}

void MyAvatar::clearJointAnimationPriorities() {
    int numStates = _skeletonModel.getJointStateCount();
    for (int i = 0; i < numStates; ++i) {
        _skeletonModel.clearJointAnimationPriority(i);
    }
}

QString MyAvatar::getModelDescription() const {
    QString result;
    if (_useFullAvatar) {
        if (!getFullAvartarModelName().isEmpty()) {
            result = "Full Avatar \"" + getFullAvartarModelName() + "\"";
        } else {
            result = "Full Avatar \"" + _fullAvatarURLFromPreferences.fileName() + "\"";
        }
    } else {
        if (!getHeadModelName().isEmpty()) {
            result = "Head \"" + getHeadModelName() + "\"";
        } else {
            result = "Head \"" + _headURLFromPreferences.fileName() + "\"";
        }
        if (!getBodyModelName().isEmpty()) {
            result += " and Body \"" + getBodyModelName() + "\"";
        } else {
            result += " and Body \"" + _skeletonURLFromPreferences.fileName() + "\"";
        }
    }
    return result;
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

    if (_useFullAvatar) {
        _skeletonModel.setVisibleInScene(_prevShouldDrawHead, scene);

        const QUrl DEFAULT_SKELETON_MODEL_URL = QUrl::fromLocalFile(PathUtils::resourcesPath() + "meshes/defaultAvatar_body.fst");
        _firstPersonSkeletonModel.setURL(_skeletonModelURL, DEFAULT_SKELETON_MODEL_URL, true, !isMyAvatar());
        _firstPersonSkeletonModel.setVisibleInScene(!_prevShouldDrawHead, scene);
    } else {
        _skeletonModel.setVisibleInScene(true, scene);

        _firstPersonSkeletonModel.setVisibleInScene(false, scene);
        _firstPersonSkeletonModel.reset();
    }
}

void MyAvatar::useFullAvatarURL(const QUrl& fullAvatarURL, const QString& modelName) {

    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "useFullAvatarURL", Qt::BlockingQueuedConnection,
                                  Q_ARG(const QUrl&, fullAvatarURL),
                                  Q_ARG(const QString&, modelName));
        return;
    }

    _useFullAvatar = true;

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

    if (fullAvatarURL != getSkeletonModelURL()) {
        setSkeletonModelURL(fullAvatarURL);
        UserActivityLogger::getInstance().changedModel("skeleton", fullAvatarURL.toString());
    }
    sendIdentityPacket();
}

void MyAvatar::useHeadURL(const QUrl& headURL, const QString& modelName) {
    useHeadAndBodyURLs(headURL, _skeletonURLFromPreferences, modelName, _bodyModelName);
}

void MyAvatar::useBodyURL(const QUrl& bodyURL, const QString& modelName) {
    useHeadAndBodyURLs(_headURLFromPreferences, bodyURL, _headModelName, modelName);
}

void MyAvatar::useHeadAndBodyURLs(const QUrl& headURL, const QUrl& bodyURL, const QString& headName, const QString& bodyName) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "useFullAvatarURL", Qt::BlockingQueuedConnection,
                                  Q_ARG(const QUrl&, headURL),
                                  Q_ARG(const QUrl&, bodyURL),
                                  Q_ARG(const QString&, headName),
                                  Q_ARG(const QString&, bodyName));
        return;
    }

    _useFullAvatar = false;

    if (_headURLFromPreferences != headURL) {
        _headURLFromPreferences = headURL;
        if (headName.isEmpty()) {
            QVariantHash headFST = FSTReader::downloadMapping(_headURLFromPreferences.toString());
            _headModelName = headFST["name"].toString();
        } else {
            _headModelName = headName;
        }
    }

    if (_skeletonURLFromPreferences != bodyURL) {
        _skeletonURLFromPreferences = bodyURL;
        if (bodyName.isEmpty()) {
            QVariantHash bodyFST = FSTReader::downloadMapping(_skeletonURLFromPreferences.toString());
            _bodyModelName = bodyFST["name"].toString();
        } else {
            _bodyModelName = bodyName;
        }
    }

    if (headURL != getFaceModelURL()) {
        setFaceModelURL(headURL);
        UserActivityLogger::getInstance().changedModel("head", headURL.toString());
    }

    if (bodyURL != getSkeletonModelURL()) {
        setSkeletonModelURL(bodyURL);
        UserActivityLogger::getInstance().changedModel("skeleton", bodyURL.toString());
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
        glm::vec3 skeletonOffset = _skeletonOffset;
        if (_feetTouchFloor) {
            skeletonOffset += _skeletonModel.getStandingOffset();
        }
        return _position + getOrientation() * FLIP * skeletonOffset;
    }
    return Avatar::getPosition();
}

void MyAvatar::rebuildSkeletonBody() {
    // compute localAABox
    const CapsuleShape& capsule = _skeletonModel.getBoundingShape();
    float radius = capsule.getRadius();
    float height = 2.0f * (capsule.getHalfHeight() + radius);
    glm::vec3 corner(-radius, -0.5f * height, -radius);
    corner += _skeletonModel.getBoundingShapeOffset();
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

void MyAvatar::renderBody(RenderArgs* renderArgs, ViewFrustum* renderFrustum, bool postLighting, float glowLevel) {

    if (!(_skeletonModel.isRenderable() && getHead()->getFaceModel().isRenderable())) {
        return; // wait until all models are loaded
    }

    fixupModelsInScene();

    //  Render head so long as the camera isn't inside it
    if (shouldRenderHead(renderArgs)) {
        getHead()->render(renderArgs, 1.0f, renderFrustum, postLighting);
    }
    getHand()->render(renderArgs, true);
}

void MyAvatar::setVisibleInSceneIfReady(Model* model, render::ScenePointer scene, bool visible) {
    if (model->isActive() && model->isRenderable()) {
        model->setVisibleInScene(visible, scene);
    }
}

void MyAvatar::preRender(RenderArgs* renderArgs) {

    render::ScenePointer scene = Application::getInstance()->getMain3DScene();
    const bool shouldDrawHead = shouldRenderHead(renderArgs);

    _skeletonModel.initWhenReady(scene);
    if (_useFullAvatar) {
        _firstPersonSkeletonModel.initWhenReady(scene);
    }

    if (shouldDrawHead != _prevShouldDrawHead) {
        if (_useFullAvatar) {
            if (shouldDrawHead) {
                _skeletonModel.setVisibleInScene(true, scene);
                _firstPersonSkeletonModel.setVisibleInScene(false, scene);
            } else {
                _skeletonModel.setVisibleInScene(false, scene);
                _firstPersonSkeletonModel.setVisibleInScene(true, scene);
            }
        } else {
            getHead()->getFaceModel().setVisibleInScene(shouldDrawHead, scene);
        }

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
    _bodyYawDelta -= _driveKeys[ROT_RIGHT] * YAW_SPEED * deltaTime;
    _bodyYawDelta += _driveKeys[ROT_LEFT] * YAW_SPEED * deltaTime;
    getHead()->setBasePitch(getHead()->getBasePitch() + (_driveKeys[ROT_UP] - _driveKeys[ROT_DOWN]) * PITCH_SPEED * deltaTime);

    // update body orientation by movement inputs
    setOrientation(getOrientation() *
                   glm::quat(glm::radians(glm::vec3(0.0f, _bodyYawDelta, 0.0f) * deltaTime)));

    // decay body rotation momentum
    const float BODY_SPIN_FRICTION = 7.5f;
    float bodySpinMomentum = 1.0f - BODY_SPIN_FRICTION * deltaTime;
    if (bodySpinMomentum < 0.0f) { bodySpinMomentum = 0.0f; }
    _bodyYawDelta *= bodySpinMomentum;

    float MINIMUM_ROTATION_RATE = 2.0f;
    if (fabs(_bodyYawDelta) < MINIMUM_ROTATION_RATE) { _bodyYawDelta = 0.0f; }

    if (qApp->isHMDMode()) {
        // these angles will be in radians
        glm::quat orientation = qApp->getHeadOrientation();
        // ... so they need to be converted to degrees before we do math...
        glm::vec3 euler = glm::eulerAngles(orientation) * DEGREES_PER_RADIAN;

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
            _motionBehaviors | AVATAR_MOTION_SCRIPTED_MOTOR_ENABLED)) {
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
    if (_billboardValid || !(_skeletonModel.isLoadedWithTextures() && getHead()->getFaceModel().isLoadedWithTextures())) {
        return;
    }
    foreach (Model* model, _attachmentModels) {
        if (!model->isLoadedWithTextures()) {
            return;
        }
    }
    gpu::Context context(new gpu::GLBackend());
    RenderArgs renderArgs(&context);
    QImage image = qApp->renderAvatarBillboard(&renderArgs);
    _billboard.clear();
    QBuffer buffer(&_billboard);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
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

    glm::vec3 shiftedPosition = newPosition;

    if (hasOrientation) {
        qCDebug(interfaceapp).nospace() << "MyAvatar goToLocation - new orientation is "
            << newOrientation.x << ", " << newOrientation.y << ", " << newOrientation.z << ", " << newOrientation.w;

        // orient the user to face the target
        glm::quat quatOrientation = newOrientation;

        if (shouldFaceLocation) {

            quatOrientation = newOrientation * glm::angleAxis(PI, glm::vec3(0.0f, 1.0f, 0.0f));

            // move the user a couple units away
            const float DISTANCE_TO_USER = 2.0f;
            shiftedPosition = newPosition - quatOrientation * IDENTITY_FRONT * DISTANCE_TO_USER;
        }

        setOrientation(quatOrientation);
    }

    slamPosition(shiftedPosition);
    emit transformChanged();
}

void MyAvatar::updateMotionBehavior() {
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
    _feetTouchFloor = menu->isOptionChecked(MenuOption::ShiftHipsForIdleAnimations);
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
