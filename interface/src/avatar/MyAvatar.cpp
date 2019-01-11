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

#include "MyAvatar.h"

#include <algorithm>
#include <vector>

#include <QBuffer>

#include <glm/gtx/norm.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <QtCore/QTimer>

#include <shared/QtHelpers.h>
#include <scripting/HMDScriptingInterface.h>
#include <AccountManager.h>
#include <AddressManager.h>
#include <AudioClient.h>
#include <ClientTraitsHandler.h>
#include <display-plugins/DisplayPlugin.h>
#include <FSTReader.h>
#include <GeometryUtil.h>
#include <NodeList.h>
#include <udt/PacketHeaders.h>
#include <PathUtils.h>
#include <PerfStat.h>
#include <SharedUtil.h>
#include <SoundCache.h>
#include <ModelEntityItem.h>
#include <GLMHelpers.h>
#include <TextRenderer3D.h>
#include <UserActivityLogger.h>
#include <AnimDebugDraw.h>
#include <AnimClip.h>
#include <AnimInverseKinematics.h>
#include <recording/Deck.h>
#include <recording/Recorder.h>
#include <recording/Clip.h>
#include <recording/Frame.h>
#include <RecordingScriptingInterface.h>
#include <trackers/FaceTracker.h>
#include <RenderableModelEntityItem.h>
#include <VariantMapToScriptValue.h>

#include "MyHead.h"
#include "MySkeletonModel.h"
#include "AnimUtil.h"
#include "Application.h"
#include "AvatarManager.h"
#include "AvatarActionHold.h"
#include "Menu.h"
#include "Util.h"
#include "InterfaceLogging.h"
#include "DebugDraw.h"
#include "EntityEditPacketSender.h"
#include "MovingEntitiesOperator.h"
#include "SceneScriptingInterface.h"

using namespace std;

const float DEFAULT_REAL_WORLD_FIELD_OF_VIEW_DEGREES = 30.0f;

const float YAW_SPEED_DEFAULT = 100.0f;   // degrees/sec
const float PITCH_SPEED_DEFAULT = 75.0f; // degrees/sec

const float MAX_BOOST_SPEED = 0.5f * DEFAULT_AVATAR_MAX_WALKING_SPEED; // action motor gets additive boost below this speed
const float MIN_AVATAR_SPEED = 0.05f;
const float MIN_AVATAR_SPEED_SQUARED = MIN_AVATAR_SPEED * MIN_AVATAR_SPEED; // speed is set to zero below this

float MIN_SCRIPTED_MOTOR_TIMESCALE = 0.005f;
float DEFAULT_SCRIPTED_MOTOR_TIMESCALE = 1.0e6f;
const int SCRIPTED_MOTOR_CAMERA_FRAME = 0;
const int SCRIPTED_MOTOR_AVATAR_FRAME = 1;
const int SCRIPTED_MOTOR_WORLD_FRAME = 2;
const int SCRIPTED_MOTOR_SIMPLE_MODE = 0;
const int SCRIPTED_MOTOR_DYNAMIC_MODE = 1;
const QString& DEFAULT_AVATAR_COLLISION_SOUND_URL = "https://hifi-public.s3.amazonaws.com/sounds/Collisions-otherorganic/Body_Hits_Impact.wav";

const float MyAvatar::ZOOM_MIN = 0.5f;
const float MyAvatar::ZOOM_MAX = 25.0f;
const float MyAvatar::ZOOM_DEFAULT = 1.5f;
const float MIN_SCALE_CHANGED_DELTA = 0.001f;
const int MODE_READINGS_RING_BUFFER_SIZE = 500;
const float CENTIMETERS_PER_METER = 100.0f;

const QString AVATAR_SETTINGS_GROUP_NAME { "Avatar" };

MyAvatar::MyAvatar(QThread* thread) :
    Avatar(thread),
    _yawSpeed(YAW_SPEED_DEFAULT),
    _pitchSpeed(PITCH_SPEED_DEFAULT),
    _scriptedMotorTimescale(DEFAULT_SCRIPTED_MOTOR_TIMESCALE),
    _scriptedMotorFrame(SCRIPTED_MOTOR_CAMERA_FRAME),
    _scriptedMotorMode(SCRIPTED_MOTOR_SIMPLE_MODE),
    _motionBehaviors(AVATAR_MOTION_DEFAULTS),
    _characterController(this),
    _eyeContactTarget(LEFT_EYE),
    _realWorldFieldOfView("realWorldFieldOfView",
                          DEFAULT_REAL_WORLD_FIELD_OF_VIEW_DEGREES),
    _useAdvancedMovementControls("advancedMovementForHandControllersIsChecked", true),
    _showPlayArea("showPlayArea", true),
    _smoothOrientationTimer(std::numeric_limits<float>::max()),
    _smoothOrientationInitial(),
    _smoothOrientationTarget(),
    _hmdSensorMatrix(),
    _hmdSensorOrientation(),
    _hmdSensorPosition(),
    _recentModeReadings(MODE_READINGS_RING_BUFFER_SIZE),
    _bodySensorMatrix(),
    _goToPending(false),
    _goToSafe(true),
    _goToFeetAjustment(false),
    _goToPosition(),
    _goToOrientation(),
    _prevShouldDrawHead(true),
    _audioListenerMode(FROM_HEAD),
    _dominantHandSetting(QStringList() << AVATAR_SETTINGS_GROUP_NAME << "dominantHand", DOMINANT_RIGHT_HAND),
    _headPitchSetting(QStringList() << AVATAR_SETTINGS_GROUP_NAME << "", 0.0f),
    _scaleSetting(QStringList() << AVATAR_SETTINGS_GROUP_NAME << "scale", _targetScale),
    _yawSpeedSetting(QStringList() << AVATAR_SETTINGS_GROUP_NAME << "yawSpeed", _yawSpeed),
    _pitchSpeedSetting(QStringList() << AVATAR_SETTINGS_GROUP_NAME << "pitchSpeed", _pitchSpeed),
    _fullAvatarURLSetting(QStringList() << AVATAR_SETTINGS_GROUP_NAME << "fullAvatarURL",
                          AvatarData::defaultFullAvatarModelUrl()),
    _fullAvatarModelNameSetting(QStringList() << AVATAR_SETTINGS_GROUP_NAME << "fullAvatarModelName", _fullAvatarModelName),
    _animGraphURLSetting(QStringList() << AVATAR_SETTINGS_GROUP_NAME << "animGraphURL", QUrl("")),
    _displayNameSetting(QStringList() << AVATAR_SETTINGS_GROUP_NAME << "displayName", ""),
    _collisionSoundURLSetting(QStringList() << AVATAR_SETTINGS_GROUP_NAME << "collisionSoundURL", QUrl(_collisionSoundURL)),
    _useSnapTurnSetting(QStringList() << AVATAR_SETTINGS_GROUP_NAME << "useSnapTurn", _useSnapTurn),
    _userHeightSetting(QStringList() << AVATAR_SETTINGS_GROUP_NAME << "userHeight", DEFAULT_AVATAR_HEIGHT),
    _flyingHMDSetting(QStringList() << AVATAR_SETTINGS_GROUP_NAME << "flyingHMD", _flyingPrefHMD),
    _avatarEntityCountSetting(QStringList() << AVATAR_SETTINGS_GROUP_NAME << "avatarEntityData" << "size", 0)
{
    _clientTraitsHandler.reset(new ClientTraitsHandler(this));

    // give the pointer to our head to inherited _headData variable from AvatarData
    _headData = new MyHead(this);

    _skeletonModel = std::make_shared<MySkeletonModel>(this, nullptr);
    _skeletonModel->setLoadingPriority(MYAVATAR_LOADING_PRIORITY);
    connect(_skeletonModel.get(), &Model::setURLFinished, this, &Avatar::setModelURLFinished);
    connect(_skeletonModel.get(), &Model::setURLFinished, this, [this](bool success) {
        if (success) {
            qApp->unloadAvatarScripts();
            _shouldLoadScripts = true;
        }
    });
    connect(_skeletonModel.get(), &Model::rigReady, this, [this]() {
        if (_shouldLoadScripts) {
            auto hfmModel = getSkeletonModel()->getHFMModel();
            qApp->loadAvatarScripts(hfmModel.scripts);
            _shouldLoadScripts = false;
        }
                // Load and convert old attachments to avatar entities
        if (_oldAttachmentData.size() > 0) {
            setAttachmentData(_oldAttachmentData);
            _oldAttachmentData.clear();
            _attachmentData.clear();
        }
    });
    connect(_skeletonModel.get(), &Model::rigReady, this, &Avatar::rigReady);
    connect(_skeletonModel.get(), &Model::rigReset, this, &Avatar::rigReset);
    connect(&_skeletonModel->getRig(), &Rig::onLoadComplete, this, &MyAvatar::updateCollisionCapsuleCache);
    connect(this, &MyAvatar::sensorToWorldScaleChanged, this, &MyAvatar::updateCollisionCapsuleCache);
    using namespace recording;
    _skeletonModel->flagAsCauterized();

    clearDriveKeys();

    // Necessary to select the correct slot
    using SlotType = void(MyAvatar::*)(const glm::vec3&, bool, const glm::quat&, bool);

    // connect to AddressManager signal for location jumps
    connect(DependencyManager::get<AddressManager>().data(), &AddressManager::locationChangeRequired,
            this, static_cast<SlotType>(&MyAvatar::goToFeetLocation));

    // handle scale constraints imposed on us by the domain-server
    auto& domainHandler = DependencyManager::get<NodeList>()->getDomainHandler();

    // when we connect to a domain and retrieve its settings, we restrict our max/min scale based on those settings
    connect(&domainHandler, &DomainHandler::settingsReceived, this, &MyAvatar::restrictScaleFromDomainSettings);

    // when we leave a domain we lift whatever restrictions that domain may have placed on our scale
    connect(&domainHandler, &DomainHandler::disconnectedFromDomain, this, &MyAvatar::leaveDomain);

    _bodySensorMatrix = deriveBodyFromHMDSensor();

    using namespace recording;

    auto player = DependencyManager::get<Deck>();
    auto recorder = DependencyManager::get<Recorder>();
    connect(player.data(), &Deck::playbackStateChanged, [=] {
        bool isPlaying = player->isPlaying();
        if (isPlaying) {
            auto recordingInterface = DependencyManager::get<RecordingScriptingInterface>();
            if (recordingInterface->getPlayFromCurrentLocation()) {
                setRecordingBasis();
            }
            _previousCollisionGroup = _characterController.computeCollisionGroup();
            _characterController.setCollisionless(true);
        } else {
            clearRecordingBasis();
            useFullAvatarURL(_fullAvatarURLFromPreferences, _fullAvatarModelName);
            if (_previousCollisionGroup != BULLET_COLLISION_GROUP_COLLISIONLESS) {
                _characterController.setCollisionless(false);
            }
        }

        auto audioIO = DependencyManager::get<AudioClient>();
        audioIO->setIsPlayingBackRecording(isPlaying);

        _skeletonModel->getRig().setEnableAnimations(!isPlaying);
    });

    connect(recorder.data(), &Recorder::recordingStateChanged, [=] {
        if (recorder->isRecording()) {
            createRecordingIDs();
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

            setSkeletonModelURL(dummyAvatar.getSkeletonModelURL());
        }

        if (recordingInterface->getPlayerUseDisplayName() && dummyAvatar.getDisplayName() != getDisplayName()) {
            setDisplayName(dummyAvatar.getDisplayName());
        }

        setWorldPosition(dummyAvatar.getWorldPosition());
        setWorldOrientation(dummyAvatar.getWorldOrientation());

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

        auto jointData = dummyAvatar.getRawJointData();
        if (jointData.length() > 0) {
            _skeletonModel->getRig().copyJointsFromJointData(jointData);
        }
    });

    connect(&(_skeletonModel->getRig()), SIGNAL(onLoadComplete()), this, SIGNAL(onLoadComplete()));

    _characterController.setDensity(_density);
}

MyAvatar::~MyAvatar() {
    _lookAtTargetAvatar.reset();
    delete _myScriptEngine;
    _myScriptEngine = nullptr;
}

void MyAvatar::setDominantHand(const QString& hand) {
    if (hand == DOMINANT_LEFT_HAND || hand == DOMINANT_RIGHT_HAND) {
        _dominantHand = hand;
        emit dominantHandChanged(_dominantHand);
    }
}

void MyAvatar::requestDisableHandTouch() {
    std::lock_guard<std::mutex> guard(_disableHandTouchMutex);
    _disableHandTouchCount++;
    emit shouldDisableHandTouchChanged(_disableHandTouchCount > 0);
}

void MyAvatar::requestEnableHandTouch() {
    std::lock_guard<std::mutex> guard(_disableHandTouchMutex);
    _disableHandTouchCount = std::max(_disableHandTouchCount - 1, 0);
    emit shouldDisableHandTouchChanged(_disableHandTouchCount > 0);
}

void MyAvatar::disableHandTouchForID(const QUuid& entityID) {
    emit disableHandTouchForIDChanged(entityID, true);
}

void MyAvatar::enableHandTouchForID(const QUuid& entityID) {
    emit disableHandTouchForIDChanged(entityID, false);
}

void MyAvatar::registerMetaTypes(ScriptEnginePointer engine) {
    QScriptValue value = engine->newQObject(this, QScriptEngine::QtOwnership, QScriptEngine::ExcludeDeleteLater | QScriptEngine::ExcludeChildObjects);
    engine->globalObject().setProperty("MyAvatar", value);

    QScriptValue driveKeys = engine->newObject();
    auto metaEnum = QMetaEnum::fromType<DriveKeys>();
    for (int i = 0; i < MAX_DRIVE_KEYS; ++i) {
        driveKeys.setProperty(metaEnum.key(i), metaEnum.value(i));
    }
    engine->globalObject().setProperty("DriveKeys", driveKeys);

    qScriptRegisterMetaType(engine.data(), audioListenModeToScriptValue, audioListenModeFromScriptValue);
    qScriptRegisterMetaType(engine.data(), driveKeysToScriptValue, driveKeysFromScriptValue);
}

void MyAvatar::setOrientationVar(const QVariant& newOrientationVar) {
    Avatar::setWorldOrientation(quatFromVariant(newOrientationVar));
}

QVariant MyAvatar::getOrientationVar() const {
    return quatToVariant(Avatar::getWorldOrientation());
}

glm::quat MyAvatar::getOrientationOutbound() const {
    // Allows MyAvatar to send out smoothed data to remote agents if required.
    if (_smoothOrientationTimer > SMOOTH_TIME_ORIENTATION) {
        return (getLocalOrientation());
    }

    // Smooth the remote avatar movement.
    float t = _smoothOrientationTimer / SMOOTH_TIME_ORIENTATION;
    float interp = Interpolate::easeInOutQuad(glm::clamp(t, 0.0f, 1.0f));
    return (slerp(_smoothOrientationInitial, _smoothOrientationTarget, interp));
}

// virtual
void MyAvatar::simulateAttachments(float deltaTime) {
    // don't update attachments here, do it in harvestResultsFromPhysicsSimulation()
}

QByteArray MyAvatar::toByteArrayStateful(AvatarDataDetail dataDetail, bool dropFaceTracking) {
    CameraMode mode = qApp->getCamera().getMode();
    _globalPosition = getWorldPosition();
    // This might not be right! Isn't the capsule local offset in avatar space, and don't we need to add the radius to the y as well? -HRS 5/26/17
    _globalBoundingBoxDimensions.x = _characterController.getCapsuleRadius();
    _globalBoundingBoxDimensions.y = _characterController.getCapsuleHalfHeight();
    _globalBoundingBoxDimensions.z = _characterController.getCapsuleRadius();
    _globalBoundingBoxOffset = _characterController.getCapsuleLocalOffset();
    if (mode == CAMERA_MODE_THIRD_PERSON || mode == CAMERA_MODE_INDEPENDENT) {
        // fake the avatar position that is sent up to the AvatarMixer
        glm::vec3 oldPosition = getWorldPosition();
        setWorldPosition(getSkeletonPosition());
        QByteArray array = AvatarData::toByteArrayStateful(dataDetail);
        // copy the correct position back
        setWorldPosition(oldPosition);
        return array;
    }
    return AvatarData::toByteArrayStateful(dataDetail);
}

void MyAvatar::resetSensorsAndBody() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "resetSensorsAndBody");
        return;
    }

    qApp->getActiveDisplayPlugin()->resetSensors();
    reset(true, false, true);
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
    auto worldBodyRot = glmExtractRotation(worldBodyMatrix);

    if (_characterController.getState() == CharacterController::State::Ground) {
        // the avatar's physical aspect thinks it is standing on something
        // therefore need to be careful to not "center" the body below the floor
        float downStep = glm::dot(worldBodyPos - getWorldPosition(), _worldUpDirection);
        if (downStep < -0.5f * _characterController.getCapsuleHalfHeight() + _characterController.getCapsuleRadius()) {
            worldBodyPos -= downStep * _worldUpDirection;
        }
    }

    // this will become our new position.
    setWorldPosition(worldBodyPos);
    setWorldOrientation(worldBodyRot);

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

    _skeletonModel->getRig().clearIKJointLimitHistory();
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
        auto worldBodyRot = glmExtractRotation(worldBodyMatrix);

        // this will become our new position.
        setWorldPosition(worldBodyPos);
        setWorldOrientation(worldBodyRot);

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

void MyAvatar::updateSitStandState(float newHeightReading, float dt) {
    const float STANDING_HEIGHT_MULTIPLE = 1.2f;
    const float SITTING_HEIGHT_MULTIPLE = 0.833f;
    const float SITTING_TIMEOUT = 4.0f;  // 4 seconds
    const float STANDING_TIMEOUT = 0.3333f; // 1/3 second
    const float SITTING_UPPER_BOUND = 1.52f;
    if (!getIsSitStandStateLocked()) {
        if (!getIsAway() && getControllerPoseInAvatarFrame(controller::Action::HEAD).isValid()) {
            if (getIsInSittingState()) {
                if (newHeightReading > (STANDING_HEIGHT_MULTIPLE * _tippingPoint)) {
                    // if we recenter upwards then no longer in sitting state
                    _sitStandStateTimer += dt;
                    if (_sitStandStateTimer > STANDING_TIMEOUT) {
                        _averageUserHeightSensorSpace = newHeightReading;
                        _tippingPoint = newHeightReading;
                        setIsInSittingState(false);
                    }
                } else if (newHeightReading < (SITTING_HEIGHT_MULTIPLE * _tippingPoint)) {
                    // if we are mis labelled as sitting but we are standing in the real world this will
                    // make sure that a real sit is still recognized so we won't be stuck in sitting unable to change state
                    _sitStandStateTimer += dt;
                    if (_sitStandStateTimer > SITTING_TIMEOUT) {
                        _averageUserHeightSensorSpace = newHeightReading;
                        _tippingPoint = newHeightReading;
                        // here we stay in sit state but reset the average height
                        setIsInSittingState(true);
                    }
                } else {
                    // sanity check if average height greater than 5ft they are not sitting(or get off your dangerous barstool please)
                    if (_averageUserHeightSensorSpace > SITTING_UPPER_BOUND) {
                        setIsInSittingState(false);
                    } else {
                        // tipping point is average height when sitting.
                        _tippingPoint = _averageUserHeightSensorSpace;
                        _sitStandStateTimer = 0.0f;
                    }
                }
            } else {
                // in the standing state
                if (newHeightReading < (SITTING_HEIGHT_MULTIPLE * _tippingPoint)) {
                    _sitStandStateTimer += dt;
                    if (_sitStandStateTimer > SITTING_TIMEOUT) {
                        _averageUserHeightSensorSpace = newHeightReading;
                        _tippingPoint = newHeightReading;
                        setIsInSittingState(true);
                    }
                } else {
                    // use the mode height for the tipping point when we are standing.
                    _tippingPoint = getCurrentStandingHeight();
                    _sitStandStateTimer = 0.0f;
                }
            }
        } else {
            //if you are away then reset the average and set state to standing.
            _averageUserHeightSensorSpace = _userHeight.get();
            _tippingPoint = _userHeight.get();
            setIsInSittingState(false);
        }
    }
}

