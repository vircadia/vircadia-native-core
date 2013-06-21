//
//  Avatar.cpp
//  interface
//
//  Created by Philip Rosedale on 9/11/12.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <vector>
#include <SharedUtil.h>
#include "world.h"
#include "Application.h"
#include "Avatar.h"
#include "Head.h"
#include "Log.h"
#include "ui/TextRenderer.h"
#include <AgentList.h>
#include <AgentTypes.h>
#include <PacketHeaders.h>
#include <OculusManager.h>

using namespace std;

const bool  BALLS_ON                      = false;
const bool  USING_AVATAR_GRAVITY          = true;
const glm::vec3 DEFAULT_UP_DIRECTION        (0.0f, 1.0f, 0.0f);
const float YAW_MAG                       = 500.0;
const float BODY_SPIN_FRICTION            = 5.0;
const float MY_HAND_HOLDING_PULL          = 0.2;
const float YOUR_HAND_HOLDING_PULL        = 1.0;
const float BODY_SPRING_DEFAULT_TIGHTNESS = 1000.0f;
const float BODY_SPRING_FORCE             = 300.0f;
const float BODY_SPRING_DECAY             = 16.0f;
const float COLLISION_RADIUS_SCALAR       = 1.2;   //pertains to avatar-to-avatar collisions
const float COLLISION_BALL_FORCE          = 200.0; //pertains to avatar-to-avatar collisions
const float COLLISION_BODY_FORCE          = 30.0;  //pertains to avatar-to-avatar collisions
const float HEAD_ROTATION_SCALE           = 0.70;
const float HEAD_ROLL_SCALE               = 0.40;
const float HEAD_MAX_PITCH                = 45;
const float HEAD_MIN_PITCH                = -45;
const float HEAD_MAX_YAW                  = 85;
const float HEAD_MIN_YAW                  = -85;
const float PERIPERSONAL_RADIUS           = 1.0f;
const float AVATAR_BRAKING_STRENGTH       = 40.0f;
const float MOUSE_RAY_TOUCH_RANGE         = 0.01f;
const float FLOATING_HEIGHT               = 0.13f;
const bool  USING_HEAD_LEAN               = false;
const float LEAN_SENSITIVITY              = 0.15;
const float LEAN_MAX                      = 0.45;
const float LEAN_AVERAGING                = 10.0;
const float HEAD_RATE_MAX                 = 50.f;
const float SKIN_COLOR[]                  = {1.0, 0.84, 0.66};
const float DARK_SKIN_COLOR[]             = {0.9, 0.78, 0.63};
const int   NUM_BODY_CONE_SIDES           = 9;

bool usingBigSphereCollisionTest = true;

float chatMessageScale = 0.0015;
float chatMessageHeight = 0.20;

Avatar::Avatar(Agent* owningAgent) :
    AvatarData(owningAgent),
    _initialized(false),
    _head(this),
    _ballSpringsInitialized(false),
    _TEST_bigSphereRadius(0.5f),
    _TEST_bigSpherePosition(5.0f, _TEST_bigSphereRadius, 5.0f),
    _mousePressed(false),
    _bodyPitchDelta(0.0f),
    _bodyYawDelta(0.0f),
    _bodyRollDelta(0.0f),
    _movedHandOffset(0.0f, 0.0f, 0.0f),
    _mode(AVATAR_MODE_STANDING),
    _handHoldingPosition(0.0f, 0.0f, 0.0f),
    _velocity(0.0f, 0.0f, 0.0f),
    _thrust(0.0f, 0.0f, 0.0f),
    _shouldJump(false),
    _speed(0.0f),
    _maxArmLength(0.0f),
    _leanScale(0.5f),
    _pelvisStandingHeight(0.0f),
    _pelvisFloatingHeight(0.0f),
    _distanceToNearestAvatar(std::numeric_limits<float>::max()),
    _gravity(0.0f, -1.0f, 0.0f),
    _worldUpDirection(DEFAULT_UP_DIRECTION),
    _mouseRayOrigin(0.0f, 0.0f, 0.0f),
    _mouseRayDirection(0.0f, 0.0f, 0.0f),
    _interactingOther(NULL),
    _isMouseTurningRight(false),
    _elapsedTimeMoving(0.0f),
    _elapsedTimeStopped(0.0f),
    _elapsedTimeSinceCollision(0.0f),
    _voxels(this)
{
    // give the pointer to our head to inherited _headData variable from AvatarData
    _headData = &_head;
    
    for (int i = 0; i < MAX_DRIVE_KEYS; i++) {
        _driveKeys[i] = false;
    }
    
    _skeleton.initialize();
    
    initializeBodyBalls();
    
    _height               = _skeleton.getHeight() + _bodyBall[ BODY_BALL_LEFT_HEEL ].radius + _bodyBall[ BODY_BALL_HEAD_BASE ].radius;
    
    _maxArmLength         = _skeleton.getArmLength();
    _pelvisStandingHeight = _skeleton.getPelvisStandingHeight() + _bodyBall[ BODY_BALL_LEFT_HEEL ].radius;
    _pelvisFloatingHeight = _skeleton.getPelvisFloatingHeight() + _bodyBall[ BODY_BALL_LEFT_HEEL ].radius;
    _pelvisToHeadLength   = _skeleton.getPelvisToHeadLength();
    
    _avatarTouch.setReachableRadius(PERIPERSONAL_RADIUS);
    
    if (BALLS_ON) {
        _balls = new Balls(100);
    } else {
        _balls = NULL;
    }
}


