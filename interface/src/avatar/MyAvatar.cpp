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
#include <display-plugins/DisplayPlugin.h>
#include <FSTReader.h>
#include <GeometryUtil.h>
#include <NodeList.h>
#include <udt/PacketHeaders.h>
#include <PathUtils.h>
#include <PerfStat.h>
#include <SharedUtil.h>
#include <SoundCache.h>
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

const float MAX_WALKING_SPEED = 2.6f; // human walking speed
const float MAX_BOOST_SPEED = 0.5f * MAX_WALKING_SPEED; // action motor gets additive boost below this speed
const float MIN_AVATAR_SPEED = 0.05f;
const float MIN_AVATAR_SPEED_SQUARED = MIN_AVATAR_SPEED * MIN_AVATAR_SPEED; // speed is set to zero below this

const float YAW_SPEED_DEFAULT = 120.0f;   // degrees/sec
const float PITCH_SPEED_DEFAULT = 90.0f; // degrees/sec

// TODO: normalize avatar speed for standard avatar size, then scale all motion logic
// to properly follow avatar size.
float MAX_AVATAR_SPEED = 30.0f;
float MAX_ACTION_MOTOR_SPEED = MAX_AVATAR_SPEED;
float MIN_SCRIPTED_MOTOR_TIMESCALE = 0.005f;
float DEFAULT_SCRIPTED_MOTOR_TIMESCALE = 1.0e6f;
const int SCRIPTED_MOTOR_CAMERA_FRAME = 0;
const int SCRIPTED_MOTOR_AVATAR_FRAME = 1;
const int SCRIPTED_MOTOR_WORLD_FRAME = 2;
const QString& DEFAULT_AVATAR_COLLISION_SOUND_URL = "https://hifi-public.s3.amazonaws.com/sounds/Collisions-otherorganic/Body_Hits_Impact.wav";

const float MyAvatar::ZOOM_MIN = 0.5f;
const float MyAvatar::ZOOM_MAX = 25.0f;
const float MyAvatar::ZOOM_DEFAULT = 1.5f;

// OUTOFBODY_HACK defined in Rig.cpp
extern bool OUTOFBODY_HACK_ENABLE_DEBUG_DRAW_IK_TARGETS;

// OUTOFBODY_HACK defined in SkeletonModel.cpp
extern glm::vec3 TRUNCATE_IK_CAPSULE_POSITION;
extern float TRUNCATE_IK_CAPSULE_LENGTH;
extern float TRUNCATE_IK_CAPSULE_RADIUS;

MyAvatar::MyAvatar(RigPointer rig) :
    Avatar(rig),
    _wasPushing(false),
    _isPushing(false),
    _isBeingPushed(false),
    _isBraking(false),
    _boomLength(ZOOM_DEFAULT),
    _yawSpeed(YAW_SPEED_DEFAULT),
    _pitchSpeed(PITCH_SPEED_DEFAULT),
    _thrust(0.0f),
    _actionMotorVelocity(0.0f),
    _scriptedMotorVelocity(0.0f),
    _scriptedMotorTimescale(DEFAULT_SCRIPTED_MOTOR_TIMESCALE),
    _scriptedMotorFrame(SCRIPTED_MOTOR_CAMERA_FRAME),
    _motionBehaviors(AVATAR_MOTION_DEFAULTS),
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


    // Necessary to select the correct slot
    using SlotType = void(MyAvatar::*)(const glm::vec3&, bool, const glm::quat&, bool);

    // connect to AddressManager signal for location jumps
    connect(DependencyManager::get<AddressManager>().data(), &AddressManager::locationChangeRequired,
            this, static_cast<SlotType>(&MyAvatar::goToLocation));

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
            // head orientation
            _headData->setLookAtPosition(headData->getLookAtPosition());
        }
    });

    connect(rig.get(), SIGNAL(onLoadComplete()), this, SIGNAL(onLoadComplete()));
}

MyAvatar::~MyAvatar() {
    _lookAtTargetAvatar.reset();
}

void MyAvatar::setOrientationVar(const QVariant& newOrientationVar) {
    Avatar::setOrientation(quatFromVariant(newOrientationVar));
}

QVariant MyAvatar::getOrientationVar() const {
    return quatToVariant(Avatar::getOrientation());
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

void MyAvatar::centerBody() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "centerBody");
        return;
    }

    // derive the desired body orientation from the current hmd orientation, before the sensor reset.
    auto newBodySensorMatrix = deriveBodyFromHMDSensor(); // Based on current cached HMD position/rotation..

    // transform this body into world space
    auto worldBodyMatrix = _sensorToWorldMatrix * newBodySensorMatrix;
    auto worldBodyPos = extractTranslation(worldBodyMatrix);
    auto worldBodyRot = glm::normalize(glm::quat_cast(worldBodyMatrix));

    // this will become our new position.
    setPosition(worldBodyPos);
    setOrientation(worldBodyRot);

    // reset the body in sensor space
    _bodySensorMatrix = newBodySensorMatrix;

    // rebuild the sensor to world matrix
    updateSensorToWorldMatrix();
}

void MyAvatar::clearIKJointLimitHistory() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "clearIKJointLimitHistory");
        return;
    }

    if (_rig) {
        _rig->clearIKJointLimitHistory();
    }
}