void MyAvatar::update(float deltaTime) {
    // update moving average of HMD facing in xz plane.
    const float HMD_FACING_TIMESCALE = getRotationRecenterFilterLength();
    const float PERCENTAGE_WEIGHT_HEAD_VS_SHOULDERS_AZIMUTH = 0.0f; // 100 percent shoulders
    const float COSINE_THIRTY_DEGREES = 0.866f;
    const float SQUATTY_TIMEOUT = 30.0f; // 30 seconds
    const float HEIGHT_FILTER_COEFFICIENT = 0.01f;

    float tau = deltaTime / HMD_FACING_TIMESCALE;
    setHipToHandController(computeHandAzimuth());

    // put the average hand azimuth into sensor space.
    // then mix it with head facing direction to determine rotation recenter
    int spine2Index = _skeletonModel->getRig().indexOfJoint("Spine2");
    if (getControllerPoseInAvatarFrame(controller::Action::LEFT_HAND).isValid() && getControllerPoseInAvatarFrame(controller::Action::RIGHT_HAND).isValid() && !(spine2Index < 0)) {

        // use the spine for the azimuth origin.
        glm::quat spine2Rot = getAbsoluteJointRotationInObjectFrame(spine2Index);
        glm::vec3 handHipAzimuthAvatarSpace = spine2Rot * glm::vec3(_hipToHandController.x, 0.0f, _hipToHandController.y);
        glm::vec3 handHipAzimuthWorldSpace = transformVectorFast(getTransform().getMatrix(), handHipAzimuthAvatarSpace);
        glm::mat4 sensorToWorldMat = getSensorToWorldMatrix();
        glm::mat4 worldToSensorMat = glm::inverse(sensorToWorldMat);
        glm::vec3 handHipAzimuthSensorSpace = transformVectorFast(worldToSensorMat, handHipAzimuthWorldSpace);
        glm::vec2 normedHandHipAzimuthSensorSpace(0.0f, 1.0f);
        if (glm::length(glm::vec2(handHipAzimuthSensorSpace.x, handHipAzimuthSensorSpace.z)) > 0.0f) {
            normedHandHipAzimuthSensorSpace = glm::normalize(glm::vec2(handHipAzimuthSensorSpace.x, handHipAzimuthSensorSpace.z));
            glm::vec2 headFacingPlusHandHipAzimuthMix = lerp(normedHandHipAzimuthSensorSpace, _headControllerFacing, PERCENTAGE_WEIGHT_HEAD_VS_SHOULDERS_AZIMUTH);
            _headControllerFacingMovingAverage = lerp(_headControllerFacingMovingAverage, headFacingPlusHandHipAzimuthMix, tau);
        } else {
            // use head facing if the chest arms vector is up or down.
            _headControllerFacingMovingAverage = lerp(_headControllerFacingMovingAverage, _headControllerFacing, tau);
        }
    } else {
        _headControllerFacingMovingAverage = lerp(_headControllerFacingMovingAverage, _headControllerFacing, tau);
    }

    if (_smoothOrientationTimer < SMOOTH_TIME_ORIENTATION) {
        _rotationChanged = usecTimestampNow();
        _smoothOrientationTimer += deltaTime;
    }

    controller::Pose newHeightReading = getControllerPoseInSensorFrame(controller::Action::HEAD);
    if (newHeightReading.isValid()) {
        int newHeightReadingInCentimeters = glm::floor(newHeightReading.getTranslation().y * CENTIMETERS_PER_METER);
        _averageUserHeightSensorSpace = lerp(_averageUserHeightSensorSpace, newHeightReading.getTranslation().y, HEIGHT_FILTER_COEFFICIENT);
        _recentModeReadings.insert(newHeightReadingInCentimeters);
        setCurrentStandingHeight(computeStandingHeightMode(newHeightReading));
        setAverageHeadRotation(computeAverageHeadRotation(getControllerPoseInAvatarFrame(controller::Action::HEAD)));
    }

    // if the spine is straight and the head is below the default position by 5 cm then increment squatty count.
    const float SQUAT_THRESHOLD = 0.05f;
    glm::vec3 headDefaultPositionAvatarSpace = getAbsoluteDefaultJointTranslationInObjectFrame(getJointIndex("Head"));
    glm::quat spine2OrientationAvatarSpace = getAbsoluteJointRotationInObjectFrame(getJointIndex("Spine2"));
    glm::vec3 upSpine2 = spine2OrientationAvatarSpace * glm::vec3(0.0f, 1.0f, 0.0f);
    if (glm::length(upSpine2) > 0.0f) {
        upSpine2 = glm::normalize(upSpine2);
    }
    float angleSpine2 = glm::dot(upSpine2, glm::vec3(0.0f, 1.0f, 0.0f));

    if (getControllerPoseInAvatarFrame(controller::Action::HEAD).getTranslation().y < (headDefaultPositionAvatarSpace.y - SQUAT_THRESHOLD) &&
        (angleSpine2 > COSINE_THIRTY_DEGREES) &&
        (getUserRecenterModel() != MyAvatar::SitStandModelType::ForceStand)) {

        _squatTimer += deltaTime;
        if (_squatTimer > SQUATTY_TIMEOUT) {
            _squatTimer = 0.0f;
            _follow._squatDetected = true;
        }
    } else {
        _squatTimer = 0.0f;
    }

    // put update sit stand state counts here
    updateSitStandState(newHeightReading.getTranslation().y, deltaTime);

    if (_drawAverageFacingEnabled) {
        auto sensorHeadPose = getControllerPoseInSensorFrame(controller::Action::HEAD);
        glm::vec3 worldHeadPos = transformPoint(getSensorToWorldMatrix(), sensorHeadPose.getTranslation());
        glm::vec3 worldFacingAverage = transformVectorFast(getSensorToWorldMatrix(), glm::vec3(_headControllerFacingMovingAverage.x, 0.0f, _headControllerFacingMovingAverage.y));
        glm::vec3 worldFacing = transformVectorFast(getSensorToWorldMatrix(), glm::vec3(_headControllerFacing.x, 0.0f, _headControllerFacing.y));
        DebugDraw::getInstance().drawRay(worldHeadPos, worldHeadPos + worldFacing, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
        DebugDraw::getInstance().drawRay(worldHeadPos, worldHeadPos + worldFacingAverage, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));

        // draw hand azimuth vector
        glm::vec3 handAzimuthMidpoint = transformPoint(getTransform().getMatrix(), glm::vec3(_hipToHandController.x, 0.0f, _hipToHandController.y));
        DebugDraw::getInstance().drawRay(getWorldPosition(), handAzimuthMidpoint, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
    }

    if (_goToPending) {
        setWorldPosition(_goToPosition);
        setWorldOrientation(_goToOrientation);
        _headControllerFacingMovingAverage = _headControllerFacing; // reset moving average
        _goToPending = false;
        // updateFromHMDSensorMatrix (called from paintGL) expects that the sensorToWorldMatrix is updated for any position changes
        // that happen between render and Application::update (which calls updateSensorToWorldMatrix to do so).
        // However, render/MyAvatar::update/Application::update don't always match (e.g., when using the separate avatar update thread),
        // so we update now. It's ok if it updates again in the normal way.
        updateSensorToWorldMatrix();
        emit positionGoneTo();
        // Run safety tests as soon as we can after goToLocation, or clear if we're not colliding.
        _physicsSafetyPending = getCollisionsEnabled();
        _characterController.recomputeFlying(); // In case we've gone to into the sky.
    }
    if (_goToFeetAjustment && _skeletonModelLoaded) {
        auto feetAjustment = getWorldPosition() - getWorldFeetPosition();
        _goToPosition = getWorldPosition() + feetAjustment;
        setWorldPosition(_goToPosition);
        _goToFeetAjustment = false;
    }
    if (_physicsSafetyPending && qApp->isPhysicsEnabled() && _characterController.isEnabledAndReady()) {
        // When needed and ready, arrange to check and fix.
        _physicsSafetyPending = false;
        if (_goToSafe) {
            safeLanding(_goToPosition); // no-op if already safe
        }
    }

    Head* head = getHead();
    head->relax(deltaTime);
    updateFromTrackers(deltaTime);

    if (getIsInWalkingState() && glm::length(getControllerPoseInAvatarFrame(controller::Action::HEAD).getVelocity()) < DEFAULT_AVATAR_WALK_SPEED_THRESHOLD) {
        setIsInWalkingState(false);
    }

    //  Get audio loudness data from audio input device
    // Also get the AudioClient so we can update the avatar bounding box data
    // on the AudioClient side.
    auto audio = DependencyManager::get<AudioClient>().data();
    setAudioLoudness(audio->getLastInputLoudness());
    setAudioAverageLoudness(audio->getAudioAverageInputLoudness());

    glm::vec3 halfBoundingBoxDimensions(_characterController.getCapsuleRadius(), _characterController.getCapsuleHalfHeight(), _characterController.getCapsuleRadius());
    // This might not be right! Isn't the capsule local offset in avatar space? -HRS 5/26/17
    halfBoundingBoxDimensions += _characterController.getCapsuleLocalOffset();
    QMetaObject::invokeMethod(audio, "setAvatarBoundingBoxParameters",
        Q_ARG(glm::vec3, (getWorldPosition() - halfBoundingBoxDimensions)),
        Q_ARG(glm::vec3, (halfBoundingBoxDimensions*2.0f)));

    if (getIdentityDataChanged()) {
        sendIdentityPacket();
    }

    _clientTraitsHandler->sendChangedTraitsToMixer();

    simulate(deltaTime, true);

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

void MyAvatar::beParentOfChild(SpatiallyNestablePointer newChild) const {
    _cauterizationNeedsUpdate = true;
    SpatiallyNestable::beParentOfChild(newChild);
}

void MyAvatar::forgetChild(SpatiallyNestablePointer newChild) const {
    _cauterizationNeedsUpdate = true;
    SpatiallyNestable::forgetChild(newChild);
}

void MyAvatar::recalculateChildCauterization() const {
    _cauterizationNeedsUpdate = true;
}

bool MyAvatar::isFollowActive(FollowHelper::FollowType followType) const {
    return _follow.isActive(followType);
}

void MyAvatar::updateChildCauterization(SpatiallyNestablePointer object, bool cauterize) {
    if (object->getNestableType() == NestableType::Entity) {
        EntityItemPointer entity = std::static_pointer_cast<EntityItem>(object);
        entity->setCauterized(cauterize);
    }
}

void MyAvatar::simulate(float deltaTime, bool inView) {
    PerformanceTimer perfTimer("simulate");
    animateScaleChanges(deltaTime);

    setFlyingEnabled(getFlyingEnabled());

    if (_cauterizationNeedsUpdate) {
        _cauterizationNeedsUpdate = false;

        // Redisplay cauterized entities that are no longer children of the avatar.
        auto cauterizedChild = _cauterizedChildrenOfHead.begin();
        if (cauterizedChild != _cauterizedChildrenOfHead.end()) {
            auto children = getChildren();
            while (cauterizedChild != _cauterizedChildrenOfHead.end()) {
                if (!children.contains(*cauterizedChild)) {
                    updateChildCauterization(*cauterizedChild, false);
                    cauterizedChild = _cauterizedChildrenOfHead.erase(cauterizedChild);
                } else {
                    ++cauterizedChild;
                }
            }
        }

        // Update cauterization of entities that are children of the avatar.
        auto headBoneSet = _skeletonModel->getCauterizeBoneSet();
        forEachChild([&](SpatiallyNestablePointer object) {
            bool isChildOfHead = headBoneSet.find(object->getParentJointIndex()) != headBoneSet.end();
            if (isChildOfHead) {
                // Cauterize or display children of head per head drawing state.
                updateChildCauterization(object, !_prevShouldDrawHead);
                object->forEachDescendant([&](SpatiallyNestablePointer descendant) {
                    updateChildCauterization(descendant, !_prevShouldDrawHead);
                });
                _cauterizedChildrenOfHead.insert(object);
            } else if (_cauterizedChildrenOfHead.find(object) != _cauterizedChildrenOfHead.end()) {
                // Redisplay cauterized children that are not longer children of the head.
                updateChildCauterization(object, false);
                object->forEachDescendant([&](SpatiallyNestablePointer descendant) {
                    updateChildCauterization(descendant, false);
                });
                _cauterizedChildrenOfHead.erase(object);
            }
        });
    }

    {
        PerformanceTimer perfTimer("transform");
        bool stepAction = false;
        // When there are no step values, we zero out the last step pulse.
        // This allows a user to do faster snapping by tapping a control
        for (int i = STEP_TRANSLATE_X; !stepAction && i <= STEP_YAW; ++i) {
            if (getDriveKey((DriveKeys)i) != 0.0f) {
                stepAction = true;
            }
        }

        updateOrientation(deltaTime);
        updatePosition(deltaTime);
    }

    // update sensorToWorldMatrix for camera and hand controllers
    // before we perform rig animations and IK.
    updateSensorToWorldMatrix();

    {
        PerformanceTimer perfTimer("skeleton");

        _skeletonModel->getRig().setEnableDebugDrawIKTargets(_enableDebugDrawIKTargets);
        _skeletonModel->getRig().setEnableDebugDrawIKConstraints(_enableDebugDrawIKConstraints);
        _skeletonModel->getRig().setEnableDebugDrawIKChains(_enableDebugDrawIKChains);
        _skeletonModel->simulate(deltaTime);
    }

    // we've achived our final adjusted position and rotation for the avatar
    // and all of its joints, now update our attachements.
    Avatar::simulateAttachments(deltaTime);
    relayJointDataToChildren();
    updateGrabs();

    if (!_skeletonModel->hasSkeleton()) {
        // All the simulation that can be done has been done
        getHead()->setPosition(getWorldPosition()); // so audio-position isn't 0,0,0
        return;
    }

    {
        PerformanceTimer perfTimer("joints");
        // copy out the skeleton joints from the model
        if (_rigEnabled) {
            QWriteLocker writeLock(&_jointDataLock);
            _skeletonModel->getRig().copyJointsIntoJointData(_jointData);
        }
    }

    {
        PerformanceTimer perfTimer("head");
        Head* head = getHead();
        glm::vec3 headPosition;
        if (!_skeletonModel->getHeadPosition(headPosition)) {
            headPosition = getWorldPosition();
        }

        if (isNaN(headPosition)) {
            qCDebug(interfaceapp) << "MyAvatar::simulate headPosition is NaN";
            headPosition = glm::vec3(0.0f);
        }

        head->setPosition(headPosition);
        head->setScale(getModelScale());
        head->simulate(deltaTime);
    }

    // Record avatars movements.
    auto recorder = DependencyManager::get<recording::Recorder>();
    if (recorder->isRecording()) {
        static const recording::FrameType FRAME_TYPE = recording::Frame::registerFrameType(AvatarData::FRAME_NAME);
        recorder->recordFrame(FRAME_TYPE, toFrame(*this));
    }

    locationChanged();
    // if a entity-child of this avatar has moved outside of its queryAACube, update the cube and tell the entity server.
    auto entityTreeRenderer = qApp->getEntities();
    EntityTreePointer entityTree = entityTreeRenderer ? entityTreeRenderer->getTree() : nullptr;
    if (entityTree) {
        bool zoneAllowsFlying = true;
        bool collisionlessAllowed = true;
        entityTree->withWriteLock([&] {
            std::shared_ptr<ZoneEntityItem> zone = entityTreeRenderer->myAvatarZone();
            if (zone) {
                zoneAllowsFlying = zone->getFlyingAllowed();
                collisionlessAllowed = zone->getGhostingAllowed();
            }
            EntityEditPacketSender* packetSender = qApp->getEntityEditPacketSender();
            bool force = false;
            bool iShouldTellServer = true;
            forEachDescendant([&](SpatiallyNestablePointer object) {
                entityTree->updateEntityQueryAACube(object, packetSender, force, iShouldTellServer);
            });
        });
        bool isPhysicsEnabled = qApp->isPhysicsEnabled();
        _characterController.setFlyingAllowed((zoneAllowsFlying && _enableFlying) || !isPhysicsEnabled);
        _characterController.setCollisionlessAllowed(collisionlessAllowed);
    }

    handleChangedAvatarEntityData();

    updateFadingStatus();
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

    if (newHmdSensorPosition != getHMDSensorPosition() &&
        glm::length(newHmdSensorPosition) > MAX_HMD_ORIGIN_DISTANCE) {
        qWarning() << "Invalid HMD sensor position " << newHmdSensorPosition;
        // Ignore unreasonable HMD sensor data
        return;
    }

    _hmdSensorPosition = newHmdSensorPosition;
    _hmdSensorOrientation = glmExtractRotation(hmdSensorMatrix);
    auto headPose = getControllerPoseInSensorFrame(controller::Action::HEAD);
    if (headPose.isValid()) {
        glm::quat bodyOrientation = computeBodyFacingFromHead(headPose.rotation, Vectors::UNIT_Y);
        _headControllerFacing = getFacingDir2D(bodyOrientation);
    } else {
        _headControllerFacing = glm::vec2(1.0f, 0.0f);
    }
}

// Find the vector halfway between the hip to hand azimuth vectors
// This midpoint hand azimuth is in Spine2 space
glm::vec2 MyAvatar::computeHandAzimuth() const {
    controller::Pose leftHandPoseAvatarSpace = getLeftHandPose();
    controller::Pose rightHandPoseAvatarSpace = getRightHandPose();
    controller::Pose headPoseAvatarSpace = getControllerPoseInAvatarFrame(controller::Action::HEAD);
    const float HALFWAY = 0.50f;

    glm::vec2 latestHipToHandController = _hipToHandController;

    int spine2Index = _skeletonModel->getRig().indexOfJoint("Spine2");
    if (leftHandPoseAvatarSpace.isValid() && rightHandPoseAvatarSpace.isValid() && headPoseAvatarSpace.isValid() && !(spine2Index < 0)) {

        glm::vec3 spine2Position = getAbsoluteJointTranslationInObjectFrame(spine2Index);
        glm::quat spine2Rotation = getAbsoluteJointRotationInObjectFrame(spine2Index);

        glm::vec3 rightHandOffset = rightHandPoseAvatarSpace.translation - spine2Position;
        glm::vec3 leftHandOffset = leftHandPoseAvatarSpace.translation - spine2Position;
        glm::vec3 rightHandSpine2Space = glm::inverse(spine2Rotation) * rightHandOffset;
        glm::vec3 leftHandSpine2Space = glm::inverse(spine2Rotation) * leftHandOffset;

        // we need the old azimuth reading to prevent flipping the facing direction 180
        // in the case where the hands go from being slightly less than 180 apart to slightly more than 180 apart.
        glm::vec2 oldAzimuthReading = _hipToHandController;
        if ((glm::length(glm::vec2(rightHandSpine2Space.x, rightHandSpine2Space.z)) > 0.0f) && (glm::length(glm::vec2(leftHandSpine2Space.x, leftHandSpine2Space.z)) > 0.0f)) {
            latestHipToHandController = lerp(glm::normalize(glm::vec2(rightHandSpine2Space.x, rightHandSpine2Space.z)), glm::normalize(glm::vec2(leftHandSpine2Space.x, leftHandSpine2Space.z)), HALFWAY);
        } else {
            latestHipToHandController = glm::vec2(0.0f, 1.0f);
        }

        glm::vec3 headLookAtAvatarSpace = transformVectorFast(headPoseAvatarSpace.getMatrix(), glm::vec3(0.0f, 0.0f, 1.0f));
        glm::vec3 headLookAtSpine2Space = glm::inverse(spine2Rotation) * headLookAtAvatarSpace;

        glm::vec2 headAzimuthSpine2Space = glm::vec2(headLookAtSpine2Space.x, headLookAtSpine2Space.z);
        if (glm::length(headAzimuthSpine2Space) > 0.0f) {
            headAzimuthSpine2Space = glm::normalize(headAzimuthSpine2Space);
        } else {
            headAzimuthSpine2Space = -latestHipToHandController;
        }

        // check the angular distance from forward and back
        float cosForwardAngle = glm::dot(latestHipToHandController, oldAzimuthReading);
        float cosHeadShoulder = glm::dot(-latestHipToHandController, headAzimuthSpine2Space);
        // if we are now closer to the 180 flip of the previous chest forward
        // then we negate our computed latestHipToHandController to keep the chest from flipping.
        // also check the head to shoulder azimuth difference if we negate.
        // don't negate the chest azimuth if this is greater than 100 degrees.
        if ((cosForwardAngle < 0.0f) && !(cosHeadShoulder < -0.2f)) {
            latestHipToHandController = -latestHipToHandController;
        }
    }
    return latestHipToHandController;
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
    float sensorToWorldScale = getEyeHeight() / getUserEyeHeight();
    glm::mat4 desiredMat = createMatFromScaleQuatAndPos(glm::vec3(sensorToWorldScale), getWorldOrientation(), getWorldPosition());
    _sensorToWorldMatrix = desiredMat * glm::inverse(_bodySensorMatrix);

    bool hasSensorToWorldScaleChanged = false;
    if (fabsf(getSensorToWorldScale() - sensorToWorldScale) > MIN_SCALE_CHANGED_DELTA) {
        hasSensorToWorldScaleChanged = true;
    }

    lateUpdatePalms();

    if (_enableDebugDrawSensorToWorldMatrix) {
        DebugDraw::getInstance().addMarker("sensorToWorldMatrix", glmExtractRotation(_sensorToWorldMatrix),
                                           extractTranslation(_sensorToWorldMatrix), glm::vec4(1));
    }

    _sensorToWorldMatrixCache.set(_sensorToWorldMatrix);
    updateJointFromController(controller::Action::LEFT_HAND, _controllerLeftHandMatrixCache);
    updateJointFromController(controller::Action::RIGHT_HAND, _controllerRightHandMatrixCache);
    
    if (hasSensorToWorldScaleChanged) {
        emit sensorToWorldScaleChanged(sensorToWorldScale);
    }
    
}

//  Update avatar head rotation with sensor data
void MyAvatar::updateFromTrackers(float deltaTime) {
    glm::vec3 estimatedRotation;

    bool hasHead = getControllerPoseInAvatarFrame(controller::Action::HEAD).isValid();
    bool playing = DependencyManager::get<recording::Deck>()->isPlaying();
    if (hasHead && playing) {
        return;
    }

    FaceTracker* tracker = qApp->getActiveFaceTracker();
    bool inFacetracker = tracker && !FaceTracker::isMuted();

    if (inFacetracker) {
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
    if (hasHead || playing) {
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
    auto pose = getControllerPoseInAvatarFrame(controller::Action::LEFT_HAND);
    return pose.isValid() ? pose.getTranslation() : glm::vec3(0.0f);
}

glm::vec3 MyAvatar::getRightHandPosition() const {
    auto pose = getControllerPoseInAvatarFrame(controller::Action::RIGHT_HAND);
    return pose.isValid() ? pose.getTranslation() : glm::vec3(0.0f);
}

glm::vec3 MyAvatar::getLeftHandTipPosition() const {
    const float TIP_LENGTH = 0.3f;
    auto pose = getControllerPoseInAvatarFrame(controller::Action::LEFT_HAND);
    return pose.isValid() ? pose.getTranslation() * pose.getRotation() + glm::vec3(0.0f, TIP_LENGTH, 0.0f) : glm::vec3(0.0f);
}

glm::vec3 MyAvatar::getRightHandTipPosition() const {
    const float TIP_LENGTH = 0.3f;
    auto pose = getControllerPoseInAvatarFrame(controller::Action::RIGHT_HAND);
    return pose.isValid() ? pose.getTranslation() * pose.getRotation() + glm::vec3(0.0f, TIP_LENGTH, 0.0f) : glm::vec3(0.0f);
}

controller::Pose MyAvatar::getLeftHandPose() const {
    return getControllerPoseInAvatarFrame(controller::Action::LEFT_HAND);
}

controller::Pose MyAvatar::getRightHandPose() const {
    return getControllerPoseInAvatarFrame(controller::Action::RIGHT_HAND);
}

controller::Pose MyAvatar::getLeftHandTipPose() const {
    auto pose = getLeftHandPose();
    glm::vec3 tipTrans = getLeftHandTipPosition();
    pose.velocity += glm::cross(pose.getAngularVelocity(), pose.getTranslation() - tipTrans);
    pose.translation = tipTrans;
    return pose;
}

controller::Pose MyAvatar::getRightHandTipPose() const {
    auto pose = getRightHandPose();
    glm::vec3 tipTrans = getRightHandTipPosition();
    pose.velocity += glm::cross(pose.getAngularVelocity(), pose.getTranslation() - tipTrans);
    pose.translation = tipTrans;
    return pose;
}

glm::vec3 MyAvatar::worldToJointPoint(const glm::vec3& position, const int jointIndex) const {
    glm::vec3 jointPos = getWorldPosition();//default value if no or invalid joint specified
    glm::quat jointRot = getWorldOrientation();//default value if no or invalid joint specified
    if (jointIndex != -1) {
        if (_skeletonModel->getJointPositionInWorldFrame(jointIndex, jointPos)) {
            _skeletonModel->getJointRotationInWorldFrame(jointIndex, jointRot);
        } else {
            qWarning() << "Invalid joint index specified: " << jointIndex;
        }
    }
    glm::vec3 modelOffset = position - jointPos;
    glm::vec3 jointSpacePosition = glm::inverse(jointRot) * modelOffset;

    return jointSpacePosition;
}

glm::vec3 MyAvatar::worldToJointDirection(const glm::vec3& worldDir, const int jointIndex) const {
    glm::quat jointRot = getWorldOrientation();//default value if no or invalid joint specified
    if ((jointIndex != -1) && (!_skeletonModel->getJointRotationInWorldFrame(jointIndex, jointRot))) {
        qWarning() << "Invalid joint index specified: " << jointIndex;
    }

    glm::vec3 jointSpaceDir = glm::inverse(jointRot) * worldDir;
    return jointSpaceDir;
}

glm::quat MyAvatar::worldToJointRotation(const glm::quat& worldRot, const int jointIndex) const {
    glm::quat jointRot = getWorldOrientation();//default value if no or invalid joint specified
    if ((jointIndex != -1) && (!_skeletonModel->getJointRotationInWorldFrame(jointIndex, jointRot))) {
        qWarning() << "Invalid joint index specified: " << jointIndex;
    }
    glm::quat jointSpaceRot = glm::inverse(jointRot) * worldRot;
    return jointSpaceRot;
}

glm::vec3 MyAvatar::jointToWorldPoint(const glm::vec3& jointSpacePos, const int jointIndex) const {
    glm::vec3 jointPos = getWorldPosition();//default value if no or invalid joint specified
    glm::quat jointRot = getWorldOrientation();//default value if no or invalid joint specified

    if (jointIndex != -1) {
        if (_skeletonModel->getJointPositionInWorldFrame(jointIndex, jointPos)) {
            _skeletonModel->getJointRotationInWorldFrame(jointIndex, jointRot);
        } else {
            qWarning() << "Invalid joint index specified: " << jointIndex;
        }
    }

    glm::vec3 worldOffset = jointRot * jointSpacePos;
    glm::vec3 worldPos = jointPos + worldOffset;

    return worldPos;
}

glm::vec3 MyAvatar::jointToWorldDirection(const glm::vec3& jointSpaceDir, const int jointIndex) const {
    glm::quat jointRot = getWorldOrientation();//default value if no or invalid joint specified
    if ((jointIndex != -1) && (!_skeletonModel->getJointRotationInWorldFrame(jointIndex, jointRot))) {
        qWarning() << "Invalid joint index specified: " << jointIndex;
    }
    glm::vec3 worldDir = jointRot * jointSpaceDir;
    return worldDir;
}

glm::quat MyAvatar::jointToWorldRotation(const glm::quat& jointSpaceRot, const int jointIndex) const {
    glm::quat jointRot = getWorldOrientation();//default value if no or invalid joint specified
    if ((jointIndex != -1) && (!_skeletonModel->getJointRotationInWorldFrame(jointIndex, jointRot))) {
        qWarning() << "Invalid joint index specified: " << jointIndex;
    }
    glm::quat worldRot = jointRot * jointSpaceRot;
    return worldRot;
}

// virtual
void MyAvatar::render(RenderArgs* renderArgs) {
    // don't render if we've been asked to disable local rendering
    if (!_shouldRender) {
        return; // exit early
    }
    Avatar::render(renderArgs);
}

void MyAvatar::overrideAnimation(const QString& url, float fps, bool loop, float firstFrame, float lastFrame) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "overrideAnimation", Q_ARG(const QString&, url), Q_ARG(float, fps),
                                  Q_ARG(bool, loop), Q_ARG(float, firstFrame), Q_ARG(float, lastFrame));
        return;
    }
    _skeletonModel->getRig().overrideAnimation(url, fps, loop, firstFrame, lastFrame);
}

void MyAvatar::restoreAnimation() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "restoreAnimation");
        return;
    }
    _skeletonModel->getRig().restoreAnimation();
}

QStringList MyAvatar::getAnimationRoles() {
    if (QThread::currentThread() != thread()) {
        QStringList result;
        BLOCKING_INVOKE_METHOD(this, "getAnimationRoles", Q_RETURN_ARG(QStringList, result));
        return result;
    }
    return _skeletonModel->getRig().getAnimationRoles();
}

void MyAvatar::overrideRoleAnimation(const QString& role, const QString& url, float fps, bool loop,
                                     float firstFrame, float lastFrame) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "overrideRoleAnimation", Q_ARG(const QString&, role), Q_ARG(const QString&, url),
                                  Q_ARG(float, fps), Q_ARG(bool, loop), Q_ARG(float, firstFrame), Q_ARG(float, lastFrame));
        return;
    }
    _skeletonModel->getRig().overrideRoleAnimation(role, url, fps, loop, firstFrame, lastFrame);
}

void MyAvatar::restoreRoleAnimation(const QString& role) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "restoreRoleAnimation", Q_ARG(const QString&, role));
        return;
    }
    _skeletonModel->getRig().restoreRoleAnimation(role);
}

void MyAvatar::saveAvatarUrl() {
    if (qApp->getSaveAvatarOverrideUrl() || !qApp->getAvatarOverrideUrl().isValid()) {
        _fullAvatarURLSetting.set(_fullAvatarURLFromPreferences == AvatarData::defaultFullAvatarModelUrl() ?
                                  "" :
                                  _fullAvatarURLFromPreferences.toString());
    }
}

void MyAvatar::resizeAvatarEntitySettingHandles(uint32_t maxIndex) {
    // The original Settings interface saved avatar-entity array data like this:
    // Avatar/avatarEntityData/size: 5
    // Avatar/avatarEntityData/1/id: ...
    // Avatar/avatarEntityData/1/properties: ...
    // ...
    // Avatar/avatarEntityData/5/id: ...
    // Avatar/avatarEntityData/5/properties: ...
    //
    // Create Setting::Handles to mimic this.
    uint32_t settingsIndex = (uint32_t)_avatarEntityIDSettings.size() + 1;
    while (settingsIndex <= maxIndex) {
        Setting::Handle<QUuid> idHandle(QStringList() << AVATAR_SETTINGS_GROUP_NAME << "avatarEntityData"
                                        << QString::number(settingsIndex) << "id", QUuid());
        _avatarEntityIDSettings.push_back(idHandle);
        Setting::Handle<QByteArray> dataHandle(QStringList() << AVATAR_SETTINGS_GROUP_NAME << "avatarEntityData"
                                               << QString::number(settingsIndex) << "properties", QByteArray());
        _avatarEntityDataSettings.push_back(dataHandle);
        settingsIndex++;
    }
}

void MyAvatar::saveData() {
    _dominantHandSetting.set(_dominantHand);
    _headPitchSetting.set(getHead()->getBasePitch());
    _scaleSetting.set(_targetScale);
    _yawSpeedSetting.set(_yawSpeed);
    _pitchSpeedSetting.set(_pitchSpeed);

    // only save the fullAvatarURL if it has not been overwritten on command line
    // (so the overrideURL is not valid), or it was overridden _and_ we specified
    // --replaceAvatarURL (so _saveAvatarOverrideUrl is true)
    if (qApp->getSaveAvatarOverrideUrl() || !qApp->getAvatarOverrideUrl().isValid() ) {
        _fullAvatarURLSetting.set(_fullAvatarURLFromPreferences == AvatarData::defaultFullAvatarModelUrl() ?
                                  "" :
                                  _fullAvatarURLFromPreferences.toString());
    }

    _fullAvatarModelNameSetting.set(_fullAvatarModelName);
    QUrl animGraphUrl = _prefOverrideAnimGraphUrl.get();
    _animGraphURLSetting.set(animGraphUrl);
    _displayNameSetting.set(_displayName);
    _collisionSoundURLSetting.set(_collisionSoundURL);
    _useSnapTurnSetting.set(_useSnapTurn);
    _userHeightSetting.set(getUserHeight());
    _flyingHMDSetting.set(getFlyingHMDPref());

    auto hmdInterface = DependencyManager::get<HMDScriptingInterface>();
    saveAvatarEntityDataToSettings();
}

