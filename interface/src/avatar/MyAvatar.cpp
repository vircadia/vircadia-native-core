//
//  MyAvatar.cpp
//  interface
//
//  Created by Mark Peng on 8/16/13.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include <algorithm>
#include <vector>

#include <QBuffer>

#include <glm/gtx/vector_angle.hpp>

#include <QtCore/QTimer>

#include <AccountManager.h>
#include <NodeList.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>

#include "Application.h"
#include "Audio.h"
#include "Environment.h"
#include "Menu.h"
#include "MyAvatar.h"
#include "Physics.h"
#include "devices/Faceshift.h"
#include "devices/OculusManager.h"
#include "ui/TextRenderer.h"

using namespace std;

const glm::vec3 DEFAULT_UP_DIRECTION(0.0f, 1.0f, 0.0f);
const float YAW_SPEED = 500.0f;   // degrees/sec
const float PITCH_SPEED = 100.0f; // degrees/sec
const float COLLISION_RADIUS_SCALAR = 1.2f; // pertains to avatar-to-avatar collisions
const float COLLISION_RADIUS_SCALE = 0.125f;

const float DATA_SERVER_LOCATION_CHANGE_UPDATE_MSECS = 5 * 1000;

MyAvatar::MyAvatar() :
	Avatar(),
    _mousePressed(false),
    _bodyPitchDelta(0.0f),
    _bodyRollDelta(0.0f),
    _shouldJump(false),
    _gravity(0.0f, -1.0f, 0.0f),
    _distanceToNearestAvatar(std::numeric_limits<float>::max()),
    _elapsedTimeMoving(0.0f),
	_elapsedTimeStopped(0.0f),
    _elapsedTimeSinceCollision(0.0f),
    _lastCollisionPosition(0, 0, 0),
    _speedBrakes(false),
    _isThrustOn(false),
    _thrustMultiplier(1.0f),
    _moveTarget(0,0,0),
    _moveTargetStepCounter(0),
    _lookAtTargetAvatar(),
    _shouldRender(true),
    _billboardValid(false)
{
    for (int i = 0; i < MAX_DRIVE_KEYS; i++) {
        _driveKeys[i] = 0.0f;
    }
    
    // update our location every 5 seconds in the data-server, assuming that we are authenticated with one
    QTimer* locationUpdateTimer = new QTimer(this);
    connect(locationUpdateTimer, &QTimer::timeout, this, &MyAvatar::updateLocationInDataServer);
    locationUpdateTimer->start(DATA_SERVER_LOCATION_CHANGE_UPDATE_MSECS);
}

MyAvatar::~MyAvatar() {
    _lookAtTargetAvatar.clear();
}

void MyAvatar::reset() {
    // TODO? resurrect headMouse stuff?
    //_headMouseX = _glWidget->width() / 2;
    //_headMouseY = _glWidget->height() / 2;
    getHead()->reset();
    getHand()->reset();

    setVelocity(glm::vec3(0,0,0));
    setThrust(glm::vec3(0,0,0));
    setOrientation(glm::quat(glm::vec3(0,0,0)));
}

void MyAvatar::setMoveTarget(const glm::vec3 moveTarget) {
    _moveTarget = moveTarget;
    _moveTargetStepCounter = 0;
}

void MyAvatar::update(float deltaTime) {
    updateFromGyros(deltaTime);

    // Update head mouse from faceshift if active
    Faceshift* faceshift = Application::getInstance()->getFaceshift();
    if (faceshift->isActive()) {
        // TODO? resurrect headMouse stuff?
        //glm::vec3 headVelocity = faceshift->getHeadAngularVelocity();
        //// sets how quickly head angular rotation moves the head mouse
        //const float HEADMOUSE_FACESHIFT_YAW_SCALE = 40.f;
        //const float HEADMOUSE_FACESHIFT_PITCH_SCALE = 30.f;
        //_headMouseX -= headVelocity.y * HEADMOUSE_FACESHIFT_YAW_SCALE;
        //_headMouseY -= headVelocity.x * HEADMOUSE_FACESHIFT_PITCH_SCALE;
        //
        ////  Constrain head-driven mouse to edges of screen
        //_headMouseX = glm::clamp(_headMouseX, 0, _glWidget->width());
        //_headMouseY = glm::clamp(_headMouseY, 0, _glWidget->height());
    }

    Head* head = getHead();
    if (OculusManager::isConnected()) {
        float yaw, pitch, roll; // these angles will be in radians
        OculusManager::getEulerAngles(yaw, pitch, roll);

        // but these euler angles are stored in degrees
        head->setYaw(yaw * DEGREES_PER_RADIAN);
        head->setPitch(pitch * DEGREES_PER_RADIAN);
        head->setRoll(roll * DEGREES_PER_RADIAN);
    }

    //  Get audio loudness data from audio input device
    Audio* audio = Application::getInstance()->getAudio();
    head->setAudioLoudness(audio->getLastInputLoudness());
    head->setAudioAverageLoudness(audio->getAudioAverageInputLoudness());

    if (Menu::getInstance()->isOptionChecked(MenuOption::Gravity)) {
        setGravity(Application::getInstance()->getEnvironment()->getGravity(getPosition()));
    } else {
        setGravity(glm::vec3(0.0f, 0.0f, 0.0f));
    }

    simulate(deltaTime);
}

