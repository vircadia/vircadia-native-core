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

#include <QMessageBox>
#include <QBuffer>

#include <glm/gtx/norm.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <QtCore/QTimer>

#include <AccountManager.h>
#include <AddressManager.h>
#include <GeometryUtil.h>
#include <NodeList.h>
#include <PacketHeaders.h>
#include <PerfStat.h>
#include <ShapeCollider.h>
#include <SharedUtil.h>

#include "Application.h"
#include "Audio.h"
#include "Environment.h"
#include "Menu.h"
#include "ModelReferential.h"
#include "MyAvatar.h"
#include "Physics.h"
#include "Recorder.h"
#include "devices/Faceshift.h"
#include "devices/OculusManager.h"
#include "ui/TextRenderer.h"

using namespace std;

const glm::vec3 DEFAULT_UP_DIRECTION(0.0f, 1.0f, 0.0f);
const float YAW_SPEED = 500.0f;   // degrees/sec
const float PITCH_SPEED = 100.0f; // degrees/sec
const float COLLISION_RADIUS_SCALAR = 1.2f; // pertains to avatar-to-avatar collisions
const float COLLISION_RADIUS_SCALE = 0.125f;

const float MIN_KEYBOARD_CONTROL_SPEED = 1.5f;
const float MAX_WALKING_SPEED = 3.0f * MIN_KEYBOARD_CONTROL_SPEED;

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

MyAvatar::MyAvatar() :
	Avatar(),
    _mousePressed(false),
    _bodyPitchDelta(0.0f),
    _bodyRollDelta(0.0f),
    _gravity(0.0f, 0.0f, 0.0f),
    _distanceToNearestAvatar(std::numeric_limits<float>::max()),
    _shouldJump(false),
    _wasPushing(false),
    _isPushing(false),
    _isBraking(false),
    _trapDuration(0.0f),
    _thrust(0.0f),
    _keyboardMotorVelocity(0.0f),
    _keyboardMotorTimescale(DEFAULT_KEYBOARD_MOTOR_TIMESCALE),
    _scriptedMotorVelocity(0.0f),
    _scriptedMotorTimescale(DEFAULT_SCRIPTED_MOTOR_TIMESCALE),
    _scriptedMotorFrame(SCRIPTED_MOTOR_CAMERA_FRAME),
    _motionBehaviors(AVATAR_MOTION_DEFAULTS),
    _lookAtTargetAvatar(),
    _shouldRender(true),
    _billboardValid(false),
    _physicsSimulation(),
    _voxelShapeManager()
{
    ShapeCollider::initDispatchTable();
    for (int i = 0; i < MAX_DRIVE_KEYS; i++) {
        _driveKeys[i] = 0.0f;
    }
    _physicsSimulation.setEntity(&_skeletonModel);
    _physicsSimulation.addEntity(&_voxelShapeManager);

    _skeletonModel.setEnableShapes(true);
    _skeletonModel.buildRagdoll();
    
    // connect to AddressManager signal for location jumps
    connect(&AddressManager::getInstance(), &AddressManager::locationChangeRequired, this, &MyAvatar::goToLocation);
}

MyAvatar::~MyAvatar() {
    _physicsSimulation.clear();
    _lookAtTargetAvatar.clear();
}

void MyAvatar::reset() {
    _skeletonModel.reset();
    getHead()->reset();

    _velocity = glm::vec3(0.0f);
    setThrust(glm::vec3(0.0f));
    //  Reset the pitch and roll components of the avatar's orientation, preserve yaw direction
    glm::vec3 eulers = safeEulerAngles(getOrientation());
    eulers.x = 0.f;
    eulers.z = 0.f;
    setOrientation(glm::quat(eulers));
}

