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

#include <scripting/HMDScriptingInterface.h>
#include <AccountManager.h>
#include <AddressManager.h>
#include <AudioClient.h>
#include <DependencyManager.h>
#include <display-plugins/DisplayPlugin.h>
#include <FSTReader.h>
#include <GeometryUtil.h>
#include <NodeList.h>
#include <udt/PacketHeaders.h>
#include <PathUtils.h>
#include <PerfStat.h>
#include <SharedUtil.h>
#include <TextRenderer3D.h>
#include <UserActivityLogger.h>
#include <AnimDebugDraw.h>
#include <AnimClip.h>
#include <recording/Deck.h>
#include <recording/Recorder.h>
#include <recording/Clip.h>
#include <recording/Frame.h>
#include <RecordingScriptingInterface.h>

#include "Application.h"
#include "devices/Faceshift.h"
#include "AvatarManager.h"
#include "Menu.h"
#include "MyAvatar.h"
#include "Physics.h"
#include "Util.h"
#include "InterfaceLogging.h"
#include "DebugDraw.h"
#include "EntityEditPacketSender.h"
#include "MovingEntitiesOperator.h"

using namespace std;

const glm::vec3 DEFAULT_UP_DIRECTION(0.0f, 1.0f, 0.0f);
const float DEFAULT_REAL_WORLD_FIELD_OF_VIEW_DEGREES = 30.0f;

const float MAX_WALKING_SPEED = 2.5f; // human walking speed
const float MAX_BOOST_SPEED = 0.5f * MAX_WALKING_SPEED; // keyboard motor gets additive boost below this speed
const float MIN_AVATAR_SPEED = 0.05f; // speed is set to zero below this

const float YAW_SPEED_DEFAULT = 120.0f;   // degrees/sec
const float PITCH_SPEED_DEFAULT = 90.0f; // degrees/sec

// TODO: normalize avatar speed for standard avatar size, then scale all motion logic
// to properly follow avatar size.
float MAX_AVATAR_SPEED = 30.0f;
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
    _wasPushing(false),
    _isPushing(false),
    _isBraking(false),
    _boomLength(ZOOM_DEFAULT),
    _yawSpeed(YAW_SPEED_DEFAULT),
    _pitchSpeed(PITCH_SPEED_DEFAULT),
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
    _audioListenerMode(FROM_HEAD),
    _hmdAtRestDetector(glm::vec3(0), glm::quat())
{
    using namespace recording;

    for (int i = 0; i < MAX_DRIVE_KEYS; i++) {
        _driveKeys[i] = 0.0f;
    }

    // connect to AddressManager signal for location jumps
    connect(DependencyManager::get<AddressManager>().data(), &AddressManager::locationChangeRequired, 
        [=](const glm::vec3& newPosition, bool hasOrientation, const glm::quat& newOrientation, bool shouldFaceLocation){
        goToLocation(newPosition, hasOrientation, newOrientation, shouldFaceLocation);
    });

    _characterController.setEnabled(true);

    _bodySensorMatrix = deriveBodyFromHMDSensor();

    using namespace recording;

    auto player = DependencyManager::get<Deck>();
    auto recorder = DependencyManager::get<Recorder>();
    connect(player.data(), &Deck::playbackStateChanged, [=] {
        if (player->isPlaying()) {
            auto recordingInterface = DependencyManager::get<RecordingScriptingInterface>();
            if (recordingInterface->getPlayFromCurrentLocation()) {
                setRecordingBasis();
            }
        } else {
            clearRecordingBasis();
        }
    });

    connect(recorder.data(), &Recorder::recordingStateChanged, [=] {
        if (recorder->isRecording()) {
            setRecordingBasis();
        } else {
            clearRecordingBasis();
        }
    });

    static const recording::FrameType AVATAR_FRAME_TYPE = recording::Frame::registerFrameType(AvatarData::FRAME_NAME);
    Frame::registerFrameHandler(AVATAR_FRAME_TYPE, [=](Frame::ConstPointer frame) {
        static AvatarData dummyAvatar;
        AvatarData::fromFrame(frame->data, dummyAvatar);
        if (getRecordingBasis()) {
            dummyAvatar.setRecordingBasis(getRecordingBasis());
        } else {
            dummyAvatar.clearRecordingBasis();
        }

        auto recordingInterface = DependencyManager::get<RecordingScriptingInterface>();
        if (recordingInterface->getPlayerUseHeadModel() && dummyAvatar.getFaceModelURL().isValid() &&
            (dummyAvatar.getFaceModelURL() != getFaceModelURL())) {
            // FIXME
            //myAvatar->setFaceModelURL(_dummyAvatar.getFaceModelURL());
        }

        if (recordingInterface->getPlayerUseSkeletonModel() && dummyAvatar.getSkeletonModelURL().isValid() &&
            (dummyAvatar.getSkeletonModelURL() != getSkeletonModelURL())) {
            // FIXME
            //myAvatar->useFullAvatarURL()
        }

        if (recordingInterface->getPlayerUseDisplayName() && dummyAvatar.getDisplayName() != getDisplayName()) {
            setDisplayName(dummyAvatar.getDisplayName());
        }

        setPosition(dummyAvatar.getPosition());
        setOrientation(dummyAvatar.getOrientation());

        if (!dummyAvatar.getAttachmentData().isEmpty()) {
            setAttachmentData(dummyAvatar.getAttachmentData());
        }

        auto headData = dummyAvatar.getHeadData();
        if (headData && _headData) {
            // blendshapes
            if (!headData->getBlendshapeCoefficients().isEmpty()) {
                _headData->setBlendshapeCoefficients(headData->getBlendshapeCoefficients());
            }
            // head lean
            _headData->setLeanForward(headData->getLeanForward());
            _headData->setLeanSideways(headData->getLeanSideways());
            // head orientation
            _headData->setLookAtPosition(headData->getLookAtPosition());
        }
    });
}

MyAvatar::~MyAvatar() {
    _lookAtTargetAvatar.reset();
}

// virtual
void MyAvatar::simulateAttachments(float deltaTime) {
    // don't update attachments here, do it in harvestResultsFromPhysicsSimulation()
}

QByteArray MyAvatar::toByteArray(bool cullSmallChanges, bool sendAll) {
    CameraMode mode = qApp->getCamera()->getMode();
    _globalPosition = getPosition();
    if (mode == CAMERA_MODE_THIRD_PERSON || mode == CAMERA_MODE_INDEPENDENT) {
        // fake the avatar position that is sent up to the AvatarMixer
        glm::vec3 oldPosition = getPosition();
        setPosition(getSkeletonPosition());
        QByteArray array = AvatarData::toByteArray(cullSmallChanges, sendAll);
        // copy the correct position back
        setPosition(oldPosition);
        return array;
    }
    return AvatarData::toByteArray(cullSmallChanges, sendAll);
}