void MyAvatar::simulate(float deltaTime) {

    glm::quat orientation = getOrientation();

    // Update movement timers
    _elapsedTimeSinceCollision += deltaTime;
    const float VELOCITY_MOVEMENT_TIMER_THRESHOLD = 0.2f;
    if (glm::length(_velocity) < VELOCITY_MOVEMENT_TIMER_THRESHOLD) {
        _elapsedTimeMoving = 0.f;
        _elapsedTimeStopped += deltaTime;
    } else {
        _elapsedTimeStopped = 0.f;
        _elapsedTimeMoving += deltaTime;
    }

    if (_scale != _targetScale) {
        float scale = (1.f - SMOOTHING_RATIO) * _scale + SMOOTHING_RATIO * _targetScale;
        setScale(scale);
        Application::getInstance()->getCamera()->setScale(scale);
    }

    //  Collect thrust forces from keyboard and devices
    updateThrust(deltaTime);

    // copy velocity so we can use it later for acceleration
    glm::vec3 oldVelocity = getVelocity();

    // calculate speed
    _speed = glm::length(_velocity);

    // update the movement of the hand and process handshaking with other avatars...
    updateHandMovementAndTouching(deltaTime);

    // apply gravity
    // For gravity, always move the avatar by the amount driven by gravity, so that the collision
    // routines will detect it and collide every frame when pulled by gravity to a surface
    const float MIN_DISTANCE_AFTER_COLLISION_FOR_GRAVITY = 0.02f;
    if (glm::length(_position - _lastCollisionPosition) > MIN_DISTANCE_AFTER_COLLISION_FOR_GRAVITY) {
        _velocity += _scale * _gravity * (GRAVITY_EARTH * deltaTime);
    }

    if (_collisionFlags != 0) {
        Camera* myCamera = Application::getInstance()->getCamera();

        float radius = getSkeletonHeight() * COLLISION_RADIUS_SCALE;
        if (myCamera->getMode() == CAMERA_MODE_FIRST_PERSON && !OculusManager::isConnected()) {
            radius = myCamera->getAspectRatio() * (myCamera->getNearClip() / cosf(0.5f * RADIANS_PER_DEGREE * myCamera->getFieldOfView()));
            radius *= COLLISION_RADIUS_SCALAR;
        }

        if (_collisionFlags & COLLISION_GROUP_ENVIRONMENT) {
            updateCollisionWithEnvironment(deltaTime, radius);
        }
        if (_collisionFlags & COLLISION_GROUP_VOXELS) {
            updateCollisionWithVoxels(deltaTime, radius);
        }
        if (_collisionFlags & COLLISION_GROUP_AVATARS) {
            updateCollisionWithAvatars(deltaTime);
        }
    }

    // add thrust to velocity
    _velocity += _thrust * deltaTime;

    // update body yaw by body yaw delta
    orientation = orientation * glm::quat(glm::radians(
                glm::vec3(_bodyPitchDelta, _bodyYawDelta, _bodyRollDelta) * deltaTime));
    // decay body rotation momentum

    const float BODY_SPIN_FRICTION = 7.5f;
    float bodySpinMomentum = 1.f - BODY_SPIN_FRICTION * deltaTime;
    if (bodySpinMomentum < 0.0f) { bodySpinMomentum = 0.0f; }
    _bodyPitchDelta *= bodySpinMomentum;
    _bodyYawDelta *= bodySpinMomentum;
    _bodyRollDelta *= bodySpinMomentum;

    float MINIMUM_ROTATION_RATE = 2.0f;
    if (fabs(_bodyYawDelta) < MINIMUM_ROTATION_RATE) { _bodyYawDelta = 0.f; }
    if (fabs(_bodyRollDelta) < MINIMUM_ROTATION_RATE) { _bodyRollDelta = 0.f; }
    if (fabs(_bodyPitchDelta) < MINIMUM_ROTATION_RATE) { _bodyPitchDelta = 0.f; }

    const float MAX_STATIC_FRICTION_VELOCITY = 0.5f;
    const float STATIC_FRICTION_STRENGTH = _scale * 20.f;
    applyStaticFriction(deltaTime, _velocity, MAX_STATIC_FRICTION_VELOCITY, STATIC_FRICTION_STRENGTH);

    // Damp avatar velocity
    const float LINEAR_DAMPING_STRENGTH = 0.5f;
    const float SPEED_BRAKE_POWER = _scale * 10.0f;
    const float SQUARED_DAMPING_STRENGTH = 0.007f;

    const float SLOW_NEAR_RADIUS = 5.f;
    float linearDamping = LINEAR_DAMPING_STRENGTH;
    const float NEAR_AVATAR_DAMPING_FACTOR = 50.f;
    if (_distanceToNearestAvatar < _scale * SLOW_NEAR_RADIUS) {
        linearDamping *= 1.f + NEAR_AVATAR_DAMPING_FACTOR *
                            ((SLOW_NEAR_RADIUS - _distanceToNearestAvatar) / SLOW_NEAR_RADIUS);
    }
    if (_speedBrakes) {
        applyDamping(deltaTime, _velocity,  linearDamping * SPEED_BRAKE_POWER, SQUARED_DAMPING_STRENGTH * SPEED_BRAKE_POWER);
    } else {
        applyDamping(deltaTime, _velocity, linearDamping, SQUARED_DAMPING_STRENGTH);
    }

    // update the euler angles
    setOrientation(orientation);

    // Compute instantaneous acceleration
    float forwardAcceleration = glm::length(glm::dot(getBodyFrontDirection(), getVelocity() - oldVelocity)) / deltaTime;
    const float OCULUS_ACCELERATION_PULL_THRESHOLD = 1.0f;
    const int OCULUS_YAW_OFFSET_THRESHOLD = 10;

    if (!Application::getInstance()->getFaceshift()->isActive() && OculusManager::isConnected() &&
            fabsf(forwardAcceleration) > OCULUS_ACCELERATION_PULL_THRESHOLD &&
            fabs(getHead()->getYaw()) > OCULUS_YAW_OFFSET_THRESHOLD) {
            
        // if we're wearing the oculus
        // and this acceleration is above the pull threshold
        // and the head yaw if off the body by more than OCULUS_YAW_OFFSET_THRESHOLD

        // match the body yaw to the oculus yaw
        _bodyYaw = getAbsoluteHeadYaw();

        // set the head yaw to zero for this draw
        getHead()->setYaw(0);

        // correct the oculus yaw offset
        OculusManager::updateYawOffset();
    }

    const float WALKING_SPEED_THRESHOLD = 0.2f;
    // use speed and angular velocity to determine walking vs. standing
    if (_speed + fabs(_bodyYawDelta) > WALKING_SPEED_THRESHOLD) {
        _mode = AVATAR_MODE_WALKING;
    } else {
        _mode = AVATAR_MODE_INTERACTING;
    }

    // update moving flag based on speed
    const float MOVING_SPEED_THRESHOLD = 0.01f;
    _moving = _speed > MOVING_SPEED_THRESHOLD;

    // If a move target is set, update position explicitly
    const float MOVE_FINISHED_TOLERANCE = 0.1f;
    const float MOVE_SPEED_FACTOR = 2.f;
    const int MOVE_TARGET_MAX_STEPS = 250;
    if ((glm::length(_moveTarget) > EPSILON) && (_moveTargetStepCounter < MOVE_TARGET_MAX_STEPS))  {
        if (glm::length(_position - _moveTarget) > MOVE_FINISHED_TOLERANCE) {
            _position += (_moveTarget - _position) * (deltaTime * MOVE_SPEED_FACTOR);
            _moveTargetStepCounter++;
        } else {
            //  Move completed
            _moveTarget = glm::vec3(0,0,0);
            _moveTargetStepCounter = 0;
        }
    }

    updateChatCircle(deltaTime);

    _position += _velocity * deltaTime;

    // update avatar skeleton and simulate hand and head
    getHand()->collideAgainstOurself(); 
    getHand()->simulate(deltaTime, true);

    _skeletonModel.simulate(deltaTime);

    // copy out the skeleton joints from the model
    _jointData.resize(_skeletonModel.getJointStateCount());
    for (int i = 0; i < _jointData.size(); i++) {
        JointData& data = _jointData[i];
        data.valid = _skeletonModel.getJointState(i, data.rotation);
    }

    Head* head = getHead();
    glm::vec3 headPosition;
    if (!_skeletonModel.getHeadPosition(headPosition)) {
        headPosition = _position;
    }
    head->setPosition(headPosition);
    head->setScale(_scale);
    head->simulate(deltaTime, true);

    // Zero thrust out now that we've added it to velocity in this frame
    _thrust = glm::vec3(0, 0, 0);
    
    // consider updating our billboard
    maybeUpdateBillboard();
}