void Avatar::initializeBodyBalls() {
    
    _ballSpringsInitialized = false; //this gets set to true on the first update pass...
    
    for (int b = 0; b < NUM_AVATAR_BODY_BALLS; b++) {
        _bodyBall[b].parentJoint    = AVATAR_JOINT_NULL;
        _bodyBall[b].parentOffset   = glm::vec3(0.0, 0.0, 0.0);
        _bodyBall[b].position       = glm::vec3(0.0, 0.0, 0.0);
        _bodyBall[b].velocity       = glm::vec3(0.0, 0.0, 0.0);
        _bodyBall[b].radius         = 0.0;
        _bodyBall[b].touchForce     = 0.0;
        _bodyBall[b].isCollidable   = true;
        _bodyBall[b].jointTightness = BODY_SPRING_DEFAULT_TIGHTNESS;
    }
    
    // specify the radius of each ball
    _bodyBall[ BODY_BALL_PELVIS           ].radius = 0.07;
    _bodyBall[ BODY_BALL_TORSO            ].radius = 0.065;
    _bodyBall[ BODY_BALL_CHEST            ].radius = 0.08;
    _bodyBall[ BODY_BALL_NECK_BASE        ].radius = 0.03;
    _bodyBall[ BODY_BALL_HEAD_BASE        ].radius = 0.07;
    _bodyBall[ BODY_BALL_LEFT_COLLAR      ].radius = 0.04;
    _bodyBall[ BODY_BALL_LEFT_SHOULDER    ].radius = 0.03;
    _bodyBall[ BODY_BALL_LEFT_ELBOW	      ].radius = 0.02;
    _bodyBall[ BODY_BALL_LEFT_WRIST       ].radius = 0.02;
    _bodyBall[ BODY_BALL_LEFT_FINGERTIPS  ].radius = 0.01;
    _bodyBall[ BODY_BALL_RIGHT_COLLAR     ].radius = 0.04;
    _bodyBall[ BODY_BALL_RIGHT_SHOULDER	  ].radius = 0.03;
    _bodyBall[ BODY_BALL_RIGHT_ELBOW	  ].radius = 0.02;
    _bodyBall[ BODY_BALL_RIGHT_WRIST      ].radius = 0.02;
    _bodyBall[ BODY_BALL_RIGHT_FINGERTIPS ].radius = 0.01;
    _bodyBall[ BODY_BALL_LEFT_HIP		  ].radius = 0.04;
    
    //_bodyBall[ BODY_BALL_LEFT_MID_THIGH ].radius = 0.03;
    
    _bodyBall[ BODY_BALL_LEFT_KNEE		  ].radius = 0.025;
    _bodyBall[ BODY_BALL_LEFT_HEEL		  ].radius = 0.025;
    _bodyBall[ BODY_BALL_LEFT_TOES		  ].radius = 0.025;
    _bodyBall[ BODY_BALL_RIGHT_HIP		  ].radius = 0.04;
    _bodyBall[ BODY_BALL_RIGHT_KNEE		  ].radius = 0.025;
    _bodyBall[ BODY_BALL_RIGHT_HEEL		  ].radius = 0.025;
    _bodyBall[ BODY_BALL_RIGHT_TOES		  ].radius = 0.025;
    
    
    // specify the parent joint for each ball
    _bodyBall[ BODY_BALL_PELVIS		      ].parentJoint = AVATAR_JOINT_PELVIS;
    _bodyBall[ BODY_BALL_TORSO            ].parentJoint = AVATAR_JOINT_TORSO;
    _bodyBall[ BODY_BALL_CHEST		      ].parentJoint = AVATAR_JOINT_CHEST;
    _bodyBall[ BODY_BALL_NECK_BASE	      ].parentJoint = AVATAR_JOINT_NECK_BASE;
    _bodyBall[ BODY_BALL_HEAD_BASE        ].parentJoint = AVATAR_JOINT_HEAD_BASE;
    _bodyBall[ BODY_BALL_HEAD_TOP         ].parentJoint = AVATAR_JOINT_HEAD_TOP;
    _bodyBall[ BODY_BALL_LEFT_COLLAR      ].parentJoint = AVATAR_JOINT_LEFT_COLLAR;
    _bodyBall[ BODY_BALL_LEFT_SHOULDER    ].parentJoint = AVATAR_JOINT_LEFT_SHOULDER;
    _bodyBall[ BODY_BALL_LEFT_ELBOW	      ].parentJoint = AVATAR_JOINT_LEFT_ELBOW;
    _bodyBall[ BODY_BALL_LEFT_WRIST		  ].parentJoint = AVATAR_JOINT_LEFT_WRIST;
    _bodyBall[ BODY_BALL_LEFT_FINGERTIPS  ].parentJoint = AVATAR_JOINT_LEFT_FINGERTIPS;
    _bodyBall[ BODY_BALL_RIGHT_COLLAR     ].parentJoint = AVATAR_JOINT_RIGHT_COLLAR;
    _bodyBall[ BODY_BALL_RIGHT_SHOULDER	  ].parentJoint = AVATAR_JOINT_RIGHT_SHOULDER;
    _bodyBall[ BODY_BALL_RIGHT_ELBOW	  ].parentJoint = AVATAR_JOINT_RIGHT_ELBOW;
    _bodyBall[ BODY_BALL_RIGHT_WRIST      ].parentJoint = AVATAR_JOINT_RIGHT_WRIST;
    _bodyBall[ BODY_BALL_RIGHT_FINGERTIPS ].parentJoint = AVATAR_JOINT_RIGHT_FINGERTIPS;
    _bodyBall[ BODY_BALL_LEFT_HIP		  ].parentJoint = AVATAR_JOINT_LEFT_HIP;
    _bodyBall[ BODY_BALL_LEFT_KNEE		  ].parentJoint = AVATAR_JOINT_LEFT_KNEE;
    _bodyBall[ BODY_BALL_LEFT_HEEL		  ].parentJoint = AVATAR_JOINT_LEFT_HEEL;
    _bodyBall[ BODY_BALL_LEFT_TOES		  ].parentJoint = AVATAR_JOINT_LEFT_TOES;
    _bodyBall[ BODY_BALL_RIGHT_HIP		  ].parentJoint = AVATAR_JOINT_RIGHT_HIP;
    _bodyBall[ BODY_BALL_RIGHT_KNEE		  ].parentJoint = AVATAR_JOINT_RIGHT_KNEE;
    _bodyBall[ BODY_BALL_RIGHT_HEEL		  ].parentJoint = AVATAR_JOINT_RIGHT_HEEL;
    _bodyBall[ BODY_BALL_RIGHT_TOES		  ].parentJoint = AVATAR_JOINT_RIGHT_TOES;
    
    //_bodyBall[ BODY_BALL_LEFT_MID_THIGH].parentJoint = AVATAR_JOINT_LEFT_HIP;
    
    // specify the parent offset for each ball
    _bodyBall[ BODY_BALL_PELVIS		      ].parentOffset   = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[ BODY_BALL_TORSO            ].parentOffset   = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[ BODY_BALL_CHEST		      ].parentOffset   = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[ BODY_BALL_NECK_BASE	      ].parentOffset   = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[ BODY_BALL_HEAD_BASE        ].parentOffset   = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[ BODY_BALL_HEAD_TOP         ].parentOffset   = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[ BODY_BALL_LEFT_COLLAR      ].parentOffset   = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[ BODY_BALL_LEFT_SHOULDER    ].parentOffset   = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[ BODY_BALL_LEFT_ELBOW	      ].parentOffset   = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[ BODY_BALL_LEFT_WRIST		  ].parentOffset   = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[ BODY_BALL_LEFT_FINGERTIPS  ].parentOffset   = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[ BODY_BALL_RIGHT_COLLAR     ].parentOffset   = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[ BODY_BALL_RIGHT_SHOULDER	  ].parentOffset   = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[ BODY_BALL_RIGHT_ELBOW	  ].parentOffset   = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[ BODY_BALL_RIGHT_WRIST      ].parentOffset   = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[ BODY_BALL_RIGHT_FINGERTIPS ].parentOffset   = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[ BODY_BALL_LEFT_HIP		  ].parentOffset   = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[ BODY_BALL_LEFT_KNEE		  ].parentOffset   = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[ BODY_BALL_LEFT_HEEL		  ].parentOffset   = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[ BODY_BALL_LEFT_TOES		  ].parentOffset   = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[ BODY_BALL_RIGHT_HIP		  ].parentOffset   = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[ BODY_BALL_RIGHT_KNEE		  ].parentOffset   = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[ BODY_BALL_RIGHT_HEEL		  ].parentOffset   = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[ BODY_BALL_RIGHT_TOES		  ].parentOffset   = glm::vec3(0.0, 0.0, 0.0);
    
    
    //_bodyBall[ BODY_BALL_LEFT_MID_THIGH].parentOffset = glm::vec3(-0.1, -0.1, 0.0);
    
    
    // specify the parent BALL for each ball
    _bodyBall[ BODY_BALL_PELVIS		      ].parentBall = BODY_BALL_NULL;
    _bodyBall[ BODY_BALL_TORSO            ].parentBall = BODY_BALL_PELVIS;
    _bodyBall[ BODY_BALL_CHEST		      ].parentBall = BODY_BALL_TORSO;
    _bodyBall[ BODY_BALL_NECK_BASE	      ].parentBall = BODY_BALL_CHEST;
    _bodyBall[ BODY_BALL_HEAD_BASE        ].parentBall = BODY_BALL_NECK_BASE;
    _bodyBall[ BODY_BALL_HEAD_TOP         ].parentBall = BODY_BALL_HEAD_BASE;
    _bodyBall[ BODY_BALL_LEFT_COLLAR      ].parentBall = BODY_BALL_CHEST;
    _bodyBall[ BODY_BALL_LEFT_SHOULDER    ].parentBall = BODY_BALL_LEFT_COLLAR;
    _bodyBall[ BODY_BALL_LEFT_ELBOW	      ].parentBall = BODY_BALL_LEFT_SHOULDER;
    _bodyBall[ BODY_BALL_LEFT_WRIST		  ].parentBall = BODY_BALL_LEFT_ELBOW;
    _bodyBall[ BODY_BALL_LEFT_FINGERTIPS  ].parentBall = BODY_BALL_LEFT_WRIST;
    _bodyBall[ BODY_BALL_RIGHT_COLLAR     ].parentBall = BODY_BALL_CHEST;
    _bodyBall[ BODY_BALL_RIGHT_SHOULDER	  ].parentBall = BODY_BALL_RIGHT_COLLAR;
    _bodyBall[ BODY_BALL_RIGHT_ELBOW	  ].parentBall = BODY_BALL_RIGHT_SHOULDER;
    _bodyBall[ BODY_BALL_RIGHT_WRIST      ].parentBall = BODY_BALL_RIGHT_ELBOW;
    _bodyBall[ BODY_BALL_RIGHT_FINGERTIPS ].parentBall = BODY_BALL_RIGHT_WRIST;
    _bodyBall[ BODY_BALL_LEFT_HIP		  ].parentBall = BODY_BALL_PELVIS;
    
    //_bodyBall[ BODY_BALL_LEFT_MID_THIGH ].parentBall = BODY_BALL_LEFT_HIP;
    
    //    _bodyBall[ BODY_BALL_LEFT_KNEE		  ].parentBall = BODY_BALL_LEFT_MID_THIGH;
    _bodyBall[ BODY_BALL_LEFT_KNEE		  ].parentBall = BODY_BALL_LEFT_HIP;
    
    _bodyBall[ BODY_BALL_LEFT_HEEL		  ].parentBall = BODY_BALL_LEFT_KNEE;
    _bodyBall[ BODY_BALL_LEFT_TOES		  ].parentBall = BODY_BALL_LEFT_HEEL;
    _bodyBall[ BODY_BALL_RIGHT_HIP		  ].parentBall = BODY_BALL_PELVIS;
    _bodyBall[ BODY_BALL_RIGHT_KNEE		  ].parentBall = BODY_BALL_RIGHT_HIP;
    _bodyBall[ BODY_BALL_RIGHT_HEEL		  ].parentBall = BODY_BALL_RIGHT_KNEE;
    _bodyBall[ BODY_BALL_RIGHT_TOES		  ].parentBall = BODY_BALL_RIGHT_HEEL;
    
    /*
     // to aid in hand-shaking and hand-holding, the right hand is not collidable
     _bodyBall[ BODY_BALL_RIGHT_ELBOW	    ].isCollidable = false;
     _bodyBall[ BODY_BALL_RIGHT_WRIST	    ].isCollidable = false;
     _bodyBall[ BODY_BALL_RIGHT_FINGERTIPS].isCollidable = false;
     */
}


