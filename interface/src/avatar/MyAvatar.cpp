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
#include <AnimInverseKinematics.h>
#include <recording/Deck.h>
#include <recording/Recorder.h>
#include <recording/Clip.h>
#include <recording/Frame.h>
#include <RecordingScriptingInterface.h>
#include <trackers/FaceTracker.h>

#include "MyHead.h"
#include "MySkeletonModel.h"
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
const QString& DEFAULT_AVATAR_COLLISION_SOUND_URL = "https://hifi-public.s3.amazonaws.com/sounds/Collisions-otherorganic/Body_Hits_Impact.wav";

const float MyAvatar::ZOOM_MIN = 0.5f;
const float MyAvatar::ZOOM_MAX = 25.0f;
const float MyAvatar::ZOOM_DEFAULT = 1.5f;

MyAvatar::MyAvatar(QThread* thread) :
    Avatar(thread),
    _yawSpeed(YAW_SPEED_DEFAULT),
    _pitchSpeed(PITCH_SPEED_DEFAULT),
    _scriptedMotorTimescale(DEFAULT_SCRIPTED_MOTOR_TIMESCALE),
    _scriptedMotorFrame(SCRIPTED_MOTOR_CAMERA_FRAME),
    _motionBehaviors(AVATAR_MOTION_DEFAULTS),
    _characterController(this),
    _eyeContactTarget(LEFT_EYE),
    _realWorldFieldOfView("realWorldFieldOfView",
                          DEFAULT_REAL_WORLD_FIELD_OF_VIEW_DEGREES),
    _useAdvancedMovementControls("advancedMovementForHandControllersIsChecked", false),
    _smoothOrientationTimer(std::numeric_limits<float>::max()),
    _smoothOrientationInitial(),
    _smoothOrientationTarget(),
    _hmdSensorMatrix(),
    _hmdSensorOrientation(),
    _hmdSensorPosition(),
    _bodySensorMatrix(),
    _goToPending(false),
    _goToPosition(),
    _goToOrientation(),
    _prevShouldDrawHead(true),
    _audioListenerMode(FROM_HEAD),
    _hmdAtRestDetector(glm::vec3(0), glm::quat())
{

    // give the pointer to our head to inherited _headData variable from AvatarData
    _headData = new MyHead(this);

    _skeletonModel = std::make_shared<MySkeletonModel>(this, nullptr);
    connect(_skeletonModel.get(), &Model::setURLFinished, this, &Avatar::setModelURLFinished);


    using namespace recording;
    _skeletonModel->flagAsCauterized();

    clearDriveKeys();

    // Necessary to select the correct slot
    using SlotType = void(MyAvatar::*)(const glm::vec3&, bool, const glm::quat&, bool);

    // connect to AddressManager signal for location jumps
    connect(DependencyManager::get<AddressManager>().data(), &AddressManager::locationChangeRequired,
            this, static_cast<SlotType>(&MyAvatar::goToLocation));

    // handle scale constraints imposed on us by the domain-server
    auto& domainHandler = DependencyManager::get<NodeList>()->getDomainHandler();

    // when we connect to a domain and retrieve its settings, we restrict our max/min scale based on those settings
    connect(&domainHandler, &DomainHandler::settingsReceived, this, &MyAvatar::restrictScaleFromDomainSettings);

    // when we leave a domain we lift whatever restrictions that domain may have placed on our scale
    connect(&domainHandler, &DomainHandler::disconnectedFromDomain, this, &MyAvatar::clearScaleRestriction);

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
}

void MyAvatar::setDominantHand(const QString& hand) {
    if (hand == DOMINANT_LEFT_HAND || hand == DOMINANT_RIGHT_HAND) {
        _dominantHand = hand;
        emit dominantHandChanged(_dominantHand);
    }
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
    Avatar::setOrientation(quatFromVariant(newOrientationVar));
}