void MyAvatar::reset(bool andRecenter, bool andReload, bool andHead) {

    assert(QThread::currentThread() == thread());

    // Reset dynamic state.
    _wasPushing = _isPushing = _isBraking = false;
    _follow.deactivate();
    if (andReload) {
        _skeletonModel->reset();
    }
    if (andHead) { // which drives camera in desktop
        getHead()->reset();
    }
    setThrust(glm::vec3(0.0f));

    if (andRecenter) {
        // derive the desired body orientation from the *old* hmd orientation, before the sensor reset.
        auto newBodySensorMatrix = deriveBodyFromHMDSensor(); // Based on current cached HMD position/rotation..

        // transform this body into world space
        auto worldBodyMatrix = _sensorToWorldMatrix * newBodySensorMatrix;
        auto worldBodyPos = extractTranslation(worldBodyMatrix);
        auto worldBodyRot = glm::normalize(glm::quat_cast(worldBodyMatrix));

        // this will become our new position.
        setPosition(worldBodyPos);
        setOrientation(worldBodyRot);

        // now sample the new hmd orientation AFTER sensor reset, which should be identity.
        glm::mat4 identity;
        updateFromHMDSensorMatrix(identity);

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
    head->relax(deltaTime);
    updateFromTrackers(deltaTime);

    //  Get audio loudness data from audio input device
    auto audio = DependencyManager::get<AudioClient>();
    head->setAudioLoudness(audio->getLastInputLoudness());
    head->setAudioAverageLoudness(audio->getAudioAverageInputLoudness());

    if (_avatarEntityDataLocallyEdited) {
        sendIdentityPacket();
    }

    simulate(deltaTime);

    currentEnergy += energyChargeRate;
    currentEnergy -= getAccelerationEnergy();
    currentEnergy -= getAudioEnergy();

    if(didTeleport()) {
        currentEnergy = 0.0f;
    }
    currentEnergy = max(0.0f, min(currentEnergy,1.0f));
    emit energyChanged(currentEnergy);

    updateEyeContactTarget(deltaTime);
}

void MyAvatar::updateEyeContactTarget(float deltaTime) {

    _eyeContactTargetTimer -= deltaTime;
    if (_eyeContactTargetTimer < 0.0f) {

        const float CHANCE_OF_CHANGING_TARGET = 0.01f;
        if (randFloat() < CHANCE_OF_CHANGING_TARGET) {

            float const FIFTY_FIFTY_CHANCE = 0.5f;
            float const EYE_TO_MOUTH_CHANCE = 0.25f;
            switch (_eyeContactTarget) {
                case LEFT_EYE:
                    _eyeContactTarget = (randFloat() < EYE_TO_MOUTH_CHANCE) ? MOUTH : RIGHT_EYE;
                    break;
                case RIGHT_EYE:
                    _eyeContactTarget = (randFloat() < EYE_TO_MOUTH_CHANCE) ? MOUTH : LEFT_EYE;
                    break;
                case MOUTH:
                default:
                    _eyeContactTarget = (randFloat() < FIFTY_FIFTY_CHANCE) ? RIGHT_EYE : LEFT_EYE;
                    break;
            }

            const float EYE_TARGET_DELAY_TIME = 0.33f;
            _eyeContactTargetTimer = EYE_TARGET_DELAY_TIME;
        }
    }
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
        PerformanceTimer perfTimer("skeleton");
        _skeletonModel->simulate(deltaTime);
    }

    // we've achived our final adjusted position and rotation for the avatar
    // and all of its joints, now update our attachements.
    Avatar::simulateAttachments(deltaTime);

    if (!_skeletonModel->hasSkeleton()) {
        // All the simulation that can be done has been done
        getHead()->setPosition(getPosition()); // so audio-position isn't 0,0,0
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
        if (!_skeletonModel->getHeadPosition(headPosition)) {
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
        bool flyingAllowed = true;
        bool ghostingAllowed = true;
        entityTree->withWriteLock([&] {
            std::shared_ptr<ZoneEntityItem> zone = entityTreeRenderer->myAvatarZone();
            if (zone) {
                flyingAllowed = zone->getFlyingAllowed();
                ghostingAllowed = zone->getGhostingAllowed();
            }
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

                        packetSender->queueEditEntityMessage(PacketType::EntityEdit, entityTree, entity->getID(), properties);
                        entity->setLastBroadcast(usecTimestampNow());
                    }
                }
            });
            // also update the position of children in our local octree
            if (moveOperator.hasMovingEntities()) {
                PerformanceTimer perfTimer("recurseTreeWithOperator");
                entityTree->recurseTreeWithOperator(&moveOperator);
            }
        });
        _characterController.setFlyingAllowed(flyingAllowed);
        if (!ghostingAllowed && _characterController.getCollisionGroup() == BULLET_COLLISION_GROUP_COLLISIONLESS) {
            _characterController.setCollisionGroup(BULLET_COLLISION_GROUP_MY_AVATAR);
        }
    }

    updateAvatarEntities();
}

// As far as I know no HMD system supports a play area of a kilometer in radius.
static const float MAX_HMD_ORIGIN_DISTANCE = 1000.0f;

// Pass a recent sample of the HMD to the avatar.
// This can also update the avatar's position to follow the HMD
// as it moves through the world.
void MyAvatar::updateFromHMDSensorMatrix(const glm::mat4& hmdSensorMatrix) {
    // update the sensorMatrices based on the new hmd pose
    _hmdSensorMatrix = hmdSensorMatrix;
    auto newHmdSensorPosition = extractTranslation(hmdSensorMatrix);

    if (newHmdSensorPosition != _hmdSensorPosition &&
        glm::length(newHmdSensorPosition) > MAX_HMD_ORIGIN_DISTANCE) {
        qWarning() << "Invalid HMD sensor position " << newHmdSensorPosition;
        // Ignore unreasonable HMD sensor data
        return;
    }
    _hmdSensorPosition = newHmdSensorPosition;
    _hmdSensorOrientation = glm::quat_cast(hmdSensorMatrix);
    _hmdSensorFacing = getFacingDir2D(_hmdSensorOrientation);
}

void MyAvatar::updateJointFromController(controller::Action poseKey, ThreadSafeValueCache<glm::mat4>& matrixCache) {
    assert(QThread::currentThread() == thread());
    auto userInputMapper = DependencyManager::get<UserInputMapper>();
    controller::Pose controllerPose = userInputMapper->getPoseState(poseKey);
    Transform transform;
    transform.setTranslation(controllerPose.getTranslation());
    transform.setRotation(controllerPose.getRotation());
    glm::mat4 controllerMatrix = transform.getMatrix();
    matrixCache.set(controllerMatrix);
}

// best called at end of main loop, after physics.
// update sensor to world matrix from current body position and hmd sensor.
// This is so the correct camera can be used for rendering.
void MyAvatar::updateSensorToWorldMatrix() {
    // update the sensor mat so that the body position will end up in the desired
    // position when driven from the head.
    glm::mat4 bodyToWorld = createMatFromQuatAndPos(getOrientation(), getPosition());
    setSensorToWorldMatrix(bodyToWorld * glm::inverse(_bodySensorMatrix));
}

void MyAvatar::setSensorToWorldMatrix(const glm::mat4& sensorToWorld) {
    _sensorToWorldMatrix = sensorToWorld;

    lateUpdatePalms();

    if (_enableDebugDrawSensorToWorldMatrix) {
        DebugDraw::getInstance().addMarker("sensorToWorldMatrix", glmExtractRotation(_sensorToWorldMatrix),
                                           extractTranslation(_sensorToWorldMatrix), glm::vec4(1));
    }

    _sensorToWorldMatrixCache.set(_sensorToWorldMatrix);

    updateJointFromController(controller::Action::LEFT_HAND, _controllerLeftHandMatrixCache);
    updateJointFromController(controller::Action::RIGHT_HAND, _controllerRightHandMatrixCache);
}