void MyAvatar::saveAvatarEntityDataToSettings() {
    if (!_needToSaveAvatarEntitySettings) {
        return;
    }
    bool success = updateStaleAvatarEntityBlobs();
    if (!success) {
        return;
    }
    _needToSaveAvatarEntitySettings = false;

    uint32_t numEntities = (uint32_t)_cachedAvatarEntityBlobs.size();
    uint32_t prevNumEntities = _avatarEntityCountSetting.get(0);
    resizeAvatarEntitySettingHandles(std::max<uint32_t>(numEntities, prevNumEntities));

    // save new Settings
    if (numEntities > 0) {
        // save all unfortunately-formatted-binary-blobs to Settings
        _avatarEntitiesLock.withWriteLock([&] {
            uint32_t i = 0;
            AvatarEntityMap::const_iterator itr = _cachedAvatarEntityBlobs.begin();
            while (itr != _cachedAvatarEntityBlobs.end()) {
                _avatarEntityIDSettings[i].set(itr.key());
                _avatarEntityDataSettings[i].set(itr.value());
                ++itr;
                ++i;
            }
            numEntities = i;
        });
    }
    _avatarEntityCountSetting.set(numEntities);

    // remove old Settings if any
    if (numEntities < prevNumEntities) {
        uint32_t numEntitiesToRemove = prevNumEntities - numEntities;
        for (uint32_t i = 0; i < numEntitiesToRemove; ++i) {
            if (_avatarEntityIDSettings.size() > numEntities) {
                _avatarEntityIDSettings.back().remove();
                _avatarEntityIDSettings.pop_back();
            }
            if (_avatarEntityDataSettings.size() > numEntities) {
                _avatarEntityDataSettings.back().remove();
                _avatarEntityDataSettings.pop_back();
            }
        }
    }
}

float loadSetting(Settings& settings, const QString& name, float defaultValue) {
    float value = settings.value(name, defaultValue).toFloat();
    if (glm::isnan(value)) {
        value = defaultValue;
    }
    return value;
}

void MyAvatar::setToggleHips(bool followHead) {
    _follow.setToggleHipsFollowing(followHead);
}

void MyAvatar::FollowHelper::setToggleHipsFollowing(bool followHead) {
    _toggleHipsFollowing = followHead;
}

bool MyAvatar::FollowHelper::getToggleHipsFollowing() const {
    return _toggleHipsFollowing;
}

void MyAvatar::setEnableDebugDrawBaseOfSupport(bool isEnabled) {
    _enableDebugDrawBaseOfSupport = isEnabled;
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
}

void MyAvatar::setEnableDebugDrawIKConstraints(bool isEnabled) {
    _enableDebugDrawIKConstraints = isEnabled;
}

void MyAvatar::setEnableDebugDrawDetailedCollision(bool isEnabled) {
    _enableDebugDrawDetailedCollision = isEnabled;
}

void MyAvatar::setEnableDebugDrawIKChains(bool isEnabled) {
    _enableDebugDrawIKChains = isEnabled;
}

void MyAvatar::setEnableMeshVisible(bool isEnabled) {
    return Avatar::setEnableMeshVisible(isEnabled);
}

bool MyAvatar::getEnableMeshVisible() const {
    return Avatar::getEnableMeshVisible();
}

void MyAvatar::setEnableInverseKinematics(bool isEnabled) {
    _skeletonModel->getRig().setEnableInverseKinematics(isEnabled);
}

void MyAvatar::storeAvatarEntityDataPayload(const QUuid& entityID, const QByteArray& payload) {
    AvatarData::storeAvatarEntityDataPayload(entityID, payload);
    _avatarEntitiesLock.withWriteLock([&] {
        _cachedAvatarEntityBlobsToAddOrUpdate.push_back(entityID);
    });
}

void MyAvatar::clearAvatarEntity(const QUuid& entityID, bool requiresRemovalFromTree) {
    AvatarData::clearAvatarEntity(entityID, requiresRemovalFromTree);
    _avatarEntitiesLock.withWriteLock([&] {
        _cachedAvatarEntityBlobsToDelete.push_back(entityID);
    });
}

void MyAvatar::sanitizeAvatarEntityProperties(EntityItemProperties& properties) const {
    properties.setEntityHostType(entity::HostType::AVATAR);
    properties.setOwningAvatarID(getID());

    // there's no entity-server to tell us we're the simulation owner, so always set the
    // simulationOwner to the owningAvatarID and a high priority.
    properties.setSimulationOwner(getID(), AVATAR_ENTITY_SIMULATION_PRIORITY);

    if (properties.getParentID() == AVATAR_SELF_ID) {
        properties.setParentID(getID());
    }

    // When grabbing avatar entities, they are parented to the joint moving them, then when un-grabbed
    // they go back to the default parent (null uuid).  When un-gripped, others saw the entity disappear.
    // The thinking here is the local position was noticed as changing, but not the parentID (since it is now
    // back to the default), and the entity flew off somewhere.  Marking all changed definitely fixes this,
    // and seems safe (per Seth).
    properties.markAllChanged();
}

void MyAvatar::handleChangedAvatarEntityData() {
    // NOTE: this is a per-frame update
    if (getID().isNull() ||
        getID() == AVATAR_SELF_ID ||
        DependencyManager::get<NodeList>()->getSessionUUID() == QUuid()) {
        // wait until MyAvatar and this Node gets an ID before doing this.  Otherwise, various things go wrong:
        // things get their parent fixed up from AVATAR_SELF_ID to a null uuid which means "no parent".
        return;
    }
    if (_reloadAvatarEntityDataFromSettings) {
        loadAvatarEntityDataFromSettings();
    }

    auto treeRenderer = DependencyManager::get<EntityTreeRenderer>();
    EntityTreePointer entityTree = treeRenderer ? treeRenderer->getTree() : nullptr;
    if (!entityTree) {
        return;
    }

    // We collect changes to AvatarEntities and then handle them all in one spot per frame: handleChangedAvatarEntityData().
    // Basically this is a "transaction pattern" with an extra complication: these changes can come from two
    // "directions" and the "authoritative source" of each direction is different, so we maintain two distinct sets
    // of transaction lists:
    //
    // The _entitiesToDelete/Add/Update lists are for changes whose "authoritative sources" are already
    // correctly stored in _cachedAvatarEntityBlobs.  These come from loadAvatarEntityDataFromSettings() and
    // setAvatarEntityData().  These changes need to be extracted from _cachedAvatarEntityBlobs and applied to
    // real EntityItems.
    //
    // The _cachedAvatarEntityBlobsToDelete/Add/Update lists are for changes whose "authoritative sources" are
    // already reflected in real EntityItems. These changes need to be propagated to _cachedAvatarEntityBlobs
    // and eventually to Settings.
    //
    // The DELETEs also need to be propagated to the traits, which will eventually propagate to
    // AvatarData::_packedAvatarEntityData via deeper logic.

    // move the lists to minimize lock time
    std::vector<QUuid> cachedBlobsToDelete;
    std::vector<QUuid> cachedBlobsToUpdate;
    std::vector<QUuid> entitiesToDelete;
    std::vector<QUuid> entitiesToAdd;
    std::vector<QUuid> entitiesToUpdate;
    _avatarEntitiesLock.withWriteLock([&] {
        cachedBlobsToDelete = std::move(_cachedAvatarEntityBlobsToDelete);
        cachedBlobsToUpdate = std::move(_cachedAvatarEntityBlobsToAddOrUpdate);
        entitiesToDelete = std::move(_entitiesToDelete);
        entitiesToAdd = std::move(_entitiesToAdd);
        entitiesToUpdate = std::move(_entitiesToUpdate);
    });

    auto removeAllInstancesHelper = [] (const QUuid& id, std::vector<QUuid>& v) {
        uint32_t i = 0;
        while (i < v.size()) {
            if (id == v[i]) {
                v[i] = v.back();
                v.pop_back();
            } else {
                ++i;
            }
        }
    };

    // remove delete-add and delete-update overlap
    for (const auto& id : entitiesToDelete) {
        removeAllInstancesHelper(id, cachedBlobsToUpdate);
        removeAllInstancesHelper(id, entitiesToAdd);
        removeAllInstancesHelper(id, entitiesToUpdate);
    }
    for (const auto& id : cachedBlobsToDelete) {
        removeAllInstancesHelper(id, entitiesToUpdate);
        removeAllInstancesHelper(id, cachedBlobsToUpdate);
    }
    for (const auto& id : entitiesToAdd) {
        removeAllInstancesHelper(id, entitiesToUpdate);
    }

    // DELETE real entities
    for (const auto& id : entitiesToDelete) {
        entityTree->withWriteLock([&] {
            entityTree->deleteEntity(id);
        });
    }

    // ADD real entities
    EntityEditPacketSender* packetSender = qApp->getEntityEditPacketSender();
    for (const auto& id : entitiesToAdd) {
        bool blobFailed = false;
        EntityItemProperties properties;
        _avatarEntitiesLock.withReadLock([&] {
            AvatarEntityMap::iterator itr = _cachedAvatarEntityBlobs.find(id);
            if (itr == _cachedAvatarEntityBlobs.end()) {
                blobFailed = true; // blob doesn't exist
                return;
            }
            if (!EntityItemProperties::blobToProperties(*_myScriptEngine, itr.value(), properties)) {
                blobFailed = true; // blob is corrupt
            }
        });
        if (blobFailed) {
            // remove from _cachedAvatarEntityBlobUpdatesToSkip just in case:
            // avoids a resource leak when blob updates to be skipped are never actually skipped
            // when the blob fails to result in a real EntityItem
            _avatarEntitiesLock.withWriteLock([&] {
                removeAllInstancesHelper(id, _cachedAvatarEntityBlobUpdatesToSkip);
            });
            continue;
        }
        sanitizeAvatarEntityProperties(properties);
        entityTree->withWriteLock([&] {
            EntityItemPointer entity = entityTree->addEntity(id, properties);
            if (entity) {
                packetSender->queueEditEntityMessage(PacketType::EntityAdd, entityTree, id, properties);
            }
        });
    }

    // CHANGE real entities
    for (const auto& id : entitiesToUpdate) {
        EntityItemProperties properties;
        bool skip = false;
        _avatarEntitiesLock.withReadLock([&] {
            AvatarEntityMap::iterator itr = _cachedAvatarEntityBlobs.find(id);
            if (itr == _cachedAvatarEntityBlobs.end()) {
                skip = true;
                return;
            }
            if (!EntityItemProperties::blobToProperties(*_myScriptEngine, itr.value(), properties)) {
                skip = true;
            }
        });
        sanitizeAvatarEntityProperties(properties);
        entityTree->withWriteLock([&] {
            entityTree->updateEntity(id, properties);
        });
    }

    // DELETE cached blobs
    _avatarEntitiesLock.withWriteLock([&] {
        for (const auto& id : cachedBlobsToDelete) {
            AvatarEntityMap::iterator itr = _cachedAvatarEntityBlobs.find(id);
            // remove blob and remember to remove from settings
            if (itr != _cachedAvatarEntityBlobs.end()) {
                _cachedAvatarEntityBlobs.erase(itr);
                _needToSaveAvatarEntitySettings = true;
            }
            // also remove from list of stale blobs to avoid failed entity lookup later
            std::set<QUuid>::iterator blobItr = _staleCachedAvatarEntityBlobs.find(id);
            if (blobItr != _staleCachedAvatarEntityBlobs.end()) {
                _staleCachedAvatarEntityBlobs.erase(blobItr);
            }
            // also remove from _cachedAvatarEntityBlobUpdatesToSkip just in case:
            // avoids a resource leak when things are deleted before they could be skipped
            removeAllInstancesHelper(id, _cachedAvatarEntityBlobUpdatesToSkip);
        }
    });

    // ADD/UPDATE cached blobs
    for (const auto& id : cachedBlobsToUpdate) {
        // computing the blobs is expensive and we want to avoid it when possible
        // so we add these ids to _staleCachedAvatarEntityBlobs for later
        // and only build the blobs when absolutely necessary
        bool skip = false;
        uint32_t i = 0;
        _avatarEntitiesLock.withWriteLock([&] {
            while (i < _cachedAvatarEntityBlobUpdatesToSkip.size()) {
                if (id == _cachedAvatarEntityBlobUpdatesToSkip[i]) {
                    _cachedAvatarEntityBlobUpdatesToSkip[i] = _cachedAvatarEntityBlobUpdatesToSkip.back();
                    _cachedAvatarEntityBlobUpdatesToSkip.pop_back();
                    skip = true;
                    break; // assume no duplicates
                } else {
                    ++i;
                }
            }
        });
        if (!skip) {
            _staleCachedAvatarEntityBlobs.insert(id);
            _needToSaveAvatarEntitySettings = true;
        }
    }

    // DELETE traits
    // (no need to worry about the ADDs and UPDATEs: each will be handled when the interface
    // tries to send a real update packet (via AvatarData::storeAvatarEntityDataPayload()))
    if (_clientTraitsHandler) {
        // we have a client traits handler
        // flag removed entities as deleted so that changes are sent next frame
        _avatarEntitiesLock.withWriteLock([&] {
            for (const auto& id : entitiesToDelete) {
                if (_packedAvatarEntityData.find(id) != _packedAvatarEntityData.end()) {
                    _clientTraitsHandler->markInstancedTraitDeleted(AvatarTraits::AvatarEntity, id);
                }
            }
            for (const auto& id : cachedBlobsToDelete) {
                if (_packedAvatarEntityData.find(id) != _packedAvatarEntityData.end()) {
                    _clientTraitsHandler->markInstancedTraitDeleted(AvatarTraits::AvatarEntity, id);
                }
            }
        });
    }
}

bool MyAvatar::updateStaleAvatarEntityBlobs() const {
    // call this right before you actually need to use the blobs
    //
    // Note: this method is const (and modifies mutable data members)
    // so we can call it at the Last Minute inside other const methods
    //
    auto treeRenderer = DependencyManager::get<EntityTreeRenderer>();
    EntityTreePointer entityTree = treeRenderer ? treeRenderer->getTree() : nullptr;
    if (!entityTree) {
        return false;
    }

    std::set<QUuid> staleBlobs = std::move(_staleCachedAvatarEntityBlobs);
    int32_t numFound = 0;
    for (const auto& id : staleBlobs) {
        bool found = false;
        EntityItemProperties properties;
        entityTree->withReadLock([&] {
            EntityItemPointer entity = entityTree->findEntityByID(id);
            if (entity) {
                properties = entity->getProperties();
                found = true;
            }
        });
        if (found) {
            ++numFound;
            QByteArray blob;
            EntityItemProperties::propertiesToBlob(*_myScriptEngine, getID(), properties, blob);
            _avatarEntitiesLock.withWriteLock([&] {
                _cachedAvatarEntityBlobs[id] = blob;
            });
        }
    }
    return true;
}

void MyAvatar::prepareAvatarEntityDataForReload() {
    saveAvatarEntityDataToSettings();

    _avatarEntitiesLock.withWriteLock([&] {
        _packedAvatarEntityData.clear();
        _entitiesToDelete.clear();
        _entitiesToAdd.clear();
        _entitiesToUpdate.clear();
        _cachedAvatarEntityBlobs.clear();
        _cachedAvatarEntityBlobsToDelete.clear();
        _cachedAvatarEntityBlobsToAddOrUpdate.clear();
        _cachedAvatarEntityBlobUpdatesToSkip.clear();
    });

    _reloadAvatarEntityDataFromSettings = true;
}

AvatarEntityMap MyAvatar::getAvatarEntityData() const {
    // NOTE: the return value is expected to be a map of unfortunately-formatted-binary-blobs
    updateStaleAvatarEntityBlobs();
    AvatarEntityMap result;
    _avatarEntitiesLock.withReadLock([&] {
        result = _cachedAvatarEntityBlobs;
    });
    return result;
}

void MyAvatar::setAvatarEntityData(const AvatarEntityMap& avatarEntityData) {
    // Note: this is an invokable Script call
    // avatarEntityData is expected to be a map of QByteArrays that represent EntityItemProperties objects from JavaScript,
    // aka: unfortunately-formatted-binary-blobs because we store them in non-human-readable format in Settings.
    //
    if (avatarEntityData.size() > MAX_NUM_AVATAR_ENTITIES) {
        // the data is suspect
        qCDebug(interfaceapp) << "discard suspect AvatarEntityData with size =" << avatarEntityData.size();
        return;
    }

    // this overwrites ALL AvatarEntityData so we clear pending operations
    _avatarEntitiesLock.withWriteLock([&] {
        _packedAvatarEntityData.clear();
        _entitiesToDelete.clear();
        _entitiesToAdd.clear();
        _entitiesToUpdate.clear();
    });
    _needToSaveAvatarEntitySettings = true;

    _avatarEntitiesLock.withWriteLock([&] {
        // find new and updated IDs
        AvatarEntityMap::const_iterator constItr = avatarEntityData.begin();
        while (constItr != avatarEntityData.end()) {
            QUuid id = constItr.key();
            if (_cachedAvatarEntityBlobs.find(id) == _cachedAvatarEntityBlobs.end()) {
                _entitiesToAdd.push_back(id);
            } else {
                _entitiesToUpdate.push_back(id);
            }
            ++constItr;
        }
        // find and erase deleted IDs from _cachedAvatarEntityBlobs
        std::vector<QUuid> deletedIDs;
        AvatarEntityMap::iterator itr = _cachedAvatarEntityBlobs.begin();
        while (itr != _cachedAvatarEntityBlobs.end()) {
            QUuid id = itr.key();
            if (std::find(_entitiesToUpdate.begin(), _entitiesToUpdate.end(), id) == _entitiesToUpdate.end()) {
                deletedIDs.push_back(id);
                itr = _cachedAvatarEntityBlobs.erase(itr);
            } else {
                ++itr;
            }
        }
        // copy new data
        constItr = avatarEntityData.begin();
        while (constItr != avatarEntityData.end()) {
            _cachedAvatarEntityBlobs.insert(constItr.key(), constItr.value());
            ++constItr;
        }
        // erase deleted IDs from _packedAvatarEntityData
        for (const auto& id : deletedIDs) {
            itr = _packedAvatarEntityData.find(id);
            if (itr != _packedAvatarEntityData.end()) {
                _packedAvatarEntityData.erase(itr);
            } else {
                ++itr;
            }
            _entitiesToDelete.push_back(id);
        }
    });
}

void MyAvatar::updateAvatarEntity(const QUuid& entityID, const QByteArray& entityData) {
    // NOTE: this is an invokable Script call
    // TODO: we should handle the case where entityData is corrupt or invalid
    // BEFORE we store into _cachedAvatarEntityBlobs
    _needToSaveAvatarEntitySettings = true;
    _avatarEntitiesLock.withWriteLock([&] {
        AvatarEntityMap::iterator itr = _cachedAvatarEntityBlobs.find(entityID);
        if (itr != _cachedAvatarEntityBlobs.end()) {
            _entitiesToUpdate.push_back(entityID);
            itr.value() = entityData;
        } else {
            _entitiesToAdd.push_back(entityID);
            _cachedAvatarEntityBlobs.insert(entityID, entityData);
        }
    });
}

void MyAvatar::avatarEntityDataToJson(QJsonObject& root) const {
    updateStaleAvatarEntityBlobs();
    _avatarEntitiesLock.withReadLock([&] {
        if (!_cachedAvatarEntityBlobs.empty()) {
            QJsonArray avatarEntityJson;
            int entityCount = 0;
            AvatarEntityMap::const_iterator itr = _cachedAvatarEntityBlobs.begin();
            while (itr != _cachedAvatarEntityBlobs.end()) {
                QVariantMap entityData;
                QUuid id = _avatarEntityForRecording.size() == _cachedAvatarEntityBlobs.size() ? _avatarEntityForRecording.values()[entityCount++] : itr.key();
                entityData.insert("id", id);
                entityData.insert("properties", itr.value().toBase64());
                avatarEntityJson.push_back(QVariant(entityData).toJsonObject());
                ++itr;
            }
            const QString JSON_AVATAR_ENTITIES = QStringLiteral("attachedEntities");
            root[JSON_AVATAR_ENTITIES] = avatarEntityJson;
        }
    });
}

void MyAvatar::loadData() {
    if (!_myScriptEngine) {
        _myScriptEngine = new QScriptEngine();
    }
    getHead()->setBasePitch(_headPitchSetting.get());

    _yawSpeed = _yawSpeedSetting.get(_yawSpeed);
    _pitchSpeed = _pitchSpeedSetting.get(_pitchSpeed);

    _prefOverrideAnimGraphUrl.set(_animGraphURLSetting.get().toString());
    _fullAvatarURLFromPreferences = _fullAvatarURLSetting.get(QUrl(AvatarData::defaultFullAvatarModelUrl()));
    _fullAvatarModelName = _fullAvatarModelNameSetting.get(DEFAULT_FULL_AVATAR_MODEL_NAME).toString();

    useFullAvatarURL(_fullAvatarURLFromPreferences, _fullAvatarModelName);

    loadAvatarEntityDataFromSettings();

    // Flying preferences must be loaded before calling setFlyingEnabled()
    Setting::Handle<bool> firstRunVal { Settings::firstRun, true };
    setFlyingHMDPref(firstRunVal.get() ? false : _flyingHMDSetting.get());
    setFlyingEnabled(getFlyingEnabled());

    setDisplayName(_displayNameSetting.get());
    setCollisionSoundURL(_collisionSoundURLSetting.get(QUrl(DEFAULT_AVATAR_COLLISION_SOUND_URL)).toString());
    setSnapTurn(_useSnapTurnSetting.get());
    setDominantHand(_dominantHandSetting.get(DOMINANT_RIGHT_HAND).toLower());
    setUserHeight(_userHeightSetting.get(DEFAULT_AVATAR_HEIGHT));
    setTargetScale(_scaleSetting.get());

    setEnableMeshVisible(Menu::getInstance()->isOptionChecked(MenuOption::MeshVisible));
    _follow.setToggleHipsFollowing (Menu::getInstance()->isOptionChecked(MenuOption::ToggleHipsFollowing));
    setEnableDebugDrawBaseOfSupport(Menu::getInstance()->isOptionChecked(MenuOption::AnimDebugDrawBaseOfSupport));
    setEnableDebugDrawDefaultPose(Menu::getInstance()->isOptionChecked(MenuOption::AnimDebugDrawDefaultPose));
    setEnableDebugDrawAnimPose(Menu::getInstance()->isOptionChecked(MenuOption::AnimDebugDrawAnimPose));
    setEnableDebugDrawPosition(Menu::getInstance()->isOptionChecked(MenuOption::AnimDebugDrawPosition));
}