QVariant MyAvatar::getOrientationVar() const {
    return quatToVariant(Avatar::getOrientation());
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
    _globalPosition = getPosition();
    // This might not be right! Isn't the capsule local offset in avatar space, and don't we need to add the radius to the y as well? -HRS 5/26/17
    _globalBoundingBoxDimensions.x = _characterController.getCapsuleRadius();
    _globalBoundingBoxDimensions.y = _characterController.getCapsuleHalfHeight();
    _globalBoundingBoxDimensions.z = _characterController.getCapsuleRadius();
    _globalBoundingBoxOffset = _characterController.getCapsuleLocalOffset();
    if (mode == CAMERA_MODE_THIRD_PERSON || mode == CAMERA_MODE_INDEPENDENT) {
        // fake the avatar position that is sent up to the AvatarMixer
        glm::vec3 oldPosition = getPosition();
        setPosition(getSkeletonPosition());
        QByteArray array = AvatarData::toByteArrayStateful(dataDetail);
        // copy the correct position back
        setPosition(oldPosition);
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
        float downStep = glm::dot(worldBodyPos - getPosition(), _worldUpDirection);
        if (downStep < -0.5f * _characterController.getCapsuleHalfHeight() + _characterController.getCapsuleRadius()) {
            worldBodyPos -= downStep * _worldUpDirection;
        }
    }

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
    _headControllerFacingMovingAverage = lerp(_headControllerFacingMovingAverage, _headControllerFacing, tau);

    if (_smoothOrientationTimer < SMOOTH_TIME_ORIENTATION) {
        _rotationChanged = usecTimestampNow();
        _smoothOrientationTimer += deltaTime;
    }

#ifdef DEBUG_DRAW_HMD_MOVING_AVERAGE
    glm::vec3 p = transformPoint(getSensorToWorldMatrix(), getControllerPoseInAvatarFrame(controller::Pose::HEAD) *
                                 glm::vec3(_headControllerFacingMovingAverage.x, 0.0f, _headControllerFacingMovingAverage.y));
    DebugDraw::getInstance().addMarker("facing-avg", getOrientation(), p, glm::vec4(1.0f));
    p = transformPoint(getSensorToWorldMatrix(), getHMDSensorPosition() +
                       glm::vec3(_headControllerFacing.x, 0.0f, _headControllerFacing.y));
    DebugDraw::getInstance().addMarker("facing", getOrientation(), p, glm::vec4(1.0f));
#endif

    if (_goToPending) {
        setPosition(_goToPosition);
        setOrientation(_goToOrientation);
        _headControllerFacingMovingAverage = _headControllerFacing;  // reset moving average
        _goToPending = false;
        // updateFromHMDSensorMatrix (called from paintGL) expects that the sensorToWorldMatrix is updated for any position changes
        // that happen between render and Application::update (which calls updateSensorToWorldMatrix to do so).
        // However, render/MyAvatar::update/Application::update don't always match (e.g., when using the separate avatar update thread),
        // so we update now. It's ok if it updates again in the normal way.
        updateSensorToWorldMatrix();
        emit positionGoneTo();
        // Run safety tests as soon as we can after goToLocation, or clear if we're not colliding.
        _physicsSafetyPending = getCollisionsEnabled();
    }
    if (_physicsSafetyPending && qApp->isPhysicsEnabled() && _characterController.isEnabledAndReady()) {
        // When needed and ready, arrange to check and fix.
        _physicsSafetyPending = false;
        safeLanding(_goToPosition); // no-op if already safe
    }

    Head* head = getHead();
    head->relax(deltaTime);
    updateFromTrackers(deltaTime);

    //  Get audio loudness data from audio input device
    // Also get the AudioClient so we can update the avatar bounding box data
    // on the AudioClient side.
    auto audio = DependencyManager::get<AudioClient>();
    setAudioLoudness(audio->getLastInputLoudness());
    setAudioAverageLoudness(audio->getAudioAverageInputLoudness());

    glm::vec3 halfBoundingBoxDimensions(_characterController.getCapsuleRadius(), _characterController.getCapsuleHalfHeight(), _characterController.getCapsuleRadius());
    // This might not be right! Isn't the capsule local offset in avatar space? -HRS 5/26/17
    halfBoundingBoxDimensions += _characterController.getCapsuleLocalOffset();
    QMetaObject::invokeMethod(audio.data(), "setAvatarBoundingBoxParameters",
        Q_ARG(glm::vec3, (getPosition() - halfBoundingBoxDimensions)),
        Q_ARG(glm::vec3, (halfBoundingBoxDimensions*2.0f)));

    if (getIdentityDataChanged()) {
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

    if (!_skeletonModel->hasSkeleton()) {
        // All the simulation that can be done has been done
        getHead()->setPosition(getPosition()); // so audio-position isn't 0,0,0
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
            headPosition = getPosition();
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
            auto now = usecTimestampNow();
            EntityEditPacketSender* packetSender = qApp->getEntityEditPacketSender();
            MovingEntitiesOperator moveOperator(entityTree);
            forEachDescendant([&](SpatiallyNestablePointer object) {
                // if the queryBox has changed, tell the entity-server
                if (object->getNestableType() == NestableType::Entity && object->checkAndMaybeUpdateQueryAACube()) {
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
        _characterController.setFlyingAllowed(zoneAllowsFlying && _enableFlying);
        _characterController.setCollisionlessAllowed(collisionlessAllowed);
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
        _headControllerFacing = getFacingDir2D(headPose.rotation);
    } else {
        _headControllerFacing = glm::vec2(1.0f, 0.0f);
    }
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
    glm::mat4 desiredMat = createMatFromScaleQuatAndPos(glm::vec3(sensorToWorldScale), getOrientation(), getPosition());
    _sensorToWorldMatrix = desiredMat * glm::inverse(_bodySensorMatrix);

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
    glm::vec3 estimatedRotation;

    bool inHmd = qApp->isHMDMode();
    bool playing = DependencyManager::get<recording::Deck>()->isPlaying();
    if (inHmd && playing) {
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
    glm::vec3 jointPos = getPosition();//default value if no or invalid joint specified
    glm::quat jointRot = getRotation();//default value if no or invalid joint specified
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
    glm::quat jointRot = getRotation();//default value if no or invalid joint specified
    if ((jointIndex != -1) && (!_skeletonModel->getJointRotationInWorldFrame(jointIndex, jointRot))) {
        qWarning() << "Invalid joint index specified: " << jointIndex;
    }

    glm::vec3 jointSpaceDir = glm::inverse(jointRot) * worldDir;
    return jointSpaceDir;
}

glm::quat MyAvatar::worldToJointRotation(const glm::quat& worldRot, const int jointIndex) const {
    glm::quat jointRot = getRotation();//default value if no or invalid joint specified
    if ((jointIndex != -1) && (!_skeletonModel->getJointRotationInWorldFrame(jointIndex, jointRot))) {
        qWarning() << "Invalid joint index specified: " << jointIndex;
    }
    glm::quat jointSpaceRot = glm::inverse(jointRot) * worldRot;
    return jointSpaceRot;
}

glm::vec3 MyAvatar::jointToWorldPoint(const glm::vec3& jointSpacePos, const int jointIndex) const {
    glm::vec3 jointPos = getPosition();//default value if no or invalid joint specified
    glm::quat jointRot = getRotation();//default value if no or invalid joint specified

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
    glm::quat jointRot = getRotation();//default value if no or invalid joint specified
    if ((jointIndex != -1) && (!_skeletonModel->getJointRotationInWorldFrame(jointIndex, jointRot))) {
        qWarning() << "Invalid joint index specified: " << jointIndex;
    }
    glm::vec3 worldDir = jointRot * jointSpaceDir;
    return worldDir;
}

glm::quat MyAvatar::jointToWorldRotation(const glm::quat& jointSpaceRot, const int jointIndex) const {
    glm::quat jointRot = getRotation();//default value if no or invalid joint specified
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

void MyAvatar::saveData() {
    Settings settings;
    settings.beginGroup("Avatar");

    settings.setValue("dominantHand", _dominantHand);
    settings.setValue("headPitch", getHead()->getBasePitch());

    settings.setValue("scale", _targetScale);

    settings.setValue("yawSpeed", _yawSpeed);
    settings.setValue("pitchSpeed", _pitchSpeed);

    // only save the fullAvatarURL if it has not been overwritten on command line
    // (so the overrideURL is not valid), or it was overridden _and_ we specified
    // --replaceAvatarURL (so _saveAvatarOverrideUrl is true)
    if (qApp->getSaveAvatarOverrideUrl() || !qApp->getAvatarOverrideUrl().isValid() ) {
        settings.setValue("fullAvatarURL",
                      _fullAvatarURLFromPreferences == AvatarData::defaultFullAvatarModelUrl() ?
                      "" :
                      _fullAvatarURLFromPreferences.toString());
    }

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

    if (_avatarEntityData.size() == 0) {
        // HACK: manually remove empty 'avatarEntityData' else deleted avatarEntityData may show up in settings file
        settings.remove("avatarEntityData");
    }

    settings.beginWriteArray("avatarEntityData");
    int avatarEntityIndex = 0;
    auto hmdInterface = DependencyManager::get<HMDScriptingInterface>();
    _avatarEntitiesLock.withReadLock([&] {
        for (auto entityID : _avatarEntityData.keys()) {
            if (hmdInterface->getCurrentTabletFrameID() == entityID) {
                // don't persist the tablet between domains / sessions
                continue;
            }

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
    settings.setValue("userHeight", getUserHeight());

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
    _skeletonModel->setVisibleInScene(isEnabled, qApp->getMain3DScene());
}

void MyAvatar::setUseAnimPreAndPostRotations(bool isEnabled) {
    AnimClip::usePreAndPostPoseFromAnim = isEnabled;
    reset(true);
}

void MyAvatar::setEnableInverseKinematics(bool isEnabled) {
    _skeletonModel->getRig().setEnableInverseKinematics(isEnabled);
}

void MyAvatar::loadData() {
    Settings settings;
    settings.beginGroup("Avatar");

    getHead()->setBasePitch(loadSetting(settings, "headPitch", 0.0f));

    _yawSpeed = loadSetting(settings, "yawSpeed", _yawSpeed);
    _pitchSpeed = loadSetting(settings, "pitchSpeed", _pitchSpeed);

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
    if (avatarEntityCount == 0) {
        // HACK: manually remove empty 'avatarEntityData' else legacy data may persist in settings file
        settings.remove("avatarEntityData");
    }
    setAvatarEntityDataChanged(true);

    setDisplayName(settings.value("displayName").toString());
    setCollisionSoundURL(settings.value("collisionSoundURL", DEFAULT_AVATAR_COLLISION_SOUND_URL).toString());
    setSnapTurn(settings.value("useSnapTurn", _useSnapTurn).toBool());
    setClearOverlayWhenMoving(settings.value("clearOverlayWhenMoving", _clearOverlayWhenMoving).toBool());
    setDominantHand(settings.value("dominantHand", _dominantHand).toString().toLower());
    setUserHeight(settings.value("userHeight", DEFAULT_AVATAR_HEIGHT).toDouble());
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

ScriptAvatarData* MyAvatar::getTargetAvatar() const {
    auto avatar = std::static_pointer_cast<Avatar>(_lookAtTargetAvatar.lock());
    if (avatar) {
        return new ScriptAvatar(avatar);
    } else {
        return nullptr;
    }
}

void MyAvatar::updateLookAtTargetAvatar() {
    //
    //  Look at the avatar whose eyes are closest to the ray in direction of my avatar's head
    //  And set the correctedLookAt for all (nearby) avatars that are looking at me.
    _lookAtTargetAvatar.reset();
    _targetAvatarPosition = glm::vec3(0.0f);

    glm::vec3 lookForward = getHead()->getFinalOrientationInWorldFrame() * IDENTITY_FORWARD;
    glm::vec3 cameraPosition = qApp->getCamera().getPosition();

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
            (distanceTo < GREATEST_LOOKING_AT_DISTANCE * getModelScale())) {
            float radius = glm::length(avatar->getHead()->getEyePosition() - avatar->getHead()->getRightEyePosition());
            float angleTo = coneSphereAngle(getHead()->getEyePosition(), lookForward, avatar->getHead()->getEyePosition(), radius);
            if (angleTo < (smallestAngleTo * (isCurrentTarget ? KEEP_LOOKING_AT_CURRENT_ANGLE_FACTOR : 1.0f))) {
                _lookAtTargetAvatar = avatarPointer;
                _targetAvatarPosition = avatarPointer->getPosition();
            }
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
    return getPosition() + getOrientation() * Quaternions::Y_180 * _skeletonModel->getDefaultEyeModelPosition();
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
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setJointData", Q_ARG(int, index), Q_ARG(const glm::quat&, rotation),
            Q_ARG(const glm::vec3&, translation));
        return;
    }
    // HACK: ATM only JS scripts call setJointData() on MyAvatar so we hardcode the priority
    _skeletonModel->getRig().setJointState(index, true, rotation, translation, SCRIPT_PRIORITY);
}

void MyAvatar::setJointRotation(int index, const glm::quat& rotation) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setJointRotation", Q_ARG(int, index), Q_ARG(const glm::quat&, rotation));
        return;
    }
    // HACK: ATM only JS scripts call setJointData() on MyAvatar so we hardcode the priority
    _skeletonModel->getRig().setJointRotation(index, true, rotation, SCRIPT_PRIORITY);
}

void MyAvatar::setJointTranslation(int index, const glm::vec3& translation) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setJointTranslation", Q_ARG(int, index), Q_ARG(const glm::vec3&, translation));
        return;
    }
    // HACK: ATM only JS scripts call setJointData() on MyAvatar so we hardcode the priority
    _skeletonModel->getRig().setJointTranslation(index, true, translation, SCRIPT_PRIORITY);
}

void MyAvatar::clearJointData(int index) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "clearJointData", Q_ARG(int, index));
        return;
    }
    _skeletonModel->getRig().clearJointAnimationPriority(index);
}

void MyAvatar::setJointData(const QString& name, const glm::quat& rotation, const glm::vec3& translation) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setJointData", Q_ARG(QString, name), Q_ARG(const glm::quat&, rotation),
            Q_ARG(const glm::vec3&, translation));
        return;
    }
    writeLockWithNamedJointIndex(name, [&](int index) {
        // HACK: ATM only JS scripts call setJointData() on MyAvatar so we hardcode the priority
        _skeletonModel->getRig().setJointState(index, true, rotation, translation, SCRIPT_PRIORITY);
    });
}