Avatar::~Avatar() {
    _headData = NULL;
    delete _balls;
}

void Avatar::init() {
    _head.init();
    _voxels.init();
    _initialized = true;
}

void Avatar::reset() {
    _head.reset();
}

//  Update avatar head rotation with sensor data
void Avatar::updateHeadFromGyrosAndOrWebcam() {
    const float AMPLIFY_PITCH = 1.f;
    const float AMPLIFY_YAW = 1.f;
    const float AMPLIFY_ROLL = 1.f;

    SerialInterface* gyros = Application::getInstance()->getSerialHeadSensor();
    Webcam* webcam = Application::getInstance()->getWebcam();
    glm::vec3 estimatedPosition, estimatedRotation;
    if (gyros->isActive()) {
        estimatedPosition = gyros->getEstimatedPosition();
        estimatedRotation = gyros->getEstimatedRotation();
        
    } else if (webcam->isActive()) {
        estimatedPosition = webcam->getEstimatedPosition();
        estimatedRotation = webcam->getEstimatedRotation();
    
    } else {
        return;
    }
    _head.setPitch(estimatedRotation.x * AMPLIFY_PITCH);
    _head.setYaw(estimatedRotation.y * AMPLIFY_YAW);
    _head.setRoll(estimatedRotation.z * AMPLIFY_ROLL);
        
    //  Update torso lean distance based on accelerometer data
    const float TORSO_LENGTH = 0.5f;
    const float MAX_LEAN = 45.0f;
    _head.setLeanSideways(glm::clamp(glm::degrees(atanf(-estimatedPosition.x * _leanScale / TORSO_LENGTH)),
        -MAX_LEAN, MAX_LEAN));
    _head.setLeanForward(glm::clamp(glm::degrees(atanf(estimatedPosition.z * _leanScale / TORSO_LENGTH)),
        -MAX_LEAN, MAX_LEAN));
}

float Avatar::getAbsoluteHeadYaw() const {
    return glm::yaw(_head.getOrientation());
}

float Avatar::getAbsoluteHeadPitch() const {
    return glm::pitch(_head.getOrientation());
}

glm::quat Avatar::getOrientation() const {
    return glm::quat(glm::radians(glm::vec3(_bodyPitch, _bodyYaw, _bodyRoll)));
}

glm::quat Avatar::getWorldAlignedOrientation () const {
    return computeRotationFromBodyToWorldUp() * getOrientation();
}

glm::vec3 Avatar::getUprightHeadPosition() const {
    return _position + getWorldAlignedOrientation() * glm::vec3(0.0f, _pelvisToHeadLength, 0.0f);
}

void  Avatar::updateFromMouse(int mouseX, int mouseY, int screenWidth, int screenHeight) {
    //  Update head yaw and pitch based on mouse input
    const float MOUSE_ROTATE_SPEED = 0.01f;
    const float MOUSE_PITCH_SPEED = 0.02f;
    const int TITLE_BAR_HEIGHT = 46;
    
    if ((mouseX > 1) && (mouseX < screenWidth) && (mouseY > TITLE_BAR_HEIGHT) && (mouseY < screenHeight)) {
        //
        //  Mouse must be inside screen (not at edge) and not on title bar for movement to happen
        //
        
        int pixelMoveThreshold = screenWidth / 6;
        glm::vec2 mouseVector(mouseX - (screenWidth / 2), mouseY - (screenHeight / 2));
        if (glm::length(mouseVector) > pixelMoveThreshold) {
            mouseVector -= glm::normalize(mouseVector) * (float) pixelMoveThreshold;
            _head.addYaw(-mouseVector.x * MOUSE_ROTATE_SPEED);
            _head.addPitch(-mouseVector.y * MOUSE_PITCH_SPEED);
        }
    }
}

void  Avatar::updateFromTouch(float touchAvgDistX, float touchAvgDistY) {
    const float TOUCH_ROTATE_SPEED = 0.01f;
    const float TOUCH_PITCH_SPEED = 0.02f;
    
    _head.addYaw(-touchAvgDistX * TOUCH_ROTATE_SPEED);
    _head.addPitch(-touchAvgDistY * TOUCH_PITCH_SPEED);
}