void MyAvatar::reset(bool andReload) {
    if (andReload) {
        qApp->setRawAvatarUpdateThreading(false);
    }

    // Reset dynamic state.
    _wasPushing = _isPushing = _isBraking = false;
    _follow.deactivate();
    _skeletonModel.reset();
    getHead()->reset();
    _targetVelocity = glm::vec3(0.0f);
    setThrust(glm::vec3(0.0f));

    if (andReload) {
        // derive the desired body orientation from the *old* hmd orientation, before the sensor reset.
        auto newBodySensorMatrix = deriveBodyFromHMDSensor(); // Based on current cached HMD position/rotation..

        // transform this body into world space
        auto worldBodyMatrix = _sensorToWorldMatrix * newBodySensorMatrix;
        auto worldBodyPos = extractTranslation(worldBodyMatrix);
        auto worldBodyRot = glm::normalize(glm::quat_cast(worldBodyMatrix));

        // this will become our new position.
        setPosition(worldBodyPos);
        setOrientation(worldBodyRot);

        // now sample the new hmd orientation AFTER sensor reset.
        updateFromHMDSensorMatrix(qApp->getHMDSensorPose());

        // update the body in sensor space using the new hmd sensor sample
        _bodySensorMatrix = deriveBodyFromHMDSensor();

        // rebuild the sensor to world matrix such that, the HMD will point in the desired orientation.
        // i.e. the along avatar's current position and orientation.
        updateSensorToWorldMatrix();
    }
}

void MyAvatar::update(float deltaTime) {

    // update moving average of HMD facing in xz plane.
    const float HMD_FACING_TIMESCALE = 4.0f; // very slow average
    float tau = deltaTime / HMD_FACING_TIMESCALE;
    _hmdSensorFacingMovingAverage = lerp(_hmdSensorFacingMovingAverage, _hmdSensorFacing, tau);

#ifdef DEBUG_DRAW_HMD_MOVING_AVERAGE
    glm::vec3 p = transformPoint(getSensorToWorldMatrix(), _hmdSensorPosition + glm::vec3(_hmdSensorFacingMovingAverage.x, 0.0f, _hmdSensorFacingMovingAverage.y));
    DebugDraw::getInstance().addMarker("facing-avg", getOrientation(), p, glm::vec4(1.0f));
    p = transformPoint(getSensorToWorldMatrix(), _hmdSensorPosition + glm::vec3(_hmdSensorFacing.x, 0.0f, _hmdSensorFacing.y));
    DebugDraw::getInstance().addMarker("facing", getOrientation(), p, glm::vec4(1.0f));
#endif

    if (_goToPending) {
        setPosition(_goToPosition);
        setOrientation(_goToOrientation);
        _hmdSensorFacingMovingAverage = _hmdSensorFacing;  // reset moving average
        _goToPending = false;
        // updateFromHMDSensorMatrix (called from paintGL) expects that the sensorToWorldMatrix is updated for any position changes
        // that happen between render and Application::update (which calls updateSensorToWorldMatrix to do so).
        // However, render/MyAvatar::update/Application::update don't always match (e.g., when using the separate avatar update thread),
        // so we update now. It's ok if it updates again in the normal way.
        updateSensorToWorldMatrix();
        emit positionGoneTo();
    }

    Head* head = getHead();
    head->relaxLean(deltaTime);
    updateFromTrackers(deltaTime);

    //  Get audio loudness data from audio input device
    auto audio = DependencyManager::get<AudioClient>();
    head->setAudioLoudness(audio->getLastInputLoudness());
    head->setAudioAverageLoudness(audio->getAudioAverageInputLoudness());
    
     simulate(deltaTime);
    
    currentEnergy += energyChargeRate;
    currentEnergy -= getAccelerationEnergy();
    currentEnergy -= getAudioEnergy();
    
    if(didTeleport()) {
        currentEnergy = 0.0f;
    }
    currentEnergy = max(0.0f, min(currentEnergy,1.0f));
    emit energyChanged(currentEnergy);
     
   
}

extern QByteArray avatarStateToFrame(const AvatarData* _avatar);
extern void avatarStateFromFrame(const QByteArray& frameData, AvatarData* _avatar);

void MyAvatar::simulate(float deltaTime) {
    PerformanceTimer perfTimer("simulate");

    animateScaleChanges(deltaTime);

    {
        PerformanceTimer perfTimer("transform");
        bool stepAction = false;
        // When there are no step values, we zero out the last step pulse.
        // This allows a user to do faster snapping by tapping a control
        for (int i = STEP_TRANSLATE_X; !stepAction && i <= STEP_YAW; ++i) {
            if (_driveKeys[i] != 0.0f) {
                stepAction = true;
            }
        }

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
        PerformanceTimer perfTimer("joints");
        // copy out the skeleton joints from the model
        _rig->copyJointsIntoJointData(_jointData);
    }

    {
        PerformanceTimer perfTimer("head");
        Head* head = getHead();
        glm::vec3 headPosition;
        if (!_skeletonModel.getHeadPosition(headPosition)) {
            headPosition = getPosition();
        }
        head->setPosition(headPosition);
        head->setScale(getUniformScale());
        head->simulate(deltaTime, true);
    }

    // Record avatars movements.
    auto recorder = DependencyManager::get<recording::Recorder>();
    if (recorder->isRecording()) {
        static const recording::FrameType FRAME_TYPE = recording::Frame::registerFrameType(AvatarData::FRAME_NAME);
        recorder->recordFrame(FRAME_TYPE, toFrame(*this));
    }

    locationChanged();
    // if a entity-child of this avatar has moved outside of its queryAACube, update the cube and tell the entity server.
    EntityTreeRenderer* entityTreeRenderer = qApp->getEntities();
    EntityTreePointer entityTree = entityTreeRenderer ? entityTreeRenderer->getTree() : nullptr;
    if (entityTree) {
        auto now = usecTimestampNow();
        EntityEditPacketSender* packetSender = qApp->getEntityEditPacketSender();
        MovingEntitiesOperator moveOperator(entityTree);
        forEachDescendant([&](SpatiallyNestablePointer object) {
            // if the queryBox has changed, tell the entity-server
            if (object->computePuffedQueryAACube() && object->getNestableType() == NestableType::Entity) {
                EntityItemPointer entity = std::static_pointer_cast<EntityItem>(object);
                bool success;
                AACube newCube = entity->getQueryAACube(success);
                if (success) {
                    moveOperator.addEntityToMoveList(entity, newCube);
                }
                if (packetSender) {
                    EntityItemProperties properties = entity->getProperties();
                    properties.setQueryAACubeDirty();
                    properties.setLastEdited(now);
                    packetSender->queueEditEntityMessage(PacketType::EntityEdit, entity->getID(), properties);
                    entity->setLastBroadcast(usecTimestampNow());
                }
            }
        });
        // also update the position of children in our local octree
        if (moveOperator.hasMovingEntities()) {
            PerformanceTimer perfTimer("recurseTreeWithOperator");
            entityTree->recurseTreeWithOperator(&moveOperator);
        }
    }
}

// thread-safe
glm::mat4 MyAvatar::getSensorToWorldMatrix() const {
    return _sensorToWorldMatrixCache.get();
}

// Pass a recent sample of the HMD to the avatar.
// This can also update the avatar's position to follow the HMD
// as it moves through the world.
void MyAvatar::updateFromHMDSensorMatrix(const glm::mat4& hmdSensorMatrix) {
    // update the sensorMatrices based on the new hmd pose
    _hmdSensorMatrix = hmdSensorMatrix;
    _hmdSensorPosition = extractTranslation(hmdSensorMatrix);
    _hmdSensorOrientation = glm::quat_cast(hmdSensorMatrix);
    _hmdSensorFacing = getFacingDir2D(_hmdSensorOrientation);
}

// best called at end of main loop, just before rendering.
// update sensor to world matrix from current body position and hmd sensor.
// This is so the correct camera can be used for rendering.
void MyAvatar::updateSensorToWorldMatrix() {

    // update the sensor mat so that the body position will end up in the desired
    // position when driven from the head.
    glm::mat4 desiredMat = createMatFromQuatAndPos(getOrientation(), getPosition());
    _sensorToWorldMatrix = desiredMat * glm::inverse(_bodySensorMatrix);

    lateUpdatePalms();

    _sensorToWorldMatrixCache.set(_sensorToWorldMatrix);
}