void MyAvatar::setJointRotation(const QString& name, const glm::quat& rotation) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setJointRotation", Q_ARG(QString, name), Q_ARG(const glm::quat&, rotation));
        return;
    }
    writeLockWithNamedJointIndex(name, [&](int index) {
        // HACK: ATM only JS scripts call setJointData() on MyAvatar so we hardcode the priority
        _skeletonModel->getRig().setJointRotation(index, true, rotation, SCRIPT_PRIORITY);
    });
}

void MyAvatar::setJointTranslation(const QString& name, const glm::vec3& translation) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setJointTranslation", Q_ARG(QString, name), Q_ARG(const glm::vec3&, translation));
        return;
    }
    writeLockWithNamedJointIndex(name, [&](int index) {
        // HACK: ATM only JS scripts call setJointData() on MyAvatar so we hardcode the priority
        _skeletonModel->getRig().setJointTranslation(index, true, translation, SCRIPT_PRIORITY);
    });
}

void MyAvatar::clearJointData(const QString& name) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "clearJointData", Q_ARG(QString, name));
        return;
    }
    writeLockWithNamedJointIndex(name, [&](int index) {
        _skeletonModel->getRig().clearJointAnimationPriority(index);
    });
}

void MyAvatar::clearJointsData() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "clearJointsData");
        return;
    }
    _skeletonModel->getRig().clearJointStates();
}