void MyAvatar::loadAvatarEntityDataFromSettings() {
    // this overwrites ALL AvatarEntityData so we clear pending operations
    _avatarEntitiesLock.withWriteLock([&] {
        _packedAvatarEntityData.clear();
        _entitiesToDelete.clear();
        _entitiesToAdd.clear();
        _entitiesToUpdate.clear();
    });
    _reloadAvatarEntityDataFromSettings = false;
    _needToSaveAvatarEntitySettings = false;

    int numEntities = _avatarEntityCountSetting.get(0);
    if (numEntities == 0) {
        return;
    }
    resizeAvatarEntitySettingHandles(numEntities);

    _avatarEntitiesLock.withWriteLock([&] {
        _entitiesToAdd.reserve(numEntities);
        // TODO: build map between old and new IDs so we can restitch parent-child relationships
        for (int i = 0; i < numEntities; i++) {
            QUuid id = QUuid::createUuid(); // generate a new ID
            _cachedAvatarEntityBlobs[id] = _avatarEntityDataSettings[i].get();
            _entitiesToAdd.push_back(id);
            // this blob is the "authoritative source" for this AvatarEntity and we want to avoid overwriting it
            // (the outgoing update packet will flag it for save-back into the blob)
            // which is why we remember its id: to skip its save-back later
            _cachedAvatarEntityBlobUpdatesToSkip.push_back(id);
        }
    });
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

ScriptAvatarData* MyAvatar::getTargetAvatar() const {
    auto avatar = std::static_pointer_cast<Avatar>(_lookAtTargetAvatar.lock());
    if (avatar) {
        return new ScriptAvatar(avatar);
    } else {
        return nullptr;
    }
}

static float lookAtCostFunction(const glm::vec3& myForward, const glm::vec3& myPosition, const glm::vec3& otherForward, const glm::vec3& otherPosition,
                                bool otherIsTalking, bool lookingAtOtherAlready) {
    const float DISTANCE_FACTOR = 3.14f;
    const float MY_ANGLE_FACTOR = 1.0f;
    const float OTHER_ANGLE_FACTOR = 1.0f;
    const float OTHER_IS_TALKING_TERM = otherIsTalking ? 1.0f : 0.0f;
    const float LOOKING_AT_OTHER_ALREADY_TERM = lookingAtOtherAlready ? -0.2f : 0.0f;

    const float GREATEST_LOOKING_AT_DISTANCE = 10.0f;  // meters
    const float MAX_MY_ANGLE = PI / 8.0f; // 22.5 degrees, Don't look too far away from the head facing.
    const float MAX_OTHER_ANGLE = (3.0f * PI) / 4.0f;  // 135 degrees, Don't stare at the back of another avatars head.

    glm::vec3 d = otherPosition - myPosition;
    float distance = glm::length(d);
    glm::vec3 dUnit = d / distance;
    float myAngle = acosf(glm::dot(myForward, dUnit));
    float otherAngle = acosf(glm::dot(otherForward, -dUnit));

    if (distance > GREATEST_LOOKING_AT_DISTANCE || myAngle > MAX_MY_ANGLE || otherAngle > MAX_OTHER_ANGLE) {
        return FLT_MAX;
    } else {
        return (DISTANCE_FACTOR * distance +
                MY_ANGLE_FACTOR * myAngle +
                OTHER_ANGLE_FACTOR * otherAngle +
                OTHER_IS_TALKING_TERM +
                LOOKING_AT_OTHER_ALREADY_TERM);
    }
}

void MyAvatar::computeMyLookAtTarget(const AvatarHash& hash) {
    glm::vec3 myForward = getHead()->getFinalOrientationInWorldFrame() * IDENTITY_FORWARD;
    glm::vec3 myPosition = getHead()->getEyePosition();
    CameraMode mode = qApp->getCamera().getMode();
    if (mode == CAMERA_MODE_FIRST_PERSON) {
        myPosition = qApp->getCamera().getPosition();
    }

    float bestCost = FLT_MAX;
    std::shared_ptr<Avatar> bestAvatar;

    foreach (const AvatarSharedPointer& avatarData, hash) {
        std::shared_ptr<Avatar> avatar = std::static_pointer_cast<Avatar>(avatarData);
        if (!avatar->isMyAvatar() && avatar->isInitialized()) {
            glm::vec3 otherForward = avatar->getHead()->getForwardDirection();
            glm::vec3 otherPosition = avatar->getHead()->getEyePosition();
            const float TIME_WITHOUT_TALKING_THRESHOLD = 1.0f;
            bool otherIsTalking = avatar->getHead()->getTimeWithoutTalking() <= TIME_WITHOUT_TALKING_THRESHOLD;
            bool lookingAtOtherAlready = _lookAtTargetAvatar.lock().get() == avatar.get();
            float cost = lookAtCostFunction(myForward, myPosition, otherForward, otherPosition, otherIsTalking, lookingAtOtherAlready);
            if (cost < bestCost) {
                bestCost = cost;
                bestAvatar = avatar;
            }
        }
    }

    if (bestAvatar) {
        _lookAtTargetAvatar = bestAvatar;
        _targetAvatarPosition = bestAvatar->getWorldPosition();
    } else {
        _lookAtTargetAvatar.reset();
    }
}

void MyAvatar::snapOtherAvatarLookAtTargetsToMe(const AvatarHash& hash) {
    foreach (const AvatarSharedPointer& avatarData, hash) {
        std::shared_ptr<Avatar> avatar = std::static_pointer_cast<Avatar>(avatarData);
        if (!avatar->isMyAvatar() && avatar->isInitialized()) {
            if (_lookAtSnappingEnabled && avatar->getLookAtSnappingEnabled() && isLookingAtMe(avatar)) {

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

                 glm::vec3 viewPosition = viewFrustum.getPosition();
#if DEBUG_ALWAYS_LOOKAT_EYES_NOT_CAMERA
                 viewPosition = (avatarLeftEye + avatarRightEye) / 2.0f;
#endif
                // scale gazeOffset by IPD, if wearing an HMD.
                if (qApp->isHMDMode()) {
                    glm::quat viewOrientation = viewFrustum.getOrientation();
                    glm::mat4 leftEye = qApp->getEyeOffset(Eye::Left);
                    glm::mat4 rightEye = qApp->getEyeOffset(Eye::Right);
                    glm::vec3 leftEyeHeadLocal = glm::vec3(leftEye[3]);
                    glm::vec3 rightEyeHeadLocal = glm::vec3(rightEye[3]);
                    glm::vec3 humanLeftEye = viewPosition + (viewOrientation * leftEyeHeadLocal);
                    glm::vec3 humanRightEye = viewPosition + (viewOrientation * rightEyeHeadLocal);

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
                glm::vec3 corrected = viewPosition + gazeOffset;

                avatar->getHead()->setCorrectedLookAtPosition(corrected);

            } else {
                avatar->getHead()->clearCorrectedLookAtPosition();
            }
        } else {
            avatar->getHead()->clearCorrectedLookAtPosition();
        }
    }
}

void MyAvatar::updateLookAtTargetAvatar() {

    // The AvatarManager is a mutable class shared by many threads.  We make a thread-safe deep copy of it,
    // to avoid having to hold a lock while we iterate over all the avatars within.
    AvatarHash hash = DependencyManager::get<AvatarManager>()->getHashCopy();

    // determine what the best look at target for my avatar should be.
    computeMyLookAtTarget(hash);

    // snap look at position for avatars that are looking at me.
    snapOtherAvatarLookAtTargetsToMe(hash);
}

void MyAvatar::clearLookAtTargetAvatar() {
    _lookAtTargetAvatar.reset();
}

eyeContactTarget MyAvatar::getEyeContactTarget() {
    return _eyeContactTarget;
}

glm::vec3 MyAvatar::getDefaultEyePosition() const {
    return getWorldPosition() + getWorldOrientation() * Quaternions::Y_180 * _skeletonModel->getDefaultEyeModelPosition();
}

const float SCRIPT_PRIORITY = 1.0f + 1.0f;
const float RECORDER_PRIORITY = 1.0f + 1.0f;

void MyAvatar::setJointRotations(const QVector<glm::quat>& jointRotations) {
    int numStates = glm::min(_skeletonModel->getJointStateCount(), jointRotations.size());
    for (int i = 0; i < numStates; ++i) {
        // HACK: ATM only Recorder calls setJointRotations() so we hardcode its priority here
        _skeletonModel->setJointRotation(i, true, jointRotations[i], RECORDER_PRIORITY);
    }
}

void MyAvatar::setJointData(int index, const glm::quat& rotation, const glm::vec3& translation) {
    switch (index) {
        case FARGRAB_RIGHTHAND_INDEX: {
            _farGrabRightMatrixCache.set(createMatFromQuatAndPos(rotation, translation));
            break;
        }
        case FARGRAB_LEFTHAND_INDEX: {
            _farGrabLeftMatrixCache.set(createMatFromQuatAndPos(rotation, translation));
            break;
        }
        case FARGRAB_MOUSE_INDEX: {
            _farGrabMouseMatrixCache.set(createMatFromQuatAndPos(rotation, translation));
            break;
        }
        default: {
            if (QThread::currentThread() != thread()) {
                QMetaObject::invokeMethod(this, "setJointData", Q_ARG(int, index), Q_ARG(const glm::quat&, rotation),
                                          Q_ARG(const glm::vec3&, translation));
                return;
            }
            // HACK: ATM only JS scripts call setJointData() on MyAvatar so we hardcode the priority
            _skeletonModel->getRig().setJointState(index, true, rotation, translation, SCRIPT_PRIORITY);
        }
    }
}

void MyAvatar::setJointRotation(int index, const glm::quat& rotation) {
    switch (index) {
        case FARGRAB_RIGHTHAND_INDEX: {
            glm::mat4 prevMat = _farGrabRightMatrixCache.get();
            glm::vec3 previousTranslation = extractTranslation(prevMat);
            _farGrabRightMatrixCache.set(createMatFromQuatAndPos(rotation, previousTranslation));
            break;
        }
        case FARGRAB_LEFTHAND_INDEX: {
            glm::mat4 prevMat = _farGrabLeftMatrixCache.get();
            glm::vec3 previousTranslation = extractTranslation(prevMat);
            _farGrabLeftMatrixCache.set(createMatFromQuatAndPos(rotation, previousTranslation));
            break;
        }
        case FARGRAB_MOUSE_INDEX: {
            glm::mat4 prevMat = _farGrabMouseMatrixCache.get();
            glm::vec3 previousTranslation = extractTranslation(prevMat);
            _farGrabMouseMatrixCache.set(createMatFromQuatAndPos(rotation, previousTranslation));
            break;
        }
        default: {
            if (QThread::currentThread() != thread()) {
                QMetaObject::invokeMethod(this, "setJointRotation", Q_ARG(int, index), Q_ARG(const glm::quat&, rotation));
                return;
            }
            // HACK: ATM only JS scripts call setJointData() on MyAvatar so we hardcode the priority
            _skeletonModel->getRig().setJointRotation(index, true, rotation, SCRIPT_PRIORITY);
        }
    }
}

void MyAvatar::setJointTranslation(int index, const glm::vec3& translation) {
    switch (index) {
        case FARGRAB_RIGHTHAND_INDEX: {
            glm::mat4 prevMat = _farGrabRightMatrixCache.get();
            glm::quat previousRotation = extractRotation(prevMat);
            _farGrabRightMatrixCache.set(createMatFromQuatAndPos(previousRotation, translation));
            break;
        }
        case FARGRAB_LEFTHAND_INDEX: {
            glm::mat4 prevMat = _farGrabLeftMatrixCache.get();
            glm::quat previousRotation = extractRotation(prevMat);
            _farGrabLeftMatrixCache.set(createMatFromQuatAndPos(previousRotation, translation));
            break;
        }
        case FARGRAB_MOUSE_INDEX: {
            glm::mat4 prevMat = _farGrabMouseMatrixCache.get();
            glm::quat previousRotation = extractRotation(prevMat);
            _farGrabMouseMatrixCache.set(createMatFromQuatAndPos(previousRotation, translation));
            break;
        }
        default: {
            if (QThread::currentThread() != thread()) {
                QMetaObject::invokeMethod(this, "setJointTranslation",
                                          Q_ARG(int, index), Q_ARG(const glm::vec3&, translation));
                return;
            }
            // HACK: ATM only JS scripts call setJointData() on MyAvatar so we hardcode the priority
            _skeletonModel->getRig().setJointTranslation(index, true, translation, SCRIPT_PRIORITY);
        }
    }
}

void MyAvatar::clearJointData(int index) {
    switch (index) {
        case FARGRAB_RIGHTHAND_INDEX: {
            _farGrabRightMatrixCache.invalidate();
            break;
        }
        case FARGRAB_LEFTHAND_INDEX: {
            _farGrabLeftMatrixCache.invalidate();
            break;
        }
        case FARGRAB_MOUSE_INDEX: {
            _farGrabMouseMatrixCache.invalidate();
            break;
        }
        default: {
            if (QThread::currentThread() != thread()) {
                QMetaObject::invokeMethod(this, "clearJointData", Q_ARG(int, index));
                return;
            }
            _skeletonModel->getRig().clearJointAnimationPriority(index);
        }
    }
}

void MyAvatar::setJointData(const QString& name, const glm::quat& rotation, const glm::vec3& translation) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setJointData", Q_ARG(QString, name), Q_ARG(const glm::quat&, rotation),
                                  Q_ARG(const glm::vec3&, translation));
        return;
    }
    writeLockWithNamedJointIndex(name, [&](int index) {
        setJointData(index, rotation, translation);
    });
}

void MyAvatar::setJointRotation(const QString& name, const glm::quat& rotation) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setJointRotation", Q_ARG(QString, name), Q_ARG(const glm::quat&, rotation));
        return;
    }
    writeLockWithNamedJointIndex(name, [&](int index) {
        setJointRotation(index, rotation);
    });
}

void MyAvatar::setJointTranslation(const QString& name, const glm::vec3& translation) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setJointTranslation", Q_ARG(QString, name), Q_ARG(const glm::vec3&, translation));
        return;
    }
    writeLockWithNamedJointIndex(name, [&](int index) {
        setJointTranslation(index, translation);
    });
}

void MyAvatar::clearJointData(const QString& name) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "clearJointData", Q_ARG(QString, name));
        return;
    }
    writeLockWithNamedJointIndex(name, [&](int index) {
        clearJointData(index);
    });
}

void MyAvatar::clearJointsData() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "clearJointsData");
        return;
    }
    _farGrabRightMatrixCache.invalidate();
    _farGrabLeftMatrixCache.invalidate();
    _farGrabMouseMatrixCache.invalidate();
    _skeletonModel->getRig().clearJointStates();
}

void MyAvatar::setSkeletonModelURL(const QUrl& skeletonModelURL) {
    _skeletonModelChangeCount++;
    int skeletonModelChangeCount = _skeletonModelChangeCount;

    auto previousSkeletonModelURL = _skeletonModelURL;
    Avatar::setSkeletonModelURL(skeletonModelURL);

    _skeletonModel->setTagMask(render::hifi::TAG_NONE);
    _skeletonModel->setGroupCulled(true);
    _skeletonModel->setVisibleInScene(true, qApp->getMain3DScene());

    _headBoneSet.clear();
    _cauterizationNeedsUpdate = true;
    _skeletonModelLoaded = false;

    std::shared_ptr<QMetaObject::Connection> skeletonConnection = std::make_shared<QMetaObject::Connection>();
    *skeletonConnection = QObject::connect(_skeletonModel.get(), &SkeletonModel::skeletonLoaded, [this, skeletonModelChangeCount, skeletonConnection]() {
       if (skeletonModelChangeCount == _skeletonModelChangeCount) {

           if (_fullAvatarModelName.isEmpty()) {
               // Store the FST file name into preferences
               const auto& mapping = _skeletonModel->getGeometry()->getMapping();
               if (mapping.value("name").isValid()) {
                   _fullAvatarModelName = mapping.value("name").toString();
               }
           }

           initHeadBones();
           _skeletonModel->setCauterizeBoneSet(_headBoneSet);
           _fstAnimGraphOverrideUrl = _skeletonModel->getGeometry()->getAnimGraphOverrideUrl();
           initAnimGraph();
           _skeletonModelLoaded = true;
       }
       QObject::disconnect(*skeletonConnection);
    });
    
    saveAvatarUrl();
    emit skeletonChanged();
}

bool isWearableEntity(const EntityItemPointer& entity) {
    return entity->isVisible()
        && (entity->getParentID() == DependencyManager::get<NodeList>()->getSessionUUID()
            || entity->getParentID() == AVATAR_SELF_ID);
}

void MyAvatar::clearAvatarEntities() {
    auto treeRenderer = DependencyManager::get<EntityTreeRenderer>();
    EntityTreePointer entityTree = treeRenderer ? treeRenderer->getTree() : nullptr;

    QList<QUuid> avatarEntityIDs;
    _avatarEntitiesLock.withReadLock([&] {
            avatarEntityIDs = _packedAvatarEntityData.keys();
    });
    for (const auto& entityID : avatarEntityIDs) {
        entityTree->withWriteLock([&entityID, &entityTree] {
            // remove this entity first from the entity tree
            entityTree->deleteEntity(entityID, true, true);
        });

        // remove the avatar entity from our internal list
        // (but indicate it doesn't need to be pulled from the tree)
        clearAvatarEntity(entityID, false);
    }
}

void MyAvatar::removeWearableAvatarEntities() {
    auto treeRenderer = DependencyManager::get<EntityTreeRenderer>();
    EntityTreePointer entityTree = treeRenderer ? treeRenderer->getTree() : nullptr;
    if (entityTree) {
        QList<QUuid> avatarEntityIDs;
        _avatarEntitiesLock.withReadLock([&] {
                avatarEntityIDs = _packedAvatarEntityData.keys();
        });
        for (const auto& entityID : avatarEntityIDs) {
            auto entity = entityTree->findEntityByID(entityID);
            if (entity && isWearableEntity(entity)) {
                entityTree->withWriteLock([&entityID, &entityTree] {
                    // remove this entity first from the entity tree
                    entityTree->deleteEntity(entityID, true, true);
                });

                // remove the avatar entity from our internal list
                // (but indicate it doesn't need to be pulled from the tree)
                clearAvatarEntity(entityID, false);
            }
        }
    }
}

QVariantList MyAvatar::getAvatarEntitiesVariant() {
    // NOTE: this method is NOT efficient
    QVariantList avatarEntitiesData;
    auto treeRenderer = DependencyManager::get<EntityTreeRenderer>();
    EntityTreePointer entityTree = treeRenderer ? treeRenderer->getTree() : nullptr;
    if (entityTree) {
        QList<QUuid> avatarEntityIDs;
        _avatarEntitiesLock.withReadLock([&] {
                avatarEntityIDs = _packedAvatarEntityData.keys();
        });
        for (const auto& entityID : avatarEntityIDs) {
            auto entity = entityTree->findEntityByID(entityID);
            if (!entity) {
                continue;
            }
            QVariantMap avatarEntityData;
            EncodeBitstreamParams params;
            auto desiredProperties = entity->getEntityProperties(params);
            desiredProperties += PROP_LOCAL_POSITION;
            desiredProperties += PROP_LOCAL_ROTATION;
            EntityItemProperties entityProperties = entity->getProperties(desiredProperties);
            QScriptValue scriptProperties = EntityItemPropertiesToScriptValue(_myScriptEngine, entityProperties);
            avatarEntityData["properties"] = scriptProperties.toVariant();
            avatarEntitiesData.append(QVariant(avatarEntityData));
        }
    }
    return avatarEntitiesData;
}


void MyAvatar::resetFullAvatarURL() {
    auto lastAvatarURL = getFullAvatarURLFromPreferences();
    auto lastAvatarName = getFullAvatarModelName();
    useFullAvatarURL(QUrl());
    useFullAvatarURL(lastAvatarURL, lastAvatarName);
}

void MyAvatar::useFullAvatarURL(const QUrl& fullAvatarURL, const QString& modelName) {

    if (QThread::currentThread() != thread()) {
        BLOCKING_INVOKE_METHOD(this, "useFullAvatarURL",
                                  Q_ARG(const QUrl&, fullAvatarURL),
                                  Q_ARG(const QString&, modelName));
        return;
    }

    if (_fullAvatarURLFromPreferences != fullAvatarURL) {
        _fullAvatarURLFromPreferences = fullAvatarURL;
        _fullAvatarModelName = modelName;
    }

    const QString& urlString = fullAvatarURL.toString();
    if (urlString.isEmpty() || (fullAvatarURL != getSkeletonModelURL())) {
        setSkeletonModelURL(fullAvatarURL);
        UserActivityLogger::getInstance().changedModel("skeleton", urlString);
    }
}

glm::vec3 MyAvatar::getSkeletonPosition() const {
    CameraMode mode = qApp->getCamera().getMode();
    if (mode == CAMERA_MODE_THIRD_PERSON || mode == CAMERA_MODE_INDEPENDENT) {
        // The avatar is rotated PI about the yAxis, so we have to correct for it
        // to get the skeleton offset contribution in the world-frame.
        const glm::quat FLIP = glm::angleAxis(PI, glm::vec3(0.0f, 1.0f, 0.0f));
        return getWorldPosition() + getWorldOrientation() * FLIP * _skeletonOffset;
    }
    return Avatar::getWorldPosition();
}

void MyAvatar::rebuildCollisionShape() {
    // compute localAABox
    float scale = getModelScale();
    float radius = scale * _skeletonModel->getBoundingCapsuleRadius();
    float height = scale * _skeletonModel->getBoundingCapsuleHeight() + 2.0f * radius;
    glm::vec3 corner(-radius, -0.5f * height, -radius);
    corner += scale * _skeletonModel->getBoundingCapsuleOffset();
    glm::vec3 diagonal(2.0f * radius, height, 2.0f * radius);
    _characterController.setLocalBoundingBox(corner, diagonal);
}

void MyAvatar::setControllerPoseInSensorFrame(controller::Action action, const controller::Pose& pose) {
    std::lock_guard<std::mutex> guard(_controllerPoseMapMutex);
    auto iter = _controllerPoseMap.find(action);
    if (iter != _controllerPoseMap.end()) {
        iter->second = pose;
    } else {
        _controllerPoseMap.insert({ action, pose });
    }
}

controller::Pose MyAvatar::getControllerPoseInSensorFrame(controller::Action action) const {
    std::lock_guard<std::mutex> guard(_controllerPoseMapMutex);
    auto iter = _controllerPoseMap.find(action);
    if (iter != _controllerPoseMap.end()) {
        return iter->second;
    } else {
        return controller::Pose(); // invalid pose
    }
}

controller::Pose MyAvatar::getControllerPoseInWorldFrame(controller::Action action) const {
    auto pose = getControllerPoseInSensorFrame(action);
    if (pose.valid) {
        return pose.transform(getSensorToWorldMatrix());
    } else {
        return controller::Pose(); // invalid pose
    }
}

controller::Pose MyAvatar::getControllerPoseInAvatarFrame(controller::Action action) const {
    auto pose = getControllerPoseInWorldFrame(action);
    if (pose.valid) {
        glm::mat4 invAvatarMatrix = glm::inverse(createMatFromQuatAndPos(getWorldOrientation(), getWorldPosition()));
        return pose.transform(invAvatarMatrix);
    } else {
        return controller::Pose(); // invalid pose
    }
}

