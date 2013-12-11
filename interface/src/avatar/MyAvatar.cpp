//
//  MyAvatar.cpp
//  interface
//
//  Created by Mark Peng on 8/16/13.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include <vector>

#include <glm/gtx/vector_angle.hpp>

#include <NodeList.h>
#include <NodeTypes.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>

#include "Application.h"
#include "DataServerClient.h"
#include "Menu.h"
#include "MyAvatar.h"
#include "Physics.h"
#include "devices/OculusManager.h"
#include "ui/TextRenderer.h"

using namespace std;

const glm::vec3 DEFAULT_UP_DIRECTION(0.0f, 1.0f, 0.0f);
const float YAW_MAG = 500.0;
const float PITCH_MAG = 100.0f;
const float COLLISION_RADIUS_SCALAR = 1.2; // pertains to avatar-to-avatar collisions
const float COLLISION_BALL_FORCE = 200.0; // pertains to avatar-to-avatar collisions
const float COLLISION_BODY_FORCE = 30.0; // pertains to avatar-to-avatar collisions
const float COLLISION_RADIUS_SCALE = 0.125f;
const float MOUSE_RAY_TOUCH_RANGE = 0.01f;
const bool USING_HEAD_LEAN = false;
const float SKIN_COLOR[] = {1.0, 0.84, 0.66};
const float DARK_SKIN_COLOR[] = {0.9, 0.78, 0.63};

MyAvatar::MyAvatar(Node* owningNode) :
	Avatar(owningNode),
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
    _moveTargetStepCounter(0)
{
    for (int i = 0; i < MAX_DRIVE_KEYS; i++) {
        _driveKeys[i] = 0.0f;
    }

    _collisionRadius = _height * COLLISION_RADIUS_SCALE;   
}

void MyAvatar::reset() {
    _head.reset();
    _hand.reset();
}

void MyAvatar::setMoveTarget(const glm::vec3 moveTarget) {
    _moveTarget = moveTarget;
    _moveTargetStepCounter = 0;
}