void MyAvatar::setSkeletonModelURL(const QUrl& skeletonModelURL) {
    Avatar::setSkeletonModelURL(skeletonModelURL);
    _skeletonModel->setVisibleInScene(true, qApp->getMain3DScene());
    _headBoneSet.clear();
    emit skeletonChanged();
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
    markIdentityDataChanged();
}

void MyAvatar::setAttachmentData(const QVector<AttachmentData>& attachmentData) {
    if (QThread::currentThread() != thread()) {
        BLOCKING_INVOKE_METHOD(this, "setAttachmentData",
                                  Q_ARG(const QVector<AttachmentData>, attachmentData));
        return;
    }
    Avatar::setAttachmentData(attachmentData);
}

glm::vec3 MyAvatar::getSkeletonPosition() const {
    CameraMode mode = qApp->getCamera().getMode();
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
        glm::mat4 invAvatarMatrix = glm::inverse(createMatFromQuatAndPos(getOrientation(), getPosition()));
        return pose.transform(invAvatarMatrix);
    } else {
        return controller::Pose(); // invalid pose
    }
}

void MyAvatar::updateMotors() {
    _characterController.clearMotors();
    glm::quat motorRotation;
    if (_motionBehaviors & AVATAR_MOTION_ACTION_MOTOR_ENABLED) {
        if (_characterController.getState() == CharacterController::State::Hover ||
                _characterController.computeCollisionGroup() == BULLET_COLLISION_GROUP_COLLISIONLESS) {
            motorRotation = getMyHead()->getHeadOrientation();
        } else {
            // non-hovering = walking: follow camera twist about vertical but not lift
            // we decompose camera's rotation and store the twist part in motorRotation
            // however, we need to perform the decomposition in the avatar-frame
            // using the local UP axis and then transform back into world-frame
            glm::quat orientation = getOrientation();
            glm::quat headOrientation = glm::inverse(orientation) * getMyHead()->getHeadOrientation(); // avatar-frame
            glm::quat liftRotation;
            swingTwistDecomposition(headOrientation, Vectors::UNIT_Y, liftRotation, motorRotation);
            motorRotation = orientation * motorRotation;
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
            motorRotation = getMyHead()->getHeadOrientation() * glm::angleAxis(PI, Vectors::UNIT_Y);
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
    _characterController.setScaleFactor(getSensorToWorldScale());

    _characterController.setPositionAndOrientation(getPosition(), getOrientation());
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
        position = getPosition();
        orientation = getOrientation();
    }
    nextAttitude(position, orientation);
    _bodySensorMatrix = _follow.postPhysicsUpdate(*this, _bodySensorMatrix);

    if (_characterController.isEnabledAndReady()) {
        setVelocity(_characterController.getLinearVelocity() + _characterController.getFollowVelocity());
        if (_characterController.isStuck()) {
            _physicsSafetyPending = true;
            _goToPosition = getPosition();
        }
    } else {
        setVelocity(getVelocity() + _characterController.getFollowVelocity());
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

void MyAvatar::setVisibleInSceneIfReady(Model* model, const render::ScenePointer& scene, bool visible) {
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
    _skeletonModel->getRig().initAnimGraph(url);

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

    _skeletonModel->getRig().initAnimGraph(graphUrl);
    _currentAnimGraphUrl.set(graphUrl);

    _bodySensorMatrix = deriveBodyFromHMDSensor(); // Based on current cached HMD position/rotation..
    updateSensorToWorldMatrix(); // Uses updated position/orientation and _bodySensorMatrix changes
}

void MyAvatar::destroyAnimGraph() {
    _skeletonModel->getRig().destroyAnimGraph();
}

void MyAvatar::postUpdate(float deltaTime, const render::ScenePointer& scene) {

    Avatar::postUpdate(deltaTime, scene);

    if (_skeletonModel->isLoaded() && !_skeletonModel->getRig().getAnimNode()) {
        initHeadBones();
        _skeletonModel->setCauterizeBoneSet(_headBoneSet);
        _fstAnimGraphOverrideUrl = _skeletonModel->getGeometry()->getAnimGraphOverrideUrl();
        initAnimGraph();
    }

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

    DebugDraw::getInstance().updateMyAvatarPos(getPosition());
    DebugDraw::getInstance().updateMyAvatarRot(getOrientation());

    AnimPose postUpdateRoomPose(_sensorToWorldMatrix);

    updateHoldActions(_prePhysicsRoomPose, postUpdateRoomPose);

    if (_enableDebugDrawDetailedCollision) {
        AnimPose rigToWorldPose(glm::vec3(1.0f), getRotation() * Quaternions::Y_180, getPosition());
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
            const FBXGeometry& geometry = _skeletonModel->getFBXGeometry();
            for (int i = 0; i < rig.getJointStateCount(); i++) {
                AnimPose jointPose;
                rig.getAbsoluteJointPoseInRigFrame(i, jointPose);
                const FBXJointShapeInfo& shapeInfo = geometry.joints[i].shapeInfo;
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

void MyAvatar::preDisplaySide(RenderArgs* renderArgs) {

    // toggle using the cauterizedBones depending on where the camera is and the rendering pass type.
    const bool shouldDrawHead = shouldRenderHead(renderArgs);
    if (shouldDrawHead != _prevShouldDrawHead) {
        _skeletonModel->setEnableCauterization(!shouldDrawHead);

        for (int i = 0; i < _attachmentData.size(); i++) {
            if (_attachmentData[i].jointName.compare("Head", Qt::CaseInsensitive) == 0 ||
                _attachmentData[i].jointName.compare("Neck", Qt::CaseInsensitive) == 0 ||
                _attachmentData[i].jointName.compare("LeftEye", Qt::CaseInsensitive) == 0 ||
                _attachmentData[i].jointName.compare("RightEye", Qt::CaseInsensitive) == 0 ||
                _attachmentData[i].jointName.compare("HeadTop_End", Qt::CaseInsensitive) == 0 ||
                _attachmentData[i].jointName.compare("Face", Qt::CaseInsensitive) == 0) {
                _attachmentModels[i]->setVisibleInScene(shouldDrawHead, qApp->getMain3DScene());
            }
        }
    }
    _prevShouldDrawHead = shouldDrawHead;
}

const float RENDER_HEAD_CUTOFF_DISTANCE = 0.3f;

bool MyAvatar::cameraInsideHead(const glm::vec3& cameraPosition) const {
    return glm::length(cameraPosition - getHeadPosition()) < (RENDER_HEAD_CUTOFF_DISTANCE * getModelScale());
}

bool MyAvatar::shouldRenderHead(const RenderArgs* renderArgs) const {
    bool defaultMode = renderArgs->_renderMode == RenderArgs::DEFAULT_RENDER_MODE;
    bool firstPerson = qApp->getCamera().getMode() == CAMERA_MODE_FIRST_PERSON;
    bool insideHead = cameraInsideHead(renderArgs->getViewFrustum().getPosition());
    return !defaultMode || !firstPerson || !insideHead;
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
        const float MIN_CONTROL_SPEED = 0.01f;
        float speed = glm::length(getVelocity());
        if (speed >= MIN_CONTROL_SPEED) {
            // Feather turn when stopping moving.
            float speedFactor;
            if (getDriveKey(TRANSLATE_Z) != 0.0f || _lastDrivenSpeed == 0.0f) {
                _lastDrivenSpeed = speed;
                speedFactor = 1.0f;
            } else {
                speedFactor = glm::min(speed / _lastDrivenSpeed, 1.0f);
            }

            float direction = glm::dot(getVelocity(), getRotation() * Vectors::UNIT_NEG_Z) > 0.0f ? 1.0f : -1.0f;

            float rollAngle = glm::degrees(asinf(glm::dot(IDENTITY_UP, _hmdSensorOrientation * IDENTITY_RIGHT)));
            float rollSign = rollAngle < 0.0f ? -1.0f : 1.0f;
            rollAngle = fabsf(rollAngle);
            rollAngle = rollAngle > _hmdRollControlDeadZone ? rollSign * (rollAngle - _hmdRollControlDeadZone) : 0.0f;

            totalBodyYaw += speedFactor * direction * rollAngle * deltaTime * _hmdRollControlRate;
        }
    }

    // update body orientation by movement inputs
    glm::quat initialOrientation = getOrientationOutbound();
    setOrientation(getOrientation() * glm::quat(glm::radians(glm::vec3(0.0f, totalBodyYaw, 0.0f))));

    if (snapTurn) {
        // Whether or not there is an existing smoothing going on, just reset the smoothing timer and set the starting position as the avatar's current position, then smooth to the new position.
        _smoothOrientationInitial = initialOrientation;
        _smoothOrientationTarget = getOrientation();
        _smoothOrientationTimer = 0.0f;
    }

    getHead()->setBasePitch(getHead()->getBasePitch() + getDriveKey(PITCH) * _pitchSpeed * deltaTime);

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
    glm::vec3 forward = (getDriveKey(TRANSLATE_Z)) * IDENTITY_FORWARD;
    glm::vec3 right = (getDriveKey(TRANSLATE_X)) * IDENTITY_RIGHT;

    glm::vec3 direction = forward + right;
    CharacterController::State state = _characterController.getState();
    if (state == CharacterController::State::Hover ||
            _characterController.computeCollisionGroup() == BULLET_COLLISION_GROUP_COLLISIONLESS) {
        // we can fly --> support vertical motion
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
        float finalMaxMotorSpeed = getSensorToWorldScale() * DEFAULT_AVATAR_MAX_FLYING_SPEED;
        float speedGrowthTimescale  = 2.0f;
        float speedIncreaseFactor = 1.8f;
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
        _actionMotorVelocity = getSensorToWorldScale() * DEFAULT_AVATAR_MAX_WALKING_SPEED * direction;
    }

    float boomChange = getDriveKey(ZOOM);
    _boomLength += 2.0f * _boomLength * boomChange + boomChange * boomChange;
    _boomLength = glm::clamp<float>(_boomLength, ZOOM_MIN, ZOOM_MAX);
}

void MyAvatar::updatePosition(float deltaTime) {
    if (_motionBehaviors & AVATAR_MOTION_ACTION_MOTOR_ENABLED) {
        updateActionMotor(deltaTime);
    }

    vec3 velocity = getVelocity();
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
            glm::vec3 position = getPosition();
            MyCharacterController::RayShotgunResult result;
            glm::vec3 step = deltaTime * (getRotation() * _actionMotorVelocity);
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

void MyAvatar::clampTargetScaleToDomainLimits() {
    // when we're about to change the target scale because the user has asked to increase or decrease their scale,
    // we first make sure that we're starting from a target scale that is allowed by the current domain

    auto clampedTargetScale = glm::clamp(_targetScale, _domainMinimumScale, _domainMaximumScale);

    if (clampedTargetScale != _targetScale) {
        qCDebug(interfaceapp, "Clamped scale to %f since original target scale %f was not allowed by domain",
                (double)clampedTargetScale, (double)_targetScale);

        setTargetScale(clampedTargetScale);
    }
}

void MyAvatar::clampScaleChangeToDomainLimits(float desiredScale) {
    auto clampedTargetScale = glm::clamp(desiredScale, _domainMinimumScale, _domainMaximumScale);

    if (clampedTargetScale != desiredScale) {
        qCDebug(interfaceapp, "Forcing scale to %f since %f is not allowed by domain",
                clampedTargetScale, desiredScale);
    }

    setTargetScale(clampedTargetScale);
    qCDebug(interfaceapp, "Changed scale to %f", (double)_targetScale);
}

float MyAvatar::getDomainMinScale() {
    return _domainMinimumScale;
}

float MyAvatar::getDomainMaxScale() {
    return _domainMaximumScale;
}

void MyAvatar::increaseSize() {
    // make sure we're starting from an allowable scale
    clampTargetScaleToDomainLimits();

    // calculate what our new scale should be
    float updatedTargetScale = _targetScale * (1.0f + SCALING_RATIO);

    // attempt to change to desired scale (clamped to the domain limits)
    clampScaleChangeToDomainLimits(updatedTargetScale);
}

void MyAvatar::decreaseSize() {
    // make sure we're starting from an allowable scale
    clampTargetScaleToDomainLimits();

    // calculate what our new scale should be
    float updatedTargetScale = _targetScale * (1.0f - SCALING_RATIO);

    // attempt to change to desired scale (clamped to the domain limits)
    clampScaleChangeToDomainLimits(updatedTargetScale);
}

void MyAvatar::resetSize() {
    // attempt to reset avatar size to the default (clamped to domain limits)
    const float DEFAULT_AVATAR_SCALE = 1.0f;

    clampScaleChangeToDomainLimits(DEFAULT_AVATAR_SCALE);
}

void MyAvatar::restrictScaleFromDomainSettings(const QJsonObject& domainSettingsObject) {
    // pull out the minimum and maximum scale and set them to restrict our scale

    static const QString AVATAR_SETTINGS_KEY = "avatars";
    auto avatarsObject = domainSettingsObject[AVATAR_SETTINGS_KEY].toObject();

    static const QString MIN_SCALE_OPTION = "min_avatar_scale";
    float settingMinScale = avatarsObject[MIN_SCALE_OPTION].toDouble(MIN_AVATAR_SCALE);
    setDomainMinimumScale(settingMinScale);

    static const QString MAX_SCALE_OPTION = "max_avatar_scale";
    float settingMaxScale = avatarsObject[MAX_SCALE_OPTION].toDouble(MAX_AVATAR_SCALE);
    setDomainMaximumScale(settingMaxScale);

    // make sure that the domain owner didn't flip min and max
    if (_domainMinimumScale > _domainMaximumScale) {
        std::swap(_domainMinimumScale, _domainMaximumScale);
    }
    // Set avatar current scale
    Settings settings;
    settings.beginGroup("Avatar");
    _targetScale = loadSetting(settings, "scale", 1.0f);

    qCDebug(interfaceapp) << "This domain requires a minimum avatar scale of " << _domainMinimumScale
        << " and a maximum avatar scale of " << _domainMaximumScale
        << ". Current avatar scale is " << _targetScale;

    // debug to log if this avatar's scale in this domain will be clamped
    float clampedScale = glm::clamp(_targetScale, _domainMinimumScale, _domainMaximumScale);

    if (_targetScale != clampedScale) {
        qCDebug(interfaceapp) << "Current avatar scale is clamped to " << clampedScale
            << " because " << _targetScale << " is not allowed by current domain";
        // The current scale of avatar should not be more than domain's max_avatar_scale and not less than domain's min_avatar_scale .
        _targetScale = clampedScale;
    }

    setModelScale(_targetScale);
    rebuildCollisionShape();
    settings.endGroup();
}

void MyAvatar::clearScaleRestriction() {
    _domainMinimumScale = MIN_AVATAR_SCALE;
    _domainMaximumScale = MAX_AVATAR_SCALE;
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
    auto halfHeight = _characterController.getCapsuleHalfHeight() + _characterController.getCapsuleRadius();
    if (halfHeight == 0) {
        return false; // zero height avatar
    }
    auto entityTree = DependencyManager::get<EntityTreeRenderer>()->getTree();
    if (!entityTree) {
        return false; // no entity tree
    }
    // More utilities.
    const auto offset = getOrientation() *_characterController.getCapsuleLocalOffset();
    const auto capsuleCenter = positionIn + offset;
    const auto up = _worldUpDirection, down = -up;
    glm::vec3 upperIntersection, upperNormal, lowerIntersection, lowerNormal;
    EntityItemID upperId, lowerId;
    QVector<EntityItemID> include{}, ignore{};
    auto mustMove = [&] {  // Place bottom of capsule at the upperIntersection, and check again based on the capsule center.
        betterPositionOut = upperIntersection + (up * halfHeight) - offset;
        return true;
    };
    auto findIntersection = [&](const glm::vec3& startPointIn, const glm::vec3& directionIn, glm::vec3& intersectionOut, EntityItemID& entityIdOut, glm::vec3& normalOut) {
        OctreeElementPointer element;
        EntityItemPointer intersectedEntity = NULL;
        float distance;
        BoxFace face;
        const bool visibleOnly = false;
        // This isn't quite what we really want here. findRayIntersection always works on mesh, skipping entirely based on collidable.
        // What we really want is to use the collision hull!
        // See https://highfidelity.fogbugz.com/f/cases/5003/findRayIntersection-has-option-to-use-collidableOnly-but-doesn-t-actually-use-colliders
        const bool collidableOnly = true;
        const bool precisionPicking = true;
        const auto lockType = Octree::Lock; // Should we refactor to take a lock just once?
        bool* accurateResult = NULL;

        bool intersects = entityTree->findRayIntersection(startPointIn, directionIn, include, ignore, visibleOnly, collidableOnly, precisionPicking,
            element, distance, face, normalOut, (void**)&intersectedEntity, lockType, accurateResult);
        if (!intersects || !intersectedEntity) {
             return false;
        }
        intersectionOut = startPointIn + (directionIn * distance);
        entityIdOut = intersectedEntity->getEntityItemID();
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
        const float halfHeightFactor = 2.5f; // Until case 5003 is fixed (and maybe after?), we need a fudge factor. Also account for content modelers not being precise.
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
    setCollisionsEnabled(menu->isOptionChecked(MenuOption::EnableAvatarCollisions));
    setProperty("lookAtSnappingEnabled", menu->isOptionChecked(MenuOption::EnableLookAtSnapping));
}

void MyAvatar::setFlyingEnabled(bool enabled) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setFlyingEnabled", Q_ARG(bool, enabled));
        return;
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
    return _enableFlying;
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
}

bool MyAvatar::getCollisionsEnabled() {
    // may return 'false' even though the collisionless option was requested
    // because the zone may disallow collisionless avatars
    return _characterController.computeCollisionGroup() != BULLET_COLLISION_GROUP_COLLISIONLESS;
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
    if (getDriveKey(TRANSLATE_Y) > 0.0f) {
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

// old school meat hook style
glm::mat4 MyAvatar::deriveBodyFromHMDSensor() const {
    glm::vec3 headPosition;
    glm::quat headOrientation;
    auto headPose = getControllerPoseInSensorFrame(controller::Action::HEAD);
    if (headPose.isValid()) {
        headPosition = headPose.translation;
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
    glm::vec3 headToNeck = headOrientation * Quaternions::Y_180 * (localNeck - localHead);
    glm::vec3 neckToRoot = headOrientationYawOnly  * Quaternions::Y_180 * -localNeck;

    float invSensorToWorldScale = getUserEyeHeight() / getEyeHeight();
    glm::vec3 bodyPos = headPosition + invSensorToWorldScale * (headToNeck + neckToRoot);

    return createMatFromQuatAndPos(headOrientationYawOnly, bodyPos);
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

glm::vec3 MyAvatar::getPositionForAudio() {
    switch (_audioListenerMode) {
        case AudioListenerMode::FROM_HEAD:
            return getHead()->getPosition();
        case AudioListenerMode::FROM_CAMERA:
            return qApp->getCamera().getPosition();
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
            return qApp->getCamera().getOrientation();
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
    auto cameraMode = qApp->getCamera().getMode();
    if (cameraMode == CAMERA_MODE_THIRD_PERSON) {
        return false;
    } else {
        const float FOLLOW_ROTATION_THRESHOLD = cosf(PI / 6.0f); // 30 degrees
        glm::vec2 bodyFacing = getFacingDir2D(currentBodyMatrix);
        return glm::dot(-myAvatar.getHeadControllerFacingMovingAverage(), bodyFacing) < FOLLOW_ROTATION_THRESHOLD;
    }
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

void MyAvatar::FollowHelper::prePhysicsUpdate(MyAvatar& myAvatar, const glm::mat4& desiredBodyMatrix,
                                              const glm::mat4& currentBodyMatrix, bool hasDriveInput) {

    if (myAvatar.getHMDLeanRecenterEnabled() &&
        qApp->getCamera().getMode() != CAMERA_MODE_MIRROR) {
        if (!isActive(Rotation) && (shouldActivateRotation(myAvatar, desiredBodyMatrix, currentBodyMatrix) || hasDriveInput)) {
            activate(Rotation);
        }
        if (!isActive(Horizontal) && shouldActivateHorizontal(myAvatar, desiredBodyMatrix, currentBodyMatrix)) {
            activate(Horizontal);
        }
        if (!isActive(Vertical) && (shouldActivateVertical(myAvatar, desiredBodyMatrix, currentBodyMatrix) || hasDriveInput)) {
            activate(Vertical);
        }
    }

    glm::mat4 desiredWorldMatrix = myAvatar.getSensorToWorldMatrix() * desiredBodyMatrix;
    glm::mat4 currentWorldMatrix = myAvatar.getSensorToWorldMatrix() * currentBodyMatrix;

    AnimPose followWorldPose(currentWorldMatrix);

    // remove scale present from sensorToWorldMatrix
    followWorldPose.scale() = glm::vec3(1.0f);

    if (isActive(Rotation)) {
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

glm::mat4 MyAvatar::FollowHelper::postPhysicsUpdate(const MyAvatar& myAvatar, const glm::mat4& currentBodyMatrix) {
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
    glm::mat4 avatarMatrix = createMatFromQuatAndPos(getOrientation(), getPosition());
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
        return createMatFromQuatAndPos(centerEyeRot, centerEyePos);
    } else {
        return createMatFromQuatAndPos(DEFAULT_AVATAR_MIDDLE_EYE_ROT, DEFAULT_AVATAR_MIDDLE_EYE_POS);
    }
}

glm::mat4 MyAvatar::getHeadCalibrationMat() const {
    // TODO: as an optimization cache this computation, then invalidate the cache when the avatar model is changed.
    int headIndex = _skeletonModel->getRig().indexOfJoint("Head");
    if (headIndex >= 0) {
        auto headPos = getAbsoluteDefaultJointTranslationInObjectFrame(headIndex);
        auto headRot = getAbsoluteDefaultJointRotationInObjectFrame(headIndex);
        return createMatFromQuatAndPos(headRot, headPos);
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
        return createMatFromQuatAndPos(spine2Rot, spine2Pos);
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
        return createMatFromQuatAndPos(hipsRot, hipsPos);
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
        return createMatFromQuatAndPos(leftFootRot, leftFootPos);
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
        return createMatFromQuatAndPos(rightFootRot, rightFootPos);
    } else {
        return createMatFromQuatAndPos(DEFAULT_AVATAR_RIGHTFOOT_ROT, DEFAULT_AVATAR_RIGHTFOOT_POS);
    }
}


glm::mat4 MyAvatar::getRightArmCalibrationMat() const {
    int rightArmIndex = _skeletonModel->getRig().indexOfJoint("RightArm");
    if (rightArmIndex >= 0) {
        auto rightArmPos = getAbsoluteDefaultJointTranslationInObjectFrame(rightArmIndex);
        auto rightArmRot = getAbsoluteDefaultJointRotationInObjectFrame(rightArmIndex);
        return createMatFromQuatAndPos(rightArmRot, rightArmPos);
    } else {
        return createMatFromQuatAndPos(DEFAULT_AVATAR_RIGHTARM_ROT, DEFAULT_AVATAR_RIGHTARM_POS);
    }
}

glm::mat4 MyAvatar::getLeftArmCalibrationMat() const {
    int leftArmIndex = _skeletonModel->getRig().indexOfJoint("LeftArm");
    if (leftArmIndex >= 0) {
        auto leftArmPos = getAbsoluteDefaultJointTranslationInObjectFrame(leftArmIndex);
        auto leftArmRot = getAbsoluteDefaultJointRotationInObjectFrame(leftArmIndex);
        return createMatFromQuatAndPos(leftArmRot, leftArmPos);
    } else {
        return createMatFromQuatAndPos(DEFAULT_AVATAR_LEFTARM_ROT, DEFAULT_AVATAR_LEFTARM_POS);
    }
}

glm::mat4 MyAvatar::getRightHandCalibrationMat() const {
    int rightHandIndex = _skeletonModel->getRig().indexOfJoint("RightHand");
    if (rightHandIndex >= 0) {
        auto rightHandPos = getAbsoluteDefaultJointTranslationInObjectFrame(rightHandIndex);
        auto rightHandRot = getAbsoluteDefaultJointRotationInObjectFrame(rightHandIndex);
        return createMatFromQuatAndPos(rightHandRot, rightHandPos);
    } else {
        return createMatFromQuatAndPos(DEFAULT_AVATAR_RIGHTHAND_ROT, DEFAULT_AVATAR_RIGHTHAND_POS);
    }
}

glm::mat4 MyAvatar::getLeftHandCalibrationMat() const {
    int leftHandIndex = _skeletonModel->getRig().indexOfJoint("LeftHand");
    if (leftHandIndex >= 0) {
        auto leftHandPos = getAbsoluteDefaultJointTranslationInObjectFrame(leftHandIndex);
        auto leftHandRot = getAbsoluteDefaultJointRotationInObjectFrame(leftHandIndex);
        return createMatFromQuatAndPos(leftHandRot, leftHandPos);
    } else {
        return createMatFromQuatAndPos(DEFAULT_AVATAR_LEFTHAND_ROT, DEFAULT_AVATAR_LEFTHAND_POS);
    }
}

bool MyAvatar::pinJoint(int index, const glm::vec3& position, const glm::quat& orientation) {
    auto hipsIndex = getJointIndex("Hips");
    if (index != hipsIndex) {
        qWarning() << "Pinning is only supported for the hips joint at the moment.";
        return false;
    }

    slamPosition(position);
    setOrientation(orientation);

    _skeletonModel->getRig().setMaxHipsOffsetLength(0.05f);

    auto it = std::find(_pinnedJoints.begin(), _pinnedJoints.end(), index);
    if (it == _pinnedJoints.end()) {
        _pinnedJoints.push_back(index);
    }

    return true;
}

bool MyAvatar::clearPinOnJoint(int index) {
    auto it = std::find(_pinnedJoints.begin(), _pinnedJoints.end(), index);
    if (it != _pinnedJoints.end()) {
        _pinnedJoints.erase(it);

        auto hipsIndex = getJointIndex("Hips");
        if (index == hipsIndex) {
            _skeletonModel->getRig().setMaxHipsOffsetLength(FLT_MAX);
        }

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

const MyHead* MyAvatar::getMyHead() const {
    return static_cast<const MyHead*>(getHead());
}

void MyAvatar::setModelScale(float scale) {
    bool changed = (scale != getModelScale());
    Avatar::setModelScale(scale);
    if (changed) {
        float sensorToWorldScale = getEyeHeight() / getUserEyeHeight();
        emit sensorToWorldScaleChanged(sensorToWorldScale);
    }
}