void MyAvatar::updateMotors() {
    _characterController.clearMotors();
    glm::quat motorRotation;

    const float FLYING_MOTOR_TIMESCALE = 0.05f;
    const float WALKING_MOTOR_TIMESCALE = 0.2f;
    const float INVALID_MOTOR_TIMESCALE = 1.0e6f;

    float horizontalMotorTimescale;
    float verticalMotorTimescale;

    if (_characterController.getState() == CharacterController::State::Hover ||
            _characterController.computeCollisionGroup() == BULLET_COLLISION_GROUP_COLLISIONLESS) {
        horizontalMotorTimescale = FLYING_MOTOR_TIMESCALE;
        verticalMotorTimescale = FLYING_MOTOR_TIMESCALE;
    } else {
        horizontalMotorTimescale = WALKING_MOTOR_TIMESCALE * getSensorToWorldScale();
        verticalMotorTimescale = INVALID_MOTOR_TIMESCALE;
    }

    if (_motionBehaviors & AVATAR_MOTION_ACTION_MOTOR_ENABLED) {
        if (_characterController.getState() == CharacterController::State::Hover ||
                _characterController.computeCollisionGroup() == BULLET_COLLISION_GROUP_COLLISIONLESS) {
            motorRotation = getMyHead()->getHeadOrientation();
        } else {
            // non-hovering = walking: follow camera twist about vertical but not lift
            // we decompose camera's rotation and store the twist part in motorRotation
            // however, we need to perform the decomposition in the avatar-frame
            // using the local UP axis and then transform back into world-frame
            glm::quat orientation = getWorldOrientation();
            glm::quat headOrientation = glm::inverse(orientation) * getMyHead()->getHeadOrientation(); // avatar-frame
            glm::quat liftRotation;
            swingTwistDecomposition(headOrientation, Vectors::UNIT_Y, liftRotation, motorRotation);
            motorRotation = orientation * motorRotation;
        }

        if (_isPushing || _isBraking || !_isBeingPushed) {
            _characterController.addMotor(_actionMotorVelocity, motorRotation, horizontalMotorTimescale, verticalMotorTimescale);
        } else {
            // _isBeingPushed must be true --> disable action motor by giving it a long timescale,
            // otherwise it's attempt to "stand in in place" could defeat scripted motor/thrusts
            _characterController.addMotor(_actionMotorVelocity, motorRotation, INVALID_MOTOR_TIMESCALE);
        }
    }
    if (_motionBehaviors & AVATAR_MOTION_SCRIPTED_MOTOR_ENABLED) {
        if (_scriptedMotorFrame == SCRIPTED_MOTOR_CAMERA_FRAME) {
            motorRotation = getMyHead()->getHeadOrientation() * glm::angleAxis(PI, Vectors::UNIT_Y);
        } else if (_scriptedMotorFrame == SCRIPTED_MOTOR_AVATAR_FRAME) {
            motorRotation = getWorldOrientation() * glm::angleAxis(PI, Vectors::UNIT_Y);
        } else {
            // world-frame
            motorRotation = glm::quat();
        }
        if (_scriptedMotorMode == SCRIPTED_MOTOR_SIMPLE_MODE) {
            _characterController.addMotor(_scriptedMotorVelocity, motorRotation, _scriptedMotorTimescale);
        } else {
            // dynamic mode
            _characterController.addMotor(_scriptedMotorVelocity, motorRotation, horizontalMotorTimescale, verticalMotorTimescale);
        }
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
    _characterController.setScaleFactor(getSensorToWorldScale());

    _characterController.setPositionAndOrientation(getWorldPosition(), getWorldOrientation());
    auto headPose = getControllerPoseInAvatarFrame(controller::Action::HEAD);
    if (headPose.isValid()) {
        _follow.prePhysicsUpdate(*this, deriveBodyFromHMDSensor(), _bodySensorMatrix, hasDriveInput());
    } else {
        _follow.deactivate();
    }

    _prePhysicsRoomPose = AnimPose(_sensorToWorldMatrix);
}

// There are a number of possible strategies for this set of tools through endRender, below.
void MyAvatar::nextAttitude(glm::vec3 position, glm::quat orientation) {
    bool success;
    Transform trans = getTransform(success);
    if (!success) {
        qCWarning(interfaceapp) << "Warning -- MyAvatar::nextAttitude failed";
        return;
    }
    trans.setTranslation(position);
    trans.setRotation(orientation);
    SpatiallyNestable::setTransform(trans, success);
    if (!success) {
        qCWarning(interfaceapp) << "Warning -- MyAvatar::nextAttitude failed";
    }
    updateAttitude(orientation);
}

void MyAvatar::harvestResultsFromPhysicsSimulation(float deltaTime) {
    glm::vec3 position;
    glm::quat orientation;
    if (_characterController.isEnabledAndReady()) {
        _characterController.getPositionAndOrientation(position, orientation);
    } else {
        position = getWorldPosition();
        orientation = getWorldOrientation();
    }
    nextAttitude(position, orientation);
    _bodySensorMatrix = _follow.postPhysicsUpdate(*this, _bodySensorMatrix);

    if (_characterController.isEnabledAndReady()) {
        setWorldVelocity(_characterController.getLinearVelocity() + _characterController.getFollowVelocity());
        if (_characterController.isStuck()) {
            _physicsSafetyPending = true;
            _goToPosition = getWorldPosition();
        }
    } else {
        setWorldVelocity(getWorldVelocity() + _characterController.getFollowVelocity());
    }
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

QString MyAvatar::getScriptedMotorMode() const {
    QString mode = "simple";
    if (_scriptedMotorMode == SCRIPTED_MOTOR_DYNAMIC_MODE) {
        mode = "dynamic";
    }
    return mode;
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

void MyAvatar::setScriptedMotorMode(QString mode) {
    if (mode.toLower() == "simple") {
        _scriptedMotorMode = SCRIPTED_MOTOR_SIMPLE_MODE;
    } else if (mode.toLower() == "dynamic") {
        _scriptedMotorMode = SCRIPTED_MOTOR_DYNAMIC_MODE;
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
        BLOCKING_INVOKE_METHOD(this, "attach",
            Q_ARG(const QString&, modelURL), 
            Q_ARG(const QString&, jointName), 
            Q_ARG(const glm::vec3&, translation), 
            Q_ARG(const glm::quat&, rotation),
            Q_ARG(float, scale),
            Q_ARG(bool, isSoft),
            Q_ARG(bool, allowDuplicates),
            Q_ARG(bool, useSaved)
        );
        return;
    }
    AttachmentData data;
    data.modelURL = modelURL;
    data.jointName = jointName;
    data.translation = translation;
    data.rotation = rotation;
    data.scale = scale;
    data.isSoft = isSoft;
    EntityItemProperties properties;
    attachmentDataToEntityProperties(data, properties);
    DependencyManager::get<EntityScriptingInterface>()->addEntity(properties, true);
    emit attachmentsChanged();
}

void MyAvatar::detachOne(const QString& modelURL, const QString& jointName) {
    if (QThread::currentThread() != thread()) {
        BLOCKING_INVOKE_METHOD(this, "detachOne",
            Q_ARG(const QString&, modelURL),
            Q_ARG(const QString&, jointName)
        );
        return;
    }
    QUuid entityID;
    if (findAvatarEntity(modelURL, jointName, entityID)) {
        DependencyManager::get<EntityScriptingInterface>()->deleteEntity(entityID);
    }
    emit attachmentsChanged();
}

void MyAvatar::detachAll(const QString& modelURL, const QString& jointName) {
    if (QThread::currentThread() != thread()) {
        BLOCKING_INVOKE_METHOD(this, "detachAll",
            Q_ARG(const QString&, modelURL),
            Q_ARG(const QString&, jointName)
        );
        return;
    }
    QUuid entityID;
    while (findAvatarEntity(modelURL, jointName, entityID)) {
        DependencyManager::get<EntityScriptingInterface>()->deleteEntity(entityID);
    }
    emit attachmentsChanged();
}

void MyAvatar::setAttachmentData(const QVector<AttachmentData>& attachmentData) {
    if (QThread::currentThread() != thread()) {
        BLOCKING_INVOKE_METHOD(this, "setAttachmentData",
            Q_ARG(const QVector<AttachmentData>&, attachmentData));
        return;
    }
    std::vector<EntityItemProperties> newEntitiesProperties;
    for (auto& data : attachmentData) {
        QUuid entityID;
        EntityItemProperties properties;
        if (findAvatarEntity(data.modelURL.toString(), data.jointName, entityID)) {
            properties = DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(entityID);
        }
        attachmentDataToEntityProperties(data, properties);
        newEntitiesProperties.push_back(properties);
    }

    // clear any existing avatar entities
    clearAvatarEntities();

    for (auto& properties : newEntitiesProperties) {
        DependencyManager::get<EntityScriptingInterface>()->addEntity(properties, true);
    }
    emit attachmentsChanged();
}

QVector<AttachmentData> MyAvatar::getAttachmentData() const {    
    QVector<AttachmentData> attachmentData;
    QList<QUuid> avatarEntityIDs;
    _avatarEntitiesLock.withReadLock([&] {
        avatarEntityIDs = _packedAvatarEntityData.keys();
    });
    for (const auto& entityID : avatarEntityIDs) {
        auto properties = DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(entityID);
        AttachmentData data = entityPropertiesToAttachmentData(properties);
        attachmentData.append(data);
    }
    return attachmentData;
}

QVariantList MyAvatar::getAttachmentsVariant() const {
    QVariantList result;
    for (const auto& attachment : getAttachmentData()) {
        result.append(attachment.toVariant());
    }
    return result;
}

void MyAvatar::setAttachmentsVariant(const QVariantList& variant) {
    if (QThread::currentThread() != thread()) {
        BLOCKING_INVOKE_METHOD(this, "setAttachmentsVariant",
            Q_ARG(const QVariantList&, variant));
        return;
    }
    QVector<AttachmentData> newAttachments;
    newAttachments.reserve(variant.size());
    for (const auto& attachmentVar : variant) {
        AttachmentData attachment;
        if (attachment.fromVariant(attachmentVar)) {
            newAttachments.append(attachment);
        }
    }
    setAttachmentData(newAttachments);   
}

bool MyAvatar::findAvatarEntity(const QString& modelURL, const QString& jointName, QUuid& entityID) {
    QList<QUuid> avatarEntityIDs;
    _avatarEntitiesLock.withReadLock([&] {
            avatarEntityIDs = _packedAvatarEntityData.keys();
    });
    for (const auto& entityID : avatarEntityIDs) {
        auto props = DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(entityID);
        if (props.getModelURL() == modelURL &&
            (jointName.isEmpty() || props.getParentJointIndex() == getJointIndex(jointName))) {
            return true;
        }
    }
    return false;
}

AttachmentData MyAvatar::entityPropertiesToAttachmentData(const EntityItemProperties& properties) const {
    AttachmentData data;
    data.modelURL = properties.getModelURL();
    data.translation = properties.getLocalPosition();
    data.rotation = properties.getLocalRotation();
    data.isSoft = properties.getRelayParentJoints();
    int jointIndex = (int)properties.getParentJointIndex();
    if (jointIndex > -1 && jointIndex < getJointNames().size()) {
        data.jointName = getJointNames()[jointIndex];
    }
    return data;
}

void MyAvatar::attachmentDataToEntityProperties(const AttachmentData& data, EntityItemProperties& properties) {
    QString url = data.modelURL.toString();
    properties.setName(QFileInfo(url).baseName());
    properties.setType(EntityTypes::Model);
    properties.setParentID(AVATAR_SELF_ID);
    properties.setLocalPosition(data.translation);
    properties.setLocalRotation(data.rotation);
    if (!data.isSoft) {
        properties.setParentJointIndex(getJointIndex(data.jointName));
    } else {
        properties.setRelayParentJoints(true);
    }
    properties.setModelURL(url);
}

void MyAvatar::initHeadBones() {
    int neckJointIndex = -1;
    if (_skeletonModel->isLoaded()) {
        neckJointIndex = getJointIndex("Neck");
    }
    if (neckJointIndex == -1) {
        neckJointIndex = (getJointIndex("Head") - 1);
        if (neckJointIndex < 0) {
            // return if the head is not even there. can't cauterize!!
            return;
        }
    }
    _headBoneSet.clear();
    std::queue<int> q;
    q.push(neckJointIndex);
    _headBoneSet.insert(neckJointIndex);

    // hfmJoints only hold links to parents not children, so we have to do a bit of extra work here.
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

    _cauterizationNeedsUpdate = true;
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

    emit animGraphUrlChanged(url);

    destroyAnimGraph();
    _skeletonModel->reset(); // Why is this necessary? Without this, we crash in the next render.

    _currentAnimGraphUrl.set(url);
    _skeletonModel->getRig().initAnimGraph(url);
    connect(&(_skeletonModel->getRig()), SIGNAL(onLoadComplete()), this, SLOT(animGraphLoaded()));
}

void MyAvatar::initAnimGraph() {
    QUrl graphUrl;
    if (!_prefOverrideAnimGraphUrl.get().isEmpty()) {
        graphUrl = _prefOverrideAnimGraphUrl.get();
    } else if (!_fstAnimGraphOverrideUrl.isEmpty()) {
        graphUrl = _fstAnimGraphOverrideUrl;
    } else {
        graphUrl = PathUtils::resourcesUrl("avatar/avatar-animation.json");
    }

    emit animGraphUrlChanged(graphUrl);

    _skeletonModel->getRig().initAnimGraph(graphUrl);
    _currentAnimGraphUrl.set(graphUrl);
    connect(&(_skeletonModel->getRig()), SIGNAL(onLoadComplete()), this, SLOT(animGraphLoaded()));
}

void MyAvatar::destroyAnimGraph() {
    _skeletonModel->getRig().destroyAnimGraph();
}

void MyAvatar::animGraphLoaded() {
    _bodySensorMatrix = deriveBodyFromHMDSensor(); // Based on current cached HMD position/rotation..
    updateSensorToWorldMatrix(); // Uses updated position/orientation and _bodySensorMatrix changes
    _isAnimatingScale = true;
    _cauterizationNeedsUpdate = true;
    disconnect(&(_skeletonModel->getRig()), SIGNAL(onLoadComplete()), this, SLOT(animGraphLoaded()));
}

void MyAvatar::postUpdate(float deltaTime, const render::ScenePointer& scene) {

    Avatar::postUpdate(deltaTime, scene);
    if (_enableDebugDrawDefaultPose || _enableDebugDrawAnimPose) {

        auto animSkeleton = _skeletonModel->getRig().getAnimSkeleton();

        // the rig is in the skeletonModel frame
        AnimPose xform(glm::vec3(1), _skeletonModel->getRotation(), _skeletonModel->getTranslation());

        if (_enableDebugDrawDefaultPose && animSkeleton) {
            glm::vec4 gray(0.2f, 0.2f, 0.2f, 0.2f);
            AnimDebugDraw::getInstance().addAbsolutePoses("myAvatarDefaultPoses", animSkeleton, _skeletonModel->getRig().getAbsoluteDefaultPoses(), xform, gray);
        }

        if (_enableDebugDrawAnimPose && animSkeleton) {
            // build absolute AnimPoseVec from rig
            AnimPoseVec absPoses;
            const Rig& rig = _skeletonModel->getRig();
            absPoses.reserve(rig.getJointStateCount());
            for (int i = 0; i < rig.getJointStateCount(); i++) {
                absPoses.push_back(AnimPose(rig.getJointTransform(i)));
            }
            glm::vec4 cyan(0.1f, 0.6f, 0.6f, 1.0f);
            AnimDebugDraw::getInstance().addAbsolutePoses("myAvatarAnimPoses", animSkeleton, absPoses, xform, cyan);
        }
    }

    if (_enableDebugDrawHandControllers) {
        auto leftHandPose = getControllerPoseInWorldFrame(controller::Action::LEFT_HAND);
        auto rightHandPose = getControllerPoseInWorldFrame(controller::Action::RIGHT_HAND);

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

    DebugDraw::getInstance().updateMyAvatarPos(getWorldPosition());
    DebugDraw::getInstance().updateMyAvatarRot(getWorldOrientation());

    AnimPose postUpdateRoomPose(_sensorToWorldMatrix);

    updateHoldActions(_prePhysicsRoomPose, postUpdateRoomPose);

    if (_enableDebugDrawDetailedCollision) {
        AnimPose rigToWorldPose(glm::vec3(1.0f), getWorldOrientation() * Quaternions::Y_180, getWorldPosition());
        const int NUM_DEBUG_COLORS = 8;
        const glm::vec4 DEBUG_COLORS[NUM_DEBUG_COLORS] = {
            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
            glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
            glm::vec4(0.25f, 0.25f, 1.0f, 1.0f),
            glm::vec4(1.0f, 1.0f, 0.0f, 1.0f),
            glm::vec4(0.25f, 1.0f, 1.0f, 1.0f),
            glm::vec4(1.0f, 0.25f, 1.0f, 1.0f),
            glm::vec4(1.0f, 0.65f, 0.0f, 1.0f)  // Orange you glad I added this color?
        };

        if (_skeletonModel && _skeletonModel->isLoaded()) {
            const Rig& rig = _skeletonModel->getRig();
            const HFMModel& hfmModel = _skeletonModel->getHFMModel();
            for (int i = 0; i < rig.getJointStateCount(); i++) {
                AnimPose jointPose;
                rig.getAbsoluteJointPoseInRigFrame(i, jointPose);
                const HFMJointShapeInfo& shapeInfo = hfmModel.joints[i].shapeInfo;
                const AnimPose pose = rigToWorldPose * jointPose;
                for (size_t j = 0; j < shapeInfo.debugLines.size() / 2; j++) {
                    glm::vec3 pointA = pose.xformPoint(shapeInfo.debugLines[2 * j]);
                    glm::vec3 pointB = pose.xformPoint(shapeInfo.debugLines[2 * j + 1]);
                    DebugDraw::getInstance().drawRay(pointA, pointB, DEBUG_COLORS[i % NUM_DEBUG_COLORS]);
                }
            }
        }
    }
}

void MyAvatar::preDisplaySide(const RenderArgs* renderArgs) {

    // toggle using the cauterizedBones depending on where the camera is and the rendering pass type.
    const bool shouldDrawHead = shouldRenderHead(renderArgs);
    if (shouldDrawHead != _prevShouldDrawHead) {
        _cauterizationNeedsUpdate = true;
        _skeletonModel->setEnableCauterization(!shouldDrawHead);

        for (int i = 0; i < _attachmentData.size(); i++) {
            if (_attachmentData[i].jointName.compare("Head", Qt::CaseInsensitive) == 0 ||
                _attachmentData[i].jointName.compare("Neck", Qt::CaseInsensitive) == 0 ||
                _attachmentData[i].jointName.compare("LeftEye", Qt::CaseInsensitive) == 0 ||
                _attachmentData[i].jointName.compare("RightEye", Qt::CaseInsensitive) == 0 ||
                _attachmentData[i].jointName.compare("HeadTop_End", Qt::CaseInsensitive) == 0 ||
                _attachmentData[i].jointName.compare("Face", Qt::CaseInsensitive) == 0) {
                uint8_t modelRenderTagBits = shouldDrawHead ? render::hifi::TAG_ALL_VIEWS : render::hifi::TAG_SECONDARY_VIEW;

                _attachmentModels[i]->setTagMask(modelRenderTagBits);
                _attachmentModels[i]->setGroupCulled(false);
                _attachmentModels[i]->setCanCastShadow(true);
                _attachmentModels[i]->setVisibleInScene(true, qApp->getMain3DScene());
            }
        }
    }
    _prevShouldDrawHead = shouldDrawHead;
}

const float RENDER_HEAD_CUTOFF_DISTANCE = 0.47f;

bool MyAvatar::cameraInsideHead(const glm::vec3& cameraPosition) const {
    return glm::length(cameraPosition - getHeadPosition()) < (RENDER_HEAD_CUTOFF_DISTANCE * getModelScale());
}

bool MyAvatar::shouldRenderHead(const RenderArgs* renderArgs) const {
    bool defaultMode = renderArgs->_renderMode == RenderArgs::DEFAULT_RENDER_MODE;
    bool firstPerson = qApp->getCamera().getMode() == CAMERA_MODE_FIRST_PERSON;
    bool insideHead = cameraInsideHead(renderArgs->getViewFrustum().getPosition());
    return !defaultMode || !firstPerson || !insideHead;
}

void MyAvatar::setHasScriptedBlendshapes(bool hasScriptedBlendshapes) {
    if (hasScriptedBlendshapes == _hasScriptedBlendShapes) {
        return;
    }
    if (!hasScriptedBlendshapes) {
        // send a forced avatarData update to make sure the script can send neutal blendshapes on unload
        // without having to wait for the update loop, make sure _hasScriptedBlendShapes is still true
        // before sending the update, or else it won't send the neutal blendshapes to the receiving clients
        sendAvatarDataPacket(true);
    }
    _hasScriptedBlendShapes = hasScriptedBlendshapes;
}

void MyAvatar::setHasProceduralBlinkFaceMovement(bool hasProceduralBlinkFaceMovement) {
    _headData->setHasProceduralBlinkFaceMovement(hasProceduralBlinkFaceMovement);
}

void MyAvatar::setHasProceduralEyeFaceMovement(bool hasProceduralEyeFaceMovement) {
    _headData->setHasProceduralEyeFaceMovement(hasProceduralEyeFaceMovement);
}

void MyAvatar::setHasAudioEnabledFaceMovement(bool hasAudioEnabledFaceMovement) {
    _headData->setHasAudioEnabledFaceMovement(hasAudioEnabledFaceMovement);
}

void MyAvatar::setRotationRecenterFilterLength(float length) {
    const float MINIMUM_ROTATION_RECENTER_FILTER_LENGTH = 0.01f;
    _rotationRecenterFilterLength = std::max(MINIMUM_ROTATION_RECENTER_FILTER_LENGTH, length);
}

void MyAvatar::setRotationThreshold(float angleRadians) {
    _rotationThreshold = angleRadians;
}

void MyAvatar::updateOrientation(float deltaTime) {

    //  Smoothly rotate body with arrow keys
    float targetSpeed = getDriveKey(YAW) * _yawSpeed;
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
    bool snapTurn = false;
    if (getDriveKey(STEP_YAW) != 0.0f) {
        totalBodyYaw += getDriveKey(STEP_YAW);
        snapTurn = true;
    }

    // Use head/HMD roll to turn while flying, but not when standing still.
    if (qApp->isHMDMode() && getCharacterController()->getState() == CharacterController::State::Hover && _hmdRollControlEnabled && hasDriveInput()) {

        // Turn with head roll.
        const float MIN_CONTROL_SPEED = 2.0f * getSensorToWorldScale();  // meters / sec
        const glm::vec3 characterForward = getWorldOrientation() * Vectors::UNIT_NEG_Z;
        float forwardSpeed = glm::dot(characterForward, getWorldVelocity());

        // only enable roll-turns if we are moving forward or backward at greater then MIN_CONTROL_SPEED
        if (fabsf(forwardSpeed) >= MIN_CONTROL_SPEED) {

            float direction = forwardSpeed > 0.0f ? 1.0f : -1.0f;
            float rollAngle = glm::degrees(asinf(glm::dot(IDENTITY_UP, _hmdSensorOrientation * IDENTITY_RIGHT)));
            float rollSign = rollAngle < 0.0f ? -1.0f : 1.0f;
            rollAngle = fabsf(rollAngle);

            const float MIN_ROLL_ANGLE = _hmdRollControlDeadZone;
            const float MAX_ROLL_ANGLE = 90.0f;  // degrees

            if (rollAngle > MIN_ROLL_ANGLE) {
                // rate of turning is linearly proportional to rollAngle
                rollAngle = glm::clamp(rollAngle, MIN_ROLL_ANGLE, MAX_ROLL_ANGLE);

                // scale rollAngle into a value from zero to one.
                float rollFactor = (rollAngle - MIN_ROLL_ANGLE) / (MAX_ROLL_ANGLE - MIN_ROLL_ANGLE);

                float angularSpeed = rollSign * rollFactor * _hmdRollControlRate;
                totalBodyYaw += direction * angularSpeed * deltaTime;
            }
        }
    }

    // update body orientation by movement inputs
    glm::quat initialOrientation = getOrientationOutbound();
    setWorldOrientation(getWorldOrientation() * glm::quat(glm::radians(glm::vec3(0.0f, totalBodyYaw, 0.0f))));

    if (snapTurn) {
        // Whether or not there is an existing smoothing going on, just reset the smoothing timer and set the starting position as the avatar's current position, then smooth to the new position.
        _smoothOrientationInitial = initialOrientation;
        _smoothOrientationTarget = getWorldOrientation();
        _smoothOrientationTimer = 0.0f;
    }

    Head* head = getHead();
    auto headPose = getControllerPoseInAvatarFrame(controller::Action::HEAD);
    if (headPose.isValid()) {
        glm::quat localOrientation = headPose.rotation * Quaternions::Y_180;
        // these angles will be in radians
        // ... so they need to be converted to degrees before we do math...
        glm::vec3 euler = glm::eulerAngles(localOrientation) * DEGREES_PER_RADIAN;

        Head* head = getHead();
        head->setBaseYaw(YAW(euler));
        head->setBasePitch(PITCH(euler));
        head->setBaseRoll(ROLL(euler));
    } else {
        head->setBaseYaw(0.0f);
        head->setBasePitch(getHead()->getBasePitch() + getDriveKey(PITCH) * _pitchSpeed * deltaTime);
        head->setBaseRoll(0.0f);
    }
}

static float scaleSpeedByDirection(const glm::vec2 velocityDirection, const float forwardSpeed, const float backwardSpeed) {
    // for the elipse function -->  (x^2)/(backwardSpeed*backwardSpeed) + y^2/(forwardSpeed*forwardSpeed) = 1,  scale == y^2 when x is 0
    float fwdScale = forwardSpeed * forwardSpeed;
    float backScale = backwardSpeed * backwardSpeed;
    float scaledX = velocityDirection.x * backwardSpeed;
    float scaledSpeed = forwardSpeed;
    if (velocityDirection.y < 0.0f) {
        if (backScale > 0.0f) {
            float yValue = sqrtf(fwdScale * (1.0f - ((scaledX * scaledX) / backScale)));
            scaledSpeed = sqrtf((scaledX * scaledX) + (yValue * yValue));
        }
    } else {
        scaledSpeed = backwardSpeed;
    }
    return scaledSpeed;
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

    CharacterController::State state = _characterController.getState();

    // compute action input
    glm::vec3 forward = (getDriveKey(TRANSLATE_Z)) * IDENTITY_FORWARD;
    glm::vec3 right = (getDriveKey(TRANSLATE_X)) * IDENTITY_RIGHT;

    glm::vec3 direction = forward + right;
    if (state == CharacterController::State::Hover ||
            _characterController.computeCollisionGroup() == BULLET_COLLISION_GROUP_COLLISIONLESS) {
        glm::vec3 up = (getDriveKey(TRANSLATE_Y)) * IDENTITY_UP;
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
        float finalMaxMotorSpeed = getSensorToWorldScale() * DEFAULT_AVATAR_MAX_FLYING_SPEED * _walkSpeedScalar;
        float speedGrowthTimescale  = 2.0f;
        float speedIncreaseFactor = 1.8f * _walkSpeedScalar;
        motorSpeed *= 1.0f + glm::clamp(deltaTime / speedGrowthTimescale, 0.0f, 1.0f) * speedIncreaseFactor;
        const float maxBoostSpeed = getSensorToWorldScale() * MAX_BOOST_SPEED;

        if (_isPushing) {
            if (motorSpeed < maxBoostSpeed) {
                // an active action motor should never be slower than this
                float boostCoefficient = (maxBoostSpeed - motorSpeed) / maxBoostSpeed;
                motorSpeed += getSensorToWorldScale() * MIN_AVATAR_SPEED * boostCoefficient;
            } else if (motorSpeed > finalMaxMotorSpeed) {
                motorSpeed = finalMaxMotorSpeed;
            }
        }
        _actionMotorVelocity = motorSpeed * direction;
    } else {
        // we're interacting with a floor --> simple horizontal speed and exponential decay
        const glm::vec2 currentVel = { direction.x, direction.z };
        float scaledSpeed = scaleSpeedByDirection(currentVel, _walkSpeed.get(), _walkBackwardSpeed.get());
        // _walkSpeedScalar is a multiplier if we are in sprint mode, otherwise 1.0
        _actionMotorVelocity = getSensorToWorldScale() * (scaledSpeed * _walkSpeedScalar)  * direction;
    }

    float previousBoomLength = _boomLength;
    float boomChange = getDriveKey(ZOOM);
    _boomLength += 2.0f * _boomLength * boomChange + boomChange * boomChange;
    _boomLength = glm::clamp<float>(_boomLength, ZOOM_MIN, ZOOM_MAX);

    // May need to change view if boom length has changed
    if (previousBoomLength != _boomLength) {
        qApp->changeViewAsNeeded(_boomLength);
    }
}

void MyAvatar::updatePosition(float deltaTime) {
    if (_motionBehaviors & AVATAR_MOTION_ACTION_MOTOR_ENABLED) {
        updateActionMotor(deltaTime);
    }

    vec3 velocity = getWorldVelocity();
    float sensorToWorldScale = getSensorToWorldScale();
    float sensorToWorldScale2 = sensorToWorldScale * sensorToWorldScale;
    const float MOVING_SPEED_THRESHOLD_SQUARED = 0.0001f; // 0.01 m/s
    if (!_characterController.isEnabledAndReady()) {
        // _characterController is not in physics simulation but it can still compute its target velocity
        updateMotors();
        _characterController.computeNewVelocity(deltaTime, velocity);

        float speed2 = glm::length(velocity);
        if (speed2 > sensorToWorldScale2 * MIN_AVATAR_SPEED_SQUARED) {
            // update position ourselves
            applyPositionDelta(deltaTime * velocity);
        }
        measureMotionDerivatives(deltaTime);
        _moving = speed2 > sensorToWorldScale2 * MOVING_SPEED_THRESHOLD_SQUARED;
    } else {
        float speed2 = glm::length2(velocity);
        _moving = speed2 > sensorToWorldScale2 * MOVING_SPEED_THRESHOLD_SQUARED;

        if (_moving) {
            // scan for walkability
            glm::vec3 position = getWorldPosition();
            MyCharacterController::RayShotgunResult result;
            glm::vec3 step = deltaTime * (getWorldOrientation() * _actionMotorVelocity);
            _characterController.testRayShotgun(position, step, result);
            _characterController.setStepUpEnabled(result.walkable);
        }
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

// There can be a separation between the _targetScale and the actual scale of the rendered avatar in a domain.
// When the avatar enters a domain where their target scale is not allowed according to the min/max
// we do not change their saved target scale. Instead, we use getDomainLimitedScale() to render the avatar
// at a domain appropriate size. When the avatar leaves the limiting domain, we'll return them to their previous target scale.
// While connected to a domain that limits avatar scale if the user manually changes their avatar scale, we change
// target scale to match the new scale they have chosen. When they leave the domain they will not return to the scale they were
// before they entered the limiting domain.

void MyAvatar::setGravity(float gravity) {
    _characterController.setGravity(gravity);
}

float MyAvatar::getGravity() {
    return _characterController.getGravity();
}

void MyAvatar::increaseSize() {
    float minScale = getDomainMinScale();
    float maxScale = getDomainMaxScale();

    float clampedTargetScale = glm::clamp(_targetScale, minScale, maxScale);
    float newTargetScale = glm::clamp(clampedTargetScale * (1.0f + SCALING_RATIO), minScale, maxScale);

    setTargetScale(newTargetScale);
}

void MyAvatar::decreaseSize() {
    float minScale = getDomainMinScale();
    float maxScale = getDomainMaxScale();

    float clampedTargetScale = glm::clamp(_targetScale, minScale, maxScale);
    float newTargetScale = glm::clamp(clampedTargetScale * (1.0f - SCALING_RATIO), minScale, maxScale);

    setTargetScale(newTargetScale);
}

void MyAvatar::resetSize() {
    // attempt to reset avatar size to the default (clamped to domain limits)
    const float DEFAULT_AVATAR_SCALE = 1.0f;
    setTargetScale(DEFAULT_AVATAR_SCALE);
}

void MyAvatar::restrictScaleFromDomainSettings(const QJsonObject& domainSettingsObject) {
    // pull out the minimum and maximum height and set them to restrict our scale

    static const QString AVATAR_SETTINGS_KEY = "avatars";
    auto avatarsObject = domainSettingsObject[AVATAR_SETTINGS_KEY].toObject();

    static const QString MIN_HEIGHT_OPTION = "min_avatar_height";
    float settingMinHeight = avatarsObject[MIN_HEIGHT_OPTION].toDouble(MIN_AVATAR_HEIGHT);
    setDomainMinimumHeight(settingMinHeight);

    static const QString MAX_HEIGHT_OPTION = "max_avatar_height";
    float settingMaxHeight = avatarsObject[MAX_HEIGHT_OPTION].toDouble(MAX_AVATAR_HEIGHT);
    setDomainMaximumHeight(settingMaxHeight);

    // make sure that the domain owner didn't flip min and max
    if (_domainMinimumHeight > _domainMaximumHeight) {
        std::swap(_domainMinimumHeight, _domainMaximumHeight);
    }

    // Set avatar current scale
    _targetScale = _scaleSetting.get();
    // clamp the desired _targetScale by the domain limits NOW, don't try to gracefully animate.  Because
    // this might cause our avatar to become embedded in the terrain.
    _targetScale = getDomainLimitedScale();

    qCDebug(interfaceapp) << "This domain requires a minimum avatar scale of " << _domainMinimumHeight
                          << " and a maximum avatar scale of " << _domainMaximumHeight;

    _isAnimatingScale = true;

    setModelScale(_targetScale);
    rebuildCollisionShape();

    _haveReceivedHeightLimitsFromDomain = true;
}

void MyAvatar::leaveDomain() {
    clearScaleRestriction();
    saveAvatarScale();
}

void MyAvatar::saveAvatarScale() {
    _scaleSetting.set(_targetScale);
}

void MyAvatar::clearScaleRestriction() {
    _domainMinimumHeight = MIN_AVATAR_HEIGHT;
    _domainMaximumHeight = MAX_AVATAR_HEIGHT;
    _haveReceivedHeightLimitsFromDomain = false;
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

void MyAvatar::goToFeetLocation(const glm::vec3& newPosition,
                                bool hasOrientation, const glm::quat& newOrientation,
                                bool shouldFaceLocation) {
    _goToFeetAjustment = true;
    goToLocation(newPosition, hasOrientation, newOrientation, shouldFaceLocation);
}

void MyAvatar::goToLocation(const glm::vec3& newPosition,
                            bool hasOrientation, const glm::quat& newOrientation,
                            bool shouldFaceLocation, bool withSafeLanding) {

    // Most cases of going to a place or user go through this now. Some possible improvements to think about in the future:
    // - It would be nice if this used the same teleport steps and smoothing as in the teleport.js script, as long as it
    //   still worked if the target is in the air.
    // - Sometimes (such as the response from /api/v1/users/:username/location), the location can be stale, but there is a
    //   node_id supplied by which we could update the information after going to the stale location first and "looking around".
    //   This could be passed through AddressManager::goToAddressFromObject => AddressManager::handleViewpoint => here.
    //   The trick is that you have to yield enough time to resolve the node_id.
    // - Instead of always doing the same thing for shouldFaceLocation -- which places users uncomfortabley on top of each other --
    //   it would be nice to see how many users are already "at" a person or place, and place ourself in semicircle or other shape
    //   around the target. Avatars and entities (specified by the node_id) could define an adjustable "face me" method that would
    //   compute the position (e.g., so that if I'm on stage, going to me would compute an available seat in the audience rather than
    //   being in my face on-stage). Note that this could work for going to an entity as well as to a person.

    qCDebug(interfaceapp).nospace() << "MyAvatar goToLocation - moving to " << newPosition.x << ", "
        << newPosition.y << ", " << newPosition.z;

    _goToPending = true;
    _goToPosition = newPosition;
    _goToSafe = withSafeLanding;
    _goToOrientation = getWorldOrientation();
    if (hasOrientation) {
        qCDebug(interfaceapp).nospace() << "MyAvatar goToLocation - new orientation is "
                                        << newOrientation.x << ", " << newOrientation.y << ", " << newOrientation.z << ", " << newOrientation.w;

        // orient the user to face the target
        glm::quat quatOrientation = cancelOutRollAndPitch(newOrientation);

        if (shouldFaceLocation) {
            quatOrientation = newOrientation * glm::angleAxis(PI, Vectors::UP);

            // move the user a couple units away
            const float DISTANCE_TO_USER = 2.0f;
            _goToPosition = newPosition - quatOrientation * IDENTITY_FORWARD * DISTANCE_TO_USER;
        }

        _goToOrientation = quatOrientation;
    }

    emit transformChanged();
}

void MyAvatar::goToLocationAndEnableCollisions(const glm::vec3& position) { // See use case in safeLanding.
    goToLocation(position);
    QMetaObject::invokeMethod(this, "setCollisionsEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
}
bool MyAvatar::safeLanding(const glm::vec3& position) {
    // Considers all collision hull or non-collisionless primitive intersections on a vertical line through the point.
    // There needs to be a "landing" if:
    // a) the closest above and the closest below are less than the avatar capsule height apart, or
    // b) the above point is the top surface of an entity, indicating that we are inside it.
    // If no landing is required, we go to that point directly and return false;
    // When a landing is required by a, we find the highest intersection on that closest-agbove entity
    // (which may be that same "nearest above intersection"). That highest intersection is the candidate landing point.
    // For b, use that top surface point.
    // We then place our feet there, recurse with new capsule center point, and return true.

    if (QThread::currentThread() != thread()) {
        bool result;
        BLOCKING_INVOKE_METHOD(this, "safeLanding", Q_RETURN_ARG(bool, result), Q_ARG(const glm::vec3&, position));
        return result;
    }
    glm::vec3 better;
    if (!requiresSafeLanding(position, better)) {
        return false;
    }
    if (!getCollisionsEnabled()) {
        goToLocation(better);  // recurses on next update
     } else { // If you try to go while stuck, physics will keep you stuck.
        setCollisionsEnabled(false);
        // Don't goToLocation just yet. Yield so that physics can act on the above.
        QMetaObject::invokeMethod(this, "goToLocationAndEnableCollisions", Qt::QueuedConnection, // The equivalent of javascript nextTick
            Q_ARG(glm::vec3, better));
     }
     return true;
}

// If position is not reliably safe from being stuck by physics, answer true and place a candidate better position in betterPositionOut.
bool MyAvatar::requiresSafeLanding(const glm::vec3& positionIn, glm::vec3& betterPositionOut) {

    // We begin with utilities and tests. The Algorithm in four parts is below.
    // NOTE: we use estimated avatar height here instead of the bullet capsule halfHeight, because
    // the domain avatar height limiting might not have taken effect yet on the actual bullet shape.
    auto halfHeight = 0.5f * getHeight();

    if (halfHeight == 0) {
        return false; // zero height avatar
    }
    auto entityTree = DependencyManager::get<EntityTreeRenderer>()->getTree();
    if (!entityTree) {
        return false; // no entity tree
    }
    // More utilities.
    const auto capsuleCenter = positionIn;
    const auto up = _worldUpDirection, down = -up;
    glm::vec3 upperIntersection, upperNormal, lowerIntersection, lowerNormal;
    EntityItemID upperId, lowerId;
    QVector<EntityItemID> include{}, ignore{};
    auto mustMove = [&] {  // Place bottom of capsule at the upperIntersection, and check again based on the capsule center.
        betterPositionOut = upperIntersection + (up * halfHeight);
        return true;
    };
    auto findIntersection = [&](const glm::vec3& startPointIn, const glm::vec3& directionIn, glm::vec3& intersectionOut, EntityItemID& entityIdOut, glm::vec3& normalOut) {
        OctreeElementPointer element;
        float distance;
        BoxFace face;
        const auto lockType = Octree::Lock; // Should we refactor to take a lock just once?
        bool* accurateResult = NULL;

        // This isn't quite what we really want here. findRayIntersection always works on mesh, skipping entirely based on collidable.
        // What we really want is to use the collision hull!
        // See https://highfidelity.fogbugz.com/f/cases/5003/findRayIntersection-has-option-to-use-collidableOnly-but-doesn-t-actually-use-colliders
        QVariantMap extraInfo;
        EntityItemID entityID = entityTree->evalRayIntersection(startPointIn, directionIn, include, ignore,
            PickFilter(PickFilter::getBitMask(PickFilter::FlagBit::COLLIDABLE) | PickFilter::getBitMask(PickFilter::FlagBit::PRECISE)),
            element, distance, face, normalOut, extraInfo, lockType, accurateResult);
        if (entityID.isNull()) {
            return false;
        }
        intersectionOut = startPointIn + (directionIn * distance);
        entityIdOut = entityID;
        return true;
    };

    // The Algorithm, in four parts:

    if (!findIntersection(capsuleCenter, up, upperIntersection, upperId, upperNormal)) {
        // We currently believe that physics will reliably push us out if our feet are embedded,
        // as long as our capsule center is out and there's room above us. Here we have those
        // conditions, so no need to check our feet below.
        return false; // nothing above
    }

    if (!findIntersection(capsuleCenter, down, lowerIntersection, lowerId, lowerNormal)) {
        // Our head may be embedded, but our center is out and there's room below. See corresponding comment above.
        return false; // nothing below
    }

    // See if we have room between entities above and below, but that we are not contained.
    // First check if the surface above us is the bottom of something, and the surface below us it the top of something.
    // I.e., we are in a clearing between two objects.
    if (isDown(upperNormal) && isUp(lowerNormal)) {
        auto spaceBetween = glm::distance(upperIntersection, lowerIntersection);
        const float halfHeightFactor = 2.25f; // Until case 5003 is fixed (and maybe after?), we need a fudge factor. Also account for content modelers not being precise.
        if (spaceBetween > (halfHeightFactor * halfHeight)) {
            // There is room for us to fit in that clearing. If there wasn't, physics would oscilate us between the objects above and below.
            // We're now going to iterate upwards through successive upperIntersections, testing to see if we're contained within the top surface of some entity.
            // There will be one of two outcomes:
            // a) We're not contained, so we have enough room and our position is good.
            // b) We are contained, so we'll bail out of this but try again at a position above the containing entity.
            const int iterationLimit = 1000;
            for (int counter = 0; counter < iterationLimit; counter++) {
                ignore.push_back(upperId);
                if (!findIntersection(upperIntersection, up, upperIntersection, upperId, upperNormal)) {
                    // We're not inside an entity, and from the nested tests, we have room between what is above and below. So position is good!
                    return false; // enough room
                }
                if (isUp(upperNormal)) {
                    // This new intersection is the top surface of an entity that we have not yet seen, which means we're contained within it.
                    // We could break here and recurse from the top of the original ceiling, but since we've already done the work to find the top
                    // of the enclosing entity, let's put our feet at upperIntersection and start over.
                    return mustMove();
                }
                // We found a new bottom surface, which we're not interested in.
                // But there could still be a top surface above us for an entity we haven't seen, so keep looking upward.
            }
            qCDebug(interfaceapp) << "Loop in requiresSafeLanding. Floor/ceiling do not make sense.";
        }
    }

    include.push_back(upperId); // We're now looking for the intersection from above onto this entity.
    const float big = (float)TREE_SCALE;
    const auto skyHigh = up * big;
    auto fromAbove = capsuleCenter + skyHigh;
    if (!findIntersection(fromAbove, down, upperIntersection, upperId, upperNormal)) {
        return false; // Unable to find a landing
    }
    // Our arbitrary rule is to always go up. There's no need to look down or sideways for a "closer" safe candidate.
    return mustMove();
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
    setProperty("lookAtSnappingEnabled", menu->isOptionChecked(MenuOption::EnableLookAtSnapping));
}

void MyAvatar::setFlyingEnabled(bool enabled) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setFlyingEnabled", Q_ARG(bool, enabled));
        return;
    }

    if (qApp->isHMDMode()) {
        setFlyingHMDPref(enabled);
    } else {
        setFlyingDesktopPref(enabled);
    }

    _enableFlying = enabled;
}

bool MyAvatar::isFlying() {
    // Avatar is Flying, and is not Falling, or Taking off
    return _characterController.getState() == CharacterController::State::Hover;
}

bool MyAvatar::isInAir() {
    // If Avatar is Hover, Falling, or Taking off, they are in Air.
    return _characterController.getState() != CharacterController::State::Ground;
}

bool MyAvatar::getFlyingEnabled() {
    // May return true even if client is not allowed to fly in the zone.
    return (qApp->isHMDMode() ? getFlyingHMDPref() : getFlyingDesktopPref());
}

void MyAvatar::setFlyingDesktopPref(bool enabled) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setFlyingDesktopPref", Q_ARG(bool, enabled));
        return;
    }

    _flyingPrefDesktop = enabled;
}

bool MyAvatar::getFlyingDesktopPref() {
    return _flyingPrefDesktop;
}

void MyAvatar::setFlyingHMDPref(bool enabled) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setFlyingHMDPref", Q_ARG(bool, enabled));
        return;
    }

    _flyingPrefHMD = enabled;
}

bool MyAvatar::getFlyingHMDPref() {
    return _flyingPrefHMD;
}

// Public interface for targetscale
float MyAvatar::getAvatarScale() {
    return getTargetScale();
}

void MyAvatar::setAvatarScale(float val) {

    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setAvatarScale", Q_ARG(float, val));
        return;
    }

    setTargetScale(val);
}

void MyAvatar::setCollisionsEnabled(bool enabled) {

    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setCollisionsEnabled", Q_ARG(bool, enabled));
        return;
    }
    _characterController.setCollisionless(!enabled);
    emit collisionsEnabledChanged(enabled);
}