void MyAvatar::simulate(float deltaTime, Transmitter* transmitter) {

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

    if (_leadingAvatar && !_leadingAvatar->getOwningNode()->isAlive()) {
        follow(NULL);
    }

    // Ajust, scale, position and lookAt position when following an other avatar
    if (_leadingAvatar && _newScale != _leadingAvatar->getScale()) {
        _newScale = _leadingAvatar->getScale();
    }

    if (_scale != _newScale) {
        float scale = (1.f - SMOOTHING_RATIO) * _scale + SMOOTHING_RATIO * _newScale;
        setScale(scale);
        Application::getInstance()->getCamera()->setScale(scale);
    }
    
    //  Collect thrust forces from keyboard and devices 
    updateThrust(deltaTime, transmitter);
    
    // copy velocity so we can use it later for acceleration
    glm::vec3 oldVelocity = getVelocity();
    
    // calculate speed
    _speed = glm::length(_velocity);
    
    // update balls
    if (_balls) {
        _balls->moveOrigin(_position);
        glm::vec3 lookAt = _head.getLookAtPosition();
        if (glm::length(lookAt) > EPSILON) {
            _balls->moveOrigin(lookAt);
        } else {
            _balls->moveOrigin(_position);
        }
        _balls->simulate(deltaTime);
    }
    
    // update torso rotation based on head lean
    _skeleton.joint[AVATAR_JOINT_TORSO].rotation = glm::quat(glm::radians(glm::vec3(
        _head.getLeanForward(), 0.0f, _head.getLeanSideways())));
    
    // apply joint data (if any) to skeleton
    bool enableHandMovement = true;
    for (vector<JointData>::iterator it = _joints.begin(); it != _joints.end(); it++) {
        _skeleton.joint[it->jointID].rotation = it->rotation;
        
        // disable hand movement if we have joint info for the right wrist
        enableHandMovement &= (it->jointID != AVATAR_JOINT_RIGHT_WRIST);
    }
    
    // update the movement of the hand and process handshaking with other avatars...
    updateHandMovementAndTouching(deltaTime, enableHandMovement);

    // apply gravity
    // For gravity, always move the avatar by the amount driven by gravity, so that the collision
    // routines will detect it and collide every frame when pulled by gravity to a surface
    const float MIN_DISTANCE_AFTER_COLLISION_FOR_GRAVITY = 0.02f;
    if (glm::length(_position - _lastCollisionPosition) > MIN_DISTANCE_AFTER_COLLISION_FOR_GRAVITY) {
        _velocity += _scale * _gravity * (GRAVITY_EARTH * deltaTime);
    }
    
    // Only collide if we are not moving to a target
    if (_isCollisionsOn && (glm::length(_moveTarget) < EPSILON)) {

        Camera* myCamera = Application::getInstance()->getCamera();

        if (myCamera->getMode() == CAMERA_MODE_FIRST_PERSON && !OculusManager::isConnected()) {
            _collisionRadius = myCamera->getAspectRatio() * (myCamera->getNearClip() / cos(myCamera->getFieldOfView() / 2.f));
            _collisionRadius *= COLLISION_RADIUS_SCALAR;
        } else {
            _collisionRadius = _height * COLLISION_RADIUS_SCALE;
        }

        updateCollisionWithEnvironment(deltaTime);
        updateCollisionWithVoxels(deltaTime);
        updateAvatarCollisions(deltaTime);
    }
    
    // add thrust to velocity
    _velocity += _thrust * deltaTime;
    
    // update body yaw by body yaw delta
    orientation = orientation * glm::quat(glm::radians(
        glm::vec3(_bodyPitchDelta, _bodyYawDelta, _bodyRollDelta) * deltaTime));
    // decay body rotation momentum
    
    const float BODY_SPIN_FRICTION = 7.5f;
    float bodySpinMomentum = 1.0 - BODY_SPIN_FRICTION * deltaTime;
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
    const float ACCELERATION_PITCH_DECAY = 0.4f;
    const float ACCELERATION_YAW_DECAY = 0.4f;
    const float ACCELERATION_PULL_THRESHOLD = 0.2f;
    const float OCULUS_ACCELERATION_PULL_THRESHOLD = 1.0f;
    const int OCULUS_YAW_OFFSET_THRESHOLD = 10;
    
    if (!Application::getInstance()->getFaceshift()->isActive()) {
        // Decay HeadPitch as a function of acceleration, so that you look straight ahead when
        // you start moving, but don't do this with an HMD like the Oculus.
        if (!OculusManager::isConnected()) {
            if (forwardAcceleration > ACCELERATION_PULL_THRESHOLD) {
                _head.setPitch(_head.getPitch() * (1.f - forwardAcceleration * ACCELERATION_PITCH_DECAY * deltaTime));
                _head.setYaw(_head.getYaw() * (1.f - forwardAcceleration * ACCELERATION_YAW_DECAY * deltaTime));
            }
        } else if (fabsf(forwardAcceleration) > OCULUS_ACCELERATION_PULL_THRESHOLD
                   && fabs(_head.getYaw()) > OCULUS_YAW_OFFSET_THRESHOLD) {
            // if we're wearing the oculus
            // and this acceleration is above the pull threshold
            // and the head yaw if off the body by more than OCULUS_YAW_OFFSET_THRESHOLD
            
            // match the body yaw to the oculus yaw
            _bodyYaw = getAbsoluteHeadYaw();
            
            // set the head yaw to zero for this draw
            _head.setYaw(0);
            
            // correct the oculus yaw offset
            OculusManager::updateYawOffset();
        }
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
    _skeleton.update(deltaTime, getOrientation(), _position);
    _hand.simulate(deltaTime, true);
    _skeletonModel.simulate(deltaTime);
    _head.setBodyRotation(glm::vec3(_bodyPitch, _bodyYaw, _bodyRoll));
    glm::vec3 headPosition;
    _skeletonModel.getHeadPosition(headPosition);
    _head.setPosition(headPosition);
    _head.setScale(_scale);
    _head.setSkinColor(glm::vec3(SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]));
    _head.simulate(deltaTime, true);

    // Zero thrust out now that we've added it to velocity in this frame
    _thrust = glm::vec3(0, 0, 0);

}