//  Update avatar head rotation with sensor data
void MyAvatar::updateFromTrackers(float deltaTime) {
    glm::vec3 estimatedPosition, estimatedRotation;

    bool inHmd = qApp->getAvatarUpdater()->isHMDMode();
    bool playing = DependencyManager::get<recording::Deck>()->isPlaying();
    if (inHmd && playing) {
        return;
    }

    FaceTracker* tracker = qApp->getActiveFaceTracker();
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
    if (inHmd || playing) {
        head->setDeltaPitch(estimatedRotation.x);
        head->setDeltaYaw(estimatedRotation.y);
        head->setDeltaRoll(estimatedRotation.z);
    } else {
        float magnifyFieldOfView = qApp->getViewFrustum()->getFieldOfView() / _realWorldFieldOfView.get();
        head->setDeltaPitch(estimatedRotation.x * magnifyFieldOfView);
        head->setDeltaYaw(estimatedRotation.y * magnifyFieldOfView);
        head->setDeltaRoll(estimatedRotation.z);
    }

    //  Update torso lean distance based on accelerometer data
    const float TORSO_LENGTH = 0.5f;
    glm::vec3 relativePosition = estimatedPosition - glm::vec3(0.0f, -TORSO_LENGTH, 0.0f);

    const float MAX_LEAN = 45.0f;
    head->setLeanSideways(glm::clamp(glm::degrees(atanf(relativePosition.x * _leanScale / TORSO_LENGTH)),
                                     -MAX_LEAN, MAX_LEAN));
    head->setLeanForward(glm::clamp(glm::degrees(atanf(relativePosition.z * _leanScale / TORSO_LENGTH)),
                                    -MAX_LEAN, MAX_LEAN));
}


glm::vec3 MyAvatar::getLeftHandPosition() const {
    auto palmData = getHandData()->getCopyOfPalmData(HandData::LeftHand);
    return palmData.isValid() ? palmData.getPosition() : glm::vec3(0.0f);
}

glm::vec3 MyAvatar::getRightHandPosition() const {
    auto palmData = getHandData()->getCopyOfPalmData(HandData::RightHand);
    return palmData.isValid() ? palmData.getPosition() : glm::vec3(0.0f);
}

glm::vec3 MyAvatar::getLeftHandTipPosition() const {
    auto palmData = getHandData()->getCopyOfPalmData(HandData::LeftHand);
    return palmData.isValid() ? palmData.getTipPosition() : glm::vec3(0.0f);
}

glm::vec3 MyAvatar::getRightHandTipPosition() const {
    auto palmData = getHandData()->getCopyOfPalmData(HandData::RightHand);
    return palmData.isValid() ? palmData.getTipPosition() : glm::vec3(0.0f);
}

controller::Pose MyAvatar::getLeftHandPose() const {
    auto palmData = getHandData()->getCopyOfPalmData(HandData::LeftHand);
    return palmData.isValid() ? controller::Pose(palmData.getPosition(), palmData.getRotation(),
        palmData.getVelocity(), palmData.getRawAngularVelocity()) : controller::Pose();
}

controller::Pose MyAvatar::getRightHandPose() const {
    auto palmData = getHandData()->getCopyOfPalmData(HandData::RightHand);
    return palmData.isValid() ? controller::Pose(palmData.getPosition(), palmData.getRotation(),
        palmData.getVelocity(), palmData.getRawAngularVelocity()) : controller::Pose();
}

controller::Pose MyAvatar::getLeftHandTipPose() const {
    auto palmData = getHandData()->getCopyOfPalmData(HandData::LeftHand);
    return palmData.isValid() ? controller::Pose(palmData.getTipPosition(), palmData.getRotation(),
        palmData.getTipVelocity(), palmData.getRawAngularVelocity()) : controller::Pose();
}

controller::Pose MyAvatar::getRightHandTipPose() const {
    auto palmData = getHandData()->getCopyOfPalmData(HandData::RightHand);
    return palmData.isValid() ? controller::Pose(palmData.getTipPosition(), palmData.getRotation(),
        palmData.getTipVelocity(), palmData.getRawAngularVelocity()) : controller::Pose();
}

// virtual
void MyAvatar::render(RenderArgs* renderArgs, const glm::vec3& cameraPosition) {
    // don't render if we've been asked to disable local rendering
    if (!_shouldRender) {
        return; // exit early
    }

    Avatar::render(renderArgs, cameraPosition);
}

void MyAvatar::overrideAnimation(const QString& url, float fps, bool loop, float firstFrame, float lastFrame) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "overrideAnimation", Q_ARG(const QString&, url), Q_ARG(float, fps),
                                  Q_ARG(bool, loop), Q_ARG(float, firstFrame), Q_ARG(float, lastFrame));
        return;
    }
    _rig->overrideAnimation(url, fps, loop, firstFrame, lastFrame);
}

void MyAvatar::restoreAnimation() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "restoreAnimation");
        return;
    }
    _rig->restoreAnimation();
}

QStringList MyAvatar::getAnimationRoles() {
    if (QThread::currentThread() != thread()) {
        QStringList result;
        QMetaObject::invokeMethod(this, "getAnimationRoles", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QStringList, result));
        return result;
    }
    return _rig->getAnimationRoles();
}

void MyAvatar::overrideRoleAnimation(const QString& role, const QString& url, float fps, bool loop,
                                     float firstFrame, float lastFrame) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "overrideRoleAnimation", Q_ARG(const QString&, role), Q_ARG(const QString&, url),
                                  Q_ARG(float, fps), Q_ARG(bool, loop), Q_ARG(float, firstFrame), Q_ARG(float, lastFrame));
        return;
    }
    _rig->overrideRoleAnimation(role, url, fps, loop, firstFrame, lastFrame);
}

void MyAvatar::restoreRoleAnimation(const QString& role) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "restoreRoleAnimation", Q_ARG(const QString&, role));
        return;
    }
    _rig->restoreRoleAnimation(role);
}

void MyAvatar::prefetchAnimation(const QString& url) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "prefetchAnimation", Q_ARG(const QString&, url));
        return;
    }
    _rig->prefetchAnimation(url);
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
        settings.setValue("isSoft", attachment.isSoft);
    }
    settings.endArray();

    settings.setValue("displayName", _displayName);
    settings.setValue("collisionSoundURL", _collisionSoundURL);
    settings.setValue("useSnapTurn", _useSnapTurn);

    settings.endGroup();
}

float loadSetting(QSettings& settings, const char* name, float defaultValue) {
    float value = settings.value(name, defaultValue).toFloat();
    if (glm::isnan(value)) {
        value = defaultValue;
    }
    return value;
}

void MyAvatar::setEnableDebugDrawDefaultPose(bool isEnabled) {
    _enableDebugDrawDefaultPose = isEnabled;

    if (!isEnabled) {
        AnimDebugDraw::getInstance().removeAbsolutePoses("myAvatarDefaultPoses");
    }
}

void MyAvatar::setEnableDebugDrawAnimPose(bool isEnabled) {
    _enableDebugDrawAnimPose = isEnabled;

    if (!isEnabled) {
        AnimDebugDraw::getInstance().removeAbsolutePoses("myAvatarAnimPoses");
    }
}