bool MyAvatar::getCollisionsEnabled() {
    // may return 'false' even though the collisionless option was requested
    // because the zone may disallow collisionless avatars
    return _characterController.computeCollisionGroup() != BULLET_COLLISION_GROUP_COLLISIONLESS;
}

void MyAvatar::setOtherAvatarsCollisionsEnabled(bool enabled) {

    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setOtherAvatarsCollisionsEnabled", Q_ARG(bool, enabled));
        return;
    }
    _collideWithOtherAvatars = enabled;
    emit otherAvatarsCollisionsEnabledChanged(enabled);
}

bool MyAvatar::getOtherAvatarsCollisionsEnabled() {
    return _collideWithOtherAvatars;
}

void MyAvatar::updateCollisionCapsuleCache() {
    glm::vec3 start, end;
    float radius;
    getCapsule(start, end, radius);
    QVariantMap capsule;
    capsule["start"] = vec3toVariant(start);
    capsule["end"] = vec3toVariant(end);
    capsule["radius"] = QVariant(radius);
    _collisionCapsuleCache.set(capsule);
}

// thread safe
QVariantMap MyAvatar::getCollisionCapsule() const {
    return _collisionCapsuleCache.get();
}

void MyAvatar::setCharacterControllerEnabled(bool enabled) {
    qCDebug(interfaceapp) << "MyAvatar.characterControllerEnabled is deprecated. Use MyAvatar.collisionsEnabled instead.";
    setCollisionsEnabled(enabled);
}

bool MyAvatar::getCharacterControllerEnabled() {
    qCDebug(interfaceapp) << "MyAvatar.characterControllerEnabled is deprecated. Use MyAvatar.collisionsEnabled instead.";
    return getCollisionsEnabled();
}

void MyAvatar::clearDriveKeys() {
    _driveKeys.fill(0.0f);
}

void MyAvatar::setDriveKey(DriveKeys key, float val) {
    try {
        _driveKeys.at(key) = val;
    } catch (const std::exception&) {
        qCCritical(interfaceapp) << Q_FUNC_INFO << ": Index out of bounds";
    }
}

float MyAvatar::getDriveKey(DriveKeys key) const {
    return isDriveKeyDisabled(key) ? 0.0f : getRawDriveKey(key);
}

float MyAvatar::getRawDriveKey(DriveKeys key) const {
    try {
        return _driveKeys.at(key);
    } catch (const std::exception&) {
        qCCritical(interfaceapp) << Q_FUNC_INFO << ": Index out of bounds";
        return 0.0f;
    }
}

void MyAvatar::relayDriveKeysToCharacterController() {
    if (getDriveKey(TRANSLATE_Y) > 0.0f && (!qApp->isHMDMode() || (useAdvancedMovementControls() && getFlyingHMDPref()))) {
        _characterController.jump();
    }
}

void MyAvatar::disableDriveKey(DriveKeys key) {
    try {
        _disabledDriveKeys.set(key);
    } catch (const std::exception&) {
        qCCritical(interfaceapp) << Q_FUNC_INFO << ": Index out of bounds";
    }
}

void MyAvatar::enableDriveKey(DriveKeys key) {
    try {
        _disabledDriveKeys.reset(key);
    } catch (const std::exception&) {
        qCCritical(interfaceapp) << Q_FUNC_INFO << ": Index out of bounds";
    }
}

bool MyAvatar::isDriveKeyDisabled(DriveKeys key) const {
    try {
        return _disabledDriveKeys.test(key);
    } catch (const std::exception&) {
        qCCritical(interfaceapp) << Q_FUNC_INFO << ": Index out of bounds";
        return true;
    }
}

void MyAvatar::triggerVerticalRecenter() {
    _follow.setForceActivateVertical(true);
}

void MyAvatar::triggerHorizontalRecenter() {
    _follow.setForceActivateHorizontal(true);
}

void MyAvatar::triggerRotationRecenter() {
    _follow.setForceActivateRotation(true);
}

// old school meat hook style
glm::mat4 MyAvatar::deriveBodyFromHMDSensor() const {
    glm::vec3 headPosition(0.0f, _userHeight.get(), 0.0f);
    glm::quat headOrientation;
    auto headPose = getControllerPoseInSensorFrame(controller::Action::HEAD);
    if (headPose.isValid()) {
        headPosition = headPose.translation;
        // AJT: TODO: can remove this Y_180
        headOrientation = headPose.rotation * Quaternions::Y_180;
    }
    const glm::quat headOrientationYawOnly = cancelOutRollAndPitch(headOrientation);

    const Rig& rig = _skeletonModel->getRig();
    int headIndex = rig.indexOfJoint("Head");
    int neckIndex = rig.indexOfJoint("Neck");
    int hipsIndex = rig.indexOfJoint("Hips");

    glm::vec3 rigHeadPos = headIndex != -1 ? rig.getAbsoluteDefaultPose(headIndex).trans() : DEFAULT_AVATAR_HEAD_POS;
    glm::vec3 rigNeckPos = neckIndex != -1 ? rig.getAbsoluteDefaultPose(neckIndex).trans() : DEFAULT_AVATAR_NECK_POS;
    glm::vec3 rigHipsPos = hipsIndex != -1 ? rig.getAbsoluteDefaultPose(hipsIndex).trans() : DEFAULT_AVATAR_HIPS_POS;

    glm::vec3 localHead = (rigHeadPos - rigHipsPos);
    glm::vec3 localNeck = (rigNeckPos - rigHipsPos);

    // apply simplistic head/neck model
    // figure out where the avatar body should be by applying offsets from the avatar's neck & head joints.

    // eyeToNeck offset is relative full HMD orientation.
    // while neckToRoot offset is only relative to HMDs yaw.
    // Y_180 is necessary because rig is z forward and hmdOrientation is -z forward

    // AJT: TODO: can remove this Y_180, if we remove the higher level one.
    glm::vec3 headToNeck = headOrientation * Quaternions::Y_180 * (localNeck - localHead);
    glm::vec3 neckToRoot = headOrientationYawOnly  * Quaternions::Y_180 * -localNeck;

    float invSensorToWorldScale = getUserEyeHeight() / getEyeHeight();
    glm::vec3 bodyPos = headPosition + invSensorToWorldScale * (headToNeck + neckToRoot);

    return createMatFromQuatAndPos(headOrientationYawOnly, bodyPos);
}