//  Update avatar head rotation with sensor data
void MyAvatar::updateFromTrackers(float deltaTime) {
    glm::vec3 estimatedPosition, estimatedRotation;

    bool inHmd = qApp->isHMDMode();
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
        ViewFrustum viewFrustum;
        qApp->copyViewFrustum(viewFrustum);
        float magnifyFieldOfView = viewFrustum.getFieldOfView() / _realWorldFieldOfView.get();
        head->setDeltaPitch(estimatedRotation.x * magnifyFieldOfView);
        head->setDeltaYaw(estimatedRotation.y * magnifyFieldOfView);
        head->setDeltaRoll(estimatedRotation.z);
    }
}

glm::vec3 MyAvatar::getLeftHandPosition() const {
    auto pose = getLeftHandControllerPoseInAvatarFrame();
    return pose.isValid() ? pose.getTranslation() : glm::vec3(0.0f);
}

glm::vec3 MyAvatar::getRightHandPosition() const {
    auto pose = getRightHandControllerPoseInAvatarFrame();
    return pose.isValid() ? pose.getTranslation() : glm::vec3(0.0f);
}

glm::vec3 MyAvatar::getLeftHandTipPosition() const {
    const float TIP_LENGTH = 0.3f;
    auto pose = getLeftHandControllerPoseInAvatarFrame();
    return pose.isValid() ? pose.getTranslation() * pose.getRotation() + glm::vec3(0.0f, TIP_LENGTH, 0.0f) : glm::vec3(0.0f);
}

glm::vec3 MyAvatar::getRightHandTipPosition() const {
    const float TIP_LENGTH = 0.3f;
    auto pose = getRightHandControllerPoseInAvatarFrame();
    return pose.isValid() ? pose.getTranslation() * pose.getRotation() + glm::vec3(0.0f, TIP_LENGTH, 0.0f) : glm::vec3(0.0f);
}

controller::Pose MyAvatar::getLeftHandPose() const {
    return getLeftHandControllerPoseInAvatarFrame();
}

controller::Pose MyAvatar::getRightHandPose() const {
    return getRightHandControllerPoseInAvatarFrame();
}

controller::Pose MyAvatar::getLeftHandTipPose() const {
    auto pose = getLeftHandControllerPoseInAvatarFrame();
    glm::vec3 tipTrans = getLeftHandTipPosition();
    pose.velocity += glm::cross(pose.getAngularVelocity(), pose.getTranslation() - tipTrans);
    pose.translation = tipTrans;
    return pose;
}