void MyAvatar::setEnableDebugDrawPosition(bool isEnabled) {
    if (isEnabled) {
        const glm::vec4 red(1.0f, 0.0f, 0.0f, 1.0f);
        DebugDraw::getInstance().addMyAvatarMarker("avatarPosition", glm::quat(), glm::vec3(), red);
    } else {
        DebugDraw::getInstance().removeMyAvatarMarker("avatarPosition");
    }
}

void MyAvatar::setEnableMeshVisible(bool isEnabled) {
    render::ScenePointer scene = qApp->getMain3DScene();
    _skeletonModel.setVisibleInScene(isEnabled, scene);
}

void MyAvatar::setUseAnimPreAndPostRotations(bool isEnabled) {
    AnimClip::usePreAndPostPoseFromAnim = isEnabled;
    reset(true);
}

void MyAvatar::setEnableInverseKinematics(bool isEnabled) {
    _rig->setEnableInverseKinematics(isEnabled);
}

void MyAvatar::loadData() {
    Settings settings;
    settings.beginGroup("Avatar");

    getHead()->setBasePitch(loadSetting(settings, "headPitch", 0.0f));

    getHead()->setPupilDilation(loadSetting(settings, "pupilDilation", 0.0f));

    _leanScale = loadSetting(settings, "leanScale", 0.05f);
    _targetScale = loadSetting(settings, "scale", 1.0f);
    setScale(glm::vec3(_targetScale));

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
        attachment.isSoft = settings.value("isSoft").toBool();
        attachmentData.append(attachment);
    }
    settings.endArray();
    setAttachmentData(attachmentData);

    setDisplayName(settings.value("displayName").toString());
    setCollisionSoundURL(settings.value("collisionSoundURL", DEFAULT_AVATAR_COLLISION_SOUND_URL).toString());
    setSnapTurn(settings.value("useSnapTurn", _useSnapTurn).toBool());

    settings.endGroup();

    setEnableMeshVisible(Menu::getInstance()->isOptionChecked(MenuOption::MeshVisible));
    setEnableDebugDrawDefaultPose(Menu::getInstance()->isOptionChecked(MenuOption::AnimDebugDrawDefaultPose));
    setEnableDebugDrawAnimPose(Menu::getInstance()->isOptionChecked(MenuOption::AnimDebugDrawAnimPose));
    setEnableDebugDrawPosition(Menu::getInstance()->isOptionChecked(MenuOption::AnimDebugDrawPosition));
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

void MyAvatar::updateLookAtTargetAvatar() {
    //
    //  Look at the avatar whose eyes are closest to the ray in direction of my avatar's head
    //  And set the correctedLookAt for all (nearby) avatars that are looking at me.
    _lookAtTargetAvatar.reset();
    _targetAvatarPosition = glm::vec3(0.0f);

    glm::vec3 lookForward = getHead()->getFinalOrientationInWorldFrame() * IDENTITY_FRONT;
    glm::vec3 cameraPosition = qApp->getCamera()->getPosition();

    float smallestAngleTo = glm::radians(DEFAULT_FIELD_OF_VIEW_DEGREES) / 2.0f;
    const float KEEP_LOOKING_AT_CURRENT_ANGLE_FACTOR = 1.3f;
    const float GREATEST_LOOKING_AT_DISTANCE = 10.0f;

    AvatarHash hash = DependencyManager::get<AvatarManager>()->getHashCopy();

    foreach (const AvatarSharedPointer& avatarPointer, hash) {
        auto avatar = static_pointer_cast<Avatar>(avatarPointer);
        bool isCurrentTarget = avatar->getIsLookAtTarget();
        float distanceTo = glm::length(avatar->getHead()->getEyePosition() - cameraPosition);
        avatar->setIsLookAtTarget(false);
        if (!avatar->isMyAvatar() && avatar->isInitialized() &&
            (distanceTo < GREATEST_LOOKING_AT_DISTANCE * getUniformScale())) {
            float angleTo = glm::angle(lookForward, glm::normalize(avatar->getHead()->getEyePosition() - getHead()->getEyePosition()));
            if (angleTo < (smallestAngleTo * (isCurrentTarget ? KEEP_LOOKING_AT_CURRENT_ANGLE_FACTOR : 1.0f))) {
                _lookAtTargetAvatar = avatarPointer;
                _targetAvatarPosition = avatarPointer->getPosition();
                smallestAngleTo = angleTo;
            }
            if (isLookingAtMe(avatar)) {

                // Alter their gaze to look directly at my camera; this looks more natural than looking at my avatar's face.
                glm::vec3 lookAtPosition = avatar->getHead()->getLookAtPosition(); // A position, in world space, on my avatar.

                // The camera isn't at the point midway between the avatar eyes. (Even without an HMD, the head can be offset a bit.)
                // Let's get everything to world space:
                glm::vec3 avatarLeftEye = getHead()->getLeftEyePosition();
                glm::vec3 avatarRightEye = getHead()->getRightEyePosition();

                // First find out where (in world space) the person is looking relative to that bridge-of-the-avatar point.
                // (We will be adding that offset to the camera position, after making some other adjustments.)
                glm::vec3 gazeOffset = lookAtPosition - getHead()->getEyePosition();

                // scale gazeOffset by IPD, if wearing an HMD.
                if (qApp->isHMDMode()) {
                    glm::mat4 leftEye = qApp->getEyeOffset(Eye::Left);
                    glm::mat4 rightEye = qApp->getEyeOffset(Eye::Right);
                    glm::vec3 leftEyeHeadLocal = glm::vec3(leftEye[3]);
                    glm::vec3 rightEyeHeadLocal = glm::vec3(rightEye[3]);
                    auto humanSystem = qApp->getViewFrustum();
                    glm::vec3 humanLeftEye = humanSystem->getPosition() + (humanSystem->getOrientation() * leftEyeHeadLocal);
                    glm::vec3 humanRightEye = humanSystem->getPosition() + (humanSystem->getOrientation() * rightEyeHeadLocal);

                    auto hmdInterface = DependencyManager::get<HMDScriptingInterface>();
                    float ipdScale = hmdInterface->getIPDScale();

                    // Scale by proportional differences between avatar and human.
                    float humanEyeSeparationInModelSpace = glm::length(humanLeftEye - humanRightEye) * ipdScale;
                    float avatarEyeSeparation = glm::length(avatarLeftEye - avatarRightEye);
                    gazeOffset = gazeOffset * humanEyeSeparationInModelSpace / avatarEyeSeparation;
                }

                // And now we can finally add that offset to the camera.
                glm::vec3 corrected = qApp->getViewFrustum()->getPosition() + gazeOffset;

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
    return getPosition() + getWorldAlignedOrientation() * Quaternions::Y_180 * _skeletonModel.getDefaultEyeModelPosition();
}

const float SCRIPT_PRIORITY = 1.0f + 1.0f;
const float RECORDER_PRIORITY = 1.0f + 1.0f;

void MyAvatar::setJointRotations(QVector<glm::quat> jointRotations) {
    int numStates = glm::min(_skeletonModel.getJointStateCount(), jointRotations.size());
    for (int i = 0; i < numStates; ++i) {
        // HACK: ATM only Recorder calls setJointRotations() so we hardcode its priority here
        _skeletonModel.setJointRotation(i, true, jointRotations[i], RECORDER_PRIORITY);
    }
}

void MyAvatar::setJointData(int index, const glm::quat& rotation, const glm::vec3& translation) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setJointData", Q_ARG(int, index), Q_ARG(const glm::quat&, rotation),
            Q_ARG(const glm::vec3&, translation));
        return;
    }
    // HACK: ATM only JS scripts call setJointData() on MyAvatar so we hardcode the priority
    _rig->setJointState(index, true, rotation, translation, SCRIPT_PRIORITY);
}