glm::mat4 MyAvatar::getSpine2RotationRigSpace() const {
    int spine2Index = _skeletonModel->getRig().indexOfJoint("Spine2");
    glm::quat spine2Rot = Quaternions::IDENTITY;
    if (!(spine2Index < 0)) {
        // use the spine for the azimuth origin.
        spine2Rot = getAbsoluteJointRotationInObjectFrame(spine2Index);
    }
    glm::vec3 spine2UpAvatarSpace = spine2Rot * glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 spine2FwdAvatarSpace = spine2Rot * glm::vec3(_hipToHandController.x, 0.0f, _hipToHandController.y);

    // static const glm::quat RIG_CHANGE_OF_BASIS = Quaternions::Y_180;
    // RIG_CHANGE_OF_BASIS * AVATAR_TO_RIG_ROTATION * inverse(RIG_CHANGE_OF_BASIS) = Quaternions::Y_180; //avatar Space;
    const glm::quat AVATAR_TO_RIG_ROTATION = Quaternions::Y_180;
    glm::vec3 spine2UpRigSpace = AVATAR_TO_RIG_ROTATION * spine2UpAvatarSpace;
    glm::vec3 spine2FwdRigSpace = AVATAR_TO_RIG_ROTATION * spine2FwdAvatarSpace;

    glm::vec3 u, v, w;
    if (glm::length(spine2FwdRigSpace) > 0.0f) {
        spine2FwdRigSpace = glm::normalize(spine2FwdRigSpace);
    } else {
        spine2FwdRigSpace = glm::vec3(0.0f, 0.0f, 1.0f);
    }
    if (glm::length(spine2UpRigSpace) > 0.0f) {
        spine2UpRigSpace = glm::normalize(spine2UpRigSpace);
    } else {
        spine2UpRigSpace = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    generateBasisVectors(spine2UpRigSpace, spine2FwdRigSpace, u, v, w);
    glm::mat4 spine2RigSpace(glm::vec4(w, 0.0f), glm::vec4(u, 0.0f), glm::vec4(v, 0.0f), glm::vec4(glm::vec3(0.0f, 0.0f, 0.0f), 1.0f));
    return spine2RigSpace;
}

// ease in function for dampening cg movement
static float slope(float num) {
    const float CURVE_CONSTANT = 1.0f;
    float ret = 1.0f;
    if (num > 0.0f) {
        ret = 1.0f - (1.0f / (1.0f + CURVE_CONSTANT * num));
    }
    return ret;
}

// This function gives a soft clamp at the edge of the base of support
// dampenCgMovement returns the damped cg value in Avatar space.
// cgUnderHeadHandsAvatarSpace is also in Avatar space
// baseOfSupportScale is based on the height of the user
static glm::vec3 dampenCgMovement(glm::vec3 cgUnderHeadHandsAvatarSpace, float baseOfSupportScale) {
    float distanceFromCenterZ = cgUnderHeadHandsAvatarSpace.z;
    float distanceFromCenterX = cgUnderHeadHandsAvatarSpace.x;

    // In the forward direction we need a different scale because forward is in
    // the direction of the hip extensor joint, which means bending usually happens
    // well before reaching the edge of the base of support.
    const float clampFront = DEFAULT_AVATAR_SUPPORT_BASE_FRONT * DEFAULT_AVATAR_FORWARD_DAMPENING_FACTOR * baseOfSupportScale;
    float clampBack = DEFAULT_AVATAR_SUPPORT_BASE_BACK * DEFAULT_AVATAR_LATERAL_DAMPENING_FACTOR * baseOfSupportScale;
    float clampLeft = DEFAULT_AVATAR_SUPPORT_BASE_LEFT * DEFAULT_AVATAR_LATERAL_DAMPENING_FACTOR * baseOfSupportScale;
    float clampRight = DEFAULT_AVATAR_SUPPORT_BASE_RIGHT * DEFAULT_AVATAR_LATERAL_DAMPENING_FACTOR * baseOfSupportScale;
    glm::vec3 dampedCg(0.0f, 0.0f, 0.0f);

    // find the damped z coord of the cg
    if (cgUnderHeadHandsAvatarSpace.z < 0.0f) {
        // forward displacement
        dampedCg.z = slope(fabs(distanceFromCenterZ / clampFront)) * clampFront;
    } else {
        // backwards displacement
        dampedCg.z = slope(fabs(distanceFromCenterZ / clampBack)) * clampBack;
    }

    // find the damped x coord of the cg
    if (cgUnderHeadHandsAvatarSpace.x > 0.0f) {
        // right of center
        dampedCg.x = slope(fabs(distanceFromCenterX / clampRight)) * clampRight;
    } else {
        // left of center
        dampedCg.x = slope(fabs(distanceFromCenterX / clampLeft)) * clampLeft;
    }
    return dampedCg;
}

// computeCounterBalance returns the center of gravity in Avatar space
glm::vec3 MyAvatar::computeCounterBalance() {
    struct JointMass {
        QString name;
        float weight;
        glm::vec3 position;
        JointMass() {};
        JointMass(QString n, float w, glm::vec3 p) {
            name = n;
            weight = w;
            position = p;
        }
    };

    // init the body part weights
    JointMass cgHeadMass(QString("Head"), DEFAULT_AVATAR_HEAD_MASS, glm::vec3(0.0f, 0.0f, 0.0f));
    JointMass cgLeftHandMass(QString("LeftHand"), DEFAULT_AVATAR_LEFTHAND_MASS, glm::vec3(0.0f, 0.0f, 0.0f));
    JointMass cgRightHandMass(QString("RightHand"), DEFAULT_AVATAR_RIGHTHAND_MASS, glm::vec3(0.0f, 0.0f, 0.0f));
    glm::vec3 tposeHead = DEFAULT_AVATAR_HEAD_POS;
    glm::vec3 tposeHips = DEFAULT_AVATAR_HIPS_POS;
    glm::vec3 tposeRightFoot = DEFAULT_AVATAR_RIGHTFOOT_POS;

    if (_skeletonModel->getRig().indexOfJoint(cgHeadMass.name) != -1) {
        cgHeadMass.position = getAbsoluteJointTranslationInObjectFrame(_skeletonModel->getRig().indexOfJoint(cgHeadMass.name));
        tposeHead = getAbsoluteDefaultJointTranslationInObjectFrame(_skeletonModel->getRig().indexOfJoint(cgHeadMass.name));
    }
    if (_skeletonModel->getRig().indexOfJoint(cgLeftHandMass.name) != -1) {
        cgLeftHandMass.position = getAbsoluteJointTranslationInObjectFrame(_skeletonModel->getRig().indexOfJoint(cgLeftHandMass.name));
    } else {
        cgLeftHandMass.position = DEFAULT_AVATAR_LEFTHAND_POS;
    }
    if (_skeletonModel->getRig().indexOfJoint(cgRightHandMass.name) != -1) {
        cgRightHandMass.position = getAbsoluteJointTranslationInObjectFrame(_skeletonModel->getRig().indexOfJoint(cgRightHandMass.name));
    } else {
        cgRightHandMass.position = DEFAULT_AVATAR_RIGHTHAND_POS;
    }
    if (_skeletonModel->getRig().indexOfJoint("Hips") != -1) {
        tposeHips = getAbsoluteDefaultJointTranslationInObjectFrame(_skeletonModel->getRig().indexOfJoint("Hips"));
    }
    if (_skeletonModel->getRig().indexOfJoint("RightFoot") != -1) {
        tposeRightFoot = getAbsoluteDefaultJointTranslationInObjectFrame(_skeletonModel->getRig().indexOfJoint("RightFoot"));
    }

    // find the current center of gravity position based on head and hand moments
    glm::vec3 sumOfMoments = (cgHeadMass.weight * cgHeadMass.position) + (cgLeftHandMass.weight * cgLeftHandMass.position) + (cgRightHandMass.weight * cgRightHandMass.position);
    float totalMass = cgHeadMass.weight + cgLeftHandMass.weight + cgRightHandMass.weight;

    glm::vec3 currentCg = (1.0f / totalMass) * sumOfMoments;
    currentCg.y = 0.0f;
    // dampening the center of gravity, in effect, limits the value to the perimeter of the base of support
    float baseScale = 1.0f;
    if (getUserEyeHeight() > 0.0f) {
        baseScale = getUserEyeHeight() / DEFAULT_AVATAR_EYE_HEIGHT;
    }
    glm::vec3 desiredCg = dampenCgMovement(currentCg, baseScale);

    // compute hips position to maintain desiredCg
    glm::vec3 counterBalancedForHead = (totalMass + DEFAULT_AVATAR_HIPS_MASS) * desiredCg;
    counterBalancedForHead -= sumOfMoments;
    glm::vec3 counterBalancedCg = (1.0f / DEFAULT_AVATAR_HIPS_MASS) * counterBalancedForHead;

    // find the height of the hips
    glm::vec3 xzDiff((cgHeadMass.position.x - counterBalancedCg.x), 0.0f, (cgHeadMass.position.z - counterBalancedCg.z));
    float headMinusHipXz = glm::length(xzDiff);
    float headHipDefault = glm::length(tposeHead - tposeHips);
    float hipHeight = 0.0f;
    if (headHipDefault > headMinusHipXz) {
        hipHeight = sqrtf((headHipDefault * headHipDefault) - (headMinusHipXz * headMinusHipXz));
    }
    counterBalancedCg.y = (cgHeadMass.position.y - hipHeight);

    // this is to be sure that the feet don't lift off the floor.
    // add 5 centimeters to allow for going up on the toes.
    if (counterBalancedCg.y > (tposeHips.y + 0.05f)) {
        // if the height is higher than default hips, clamp to default hips
        counterBalancedCg.y = tposeHips.y + 0.05f;
    }
    return counterBalancedCg;
}

// this function matches the hips rotation to the new cghips-head axis
// headOrientation, headPosition and hipsPosition are in avatar space
// returns the matrix of the hips in Avatar space
static glm::mat4 computeNewHipsMatrix(glm::quat headOrientation, glm::vec3 headPosition, glm::vec3 hipsPosition) {

    glm::quat bodyOrientation = computeBodyFacingFromHead(headOrientation, Vectors::UNIT_Y);

    const float MIX_RATIO = 0.3f;
    glm::quat hipsRot = safeLerp(Quaternions::IDENTITY, bodyOrientation, MIX_RATIO);
    glm::vec3 hipsFacing = hipsRot * Vectors::UNIT_Z;

    glm::vec3 spineVec = headPosition - hipsPosition;
    glm::vec3 u, v, w;
    generateBasisVectors(glm::normalize(spineVec), hipsFacing, u, v, w);
    return glm::mat4(glm::vec4(w, 0.0f),
        glm::vec4(u, 0.0f),
        glm::vec4(v, 0.0f),
        glm::vec4(hipsPosition, 1.0f));
}

static void drawBaseOfSupport(float baseOfSupportScale, float footLocal, glm::mat4 avatarToWorld) {
    // scale the base of support based on user height
    float clampFront = DEFAULT_AVATAR_SUPPORT_BASE_FRONT * baseOfSupportScale;
    float clampBack = DEFAULT_AVATAR_SUPPORT_BASE_BACK * baseOfSupportScale;
    float clampLeft = DEFAULT_AVATAR_SUPPORT_BASE_LEFT * baseOfSupportScale;
    float clampRight = DEFAULT_AVATAR_SUPPORT_BASE_RIGHT * baseOfSupportScale;
    float floor = footLocal + 0.05f;

    // transform the base of support corners to world space
    glm::vec3 frontRight = transformPoint(avatarToWorld, { clampRight, floor, clampFront });
    glm::vec3 frontLeft = transformPoint(avatarToWorld, { clampLeft, floor, clampFront });
    glm::vec3 backRight = transformPoint(avatarToWorld, { clampRight, floor, clampBack });
    glm::vec3 backLeft = transformPoint(avatarToWorld, { clampLeft, floor, clampBack });

    // draw the borders
    const glm::vec4 rayColor = { 1.0f, 0.0f, 0.0f, 1.0f };
    DebugDraw::getInstance().drawRay(backLeft, frontLeft, rayColor);
    DebugDraw::getInstance().drawRay(backLeft, backRight, rayColor);
    DebugDraw::getInstance().drawRay(backRight, frontRight, rayColor);
    DebugDraw::getInstance().drawRay(frontLeft, frontRight, rayColor);
}

// this function finds the hips position using a center of gravity model that
// balances the head and hands with the hips over the base of support
// returns the rotation (-z forward) and position of the Avatar in Sensor space
glm::mat4 MyAvatar::deriveBodyUsingCgModel() {
    glm::mat4 sensorToWorldMat = getSensorToWorldMatrix();
    glm::mat4 worldToSensorMat = glm::inverse(sensorToWorldMat);
    auto headPose = getControllerPoseInSensorFrame(controller::Action::HEAD);

    glm::mat4 sensorHeadMat = createMatFromQuatAndPos(headPose.rotation * Quaternions::Y_180, headPose.translation);

    // convert into avatar space
    glm::mat4 avatarToWorldMat = getTransform().getMatrix();
    glm::mat4 avatarHeadMat = glm::inverse(avatarToWorldMat) * sensorToWorldMat * sensorHeadMat;

    if (_enableDebugDrawBaseOfSupport) {
        float scaleBaseOfSupport = getUserEyeHeight() / DEFAULT_AVATAR_EYE_HEIGHT;
        glm::vec3 rightFootPositionLocal = getAbsoluteJointTranslationInObjectFrame(_skeletonModel->getRig().indexOfJoint("RightFoot"));
        drawBaseOfSupport(scaleBaseOfSupport, rightFootPositionLocal.y, avatarToWorldMat);
    }

    // get the new center of gravity
    glm::vec3 cgHipsPosition = computeCounterBalance();

    // find the new hips rotation using the new head-hips axis as the up axis
    glm::mat4 avatarHipsMat = computeNewHipsMatrix(glmExtractRotation(avatarHeadMat), extractTranslation(avatarHeadMat), cgHipsPosition);

    // convert hips from avatar to sensor space
    // The Y_180 is to convert from z forward to -z forward.
    return worldToSensorMat * avatarToWorldMat * avatarHipsMat;
}

static bool isInsideLine(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
    return (((b.x - a.x) * (c.z - a.z) - (b.z - a.z) * (c.x - a.x)) > 0);
}

static bool withinBaseOfSupport(const controller::Pose& head) {
    float userScale = 1.0f;

    glm::vec3 frontLeft(-DEFAULT_AVATAR_LATERAL_STEPPING_THRESHOLD, 0.0f, -DEFAULT_AVATAR_ANTERIOR_STEPPING_THRESHOLD);
    glm::vec3 frontRight(DEFAULT_AVATAR_LATERAL_STEPPING_THRESHOLD, 0.0f, -DEFAULT_AVATAR_ANTERIOR_STEPPING_THRESHOLD);
    glm::vec3 backLeft(-DEFAULT_AVATAR_LATERAL_STEPPING_THRESHOLD, 0.0f, DEFAULT_AVATAR_POSTERIOR_STEPPING_THRESHOLD);
    glm::vec3 backRight(DEFAULT_AVATAR_LATERAL_STEPPING_THRESHOLD, 0.0f, DEFAULT_AVATAR_POSTERIOR_STEPPING_THRESHOLD);

    bool isWithinSupport = false;
    if (head.isValid()) {
        bool withinFrontBase = isInsideLine(userScale * frontLeft, userScale * frontRight, head.getTranslation());
        bool withinBackBase = isInsideLine(userScale * backRight, userScale * backLeft, head.getTranslation());
        bool withinLateralBase = (isInsideLine(userScale * frontRight, userScale * backRight, head.getTranslation()) &&
            isInsideLine(userScale * backLeft, userScale * frontLeft, head.getTranslation()));
        isWithinSupport = (withinFrontBase && withinBackBase && withinLateralBase);
    }
    return isWithinSupport;
}

static bool headAngularVelocityBelowThreshold(const controller::Pose& head) {
    glm::vec3 xzPlaneAngularVelocity(0.0f, 0.0f, 0.0f);
    if (head.isValid()) {
        xzPlaneAngularVelocity.x = head.getAngularVelocity().x;
        xzPlaneAngularVelocity.z = head.getAngularVelocity().z;
    }
    float magnitudeAngularVelocity = glm::length(xzPlaneAngularVelocity);
    bool isBelowThreshold = (magnitudeAngularVelocity < DEFAULT_AVATAR_HEAD_ANGULAR_VELOCITY_STEPPING_THRESHOLD);

    return isBelowThreshold;
}

static bool isWithinThresholdHeightMode(const controller::Pose& head, const float& newMode, const float& scale) {
    bool isWithinThreshold = true;
    if (head.isValid()) {
        isWithinThreshold = (head.getTranslation().y - newMode) > (DEFAULT_AVATAR_MODE_HEIGHT_STEPPING_THRESHOLD * scale);
    }
    return isWithinThreshold;
}

float MyAvatar::computeStandingHeightMode(const controller::Pose& head) {
    const float MODE_CORRECTION_FACTOR = 0.02f;
    int greatestFrequency = 0;
    int mode = 0;
    // init mode in meters to the current mode
    float modeInMeters = getCurrentStandingHeight();
    if (head.isValid()) {
        std::map<int, int> freq;
        for(auto recentModeReadingsIterator = _recentModeReadings.begin(); recentModeReadingsIterator != _recentModeReadings.end(); ++recentModeReadingsIterator) {
            freq[*recentModeReadingsIterator] += 1;
            if (freq[*recentModeReadingsIterator] > greatestFrequency) {
                greatestFrequency = freq[*recentModeReadingsIterator];
                mode = *recentModeReadingsIterator;
            }
        }

        modeInMeters = ((float)mode) / CENTIMETERS_PER_METER;
        if (!(modeInMeters > getCurrentStandingHeight())) {
            // if not greater check for a reset
            if (getResetMode() && getControllerPoseInAvatarFrame(controller::Action::HEAD).isValid()) {
                setResetMode(false);
                float resetModeInCentimeters = glm::floor((head.getTranslation().y - MODE_CORRECTION_FACTOR)*CENTIMETERS_PER_METER);
                modeInMeters = (resetModeInCentimeters / CENTIMETERS_PER_METER);
                _recentModeReadings.clear();

            } else {
                // if not greater and no reset, keep the mode as it is
                modeInMeters = getCurrentStandingHeight();

            }
        }
    }
    return modeInMeters;
}

static bool handDirectionMatchesHeadDirection(const controller::Pose& leftHand, const controller::Pose& rightHand, const controller::Pose& head) {
    const float VELOCITY_EPSILON = 0.02f;
    bool leftHandDirectionMatchesHead = true;
    bool rightHandDirectionMatchesHead = true;
    glm::vec3 xzHeadVelocity(head.velocity.x, 0.0f, head.velocity.z);
    if (leftHand.isValid() && head.isValid()) {
        glm::vec3 xzLeftHandVelocity(leftHand.velocity.x, 0.0f, leftHand.velocity.z);
        if ((glm::length(xzLeftHandVelocity) > VELOCITY_EPSILON) && (glm::length(xzHeadVelocity) > VELOCITY_EPSILON)) {
            float handDotHeadLeft = glm::dot(glm::normalize(xzLeftHandVelocity), glm::normalize(xzHeadVelocity));
            leftHandDirectionMatchesHead = ((handDotHeadLeft > DEFAULT_HANDS_VELOCITY_DIRECTION_STEPPING_THRESHOLD));
        } else {
            leftHandDirectionMatchesHead = false;
        }
    }
    if (rightHand.isValid() && head.isValid()) {
        glm::vec3 xzRightHandVelocity(rightHand.velocity.x, 0.0f, rightHand.velocity.z);
        if ((glm::length(xzRightHandVelocity) > VELOCITY_EPSILON) && (glm::length(xzHeadVelocity) > VELOCITY_EPSILON)) {
            float handDotHeadRight = glm::dot(glm::normalize(xzRightHandVelocity), glm::normalize(xzHeadVelocity));
            rightHandDirectionMatchesHead = (handDotHeadRight > DEFAULT_HANDS_VELOCITY_DIRECTION_STEPPING_THRESHOLD);
        } else {
            rightHandDirectionMatchesHead = false;
        }
    }
    return leftHandDirectionMatchesHead && rightHandDirectionMatchesHead;
}

static bool handAngularVelocityBelowThreshold(const controller::Pose& leftHand, const controller::Pose& rightHand) {
    float leftHandXZAngularVelocity = 0.0f;
    float rightHandXZAngularVelocity = 0.0f;
    if (leftHand.isValid()) {
        glm::vec3 xzLeftHandAngularVelocity(leftHand.angularVelocity.x, 0.0f, leftHand.angularVelocity.z);
        leftHandXZAngularVelocity = glm::length(xzLeftHandAngularVelocity);
    }
    if (rightHand.isValid()) {
        glm::vec3 xzRightHandAngularVelocity(rightHand.angularVelocity.x, 0.0f, rightHand.angularVelocity.z);
        rightHandXZAngularVelocity = glm::length(xzRightHandAngularVelocity);
    }
    return ((leftHandXZAngularVelocity < DEFAULT_HANDS_ANGULAR_VELOCITY_STEPPING_THRESHOLD) &&
        (rightHandXZAngularVelocity < DEFAULT_HANDS_ANGULAR_VELOCITY_STEPPING_THRESHOLD));
}

static bool headVelocityGreaterThanThreshold(const controller::Pose& head) {
    float headVelocityMagnitude = 0.0f;
    if (head.isValid()) {
        headVelocityMagnitude = glm::length(head.getVelocity());
    }
    return headVelocityMagnitude > DEFAULT_HEAD_VELOCITY_STEPPING_THRESHOLD;
}

glm::quat MyAvatar::computeAverageHeadRotation(const controller::Pose& head) {
    const float AVERAGING_RATE = 0.03f;
    return safeLerp(_averageHeadRotation, head.getRotation(), AVERAGING_RATE);
}

static bool isHeadLevel(const controller::Pose& head, const glm::quat& averageHeadRotation) {
    glm::vec3 diffFromAverageEulers(0.0f, 0.0f, 0.0f);
    if (head.isValid()) {
        glm::vec3 averageHeadEulers = glm::degrees(safeEulerAngles(averageHeadRotation));
        glm::vec3 currentHeadEulers = glm::degrees(safeEulerAngles(head.getRotation()));
        diffFromAverageEulers = averageHeadEulers - currentHeadEulers;
    }
    return ((fabs(diffFromAverageEulers.x) < DEFAULT_HEAD_PITCH_STEPPING_TOLERANCE) && (fabs(diffFromAverageEulers.z) < DEFAULT_HEAD_ROLL_STEPPING_TOLERANCE));
}

float MyAvatar::getUserHeight() const {
    return _userHeight.get();
}

void MyAvatar::setUserHeight(float value) {
    _userHeight.set(value);

    float sensorToWorldScale = getEyeHeight() / getUserEyeHeight();
    emit sensorToWorldScaleChanged(sensorToWorldScale);
}

float MyAvatar::getUserEyeHeight() const {
    float ratio = DEFAULT_AVATAR_EYE_TO_TOP_OF_HEAD / DEFAULT_AVATAR_HEIGHT;
    float userHeight = _userHeight.get();
    return userHeight - userHeight * ratio;
}

bool MyAvatar::getIsInWalkingState() const {
    return _isInWalkingState;
}

bool MyAvatar::getIsInSittingState() const {
    return _isInSittingState.get();
}

MyAvatar::SitStandModelType MyAvatar::getUserRecenterModel() const {
    return _userRecenterModel.get();
}

bool MyAvatar::getIsSitStandStateLocked() const {
    return _lockSitStandState.get();
}

float MyAvatar::getWalkSpeed() const {
    return _walkSpeed.get() * _walkSpeedScalar;
}

float MyAvatar::getWalkBackwardSpeed() const {
    return _walkSpeed.get() * _walkSpeedScalar;
}

bool MyAvatar::isReadyForPhysics() const {
    return qApp->isServerlessMode() || _haveReceivedHeightLimitsFromDomain;
}

void MyAvatar::setSprintMode(bool sprint) {
    _walkSpeedScalar = sprint ? _sprintSpeed.get() : AVATAR_WALK_SPEED_SCALAR;
}

void MyAvatar::setIsInWalkingState(bool isWalking) {
    _isInWalkingState = isWalking;
}

void MyAvatar::setIsInSittingState(bool isSitting) {
    _sitStandStateTimer = 0.0f;
    _squatTimer = 0.0f;
    // on reset height we need the count to be more than one in case the user sits and stands up quickly.
    _isInSittingState.set(isSitting);
    setResetMode(true);
    if (isSitting) {
        setCenterOfGravityModelEnabled(false);
    } else {
        setCenterOfGravityModelEnabled(true);
    }
    setSitStandStateChange(true);
}

void MyAvatar::setUserRecenterModel(MyAvatar::SitStandModelType modelName) {

    _userRecenterModel.set(modelName);

    switch (modelName) {
        case MyAvatar::SitStandModelType::ForceSit:
            setHMDLeanRecenterEnabled(true);
            setIsInSittingState(true);
            setIsSitStandStateLocked(true);
            break;
        case MyAvatar::SitStandModelType::ForceStand:
            setHMDLeanRecenterEnabled(true);
            setIsInSittingState(false);
            setIsSitStandStateLocked(true);
            break;
        case MyAvatar::SitStandModelType::Auto:
        default:
            setHMDLeanRecenterEnabled(true);
            setIsInSittingState(false);
            setIsSitStandStateLocked(false);
            break;
        case MyAvatar::SitStandModelType::DisableHMDLean:
            setHMDLeanRecenterEnabled(false);
            setIsInSittingState(false);
            setIsSitStandStateLocked(false);
            break;
    }
}

void MyAvatar::setIsSitStandStateLocked(bool isLocked) {
    _lockSitStandState.set(isLocked);
    _sitStandStateTimer = 0.0f;
    _squatTimer = 0.0f;
    _averageUserHeightSensorSpace = _userHeight.get();
    _tippingPoint = _userHeight.get();
    if (!isLocked) {
        // always start the auto transition mode in standing state.
        setIsInSittingState(false);
    }
}

void MyAvatar::setWalkSpeed(float value) {
    _walkSpeed.set(value);
}

void MyAvatar::setWalkBackwardSpeed(float value) {
    _walkBackwardSpeed.set(value);
}

void MyAvatar::setSprintSpeed(float value) {
    _sprintSpeed.set(value);
}

float MyAvatar::getSprintSpeed() const {
    return _sprintSpeed.get();
}

void MyAvatar::setSitStandStateChange(bool stateChanged) {
    _sitStandStateChange = stateChanged;
}

float MyAvatar::getSitStandStateChange() const {
    return _sitStandStateChange;
}

QVector<QString> MyAvatar::getScriptUrls() {
    QVector<QString> scripts = _skeletonModel->isLoaded() ? _skeletonModel->getHFMModel().scripts : QVector<QString>();
    return scripts;
}

glm::vec3 MyAvatar::getPositionForAudio() {
    glm::vec3 result;
    switch (_audioListenerMode) {
        case AudioListenerMode::FROM_HEAD:
            result = getHead()->getPosition();
            break;
        case AudioListenerMode::FROM_CAMERA:
            result = qApp->getCamera().getPosition();
            break;
        case AudioListenerMode::CUSTOM:
            result = _customListenPosition;
            break;
    }

    if (isNaN(result)) {
        qCDebug(interfaceapp) << "MyAvatar::getPositionForAudio produced NaN" << _audioListenerMode;
        result = glm::vec3(0.0f);
    }

    return result;
}

glm::quat MyAvatar::getOrientationForAudio() {
    glm::quat result;

    switch (_audioListenerMode) {
        case AudioListenerMode::FROM_HEAD:
            result = getHead()->getFinalOrientationInWorldFrame();
            break;
        case AudioListenerMode::FROM_CAMERA:
            result = qApp->getCamera().getOrientation();
            break;
        case AudioListenerMode::CUSTOM:
            result = _customListenOrientation;
            break;
    }

    if (isNaN(result)) {
        qCDebug(interfaceapp) << "MyAvatar::getOrientationForAudio produced NaN" << _audioListenerMode;
        result = glm::quat();
    }

    return result;
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
    audioListenerMode = static_cast<AudioListenerMode>(object.toUInt16());
}

QScriptValue driveKeysToScriptValue(QScriptEngine* engine, const MyAvatar::DriveKeys& driveKeys) {
    return driveKeys;
}

void driveKeysFromScriptValue(const QScriptValue& object, MyAvatar::DriveKeys& driveKeys) {
    driveKeys = static_cast<MyAvatar::DriveKeys>(object.toUInt16());
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
    const float FOLLOW_ROTATION_THRESHOLD = cosf(myAvatar.getRotationThreshold());
    glm::vec2 bodyFacing = getFacingDir2D(currentBodyMatrix);
    return glm::dot(-myAvatar.getHeadControllerFacingMovingAverage(), bodyFacing) < FOLLOW_ROTATION_THRESHOLD;
}

bool MyAvatar::FollowHelper::shouldActivateHorizontal(const MyAvatar& myAvatar, const glm::mat4& desiredBodyMatrix, const glm::mat4& currentBodyMatrix) const {
    // -z axis of currentBodyMatrix in world space.
    glm::vec3 forward = glm::normalize(glm::vec3(-currentBodyMatrix[0][2], -currentBodyMatrix[1][2], -currentBodyMatrix[2][2]));
    // x axis of currentBodyMatrix in world space.
    glm::vec3 right = glm::normalize(glm::vec3(currentBodyMatrix[0][0], currentBodyMatrix[1][0], currentBodyMatrix[2][0]));
    glm::vec3 offset = extractTranslation(desiredBodyMatrix) - extractTranslation(currentBodyMatrix);
    controller::Pose currentHeadPose = myAvatar.getControllerPoseInAvatarFrame(controller::Action::HEAD);

    float forwardLeanAmount = glm::dot(forward, offset);
    float lateralLeanAmount = glm::dot(right, offset);

    const float MAX_LATERAL_LEAN = 0.3f;
    const float MAX_FORWARD_LEAN = 0.15f;
    const float MAX_BACKWARD_LEAN = 0.1f;

    bool stepDetected = false;
    if (myAvatar.getIsInSittingState()) {
        if (!withinBaseOfSupport(currentHeadPose)) {
            stepDetected = true;
        }
    } else if (forwardLeanAmount > 0 && forwardLeanAmount > MAX_FORWARD_LEAN) {
        stepDetected = true;
    } else if (forwardLeanAmount < 0 && forwardLeanAmount < -MAX_BACKWARD_LEAN) {
        stepDetected = true;
    } else {
        stepDetected = fabs(lateralLeanAmount) > MAX_LATERAL_LEAN;
    }
    return stepDetected;
}

bool MyAvatar::FollowHelper::shouldActivateHorizontalCG(MyAvatar& myAvatar) const {

    // get the current readings
    controller::Pose currentHeadPose = myAvatar.getControllerPoseInAvatarFrame(controller::Action::HEAD);
    controller::Pose currentLeftHandPose = myAvatar.getControllerPoseInAvatarFrame(controller::Action::LEFT_HAND);
    controller::Pose currentRightHandPose = myAvatar.getControllerPoseInAvatarFrame(controller::Action::RIGHT_HAND);
    controller::Pose currentHeadSensorPose = myAvatar.getControllerPoseInSensorFrame(controller::Action::HEAD);

    bool stepDetected = false;
    float myScale = myAvatar.getAvatarScale();

    if (myAvatar.getIsInWalkingState()) {
        stepDetected = true;
    } else {
        if (!withinBaseOfSupport(currentHeadPose) &&
            headAngularVelocityBelowThreshold(currentHeadPose) &&
            isWithinThresholdHeightMode(currentHeadSensorPose, myAvatar.getCurrentStandingHeight(), myScale) &&
            handDirectionMatchesHeadDirection(currentLeftHandPose, currentRightHandPose, currentHeadPose) &&
            handAngularVelocityBelowThreshold(currentLeftHandPose, currentRightHandPose) &&
            headVelocityGreaterThanThreshold(currentHeadPose) &&
            isHeadLevel(currentHeadPose, myAvatar.getAverageHeadRotation())) {
            // a step is detected
            stepDetected = true;
            if (glm::length(currentHeadPose.velocity) > DEFAULT_AVATAR_WALK_SPEED_THRESHOLD) {
                myAvatar.setIsInWalkingState(true);
            }
        } else {
            glm::vec3 defaultHipsPosition = myAvatar.getAbsoluteDefaultJointTranslationInObjectFrame(myAvatar.getJointIndex("Hips"));
            glm::vec3 defaultHeadPosition = myAvatar.getAbsoluteDefaultJointTranslationInObjectFrame(myAvatar.getJointIndex("Head"));
            glm::vec3 currentHeadPosition = currentHeadPose.getTranslation();
            float anatomicalHeadToHipsDistance = glm::length(defaultHeadPosition - defaultHipsPosition);
            if (!isActive(Horizontal) &&
                (!isActive(Vertical)) &&
                (glm::length(currentHeadPosition - defaultHipsPosition) > (anatomicalHeadToHipsDistance + (DEFAULT_AVATAR_SPINE_STRETCH_LIMIT * anatomicalHeadToHipsDistance)))) {
                myAvatar.setResetMode(true);
                stepDetected = true;
                if (glm::length(currentHeadPose.velocity) > DEFAULT_AVATAR_WALK_SPEED_THRESHOLD) {
                    myAvatar.setIsInWalkingState(true);
                }
            }
        }
    }
    return stepDetected;
}

bool MyAvatar::FollowHelper::shouldActivateVertical(const MyAvatar& myAvatar, const glm::mat4& desiredBodyMatrix, const glm::mat4& currentBodyMatrix) const {
    const float CYLINDER_TOP = 0.1f;
    const float CYLINDER_BOTTOM = -1.5f;
    const float SITTING_BOTTOM = -0.02f;

    glm::vec3 offset = extractTranslation(desiredBodyMatrix) - extractTranslation(currentBodyMatrix);
    bool returnValue = false;

    if (myAvatar.getSitStandStateChange()) {
        returnValue = true;
    } else {
        if (myAvatar.getIsInSittingState()) {
            if (myAvatar.getIsSitStandStateLocked()) {
                returnValue = (offset.y > CYLINDER_TOP);
            }
            if (offset.y < SITTING_BOTTOM) {
                // we recenter more easily when in sitting state.
                returnValue = true;
            }
        } else {
            // in the standing state
            returnValue = (offset.y > CYLINDER_TOP) || (offset.y < CYLINDER_BOTTOM);
            // finally check for squats in standing
            if (_squatDetected) {
                returnValue = true;
            }
        }
    }
    return returnValue;
}

void MyAvatar::FollowHelper::prePhysicsUpdate(MyAvatar& myAvatar, const glm::mat4& desiredBodyMatrix,
                                              const glm::mat4& currentBodyMatrix, bool hasDriveInput) {

    if (myAvatar.getHMDLeanRecenterEnabled() &&
        qApp->getCamera().getMode() != CAMERA_MODE_MIRROR) {
        if (!isActive(Rotation) && (shouldActivateRotation(myAvatar, desiredBodyMatrix, currentBodyMatrix) || hasDriveInput)) {
            activate(Rotation);
            myAvatar.setHeadControllerFacingMovingAverage(myAvatar.getHeadControllerFacing());
        }
        if (myAvatar.getCenterOfGravityModelEnabled()) {
            if (!isActive(Horizontal) && (shouldActivateHorizontalCG(myAvatar) || hasDriveInput)) {
                activate(Horizontal);
                if (myAvatar.getEnableStepResetRotation()) {
                    activate(Rotation);
                    myAvatar.setHeadControllerFacingMovingAverage(myAvatar.getHeadControllerFacing());
                }
            }
        } else {
            // center of gravity model is not enabled
            if (!isActive(Horizontal) && (shouldActivateHorizontal(myAvatar, desiredBodyMatrix, currentBodyMatrix) || hasDriveInput)) {
                activate(Horizontal);
                if (myAvatar.getEnableStepResetRotation() && !myAvatar.getIsInSittingState()) {
                    activate(Rotation);
                    myAvatar.setHeadControllerFacingMovingAverage(myAvatar.getHeadControllerFacing());
                }
            }
        }
        if (!isActive(Vertical) && (shouldActivateVertical(myAvatar, desiredBodyMatrix, currentBodyMatrix) || hasDriveInput)) {
            activate(Vertical);
            if (_squatDetected) {
                _squatDetected = false;
            }
        }
    } else {
        if (!isActive(Rotation) && getForceActivateRotation()) {
            activate(Rotation);
            myAvatar.setHeadControllerFacingMovingAverage(myAvatar.getHeadControllerFacing());
            setForceActivateRotation(false);
        }
        if (!isActive(Horizontal) && getForceActivateHorizontal()) {
            activate(Horizontal);
            setForceActivateHorizontal(false);
        }
        if (!isActive(Vertical) && getForceActivateVertical()) {
            activate(Vertical);
            setForceActivateVertical(false);
        }
    }

    glm::mat4 desiredWorldMatrix = myAvatar.getSensorToWorldMatrix() * desiredBodyMatrix;
    glm::mat4 currentWorldMatrix = myAvatar.getSensorToWorldMatrix() * currentBodyMatrix;

    AnimPose followWorldPose(currentWorldMatrix);

    glm::quat currentHipsLocal = myAvatar.getAbsoluteJointRotationInObjectFrame(myAvatar.getJointIndex("Hips"));
    const glm::quat hipsinWorldSpace = followWorldPose.rot() * (Quaternions::Y_180 * (currentHipsLocal));
    const glm::vec3 avatarUpWorld = glm::normalize(followWorldPose.rot()*(Vectors::UP));
    glm::quat resultingSwingInWorld;
    glm::quat resultingTwistInWorld;
    swingTwistDecomposition(hipsinWorldSpace, avatarUpWorld, resultingSwingInWorld, resultingTwistInWorld);

    // remove scale present from sensorToWorldMatrix
    followWorldPose.scale() = glm::vec3(1.0f);

    if (isActive(Rotation)) {
            //use the hmd reading for the hips follow
            followWorldPose.rot() = glmExtractRotation(desiredWorldMatrix);
    }
    if (isActive(Horizontal)) {
        glm::vec3 desiredTranslation = extractTranslation(desiredWorldMatrix);
        followWorldPose.trans().x = desiredTranslation.x;
        followWorldPose.trans().z = desiredTranslation.z;
    }
    if (isActive(Vertical)) {
        glm::vec3 desiredTranslation = extractTranslation(desiredWorldMatrix);
        followWorldPose.trans().y = desiredTranslation.y;
    }

    myAvatar.getCharacterController()->setFollowParameters(followWorldPose, getMaxTimeRemaining());
}

glm::mat4 MyAvatar::FollowHelper::postPhysicsUpdate(MyAvatar& myAvatar, const glm::mat4& currentBodyMatrix) {
    if (isActive()) {
        float dt = myAvatar.getCharacterController()->getFollowTime();
        decrementTimeRemaining(dt);

        // apply follow displacement to the body matrix.
        glm::vec3 worldLinearDisplacement = myAvatar.getCharacterController()->getFollowLinearDisplacement();
        glm::quat worldAngularDisplacement = myAvatar.getCharacterController()->getFollowAngularDisplacement();

        glm::mat4 sensorToWorldMatrix = myAvatar.getSensorToWorldMatrix();
        glm::mat4 worldToSensorMatrix = glm::inverse(sensorToWorldMatrix);

        glm::vec3 sensorLinearDisplacement = transformVectorFast(worldToSensorMatrix, worldLinearDisplacement);
        glm::quat sensorAngularDisplacement = glmExtractRotation(worldToSensorMatrix) * worldAngularDisplacement * glmExtractRotation(sensorToWorldMatrix);

        glm::mat4 newBodyMat = createMatFromQuatAndPos(sensorAngularDisplacement * glmExtractRotation(currentBodyMatrix),
                                                       sensorLinearDisplacement + extractTranslation(currentBodyMatrix));
        if (myAvatar.getSitStandStateChange()) {
            myAvatar.setSitStandStateChange(false);
            deactivate(Vertical);
            setTranslation(newBodyMat, extractTranslation(myAvatar.deriveBodyFromHMDSensor()));
        }
        return newBodyMat;
    } else {
        return currentBodyMatrix;
    }
}

bool MyAvatar::FollowHelper::getForceActivateRotation() const {
    return _forceActivateRotation;
}

void MyAvatar::FollowHelper::setForceActivateRotation(bool val) {
    _forceActivateRotation = val;
}

bool MyAvatar::FollowHelper::getForceActivateVertical() const {
    return _forceActivateVertical;
}

void MyAvatar::FollowHelper::setForceActivateVertical(bool val) {
    _forceActivateVertical = val;
}

bool MyAvatar::FollowHelper::getForceActivateHorizontal() const {
    return _forceActivateHorizontal;
}

void MyAvatar::FollowHelper::setForceActivateHorizontal(bool val) {
    _forceActivateHorizontal = val;
}

float MyAvatar::getAccelerationEnergy() {
    glm::vec3 velocity = getWorldVelocity();
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
    glm::vec3 pos = getWorldPosition();
    glm::vec3 changeInPosition = pos - lastPosition;
    lastPosition = pos;
    return (changeInPosition.length() > MAX_AVATAR_MOVEMENT_PER_FRAME);
}

bool MyAvatar::hasDriveInput() const {
    return fabsf(getDriveKey(TRANSLATE_X)) > 0.0f || fabsf(getDriveKey(TRANSLATE_Y)) > 0.0f || fabsf(getDriveKey(TRANSLATE_Z)) > 0.0f;
}

void MyAvatar::setAway(bool value) {
    _isAway = value;
    if (_isAway) {
        emit wentAway();
    } else {
        emit wentActive();
    }
}

// The resulting matrix is used to render the hand controllers, even if the camera is decoupled from the avatar.
// Specificly, if we are rendering using a third person camera.  We would like to render the hand controllers in front of the camera,
// not in front of the avatar.
glm::mat4 MyAvatar::computeCameraRelativeHandControllerMatrix(const glm::mat4& controllerSensorMatrix) const {

    // Fetch the current camera transform.
    glm::mat4 cameraWorldMatrix = qApp->getCamera().getTransform();
    if (qApp->getCamera().getMode() == CAMERA_MODE_MIRROR) {
        cameraWorldMatrix *= createMatFromScaleQuatAndPos(vec3(-1.0f, 1.0f, 1.0f), glm::quat(), glm::vec3());
    }

    // move the camera into sensor space.
    glm::mat4 cameraSensorMatrix = glm::inverse(getSensorToWorldMatrix()) * cameraWorldMatrix;

    // cancel out scale
    glm::vec3 scale = extractScale(cameraSensorMatrix);
    cameraSensorMatrix = glm::scale(cameraSensorMatrix, 1.0f / scale);

    // measure the offset from the hmd and the camera, in sensor space
    glm::mat4 delta = cameraSensorMatrix * glm::inverse(getHMDSensorMatrix());

    // apply the delta offset to the controller, in sensor space, then transform it into world space.
    glm::mat4 controllerWorldMatrix = getSensorToWorldMatrix() * delta * controllerSensorMatrix;

    // transform controller into avatar space
    glm::mat4 avatarMatrix = createMatFromQuatAndPos(getWorldOrientation(), getWorldPosition());
    return glm::inverse(avatarMatrix) * controllerWorldMatrix;
}

glm::quat MyAvatar::getAbsoluteJointRotationInObjectFrame(int index) const {
    if (index < 0) {
        index += numeric_limits<unsigned short>::max() + 1; // 65536
    }

    switch (index) {
        case CONTROLLER_LEFTHAND_INDEX: {
            return getControllerPoseInAvatarFrame(controller::Action::LEFT_HAND).getRotation();
        }
        case CONTROLLER_RIGHTHAND_INDEX: {
            return getControllerPoseInAvatarFrame(controller::Action::RIGHT_HAND).getRotation();
        }
        case CAMERA_RELATIVE_CONTROLLER_LEFTHAND_INDEX: {
            auto pose = getControllerPoseInSensorFrame(controller::Action::LEFT_HAND);
            glm::mat4 controllerSensorMatrix = createMatFromQuatAndPos(pose.rotation, pose.translation);
            glm::mat4 result = computeCameraRelativeHandControllerMatrix(controllerSensorMatrix);
            return glmExtractRotation(result);
        }
        case CAMERA_RELATIVE_CONTROLLER_RIGHTHAND_INDEX: {
            auto pose = getControllerPoseInSensorFrame(controller::Action::RIGHT_HAND);
            glm::mat4 controllerSensorMatrix = createMatFromQuatAndPos(pose.rotation, pose.translation);
            glm::mat4 result = computeCameraRelativeHandControllerMatrix(controllerSensorMatrix);
            return glmExtractRotation(result);
        }
        case CAMERA_MATRIX_INDEX: {
            bool success;
            Transform avatarTransform;
            Transform::mult(avatarTransform, getParentTransform(success), getLocalTransform());
            glm::mat4 invAvatarMat = avatarTransform.getInverseMatrix();
            return glmExtractRotation(invAvatarMat * qApp->getCamera().getTransform());
        }
        default: {
            return Avatar::getAbsoluteJointRotationInObjectFrame(index);
        }
    }
}

glm::vec3 MyAvatar::getAbsoluteJointTranslationInObjectFrame(int index) const {
    if (index < 0) {
        index += numeric_limits<unsigned short>::max() + 1; // 65536
    }

    switch (index) {
        case CONTROLLER_LEFTHAND_INDEX: {
            return getControllerPoseInAvatarFrame(controller::Action::LEFT_HAND).getTranslation();
        }
        case CONTROLLER_RIGHTHAND_INDEX: {
            return getControllerPoseInAvatarFrame(controller::Action::RIGHT_HAND).getTranslation();
        }
        case CAMERA_RELATIVE_CONTROLLER_LEFTHAND_INDEX: {
            auto pose = getControllerPoseInSensorFrame(controller::Action::LEFT_HAND);
            glm::mat4 controllerSensorMatrix = createMatFromQuatAndPos(pose.rotation, pose.translation);
            glm::mat4 result = computeCameraRelativeHandControllerMatrix(controllerSensorMatrix);
            return extractTranslation(result);
        }
        case CAMERA_RELATIVE_CONTROLLER_RIGHTHAND_INDEX: {
            auto pose = getControllerPoseInSensorFrame(controller::Action::RIGHT_HAND);
            glm::mat4 controllerSensorMatrix = createMatFromQuatAndPos(pose.rotation, pose.translation);
            glm::mat4 result = computeCameraRelativeHandControllerMatrix(controllerSensorMatrix);
            return extractTranslation(result);
        }
        case CAMERA_MATRIX_INDEX: {
            bool success;
            Transform avatarTransform;
            Transform::mult(avatarTransform, getParentTransform(success), getLocalTransform());
            glm::mat4 invAvatarMat = avatarTransform.getInverseMatrix();
            return extractTranslation(invAvatarMat * qApp->getCamera().getTransform());
        }
        default: {
            return Avatar::getAbsoluteJointTranslationInObjectFrame(index);
        }
    }
}

glm::mat4 MyAvatar::getCenterEyeCalibrationMat() const {
    // TODO: as an optimization cache this computation, then invalidate the cache when the avatar model is changed.
    int rightEyeIndex = _skeletonModel->getRig().indexOfJoint("RightEye");
    int leftEyeIndex = _skeletonModel->getRig().indexOfJoint("LeftEye");
    if (rightEyeIndex >= 0 && leftEyeIndex >= 0) {
        auto centerEyePos = (getAbsoluteDefaultJointTranslationInObjectFrame(rightEyeIndex) + getAbsoluteDefaultJointTranslationInObjectFrame(leftEyeIndex)) * 0.5f;
        auto centerEyeRot = Quaternions::Y_180;
        return createMatFromQuatAndPos(centerEyeRot, centerEyePos / getSensorToWorldScale());
    } else {
        glm::mat4 headMat = getHeadCalibrationMat();
        return createMatFromQuatAndPos(DEFAULT_AVATAR_MIDDLE_EYE_ROT, extractTranslation(headMat) + DEFAULT_AVATAR_HEAD_TO_MIDDLE_EYE_OFFSET);
    }
}

glm::mat4 MyAvatar::getHeadCalibrationMat() const {
    // TODO: as an optimization cache this computation, then invalidate the cache when the avatar model is changed.
    int headIndex = _skeletonModel->getRig().indexOfJoint("Head");
    if (headIndex >= 0) {
        auto headPos = getAbsoluteDefaultJointTranslationInObjectFrame(headIndex);
        auto headRot = getAbsoluteDefaultJointRotationInObjectFrame(headIndex);

        return createMatFromQuatAndPos(headRot, headPos / getSensorToWorldScale());
    } else {
        return createMatFromQuatAndPos(DEFAULT_AVATAR_HEAD_ROT, DEFAULT_AVATAR_HEAD_POS);
    }
}

glm::mat4 MyAvatar::getSpine2CalibrationMat() const {
    // TODO: as an optimization cache this computation, then invalidate the cache when the avatar model is changed.
    int spine2Index = _skeletonModel->getRig().indexOfJoint("Spine2");
    if (spine2Index >= 0) {
        auto spine2Pos = getAbsoluteDefaultJointTranslationInObjectFrame(spine2Index);
        auto spine2Rot = getAbsoluteDefaultJointRotationInObjectFrame(spine2Index);
        return createMatFromQuatAndPos(spine2Rot, spine2Pos / getSensorToWorldScale());
    } else {
        return createMatFromQuatAndPos(DEFAULT_AVATAR_SPINE2_ROT, DEFAULT_AVATAR_SPINE2_POS);
    }
}

glm::mat4 MyAvatar::getHipsCalibrationMat() const {
    // TODO: as an optimization cache this computation, then invalidate the cache when the avatar model is changed.
    int hipsIndex = _skeletonModel->getRig().indexOfJoint("Hips");
    if (hipsIndex >= 0) {
        auto hipsPos = getAbsoluteDefaultJointTranslationInObjectFrame(hipsIndex);
        auto hipsRot = getAbsoluteDefaultJointRotationInObjectFrame(hipsIndex);
        return createMatFromQuatAndPos(hipsRot, hipsPos / getSensorToWorldScale());
    } else {
        return createMatFromQuatAndPos(DEFAULT_AVATAR_HIPS_ROT, DEFAULT_AVATAR_HIPS_POS);
    }
}

glm::mat4 MyAvatar::getLeftFootCalibrationMat() const {
    // TODO: as an optimization cache this computation, then invalidate the cache when the avatar model is changed.
    int leftFootIndex = _skeletonModel->getRig().indexOfJoint("LeftFoot");
    if (leftFootIndex >= 0) {
        auto leftFootPos = getAbsoluteDefaultJointTranslationInObjectFrame(leftFootIndex);
        auto leftFootRot = getAbsoluteDefaultJointRotationInObjectFrame(leftFootIndex);
        return createMatFromQuatAndPos(leftFootRot, leftFootPos / getSensorToWorldScale());
    } else {
        return createMatFromQuatAndPos(DEFAULT_AVATAR_LEFTFOOT_ROT, DEFAULT_AVATAR_LEFTFOOT_POS);
    }
}

glm::mat4 MyAvatar::getRightFootCalibrationMat() const {
    // TODO: as an optimization cache this computation, then invalidate the cache when the avatar model is changed.
    int rightFootIndex = _skeletonModel->getRig().indexOfJoint("RightFoot");
    if (rightFootIndex >= 0) {
        auto rightFootPos = getAbsoluteDefaultJointTranslationInObjectFrame(rightFootIndex);
        auto rightFootRot = getAbsoluteDefaultJointRotationInObjectFrame(rightFootIndex);
        return createMatFromQuatAndPos(rightFootRot, rightFootPos / getSensorToWorldScale());
    } else {
        return createMatFromQuatAndPos(DEFAULT_AVATAR_RIGHTFOOT_ROT, DEFAULT_AVATAR_RIGHTFOOT_POS);
    }
}

glm::mat4 MyAvatar::getRightArmCalibrationMat() const {
    int rightArmIndex = _skeletonModel->getRig().indexOfJoint("RightArm");
    if (rightArmIndex >= 0) {
        auto rightArmPos = getAbsoluteDefaultJointTranslationInObjectFrame(rightArmIndex);
        auto rightArmRot = getAbsoluteDefaultJointRotationInObjectFrame(rightArmIndex);
        return createMatFromQuatAndPos(rightArmRot, rightArmPos / getSensorToWorldScale());
    } else {
        return createMatFromQuatAndPos(DEFAULT_AVATAR_RIGHTARM_ROT, DEFAULT_AVATAR_RIGHTARM_POS);
    }
}

glm::mat4 MyAvatar::getLeftArmCalibrationMat() const {
    int leftArmIndex = _skeletonModel->getRig().indexOfJoint("LeftArm");
    if (leftArmIndex >= 0) {
        auto leftArmPos = getAbsoluteDefaultJointTranslationInObjectFrame(leftArmIndex);
        auto leftArmRot = getAbsoluteDefaultJointRotationInObjectFrame(leftArmIndex);
        return createMatFromQuatAndPos(leftArmRot, leftArmPos / getSensorToWorldScale());
    } else {
        return createMatFromQuatAndPos(DEFAULT_AVATAR_LEFTARM_ROT, DEFAULT_AVATAR_LEFTARM_POS);
    }
}

glm::mat4 MyAvatar::getRightHandCalibrationMat() const {
    int rightHandIndex = _skeletonModel->getRig().indexOfJoint("RightHand");
    if (rightHandIndex >= 0) {
        auto rightHandPos = getAbsoluteDefaultJointTranslationInObjectFrame(rightHandIndex);
        auto rightHandRot = getAbsoluteDefaultJointRotationInObjectFrame(rightHandIndex);
        return createMatFromQuatAndPos(rightHandRot, rightHandPos / getSensorToWorldScale());
    } else {
        return createMatFromQuatAndPos(DEFAULT_AVATAR_RIGHTHAND_ROT, DEFAULT_AVATAR_RIGHTHAND_POS);
    }
}

glm::mat4 MyAvatar::getLeftHandCalibrationMat() const {
    int leftHandIndex = _skeletonModel->getRig().indexOfJoint("LeftHand");
    if (leftHandIndex >= 0) {
        auto leftHandPos = getAbsoluteDefaultJointTranslationInObjectFrame(leftHandIndex);
        auto leftHandRot = getAbsoluteDefaultJointRotationInObjectFrame(leftHandIndex);
        return createMatFromQuatAndPos(leftHandRot, leftHandPos / getSensorToWorldScale());
    } else {
        return createMatFromQuatAndPos(DEFAULT_AVATAR_LEFTHAND_ROT, DEFAULT_AVATAR_LEFTHAND_POS);
    }
}

bool MyAvatar::pinJoint(int index, const glm::vec3& position, const glm::quat& orientation) {
    std::lock_guard<std::mutex> guard(_pinnedJointsMutex);
    auto hipsIndex = getJointIndex("Hips");
    if (index != hipsIndex) {
        qWarning() << "Pinning is only supported for the hips joint at the moment.";
        return false;
    }

    slamPosition(position);
    setWorldOrientation(orientation);

    auto it = std::find(_pinnedJoints.begin(), _pinnedJoints.end(), index);
    if (it == _pinnedJoints.end()) {
        _pinnedJoints.push_back(index);
    }

    return true;
}

bool MyAvatar::isJointPinned(int index) {
    std::lock_guard<std::mutex> guard(_pinnedJointsMutex);
    auto it = std::find(_pinnedJoints.begin(), _pinnedJoints.end(), index);
    return it != _pinnedJoints.end();
}

bool MyAvatar::clearPinOnJoint(int index) {
    std::lock_guard<std::mutex> guard(_pinnedJointsMutex);
    auto it = std::find(_pinnedJoints.begin(), _pinnedJoints.end(), index);
    if (it != _pinnedJoints.end()) {
        _pinnedJoints.erase(it);
        return true;
    }
    return false;
}

float MyAvatar::getIKErrorOnLastSolve() const {
    return _skeletonModel->getRig().getIKErrorOnLastSolve();
}

// thread-safe
void MyAvatar::addHoldAction(AvatarActionHold* holdAction) {
    std::lock_guard<std::mutex> guard(_holdActionsMutex);
    _holdActions.push_back(holdAction);
}

// thread-safe
void MyAvatar::removeHoldAction(AvatarActionHold* holdAction) {
    std::lock_guard<std::mutex> guard(_holdActionsMutex);
    auto iter = std::find(std::begin(_holdActions), std::end(_holdActions), holdAction);
    if (iter != std::end(_holdActions)) {
        _holdActions.erase(iter);
    }
}

void MyAvatar::updateHoldActions(const AnimPose& prePhysicsPose, const AnimPose& postUpdatePose) {
    auto entityTreeRenderer = qApp->getEntities();
    EntityTreePointer entityTree = entityTreeRenderer ? entityTreeRenderer->getTree() : nullptr;
    if (entityTree) {
        // lateAvatarUpdate will modify entity position & orientation, so we need an entity write lock
        entityTree->withWriteLock([&] {

            // to prevent actions from adding or removing themselves from the _holdActions vector
            // while we are iterating, we need to enter a critical section.
            std::lock_guard<std::mutex> guard(_holdActionsMutex);

            for (auto& holdAction : _holdActions) {
                holdAction->lateAvatarUpdate(prePhysicsPose, postUpdatePose);
            }
        });
    }
}

bool MyAvatar::isRecenteringHorizontally() const {
    return _follow.isActive(FollowHelper::Horizontal);
}

const MyHead* MyAvatar::getMyHead() const {
    return static_cast<const MyHead*>(getHead());
}

void MyAvatar::setModelScale(float scale) {
    bool changed = (scale != getModelScale());
    Avatar::setModelScale(scale);
    if (changed) {
        float sensorToWorldScale = getEyeHeight() / getUserEyeHeight();
        emit sensorToWorldScaleChanged(sensorToWorldScale);
        emit scaleChanged();
    }
}

SpatialParentTree* MyAvatar::getParentTree() const {
    auto entityTreeRenderer = qApp->getEntities();
    EntityTreePointer entityTree = entityTreeRenderer ? entityTreeRenderer->getTree() : nullptr;
    return entityTree.get();
}

const QUuid MyAvatar::grab(const QUuid& targetID, int parentJointIndex,
                           glm::vec3 positionalOffset, glm::quat rotationalOffset) {
    auto grabID = QUuid::createUuid();
    // create a temporary grab object to get grabData

    QString hand = "none";
    if (parentJointIndex == CONTROLLER_RIGHTHAND_INDEX ||
        parentJointIndex == CAMERA_RELATIVE_CONTROLLER_RIGHTHAND_INDEX ||
        parentJointIndex == FARGRAB_RIGHTHAND_INDEX ||
        parentJointIndex == getJointIndex("RightHand")) {
        hand = "right";
    } else if (parentJointIndex == CONTROLLER_LEFTHAND_INDEX ||
               parentJointIndex == CAMERA_RELATIVE_CONTROLLER_LEFTHAND_INDEX ||
               parentJointIndex == FARGRAB_LEFTHAND_INDEX ||
               parentJointIndex == getJointIndex("LeftHand")) {
        hand = "left";
    }

    Grab tmpGrab(DependencyManager::get<NodeList>()->getSessionUUID(),
                 targetID, parentJointIndex, hand, positionalOffset, rotationalOffset);
    QByteArray grabData = tmpGrab.toByteArray();
    bool dataChanged = updateAvatarGrabData(grabID, grabData);

    if (dataChanged && _clientTraitsHandler) {
        // indicate that the changed data should be sent to the mixer
        _clientTraitsHandler->markInstancedTraitUpdated(AvatarTraits::Grab, grabID);
    }

    return grabID;
}

void MyAvatar::releaseGrab(const QUuid& grabID) {
    bool tellHandler { false };

    _avatarGrabsLock.withWriteLock([&] {
        if (_avatarGrabData.remove(grabID)) {
            _deletedAvatarGrabs.insert(grabID);
            tellHandler = true;
        }
    });

    if (tellHandler && _clientTraitsHandler) {
        // indicate the deletion of the data to the mixer
        _clientTraitsHandler->markInstancedTraitDeleted(AvatarTraits::Grab, grabID);
    }
}