controller::Pose MyAvatar::getRightHandTipPose() const {
    auto pose = getRightHandControllerPoseInAvatarFrame();
    glm::vec3 tipTrans = getRightHandTipPosition();
    pose.velocity += glm::cross(pose.getAngularVelocity(), pose.getTranslation() - tipTrans);
    pose.translation = tipTrans;
    return pose;
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

void MyAvatar::saveData() {
    Settings settings;
    settings.beginGroup("Avatar");

    settings.setValue("headPitch", getHead()->getBasePitch());

    settings.setValue("scale", _targetScale);

    settings.setValue("fullAvatarURL",
                      _fullAvatarURLFromPreferences == AvatarData::defaultFullAvatarModelUrl() ?
                      "" :
                      _fullAvatarURLFromPreferences.toString());

    settings.setValue("fullAvatarModelName", _fullAvatarModelName);

    QUrl animGraphUrl = _prefOverrideAnimGraphUrl.get();
    settings.setValue("animGraphURL", animGraphUrl);

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

    settings.beginWriteArray("avatarEntityData");
    int avatarEntityIndex = 0;
    _avatarEntitiesLock.withReadLock([&] {
        for (auto entityID : _avatarEntityData.keys()) {
            settings.setArrayIndex(avatarEntityIndex);
            settings.setValue("id", entityID);
            settings.setValue("properties", _avatarEntityData.value(entityID));
            avatarEntityIndex++;
        }
    });
    settings.endArray();

    settings.setValue("displayName", _displayName);
    settings.setValue("collisionSoundURL", _collisionSoundURL);
    settings.setValue("useSnapTurn", _useSnapTurn);
    settings.setValue("clearOverlayWhenMoving", _clearOverlayWhenMoving);

    settings.endGroup();
}

float loadSetting(Settings& settings, const QString& name, float defaultValue) {
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

void MyAvatar::setEnableDebugDrawHandControllers(bool isEnabled) {
    _enableDebugDrawHandControllers = isEnabled;

    if (!isEnabled) {
        DebugDraw::getInstance().removeMarker("leftHandController");
        DebugDraw::getInstance().removeMarker("rightHandController");
    }
}

void MyAvatar::setEnableDebugDrawSensorToWorldMatrix(bool isEnabled) {
    _enableDebugDrawSensorToWorldMatrix = isEnabled;

    if (!isEnabled) {
        DebugDraw::getInstance().removeMarker("sensorToWorldMatrix");
    }
}

void MyAvatar::setEnableDebugDrawIKTargets(bool isEnabled) {
    _enableDebugDrawIKTargets = isEnabled;

    OUTOFBODY_HACK_ENABLE_DEBUG_DRAW_IK_TARGETS = isEnabled;
}


void MyAvatar::setEnableMeshVisible(bool isEnabled) {
    render::ScenePointer scene = qApp->getMain3DScene();
    _skeletonModel->setVisibleInScene(isEnabled, scene);
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

    _targetScale = loadSetting(settings, "scale", 1.0f);
    setScale(glm::vec3(_targetScale));

    _prefOverrideAnimGraphUrl.set(QUrl(settings.value("animGraphURL", "").toString()));
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

    int avatarEntityCount = settings.beginReadArray("avatarEntityData");
    for (int i = 0; i < avatarEntityCount; i++) {
        settings.setArrayIndex(i);
        QUuid entityID = settings.value("id").toUuid();
        // QUuid entityID = QUuid::createUuid(); // generate a new ID
        QByteArray properties = settings.value("properties").toByteArray();
        updateAvatarEntity(entityID, properties);
    }
    settings.endArray();
    setAvatarEntityDataChanged(true);

    setDisplayName(settings.value("displayName").toString());
    setCollisionSoundURL(settings.value("collisionSoundURL", DEFAULT_AVATAR_COLLISION_SOUND_URL).toString());
    setSnapTurn(settings.value("useSnapTurn", _useSnapTurn).toBool());
    setClearOverlayWhenMoving(settings.value("clearOverlayWhenMoving", _clearOverlayWhenMoving).toBool());

    settings.endGroup();

    setEnableMeshVisible(Menu::getInstance()->isOptionChecked(MenuOption::MeshVisible));
    setEnableDebugDrawDefaultPose(Menu::getInstance()->isOptionChecked(MenuOption::AnimDebugDrawDefaultPose));
    setEnableDebugDrawAnimPose(Menu::getInstance()->isOptionChecked(MenuOption::AnimDebugDrawAnimPose));
    setEnableDebugDrawPosition(Menu::getInstance()->isOptionChecked(MenuOption::AnimDebugDrawPosition));
}

void MyAvatar::saveAttachmentData(const AttachmentData& attachment) const {
    Settings settings;
    settings.beginGroup("savedAttachmentData");
    settings.beginGroup(_skeletonModel->getURL().toString());
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
    settings.beginGroup(_skeletonModel->getURL().toString());
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
            float radius = glm::length(avatar->getHead()->getEyePosition() - avatar->getHead()->getRightEyePosition());
            float angleTo = coneSphereAngle(getHead()->getEyePosition(), lookForward, avatar->getHead()->getEyePosition(), radius);
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

                 ViewFrustum viewFrustum;
                 qApp->copyViewFrustum(viewFrustum);

                // scale gazeOffset by IPD, if wearing an HMD.
                if (qApp->isHMDMode()) {
                    glm::mat4 leftEye = qApp->getEyeOffset(Eye::Left);
                    glm::mat4 rightEye = qApp->getEyeOffset(Eye::Right);
                    glm::vec3 leftEyeHeadLocal = glm::vec3(leftEye[3]);
                    glm::vec3 rightEyeHeadLocal = glm::vec3(rightEye[3]);
                    glm::vec3 humanLeftEye = viewFrustum.getPosition() + (viewFrustum.getOrientation() * leftEyeHeadLocal);
                    glm::vec3 humanRightEye = viewFrustum.getPosition() + (viewFrustum.getOrientation() * rightEyeHeadLocal);

                    auto hmdInterface = DependencyManager::get<HMDScriptingInterface>();
                    float ipdScale = hmdInterface->getIPDScale();

                    // Scale by proportional differences between avatar and human.
                    float humanEyeSeparationInModelSpace = glm::length(humanLeftEye - humanRightEye) * ipdScale;
                    float avatarEyeSeparation = glm::length(avatarLeftEye - avatarRightEye);
                    if (avatarEyeSeparation > 0.0f) {
                        gazeOffset = gazeOffset * humanEyeSeparationInModelSpace / avatarEyeSeparation;
                    }
                }

                // And now we can finally add that offset to the camera.
                glm::vec3 corrected = viewFrustum.getPosition() + gazeOffset;

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
    return _eyeContactTarget;
}

glm::vec3 MyAvatar::getDefaultEyePosition() const {
    return getPosition() + getWorldAlignedOrientation() * Quaternions::Y_180 * _skeletonModel->getDefaultEyeModelPosition();
}

const float SCRIPT_PRIORITY = 1.0f + 1.0f;
const float RECORDER_PRIORITY = 1.0f + 1.0f;

void MyAvatar::setJointRotations(QVector<glm::quat> jointRotations) {
    int numStates = glm::min(_skeletonModel->getJointStateCount(), jointRotations.size());
    for (int i = 0; i < numStates; ++i) {
        // HACK: ATM only Recorder calls setJointRotations() so we hardcode its priority here
        _skeletonModel->setJointRotation(i, true, jointRotations[i], RECORDER_PRIORITY);
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

void MyAvatar::setSkeletonModelURL(const QUrl& skeletonModelURL) {

    Avatar::setSkeletonModelURL(skeletonModelURL);
    render::ScenePointer scene = qApp->getMain3DScene();
    _skeletonModel->setVisibleInScene(true, scene);
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

    const QString& urlString = fullAvatarURL.toString();
    if (urlString.isEmpty() || (fullAvatarURL != getSkeletonModelURL())) {
        setSkeletonModelURL(fullAvatarURL);
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
    float radius = scale * _skeletonModel->getBoundingCapsuleRadius();
    float height = scale * _skeletonModel->getBoundingCapsuleHeight() + 2.0f * radius;
    glm::vec3 corner(-radius, -0.5f * height, -radius);
    corner += scale * _skeletonModel->getBoundingCapsuleOffset();
    glm::vec3 diagonal(2.0f * radius, height, 2.0f * radius);
    _characterController.setLocalBoundingBox(corner, diagonal);
}

static controller::Pose applyLowVelocityFilter(const controller::Pose& oldPose, const controller::Pose& newPose) {
    controller::Pose finalPose = newPose;
    if (newPose.isValid()) {
        //  Use a velocity sensitive filter to damp small motions and preserve large ones with
        //  no latency.
        float velocityFilter = glm::clamp(1.0f - glm::length(oldPose.getVelocity()), 0.0f, 1.0f);
        finalPose.translation = oldPose.getTranslation() * velocityFilter + newPose.getTranslation() * (1.0f - velocityFilter);
        finalPose.rotation = safeMix(oldPose.getRotation(), newPose.getRotation(), 1.0f - velocityFilter);
    }
    return finalPose;
}

void MyAvatar::setHandControllerPosesInSensorFrame(const controller::Pose& left, const controller::Pose& right) {
    if (controller::InputDevice::getLowVelocityFilter()) {
        auto oldLeftPose = getLeftHandControllerPoseInSensorFrame();
        auto oldRightPose = getRightHandControllerPoseInSensorFrame();
        _leftHandControllerPoseInSensorFrameCache.set(applyLowVelocityFilter(oldLeftPose, left));
        _rightHandControllerPoseInSensorFrameCache.set(applyLowVelocityFilter(oldRightPose, right));
    } else {
        _leftHandControllerPoseInSensorFrameCache.set(left);
        _rightHandControllerPoseInSensorFrameCache.set(right);
    }
}

controller::Pose MyAvatar::getLeftHandControllerPoseInSensorFrame() const {
    return _leftHandControllerPoseInSensorFrameCache.get();
}

controller::Pose MyAvatar::getRightHandControllerPoseInSensorFrame() const {
    return _rightHandControllerPoseInSensorFrameCache.get();
}

controller::Pose MyAvatar::getLeftHandControllerPoseInWorldFrame() const {
    return _leftHandControllerPoseInSensorFrameCache.get().transform(getSensorToWorldMatrix());
}

controller::Pose MyAvatar::getRightHandControllerPoseInWorldFrame() const {
    return _rightHandControllerPoseInSensorFrameCache.get().transform(getSensorToWorldMatrix());
}

controller::Pose MyAvatar::getLeftHandControllerPoseInAvatarFrame() const {
    glm::mat4 invAvatarMatrix = glm::inverse(createMatFromQuatAndPos(getOrientation(), getPosition()));
    return getLeftHandControllerPoseInWorldFrame().transform(invAvatarMatrix);
}

controller::Pose MyAvatar::getRightHandControllerPoseInAvatarFrame() const {
    glm::mat4 invAvatarMatrix = glm::inverse(createMatFromQuatAndPos(getOrientation(), getPosition()));
    return getRightHandControllerPoseInWorldFrame().transform(invAvatarMatrix);
}

void MyAvatar::updateMotors() {
    _characterController.clearMotors();
    glm::quat motorRotation;
    if (_motionBehaviors & AVATAR_MOTION_ACTION_MOTOR_ENABLED) {
        if (_characterController.getState() == CharacterController::State::Hover ||
                _characterController.getCollisionGroup() == BULLET_COLLISION_GROUP_COLLISIONLESS) {
            motorRotation = getHead()->getCameraOrientation();
        } else {
            // non-hovering = walking: follow camera twist about vertical but not lift
            // so we decompose camera's rotation and store the twist part in motorRotation
            glm::quat liftRotation;
            swingTwistDecomposition(getHead()->getCameraOrientation(), _worldUpDirection, liftRotation, motorRotation);
        }
        const float DEFAULT_MOTOR_TIMESCALE = 0.2f;
        const float INVALID_MOTOR_TIMESCALE = 1.0e6f;
        if (_isPushing || _isBraking || !_isBeingPushed) {
            _characterController.addMotor(_actionMotorVelocity, motorRotation, DEFAULT_MOTOR_TIMESCALE, INVALID_MOTOR_TIMESCALE);
        } else {
            // _isBeingPushed must be true --> disable action motor by giving it a long timescale,
            // otherwise it's attempt to "stand in in place" could defeat scripted motor/thrusts
            _characterController.addMotor(_actionMotorVelocity, motorRotation, INVALID_MOTOR_TIMESCALE);
        }
    }
    if (_motionBehaviors & AVATAR_MOTION_SCRIPTED_MOTOR_ENABLED) {
        if (_scriptedMotorFrame == SCRIPTED_MOTOR_CAMERA_FRAME) {
            motorRotation = getHead()->getCameraOrientation() * glm::angleAxis(PI, Vectors::UNIT_Y);
        } else if (_scriptedMotorFrame == SCRIPTED_MOTOR_AVATAR_FRAME) {
            motorRotation = getOrientation() * glm::angleAxis(PI, Vectors::UNIT_Y);
        } else {
            // world-frame
            motorRotation = glm::quat();
        }
        _characterController.addMotor(_scriptedMotorVelocity, motorRotation, _scriptedMotorTimescale);
    }

    // legacy support for 'MyAvatar::applyThrust()', which has always been implemented as a
    // short-lived linearAcceleration
    _characterController.setLinearAcceleration(_thrust);
    _thrust = Vectors::ZERO;
}

void MyAvatar::prepareForPhysicsSimulation() {
    relayDriveKeysToCharacterController();
    updateMotors();

    bool success;
    glm::vec3 parentVelocity = getParentVelocity(success);
    if (!success) {
        qDebug() << "Warning: getParentVelocity failed" << getID();
        parentVelocity = glm::vec3();
    }
    _characterController.handleChangedCollisionGroup();
    _characterController.setParentVelocity(parentVelocity);

    _characterController.setPositionAndOrientation(getPosition(), getOrientation());
    if (qApp->isHMDMode()) {
        _follow.prePhysicsUpdate(*this, deriveBodyFromHMDSensor(), _bodySensorMatrix);
    } else {
        _follow.deactivate();
        getCharacterController()->disableFollow();
    }
}

void MyAvatar::harvestResultsFromPhysicsSimulation(float deltaTime) {
    glm::vec3 position = getPosition();
    glm::quat orientation = getOrientation();
    if (_characterController.isEnabledAndReady()) {
        _characterController.getPositionAndOrientation(position, orientation);
    }
    nextAttitude(position, orientation);

    // compute new _bodyToSensorMatrix
    glm::mat4 bodyToWorldMatrix = createMatFromQuatAndPos(orientation, position);
    _bodySensorMatrix = glm::inverse(_sensorToWorldMatrix) * bodyToWorldMatrix;

    if (_characterController.isEnabledAndReady()) {
        setVelocity(_characterController.getLinearVelocity() + _characterController.getFollowVelocity());
    } else {
        setVelocity(getVelocity() + _characterController.getFollowVelocity());
    }

    _follow.postPhysicsUpdate(*this);
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
    _scriptedMotorVelocity = Vectors::ZERO;
    _scriptedMotorTimescale = DEFAULT_SCRIPTED_MOTOR_TIMESCALE;
}

void MyAvatar::setCollisionSoundURL(const QString& url) {
    if (url != _collisionSoundURL) {
        _collisionSoundURL = url;

        emit newCollisionSoundURL(QUrl(_collisionSoundURL));
    }
}

SharedSoundPointer MyAvatar::getCollisionSound() {
    if (!_collisionSound) {
        _collisionSound = DependencyManager::get<SoundCache>()->getSound(_collisionSoundURL);
    }
    return _collisionSound;
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

void MyAvatar::setVisibleInSceneIfReady(Model* model, render::ScenePointer scene, bool visible) {
    if (model->isActive() && model->isRenderable()) {
        model->setVisibleInScene(visible, scene);
    }
}

void MyAvatar::initHeadBones() {
    int neckJointIndex = -1;
    if (_skeletonModel->isLoaded()) {
        neckJointIndex = _skeletonModel->getFBXGeometry().neckJointIndex;
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
        for (int i = 0; i < _skeletonModel->getJointStateCount(); i++) {
            if (jointIndex == _skeletonModel->getParentJointIndex(i)) {
                _headBoneSet.insert(i);
                q.push(i);
            }
        }
        q.pop();
    }
}

QUrl MyAvatar::getAnimGraphOverrideUrl() const {
    return _prefOverrideAnimGraphUrl.get();
}

void MyAvatar::setAnimGraphOverrideUrl(QUrl value) {
    _prefOverrideAnimGraphUrl.set(value);
    if (!value.isEmpty()) {
        setAnimGraphUrl(value);
    } else {
        initAnimGraph();
    }
}

QUrl MyAvatar::getAnimGraphUrl() const {
    return _currentAnimGraphUrl.get();
}

void MyAvatar::setAnimGraphUrl(const QUrl& url) {

    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setAnimGraphUrl", Q_ARG(QUrl, url));
        return;
    }

    if (_currentAnimGraphUrl.get() == url) {
        return;
    }
    destroyAnimGraph();
    _skeletonModel->reset(); // Why is this necessary? Without this, we crash in the next render.

    _currentAnimGraphUrl.set(url);
    _rig->initAnimGraph(url);

    _bodySensorMatrix = deriveBodyFromHMDSensor(); // Based on current cached HMD position/rotation..
    updateSensorToWorldMatrix(); // Uses updated position/orientation and _bodySensorMatrix changes
}

void MyAvatar::initAnimGraph() {
    QUrl graphUrl;
    if (!_prefOverrideAnimGraphUrl.get().isEmpty()) {
        graphUrl = _prefOverrideAnimGraphUrl.get();
    } else if (!_fstAnimGraphOverrideUrl.isEmpty()) {
        graphUrl = _fstAnimGraphOverrideUrl;
    } else {
        graphUrl = QUrl::fromLocalFile(PathUtils::resourcesPath() + "avatar/avatar-animation.json");
    }

    _rig->initAnimGraph(graphUrl);
    _currentAnimGraphUrl.set(graphUrl);

    _bodySensorMatrix = deriveBodyFromHMDSensor(); // Based on current cached HMD position/rotation..
    updateSensorToWorldMatrix(); // Uses updated position/orientation and _bodySensorMatrix changes
}

void MyAvatar::destroyAnimGraph() {
    _rig->destroyAnimGraph();
}

void MyAvatar::postUpdate(float deltaTime) {

    Avatar::postUpdate(deltaTime);

    render::ScenePointer scene = qApp->getMain3DScene();
    if (_skeletonModel->initWhenReady(scene)) {
        initHeadBones();
        _skeletonModel->setCauterizeBoneSet(_headBoneSet);
        _fstAnimGraphOverrideUrl = _skeletonModel->getGeometry()->getAnimGraphOverrideUrl();
        initAnimGraph();
    }

    if (_enableDebugDrawDefaultPose || _enableDebugDrawAnimPose) {

        auto animSkeleton = _rig->getAnimSkeleton();

        // the rig is in the skeletonModel frame
        AnimPose xform(glm::vec3(1), _skeletonModel->getRotation(), _skeletonModel->getTranslation());

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

    if (_enableDebugDrawHandControllers) {
        auto leftHandPose = getLeftHandControllerPoseInWorldFrame();
        auto rightHandPose = getRightHandControllerPoseInWorldFrame();

        if (leftHandPose.isValid()) {
            DebugDraw::getInstance().addMarker("leftHandController", leftHandPose.getRotation(), leftHandPose.getTranslation(), glm::vec4(1));
        } else {
            DebugDraw::getInstance().removeMarker("leftHandController");
        }

        if (rightHandPose.isValid()) {
            DebugDraw::getInstance().addMarker("rightHandController", rightHandPose.getRotation(), rightHandPose.getTranslation(), glm::vec4(1));
        } else {
            DebugDraw::getInstance().removeMarker("rightHandController");
        }
    }

    DebugDraw::getInstance().updateMyAvatarPos(getPosition());
    DebugDraw::getInstance().updateMyAvatarRot(getOrientation());
}


void MyAvatar::preDisplaySide(RenderArgs* renderArgs) {

    // toggle using the cauterizedBones depending on where the camera is and the rendering pass type.
    const bool shouldDrawHead = shouldRenderHead(renderArgs);
    if (shouldDrawHead != _prevShouldDrawHead) {
        _skeletonModel->setCauterizeBones(!shouldDrawHead);
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

    if (qApp->isHMDMode()) {
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

void MyAvatar::updateActionMotor(float deltaTime) {
    bool thrustIsPushing = (glm::length2(_thrust) > EPSILON);
    bool scriptedMotorIsPushing = (_motionBehaviors & AVATAR_MOTION_SCRIPTED_MOTOR_ENABLED)
        && _scriptedMotorTimescale < MAX_CHARACTER_MOTOR_TIMESCALE;
    _isBeingPushed = thrustIsPushing || scriptedMotorIsPushing;
    if (_isPushing || _isBeingPushed) {
        // we don't want the motor to brake if a script is pushing the avatar around
        // (we assume the avatar is driving itself via script)
        _isBraking = false;
    } else {
        float speed = glm::length(_actionMotorVelocity);
        const float MIN_ACTION_BRAKE_SPEED = 0.1f;
        _isBraking = _wasPushing || (_isBraking && speed > MIN_ACTION_BRAKE_SPEED);
    }

    // compute action input
    glm::vec3 front = (_driveKeys[TRANSLATE_Z]) * IDENTITY_FRONT;
    glm::vec3 right = (_driveKeys[TRANSLATE_X]) * IDENTITY_RIGHT;

    glm::vec3 direction = front + right;
    CharacterController::State state = _characterController.getState();
    if (state == CharacterController::State::Hover) {
        // we're flying --> support vertical motion
        glm::vec3 up = (_driveKeys[TRANSLATE_Y]) * IDENTITY_UP;
        direction += up;
    }

    _wasPushing = _isPushing;
    float directionLength = glm::length(direction);
    _isPushing = directionLength > EPSILON;

    // normalize direction
    if (_isPushing) {
        direction /= directionLength;
    } else {
        direction = Vectors::ZERO;
    }

    if (state == CharacterController::State::Hover) {
        // we're flying --> complex acceleration curve that builds on top of current motor speed and caps at some max speed
        float motorSpeed = glm::length(_actionMotorVelocity);
        float finalMaxMotorSpeed = getUniformScale() * MAX_ACTION_MOTOR_SPEED;
        float speedGrowthTimescale  = 2.0f;
        float speedIncreaseFactor = 1.8f;
        motorSpeed *= 1.0f + glm::clamp(deltaTime / speedGrowthTimescale , 0.0f, 1.0f) * speedIncreaseFactor;
        const float maxBoostSpeed = getUniformScale() * MAX_BOOST_SPEED;

        if (_isPushing) {
            if (motorSpeed < maxBoostSpeed) {
                // an active action motor should never be slower than this
                float boostCoefficient = (maxBoostSpeed - motorSpeed) / maxBoostSpeed;
                motorSpeed += MIN_AVATAR_SPEED * boostCoefficient;
            } else if (motorSpeed > finalMaxMotorSpeed) {
                motorSpeed = finalMaxMotorSpeed;
            }
        }
        _actionMotorVelocity = motorSpeed * direction;
    } else {
        // we're interacting with a floor --> simple horizontal speed and exponential decay
        _actionMotorVelocity = MAX_WALKING_SPEED * direction;
    }

    float boomChange = _driveKeys[ZOOM];
    _boomLength += 2.0f * _boomLength * boomChange + boomChange * boomChange;
    _boomLength = glm::clamp<float>(_boomLength, ZOOM_MIN, ZOOM_MAX);
}

void MyAvatar::updatePosition(float deltaTime) {
    if (_motionBehaviors & AVATAR_MOTION_ACTION_MOTOR_ENABLED) {
        updateActionMotor(deltaTime);
    }

    vec3 velocity = getVelocity();
    const float MOVING_SPEED_THRESHOLD_SQUARED = 0.0001f; // 0.01 m/s
    if (!_characterController.isEnabledAndReady()) {
        // _characterController is not in physics simulation but it can still compute its target velocity
        updateMotors();
        _characterController.computeNewVelocity(deltaTime, velocity);

        float speed2 = glm::length2(velocity);
        if (speed2 > MIN_AVATAR_SPEED_SQUARED) {
            // update position ourselves
            applyPositionDelta(deltaTime * velocity);
        }
        measureMotionDerivatives(deltaTime);
        _moving = speed2 > MOVING_SPEED_THRESHOLD_SQUARED;
    } else {
        // physics physics simulation updated elsewhere
        float speed2 = glm::length2(velocity);
        _moving = speed2 > MOVING_SPEED_THRESHOLD_SQUARED;
    }

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
        glm::quat quatOrientation = cancelOutRollAndPitch(newOrientation);

        if (shouldFaceLocation) {
            quatOrientation = newOrientation * glm::angleAxis(PI, Vectors::UP);

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
    if (menu->isOptionChecked(MenuOption::ActionMotorControl)) {
        _motionBehaviors |= AVATAR_MOTION_ACTION_MOTOR_ENABLED;
    } else {
        _motionBehaviors &= ~AVATAR_MOTION_ACTION_MOTOR_ENABLED;
    }
    if (menu->isOptionChecked(MenuOption::ScriptedMotorControl)) {
        _motionBehaviors |= AVATAR_MOTION_SCRIPTED_MOTOR_ENABLED;
    } else {
        _motionBehaviors &= ~AVATAR_MOTION_SCRIPTED_MOTOR_ENABLED;
    }

    setAvatarCollisionsEnabled(menu->isOptionChecked(MenuOption::EnableAvatarCollisions));
}

void MyAvatar::setAvatarCollisionsEnabled(bool enabled) {

    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setAvatarCollisionsEnabled", Q_ARG(bool, enabled));
        return;
    }

    bool ghostingAllowed = true;
    EntityTreeRenderer* entityTreeRenderer = qApp->getEntities();
    if (entityTreeRenderer) {
        std::shared_ptr<ZoneEntityItem> zone = entityTreeRenderer->myAvatarZone();
        if (zone) {
            ghostingAllowed = zone->getGhostingAllowed();
        }
    }
    int16_t group = enabled || !ghostingAllowed ? BULLET_COLLISION_GROUP_MY_AVATAR : BULLET_COLLISION_GROUP_COLLISIONLESS;
    _characterController.setCollisionGroup(group);
}

bool MyAvatar::getAvatarCollisionsEnabled() {
    return _characterController.getCollisionGroup() != BULLET_COLLISION_GROUP_COLLISIONLESS;
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

MyAvatar::FollowHelper::FollowHelper() {
    deactivate();
}

void MyAvatar::FollowHelper::deactivate() {
    _activeBits = 0;
}

void MyAvatar::FollowHelper::deactivate(FollowType type) {
    assert(type >= 0 && type < NumFollowTypes);
    _activeBits &= ~(uint8_t)(0x01 << (int)type);
}

void MyAvatar::FollowHelper::activate(FollowType type) {
    assert(type >= 0 && type < NumFollowTypes);
    _activeBits |= (uint8_t)(0x01 << (int)type);
}

bool MyAvatar::FollowHelper::isActive(FollowType type) const {
    assert(type >= 0 && type < NumFollowTypes);
    return (bool)(_activeBits & (uint8_t)(0x01 << (int)type));
}

bool MyAvatar::FollowHelper::isActive() const {
    return (bool)_activeBits;
}

void MyAvatar::FollowHelper::updateRotationActivation(const MyAvatar& myAvatar, const glm::mat4& desiredBodyMatrix, const glm::mat4& currentBodyMatrix) {
    auto cameraMode = qApp->getCamera()->getMode();
    if (cameraMode == CAMERA_MODE_THIRD_PERSON) {
        deactivate(Rotation);
    } else {
        const float FOLLOW_ROTATION_THRESHOLD = cosf(PI / 6.0f); // 30 degrees
        const float STOP_FOLLOW_ROTATION_THRESHOLD = 0.99f;
        glm::vec2 bodyFacing = getFacingDir2D(currentBodyMatrix);
        if (isActive(Rotation)) {
            if (glm::dot(myAvatar.getHMDSensorFacingMovingAverage(), bodyFacing) > STOP_FOLLOW_ROTATION_THRESHOLD) {
                deactivate(Rotation);
            }
        } else if (glm::dot(myAvatar.getHMDSensorFacingMovingAverage(), bodyFacing) < FOLLOW_ROTATION_THRESHOLD) {
            activate(Rotation);
        }
    }
}

void MyAvatar::FollowHelper::updateHorizontalActivation(const MyAvatar& myAvatar, const glm::mat4& desiredBodyMatrix, const glm::mat4& currentBodyMatrix) {

    // -z axis of currentBodyMatrix in world space.
    glm::vec3 forward = glm::normalize(glm::vec3(-currentBodyMatrix[0][2], -currentBodyMatrix[1][2], -currentBodyMatrix[2][2]));
    // x axis of currentBodyMatrix in world space.
    glm::vec3 right = glm::normalize(glm::vec3(currentBodyMatrix[0][0], currentBodyMatrix[1][0], currentBodyMatrix[2][0]));
    glm::vec3 offset = extractTranslation(desiredBodyMatrix) - extractTranslation(currentBodyMatrix);

    float forwardLeanAmount = glm::dot(forward, offset);
    float lateralLeanAmount = fabsf(glm::dot(right, offset));

    const float MAX_LATERAL_LEAN = 0.3f;
    const float MAX_FORWARD_LEAN = 0.15f;
    const float MAX_BACKWARD_LEAN = 0.1f;
    const float MIN_LEAN = 0.02f;

    if (isActive(Horizontal)) {
        if (fabsf(forwardLeanAmount) < MIN_LEAN && lateralLeanAmount < MIN_LEAN) {
            deactivate(Horizontal);
        }
    } else {
        if (forwardLeanAmount > MAX_FORWARD_LEAN ||
                forwardLeanAmount < -MAX_BACKWARD_LEAN ||
                lateralLeanAmount > MAX_LATERAL_LEAN) {
            activate(Horizontal);
        }
    }
}

void MyAvatar::FollowHelper::updateVerticalActivation(const MyAvatar& myAvatar, const glm::mat4& desiredBodyMatrix, const glm::mat4& currentBodyMatrix) {
    const float CYLINDER_TOP = 0.1f;
    const float CYLINDER_BOTTOM = -1.5f;
    const float MIN_VERTICAL_OFFSET = 0.02f;

    glm::vec3 offset = extractTranslation(desiredBodyMatrix) - extractTranslation(currentBodyMatrix);
    if (isActive(Vertical)) {
        if (fabsf(offset.y) < MIN_VERTICAL_OFFSET) {
            deactivate(Vertical);
        }
    } else if (offset.y > CYLINDER_TOP || offset.y < CYLINDER_BOTTOM) {
        activate(Vertical);
    }
}

void MyAvatar::FollowHelper::prePhysicsUpdate(MyAvatar& myAvatar, const glm::mat4& desiredBodyMatrix, const glm::mat4& currentBodyMatrix) {
    _desiredBodyMatrix = desiredBodyMatrix;

    if (myAvatar.getHMDLeanRecenterEnabled()) {

        if (_isOutOfBody) {

            glm::mat4 desiredWorldMatrix = myAvatar.getSensorToWorldMatrix() * _desiredBodyMatrix;
            glm::mat4 currentWorldMatrix = createMatFromQuatAndPos(myAvatar.getOrientation(), myAvatar.getPosition());
            AnimPose followWorldPose(currentWorldMatrix);

            // OUTOFBODY_HACK, only takes horizontal movement into account.

            // horizontal follow
            glm::vec3 desiredTranslation = extractTranslation(desiredWorldMatrix);
            followWorldPose.trans.x = desiredTranslation.x;
            followWorldPose.trans.z = desiredTranslation.z;

            // rotation follow
            // face the HMD
            glm::vec3 hmdWorldPosition = extractTranslation(myAvatar.getSensorToWorldMatrix() * myAvatar.getHMDSensorMatrix());
            glm::vec3 facing = myAvatar.getPosition() - hmdWorldPosition;
            facing.y = 0.0f;
            if (glm::length(facing) > EPSILON) {
                // turn to face the hmd
                followWorldPose.rot = glm::angleAxis(atan2(facing.x, facing.z), Vectors::UNIT_Y);
            } else {
                followWorldPose.rot = glmExtractRotation(desiredWorldMatrix);
            }

            myAvatar.getCharacterController()->setFollowParameters(followWorldPose);

        } else {
            updateRotationActivation(myAvatar, desiredBodyMatrix, currentBodyMatrix);
            updateHorizontalActivation(myAvatar, desiredBodyMatrix, currentBodyMatrix);
            updateVerticalActivation(myAvatar, desiredBodyMatrix, currentBodyMatrix);

            glm::mat4 desiredWorldMatrix = myAvatar.getSensorToWorldMatrix() * _desiredBodyMatrix;
            glm::mat4 currentWorldMatrix = createMatFromQuatAndPos(myAvatar.getOrientation(), myAvatar.getPosition());

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

            if (isActive()) {
                myAvatar.getCharacterController()->setFollowParameters(followWorldPose);
            } else {
                myAvatar.getCharacterController()->disableFollow();
            }

            glm::mat4 currentWorldMatrixY180 = createMatFromQuatAndPos(myAvatar.getOrientation() * Quaternions::Y_180, myAvatar.getPosition());
            _prevInBodyHMDMatInAvatarSpace = _inBodyHMDMatInAvatarSpace;
            _inBodyHMDMatInAvatarSpace = glm::inverse(currentWorldMatrixY180) * myAvatar.getSensorToWorldMatrix() * myAvatar.getHMDSensorMatrix();
        }
    } else {
        deactivate();
        myAvatar.getCharacterController()->disableFollow();
    }
}

void MyAvatar::FollowHelper::postPhysicsUpdate(MyAvatar& myAvatar) {

    // get HMD position from sensor space into world space, and back into rig space
    glm::mat4 worldHMDMat = myAvatar.getSensorToWorldMatrix() * myAvatar.getHMDSensorMatrix();
    glm::mat4 rigToWorld = createMatFromQuatAndPos(myAvatar.getRotation() * Quaternions::Y_180, myAvatar.getPosition());
    glm::mat4 worldToRig = glm::inverse(rigToWorld);
    glm::mat4 rigHMDMat = worldToRig * worldHMDMat;
    glm::vec3 rigHMDPosition = extractTranslation(rigHMDMat);

    // detect if the rig head position is too far from the avatar's position.
    _isOutOfBody = !pointIsInsideCapsule(rigHMDPosition, TRUNCATE_IK_CAPSULE_POSITION, TRUNCATE_IK_CAPSULE_LENGTH, TRUNCATE_IK_CAPSULE_RADIUS);
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

bool MyAvatar::hasDriveInput() const {
    return fabsf(_driveKeys[TRANSLATE_X]) > 0.0f || fabsf(_driveKeys[TRANSLATE_Y]) > 0.0f || fabsf(_driveKeys[TRANSLATE_Z]) > 0.0f;
}

glm::quat MyAvatar::getAbsoluteJointRotationInObjectFrame(int index) const {
    switch(index) {
        case CONTROLLER_LEFTHAND_INDEX: {
            return getLeftHandControllerPoseInAvatarFrame().getRotation();
        }
        case CONTROLLER_RIGHTHAND_INDEX: {
            return getRightHandControllerPoseInAvatarFrame().getRotation();
        }
        default: {
            return Avatar::getAbsoluteJointRotationInObjectFrame(index);
        }
    }
}

glm::vec3 MyAvatar::getAbsoluteJointTranslationInObjectFrame(int index) const {
    switch(index) {
        case CONTROLLER_LEFTHAND_INDEX: {
            return getLeftHandControllerPoseInAvatarFrame().getTranslation();
        }
        case CONTROLLER_RIGHTHAND_INDEX: {
            return getRightHandControllerPoseInAvatarFrame().getTranslation();
        }
        default: {
            return Avatar::getAbsoluteJointTranslationInObjectFrame(index);
        }
    }
}