void MyAvatar::setJointRotation(int index, const glm::quat& rotation) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setJointRotation", Q_ARG(int, index), Q_ARG(const glm::quat&, rotation));
        return;
    }
    // HACK: ATM only JS scripts call setJointData() on MyAvatar so we hardcode the priority
    _rig->setJointRotation(index, true, rotation, SCRIPT_PRIORITY);
}

void MyAvatar::setJointTranslation(int index, const glm::vec3& translation) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setJointTranslation", Q_ARG(int, index), Q_ARG(const glm::vec3&, translation));
        return;
    }
    // HACK: ATM only JS scripts call setJointData() on MyAvatar so we hardcode the priority
    _rig->setJointTranslation(index, true, translation, SCRIPT_PRIORITY);
}

void MyAvatar::clearJointData(int index) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "clearJointData", Q_ARG(int, index));
        return;
    }
    _rig->clearJointAnimationPriority(index);
}

void MyAvatar::clearJointsData() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "clearJointsData");
        return;
    }
    _rig->clearJointStates();
}

void MyAvatar::setFaceModelURL(const QUrl& faceModelURL) {

    Avatar::setFaceModelURL(faceModelURL);
    render::ScenePointer scene = qApp->getMain3DScene();
    getHead()->getFaceModel().setVisibleInScene(_prevShouldDrawHead, scene);
}

void MyAvatar::setSkeletonModelURL(const QUrl& skeletonModelURL) {

    Avatar::setSkeletonModelURL(skeletonModelURL);
    render::ScenePointer scene = qApp->getMain3DScene();
    _skeletonModel.setVisibleInScene(true, scene);
    _headBoneSet.clear();
}


void MyAvatar::resetFullAvatarURL() {
    auto lastAvatarURL = getFullAvatarURLFromPreferences();
    auto lastAvatarName = getFullAvatarModelName();
    useFullAvatarURL(QUrl());
    useFullAvatarURL(lastAvatarURL, lastAvatarName);
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
        qApp->setRawAvatarUpdateThreading(false);
        setSkeletonModelURL(fullAvatarURL);
        qApp->setRawAvatarUpdateThreading();
        UserActivityLogger::getInstance().changedModel("skeleton", urlString);
    }
    sendIdentityPacket();
}

void MyAvatar::setAttachmentData(const QVector<AttachmentData>& attachmentData) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setAttachmentData", Qt::BlockingQueuedConnection,
                                  Q_ARG(const QVector<AttachmentData>, attachmentData));
        return;
    }
    Avatar::setAttachmentData(attachmentData);
}

glm::vec3 MyAvatar::getSkeletonPosition() const {
    CameraMode mode = qApp->getCamera()->getMode();
    if (mode == CAMERA_MODE_THIRD_PERSON || mode == CAMERA_MODE_INDEPENDENT) {
        // The avatar is rotated PI about the yAxis, so we have to correct for it
        // to get the skeleton offset contribution in the world-frame.
        const glm::quat FLIP = glm::angleAxis(PI, glm::vec3(0.0f, 1.0f, 0.0f));
        return getPosition() + getOrientation() * FLIP * _skeletonOffset;
    }
    return Avatar::getPosition();
}

void MyAvatar::rebuildCollisionShape() {
    // compute localAABox
    float scale = getUniformScale();
    float radius = scale * _skeletonModel.getBoundingCapsuleRadius();
    float height = scale * _skeletonModel.getBoundingCapsuleHeight() + 2.0f * radius;
    glm::vec3 corner(-radius, -0.5f * height, -radius);
    corner += scale * _skeletonModel.getBoundingCapsuleOffset();
    glm::vec3 diagonal(2.0f * radius, height, 2.0f * radius);
    _characterController.setLocalBoundingBox(corner, diagonal);
}

void MyAvatar::prepareForPhysicsSimulation() {
    relayDriveKeysToCharacterController();

    bool success;
    glm::vec3 parentVelocity = getParentVelocity(success);
    if (!success) {
        qDebug() << "Warning: getParentVelocity failed" << getID();
        parentVelocity = glm::vec3();
    }
    _characterController.setParentVelocity(parentVelocity);

    _characterController.setTargetVelocity(getTargetVelocity());
    _characterController.setPositionAndOrientation(getPosition(), getOrientation());
    if (qApp->isHMDMode()) {
        bool hasDriveInput = fabsf(_driveKeys[TRANSLATE_X]) > 0.0f || fabsf(_driveKeys[TRANSLATE_Z]) > 0.0f;
        _follow.prePhysicsUpdate(*this, deriveBodyFromHMDSensor(), _bodySensorMatrix, hasDriveInput);
    } else {
        _follow.deactivate();
    }
}

void MyAvatar::harvestResultsFromPhysicsSimulation(float deltaTime) {
    glm::vec3 position = getPosition();
    glm::quat orientation = getOrientation();
    _characterController.getPositionAndOrientation(position, orientation);
    nextAttitude(position, orientation);
    _bodySensorMatrix = _follow.postPhysicsUpdate(*this, _bodySensorMatrix);

    setVelocity(_characterController.getLinearVelocity() + _characterController.getFollowVelocity());

    // now that physics has adjusted our position, we can update attachements.
    Avatar::simulateAttachments(deltaTime);
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
    _scriptedMotorVelocity = glm::vec3(0.0f);
    _scriptedMotorTimescale = DEFAULT_SCRIPTED_MOTOR_TIMESCALE;
}

void MyAvatar::setCollisionSoundURL(const QString& url) {
    _collisionSoundURL = url;
    if (!url.isEmpty() && (url != _collisionSoundURL)) {
        emit newCollisionSoundURL(QUrl(url));
    }
}