//  Update avatar head rotation with sensor data
void MyAvatar::updateFromGyros(float deltaTime) {
    Faceshift* faceshift = Application::getInstance()->getFaceshift();
    Visage* visage = Application::getInstance()->getVisage();
    glm::vec3 estimatedPosition, estimatedRotation;

    bool trackerActive = false;
    if (faceshift->isActive()) {
        estimatedPosition = faceshift->getHeadTranslation();
        estimatedRotation = glm::degrees(safeEulerAngles(faceshift->getHeadRotation()));
        trackerActive = true;
    
    } else if (visage->isActive()) {
        estimatedPosition = visage->getHeadTranslation();
        estimatedRotation = glm::degrees(safeEulerAngles(visage->getHeadRotation()));
        trackerActive = true;
    }

    Head* head = getHead();
    if (trackerActive) {
        //  Rotate the body if the head is turned beyond the screen
        if (Menu::getInstance()->isOptionChecked(MenuOption::TurnWithHead)) {
            const float TRACKER_YAW_TURN_SENSITIVITY = 0.5f;
            const float TRACKER_MIN_YAW_TURN = 15.f;
            const float TRACKER_MAX_YAW_TURN = 50.f;
            if ( (fabs(estimatedRotation.y) > TRACKER_MIN_YAW_TURN) &&
                 (fabs(estimatedRotation.y) < TRACKER_MAX_YAW_TURN) ) {
                if (estimatedRotation.y > 0.f) {
                    _bodyYawDelta += (estimatedRotation.y - TRACKER_MIN_YAW_TURN) * TRACKER_YAW_TURN_SENSITIVITY;
                } else {
                    _bodyYawDelta += (estimatedRotation.y + TRACKER_MIN_YAW_TURN) * TRACKER_YAW_TURN_SENSITIVITY;
                }
            }
        }
    } else {
        // restore rotation, lean to neutral positions
        const float RESTORE_PERIOD = 1.f;   // seconds
        float restorePercentage = glm::clamp(deltaTime/RESTORE_PERIOD, 0.f, 1.f);
        head->setPitchTweak(glm::mix(head->getPitchTweak(), 0.0f, restorePercentage));
        head->setYawTweak(glm::mix(head->getYawTweak(), 0.0f, restorePercentage));
        head->setRollTweak(glm::mix(head->getRollTweak(), 0.0f, restorePercentage));
        head->setLeanSideways(glm::mix(head->getLeanSideways(), 0.0f, restorePercentage));
        head->setLeanForward(glm::mix(head->getLeanForward(), 0.0f, restorePercentage));
        return;
    }

    // Set the rotation of the avatar's head (as seen by others, not affecting view frustum)
    // to be scaled.  Pitch is greater to emphasize nodding behavior / synchrony.
    const float AVATAR_HEAD_PITCH_MAGNIFY = 1.0f;
    const float AVATAR_HEAD_YAW_MAGNIFY = 1.0f;
    const float AVATAR_HEAD_ROLL_MAGNIFY = 1.0f;
    head->setPitchTweak(estimatedRotation.x * AVATAR_HEAD_PITCH_MAGNIFY);
    head->setYawTweak(estimatedRotation.y * AVATAR_HEAD_YAW_MAGNIFY);
    head->setRollTweak(estimatedRotation.z * AVATAR_HEAD_ROLL_MAGNIFY);

    //  Update torso lean distance based on accelerometer data
    const float TORSO_LENGTH = 0.5f;
    glm::vec3 relativePosition = estimatedPosition - glm::vec3(0.0f, -TORSO_LENGTH, 0.0f);
    const float MAX_LEAN = 45.0f;
    head->setLeanSideways(glm::clamp(glm::degrees(atanf(relativePosition.x * _leanScale / TORSO_LENGTH)),
        -MAX_LEAN, MAX_LEAN));
    head->setLeanForward(glm::clamp(glm::degrees(atanf(relativePosition.z * _leanScale / TORSO_LENGTH)),
        -MAX_LEAN, MAX_LEAN));

    // if Faceshift drive is enabled, set the avatar drive based on the head position
    if (!Menu::getInstance()->isOptionChecked(MenuOption::MoveWithLean)) {
        return;
    }

    //  Move with Lean by applying thrust proportional to leaning
    glm::quat orientation = head->getCameraOrientation();
    glm::vec3 front = orientation * IDENTITY_FRONT;
    glm::vec3 right = orientation * IDENTITY_RIGHT;
    float leanForward = head->getLeanForward();
    float leanSideways = head->getLeanSideways();

    //  Degrees of 'dead zone' when leaning, and amount of acceleration to apply to lean angle
    const float LEAN_FWD_DEAD_ZONE = 15.f;
    const float LEAN_SIDEWAYS_DEAD_ZONE = 10.f;
    const float LEAN_FWD_THRUST_SCALE = 4.f;
    const float LEAN_SIDEWAYS_THRUST_SCALE = 3.f;

    if (fabs(leanForward) > LEAN_FWD_DEAD_ZONE) {
        if (leanForward > 0.f) {
            addThrust(front * -(leanForward - LEAN_FWD_DEAD_ZONE) * LEAN_FWD_THRUST_SCALE);
        } else {
            addThrust(front * -(leanForward + LEAN_FWD_DEAD_ZONE) * LEAN_FWD_THRUST_SCALE);
        }
    }
    if (fabs(leanSideways) > LEAN_SIDEWAYS_DEAD_ZONE) {
        if (leanSideways > 0.f) {
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
    printf("head-above-torso %.2f, scale = %0.2f\n", torsoToHead, getScale());

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
}

void MyAvatar::renderHeadMouse() const {
    // TODO? resurrect headMouse stuff?
    /*
    //  Display small target box at center or head mouse target that can also be used to measure LOD
    glColor3f(1.f, 1.f, 1.f);
    glDisable(GL_LINE_SMOOTH);
    const int PIXEL_BOX = 16;
    glBegin(GL_LINES);
    glVertex2f(_headMouseX - PIXEL_BOX/2, _headMouseY);
    glVertex2f(_headMouseX + PIXEL_BOX/2, _headMouseY);
    glVertex2f(_headMouseX, _headMouseY - PIXEL_BOX/2);
    glVertex2f(_headMouseX, _headMouseY + PIXEL_BOX/2);
    glEnd();
    glEnable(GL_LINE_SMOOTH);
    glColor3f(1.f, 0.f, 0.f);
    glPointSize(3.0f);
    glDisable(GL_POINT_SMOOTH);
    glBegin(GL_POINTS);
    glVertex2f(_headMouseX - 1, _headMouseY + 1);
    glEnd();
    //  If Faceshift is active, show eye pitch and yaw as separate pointer
    if (_faceshift.isActive()) {
        const float EYE_TARGET_PIXELS_PER_DEGREE = 40.0;
        int eyeTargetX = (_glWidget->width() / 2) -  _faceshift.getEstimatedEyeYaw() * EYE_TARGET_PIXELS_PER_DEGREE;
        int eyeTargetY = (_glWidget->height() / 2) -  _faceshift.getEstimatedEyePitch() * EYE_TARGET_PIXELS_PER_DEGREE;

        glColor3f(0.f, 1.f, 1.f);
        glDisable(GL_LINE_SMOOTH);
        glBegin(GL_LINES);
        glVertex2f(eyeTargetX - PIXEL_BOX/2, eyeTargetY);
        glVertex2f(eyeTargetX + PIXEL_BOX/2, eyeTargetY);
        glVertex2f(eyeTargetX, eyeTargetY - PIXEL_BOX/2);
        glVertex2f(eyeTargetX, eyeTargetY + PIXEL_BOX/2);
        glEnd();

    }
    */
}

void MyAvatar::saveData(QSettings* settings) {
    settings->beginGroup("Avatar");

    settings->setValue("bodyYaw", _bodyYaw);
    settings->setValue("bodyPitch", _bodyPitch);
    settings->setValue("bodyRoll", _bodyRoll);

    settings->setValue("headPitch", getHead()->getPitch());

    settings->setValue("position_x", _position.x);
    settings->setValue("position_y", _position.y);
    settings->setValue("position_z", _position.z);

    settings->setValue("pupilDilation", getHead()->getPupilDilation());

    settings->setValue("leanScale", _leanScale);
    settings->setValue("scale", _targetScale);
    
    settings->setValue("faceModelURL", _faceModelURL);
    settings->setValue("skeletonModelURL", _skeletonModelURL);
    settings->setValue("displayName", _displayName);

    settings->endGroup();
}

void MyAvatar::loadData(QSettings* settings) {
    settings->beginGroup("Avatar");

    // in case settings is corrupt or missing loadSetting() will check for NaN
    _bodyYaw = loadSetting(settings, "bodyYaw", 0.0f);
    _bodyPitch = loadSetting(settings, "bodyPitch", 0.0f);
    _bodyRoll = loadSetting(settings, "bodyRoll", 0.0f);

    getHead()->setPitch(loadSetting(settings, "headPitch", 0.0f));

    _position.x = loadSetting(settings, "position_x", 0.0f);
    _position.y = loadSetting(settings, "position_y", 0.0f);
    _position.z = loadSetting(settings, "position_z", 0.0f);

    getHead()->setPupilDilation(loadSetting(settings, "pupilDilation", 0.0f));

    _leanScale = loadSetting(settings, "leanScale", 0.05f);
    _targetScale = loadSetting(settings, "scale", 1.0f);
    setScale(_scale);
    Application::getInstance()->getCamera()->setScale(_scale);
    
    setFaceModelURL(settings->value("faceModelURL").toUrl());
    setSkeletonModelURL(settings->value("skeletonModelURL").toUrl());
    setDisplayName(settings->value("displayName").toString());

    settings->endGroup();
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

void MyAvatar::orbit(const glm::vec3& position, int deltaX, int deltaY) {
    // first orbit horizontally
    glm::quat orientation = getOrientation();
    const float ANGULAR_SCALE = 0.5f;
    glm::quat rotation = glm::angleAxis(glm::radians(- deltaX * ANGULAR_SCALE), orientation * IDENTITY_UP);
    setPosition(position + rotation * (getPosition() - position));
    orientation = rotation * orientation;
    setOrientation(orientation);
    
    // then vertically
    float oldPitch = getHead()->getPitch();
    getHead()->setPitch(oldPitch - deltaY * ANGULAR_SCALE);
    rotation = glm::angleAxis(glm::radians((getHead()->getPitch() - oldPitch)), orientation * IDENTITY_RIGHT);

    setPosition(position + rotation * (getPosition() - position));
}

void MyAvatar::updateLookAtTargetAvatar() {
    Application* applicationInstance = Application::getInstance();
    
    if (!applicationInstance->isMousePressed()) {
        glm::vec3 mouseOrigin = applicationInstance->getMouseRayOrigin();
        glm::vec3 mouseDirection = applicationInstance->getMouseRayDirection();

        foreach (const AvatarSharedPointer& avatarPointer, Application::getInstance()->getAvatarManager().getAvatarHash()) {
            Avatar* avatar = static_cast<Avatar*>(avatarPointer.data());
            float distance;
            if (avatar->findRayIntersection(mouseOrigin, mouseDirection, distance)) {
                _lookAtTargetAvatar = avatarPointer;
                _targetAvatarPosition = avatarPointer->getPosition();
                return;
            }
        }
        _lookAtTargetAvatar.clear();
        _targetAvatarPosition = glm::vec3(0, 0, 0);
    }
}

void MyAvatar::clearLookAtTargetAvatar() {
    _lookAtTargetAvatar.clear();
}

float MyAvatar::getAbsoluteHeadYaw() const {
    const Head* head = static_cast<const Head*>(_headData);
    return glm::yaw(head->getOrientation());
}

glm::vec3 MyAvatar::getUprightHeadPosition() const {
    return _position + getWorldAlignedOrientation() * glm::vec3(0.0f, getPelvisToHeadLength(), 0.0f);
}

void MyAvatar::setJointData(int index, const glm::quat& rotation) {
    Avatar::setJointData(index, rotation);
    if (QThread::currentThread() == thread()) {
        _skeletonModel.setJointState(index, true, rotation);
    }
}

void MyAvatar::clearJointData(int index) {
    Avatar::clearJointData(index);
    if (QThread::currentThread() == thread()) {
        _skeletonModel.setJointState(index, false);
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

void MyAvatar::renderBody(RenderMode renderMode) {
    if (!(_skeletonModel.isRenderable() && getHead()->getFaceModel().isRenderable())) {
        return; // wait until both models are loaded
    }
    
    //  Render the body's voxels and head
    _skeletonModel.render(1.0f, renderMode == SHADOW_RENDER_MODE);

    //  Render head so long as the camera isn't inside it
    const float RENDER_HEAD_CUTOFF_DISTANCE = 0.40f;
    Camera* myCamera = Application::getInstance()->getCamera();
    if (renderMode != NORMAL_RENDER_MODE || (glm::length(myCamera->getPosition() - getHead()->calculateAverageEyePosition()) >
            RENDER_HEAD_CUTOFF_DISTANCE * _scale)) {
        getHead()->render(1.0f, renderMode == SHADOW_RENDER_MODE);
    }
    getHand()->render(true);
}

void MyAvatar::updateThrust(float deltaTime) {
    //
    //  Gather thrust information from keyboard and sensors to apply to avatar motion
    //
    glm::quat orientation = getHead()->getCameraOrientation();
    glm::vec3 front = orientation * IDENTITY_FRONT;
    glm::vec3 right = orientation * IDENTITY_RIGHT;
    glm::vec3 up = orientation * IDENTITY_UP;

    const float THRUST_MAG_UP = 800.0f;
    const float THRUST_MAG_DOWN = 300.f;
    const float THRUST_MAG_FWD = 500.f;
    const float THRUST_MAG_BACK = 300.f;
    const float THRUST_MAG_LATERAL = 250.f;
    const float THRUST_JUMP = 120.f;

    //  Add Thrusts from keyboard
    _thrust += _driveKeys[FWD] * _scale * THRUST_MAG_FWD * _thrustMultiplier * deltaTime * front;
    _thrust -= _driveKeys[BACK] * _scale * THRUST_MAG_BACK *  _thrustMultiplier * deltaTime * front;
    _thrust += _driveKeys[RIGHT] * _scale * THRUST_MAG_LATERAL * _thrustMultiplier * deltaTime * right;
    _thrust -= _driveKeys[LEFT] * _scale * THRUST_MAG_LATERAL * _thrustMultiplier * deltaTime * right;
    _thrust += _driveKeys[UP] * _scale * THRUST_MAG_UP * _thrustMultiplier * deltaTime * up;
    _thrust -= _driveKeys[DOWN] * _scale * THRUST_MAG_DOWN * _thrustMultiplier * deltaTime * up;
    _bodyYawDelta -= _driveKeys[ROT_RIGHT] * YAW_SPEED * deltaTime;
    _bodyYawDelta += _driveKeys[ROT_LEFT] * YAW_SPEED * deltaTime;
    getHead()->setPitch(getHead()->getPitch() + (_driveKeys[ROT_UP] - _driveKeys[ROT_DOWN]) * PITCH_SPEED * deltaTime);

    //  If thrust keys are being held down, slowly increase thrust to allow reaching great speeds
    if (_driveKeys[FWD] || _driveKeys[BACK] || _driveKeys[RIGHT] || _driveKeys[LEFT] || _driveKeys[UP] || _driveKeys[DOWN]) {
        const float THRUST_INCREASE_RATE = 1.05f;
        const float MAX_THRUST_MULTIPLIER = 75.0f;
        //printf("m = %.3f\n", _thrustMultiplier);
        if (_thrustMultiplier < MAX_THRUST_MULTIPLIER) {
            _thrustMultiplier *= 1.f + deltaTime * THRUST_INCREASE_RATE;
        }
    } else {
        _thrustMultiplier = 1.f;
    }

    //  Add one time jumping force if requested
    if (_shouldJump) {
        if (glm::length(_gravity) > EPSILON) {
            _thrust += _scale * THRUST_JUMP * up;
        }
        _shouldJump = false;
    }

    //  Update speed brake status
    const float MIN_SPEED_BRAKE_VELOCITY = _scale * 0.4f;
    if ((glm::length(_thrust) == 0.0f) && _isThrustOn && (glm::length(_velocity) > MIN_SPEED_BRAKE_VELOCITY)) {
        _speedBrakes = true;
    }

    if (_speedBrakes && (glm::length(_velocity) < MIN_SPEED_BRAKE_VELOCITY)) {
        _speedBrakes = false;
    }
    _isThrustOn = (glm::length(_thrust) > EPSILON);
}

void MyAvatar::updateHandMovementAndTouching(float deltaTime) {
    glm::quat orientation = getOrientation();

    // reset hand and arm positions according to hand movement
    glm::vec3 up = orientation * IDENTITY_UP;

    bool pointing = false;
    if (glm::length(_mouseRayDirection) > EPSILON && !Application::getInstance()->isMouseHidden()) {
        // confine to the approximate shoulder plane
        glm::vec3 pointDirection = _mouseRayDirection;
        if (glm::dot(_mouseRayDirection, up) > 0.0f) {
            glm::vec3 projectedVector = glm::cross(up, glm::cross(_mouseRayDirection, up));
            if (glm::length(projectedVector) > EPSILON) {
                pointDirection = glm::normalize(projectedVector);
            }
        }
        glm::vec3 shoulderPosition;
        if (_skeletonModel.getRightShoulderPosition(shoulderPosition)) {
            glm::vec3 farVector = _mouseRayOrigin + pointDirection * (float)TREE_SCALE - shoulderPosition;
            const float ARM_RETRACTION = 0.75f;
            float retractedLength = _skeletonModel.getRightArmLength() * ARM_RETRACTION;
            setHandPosition(shoulderPosition + glm::normalize(farVector) * retractedLength);
            pointing = true;
        }
    }

    if (_mousePressed) {
        _handState = HAND_STATE_GRASPING;
    } else if (pointing) {
        _handState = HAND_STATE_POINTING;
    } else {
        _handState = HAND_STATE_NULL;
    }
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
        _lastCollisionPosition = _position;
        updateCollisionSound(penetration, deltaTime, ENVIRONMENT_COLLISION_FREQUENCY);
        applyHardCollision(penetration, ENVIRONMENT_SURFACE_ELASTICITY, ENVIRONMENT_SURFACE_DAMPING);
    }
}

void MyAvatar::updateCollisionWithVoxels(float deltaTime, float radius) {
    const float VOXEL_ELASTICITY = 0.4f;
    const float VOXEL_DAMPING = 0.0f;
    const float VOXEL_COLLISION_FREQUENCY = 0.5f;
    glm::vec3 penetration;
    float pelvisFloatingHeight = getPelvisFloatingHeight();
    if (Application::getInstance()->getVoxelTree()->findCapsulePenetration(
            _position - glm::vec3(0.0f, pelvisFloatingHeight - radius, 0.0f),
            _position + glm::vec3(0.0f, getSkeletonHeight() - pelvisFloatingHeight + radius, 0.0f), radius, penetration)) {
        _lastCollisionPosition = _position;
        updateCollisionSound(penetration, deltaTime, VOXEL_COLLISION_FREQUENCY);
        applyHardCollision(penetration, VOXEL_ELASTICITY, VOXEL_DAMPING);
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
        _elapsedTimeSinceCollision = 0.0f;
        glm::vec3 direction = penetration / penetrationLength;
        _velocity -= glm::dot(_velocity, direction) * direction * (1.f + elasticity);
        _velocity *= glm::clamp(1.f - damping, 0.0f, 1.0f);
        if ((glm::length(_velocity) < HALTING_VELOCITY) && (glm::length(_thrust) == 0.f)) {
            // If moving really slowly after a collision, and not applying forces, stop altogether
            _velocity *= 0.f;
        }
    }
}

void MyAvatar::updateCollisionSound(const glm::vec3 &penetration, float deltaTime, float frequency) {
    //  consider whether to have the collision make a sound
    const float AUDIBLE_COLLISION_THRESHOLD = 0.02f;
    const float COLLISION_LOUDNESS = 1.f;
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
            std::min(COLLISION_LOUDNESS * velocityTowardCollision, 1.f),
            frequency * (1.f + velocityTangentToCollision / velocityTowardCollision),
            std::min(velocityTangentToCollision / velocityTowardCollision * NOISE_SCALING, 1.f),
            1.f - DURATION_SCALING * powf(frequency, 0.5f) / velocityTowardCollision, true);
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
            if (xzDistance > 0.f) {
                positionBA.y = 0.f;
                // note, penetration should point from A into B
                penetration = positionBA * ((radiusA + radiusB - xzDistance) / xzDistance);
                return true;
            } else {
                // exactly coaxial -- we'll return false for this case
                return false;
            }
        } else if (yDistance < halfHeights + radiusA + radiusB) {
            // caps collide
            if (positionBA.y < 0.f) {
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

    // HACK: body-body collision uses two coaxial capsules with axes parallel to y-axis
    // TODO: make the collision work without assuming avatar orientation

    // TODO: these local variables are not used in the live code, only in the
    // commented-outTODO code below.
    //Extents myStaticExtents = _skeletonModel.getStaticExtents();
    //glm::vec3 staticScale = myStaticExtents.maximum - myStaticExtents.minimum;
    //float myCapsuleRadius = 0.25f * (staticScale.x + staticScale.z);
    //float myCapsuleHeight = staticScale.y;

    CollisionInfo collisionInfo;
    foreach (const AvatarSharedPointer& avatarPointer, avatars) {
        Avatar* avatar = static_cast<Avatar*>(avatarPointer.data());
        if (static_cast<Avatar*>(this) == avatar) {
            // don't collide with ourselves
            continue;
        }
        float distance = glm::length(_position - avatar->getPosition());        
        if (_distanceToNearestAvatar > distance) {
            _distanceToNearestAvatar = distance;
        }
        float theirBoundingRadius = avatar->getBoundingRadius();
        if (distance < myBoundingRadius + theirBoundingRadius) {
            _skeletonModel.updateShapePositions();
            Model& headModel = getHead()->getFaceModel();
            headModel.updateShapePositions();

            /* TODO: Andrew to fix Avatar-Avatar body collisions
            Extents theirStaticExtents = _skeletonModel.getStaticExtents();
            glm::vec3 staticScale = theirStaticExtents.maximum - theirStaticExtents.minimum;
            float theirCapsuleRadius = 0.25f * (staticScale.x + staticScale.z);
            float theirCapsuleHeight = staticScale.y;

            glm::vec3 penetration(0.f);
            if (findAvatarAvatarPenetration(_position, myCapsuleRadius, myCapsuleHeight,
                avatar->getPosition(), theirCapsuleRadius, theirCapsuleHeight, penetration)) {
                // move the avatar out by half the penetration
                setPosition(_position - 0.5f * penetration);
            }
            */

            // collide our hands against them
            getHand()->collideAgainstAvatar(avatar, true);

            // collide their hands against us
            avatar->getHand()->collideAgainstAvatar(this, false);
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
    QImage image = Application::getInstance()->renderAvatarBillboard();
    _billboard.clear();
    QBuffer buffer(&_billboard);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    _billboardValid = true;
    
    sendBillboardPacket();
}

void MyAvatar::setGravity(glm::vec3 gravity) {
    _gravity = gravity;
    getHead()->setGravity(_gravity);

    // use the gravity to determine the new world up direction, if possible
    float gravityLength = glm::length(gravity);
    if (gravityLength > EPSILON) {
        _worldUpDirection = _gravity / -gravityLength;
    } else {
        _worldUpDirection = DEFAULT_UP_DIRECTION;
    }
}

void MyAvatar::goHome() {
    qDebug("Going Home!");
    setPosition(START_LOCATION);
}

void MyAvatar::increaseSize() {
    if ((1.f + SCALING_RATIO) * _targetScale < MAX_AVATAR_SCALE) {
        _targetScale *= (1.f + SCALING_RATIO);
        qDebug("Changed scale to %f", _targetScale);
    }
}

void MyAvatar::decreaseSize() {
    if (MIN_AVATAR_SCALE < (1.f - SCALING_RATIO) * _targetScale) {
        _targetScale *= (1.f - SCALING_RATIO);
        qDebug("Changed scale to %f", _targetScale);
    }
}

void MyAvatar::resetSize() {
    _targetScale = 1.0f;
    qDebug("Reseted scale to %f", _targetScale);
}

static QByteArray createByteArray(const glm::vec3& vector) {
    return QByteArray::number(vector.x) + ',' + QByteArray::number(vector.y) + ',' + QByteArray::number(vector.z);
}

void MyAvatar::updateLocationInDataServer() {
    // TODO: don't re-send this when it hasn't change or doesn't change by some threshold
    // This will required storing the last sent values and clearing them when the AccountManager rootURL changes
    
    AccountManager& accountManager = AccountManager::getInstance();
    
    if (accountManager.isLoggedIn()) {
        QString positionString(createByteArray(_position));
        QString orientationString(createByteArray(glm::degrees(safeEulerAngles(getOrientation()))));
        
        // construct the json to put the user's location
        QString locationPutJson = QString() + "{\"address\":{\"position\":\""
            + positionString + "\", \"orientation\":\"" + orientationString + "\"}}";
        
        accountManager.authenticatedRequest("/api/v1/users/address", QNetworkAccessManager::PutOperation,
                                            JSONCallbackParameters(), locationPutJson.toUtf8());
    }
}

void MyAvatar::goToLocationFromResponse(const QJsonObject& jsonObject) {
    
    if (jsonObject["status"].toString() == "success") {
        
        // send a node kill request, indicating to other clients that they should play the "disappeared" effect
        sendKillAvatar();
        
        QJsonObject locationObject = jsonObject["data"].toObject()["address"].toObject();
        QString positionString = locationObject["position"].toString();
        QString orientationString = locationObject["orientation"].toString();
        QString domainHostnameString = locationObject["domain"].toString();
        
        qDebug() << "Changing domain to" << domainHostnameString <<
            ", position to" << positionString <<
            ", and orientation to" << orientationString;
        
        QStringList coordinateItems = positionString.split(',');
        QStringList orientationItems = orientationString.split(',');
        
        NodeList::getInstance()->getDomainInfo().setHostname(domainHostnameString);
        
        // orient the user to face the target
        glm::quat newOrientation = glm::quat(glm::radians(glm::vec3(orientationItems[0].toFloat(),
                                                                    orientationItems[1].toFloat(),
                                                                    orientationItems[2].toFloat())))
            * glm::angleAxis(PI, glm::vec3(0.0f, 1.0f, 0.0f));
        setOrientation(newOrientation);
        
        // move the user a couple units away
        const float DISTANCE_TO_USER = 2.0f;
        glm::vec3 newPosition = glm::vec3(coordinateItems[0].toFloat(), coordinateItems[1].toFloat(),
                                          coordinateItems[2].toFloat()) - newOrientation * IDENTITY_FRONT * DISTANCE_TO_USER;
        setPosition(newPosition);
    }
    
}