void Avatar::updateThrust(float deltaTime, Transmitter * transmitter) {
    //
    //  Gather thrust information from keyboard and sensors to apply to avatar motion 
    //
    glm::quat orientation = getOrientation();
    glm::vec3 front = orientation * IDENTITY_FRONT;
    glm::vec3 right = orientation * IDENTITY_RIGHT;
    glm::vec3 up = orientation * IDENTITY_UP;

    const float THRUST_MAG_UP = 800.0f;
    const float THRUST_MAG_DOWN = 200.f;
    const float THRUST_MAG_FWD = 300.f;
    const float THRUST_MAG_BACK = 150.f;
    const float THRUST_MAG_LATERAL = 200.f;
    const float THRUST_JUMP = 120.f;
    
    //  Add Thrusts from keyboard
    if (_driveKeys[FWD      ]) {_thrust       += THRUST_MAG_FWD * deltaTime * front;}
    if (_driveKeys[BACK     ]) {_thrust       -= THRUST_MAG_BACK * deltaTime * front;}
    if (_driveKeys[RIGHT    ]) {_thrust       += THRUST_MAG_LATERAL * deltaTime * right;}
    if (_driveKeys[LEFT     ]) {_thrust       -= THRUST_MAG_LATERAL * deltaTime * right;}
    if (_driveKeys[UP       ]) {_thrust       += THRUST_MAG_UP * deltaTime * up;}
    if (_driveKeys[DOWN     ]) {_thrust       -= THRUST_MAG_DOWN * deltaTime * up;}
    if (_driveKeys[ROT_RIGHT]) {_bodyYawDelta -= YAW_MAG    * deltaTime;}
    if (_driveKeys[ROT_LEFT ]) {_bodyYawDelta += YAW_MAG    * deltaTime;}
    
    //  Add one time jumping force if requested
    if (_shouldJump) {
        _thrust += THRUST_JUMP * up;
        _shouldJump = false;
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
}
    
void Avatar::simulate(float deltaTime, Transmitter* transmitter) {
    
    glm::quat orientation = getOrientation();
    glm::vec3 front = orientation * IDENTITY_FRONT;
    glm::vec3 right = orientation * IDENTITY_RIGHT;

    // Update movement timers
    if (!_owningAgent) {
        _elapsedTimeSinceCollision += deltaTime;
        const float VELOCITY_MOVEMENT_TIMER_THRESHOLD = 0.2f;
        if (glm::length(_velocity) < VELOCITY_MOVEMENT_TIMER_THRESHOLD) {
            _elapsedTimeMoving = 0.f;
            _elapsedTimeStopped += deltaTime;
        } else {
            _elapsedTimeStopped = 0.f;
            _elapsedTimeMoving += deltaTime;
        }
    }
    
    //  Collect thrust forces from keyboard and devices 
    if (!_owningAgent) {
        updateThrust(deltaTime, transmitter);
    }
    
    // copy velocity so we can use it later for acceleration
    glm::vec3 oldVelocity = getVelocity();
    
    if (!_owningAgent) {
        // update position by velocity
        _position += _velocity * deltaTime;
        
        // calculate speed
        _speed = glm::length(_velocity);
    }
    
    //figure out if the mouse cursor is over any body spheres...
    if (!_owningAgent) {
        checkForMouseRayTouching();
    }

    // update balls
    if (_balls) { _balls->simulate(deltaTime); }
    
    // update torso rotation based on head lean
    _skeleton.joint[AVATAR_JOINT_TORSO].rotation = glm::quat(glm::radians(glm::vec3(
        _head.getLeanForward(), 0.0f, _head.getLeanSideways())));
    
	// update avatar skeleton
    _skeleton.update(deltaTime, getOrientation(), _position);
    
    //determine the lengths of the body springs now that we have updated the skeleton at least once
    if (!_ballSpringsInitialized) {
        for (int b = 0; b < NUM_AVATAR_BODY_BALLS; b++) {
            
            glm::vec3 targetPosition
            = _skeleton.joint[_bodyBall[b].parentJoint].position
            + _skeleton.joint[_bodyBall[b].parentJoint].rotation * _bodyBall[b].parentOffset;
            
            glm::vec3 parentTargetPosition
            = _skeleton.joint[_bodyBall[b].parentJoint].position
            + _skeleton.joint[_bodyBall[b].parentJoint].rotation * _bodyBall[b].parentOffset;
            
            _bodyBall[b].springLength = glm::length(targetPosition - parentTargetPosition);
        }
        
        _ballSpringsInitialized = true;
    }
    
    
    // if this is not my avatar, then hand position comes from transmitted data
    if (_owningAgent) {
        _skeleton.joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position = _handPosition;
    }
    
    //detect and respond to collisions with other avatars...
    if (!_owningAgent) {
        updateAvatarCollisions(deltaTime);
    }
    
    //update the movement of the hand and process handshaking with other avatars...
    updateHandMovementAndTouching(deltaTime);
    _avatarTouch.simulate(deltaTime);
    
    // apply gravity and collision with the ground/floor
    if (!_owningAgent && USING_AVATAR_GRAVITY) {
        _velocity += _gravity * (GRAVITY_EARTH * deltaTime);
    }
    if (!_owningAgent) {
        updateCollisionWithEnvironment();
    }
    
    // update body balls
    updateBodyBalls(deltaTime);
    
    // test for avatar collision response with the big sphere
    if (usingBigSphereCollisionTest) {
        updateCollisionWithSphere(_TEST_bigSpherePosition, _TEST_bigSphereRadius, deltaTime);
    }
    
    // collision response with voxels
    if (!_owningAgent) {
        updateCollisionWithVoxels();
    }
    
    if (!_owningAgent) {
    
        // add thrust to velocity
        _velocity += _thrust * deltaTime;

        // update body yaw by body yaw delta
        orientation = orientation * glm::quat(glm::radians(
                                                           glm::vec3(_bodyPitchDelta, _bodyYawDelta, _bodyRollDelta) * deltaTime));
        
        // decay body rotation momentum
        float bodySpinMomentum = 1.0 - BODY_SPIN_FRICTION * deltaTime;
        if  (bodySpinMomentum < 0.0f) { bodySpinMomentum = 0.0f; }
        _bodyPitchDelta *= bodySpinMomentum;
        _bodyYawDelta   *= bodySpinMomentum;
        _bodyRollDelta  *= bodySpinMomentum;
        
        // Decay velocity.  If velocity is really low, increase decay to simulate static friction
        const float VELOCITY_DECAY_UNDER_THRUST = 0.2;
        const float VELOCITY_FAST_DECAY = 0.6;
        const float VELOCITY_SLOW_DECAY = 3.0;
        const float VELOCITY_FAST_THRESHOLD = 2.0f;
        float decayConstant, decay;
        if (glm::length(_thrust) > 0.f) {
            decayConstant = VELOCITY_DECAY_UNDER_THRUST;
        } else if (glm::length(_velocity) > VELOCITY_FAST_THRESHOLD) {
            decayConstant = VELOCITY_FAST_DECAY;
        } else {
            decayConstant = VELOCITY_SLOW_DECAY;
        }             
        decay = glm::clamp(1.0f - decayConstant * deltaTime, 0.0f, 1.0f);
        _velocity *= decay;
                
        //pitch and roll the body as a function of forward speed and turning delta
        const float BODY_PITCH_WHILE_WALKING      = -20.0;
        const float BODY_ROLL_WHILE_TURNING       = 0.2;
        float forwardComponentOfVelocity = glm::dot(getBodyFrontDirection(), _velocity);
        orientation = orientation * glm::quat(glm::radians(glm::vec3(
                                                                     BODY_PITCH_WHILE_WALKING * deltaTime * forwardComponentOfVelocity, 0.0f,
                                                                     BODY_ROLL_WHILE_TURNING  * deltaTime * _speed * _bodyYawDelta)));
        
        // these forces keep the body upright...
        const float BODY_UPRIGHT_FORCE = 10.0;
        float tiltDecay = BODY_UPRIGHT_FORCE * deltaTime;
        if  (tiltDecay > 1.0f) {tiltDecay = 1.0f;}
        
        // update the euler angles
        setOrientation(orientation);
        
        //the following will be used to make the avatar upright no matter what gravity is
        setOrientation(computeRotationFromBodyToWorldUp(tiltDecay) * orientation);
        
        //  Compute instantaneous acceleration
        float forwardAcceleration = glm::length(glm::dot(getBodyFrontDirection(), getVelocity() - oldVelocity)) / deltaTime;
        const float ACCELERATION_PITCH_DECAY = 0.4f;
        const float ACCELERATION_YAW_DECAY = 0.4f;
        const float ACCELERATION_PULL_THRESHOLD = 0.2f;
        const float OCULUS_ACCELERATION_PULL_THRESHOLD = 1.0f;
        const int OCULUS_YAW_OFFSET_THRESHOLD = 10;
        
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
    
    //apply the head lean values to the ball positions...
    if (USING_HEAD_LEAN) {
        if (fabs(_head.getLeanSideways() + _head.getLeanForward()) > 0.0f) {
            glm::vec3 headLean =
            right * _head.getLeanSideways() +
            front * _head.getLeanForward();
            
            _bodyBall[ BODY_BALL_TORSO            ].position += headLean * 0.1f;
            _bodyBall[ BODY_BALL_CHEST            ].position += headLean * 0.4f;
            _bodyBall[ BODY_BALL_NECK_BASE        ].position += headLean * 0.7f;
            _bodyBall[ BODY_BALL_HEAD_BASE        ].position += headLean * 1.0f;
            
            _bodyBall[ BODY_BALL_LEFT_COLLAR      ].position += headLean * 0.6f;
            _bodyBall[ BODY_BALL_LEFT_SHOULDER    ].position += headLean * 0.6f;
            _bodyBall[ BODY_BALL_LEFT_ELBOW       ].position += headLean * 0.2f;
            _bodyBall[ BODY_BALL_LEFT_WRIST       ].position += headLean * 0.1f;
            _bodyBall[ BODY_BALL_LEFT_FINGERTIPS  ].position += headLean * 0.0f;
            
            _bodyBall[ BODY_BALL_RIGHT_COLLAR     ].position += headLean * 0.6f;
            _bodyBall[ BODY_BALL_RIGHT_SHOULDER   ].position += headLean * 0.6f;
            _bodyBall[ BODY_BALL_RIGHT_ELBOW      ].position += headLean * 0.2f;
            _bodyBall[ BODY_BALL_RIGHT_WRIST      ].position += headLean * 0.1f;
            _bodyBall[ BODY_BALL_RIGHT_FINGERTIPS ].position += headLean * 0.0f;
        }
    }
    
    _head.setBodyRotation   (glm::vec3(_bodyPitch, _bodyYaw, _bodyRoll));
    _head.setPosition(_bodyBall[ BODY_BALL_HEAD_BASE ].position);
    _head.setScale   (_bodyBall[ BODY_BALL_HEAD_BASE ].radius);
    _head.setSkinColor(glm::vec3(SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]));
    _head.simulate(deltaTime, !_owningAgent);
    
    // use speed and angular velocity to determine walking vs. standing
    if (_speed + fabs(_bodyYawDelta) > 0.2) {
        _mode = AVATAR_MODE_WALKING;
    } else {
        _mode = AVATAR_MODE_INTERACTING;
    }
    
    // Zero thrust out now that we've added it to velocity in this frame
    _thrust = glm::vec3(0, 0, 0);

}

void Avatar::checkForMouseRayTouching() {
    
    for (int b = 0; b < NUM_AVATAR_BODY_BALLS; b++) {
        
        glm::vec3 directionToBodySphere = glm::normalize(_bodyBall[b].position - _mouseRayOrigin);
        float dot = glm::dot(directionToBodySphere, _mouseRayDirection);
        
        float range = _bodyBall[b].radius * MOUSE_RAY_TOUCH_RANGE;
        
        if (dot > (1.0f - range)) {
            _bodyBall[b].touchForce = (dot - (1.0f - range)) / range;
        } else {
            _bodyBall[b].touchForce = 0.0;
        }
    }
}

void Avatar::setMouseRay(const glm::vec3 &origin, const glm::vec3 &direction ) {
    _mouseRayOrigin = origin;
    _mouseRayDirection = direction;
}

void Avatar::setOrientation(const glm::quat& orientation) {
    glm::vec3 eulerAngles = safeEulerAngles(orientation);
    _bodyPitch = eulerAngles.x;
    _bodyYaw = eulerAngles.y;
    _bodyRoll = eulerAngles.z;
}

void Avatar::updateHandMovementAndTouching(float deltaTime) {
    
    glm::quat orientation = getOrientation();
    
    // reset hand and arm positions according to hand movement
    glm::vec3 right = orientation * IDENTITY_RIGHT;
    glm::vec3 up    = orientation * IDENTITY_UP;
    glm::vec3 front = orientation * IDENTITY_FRONT;
    
    glm::vec3 transformedHandMovement
    = right *  _movedHandOffset.x * 2.0f
    + up	* -_movedHandOffset.y * 2.0f
    + front * -_movedHandOffset.y * 2.0f;
    
    _skeleton.joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position += transformedHandMovement;
    
    if (!_owningAgent) {
        _avatarTouch.setMyBodyPosition(_position);
        _avatarTouch.setMyOrientation(orientation);
        
        float closestDistance = std::numeric_limits<float>::max();
        
        _interactingOther = NULL;
        
        //loop through all the other avatars for potential interactions...
        AgentList* agentList = AgentList::getInstance();
        for (AgentList::iterator agent = agentList->begin(); agent != agentList->end(); agent++) {
            if (agent->getLinkedData() && agent->getType() == AGENT_TYPE_AVATAR) {
                Avatar *otherAvatar = (Avatar *)agent->getLinkedData();
                
                // test whether shoulders are close enough to allow for reaching to touch hands
                glm::vec3 v(_position - otherAvatar->_position);
                float distance = glm::length(v);
                if (distance < closestDistance) {
                    closestDistance = distance;
                    
                    if (distance < PERIPERSONAL_RADIUS) {
                        _interactingOther = otherAvatar;
                    }
                }
            }
        }
        
        if (_interactingOther) {
            
            _avatarTouch.setHasInteractingOther(true);
            _avatarTouch.setYourBodyPosition(_interactingOther->_position);
            _avatarTouch.setYourHandPosition(_interactingOther->_bodyBall[ BODY_BALL_RIGHT_FINGERTIPS ].position);
            _avatarTouch.setYourOrientation (_interactingOther->getOrientation());
            _avatarTouch.setYourHandState   (_interactingOther->_handState);
            
            //if hand-holding is initiated by either avatar, turn on hand-holding...
            if (_avatarTouch.getHandsCloseEnoughToGrasp()) {
                if ((_handState == HAND_STATE_GRASPING ) || (_interactingOther->_handState == HAND_STATE_GRASPING)) {
                    if (!_avatarTouch.getHoldingHands())
                    {
                        _avatarTouch.setHoldingHands(true);
                    }
                }
            }
            
            glm::vec3 vectorFromMyHandToYourHand
            (
             _interactingOther->_skeleton.joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position -
             _skeleton.joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position
             );
            
            float distanceBetweenOurHands = glm::length(vectorFromMyHandToYourHand);
            
            /*
             // if my arm can no longer reach the other hand, turn off hand-holding
             if (!_avatarTouch.getAbleToReachOtherAvatar()) {
             _avatarTouch.setHoldingHands(false);
             }
             if (distanceBetweenOurHands > _maxArmLength) {
             _avatarTouch.setHoldingHands(false);
             }
             */
            
            // if neither of us are grasping, turn off hand-holding
            if ((_handState != HAND_STATE_GRASPING ) && (_interactingOther->_handState != HAND_STATE_GRASPING)) {
                _avatarTouch.setHoldingHands(false);
            }
            
            //if holding hands, apply the appropriate forces
            if (_avatarTouch.getHoldingHands()) {
                _skeleton.joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position +=
                (
                 _interactingOther->_skeleton.joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position
                 - _skeleton.joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position
                 ) * 0.5f;
                
                if (distanceBetweenOurHands > 0.3) {
                    float force = 10.0f * deltaTime;
                    if (force > 1.0f) {force = 1.0f;}
                    _velocity += vectorFromMyHandToYourHand * force;
                }
            }
        } else {
            _avatarTouch.setHasInteractingOther(false);
        }
    }//if (_isMine)
    
    //constrain right arm length and re-adjust elbow position as it bends
    // NOTE - the following must be called on all avatars - not just _isMine
    updateArmIKAndConstraints(deltaTime);
    
    //Set right hand position and state to be transmitted, and also tell AvatarTouch about it
    if (!_owningAgent) {
        setHandPosition(_skeleton.joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position);
        
        if (_mousePressed) {
            _handState = HAND_STATE_GRASPING;
        } else {
            _handState = HAND_STATE_NULL;
        }
        
        _avatarTouch.setMyHandState(_handState);
        _avatarTouch.setMyHandPosition(_bodyBall[ BODY_BALL_RIGHT_FINGERTIPS ].position);
    }
}

void Avatar::updateCollisionWithSphere(glm::vec3 position, float radius, float deltaTime) {
    float myBodyApproximateBoundingRadius = 1.0f;
    glm::vec3 vectorFromMyBodyToBigSphere(_position - position);
    
    float distanceToBigSphere = glm::length(vectorFromMyBodyToBigSphere);
    if (distanceToBigSphere < myBodyApproximateBoundingRadius + radius) {
        for (int b = 0; b < NUM_AVATAR_BODY_BALLS; b++) {
            glm::vec3 vectorFromBallToBigSphereCenter(_bodyBall[b].position - position);
            float distanceToBigSphereCenter = glm::length(vectorFromBallToBigSphereCenter);
            float combinedRadius = _bodyBall[b].radius + radius;
            
            if (distanceToBigSphereCenter < combinedRadius)  {
                if (distanceToBigSphereCenter > 0.0) {
                    glm::vec3 directionVector = vectorFromBallToBigSphereCenter / distanceToBigSphereCenter;
                    
                    float penetration = 1.0 - (distanceToBigSphereCenter / combinedRadius);
                    glm::vec3 collisionForce = vectorFromBallToBigSphereCenter * penetration;
                    
                    _velocity += collisionForce * 40.0f * deltaTime;
                    _bodyBall[b].position  = position + directionVector * combinedRadius;
                }
            }
        }
    }
}

void Avatar::updateCollisionWithEnvironment() {
    
    glm::vec3 up = getBodyUpDirection();
    float radius = _height * 0.125f;
    const float ENVIRONMENT_SURFACE_ELASTICITY = 1.0f;
    const float ENVIRONMENT_SURFACE_DAMPING = 0.01;
    glm::vec3 penetration;
    if (Application::getInstance()->getEnvironment()->findCapsulePenetration(
                                                                             _position - up * (_pelvisFloatingHeight - radius),
                                                                             _position + up * (_height - _pelvisFloatingHeight - radius), radius, penetration)) {
        applyHardCollision(penetration, ENVIRONMENT_SURFACE_ELASTICITY, ENVIRONMENT_SURFACE_DAMPING);
    }
}


void Avatar::updateCollisionWithVoxels() {
    float radius = _height * 0.125f;
    const float VOXEL_ELASTICITY = 1.4f;
    const float VOXEL_DAMPING = 0.0;
    glm::vec3 penetration;
    if (Application::getInstance()->getVoxels()->findCapsulePenetration(
                                                                        _position - glm::vec3(0.0f, _pelvisFloatingHeight - radius, 0.0f),
                                                                        _position + glm::vec3(0.0f, _height - _pelvisFloatingHeight - radius, 0.0f), radius, penetration)) {
        applyHardCollision(penetration, VOXEL_ELASTICITY, VOXEL_DAMPING);
    }
}

void Avatar::applyHardCollision(const glm::vec3& penetration, float elasticity, float damping) {
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

void Avatar::updateAvatarCollisions(float deltaTime) {
    
    //  Reset detector for nearest avatar
    _distanceToNearestAvatar = std::numeric_limits<float>::max();
    
    // loop through all the other avatars for potential interactions...
    AgentList* agentList = AgentList::getInstance();
    for (AgentList::iterator agent = agentList->begin(); agent != agentList->end(); agent++) {
        if (agent->getLinkedData() && agent->getType() == AGENT_TYPE_AVATAR) {
            Avatar *otherAvatar = (Avatar *)agent->getLinkedData();
            
            // check if the bounding spheres of the two avatars are colliding
            glm::vec3 vectorBetweenBoundingSpheres(_position - otherAvatar->_position);
            
            if (glm::length(vectorBetweenBoundingSpheres) < _height * ONE_HALF + otherAvatar->_height * ONE_HALF) {
                // apply forces from collision
                applyCollisionWithOtherAvatar(otherAvatar, deltaTime);
            }
            // test other avatar hand position for proximity
            glm::vec3 v(_skeleton.joint[ AVATAR_JOINT_RIGHT_SHOULDER ].position);
            v -= otherAvatar->getPosition();
            
            float distance = glm::length(v);
            if (distance < _distanceToNearestAvatar) {
                _distanceToNearestAvatar = distance;
            }
        }
    }
}

// detect collisions with other avatars and respond
void Avatar::applyCollisionWithOtherAvatar(Avatar * otherAvatar, float deltaTime) {
    
    glm::vec3 bodyPushForce = glm::vec3(0.0f, 0.0f, 0.0f);
    
    // loop through the body balls of each avatar to check for every possible collision
    for (int b = 1; b < NUM_AVATAR_BODY_BALLS; b++) {
        if (_bodyBall[b].isCollidable) {
            
            for (int o = b+1; o < NUM_AVATAR_BODY_BALLS; o++) {
                if (otherAvatar->_bodyBall[o].isCollidable) {
                    
                    glm::vec3 vectorBetweenBalls(_bodyBall[b].position - otherAvatar->_bodyBall[o].position);
                    float distanceBetweenBalls = glm::length(vectorBetweenBalls);
                    
                    if (distanceBetweenBalls > 0.0) { // to avoid divide by zero
                        float combinedRadius = _bodyBall[b].radius + otherAvatar->_bodyBall[o].radius;
                        
                        // check for collision
                        if (distanceBetweenBalls < combinedRadius * COLLISION_RADIUS_SCALAR)  {
                            glm::vec3 directionVector = vectorBetweenBalls / distanceBetweenBalls;
                            
                            // push balls away from each other and apply friction
                            float penetration = 1.0f - (distanceBetweenBalls / (combinedRadius * COLLISION_RADIUS_SCALAR));
                            
                            glm::vec3 ballPushForce = directionVector * COLLISION_BALL_FORCE * penetration * deltaTime;
                            bodyPushForce +=          directionVector * COLLISION_BODY_FORCE * penetration * deltaTime;
                            
                            _bodyBall[b].velocity += ballPushForce;
                            otherAvatar->_bodyBall[o].velocity -= ballPushForce;
                            
                        }// check for collision
                    }   // to avoid divide by zero
                }      // o loop
            }         // collidable
        }            // b loop
    }               // collidable
    
    // apply force on the whole body
    _velocity += bodyPushForce;
}


static TextRenderer* textRenderer() {
    static TextRenderer* renderer = new TextRenderer(SANS_FONT_FAMILY, 24, -1, false, TextRenderer::SHADOW_EFFECT);
    return renderer;
}

void Avatar::setGravity(glm::vec3 gravity) {
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

void Avatar::render(bool lookingInMirror, bool renderAvatarBalls) {

    if (!_owningAgent && usingBigSphereCollisionTest) {
        // show TEST big sphere
        glColor4f(0.5f, 0.6f, 0.8f, 0.7);
        glPushMatrix();
        glTranslatef(_TEST_bigSpherePosition.x, _TEST_bigSpherePosition.y, _TEST_bigSpherePosition.z);
        glScalef(_TEST_bigSphereRadius, _TEST_bigSphereRadius, _TEST_bigSphereRadius);
        glutSolidSphere(1, 20, 20);
        glPopMatrix();
    }
    
    // render a simple round on the ground projected down from the avatar's position
    renderDiskShadow(_position, glm::vec3(0.0f, 1.0f, 0.0f), 0.1f, 0.2f);
    
    // render body
    renderBody(lookingInMirror, renderAvatarBalls);
    
    // if this is my avatar, then render my interactions with the other avatar
    if (!_owningAgent) {
        _avatarTouch.render(Application::getInstance()->getCamera()->getPosition());
    }
    
    //  Render the balls
    if (_balls) {
        glPushMatrix();
        glTranslatef(_position.x, _position.y, _position.z);
        _balls->render();
        glPopMatrix();
    }
    
    if (!_chatMessage.empty()) {
        int width = 0;
        int lastWidth;
        for (string::iterator it = _chatMessage.begin(); it != _chatMessage.end(); it++) {
            width += (lastWidth = textRenderer()->computeWidth(*it));
        }
        glPushMatrix();
        
        glm::vec3 chatPosition = _bodyBall[BODY_BALL_HEAD_BASE].position + getBodyUpDirection() * chatMessageHeight;
        glTranslatef(chatPosition.x, chatPosition.y, chatPosition.z);
        glm::quat chatRotation = Application::getInstance()->getCamera()->getRotation();
        glm::vec3 chatAxis = glm::axis(chatRotation);
        glRotatef(glm::angle(chatRotation), chatAxis.x, chatAxis.y, chatAxis.z);
        
        
        glColor3f(0, 0.8, 0);
        glRotatef(180, 0, 1, 0);
        glRotatef(180, 0, 0, 1);
        glScalef(chatMessageScale, chatMessageScale, 1.0f);
        
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

void Avatar::resetBodyBalls() {
    for (int b = 0; b < NUM_AVATAR_BODY_BALLS; b++) {
        
        glm::vec3 targetPosition
        = _skeleton.joint[_bodyBall[b].parentJoint].position
        + _skeleton.joint[_bodyBall[b].parentJoint].rotation * _bodyBall[b].parentOffset;
        
        _bodyBall[b].position = targetPosition; // put ball on target position
        _bodyBall[b].velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    }
}

void Avatar::updateBodyBalls(float deltaTime) {
    // Check for a large repositioning, and re-initialize balls if this has happened
    const float BEYOND_BODY_SPRING_RANGE = 2.f;
    if (glm::length(_position - _bodyBall[BODY_BALL_PELVIS].position) > BEYOND_BODY_SPRING_RANGE) {
        resetBodyBalls();
    }
    glm::quat orientation = getOrientation();
    for (int b = 0; b < NUM_AVATAR_BODY_BALLS; b++) {
        
        glm::vec3 springVector;
        float length = 0.0f;
        if (_ballSpringsInitialized) {
            
            // apply spring forces
            springVector = _bodyBall[b].position;
            
            if (b == BODY_BALL_PELVIS) {
                springVector -= _position;
            } else {
                springVector -= _bodyBall[_bodyBall[b].parentBall].position;
            }
            
            length = glm::length(springVector);
            
            if (length > 0.0f) { // to avoid divide by zero
                glm::vec3 springDirection = springVector / length;
                
                float force = (length - _skeleton.joint[b].length) * BODY_SPRING_FORCE * deltaTime;
                _bodyBall[b].velocity -= springDirection * force;
                
                if (_bodyBall[b].parentBall != BODY_BALL_NULL) {
                    _bodyBall[_bodyBall[b].parentBall].velocity += springDirection * force;
                }
            }
        }
        
        // apply tightness force - (causing ball position to be close to skeleton joint position)
        glm::vec3 targetPosition
        = _skeleton.joint[_bodyBall[b].parentJoint].position
        + _skeleton.joint[_bodyBall[b].parentJoint].rotation * _bodyBall[b].parentOffset;
        
        _bodyBall[b].velocity += (targetPosition - _bodyBall[b].position) * _bodyBall[b].jointTightness * deltaTime;
        
        // apply decay
        float decay = 1.0 - BODY_SPRING_DECAY * deltaTime;
        if (decay > 0.0) {
            _bodyBall[b].velocity *= decay;
        } else {
            _bodyBall[b].velocity = glm::vec3(0.0f, 0.0f, 0.0f);
        }
        
        /*
         // apply forces from touch...
         if (_bodyBall[b].touchForce > 0.0) {
         _bodyBall[b].velocity += _mouseRayDirection * _bodyBall[b].touchForce * 0.7f;
         }
         */
        
        // update position by velocity...
        _bodyBall[b].position += _bodyBall[b].velocity * deltaTime;
        
        // update rotation
        const float SMALL_SPRING_LENGTH = 0.001f; // too-small springs can change direction rapidly
        if (_skeleton.joint[b].parent == AVATAR_JOINT_NULL || length < SMALL_SPRING_LENGTH) {
            _bodyBall[b].rotation = orientation * _skeleton.joint[_bodyBall[b].parentJoint].absoluteBindPoseRotation;
        } else {
            glm::vec3 parentDirection = _bodyBall[ _bodyBall[b].parentBall ].rotation * JOINT_DIRECTION;
            _bodyBall[b].rotation = rotationBetween(parentDirection, springVector) *
                _bodyBall[ _bodyBall[b].parentBall ].rotation;
        }
    }
    
    // copy the head's rotation
    _bodyBall[BODY_BALL_HEAD_BASE].rotation = _bodyBall[BODY_BALL_HEAD_TOP].rotation = _head.getOrientation();
    _bodyBall[BODY_BALL_HEAD_BASE].position = _bodyBall[BODY_BALL_NECK_BASE].position +
        _bodyBall[BODY_BALL_HEAD_BASE].rotation * _skeleton.joint[BODY_BALL_HEAD_BASE].bindPosePosition;
    _bodyBall[BODY_BALL_HEAD_TOP].position = _bodyBall[BODY_BALL_HEAD_BASE].position +
        _bodyBall[BODY_BALL_HEAD_TOP].rotation * _skeleton.joint[BODY_BALL_HEAD_TOP].bindPosePosition;
}

void Avatar::updateArmIKAndConstraints(float deltaTime) {
    
    // determine the arm vector
    glm::vec3 armVector = _skeleton.joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position;
    armVector -= _skeleton.joint[ AVATAR_JOINT_RIGHT_SHOULDER ].position;
    
    // test to see if right hand is being dragged beyond maximum arm length
    float distance = glm::length(armVector);
    
    // don't let right hand get dragged beyond maximum arm length...
    if (distance > _maxArmLength) {
        // reset right hand to be constrained to maximum arm length
        _skeleton.joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position = _skeleton.joint[ AVATAR_JOINT_RIGHT_SHOULDER ].position;
        glm::vec3 armNormal = armVector / distance;
        armVector = armNormal * _maxArmLength;
        distance = _maxArmLength;
        glm::vec3 constrainedPosition = _skeleton.joint[ AVATAR_JOINT_RIGHT_SHOULDER ].position;
        constrainedPosition += armVector;
        _skeleton.joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position = constrainedPosition;
    }
    
    // set elbow position
    glm::vec3 newElbowPosition = _skeleton.joint[ AVATAR_JOINT_RIGHT_SHOULDER ].position + armVector * ONE_HALF;
    
    glm::vec3 perpendicular = glm::cross(getBodyRightDirection(),  armVector);
    
    newElbowPosition += perpendicular * (1.0f - (_maxArmLength / distance)) * ONE_HALF;
    _skeleton.joint[ AVATAR_JOINT_RIGHT_ELBOW ].position = newElbowPosition;
    
    // set wrist position
    glm::vec3 vv(_skeleton.joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position);
    vv -= _skeleton.joint[ AVATAR_JOINT_RIGHT_ELBOW ].position;
    glm::vec3 newWristPosition = _skeleton.joint[ AVATAR_JOINT_RIGHT_ELBOW ].position + vv * 0.7f;
    _skeleton.joint[ AVATAR_JOINT_RIGHT_WRIST ].position = newWristPosition;
}

glm::quat Avatar::computeRotationFromBodyToWorldUp(float proportion) const {
    glm::quat orientation = getOrientation();
    glm::vec3 currentUp = orientation * IDENTITY_UP;
    float angle = glm::degrees(acosf(glm::clamp(glm::dot(currentUp, _worldUpDirection), -1.0f, 1.0f)));
    if (angle < EPSILON) {
        return glm::quat();
    }
    glm::vec3 axis;
    if (angle > 179.99f) { // 180 degree rotation; must use another axis
        axis = orientation * IDENTITY_RIGHT;
    } else {
        axis = glm::normalize(glm::cross(currentUp, _worldUpDirection));
    }
    return glm::angleAxis(angle * proportion, axis);
}

float Avatar::getBallRenderAlpha(int ball, bool lookingInMirror) const {
    const float RENDER_OPAQUE_OUTSIDE = 1.25f; // render opaque if greater than this distance
    const float DO_NOT_RENDER_INSIDE = 0.75f; // do not render if less than this distance
    float distanceToCamera = glm::length(Application::getInstance()->getCamera()->getPosition() - _bodyBall[ball].position);
    return (lookingInMirror || _owningAgent) ? 1.0f : glm::clamp(
        (distanceToCamera - DO_NOT_RENDER_INSIDE) / (RENDER_OPAQUE_OUTSIDE - DO_NOT_RENDER_INSIDE), 0.f, 1.f);
}

void Avatar::renderBody(bool lookingInMirror, bool renderAvatarBalls) {
    
    //  Render the body as balls and cones
    if (renderAvatarBalls || !_voxels.getVoxelURL().isValid()) {
        for (int b = 0; b < NUM_AVATAR_BODY_BALLS; b++) {
            float alpha = getBallRenderAlpha(b, lookingInMirror);
            
            //  Always render other people, and render myself when beyond threshold distance
            if (b == BODY_BALL_HEAD_BASE) { // the head is rendered as a special
                if (alpha > 0.0f) {
                    _head.render(lookingInMirror, alpha);
                }
            } else if (alpha > 0.0f) {
                //  Render the body ball sphere
                if (_owningAgent || b == BODY_BALL_RIGHT_ELBOW
                    || b == BODY_BALL_RIGHT_WRIST
                    || b == BODY_BALL_RIGHT_FINGERTIPS ) {
                    glColor3f(SKIN_COLOR[0] + _bodyBall[b].touchForce * 0.3f,
                              SKIN_COLOR[1] - _bodyBall[b].touchForce * 0.2f,
                              SKIN_COLOR[2] - _bodyBall[b].touchForce * 0.1f);
                } else {
                    glColor4f(SKIN_COLOR[0] + _bodyBall[b].touchForce * 0.3f,
                              SKIN_COLOR[1] - _bodyBall[b].touchForce * 0.2f,
                              SKIN_COLOR[2] - _bodyBall[b].touchForce * 0.1f,
                              alpha);
                }
                
                if ((b != BODY_BALL_HEAD_TOP  )
                    &&  (b != BODY_BALL_HEAD_BASE )) {
                    glPushMatrix();
                    glTranslatef(_bodyBall[b].position.x, _bodyBall[b].position.y, _bodyBall[b].position.z);
                    glutSolidSphere(_bodyBall[b].radius, 20.0f, 20.0f);
                    glPopMatrix();
                }
                
                //  Render the cone connecting this ball to its parent
                if (_bodyBall[b].parentBall != BODY_BALL_NULL) {
                    if ((b != BODY_BALL_HEAD_TOP      )
                        &&  (b != BODY_BALL_HEAD_BASE     )
                        &&  (b != BODY_BALL_PELVIS        )
                        &&  (b != BODY_BALL_TORSO         )
                        &&  (b != BODY_BALL_CHEST         )
                        &&  (b != BODY_BALL_LEFT_COLLAR   )
                        &&  (b != BODY_BALL_LEFT_SHOULDER )
                        &&  (b != BODY_BALL_RIGHT_COLLAR  )
                        &&  (b != BODY_BALL_RIGHT_SHOULDER)) {
                        glColor3fv(DARK_SKIN_COLOR);
                        
                        float r1 = _bodyBall[_bodyBall[b].parentBall ].radius * 0.8;
                        float r2 = _bodyBall[b].radius * 0.8;
                        if (b == BODY_BALL_HEAD_BASE) {
                            r1 *= 0.5f;
                        }
                        renderJointConnectingCone
                        (
                         _bodyBall[_bodyBall[b].parentBall].position,
                         _bodyBall[b].position, r2, r2
                         );
                    }
                }
            }
        }
    } else {
        //  Render the body's voxels
        float alpha = getBallRenderAlpha(BODY_BALL_HEAD_BASE, lookingInMirror);
        if (alpha > 0.0f) {
            _voxels.render(false);
        }
    }
}


void Avatar::loadData(QSettings* settings) {
    settings->beginGroup("Avatar");

    // in case settings is corrupt or missing loadSetting() will check for NaN
    _bodyYaw    = loadSetting(settings, "bodyYaw"  , 0.0f);
    _bodyPitch  = loadSetting(settings, "bodyPitch", 0.0f);
    _bodyRoll   = loadSetting(settings, "bodyRoll" , 0.0f);
    _position.x = loadSetting(settings, "position_x", 0.0f);
    _position.y = loadSetting(settings, "position_y", 0.0f);
    _position.z = loadSetting(settings, "position_z", 0.0f);
    
    _voxels.setVoxelURL(settings->value("voxelURL").toUrl());
    
    _leanScale = loadSetting(settings, "leanScale", 0.5f);
    
    settings->endGroup();
}

void Avatar::getBodyBallTransform(AvatarJointID jointID, glm::vec3& position, glm::quat& rotation) const {
    position = _bodyBall[jointID].position;
    rotation = _bodyBall[jointID].rotation;
}

void Avatar::saveData(QSettings* set) {
    set->beginGroup("Avatar");
    
    set->setValue("bodyYaw",  _bodyYaw);
    set->setValue("bodyPitch", _bodyPitch);
    set->setValue("bodyRoll",  _bodyRoll);
    
    set->setValue("position_x", _position.x);
    set->setValue("position_y", _position.y);
    set->setValue("position_z", _position.z);
    
    set->setValue("voxelURL", _voxels.getVoxelURL());
    
    set->setValue("leanScale", _leanScale);
    
    set->endGroup();
}

// render a makeshift cone section that serves as a body part connecting joint spheres
void Avatar::renderJointConnectingCone(glm::vec3 position1, glm::vec3 position2, float radius1, float radius2) {
    
    glBegin(GL_TRIANGLES);
    
    glm::vec3 axis = position2 - position1;
    float length = glm::length(axis);
    
    if (length > 0.0f) {
        
        axis /= length;
        
        glm::vec3 perpSin = glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 perpCos = glm::normalize(glm::cross(axis, perpSin));
        perpSin = glm::cross(perpCos, axis);
        
        float anglea = 0.0;
        float angleb = 0.0;
        
        for (int i = 0; i < NUM_BODY_CONE_SIDES; i ++) {
            
            // the rectangles that comprise the sides of the cone section are
            // referenced by "a" and "b" in one dimension, and "1", and "2" in the other dimension.
            anglea = angleb;
            angleb = ((float)(i+1) / (float)NUM_BODY_CONE_SIDES) * PIf * 2.0f;
            
            float sa = sinf(anglea);
            float sb = sinf(angleb);
            float ca = cosf(anglea);
            float cb = cosf(angleb);
            
            glm::vec3 p1a = position1 + perpSin * sa * radius1 + perpCos * ca * radius1;
            glm::vec3 p1b = position1 + perpSin * sb * radius1 + perpCos * cb * radius1; 
            glm::vec3 p2a = position2 + perpSin * sa * radius2 + perpCos * ca * radius2;   
            glm::vec3 p2b = position2 + perpSin * sb * radius2 + perpCos * cb * radius2;  
            
            glVertex3f(p1a.x, p1a.y, p1a.z); 
            glVertex3f(p1b.x, p1b.y, p1b.z); 
            glVertex3f(p2a.x, p2a.y, p2a.z); 
            glVertex3f(p1b.x, p1b.y, p1b.z); 
            glVertex3f(p2a.x, p2a.y, p2a.z); 
            glVertex3f(p2b.x, p2b.y, p2b.z); 
        }
    }
    
    glEnd();
}