void MyAvatar::attach(const QString& modelURL, const QString& jointName,
                      const glm::vec3& translation, const glm::quat& rotation,
                      float scale, bool isSoft,
                      bool allowDuplicates, bool useSaved) {
    if (QThread::currentThread() != thread()) {
        Avatar::attach(modelURL, jointName, translation, rotation, scale, isSoft, allowDuplicates, useSaved);
        return;
    }
    if (useSaved) {
        AttachmentData attachment = loadAttachmentData(modelURL, jointName);
        if (attachment.isValid()) {
            Avatar::attach(modelURL, attachment.jointName,
                           attachment.translation, attachment.rotation,
                           attachment.scale, attachment.isSoft,
                           allowDuplicates, useSaved);
            return;
        }
    }
    Avatar::attach(modelURL, jointName, translation, rotation, scale, isSoft, allowDuplicates, useSaved);
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
        glm::vec3 cameraPosition = qApp->getCamera()->getPosition();

        glm::mat4 headPose = qApp->getActiveDisplayPlugin()->getHeadPose(qApp->getFrameCount());
        glm::mat4 leftEyePose = qApp->getActiveDisplayPlugin()->getEyeToHeadTransform(Eye::Left);
        leftEyePose = leftEyePose * headPose;
        glm::vec3 leftEyePosition = extractTranslation(leftEyePose);
        glm::mat4 rightEyePose = qApp->getActiveDisplayPlugin()->getEyeToHeadTransform(Eye::Right);
        rightEyePose = rightEyePose * headPose;
        glm::vec3 rightEyePosition = extractTranslation(rightEyePose);
        glm::vec3 headPosition = extractTranslation(headPose);

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

void MyAvatar::setAnimGraphUrl(const QUrl& url) {
    if (_animGraphUrl == url) {
        return;
    }
    destroyAnimGraph();
    _skeletonModel.reset(); // Why is this necessary? Without this, we crash in the next render.
    _animGraphUrl = url;
    initAnimGraph();
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
    auto graphUrl =_animGraphUrl.isEmpty() ?
        QUrl::fromLocalFile(PathUtils::resourcesPath() + "meshes/defaultAvatar_full/avatar-animation.json") :
        QUrl(_animGraphUrl);
    _rig->initAnimGraph(graphUrl);

    _bodySensorMatrix = deriveBodyFromHMDSensor(); // Based on current cached HMD position/rotation..
    updateSensorToWorldMatrix(); // Uses updated position/orientation and _bodySensorMatrix changes
}

void MyAvatar::destroyAnimGraph() {
    _rig->destroyAnimGraph();
}

void MyAvatar::preRender(RenderArgs* renderArgs) {

    render::ScenePointer scene = qApp->getMain3DScene();
    const bool shouldDrawHead = shouldRenderHead(renderArgs);

    if (_skeletonModel.initWhenReady(scene)) {
        initHeadBones();
        _skeletonModel.setCauterizeBoneSet(_headBoneSet);
        initAnimGraph();
    }

    if (_enableDebugDrawDefaultPose || _enableDebugDrawAnimPose) {

        auto animSkeleton = _rig->getAnimSkeleton();

        // the rig is in the skeletonModel frame
        AnimPose xform(glm::vec3(1), _skeletonModel.getRotation(), _skeletonModel.getTranslation());

        if (_enableDebugDrawDefaultPose && animSkeleton) {
            glm::vec4 gray(0.2f, 0.2f, 0.2f, 0.2f);
            AnimDebugDraw::getInstance().addAbsolutePoses("myAvatarDefaultPoses", animSkeleton, _rig->getAbsoluteDefaultPoses(), xform, gray);
        }

        if (_enableDebugDrawAnimPose && animSkeleton) {
            // build absolute AnimPoseVec from rig
            AnimPoseVec absPoses;
            absPoses.reserve(_rig->getJointStateCount());
            for (int i = 0; i < _rig->getJointStateCount(); i++) {
                absPoses.push_back(AnimPose(_rig->getJointTransform(i)));
            }
            glm::vec4 cyan(0.1f, 0.6f, 0.6f, 1.0f);
            AnimDebugDraw::getInstance().addAbsolutePoses("myAvatarAnimPoses", animSkeleton, absPoses, xform, cyan);
        }
    }

    DebugDraw::getInstance().updateMyAvatarPos(getPosition());
    DebugDraw::getInstance().updateMyAvatarRot(getOrientation());

    if (shouldDrawHead != _prevShouldDrawHead) {
        _skeletonModel.setCauterizeBones(!shouldDrawHead);
    }
    _prevShouldDrawHead = shouldDrawHead;
}

const float RENDER_HEAD_CUTOFF_DISTANCE = 0.3f;

bool MyAvatar::cameraInsideHead() const {
    const glm::vec3 cameraPosition = qApp->getCamera()->getPosition();
    return glm::length(cameraPosition - getHeadPosition()) < (RENDER_HEAD_CUTOFF_DISTANCE * getUniformScale());
}

bool MyAvatar::shouldRenderHead(const RenderArgs* renderArgs) const {
    bool defaultMode = renderArgs->_renderMode == RenderArgs::DEFAULT_RENDER_MODE;
    bool firstPerson = qApp->getCamera()->getMode() == CAMERA_MODE_FIRST_PERSON;
    bool insideHead = cameraInsideHead();
    return !defaultMode || !firstPerson || !insideHead;
}

void MyAvatar::updateOrientation(float deltaTime) {

    //  Smoothly rotate body with arrow keys
    float targetSpeed = _driveKeys[YAW] * _yawSpeed;
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

    float totalBodyYaw = _bodyYawDelta * deltaTime;


    // Comfort Mode: If you press any of the left/right rotation drive keys or input, you'll
    // get an instantaneous 15 degree turn. If you keep holding the key down you'll get another
    // snap turn every half second.
    if (_driveKeys[STEP_YAW] != 0.0f) {
        totalBodyYaw += _driveKeys[STEP_YAW];
    }

    // use head/HMD orientation to turn while flying
    if (getCharacterController()->getState() == CharacterController::State::Hover) {

        // This is the direction the user desires to fly in.
        glm::vec3 desiredFacing = getHead()->getCameraOrientation() * Vectors::UNIT_Z;
        desiredFacing.y = 0.0f;

        // This is our reference frame, it is captured when the user begins to move.
        glm::vec3 referenceFacing = transformVectorFast(_sensorToWorldMatrix, _hoverReferenceCameraFacing);
        referenceFacing.y = 0.0f;
        referenceFacing = glm::normalize(referenceFacing);
        glm::vec3 referenceRight(referenceFacing.z, 0.0f, -referenceFacing.x);
        const float HOVER_FLY_ROTATION_PERIOD = 0.5f;
        float tau = glm::clamp(deltaTime / HOVER_FLY_ROTATION_PERIOD, 0.0f, 1.0f);

        // new facing is a linear interpolation between the desired and reference vectors.
        glm::vec3 newFacing = glm::normalize((1.0f - tau) * referenceFacing + tau * desiredFacing);

        // calcualte the signed delta yaw angle to apply so that we match our newFacing.
        float sign = copysignf(1.0f, glm::dot(desiredFacing, referenceRight));
        float deltaAngle = sign * acosf(glm::clamp(glm::dot(referenceFacing, newFacing), -1.0f, 1.0f));

        // speedFactor is 0 when we are at rest adn 1.0 when we are at max flying speed.
        const float MAX_FLYING_SPEED = 30.0f;
        float speedFactor = glm::min(glm::length(getVelocity()) / MAX_FLYING_SPEED, 1.0f);

        // apply our delta, but scale it by the speed factor, so we turn faster when we are flying faster.
        totalBodyYaw += (speedFactor * deltaAngle * (180.0f / PI));
    }


    // update body orientation by movement inputs
    setOrientation(getOrientation() * glm::quat(glm::radians(glm::vec3(0.0f, totalBodyYaw, 0.0f))));

    getHead()->setBasePitch(getHead()->getBasePitch() + _driveKeys[PITCH] * _pitchSpeed * deltaTime);

    if (qApp->getAvatarUpdater()->isHMDMode()) {
        glm::quat orientation = glm::quat_cast(getSensorToWorldMatrix()) * getHMDSensorOrientation();
        glm::quat bodyOrientation = getWorldBodyOrientation();
        glm::quat localOrientation = glm::inverse(bodyOrientation) * orientation;

        // these angles will be in radians
        // ... so they need to be converted to degrees before we do math...
        glm::vec3 euler = glm::eulerAngles(localOrientation) * DEGREES_PER_RADIAN;

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

    // FIXME how do I implement step translation as well?


    float keyboardInput = fabsf(_driveKeys[TRANSLATE_Z]) + fabsf(_driveKeys[TRANSLATE_X]) + fabsf(_driveKeys[TRANSLATE_Y]);
    if (keyboardInput) {
        // Compute keyboard input
        glm::vec3 front = (_driveKeys[TRANSLATE_Z]) * IDENTITY_FRONT;
        glm::vec3 right = (_driveKeys[TRANSLATE_X]) * IDENTITY_RIGHT;
        glm::vec3 up = (_driveKeys[TRANSLATE_Y]) * IDENTITY_UP;

        glm::vec3 direction = front + right + up;
        float directionLength = glm::length(direction);

        //qCDebug(interfaceapp, "direction = (%.5f, %.5f, %.5f)", direction.x, direction.y, direction.z);

        // Compute motor magnitude
        if (directionLength > EPSILON) {
            direction /= directionLength;

            if (isHovering) {
                // we're flying --> complex acceleration curve with high max speed
                float motorSpeed = glm::length(_keyboardMotorVelocity);
                float finalMaxMotorSpeed = getUniformScale() * MAX_KEYBOARD_MOTOR_SPEED;
                float speedGrowthTimescale  = 2.0f;
                float speedIncreaseFactor = 1.8f;
                motorSpeed *= 1.0f + glm::clamp(deltaTime / speedGrowthTimescale , 0.0f, 1.0f) * speedIncreaseFactor;
                const float maxBoostSpeed = getUniformScale() * MAX_BOOST_SPEED;
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

    float boomChange = _driveKeys[ZOOM];
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
    bool isHovering = _characterController.getState() == CharacterController::State::Hover;
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


    // capture the head rotation, in sensor space, when the user first indicates they would like to move/fly.
    if (!_hoverReferenceCameraFacingIsCaptured && (fabs(_driveKeys[TRANSLATE_Z]) > 0.1f || fabs(_driveKeys[TRANSLATE_X]) > 0.1f)) {
        _hoverReferenceCameraFacingIsCaptured = true;
        // transform the camera facing vector into sensor space.
        _hoverReferenceCameraFacing = transformVectorFast(glm::inverse(_sensorToWorldMatrix), getHead()->getCameraOrientation() * Vectors::UNIT_Z);
    } else if (_hoverReferenceCameraFacingIsCaptured && (fabs(_driveKeys[TRANSLATE_Z]) <= 0.1f && fabs(_driveKeys[TRANSLATE_X]) <= 0.1f)) {
        _hoverReferenceCameraFacingIsCaptured = false;
    }
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
    qCDebug(interfaceapp, "Reset scale to %f", (double)_targetScale);
}


void MyAvatar::goToLocation(const QVariant& propertiesVar) {
    qCDebug(interfaceapp, "MyAvatar QML goToLocation");
    auto properties = propertiesVar.toMap();
    if (!properties.contains("position")) {
        qCWarning(interfaceapp, "goToLocation called without a position variable");
        return;
    }

    bool validPosition;
    glm::vec3 v = vec3FromVariant(properties["position"], validPosition);
    if (!validPosition) {
        qCWarning(interfaceapp, "goToLocation called with invalid position variable");
        return;
    }
    bool validOrientation = false;
    glm::quat q;
    if (properties.contains("orientation")) {
        q = quatFromVariant(properties["orientation"], validOrientation);
        if (!validOrientation) {
            glm::vec3 eulerOrientation = vec3FromVariant(properties["orientation"], validOrientation);
            q = glm::quat(eulerOrientation);
            if (!validOrientation) {
                qCWarning(interfaceapp, "goToLocation called with invalid orientation variable");
            }
        }
    }

    if (validOrientation) {
        goToLocation(v, true, q);
    } else {
        goToLocation(v);
    }
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

void MyAvatar::clearDriveKeys() {
    for (int i = 0; i < MAX_DRIVE_KEYS; ++i) {
        _driveKeys[i] = 0.0f;
    }
}

void MyAvatar::relayDriveKeysToCharacterController() {
    if (_driveKeys[TRANSLATE_Y] > 0.0f) {
        _characterController.jump();
    }
}

glm::vec3 MyAvatar::getWorldBodyPosition() const {
    return transformPoint(_sensorToWorldMatrix, extractTranslation(_bodySensorMatrix));
}

glm::quat MyAvatar::getWorldBodyOrientation() const {
    return glm::quat_cast(_sensorToWorldMatrix * _bodySensorMatrix);
}

// old school meat hook style
glm::mat4 MyAvatar::deriveBodyFromHMDSensor() const {

    // HMD is in sensor space.
    const glm::vec3 hmdPosition = getHMDSensorPosition();
    const glm::quat hmdOrientation = getHMDSensorOrientation();
    const glm::quat hmdOrientationYawOnly = cancelOutRollAndPitch(hmdOrientation);

    // 2 meter tall dude (in rig coordinates)
    const glm::vec3 DEFAULT_RIG_MIDDLE_EYE_POS(0.0f, 0.9f, 0.0f);
    const glm::vec3 DEFAULT_RIG_NECK_POS(0.0f, 0.70f, 0.0f);
    const glm::vec3 DEFAULT_RIG_HIPS_POS(0.0f, 0.05f, 0.0f);

    int rightEyeIndex = _rig->indexOfJoint("RightEye");
    int leftEyeIndex = _rig->indexOfJoint("LeftEye");
    int neckIndex = _rig->indexOfJoint("Neck");
    int hipsIndex = _rig->indexOfJoint("Hips");

    glm::vec3 rigMiddleEyePos = DEFAULT_RIG_MIDDLE_EYE_POS;
    if (leftEyeIndex >= 0 && rightEyeIndex >= 0) {
        rigMiddleEyePos = (_rig->getAbsoluteDefaultPose(leftEyeIndex).trans + _rig->getAbsoluteDefaultPose(rightEyeIndex).trans) / 2.0f;
    }
    glm::vec3 rigNeckPos = neckIndex != -1 ? _rig->getAbsoluteDefaultPose(neckIndex).trans : DEFAULT_RIG_NECK_POS;
    glm::vec3 rigHipsPos = hipsIndex != -1 ? _rig->getAbsoluteDefaultPose(hipsIndex).trans : DEFAULT_RIG_HIPS_POS;

    glm::vec3 localEyes = (rigMiddleEyePos - rigHipsPos);
    glm::vec3 localNeck = (rigNeckPos - rigHipsPos);

    // apply simplistic head/neck model
    // figure out where the avatar body should be by applying offsets from the avatar's neck & head joints.

    // eyeToNeck offset is relative full HMD orientation.
    // while neckToRoot offset is only relative to HMDs yaw.
    // Y_180 is necessary because rig is z forward and hmdOrientation is -z forward
    glm::vec3 eyeToNeck = hmdOrientation * Quaternions::Y_180 * (localNeck - localEyes);
    glm::vec3 neckToRoot = hmdOrientationYawOnly * Quaternions::Y_180 * -localNeck;
    glm::vec3 bodyPos = hmdPosition + eyeToNeck + neckToRoot;

    return createMatFromQuatAndPos(hmdOrientationYawOnly, bodyPos);
}

glm::vec3 MyAvatar::getPositionForAudio() {
    switch (_audioListenerMode) {
        case AudioListenerMode::FROM_HEAD:
            return getHead()->getPosition();
        case AudioListenerMode::FROM_CAMERA:
            return qApp->getCamera()->getPosition();
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
            return qApp->getCamera()->getOrientation();
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


void MyAvatar::lateUpdatePalms() {
    Avatar::updatePalms();
}


static const float FOLLOW_TIME = 0.5f;

MyAvatar::FollowHelper::FollowHelper() {
    deactivate();
}

void MyAvatar::FollowHelper::deactivate() {
    for (int i = 0; i < NumFollowTypes; i++) {
        deactivate((FollowType)i);
    }
}

void MyAvatar::FollowHelper::deactivate(FollowType type) {
    assert(type >= 0 && type < NumFollowTypes);
    _timeRemaining[(int)type] = 0.0f;
}

void MyAvatar::FollowHelper::activate(FollowType type) {
    assert(type >= 0 && type < NumFollowTypes);
    // TODO: Perhaps, the follow time should be proportional to the displacement.
    _timeRemaining[(int)type] = FOLLOW_TIME;
}

bool MyAvatar::FollowHelper::isActive(FollowType type) const {
    assert(type >= 0 && type < NumFollowTypes);
    return _timeRemaining[(int)type] > 0.0f;
}

bool MyAvatar::FollowHelper::isActive() const {
    for (int i = 0; i < NumFollowTypes; i++) {
        if (isActive((FollowType)i)) {
            return true;
        }
    }
    return false;
}

float MyAvatar::FollowHelper::getMaxTimeRemaining() const {
    float max = 0.0f;
    for (int i = 0; i < NumFollowTypes; i++) {
        if (_timeRemaining[i] > max) {
            max = _timeRemaining[i];
        }
    }
    return max;
}

void MyAvatar::FollowHelper::decrementTimeRemaining(float dt) {
    for (int i = 0; i < NumFollowTypes; i++) {
        _timeRemaining[i] -= dt;
    }
}

bool MyAvatar::FollowHelper::shouldActivateRotation(const MyAvatar& myAvatar, const glm::mat4& desiredBodyMatrix, const glm::mat4& currentBodyMatrix) const {
    const float FOLLOW_ROTATION_THRESHOLD = cosf(PI / 6.0f); // 30 degrees
    glm::vec2 bodyFacing = getFacingDir2D(currentBodyMatrix);
    return glm::dot(myAvatar.getHMDSensorFacingMovingAverage(), bodyFacing) < FOLLOW_ROTATION_THRESHOLD;
}

bool MyAvatar::FollowHelper::shouldActivateHorizontal(const MyAvatar& myAvatar, const glm::mat4& desiredBodyMatrix, const glm::mat4& currentBodyMatrix) const {

    // -z axis of currentBodyMatrix in world space.
    glm::vec3 forward = glm::normalize(glm::vec3(-currentBodyMatrix[0][2], -currentBodyMatrix[1][2], -currentBodyMatrix[2][2]));
    // x axis of currentBodyMatrix in world space.
    glm::vec3 right = glm::normalize(glm::vec3(currentBodyMatrix[0][0], currentBodyMatrix[1][0], currentBodyMatrix[2][0]));
    glm::vec3 offset = extractTranslation(desiredBodyMatrix) - extractTranslation(currentBodyMatrix);

    float forwardLeanAmount = glm::dot(forward, offset);
    float lateralLeanAmount = glm::dot(right, offset);

    const float MAX_LATERAL_LEAN = 0.3f;
    const float MAX_FORWARD_LEAN = 0.15f;
    const float MAX_BACKWARD_LEAN = 0.1f;

    if (forwardLeanAmount > 0 && forwardLeanAmount > MAX_FORWARD_LEAN) {
        return true;
    } else if (forwardLeanAmount < 0 && forwardLeanAmount < -MAX_BACKWARD_LEAN) {
        return true;
    }

    return fabs(lateralLeanAmount) > MAX_LATERAL_LEAN;
}

bool MyAvatar::FollowHelper::shouldActivateVertical(const MyAvatar& myAvatar, const glm::mat4& desiredBodyMatrix, const glm::mat4& currentBodyMatrix) const {

    const float CYLINDER_TOP = 0.1f;
    const float CYLINDER_BOTTOM = -1.5f;

    glm::vec3 offset = extractTranslation(desiredBodyMatrix) - extractTranslation(currentBodyMatrix);
    return (offset.y > CYLINDER_TOP) || (offset.y < CYLINDER_BOTTOM);
}

void MyAvatar::FollowHelper::prePhysicsUpdate(MyAvatar& myAvatar, const glm::mat4& desiredBodyMatrix, const glm::mat4& currentBodyMatrix, bool hasDriveInput) {
    _desiredBodyMatrix = desiredBodyMatrix;
    if (!isActive(Rotation) && shouldActivateRotation(myAvatar, desiredBodyMatrix, currentBodyMatrix)) {
        activate(Rotation);
    }
    if (!isActive(Horizontal) && shouldActivateHorizontal(myAvatar, desiredBodyMatrix, currentBodyMatrix)) {
        activate(Horizontal);
    }
    if (!isActive(Vertical) && (shouldActivateVertical(myAvatar, desiredBodyMatrix, currentBodyMatrix) || hasDriveInput)) {
        activate(Vertical);
    }

    glm::mat4 desiredWorldMatrix = myAvatar.getSensorToWorldMatrix() * _desiredBodyMatrix;
    glm::mat4 currentWorldMatrix = myAvatar.getSensorToWorldMatrix() * currentBodyMatrix;

    AnimPose followWorldPose(currentWorldMatrix);
    if (isActive(Rotation)) {
        followWorldPose.rot = glmExtractRotation(desiredWorldMatrix);
    }
    if (isActive(Horizontal)) {
        glm::vec3 desiredTranslation = extractTranslation(desiredWorldMatrix);
        followWorldPose.trans.x = desiredTranslation.x;
        followWorldPose.trans.z = desiredTranslation.z;
    }
    if (isActive(Vertical)) {
        glm::vec3 desiredTranslation = extractTranslation(desiredWorldMatrix);
        followWorldPose.trans.y = desiredTranslation.y;
    }

    myAvatar.getCharacterController()->setFollowParameters(followWorldPose, getMaxTimeRemaining());
}

glm::mat4 MyAvatar::FollowHelper::postPhysicsUpdate(const MyAvatar& myAvatar, const glm::mat4& currentBodyMatrix) {
    if (isActive()) {
        float dt = myAvatar.getCharacterController()->getFollowTime();
        decrementTimeRemaining(dt);

        // apply follow displacement to the body matrix.
        glm::vec3 worldLinearDisplacement = myAvatar.getCharacterController()->getFollowLinearDisplacement();
        glm::quat worldAngularDisplacement = myAvatar.getCharacterController()->getFollowAngularDisplacement();
        glm::quat sensorToWorld = glmExtractRotation(myAvatar.getSensorToWorldMatrix());
        glm::quat worldToSensor = glm::inverse(sensorToWorld);

        glm::vec3 sensorLinearDisplacement = worldToSensor * worldLinearDisplacement;
        glm::quat sensorAngularDisplacement = worldToSensor * worldAngularDisplacement * sensorToWorld;

        glm::mat4 newBodyMat = createMatFromQuatAndPos(sensorAngularDisplacement * glmExtractRotation(currentBodyMatrix),
                                                       sensorLinearDisplacement + extractTranslation(currentBodyMatrix));
        return newBodyMat;

    } else {
        return currentBodyMatrix;
    }
}

float MyAvatar::getAccelerationEnergy() {
    glm::vec3 velocity = getVelocity();
    int changeInVelocity = abs(velocity.length() - priorVelocity.length());
    float changeInEnergy = priorVelocity.length() * changeInVelocity * AVATAR_MOVEMENT_ENERGY_CONSTANT;
    priorVelocity = velocity;
    
    return changeInEnergy;
}

float MyAvatar::getEnergy() {
    return currentEnergy;
}

void MyAvatar::setEnergy(float value) {
    currentEnergy = value;
}

float MyAvatar::getAudioEnergy() {
    return getAudioLoudness() * AUDIO_ENERGY_CONSTANT;
}

bool MyAvatar::didTeleport() {
    glm::vec3 pos = getPosition();
    glm::vec3 changeInPosition = pos - lastPosition;
    lastPosition = pos;
    return (changeInPosition.length() > MAX_AVATAR_MOVEMENT_PER_FRAME);
}