void MyAvatar::update(float deltaTime) {
    if (_referential) {
        _referential->update();
    }
    
    Head* head = getHead();
    head->relaxLean(deltaTime);
    updateFromTrackers(deltaTime);
    if (Menu::getInstance()->isOptionChecked(MenuOption::MoveWithLean)) {
        // Faceshift drive is enabled, set the avatar drive based on the head position
        moveWithLean();
    }
    
    //  Get audio loudness data from audio input device
    Audio* audio = Application::getInstance()->getAudio();
    head->setAudioLoudness(audio->getLastInputLoudness());
    head->setAudioAverageLoudness(audio->getAudioAverageInputLoudness());

    if (_motionBehaviors & AVATAR_MOTION_OBEY_ENVIRONMENTAL_GRAVITY) {
        setGravity(Application::getInstance()->getEnvironment()->getGravity(getPosition()));
    }

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
        Application::getInstance()->getCamera()->setScale(scale);
    }
    _skeletonModel.setShowTrueJointTransforms(! Menu::getInstance()->isOptionChecked(MenuOption::CollideAsRagdoll));

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
    {
        PerformanceTimer perfTimer("attachments");
        simulateAttachments(deltaTime);
    }

    {
        PerformanceTimer perfTimer("joints");
        // copy out the skeleton joints from the model
        _jointData.resize(_skeletonModel.getJointStateCount());
        if (Menu::getInstance()->isOptionChecked(MenuOption::CollideAsRagdoll)) {
            for (int i = 0; i < _jointData.size(); i++) {
                JointData& data = _jointData[i];
                data.valid = _skeletonModel.getVisibleJointState(i, data.rotation);
            }
        } else {
            for (int i = 0; i < _jointData.size(); i++) {
                JointData& data = _jointData[i];
                data.valid = _skeletonModel.getJointState(i, data.rotation);
            }
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
    
    {
        PerformanceTimer perfTimer("hair");
        if (Menu::getInstance()->isOptionChecked(MenuOption::StringHair)) {
            _hair.setAcceleration(getAcceleration() * getHead()->getFinalOrientationInWorldFrame());
            _hair.setAngularVelocity((getAngularVelocity() + getHead()->getAngularVelocity()) * getHead()->getFinalOrientationInWorldFrame());
            _hair.setAngularAcceleration(getAngularAcceleration() * getHead()->getFinalOrientationInWorldFrame());
            _hair.setGravity(Application::getInstance()->getEnvironment()->getGravity(getPosition()) * getHead()->getFinalOrientationInWorldFrame());
            _hair.simulate(deltaTime);
        }
    }

    {
        PerformanceTimer perfTimer("physics");
        const float minError = 0.00001f;
        const float maxIterations = 3;
        const quint64 maxUsec = 4000;
        _physicsSimulation.setTranslation(_position);
        _physicsSimulation.stepForward(deltaTime, minError, maxIterations, maxUsec);

        Ragdoll* ragdoll = _skeletonModel.getRagdoll();
        if (ragdoll && Menu::getInstance()->isOptionChecked(MenuOption::CollideAsRagdoll)) {
            // harvest any displacement of the Ragdoll that is a result of collisions
            glm::vec3 ragdollDisplacement = ragdoll->getAndClearAccumulatedMovement();
            const float MAX_RAGDOLL_DISPLACEMENT_2 = 1.0f;
            float length2 = glm::length2(ragdollDisplacement);
            if (length2 > EPSILON && length2 < MAX_RAGDOLL_DISPLACEMENT_2) {
                applyPositionDelta(ragdollDisplacement);
            }
        } else {
            _skeletonModel.moveShapesTowardJoints(1.0f);
        }
    }

    // now that we're done stepping the avatar forward in time, compute new collisions
    if (_collisionGroups != 0) {
        PerformanceTimer perfTimer("collisions");
        Camera* myCamera = Application::getInstance()->getCamera();

        float radius = getSkeletonHeight() * COLLISION_RADIUS_SCALE;
        if (myCamera->getMode() == CAMERA_MODE_FIRST_PERSON && !OculusManager::isConnected()) {
            radius = myCamera->getAspectRatio() * (myCamera->getNearClip() / cos(myCamera->getFieldOfView() / 2.0f));
            radius *= COLLISION_RADIUS_SCALAR;
        }
        if (_collisionGroups & COLLISION_GROUP_ENVIRONMENT) {
            PerformanceTimer perfTimer("environment");
            updateCollisionWithEnvironment(deltaTime, radius);
        }
        if (_collisionGroups & COLLISION_GROUP_VOXELS) {
            PerformanceTimer perfTimer("voxels");
            updateCollisionWithVoxels(deltaTime, radius);
        } else {
            _trapDuration = 0.0f;
        }
        if (_collisionGroups & COLLISION_GROUP_AVATARS) {
            PerformanceTimer perfTimer("avatars");
            updateCollisionWithAvatars(deltaTime);
        }
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
    
    if (isPlaying() && !OculusManager::isConnected()) {
        return;
    }
    
    if (Application::getInstance()->getPrioVR()->hasHeadRotation()) {
        estimatedRotation = glm::degrees(safeEulerAngles(Application::getInstance()->getPrioVR()->getHeadRotation()));
        estimatedRotation.x *= -1.0f;
        estimatedRotation.z *= -1.0f;

    } else if (OculusManager::isConnected()) {
        estimatedPosition = OculusManager::getRelativePosition();
        estimatedPosition.x *= -1.0f;
        
        const float OCULUS_LEAN_SCALE = 0.05f;
        estimatedPosition /= OCULUS_LEAN_SCALE;
    } else {
        FaceTracker* tracker = Application::getInstance()->getActiveFaceTracker();
        if (tracker) {
            estimatedPosition = tracker->getHeadTranslation();
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
    if (OculusManager::isConnected() || isPlaying()) {
        head->setDeltaPitch(estimatedRotation.x);
        head->setDeltaYaw(estimatedRotation.y);
    } else {
        float magnifyFieldOfView = Menu::getInstance()->getFieldOfView() / Menu::getInstance()->getRealWorldFieldOfView();
        head->setDeltaPitch(estimatedRotation.x * magnifyFieldOfView);
        head->setDeltaYaw(estimatedRotation.y * magnifyFieldOfView);
    }
    head->setDeltaRoll(estimatedRotation.z);

    // the priovr can give us exact lean
    if (Application::getInstance()->getPrioVR()->isActive()) {
        glm::vec3 eulers = glm::degrees(safeEulerAngles(Application::getInstance()->getPrioVR()->getTorsoRotation()));
        head->setLeanSideways(eulers.z);
        head->setLeanForward(eulers.x);
        return;
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

void MyAvatar::moveWithLean() {
    //  Move with Lean by applying thrust proportional to leaning
    Head* head = getHead();
    glm::quat orientation = head->getCameraOrientation();
    glm::vec3 front = orientation * IDENTITY_FRONT;
    glm::vec3 right = orientation * IDENTITY_RIGHT;
    float leanForward = head->getLeanForward();
    float leanSideways = head->getLeanSideways();

    //  Degrees of 'dead zone' when leaning, and amount of acceleration to apply to lean angle
    const float LEAN_FWD_DEAD_ZONE = 15.0f;
    const float LEAN_SIDEWAYS_DEAD_ZONE = 10.0f;
    const float LEAN_FWD_THRUST_SCALE = 4.0f;
    const float LEAN_SIDEWAYS_THRUST_SCALE = 3.0f;

    if (fabs(leanForward) > LEAN_FWD_DEAD_ZONE) {
        if (leanForward > 0.0f) {
            addThrust(front * -(leanForward - LEAN_FWD_DEAD_ZONE) * LEAN_FWD_THRUST_SCALE);
        } else {
            addThrust(front * -(leanForward + LEAN_FWD_DEAD_ZONE) * LEAN_FWD_THRUST_SCALE);
        }
    }
    if (fabs(leanSideways) > LEAN_SIDEWAYS_DEAD_ZONE) {
        if (leanSideways > 0.0f) {
            addThrust(right * -(leanSideways - LEAN_SIDEWAYS_DEAD_ZONE) * LEAN_SIDEWAYS_THRUST_SCALE);
        } else {
            addThrust(right * -(leanSideways + LEAN_SIDEWAYS_DEAD_ZONE) * LEAN_SIDEWAYS_THRUST_SCALE);
        }
    }
}

void MyAvatar::renderDebugBodyPoints() {
    glm::vec3 torsoPosition(getPosition());
    glm::vec3 headPosition(getHead()->getEyePosition());
    float torsoToHead = glm::length(headPosition - torsoPosition);
    glm::vec3 position;
    qDebug("head-above-torso %.2f, scale = %0.2f", torsoToHead, getScale());

    //  Torso Sphere
    position = torsoPosition;
    glPushMatrix();
    glColor4f(0, 1, 0, .5f);
    glTranslatef(position.x, position.y, position.z);
    glutSolidSphere(0.2, 10, 10);
    glPopMatrix();

    //  Head Sphere
    position = headPosition;
    glPushMatrix();
    glColor4f(0, 1, 0, .5f);
    glTranslatef(position.x, position.y, position.z);
    glutSolidSphere(0.15, 10, 10);
    glPopMatrix();
}

// virtual
void MyAvatar::render(const glm::vec3& cameraPosition, RenderMode renderMode) {
    // don't render if we've been asked to disable local rendering
    if (!_shouldRender) {
        return; // exit early
    }

    Avatar::render(cameraPosition, renderMode);
    
    // don't display IK constraints in shadow mode
    if (Menu::getInstance()->isOptionChecked(MenuOption::ShowIKConstraints) && renderMode != SHADOW_RENDER_MODE) {
        _skeletonModel.renderIKConstraints();
    }
}

void MyAvatar::renderHeadMouse(int screenWidth, int screenHeight) const {
    
    Faceshift* faceshift = Application::getInstance()->getFaceshift();
    
    float pixelsPerDegree = screenHeight / Menu::getInstance()->getFieldOfView();
    
    //  Display small target box at center or head mouse target that can also be used to measure LOD
    float headPitch = getHead()->getFinalPitch();
    float headYaw = getHead()->getFinalYaw();

    float aspectRatio = (float) screenWidth / (float) screenHeight;
    int headMouseX = (int)((float)screenWidth / 2.0f - headYaw * aspectRatio * pixelsPerDegree);
    int headMouseY = (int)((float)screenHeight / 2.0f - headPitch * pixelsPerDegree);
    
    glColor3f(1.0f, 1.0f, 1.0f);
    glDisable(GL_LINE_SMOOTH);
    const int PIXEL_BOX = 16;
    glBegin(GL_LINES);
    glVertex2f(headMouseX - PIXEL_BOX/2, headMouseY);
    glVertex2f(headMouseX + PIXEL_BOX/2, headMouseY);
    glVertex2f(headMouseX, headMouseY - PIXEL_BOX/2);
    glVertex2f(headMouseX, headMouseY + PIXEL_BOX/2);
    glEnd();
    glEnable(GL_LINE_SMOOTH);
    //  If Faceshift is active, show eye pitch and yaw as separate pointer
    if (faceshift->isActive()) {

        float avgEyePitch = faceshift->getEstimatedEyePitch();
        float avgEyeYaw = faceshift->getEstimatedEyeYaw();
        int eyeTargetX = (int)((float)(screenWidth) / 2.0f - avgEyeYaw * aspectRatio * pixelsPerDegree);
        int eyeTargetY = (int)((float)(screenHeight) / 2.0f - avgEyePitch * pixelsPerDegree);
        
        glColor3f(0.0f, 1.0f, 1.0f);
        glDisable(GL_LINE_SMOOTH);
        glBegin(GL_LINES);
        glVertex2f(eyeTargetX - PIXEL_BOX/2, eyeTargetY);
        glVertex2f(eyeTargetX + PIXEL_BOX/2, eyeTargetY);
        glVertex2f(eyeTargetX, eyeTargetY - PIXEL_BOX/2);
        glVertex2f(eyeTargetX, eyeTargetY + PIXEL_BOX/2);
        glEnd();

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

glm::vec3 MyAvatar::getRightPalmPosition() {
    glm::vec3 rightHandPosition;
    getSkeletonModel().getRightHandPosition(rightHandPosition);
    glm::quat rightRotation;
    getSkeletonModel().getJointRotationInWorldFrame(getSkeletonModel().getRightHandJointIndex(), rightRotation);
    rightHandPosition += HAND_TO_PALM_OFFSET * glm::inverse(rightRotation);
    return rightHandPosition;
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
        _recorder = RecorderPointer(new Recorder(this));
    }
    Application::getInstance()->getAudio()->setRecorder(_recorder);
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
        _recorder->stopRecording();
    }
}

void MyAvatar::saveRecording(QString filename) {
    if (!_recorder) {
        qDebug() << "There is no recording to save";
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
        qDebug() << "There is no recording to load";
        return;
    }
    if (!_player) {
        _player = PlayerPointer(new Player(this));
    }
    
    _player->loadRecording(_recorder->getRecording());
}

void MyAvatar::setLocalGravity(glm::vec3 gravity) {
    _motionBehaviors |= AVATAR_MOTION_OBEY_LOCAL_GRAVITY;
    // Environmental and Local gravities are incompatible.  Since Local is being set here
    // the environmental setting must be removed.
    _motionBehaviors &= ~AVATAR_MOTION_OBEY_ENVIRONMENTAL_GRAVITY;
    setGravity(gravity);
}

void MyAvatar::setGravity(const glm::vec3& gravity) {
    _gravity = gravity;

    // use the gravity to determine the new world up direction, if possible
    float gravityLength = glm::length(gravity);
    if (gravityLength > EPSILON) {
        _worldUpDirection = _gravity / -gravityLength;
    }
    // NOTE: the else case here it to leave _worldUpDirection unchanged
    // so it continues to point opposite to the previous gravity setting.
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

void MyAvatar::saveData(QSettings* settings) {
    settings->beginGroup("Avatar");

    settings->setValue("bodyYaw", _bodyYaw);
    settings->setValue("bodyPitch", _bodyPitch);
    settings->setValue("bodyRoll", _bodyRoll);

    settings->setValue("headPitch", getHead()->getBasePitch());

    settings->setValue("position_x", _position.x);
    settings->setValue("position_y", _position.y);
    settings->setValue("position_z", _position.z);

    settings->setValue("pupilDilation", getHead()->getPupilDilation());

    settings->setValue("leanScale", _leanScale);
    settings->setValue("scale", _targetScale);
    
    settings->setValue("faceModelURL", _faceModelURL);
    settings->setValue("skeletonModelURL", _skeletonModelURL);
    
    settings->beginWriteArray("attachmentData");
    for (int i = 0; i < _attachmentData.size(); i++) {
        settings->setArrayIndex(i);
        const AttachmentData& attachment = _attachmentData.at(i);
        settings->setValue("modelURL", attachment.modelURL);
        settings->setValue("jointName", attachment.jointName);
        settings->setValue("translation_x", attachment.translation.x);
        settings->setValue("translation_y", attachment.translation.y);
        settings->setValue("translation_z", attachment.translation.z);
        glm::vec3 eulers = safeEulerAngles(attachment.rotation);
        settings->setValue("rotation_x", eulers.x);
        settings->setValue("rotation_y", eulers.y);
        settings->setValue("rotation_z", eulers.z);
        settings->setValue("scale", attachment.scale);
    }
    settings->endArray();
    
    settings->beginWriteArray("animationHandles");
    for (int i = 0; i < _animationHandles.size(); i++) {
        settings->setArrayIndex(i);
        const AnimationHandlePointer& pointer = _animationHandles.at(i);
        settings->setValue("role", pointer->getRole());
        settings->setValue("url", pointer->getURL());
        settings->setValue("fps", pointer->getFPS());
        settings->setValue("priority", pointer->getPriority());
        settings->setValue("loop", pointer->getLoop());
        settings->setValue("hold", pointer->getHold());
        settings->setValue("startAutomatically", pointer->getStartAutomatically());
        settings->setValue("firstFrame", pointer->getFirstFrame());
        settings->setValue("lastFrame", pointer->getLastFrame());
        settings->setValue("maskedJoints", pointer->getMaskedJoints());
    }
    settings->endArray();
    
    settings->setValue("displayName", _displayName);

    settings->endGroup();
}

void MyAvatar::loadData(QSettings* settings) {
    settings->beginGroup("Avatar");

    // in case settings is corrupt or missing loadSetting() will check for NaN
    _bodyYaw = loadSetting(settings, "bodyYaw", 0.0f);
    _bodyPitch = loadSetting(settings, "bodyPitch", 0.0f);
    _bodyRoll = loadSetting(settings, "bodyRoll", 0.0f);

    getHead()->setBasePitch(loadSetting(settings, "headPitch", 0.0f));

    glm::vec3 newPosition;
    newPosition.x = loadSetting(settings, "position_x", START_LOCATION.x);
    newPosition.y = loadSetting(settings, "position_y", START_LOCATION.y);
    newPosition.z = loadSetting(settings, "position_z", START_LOCATION.z);
    slamPosition(newPosition);

    getHead()->setPupilDilation(loadSetting(settings, "pupilDilation", 0.0f));

    _leanScale = loadSetting(settings, "leanScale", 0.05f);
    _targetScale = loadSetting(settings, "scale", 1.0f);
    setScale(_scale);
    Application::getInstance()->getCamera()->setScale(_scale);
    
    setFaceModelURL(settings->value("faceModelURL", DEFAULT_HEAD_MODEL_URL).toUrl());
    setSkeletonModelURL(settings->value("skeletonModelURL").toUrl());
    
    QVector<AttachmentData> attachmentData;
    int attachmentCount = settings->beginReadArray("attachmentData");
    for (int i = 0; i < attachmentCount; i++) {
        settings->setArrayIndex(i);
        AttachmentData attachment;
        attachment.modelURL = settings->value("modelURL").toUrl();
        attachment.jointName = settings->value("jointName").toString();
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
    settings->endArray();
    setAttachmentData(attachmentData);
    
    int animationCount = settings->beginReadArray("animationHandles");
    while (_animationHandles.size() > animationCount) {
        _animationHandles.takeLast()->stop();
    }
    while (_animationHandles.size() < animationCount) {
        addAnimationHandle();
    }
    for (int i = 0; i < animationCount; i++) {
        settings->setArrayIndex(i);
        const AnimationHandlePointer& handle = _animationHandles.at(i);
        handle->setRole(settings->value("role", "idle").toString());
        handle->setURL(settings->value("url").toUrl());
        handle->setFPS(loadSetting(settings, "fps", 30.0f));
        handle->setPriority(loadSetting(settings, "priority", 1.0f));
        handle->setLoop(settings->value("loop", true).toBool());
        handle->setHold(settings->value("hold", false).toBool());
        handle->setStartAutomatically(settings->value("startAutomatically", true).toBool());
        handle->setFirstFrame(settings->value("firstFrame", 0.0f).toFloat());
        handle->setLastFrame(settings->value("lastFrame", INT_MAX).toFloat());
        handle->setMaskedJoints(settings->value("maskedJoints").toStringList());
    }
    settings->endArray();
    
    setDisplayName(settings->value("displayName").toString());

    settings->endGroup();
}

void MyAvatar::saveAttachmentData(const AttachmentData& attachment) const {
    QSettings* settings = Application::getInstance()->lockSettings();
    settings->beginGroup("savedAttachmentData");
    settings->beginGroup(_skeletonModel.getURL().toString());
    settings->beginGroup(attachment.modelURL.toString());
    settings->setValue("jointName", attachment.jointName);
    
    settings->beginGroup(attachment.jointName);
    settings->setValue("translation_x", attachment.translation.x);
    settings->setValue("translation_y", attachment.translation.y);
    settings->setValue("translation_z", attachment.translation.z);
    glm::vec3 eulers = safeEulerAngles(attachment.rotation);
    settings->setValue("rotation_x", eulers.x);
    settings->setValue("rotation_y", eulers.y);
    settings->setValue("rotation_z", eulers.z);
    settings->setValue("scale", attachment.scale);
    
    settings->endGroup();
    settings->endGroup();
    settings->endGroup();
    settings->endGroup();
    Application::getInstance()->unlockSettings();
}

AttachmentData MyAvatar::loadAttachmentData(const QUrl& modelURL, const QString& jointName) const {
    QSettings* settings = Application::getInstance()->lockSettings();
    settings->beginGroup("savedAttachmentData");
    settings->beginGroup(_skeletonModel.getURL().toString());
    settings->beginGroup(modelURL.toString());
    
    AttachmentData attachment;
    attachment.modelURL = modelURL;
    if (jointName.isEmpty()) {
        attachment.jointName = settings->value("jointName").toString();
    } else {
        attachment.jointName = jointName;
    }
    settings->beginGroup(attachment.jointName);
    if (settings->contains("translation_x")) {
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
    
    settings->endGroup();
    settings->endGroup();
    settings->endGroup();
    settings->endGroup();
    Application::getInstance()->unlockSettings();
    
    return attachment;
}

int MyAvatar::parseDataAtOffset(const QByteArray& packet, int offset) {
    qDebug() << "Error: ignoring update packet for MyAvatar"
        << " packetLength = " << packet.size() 
        << "  offset = " << offset;
    // this packet is just bad, so we pretend that we unpacked it ALL
    return packet.size() - offset;
}

void MyAvatar::sendKillAvatar() {
    QByteArray killPacket = byteArrayWithPopulatedHeader(PacketTypeKillAvatar);
    NodeList::getInstance()->broadcastToNodes(killPacket, NodeSet() << NodeType::AvatarMixer);
}

void MyAvatar::updateLookAtTargetAvatar() {
    //
    //  Look at the avatar whose eyes are closest to the ray in direction of my avatar's head
    //
    _lookAtTargetAvatar.clear();
    _targetAvatarPosition = glm::vec3(0.0f);
    const float MIN_LOOKAT_ANGLE = PI / 4.0f;        //  Smallest angle between face and person where we will look at someone
    float smallestAngleTo = MIN_LOOKAT_ANGLE;
    foreach (const AvatarSharedPointer& avatarPointer, Application::getInstance()->getAvatarManager().getAvatarHash()) {
        Avatar* avatar = static_cast<Avatar*>(avatarPointer.data());
        avatar->setIsLookAtTarget(false);
        if (!avatar->isMyAvatar()) {
            glm::vec3 DEFAULT_GAZE_IN_HEAD_FRAME = glm::vec3(0.0f, 0.0f, -1.0f);
            float angleTo = glm::angle(getHead()->getFinalOrientationInWorldFrame() * DEFAULT_GAZE_IN_HEAD_FRAME,
                                       glm::normalize(avatar->getHead()->getEyePosition() - getHead()->getEyePosition()));
            if (angleTo < smallestAngleTo) {
                _lookAtTargetAvatar = avatarPointer;
                _targetAvatarPosition = avatarPointer->getPosition();
                smallestAngleTo = angleTo;
            }
        }
    }
    if (_lookAtTargetAvatar) {
        static_cast<Avatar*>(_lookAtTargetAvatar.data())->setIsLookAtTarget(true);
    }
}

void MyAvatar::clearLookAtTargetAvatar() {
    _lookAtTargetAvatar.clear();
}

glm::vec3 MyAvatar::getUprightHeadPosition() const {
    return _position + getWorldAlignedOrientation() * glm::vec3(0.0f, getPelvisToHeadLength(), 0.0f);
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

void MyAvatar::setFaceModelURL(const QUrl& faceModelURL) {
    Avatar::setFaceModelURL(faceModelURL);
    _billboardValid = false;
}

void MyAvatar::setSkeletonModelURL(const QUrl& skeletonModelURL) {
    Avatar::setSkeletonModelURL(skeletonModelURL);
    _billboardValid = false;
}

void MyAvatar::setAttachmentData(const QVector<AttachmentData>& attachmentData) {
    Avatar::setAttachmentData(attachmentData);
    if (QThread::currentThread() != thread()) {    
        return;
    }
    _billboardValid = false;
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

void MyAvatar::renderBody(RenderMode renderMode, float glowLevel) {
    if (!(_skeletonModel.isRenderable() && getHead()->getFaceModel().isRenderable())) {
        return; // wait until both models are loaded
    }
    
    //  Render the body's voxels and head
    Model::RenderMode modelRenderMode = (renderMode == SHADOW_RENDER_MODE) ?
        Model::SHADOW_RENDER_MODE : Model::DEFAULT_RENDER_MODE;
    _skeletonModel.render(1.0f, modelRenderMode);
    renderAttachments(renderMode);
    
    //  Render head so long as the camera isn't inside it
    const Camera *camera = Application::getInstance()->getCamera();
    const glm::vec3 cameraPos = camera->getPosition() + (camera->getRotation() * glm::vec3(0.0f, 0.0f, 1.0f)) * camera->getDistance();
    if (shouldRenderHead(cameraPos, renderMode)) {
        getHead()->render(1.0f, modelRenderMode);
        
        if (Menu::getInstance()->isOptionChecked(MenuOption::StringHair)) {
            // Render Hair
            glPushMatrix();
            glm::vec3 headPosition = getHead()->getPosition();
            glTranslatef(headPosition.x, headPosition.y, headPosition.z);
            const glm::quat& rotation = getHead()->getFinalOrientationInWorldFrame();
            glm::vec3 axis = glm::axis(rotation);
            glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
            _hair.render();
            glPopMatrix();
        }
    }
    getHand()->render(true, modelRenderMode);
}

const float RENDER_HEAD_CUTOFF_DISTANCE = 0.50f;

bool MyAvatar::shouldRenderHead(const glm::vec3& cameraPosition, RenderMode renderMode) const {
    const Head* head = getHead();
    return (renderMode != NORMAL_RENDER_MODE) || 
        (glm::length(cameraPosition - head->getEyePosition()) > RENDER_HEAD_CUTOFF_DISTANCE * _scale);
}

void MyAvatar::updateOrientation(float deltaTime) {
    //  Gather rotation information from keyboard
    _bodyYawDelta -= _driveKeys[ROT_RIGHT] * YAW_SPEED * deltaTime;
    _bodyYawDelta += _driveKeys[ROT_LEFT] * YAW_SPEED * deltaTime;
    getHead()->setBasePitch(getHead()->getBasePitch() + (_driveKeys[ROT_UP] - _driveKeys[ROT_DOWN]) * PITCH_SPEED * deltaTime);

    // update body yaw by body yaw delta
    glm::quat orientation = getOrientation() * glm::quat(glm::radians(
                glm::vec3(_bodyPitchDelta, _bodyYawDelta, _bodyRollDelta) * deltaTime));

    // decay body rotation momentum
    const float BODY_SPIN_FRICTION = 7.5f;
    float bodySpinMomentum = 1.0f - BODY_SPIN_FRICTION * deltaTime;
    if (bodySpinMomentum < 0.0f) { bodySpinMomentum = 0.0f; }
    _bodyPitchDelta *= bodySpinMomentum;
    _bodyYawDelta *= bodySpinMomentum;
    _bodyRollDelta *= bodySpinMomentum;

    float MINIMUM_ROTATION_RATE = 2.0f;
    if (fabs(_bodyYawDelta) < MINIMUM_ROTATION_RATE) { _bodyYawDelta = 0.0f; }
    if (fabs(_bodyRollDelta) < MINIMUM_ROTATION_RATE) { _bodyRollDelta = 0.0f; }
    if (fabs(_bodyPitchDelta) < MINIMUM_ROTATION_RATE) { _bodyPitchDelta = 0.0f; }

    if (OculusManager::isConnected()) {
        // these angles will be in radians
        float yaw, pitch, roll; 
        OculusManager::getEulerAngles(yaw, pitch, roll);
        // ... so they need to be converted to degrees before we do math...
        yaw *= DEGREES_PER_RADIAN;
        pitch *= DEGREES_PER_RADIAN;
        roll *= DEGREES_PER_RADIAN;
        
        // Record the angular velocity
        Head* head = getHead();
        glm::vec3 angularVelocity(yaw - head->getBaseYaw(), pitch - head->getBasePitch(), roll - head->getBaseRoll());
        head->setAngularVelocity(angularVelocity);
        
        //Invert yaw and roll when in mirror mode
        if (Application::getInstance()->getCamera()->getMode() == CAMERA_MODE_MIRROR) {
            head->setBaseYaw(-yaw);
            head->setBasePitch(pitch);
            head->setBaseRoll(-roll);
        } else {
            head->setBaseYaw(yaw);
            head->setBasePitch(pitch);
            head->setBaseRoll(roll);
        }
        
    }

    // update the euler angles
    setOrientation(orientation);
}

glm::vec3 MyAvatar::applyKeyboardMotor(float deltaTime, const glm::vec3& localVelocity, bool hasFloor) {
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
        // we don't want to break if anything is pushing the avatar around
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

            // Compute the target keyboard velocity (which ramps up slowly, and damps very quickly)
            // the max magnitude of which depends on what we're doing:
            float finalMaxMotorSpeed = hasFloor ? _scale * MAX_WALKING_SPEED : _scale * MAX_KEYBOARD_MOTOR_SPEED;
            float motorLength = glm::length(_keyboardMotorVelocity);
            if (motorLength < _scale * MIN_KEYBOARD_CONTROL_SPEED) {
                // an active keyboard motor should never be slower than this
                _keyboardMotorVelocity = _scale * MIN_KEYBOARD_CONTROL_SPEED * direction;
                motorEfficiency = 1.0f;
            } else {
                float KEYBOARD_MOTOR_LENGTH_TIMESCALE = 2.0f;
                float INCREASE_FACTOR = 1.8f;
                motorLength *= 1.0f + glm::clamp(deltaTime / KEYBOARD_MOTOR_LENGTH_TIMESCALE, 0.0f, 1.0f) * INCREASE_FACTOR;
                if (motorLength > finalMaxMotorSpeed) {
                    motorLength = finalMaxMotorSpeed;
                }
                _keyboardMotorVelocity = motorLength * direction;
            }
            _isPushing = true;
        } 
    } else {
        _keyboardMotorVelocity = glm::vec3(0.0f);
    }

    // apply keyboard motor
    return localVelocity + motorEfficiency * (_keyboardMotorVelocity - localVelocity);
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

const float NEARBY_FLOOR_THRESHOLD = 5.0f;

void MyAvatar::updatePosition(float deltaTime) {

    // check for floor by casting a ray straight down from avatar's position
    float heightAboveFloor = FLT_MAX;
    bool hasFloor = false;
    const CapsuleShape& boundingShape = _skeletonModel.getBoundingShape();
    const float maxFloorDistance = boundingShape.getBoundingRadius() * NEARBY_FLOOR_THRESHOLD;

    RayIntersectionInfo intersection;
    // NOTE: avatar is center of PhysicsSimulation, so rayStart is the origin for the purposes of the raycast
    intersection._rayStart = glm::vec3(0.0f);
    intersection._rayDirection = - _worldUpDirection;
    intersection._rayLength = 4.0f * boundingShape.getBoundingRadius();
    if (_physicsSimulation.findFloorRayIntersection(intersection)) {
        // NOTE: heightAboveFloor is the distance between the bottom of the avatar and the floor
        heightAboveFloor = intersection._hitDistance - boundingShape.getBoundingRadius()
            + _skeletonModel.getBoundingShapeOffset().y;
        if (heightAboveFloor < maxFloorDistance) {
            hasFloor = true;
        }
    }

    // velocity is initialized to the measured _velocity but will be modified by friction, external thrust, etc
    glm::vec3 velocity = _velocity;

    bool pushingUp = (_driveKeys[UP] - _driveKeys[DOWN] > 0.0f) || _scriptedMotorVelocity.y > 0.0f;
    if (_motionBehaviors & AVATAR_MOTION_STAND_ON_NEARBY_FLOORS) {
        const float MAX_SPEED_UNDER_GRAVITY = 2.0f * _scale * MAX_WALKING_SPEED;
        if (pushingUp || glm::length2(velocity) > MAX_SPEED_UNDER_GRAVITY * MAX_SPEED_UNDER_GRAVITY) {
            // we're pushing up or moving quickly, so disable gravity
            setLocalGravity(glm::vec3(0.0f));
            hasFloor = false;
        } else {
            if (heightAboveFloor > maxFloorDistance) {
                // disable local gravity when floor is too far away
                setLocalGravity(glm::vec3(0.0f));
                hasFloor = false;
            } else {
                // enable gravity
                setLocalGravity(-_worldUpDirection);
            }
        }
    }

    bool zeroDownwardVelocity = false;
    bool gravityEnabled = (glm::length2(_gravity) > EPSILON);
    if (gravityEnabled) {
        const float SLOP = 0.002f;
        if (heightAboveFloor < SLOP) {
            if (heightAboveFloor < 0.0) {
                // Gravity is in effect so we assume that the avatar is colliding against the world and we need 
                // to lift avatar out of floor, but we don't want to do it too fast (keep it smooth).
                float distanceToLift = glm::min(-heightAboveFloor, MAX_WALKING_SPEED * deltaTime);
    
                // We don't use applyPositionDelta() for this lift distance because we don't want the avatar 
                // to come flying out of the floor.  Instead we update position directly, and set a boolean
                // that will remind us later to zero any downward component of the velocity.
                _position += distanceToLift * _worldUpDirection;
            }
            zeroDownwardVelocity = true;
        }
        if (!zeroDownwardVelocity) {
            velocity += (deltaTime * GRAVITY_EARTH) * _gravity;
        }
    }

    // rotate velocity into camera frame
    glm::quat rotation = getHead()->getCameraOrientation();
    glm::vec3 localVelocity = glm::inverse(rotation) * velocity;

    // apply motors in camera frame
    glm::vec3 newLocalVelocity = applyKeyboardMotor(deltaTime, localVelocity, hasFloor);
    newLocalVelocity = applyScriptedMotor(deltaTime, newLocalVelocity);

    // rotate back into world-frame
    velocity = rotation * newLocalVelocity;

    // apply thrust
    velocity += _thrust * deltaTime;
    _thrust = glm::vec3(0.0f);

    // remove downward velocity so we don't push into floor
    if (zeroDownwardVelocity) {
        float verticalSpeed = glm::dot(velocity, _worldUpDirection);
        if (verticalSpeed < 0.0f || !pushingUp) {
            velocity -= verticalSpeed * _worldUpDirection;
        }
    }

    // cap avatar speed
    float speed = glm::length(velocity);
    if (speed > MAX_AVATAR_SPEED) {
        velocity *= MAX_AVATAR_SPEED / speed;
        speed = MAX_AVATAR_SPEED;
    }

    // update position
    const float MIN_AVATAR_SPEED = 0.075f;
    if (speed > MIN_AVATAR_SPEED) {
        applyPositionDelta(deltaTime * velocity);
    }

    // update _moving flag based on speed
    const float MOVING_SPEED_THRESHOLD = 0.01f;
    _moving = speed > MOVING_SPEED_THRESHOLD;

    updateChatCircle(deltaTime);
    measureMotionDerivatives(deltaTime);
}

void MyAvatar::updateCollisionWithEnvironment(float deltaTime, float radius) {
    glm::vec3 up = getBodyUpDirection();
    const float ENVIRONMENT_SURFACE_ELASTICITY = 0.0f;
    const float ENVIRONMENT_SURFACE_DAMPING = 0.01f;
    const float ENVIRONMENT_COLLISION_FREQUENCY = 0.05f;
    glm::vec3 penetration;
    float pelvisFloatingHeight = getPelvisFloatingHeight();
    if (Application::getInstance()->getEnvironment()->findCapsulePenetration(
            _position - up * (pelvisFloatingHeight - radius),
            _position + up * (getSkeletonHeight() - pelvisFloatingHeight + radius), radius, penetration)) {
        updateCollisionSound(penetration, deltaTime, ENVIRONMENT_COLLISION_FREQUENCY);
        applyHardCollision(penetration, ENVIRONMENT_SURFACE_ELASTICITY, ENVIRONMENT_SURFACE_DAMPING);
    }
}

static CollisionList myCollisions(64);

void MyAvatar::updateCollisionWithVoxels(float deltaTime, float radius) {

    quint64 now = usecTimestampNow();
    if (_voxelShapeManager.needsUpdate(now)) {
        // We use a multiple of the avatar's boundingRadius as the size of the cube of interest.
        float cubeScale = 6.0f * getBoundingRadius();
        glm::vec3 corner = getPosition() - glm::vec3(0.5f * cubeScale);
        AACube boundingCube(corner, cubeScale);

        // query the VoxelTree for cubes that touch avatar's boundingCube
        CubeList cubes;
        if (Application::getInstance()->getVoxelTree()->findContentInCube(boundingCube, cubes)) {
            _voxelShapeManager.updateVoxels(now, cubes);
        }
    }

    // TODO: Andrew to do ground/walking detection in ragdoll mode
    if (!Menu::getInstance()->isOptionChecked(MenuOption::CollideAsRagdoll)) {
        const float MAX_VOXEL_COLLISION_SPEED = 100.0f;
        float speed = glm::length(_velocity);
        if (speed > MAX_VOXEL_COLLISION_SPEED) {
            // don't even bother to try to collide against voxles when moving very fast
            _trapDuration = 0.0f;
            return;
        }
        bool isTrapped = false;
        myCollisions.clear();
        // copy the boundingShape and tranform into physicsSimulation frame
        CapsuleShape boundingShape = _skeletonModel.getBoundingShape();
        boundingShape.setTranslation(boundingShape.getTranslation() - _position);

        if (_physicsSimulation.getShapeCollisions(&boundingShape, myCollisions)) {
            // we temporarily move b
            const float VOXEL_ELASTICITY = 0.0f;
            const float VOXEL_DAMPING = 0.0f;
            const float capsuleRadius = boundingShape.getRadius();
            const float capsuleHalfHeight = boundingShape.getHalfHeight();
            const float MAX_STEP_HEIGHT = capsuleRadius + capsuleHalfHeight;
            const float MIN_STEP_HEIGHT = 0.0f;
            float highestStep = 0.0f;
            float lowestStep = MAX_STEP_HEIGHT;
            glm::vec3 floorPoint;
            glm::vec3 stepPenetration(0.0f);
            glm::vec3 totalPenetration(0.0f);
    
            for (int i = 0; i < myCollisions.size(); ++i) {
                CollisionInfo* collision = myCollisions[i];

                float verticalDepth = glm::dot(collision->_penetration, _worldUpDirection);
                float horizontalDepth = glm::length(collision->_penetration - verticalDepth * _worldUpDirection);
                const float MAX_TRAP_PERIOD = 0.125f;
                if (horizontalDepth > capsuleRadius || fabsf(verticalDepth) > MAX_STEP_HEIGHT) {
                    isTrapped = true;
                    if (_trapDuration > MAX_TRAP_PERIOD) {
                        RayIntersectionInfo intersection;
                        // we pick a rayStart that we expect to be inside the boundingShape (aka shapeA)
                        intersection._rayStart = collision->_contactPoint - MAX_STEP_HEIGHT * glm::normalize(collision->_penetration);
                        intersection._rayDirection = -_worldUpDirection;
                        // cast the ray down against shapeA
                        if (collision->_shapeA->findRayIntersection(intersection)) {
                            float firstDepth = - intersection._hitDistance;
                            // recycle intersection and cast again in up against shapeB
                            intersection._rayDirection = _worldUpDirection;
                            intersection._hitDistance = FLT_MAX;
                            if (collision->_shapeB->findRayIntersection(intersection)) {
                                // now we know how much we need to move UP to get out
                                totalPenetration = addPenetrations(totalPenetration, 
                                        (firstDepth + intersection._hitDistance) * _worldUpDirection);
                            }
                        }
                        continue;
                    }
                } else if (_trapDuration > MAX_TRAP_PERIOD) {
                    // we're trapped, ignore this shallow collision
                    continue;
                }
                totalPenetration = addPenetrations(totalPenetration, collision->_penetration);
                
                // some logic to help us walk up steps
                if (glm::dot(collision->_penetration, _velocity) >= 0.0f) {
                    float stepHeight = - glm::dot(_worldUpDirection, collision->_penetration);
                    if (stepHeight > highestStep) {
                        highestStep = stepHeight;
                        stepPenetration = collision->_penetration;
                    }
                    if (stepHeight < lowestStep) {
                        lowestStep = stepHeight;
                        // remember that collision is in _physicsSimulation frame so we must add _position
                        floorPoint = _position + collision->_contactPoint - collision->_penetration;
                    }
                }
            }
    
            float penetrationLength = glm::length(totalPenetration);
            if (penetrationLength < EPSILON) {
                _trapDuration = 0.0f;
                return;
            }
            float verticalPenetration = glm::dot(totalPenetration, _worldUpDirection);
            if (highestStep > MIN_STEP_HEIGHT && highestStep < MAX_STEP_HEIGHT && verticalPenetration <= 0.0f) {
                // we're colliding against an edge

                // rotate _keyboardMotorVelocity into world frame
                glm::vec3 targetVelocity = _keyboardMotorVelocity;
                glm::quat rotation = getHead()->getCameraOrientation();
                targetVelocity = rotation * _keyboardMotorVelocity;
                if (_wasPushing && glm::dot(targetVelocity, totalPenetration) > EPSILON) {
                    // we're puhing into the edge, so we want to lift
    
                    // remove unhelpful horizontal component of the step's penetration
                    totalPenetration -= stepPenetration - (glm::dot(stepPenetration, _worldUpDirection) * _worldUpDirection);
    
                    // further adjust penetration to help lift
                    float liftSpeed = glm::max(MAX_WALKING_SPEED, speed);
                    float thisStep = glm::min(liftSpeed * deltaTime, highestStep);
                    float extraStep = glm::dot(totalPenetration, _worldUpDirection) + thisStep;
                    if (extraStep > 0.0f) {
                        totalPenetration -= extraStep * _worldUpDirection;
                    }
    
                    _position -= totalPenetration;
                } else {
                    // we're not pushing into the edge, so let the avatar fall
                    applyHardCollision(totalPenetration, VOXEL_ELASTICITY, VOXEL_DAMPING);
                }
            } else {
                applyHardCollision(totalPenetration, VOXEL_ELASTICITY, VOXEL_DAMPING);
            }
    
            // Don't make a collision sound against voxlels by default -- too annoying when walking
            //const float VOXEL_COLLISION_FREQUENCY = 0.5f;
            //updateCollisionSound(myCollisions[0]->_penetration, deltaTime, VOXEL_COLLISION_FREQUENCY);
        } 
        _trapDuration = isTrapped ? _trapDuration + deltaTime : 0.0f;
    }
}

void MyAvatar::applyHardCollision(const glm::vec3& penetration, float elasticity, float damping) {
    //
    //  Update the avatar in response to a hard collision.  Position will be reset exactly
    //  to outside the colliding surface.  Velocity will be modified according to elasticity.
    //
    //  if elasticity = 0.0, collision is 100% inelastic.
    //  if elasticity = 1.0, collision is elastic.
    //
    _position -= penetration;
    static float HALTING_VELOCITY = 0.2f;
    // cancel out the velocity component in the direction of penetration
    float penetrationLength = glm::length(penetration);
    if (penetrationLength > EPSILON) {
        glm::vec3 direction = penetration / penetrationLength;
        _velocity -= glm::dot(_velocity, direction) * direction * (1.0f + elasticity);
        _velocity *= glm::clamp(1.0f - damping, 0.0f, 1.0f);
        if ((glm::length(_velocity) < HALTING_VELOCITY) && (glm::length(_thrust) == 0.0f)) {
            // If moving really slowly after a collision, and not applying forces, stop altogether
            _velocity *= 0.0f;
        }
    }
}

void MyAvatar::updateCollisionSound(const glm::vec3 &penetration, float deltaTime, float frequency) {
    //  consider whether to have the collision make a sound
    const float AUDIBLE_COLLISION_THRESHOLD = 0.02f;
    const float COLLISION_LOUDNESS = 1.0f;
    const float DURATION_SCALING = 0.004f;
    const float NOISE_SCALING = 0.1f;
    glm::vec3 velocity = _velocity;
    glm::vec3 gravity = getGravity();

    if (glm::length(gravity) > EPSILON) {
        //  If gravity is on, remove the effect of gravity on velocity for this
        //  frame, so that we are not constantly colliding with the surface
        velocity -= _scale * glm::length(gravity) * GRAVITY_EARTH * deltaTime * glm::normalize(gravity);
    }
    float velocityTowardCollision = glm::dot(velocity, glm::normalize(penetration));
    float velocityTangentToCollision = glm::length(velocity) - velocityTowardCollision;

    if (velocityTowardCollision > AUDIBLE_COLLISION_THRESHOLD) {
        //  Volume is proportional to collision velocity
        //  Base frequency is modified upward by the angle of the collision
        //  Noise is a function of the angle of collision
        //  Duration of the sound is a function of both base frequency and velocity of impact
        Application::getInstance()->getAudio()->startCollisionSound(
            std::min(COLLISION_LOUDNESS * velocityTowardCollision, 1.0f),
            frequency * (1.0f + velocityTangentToCollision / velocityTowardCollision),
            std::min(velocityTangentToCollision / velocityTowardCollision * NOISE_SCALING, 1.0f),
            1.0f - DURATION_SCALING * powf(frequency, 0.5f) / velocityTowardCollision, false);
    }
}

bool findAvatarAvatarPenetration(const glm::vec3 positionA, float radiusA, float heightA,
        const glm::vec3 positionB, float radiusB, float heightB, glm::vec3& penetration) {
    glm::vec3 positionBA = positionB - positionA;
    float xzDistance = sqrt(positionBA.x * positionBA.x + positionBA.z * positionBA.z);
    if (xzDistance < (radiusA + radiusB)) {
        float yDistance = fabs(positionBA.y);
        float halfHeights = 0.5 * (heightA + heightB);
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

void MyAvatar::updateCollisionWithAvatars(float deltaTime) {
    //  Reset detector for nearest avatar
    _distanceToNearestAvatar = std::numeric_limits<float>::max();
    const AvatarHash& avatars = Application::getInstance()->getAvatarManager().getAvatarHash();
    if (avatars.size() <= 1) {
        // no need to compute a bunch of stuff if we have one or fewer avatars
        return;
    }
    float myBoundingRadius = getBoundingRadius();

    // find nearest avatar
    float nearestDistance2 = std::numeric_limits<float>::max();
    Avatar* nearestAvatar = NULL;
    foreach (const AvatarSharedPointer& avatarPointer, avatars) {
        Avatar* avatar = static_cast<Avatar*>(avatarPointer.data());
        if (static_cast<Avatar*>(this) == avatar) {
            // don't collide with ourselves
            continue;
        }
        float distance2 = glm::distance2(_position, avatar->getPosition());        
        if (nearestDistance2 > distance2) {
            nearestDistance2 = distance2;
            nearestAvatar = avatar;
        }
    }
    _distanceToNearestAvatar = glm::sqrt(nearestDistance2);

    if (Menu::getInstance()->isOptionChecked(MenuOption::CollideAsRagdoll)) {
        if (nearestAvatar != NULL) {
            if (_distanceToNearestAvatar > myBoundingRadius + nearestAvatar->getBoundingRadius()) {
                // they aren't close enough to put into the _physicsSimulation
                // so we clear the pointer
                nearestAvatar = NULL;
            }
        }
    
        foreach (const AvatarSharedPointer& avatarPointer, avatars) {
            Avatar* avatar = static_cast<Avatar*>(avatarPointer.data());
            if (static_cast<Avatar*>(this) == avatar) {
                // don't collide with ourselves
                continue;
            }
            SkeletonModel* skeleton = &(avatar->getSkeletonModel());
            PhysicsSimulation* simulation = skeleton->getSimulation();
            if (avatar == nearestAvatar) {
                if (simulation != &(_physicsSimulation)) {
                    skeleton->setEnableShapes(true);
                    _physicsSimulation.addEntity(skeleton);
                    _physicsSimulation.addRagdoll(skeleton->getRagdoll());
                }
            } else if (simulation == &(_physicsSimulation)) {
                _physicsSimulation.removeRagdoll(skeleton->getRagdoll());
                _physicsSimulation.removeEntity(skeleton);
                skeleton->setEnableShapes(false);
            }
        }
    }
}

class SortedAvatar {
public:
    Avatar* avatar;
    float distance;
    glm::vec3 accumulatedCenter;
};

bool operator<(const SortedAvatar& s1, const SortedAvatar& s2) {
    return s1.distance < s2.distance;
}

void MyAvatar::updateChatCircle(float deltaTime) {
    if (!(_isChatCirclingEnabled = Menu::getInstance()->isOptionChecked(MenuOption::ChatCircling))) {
        return;
    }

    // find all circle-enabled members and sort by distance
    QVector<SortedAvatar> sortedAvatars;
    
    foreach (const AvatarSharedPointer& avatarPointer, Application::getInstance()->getAvatarManager().getAvatarHash()) {
        Avatar* avatar = static_cast<Avatar*>(avatarPointer.data());
        if ( ! avatar->isChatCirclingEnabled() ||
                avatar == static_cast<Avatar*>(this)) {
            continue;
        }
    
        SortedAvatar sortedAvatar;
        sortedAvatar.avatar = avatar;
        sortedAvatar.distance = glm::distance(_position, sortedAvatar.avatar->getPosition());
        sortedAvatars.append(sortedAvatar);
    }
    
    qSort(sortedAvatars.begin(), sortedAvatars.end());

    // compute the accumulated centers
    glm::vec3 center = _position;
    for (int i = 0; i < sortedAvatars.size(); i++) {
        SortedAvatar& sortedAvatar = sortedAvatars[i];
        sortedAvatar.accumulatedCenter = (center += sortedAvatar.avatar->getPosition()) / (i + 2.0f);
    }

    // remove members whose accumulated circles are too far away to influence us
    const float CIRCUMFERENCE_PER_MEMBER = 0.5f;
    const float CIRCLE_INFLUENCE_SCALE = 2.0f;
    const float MIN_RADIUS = 0.3f;
    for (int i = sortedAvatars.size() - 1; i >= 0; i--) {
        float radius = qMax(MIN_RADIUS, (CIRCUMFERENCE_PER_MEMBER * (i + 2)) / TWO_PI);
        if (glm::distance(_position, sortedAvatars[i].accumulatedCenter) > radius * CIRCLE_INFLUENCE_SCALE) {
            sortedAvatars.remove(i);
        } else {
            break;
        }
    }
    if (sortedAvatars.isEmpty()) {
        return;
    }
    center = sortedAvatars.last().accumulatedCenter;
    float radius = qMax(MIN_RADIUS, (CIRCUMFERENCE_PER_MEMBER * (sortedAvatars.size() + 1)) / TWO_PI);

    // compute the average up vector
    glm::vec3 up = getWorldAlignedOrientation() * IDENTITY_UP;
    foreach (const SortedAvatar& sortedAvatar, sortedAvatars) {
        up += sortedAvatar.avatar->getWorldAlignedOrientation() * IDENTITY_UP;
    }
    up = glm::normalize(up);

    // find reasonable corresponding right/front vectors
    glm::vec3 front = glm::cross(up, IDENTITY_RIGHT);
    if (glm::length(front) < EPSILON) {
        front = glm::cross(up, IDENTITY_FRONT);
    }
    front = glm::normalize(front);
    glm::vec3 right = glm::cross(front, up);

    // find our angle and the angular distances to our closest neighbors
    glm::vec3 delta = _position - center;
    glm::vec3 projected = glm::vec3(glm::dot(right, delta), glm::dot(front, delta), 0.0f);
    float myAngle = glm::length(projected) > EPSILON ? atan2f(projected.y, projected.x) : 0.0f;
    float leftDistance = TWO_PI;
    float rightDistance = TWO_PI;
    foreach (const SortedAvatar& sortedAvatar, sortedAvatars) {
        delta = sortedAvatar.avatar->getPosition() - center;
        projected = glm::vec3(glm::dot(right, delta), glm::dot(front, delta), 0.0f);
        float angle = glm::length(projected) > EPSILON ? atan2f(projected.y, projected.x) : 0.0f;
        if (angle < myAngle) {
            leftDistance = min(myAngle - angle, leftDistance);
            rightDistance = min(TWO_PI - (myAngle - angle), rightDistance);

        } else {
            leftDistance = min(TWO_PI - (angle - myAngle), leftDistance);
            rightDistance = min(angle - myAngle, rightDistance);
        }
    }

    // if we're on top of a neighbor, we need to randomize so that they don't both go in the same direction
    if (rightDistance == 0.0f && randomBoolean()) {
        swap(leftDistance, rightDistance);
    }

    // split the difference between our neighbors
    float targetAngle = myAngle + (rightDistance - leftDistance) / 4.0f;
    glm::vec3 targetPosition = center + (front * sinf(targetAngle) + right * cosf(targetAngle)) * radius;

    // approach the target position
    const float APPROACH_RATE = 0.05f;
    _position = glm::mix(_position, targetPosition, APPROACH_RATE);
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
    QImage image = Application::getInstance()->renderAvatarBillboard();
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
        qDebug("Changed scale to %f", _targetScale);
    }
}

void MyAvatar::decreaseSize() {
    if (MIN_AVATAR_SCALE < (1.0f - SCALING_RATIO) * _targetScale) {
        _targetScale *= (1.0f - SCALING_RATIO);
        qDebug("Changed scale to %f", _targetScale);
    }
}

void MyAvatar::resetSize() {
    _targetScale = 1.0f;
    qDebug("Reseted scale to %f", _targetScale);
}

void MyAvatar::goToLocation(const glm::vec3& newPosition,
                            bool hasOrientation, const glm::quat& newOrientation,
                            bool shouldFaceLocation) {
    
    qDebug().nospace() << "MyAvatar goToLocation - moving to " << newPosition.x << ", "
        << newPosition.y << ", " << newPosition.z;
    
    glm::vec3 shiftedPosition = newPosition;
    
    if (hasOrientation) {
        qDebug().nospace() << "MyAvatar goToLocation - new orientation is "
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
    if (menu->isOptionChecked(MenuOption::ObeyEnvironmentalGravity)) {
        _motionBehaviors |= AVATAR_MOTION_OBEY_ENVIRONMENTAL_GRAVITY;
        // Environmental and Local gravities are incompatible.  Environmental setting trumps local.
        _motionBehaviors &= ~AVATAR_MOTION_OBEY_LOCAL_GRAVITY;
    } else {
        _motionBehaviors &= ~AVATAR_MOTION_OBEY_ENVIRONMENTAL_GRAVITY;
    }
    if (! (_motionBehaviors & (AVATAR_MOTION_OBEY_ENVIRONMENTAL_GRAVITY | AVATAR_MOTION_OBEY_LOCAL_GRAVITY))) {
        setGravity(glm::vec3(0.0f));
    }
    if (menu->isOptionChecked(MenuOption::StandOnNearbyFloors)) {
        _motionBehaviors |= AVATAR_MOTION_STAND_ON_NEARBY_FLOORS;
        // standing on floors requires collision with voxels
        _collisionGroups |= COLLISION_GROUP_VOXELS;
        menu->setIsOptionChecked(MenuOption::CollideWithVoxels, true);
    } else {
        _motionBehaviors &= ~AVATAR_MOTION_STAND_ON_NEARBY_FLOORS;
    }
    if (!(_collisionGroups | COLLISION_GROUP_VOXELS)) {
        _voxelShapeManager.clearShapes();
    }
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
}

void MyAvatar::onToggleRagdoll() {
    Ragdoll* ragdoll = _skeletonModel.getRagdoll();
    if (ragdoll) {
        if (Menu::getInstance()->isOptionChecked(MenuOption::CollideAsRagdoll)) {
            _physicsSimulation.setRagdoll(ragdoll);
        } else {
            _physicsSimulation.setRagdoll(NULL);
        }
    }
}

void MyAvatar::renderAttachments(RenderMode renderMode) {
    if (Application::getInstance()->getCamera()->getMode() != CAMERA_MODE_FIRST_PERSON || renderMode == MIRROR_RENDER_MODE) {
        Avatar::renderAttachments(renderMode);
        return;
    }
    const FBXGeometry& geometry = _skeletonModel.getGeometry()->getFBXGeometry();
    QString headJointName = (geometry.headJointIndex == -1) ? QString() : geometry.joints.at(geometry.headJointIndex).name;
    Model::RenderMode modelRenderMode = (renderMode == SHADOW_RENDER_MODE) ?
        Model::SHADOW_RENDER_MODE : Model::DEFAULT_RENDER_MODE;
    for (int i = 0; i < _attachmentData.size(); i++) {
        const QString& jointName = _attachmentData.at(i).jointName;
        if (jointName != headJointName && jointName != "Head") {
            _attachmentModels.at(i)->render(1.0f, modelRenderMode);        
        }
    }
}

void MyAvatar::setCollisionGroups(quint32 collisionGroups) {
    Avatar::setCollisionGroups(collisionGroups & VALID_COLLISION_GROUPS);
    Menu* menu = Menu::getInstance();
    menu->setIsOptionChecked(MenuOption::CollideWithEnvironment, (bool)(_collisionGroups & COLLISION_GROUP_ENVIRONMENT));
    menu->setIsOptionChecked(MenuOption::CollideWithAvatars, (bool)(_collisionGroups & COLLISION_GROUP_AVATARS));
    menu->setIsOptionChecked(MenuOption::CollideWithVoxels, (bool)(_collisionGroups & COLLISION_GROUP_VOXELS));
    menu->setIsOptionChecked(MenuOption::CollideWithParticles, (bool)(_collisionGroups & COLLISION_GROUP_PARTICLES));
    if (! (_collisionGroups & COLLISION_GROUP_VOXELS)) {
        // no collision with voxels --> disable standing on floors
        _motionBehaviors &= ~AVATAR_MOTION_STAND_ON_NEARBY_FLOORS;
        menu->setIsOptionChecked(MenuOption::StandOnNearbyFloors, false);
    }
}

void MyAvatar::applyCollision(const glm::vec3& contactPoint, const glm::vec3& penetration) {
    glm::vec3 leverAxis = contactPoint - getPosition();
    float leverLength = glm::length(leverAxis);
    if (leverLength > EPSILON) {
        // compute lean perturbation angles
        glm::quat bodyRotation = getOrientation();
        glm::vec3 xAxis = bodyRotation * glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 zAxis = bodyRotation * glm::vec3(0.0f, 0.0f, 1.0f);

        leverAxis = leverAxis / leverLength;
        glm::vec3 effectivePenetration = penetration - glm::dot(penetration, leverAxis) * leverAxis;
        // use the small-angle approximation for sine
        float sideways = - glm::dot(effectivePenetration, xAxis) / leverLength;
        float forward = glm::dot(effectivePenetration, zAxis) / leverLength;
        getHead()->addLeanDeltas(sideways, forward);
    }
}

//Renders sixense laser pointers for UI selection with controllers
void MyAvatar::renderLaserPointers() {
    const float PALM_TIP_ROD_RADIUS = 0.002f;

    //If the Oculus is enabled, we will draw a blue cursor ray

    for (size_t i = 0; i < getHand()->getNumPalms(); ++i) {
        PalmData& palm = getHand()->getPalms()[i];
        if (palm.isActive()) {
            glColor4f(0, 1, 1, 1);
            glm::vec3 tip = getLaserPointerTipPosition(&palm);
            glm::vec3 root = palm.getPosition();

            //Scale the root vector with the avatar scale
            scaleVectorRelativeToPosition(root);

            Avatar::renderJointConnectingCone(root, tip, PALM_TIP_ROD_RADIUS, PALM_TIP_ROD_RADIUS);
        }
    }
}

//Gets the tip position for the laser pointer
glm::vec3 MyAvatar::getLaserPointerTipPosition(const PalmData* palm) {
    const ApplicationOverlay& applicationOverlay = Application::getInstance()->getApplicationOverlay();
    glm::vec3 direction = glm::normalize(palm->getTipPosition() - palm->getPosition());

    glm::vec3 position = palm->getPosition();
    //scale the position with the avatar
    scaleVectorRelativeToPosition(position);


    glm::vec3 result;
    if (applicationOverlay.calculateRayUICollisionPoint(position, direction, result)) {
        return result;
    }

    return palm->getPosition();
}

void MyAvatar::clearDriveKeys() {
    for (int i = 0; i < MAX_DRIVE_KEYS; ++i) {
        _driveKeys[i] = 0.0f;
    }
}