const float MAX_PITCH = 90.0f;

//  Update avatar head rotation with sensor data
void MyAvatar::updateFromGyrosAndOrWebcam(bool turnWithHead) {
    Faceshift* faceshift = Application::getInstance()->getFaceshift();
    SerialInterface* gyros = Application::getInstance()->getSerialHeadSensor();
    Webcam* webcam = Application::getInstance()->getWebcam();
    glm::vec3 estimatedPosition, estimatedRotation;
    
    if (faceshift->isActive()) {
        estimatedPosition = faceshift->getHeadTranslation();
        estimatedRotation = safeEulerAngles(faceshift->getHeadRotation());
        //  Rotate the body if the head is turned beyond the screen
        if (turnWithHead) {
            const float FACESHIFT_YAW_TURN_SENSITIVITY = 0.5f;
            const float FACESHIFT_MIN_YAW_TURN = 15.f;
            const float FACESHIFT_MAX_YAW_TURN = 50.f;
            if ( (fabs(estimatedRotation.y) > FACESHIFT_MIN_YAW_TURN) &&
                 (fabs(estimatedRotation.y) < FACESHIFT_MAX_YAW_TURN) ) {
                if (estimatedRotation.y > 0.f) {
                    _bodyYawDelta += (estimatedRotation.y - FACESHIFT_MIN_YAW_TURN) * FACESHIFT_YAW_TURN_SENSITIVITY;
                } else {
                    _bodyYawDelta += (estimatedRotation.y + FACESHIFT_MIN_YAW_TURN) * FACESHIFT_YAW_TURN_SENSITIVITY;
                }
            }
        }
    } else if (gyros->isActive()) {
        estimatedRotation = gyros->getEstimatedRotation();
    
    } else if (webcam->isActive()) {
        estimatedRotation = webcam->getEstimatedRotation();
    
    } else {
        if (!_leadingAvatar) {
            _head.setPitch(_head.getMousePitch());
        }
        _head.getVideoFace().clearFrame();
        
        // restore rotation, lean to neutral positions
        const float RESTORE_RATE = 0.05f;
        _head.setYaw(glm::mix(_head.getYaw(), 0.0f, RESTORE_RATE));
        _head.setRoll(glm::mix(_head.getRoll(), 0.0f, RESTORE_RATE));
        _head.setLeanSideways(glm::mix(_head.getLeanSideways(), 0.0f, RESTORE_RATE));
        _head.setLeanForward(glm::mix(_head.getLeanForward(), 0.0f, RESTORE_RATE));
        return;
    }

    if (webcam->isActive()) {
        estimatedPosition = webcam->getEstimatedPosition();
        
        // apply face data
        _head.getVideoFace().setFrameFromWebcam();
        
        // compute and store the joint rotations
        const JointVector& joints = webcam->getEstimatedJoints();
        _joints.clear();
        for (int i = 0; i < NUM_AVATAR_JOINTS; i++) {
            if (joints.size() > i && joints[i].isValid) {
                JointData data = { i, joints[i].rotation };
                _joints.push_back(data);
                
                if (i == AVATAR_JOINT_CHEST) {
                    // if we have a chest rotation, don't apply lean based on head
                    estimatedPosition = glm::vec3();
                }
            }
        }
    } else {
        _head.getVideoFace().clearFrame();
    }
    
    // Set the rotation of the avatar's head (as seen by others, not affecting view frustum)
    // to be scaled.  Pitch is greater to emphasize nodding behavior / synchrony. 
    const float AVATAR_HEAD_PITCH_MAGNIFY = 1.0f;
    const float AVATAR_HEAD_YAW_MAGNIFY = 1.0f;
    const float AVATAR_HEAD_ROLL_MAGNIFY = 1.0f;
    _head.setPitch(estimatedRotation.x * AVATAR_HEAD_PITCH_MAGNIFY);
    _head.setYaw(estimatedRotation.y * AVATAR_HEAD_YAW_MAGNIFY);
    _head.setRoll(estimatedRotation.z * AVATAR_HEAD_ROLL_MAGNIFY);
        
    //  Update torso lean distance based on accelerometer data
    const float TORSO_LENGTH = 0.5f;
    glm::vec3 relativePosition = estimatedPosition - glm::vec3(0.0f, -TORSO_LENGTH, 0.0f);
    const float MAX_LEAN = 45.0f;
    _head.setLeanSideways(glm::clamp(glm::degrees(atanf(relativePosition.x * _leanScale / TORSO_LENGTH)),
        -MAX_LEAN, MAX_LEAN));
    _head.setLeanForward(glm::clamp(glm::degrees(atanf(relativePosition.z * _leanScale / TORSO_LENGTH)),
        -MAX_LEAN, MAX_LEAN));
    
    // if Faceshift drive is enabled, set the avatar drive based on the head position
    if (!Menu::getInstance()->isOptionChecked(MenuOption::MoveWithLean)) {
        return;
    }
    
    //  Move with Lean by applying thrust proportional to leaning
    glm::quat orientation = _head.getCameraOrientation();
    glm::vec3 front = orientation * IDENTITY_FRONT;
    glm::vec3 right = orientation * IDENTITY_RIGHT;
    float leanForward = _head.getLeanForward();
    float leanSideways = _head.getLeanSideways();

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

static TextRenderer* textRenderer() {
    static TextRenderer* renderer = new TextRenderer(SANS_FONT_FAMILY, 24, -1, false, TextRenderer::SHADOW_EFFECT);
    return renderer;
}

void MyAvatar::render(bool forceRenderHead) {
        
    // render body
    renderBody(forceRenderHead);

    //  Render the balls
    if (_balls) {
        glPushMatrix();
        _balls->render();
        glPopMatrix();
    }
    
    if (!_chatMessage.empty()) {
        int width = 0;
        int lastWidth = 0;
        for (string::iterator it = _chatMessage.begin(); it != _chatMessage.end(); it++) {
            width += (lastWidth = textRenderer()->computeWidth(*it));
        }
        glPushMatrix();
        
        glm::vec3 chatPosition = getPosition() + getBodyUpDirection() * chatMessageHeight * _scale;
        glTranslatef(chatPosition.x, chatPosition.y, chatPosition.z);
        glm::quat chatRotation = Application::getInstance()->getCamera()->getRotation();
        glm::vec3 chatAxis = glm::axis(chatRotation);
        glRotatef(glm::angle(chatRotation), chatAxis.x, chatAxis.y, chatAxis.z);
        
        
        glColor3f(0, 0.8, 0);
        glRotatef(180, 0, 1, 0);
        glRotatef(180, 0, 0, 1);
        glScalef(_scale * chatMessageScale, _scale * chatMessageScale, 1.0f);
        
        glDisable(GL_LIGHTING);
        glDepthMask(false);
        if (_keyState == NO_KEY_DOWN) {
            textRenderer()->draw(-width / 2.0f, 0, _chatMessage.c_str());
            
        } else {
            // rather than using substr and allocating a new string, just replace the last
            // character with a null, then restore it
            int lastIndex = _chatMessage.size() - 1;
            char lastChar = _chatMessage[lastIndex];
            _chatMessage[lastIndex] = '\0';
            textRenderer()->draw(-width / 2.0f, 0, _chatMessage.c_str());
            _chatMessage[lastIndex] = lastChar;
            glColor3f(0, 1, 0);
            textRenderer()->draw(width / 2.0f - lastWidth, 0, _chatMessage.c_str() + lastIndex);
        }
        glEnable(GL_LIGHTING);
        glDepthMask(true);
        
        glPopMatrix();
    }
}

void MyAvatar::saveData(QSettings* settings) {
    settings->beginGroup("Avatar");
    
    settings->setValue("bodyYaw", _bodyYaw);
    settings->setValue("bodyPitch", _bodyPitch);
    settings->setValue("bodyRoll", _bodyRoll);
    
    settings->setValue("mousePitch", _head.getMousePitch());
    
    settings->setValue("position_x", _position.x);
    settings->setValue("position_y", _position.y);
    settings->setValue("position_z", _position.z);
    
    settings->setValue("pupilDilation", _head.getPupilDilation());
    
    settings->setValue("leanScale", _leanScale);
    settings->setValue("scale", _newScale);
    
    settings->endGroup();
}

void MyAvatar::loadData(QSettings* settings) {
    settings->beginGroup("Avatar");
    
    // in case settings is corrupt or missing loadSetting() will check for NaN
    _bodyYaw = loadSetting(settings, "bodyYaw", 0.0f);
    _bodyPitch = loadSetting(settings, "bodyPitch", 0.0f);
    _bodyRoll = loadSetting(settings, "bodyRoll", 0.0f);
    
    _head.setMousePitch(loadSetting(settings, "mousePitch", 0.0f));
    
    _position.x = loadSetting(settings, "position_x", 0.0f);
    _position.y = loadSetting(settings, "position_y", 0.0f);
    _position.z = loadSetting(settings, "position_z", 0.0f);
    
    _head.setPupilDilation(settings->value("pupilDilation", 0.0f).toFloat());
    
    _leanScale = loadSetting(settings, "leanScale", 0.05f);
    
    _newScale = loadSetting(settings, "scale", 1.0f);
    setScale(_scale);
    Application::getInstance()->getCamera()->setScale(_scale);
    
    settings->endGroup();
}


float MyAvatar::getAbsoluteHeadYaw() const {
    return glm::yaw(_head.getOrientation());
}

glm::vec3 MyAvatar::getUprightHeadPosition() const {
    return _position + getWorldAlignedOrientation() * glm::vec3(0.0f, _pelvisToHeadLength, 0.0f);
}

glm::vec3 MyAvatar::getEyeLevelPosition() const {
    const float EYE_UP_OFFSET = 0.36f;
    return _position + getWorldAlignedOrientation() * _skeleton.joint[AVATAR_JOINT_TORSO].rotation *
        glm::vec3(0.0f, _pelvisToHeadLength + _scale * BODY_BALL_RADIUS_HEAD_BASE * EYE_UP_OFFSET, 0.0f);
}

void MyAvatar::renderBody(bool forceRenderHead) {

    if (_head.getVideoFace().isFullFrame()) {
        //  Render the full-frame video
            _head.getVideoFace().render(1.0f);
    } else {
        //  Render the body's voxels and head
        _skeletonModel.render(1.0f);
        _head.render(1.0f, false);
    }
    _hand.render(true);
}

void MyAvatar::updateThrust(float deltaTime, Transmitter * transmitter) {
    //
    //  Gather thrust information from keyboard and sensors to apply to avatar motion 
    //
    glm::quat orientation = getHead().getCameraOrientation();
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
    _bodyYawDelta -= _driveKeys[ROT_RIGHT] * YAW_MAG * deltaTime;
    _bodyYawDelta += _driveKeys[ROT_LEFT] * YAW_MAG * deltaTime;
    _head.setMousePitch(_head.getMousePitch() + (_driveKeys[ROT_UP] - _driveKeys[ROT_DOWN]) * PITCH_MAG * deltaTime);
    
    //  If thrust keys are being held down, slowly increase thrust to allow reaching great speeds
    if (_driveKeys[FWD] || _driveKeys[BACK] || _driveKeys[RIGHT] || _driveKeys[LEFT] || _driveKeys[UP] || _driveKeys[DOWN]) {
        const float THRUST_INCREASE_RATE = 1.05;
        const float MAX_THRUST_MULTIPLIER = 75.0;
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


    // Add thrusts from leading avatar
    const float FOLLOWING_RATE = 0.02f;
    const float MIN_YAW = 5.0f;
    const float MIN_PITCH = 1.0f;
    const float PITCH_RATE = 0.1f;
    const float MIN_YAW_BEFORE_PITCH = 30.0f;

    if (_leadingAvatar != NULL) {
        glm::vec3 toTarget = _leadingAvatar->getPosition() - _position;

        if (glm::length(_position - _leadingAvatar->getPosition()) > _scale * _stringLength) {
            _position += toTarget * FOLLOWING_RATE;
        } else {
            toTarget = _leadingAvatar->getHead().getLookAtPosition() - _head.getPosition();
        }
        toTarget = glm::vec3(glm::dot(right, toTarget),
                             glm::dot(up   , toTarget),
                             glm::dot(front, toTarget));

        float yawAngle = angleBetween(-IDENTITY_FRONT, glm::vec3(toTarget.x, 0.f, toTarget.z));
        if (glm::abs(yawAngle) > MIN_YAW){
            if (IDENTITY_RIGHT.x * toTarget.x + IDENTITY_RIGHT.y * toTarget.y + IDENTITY_RIGHT.z * toTarget.z > 0) {
                _bodyYawDelta -= yawAngle;
            } else {
                _bodyYawDelta += yawAngle;
            }
        }

        float pitchAngle = glm::abs(90.0f - angleBetween(IDENTITY_UP, toTarget));
        if (glm::abs(pitchAngle) > MIN_PITCH && yawAngle < MIN_YAW_BEFORE_PITCH){
            if (IDENTITY_UP.x * toTarget.x + IDENTITY_UP.y * toTarget.y + IDENTITY_UP.z * toTarget.z > 0) {
                _head.setMousePitch(_head.getMousePitch() + PITCH_RATE * pitchAngle);
            } else {
                _head.setMousePitch(_head.getMousePitch() - PITCH_RATE * pitchAngle);
            }
            _head.setPitch(_head.getMousePitch());
        }
    }


    //  Add thrusts from Transmitter
    if (transmitter) {
        transmitter->checkForLostTransmitter();
        glm::vec3 rotation = transmitter->getEstimatedRotation();
        const float TRANSMITTER_MIN_RATE = 1.f;
        const float TRANSMITTER_MIN_YAW_RATE = 4.f;
        const float TRANSMITTER_LATERAL_FORCE_SCALE = 5.f;
        const float TRANSMITTER_FWD_FORCE_SCALE = 25.f;
        const float TRANSMITTER_UP_FORCE_SCALE = 100.f;
        const float TRANSMITTER_YAW_SCALE = 10.0f;
        const float TRANSMITTER_LIFT_SCALE = 3.f;
        const float TOUCH_POSITION_RANGE_HALF = 32767.f;
        if (fabs(rotation.z) > TRANSMITTER_MIN_RATE) {
            _thrust += rotation.z * TRANSMITTER_LATERAL_FORCE_SCALE * deltaTime * right;
        }
        if (fabs(rotation.x) > TRANSMITTER_MIN_RATE) {
            _thrust += -rotation.x * TRANSMITTER_FWD_FORCE_SCALE * deltaTime * front;
        }
        if (fabs(rotation.y) > TRANSMITTER_MIN_YAW_RATE) {
            _bodyYawDelta += rotation.y * TRANSMITTER_YAW_SCALE * deltaTime;
        }
        if (transmitter->getTouchState()->state == 'D') {
            _thrust += TRANSMITTER_UP_FORCE_SCALE *
            (float)(transmitter->getTouchState()->y - TOUCH_POSITION_RANGE_HALF) / TOUCH_POSITION_RANGE_HALF *
            TRANSMITTER_LIFT_SCALE *
            deltaTime *
            up;
        }
    }
    //  Add thrust and rotation from hand controllers
    const float THRUST_MAG_HAND_JETS = THRUST_MAG_FWD;
    const float JOYSTICK_YAW_MAG = YAW_MAG;
    const float JOYSTICK_PITCH_MAG = PITCH_MAG * 0.5f;
    const int THRUST_CONTROLLER = 0;
    const int VIEW_CONTROLLER = 1;
    for (size_t i = 0; i < getHand().getPalms().size(); ++i) {
        PalmData& palm = getHand().getPalms()[i];
        if (palm.isActive() && (palm.getSixenseID() == THRUST_CONTROLLER)) {
            if (palm.getJoystickY() != 0.f) {
                FingerData& finger = palm.getFingers()[0];
                if (finger.isActive()) {
                }
                _thrust += front * _scale * THRUST_MAG_HAND_JETS * palm.getJoystickY() * _thrustMultiplier * deltaTime;
            }
            if (palm.getJoystickX() != 0.f) {
                _thrust += right * _scale * THRUST_MAG_HAND_JETS * palm.getJoystickX() * _thrustMultiplier * deltaTime;
            }
        } else if (palm.isActive() && (palm.getSixenseID() == VIEW_CONTROLLER)) {
            if (palm.getJoystickX() != 0.f) {
                _bodyYawDelta -= palm.getJoystickX() * JOYSTICK_YAW_MAG * deltaTime;
            }
            if (palm.getJoystickY() != 0.f) {
                getHand().setPitchUpdate(getHand().getPitchUpdate() +
                                         (palm.getJoystickY() * JOYSTICK_PITCH_MAG * deltaTime));
            }
        }
        
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

void MyAvatar::updateHandMovementAndTouching(float deltaTime, bool enableHandMovement) {
    
    glm::quat orientation = getOrientation();
    
    // reset hand and arm positions according to hand movement
    glm::vec3 up = orientation * IDENTITY_UP;
    
    bool pointing = false;
    if (enableHandMovement && glm::length(_mouseRayDirection) > EPSILON && !Application::getInstance()->isMouseHidden()) {
        // confine to the approximate shoulder plane
        glm::vec3 pointDirection = _mouseRayDirection;
        if (glm::dot(_mouseRayDirection, up) > 0.0f) {
            glm::vec3 projectedVector = glm::cross(up, glm::cross(_mouseRayDirection, up));
            if (glm::length(projectedVector) > EPSILON) {
                pointDirection = glm::normalize(projectedVector);
            }
        }
        const float FAR_AWAY_POINT = TREE_SCALE;
        _skeleton.joint[AVATAR_JOINT_RIGHT_FINGERTIPS].position = _mouseRayOrigin + pointDirection * FAR_AWAY_POINT;
        pointing = true;
    }
    
    //Set right hand position and state to be transmitted, and also tell AvatarTouch about it
    setHandPosition(_skeleton.joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position);
    
    if (_mousePressed) {
        _handState = HAND_STATE_GRASPING;
    } else if (pointing) {
        _handState = HAND_STATE_POINTING;
    } else {
        _handState = HAND_STATE_NULL;
    }
}

void MyAvatar::updateCollisionWithEnvironment(float deltaTime) {
    glm::vec3 up = getBodyUpDirection();
    float radius = _collisionRadius;
    const float ENVIRONMENT_SURFACE_ELASTICITY = 1.0f;
    const float ENVIRONMENT_SURFACE_DAMPING = 0.01;
    const float ENVIRONMENT_COLLISION_FREQUENCY = 0.05f;
    glm::vec3 penetration;
    if (Application::getInstance()->getEnvironment()->findCapsulePenetration(
            _position - up * (_pelvisFloatingHeight - radius),
            _position + up * (_height - _pelvisFloatingHeight + radius), radius, penetration)) {
        _lastCollisionPosition = _position;
        updateCollisionSound(penetration, deltaTime, ENVIRONMENT_COLLISION_FREQUENCY);
        applyHardCollision(penetration, ENVIRONMENT_SURFACE_ELASTICITY, ENVIRONMENT_SURFACE_DAMPING);
    }
}


void MyAvatar::updateCollisionWithVoxels(float deltaTime) {
    float radius = _collisionRadius;
    const float VOXEL_ELASTICITY = 1.4f;
    const float VOXEL_DAMPING = 0.0;
    const float VOXEL_COLLISION_FREQUENCY = 0.5f;
    glm::vec3 penetration;
    if (Application::getInstance()->getVoxels()->findCapsulePenetration(
            _position - glm::vec3(0.0f, _pelvisFloatingHeight - radius, 0.0f),
            _position + glm::vec3(0.0f, _height - _pelvisFloatingHeight + radius, 0.0f), radius, penetration)) {
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
    //  if elasticity = 1.0, collision is inelastic.
    //  if elasticity > 1.0, collision is elastic. 
    //  
    _position -= penetration;
    static float HALTING_VELOCITY = 0.2f;
    // cancel out the velocity component in the direction of penetration
    float penetrationLength = glm::length(penetration);
    if (penetrationLength > EPSILON) {
        _elapsedTimeSinceCollision = 0.0f;
        glm::vec3 direction = penetration / penetrationLength;
        _velocity -= glm::dot(_velocity, direction) * direction * elasticity;
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
            fmin(COLLISION_LOUDNESS * velocityTowardCollision, 1.f),
            frequency * (1.f + velocityTangentToCollision / velocityTowardCollision),
            fmin(velocityTangentToCollision / velocityTowardCollision * NOISE_SCALING, 1.f),
            1.f - DURATION_SCALING * powf(frequency, 0.5f) / velocityTowardCollision, true);
    }
}

void MyAvatar::updateAvatarCollisions(float deltaTime) {
    
    //  Reset detector for nearest avatar
    _distanceToNearestAvatar = std::numeric_limits<float>::max();
    
    // loop through all the other avatars for potential interactions...
    NodeList* nodeList = NodeList::getInstance();
    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
        if (node->getLinkedData() && node->getType() == NODE_TYPE_AGENT) {
            //Avatar *otherAvatar = (Avatar *)node->getLinkedData();
            //
            // Placeholder:  Add code here when we want to add Avatar<->Avatar collision stuff
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
    NodeList* nodeList = NodeList::getInstance();
    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
        if (node->getLinkedData() && node->getType() == NODE_TYPE_AGENT) {
            SortedAvatar sortedAvatar; 
            sortedAvatar.avatar = (Avatar*)node->getLinkedData();
            if (!sortedAvatar.avatar->isChatCirclingEnabled()) {
                continue;
            }
            sortedAvatar.distance = glm::distance(_position, sortedAvatar.avatar->getPosition());
            sortedAvatars.append(sortedAvatar);
        }
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
    for (int i = sortedAvatars.size() - 1; i >= 0; i--) {
        float radius = (CIRCUMFERENCE_PER_MEMBER * (i + 2)) / PI_TIMES_TWO;
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
    float radius = (CIRCUMFERENCE_PER_MEMBER * (sortedAvatars.size() + 1)) / PI_TIMES_TWO;
    
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
    float leftDistance = PI_TIMES_TWO;
    float rightDistance = PI_TIMES_TWO;
    foreach (const SortedAvatar& sortedAvatar, sortedAvatars) {
        delta = sortedAvatar.avatar->getPosition() - center;
        projected = glm::vec3(glm::dot(right, delta), glm::dot(front, delta), 0.0f);
        float angle = glm::length(projected) > EPSILON ? atan2f(projected.y, projected.x) : 0.0f;
        if (angle < myAngle) {
            leftDistance = min(myAngle - angle, leftDistance);
            rightDistance = min(PI_TIMES_TWO - (myAngle - angle), rightDistance);
            
        } else {
            leftDistance = min(PI_TIMES_TWO - (angle - myAngle), leftDistance);
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

void MyAvatar::setGravity(glm::vec3 gravity) {
    _gravity = gravity;
    _head.setGravity(_gravity);
    
    // use the gravity to determine the new world up direction, if possible
    float gravityLength = glm::length(gravity);
    if (gravityLength > EPSILON) {
        _worldUpDirection = _gravity / -gravityLength;
    } else {
        _worldUpDirection = DEFAULT_UP_DIRECTION;
    }
}

void MyAvatar::setOrientation(const glm::quat& orientation) {
    glm::vec3 eulerAngles = safeEulerAngles(orientation);
    _bodyPitch = eulerAngles.x;
    _bodyYaw = eulerAngles.y;
    _bodyRoll = eulerAngles.z;
}
