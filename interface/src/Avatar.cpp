//
//  Avatar.cpp
//  interface
//
//  Created by Philip Rosedale on 9/11/12.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>
#include <lodepng.h>
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
const float GRAVITY_SCALE                 = 10.0f;
const float BOUNCE                        = 0.3f;
const float THRUST_MAG                    = 1200.0;
const float YAW_MAG                       = 500.0;
const float BODY_SPIN_FRICTION            = 5.0;
const float BODY_UPRIGHT_FORCE            = 10.0;
const float VELOCITY_DECAY                = 5.0;
const float MY_HAND_HOLDING_PULL          = 0.2;
const float YOUR_HAND_HOLDING_PULL        = 1.0;
const float BODY_SPRING_DEFAULT_TIGHTNESS = 1000.0f;
const float BODY_SPRING_FORCE             = 300.0f;
const float BODY_SPRING_DECAY             = 16.0f;
const float COLLISION_RADIUS_SCALAR       = 1.8;
const float COLLISION_BALL_FORCE          = 1.0;
const float COLLISION_BODY_FORCE          = 6.0;
const float COLLISION_BALL_FRICTION       = 60.0;
const float COLLISION_BODY_FRICTION       = 0.5;
const float HEAD_ROTATION_SCALE           = 0.70;
const float HEAD_ROLL_SCALE               = 0.40;
const float HEAD_MAX_PITCH                = 45;
const float HEAD_MIN_PITCH                = -45;
const float HEAD_MAX_YAW                  = 85;
const float HEAD_MIN_YAW                  = -85;
const float PERIPERSONAL_RADIUS           = 1.0f;
const float AVATAR_BRAKING_STRENGTH       = 40.0f;
const float JOINT_TOUCH_RANGE             = 0.0005f;
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
    _head(this),
    _TEST_bigSphereRadius(0.4f),
    _TEST_bigSpherePosition(5.0f, _TEST_bigSphereRadius, 5.0f),
    _mousePressed(false),
    _bodyPitchDelta(0.0f),
    _bodyYawDelta(0.0f),
    _bodyRollDelta(0.0f),
    _movedHandOffset(0.0f, 0.0f, 0.0f),
    _rotation(0.0f, 0.0f, 0.0f, 0.0f),
    _mode(AVATAR_MODE_STANDING),
    _cameraPosition(0.0f, 0.0f, 0.0f),
    _handHoldingPosition(0.0f, 0.0f, 0.0f),
    _velocity(0.0f, 0.0f, 0.0f),
    _thrust(0.0f, 0.0f, 0.0f),
    _speed(0.0f),
    _maxArmLength(0.0f),
    _pelvisStandingHeight(0.0f),
    _pelvisFloatingHeight(0.0f),
    _distanceToNearestAvatar(std::numeric_limits<float>::max()),
    _gravity(0.0f, -1.0f, 0.0f),
    _worldUpDirection(0.0f, 1.0f, 0.0),
    _mouseRayOrigin(0.0f, 0.0f, 0.0f),
    _mouseRayDirection(0.0f, 0.0f, 0.0f),
    _interactingOther(NULL),
    _cumulativeMouseYaw(0.0f),
    _isMouseTurningRight(false)
{
    
    // give the pointer to our head to inherited _headData variable from AvatarData
    _headData = &_head;

    for (int i = 0; i < MAX_DRIVE_KEYS; i++) {
        _driveKeys[i] = false;
    }

    initializeSkeleton();
    
    _avatarTouch.setReachableRadius(PERIPERSONAL_RADIUS);
        
    if (BALLS_ON) {
        _balls = new Balls(100);
    } else {
        _balls = NULL;
    }
}

Avatar::~Avatar() {
    _headData = NULL;
    delete _balls;
}

void Avatar::reset() {
    _head.reset();
}

//  Update avatar head rotation with sensor data
void Avatar::updateHeadFromGyros(float deltaTime, SerialInterface* serialInterface, glm::vec3* gravity) {
    const float AMPLIFY_PITCH = 2.f;
    const float AMPLIFY_YAW = 2.f;
    const float AMPLIFY_ROLL = 2.f;
    
    float measuredPitchRate = serialInterface->getLastPitchRate();
    float measuredYawRate = serialInterface->getLastYawRate();
    float measuredRollRate = serialInterface->getLastRollRate();
   
    //  Update avatar head position based on measured gyro rates
    
    _head.addPitch(measuredPitchRate * AMPLIFY_PITCH * deltaTime);
    _head.addYaw  (measuredYawRate   * AMPLIFY_YAW * deltaTime);
    _head.addRoll (measuredRollRate  * AMPLIFY_ROLL * deltaTime);
    
    //  Update head lean distance based on accelerometer data
    glm::vec3 headRotationRates(_head.getPitch(), _head.getYaw(), _head.getRoll());
    
    glm::vec3 leaning = (serialInterface->getLastAcceleration() - serialInterface->getGravity())
                        * LEAN_SENSITIVITY
                        * (1.f - fminf(glm::length(headRotationRates), HEAD_RATE_MAX) / HEAD_RATE_MAX);
    leaning.y = 0.f;
    if (glm::length(leaning) < LEAN_MAX) {
        _head.setLeanForward(_head.getLeanForward() * (1.f - LEAN_AVERAGING * deltaTime) +
                             (LEAN_AVERAGING * deltaTime) * leaning.z * LEAN_SENSITIVITY);
        _head.setLeanSideways(_head.getLeanSideways() * (1.f - LEAN_AVERAGING * deltaTime) +
                              (LEAN_AVERAGING * deltaTime) * leaning.x * LEAN_SENSITIVITY);
    }
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

void  Avatar::updateFromMouse(int mouseX, int mouseY, int screenWidth, int screenHeight) {
    //  Update yaw based on mouse behavior
    const float MOUSE_MOVE_RADIUS = 0.15f;
    const float MOUSE_ROTATE_SPEED = 3.0f;
    const float MOUSE_PITCH_SPEED = 1.5f;
    const float MAX_YAW_TO_ADD = 180.f;
    const int TITLE_BAR_HEIGHT = 46;
    float mouseLocationX = (float)mouseX / (float)screenWidth - 0.5f;
    float mouseLocationY = (float)mouseY / (float)screenHeight - 0.5f;

    if ((mouseX > 1) && (mouseX < screenWidth) && (mouseY > TITLE_BAR_HEIGHT) && (mouseY < screenHeight)) {
        //
        //  Mouse must be inside screen (not at edge) and not on title bar for movement to happen
        //
        if (fabs(mouseLocationX) > MOUSE_MOVE_RADIUS) {
            //  Add Yaw
            float mouseYawAdd = (fabs(mouseLocationX) - MOUSE_MOVE_RADIUS) / (0.5f - MOUSE_MOVE_RADIUS) * MOUSE_ROTATE_SPEED;
            bool rightTurning = (mouseLocationX > 0.f);
            if (_isMouseTurningRight == rightTurning) {
                _cumulativeMouseYaw += mouseYawAdd;
            } else {
                _cumulativeMouseYaw = 0;
                _isMouseTurningRight = rightTurning;
            }
            if (_cumulativeMouseYaw < MAX_YAW_TO_ADD) {
                setBodyYaw(getBodyYaw() - (rightTurning ? mouseYawAdd : -mouseYawAdd));
            }
        } else {
            _cumulativeMouseYaw = 0;
        }
        if (fabs(mouseLocationY) > MOUSE_MOVE_RADIUS) {
            float mousePitchAdd = (fabs(mouseLocationY) - MOUSE_MOVE_RADIUS) / (0.5f - MOUSE_MOVE_RADIUS) * MOUSE_PITCH_SPEED;
            bool downPitching = (mouseLocationY > 0.f);
            _head.setPitch(_head.getPitch() + (downPitching ? mousePitchAdd : -mousePitchAdd));
        }
        
    }
    
    return;
}

void Avatar::simulate(float deltaTime, Transmitter* transmitter) {

    //figure out if the mouse cursor is over any body spheres... 
    checkForMouseRayTouching();
    
    // copy velocity so we can use it later for acceleration
    glm::vec3 oldVelocity = getVelocity();
        
    // update balls
    if (_balls) { _balls->simulate(deltaTime); }
    
    // if other avatar, update head position from network data
    
    // update avatar skeleton
    updateSkeleton();
    
    //detect and respond to collisions with other avatars... 
    if (!_owningAgent) {
        updateAvatarCollisions(deltaTime);
    }
    
    //update the movement of the hand and process handshaking with other avatars... 
    updateHandMovementAndTouching(deltaTime);
    
    _avatarTouch.simulate(deltaTime);        
    
    // apply gravity and collision with the ground/floor
    if (!_owningAgent && USING_AVATAR_GRAVITY) {
        _velocity += _gravity * (GRAVITY_SCALE * deltaTime);
        
        updateCollisionWithEnvironment();
    }
    
    // update body springs
    updateBodySprings(deltaTime);
    
    // test for avatar collision response with the big sphere
    if (usingBigSphereCollisionTest) {
        updateCollisionWithSphere(_TEST_bigSpherePosition, _TEST_bigSphereRadius, deltaTime);
    }
    
    // collision response with voxels
    if (!_owningAgent) {
        updateCollisionWithVoxels();
    }
    
    glm::quat orientation = getOrientation();
    glm::vec3 front = orientation * AVATAR_FRONT;
    glm::vec3 right = orientation * AVATAR_RIGHT;
    glm::vec3 up = orientation * AVATAR_UP;
    
    // driving the avatar around should only apply if this is my avatar (as opposed to an avatar being driven remotely)
    if (!_owningAgent) {
        
        _thrust = glm::vec3(0.0f, 0.0f, 0.0f);
        
        //  Add Thrusts from keyboard
        if (_driveKeys[FWD      ]) {_thrust       += THRUST_MAG * deltaTime * front;}
        if (_driveKeys[BACK     ]) {_thrust       -= THRUST_MAG * deltaTime * front;}
        if (_driveKeys[RIGHT    ]) {_thrust       += THRUST_MAG * deltaTime * right;}
        if (_driveKeys[LEFT     ]) {_thrust       -= THRUST_MAG * deltaTime * right;}
        if (_driveKeys[UP       ]) {_thrust       += THRUST_MAG * deltaTime * up;}
        if (_driveKeys[DOWN     ]) {_thrust       -= THRUST_MAG * deltaTime * up;}
        if (_driveKeys[ROT_RIGHT]) {_bodyYawDelta -= YAW_MAG    * deltaTime;}
        if (_driveKeys[ROT_LEFT ]) {_bodyYawDelta += YAW_MAG    * deltaTime;}

        //  Add thrusts from Transmitter 
        if (transmitter) {
            glm::vec3 rotation = transmitter->getEstimatedRotation();
            const float TRANSMITTER_MIN_RATE = 1.f;
            const float TRANSMITTER_MIN_YAW_RATE = 4.f;
            const float TRANSMITTER_LATERAL_FORCE_SCALE = 25.f;
            const float TRANSMITTER_FWD_FORCE_SCALE = 100.f;
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
                _thrust += THRUST_MAG *
                           (float)(transmitter->getTouchState()->y - TOUCH_POSITION_RANGE_HALF) / TOUCH_POSITION_RANGE_HALF *
                           TRANSMITTER_LIFT_SCALE *
                           deltaTime *
                           up;
            }
        }
    }
    
    // update body yaw by body yaw delta
    if (!_owningAgent) {
        orientation = orientation * glm::quat(glm::radians(
            glm::vec3(_bodyPitchDelta, _bodyYawDelta, _bodyRollDelta) * deltaTime));
    }
    
    // decay body rotation momentum
    float bodySpinMomentum = 1.0 - BODY_SPIN_FRICTION * deltaTime;
    if  (bodySpinMomentum < 0.0f) { bodySpinMomentum = 0.0f; } 
    _bodyPitchDelta *= bodySpinMomentum;
    _bodyYawDelta   *= bodySpinMomentum;
    _bodyRollDelta  *= bodySpinMomentum;
        
    // add thrust to velocity
    _velocity += _thrust * deltaTime;
    
    // calculate speed 
    _speed = glm::length(_velocity);
    
    //pitch and roll the body as a function of forward speed and turning delta
    const float BODY_PITCH_WHILE_WALKING      = 20.0;
    const float BODY_ROLL_WHILE_TURNING       = 0.2;
    float forwardComponentOfVelocity = glm::dot(getBodyFrontDirection(), _velocity);
    orientation = orientation * glm::quat(glm::radians(glm::vec3(
        -BODY_PITCH_WHILE_WALKING * deltaTime * forwardComponentOfVelocity, 0.0f,
        BODY_ROLL_WHILE_TURNING  * deltaTime * _speed * _bodyYawDelta)));
    
    // these forces keep the body upright...     
    float tiltDecay = BODY_UPRIGHT_FORCE * deltaTime;
    if  (tiltDecay > 1.0f) {tiltDecay = 1.0f;}     

    // update the euler angles
    setOrientation(orientation);

    //the following will be used to make the avatar upright no matter what gravity is
    setOrientation(computeRotationFromBodyToWorldUp(tiltDecay) * orientation);

    // update position by velocity
    _position += _velocity * deltaTime;

    // decay velocity
    float decay = 1.0 - VELOCITY_DECAY * deltaTime;
    if ( decay < 0.0 ) {
        _velocity = glm::vec3( 0.0f, 0.0f, 0.0f );
    } else {
        _velocity *= decay;
    }
    
    // If another avatar is near, dampen velocity as a function of closeness
    if (!_owningAgent && (_distanceToNearestAvatar < PERIPERSONAL_RADIUS)) {
        float closeness = 1.0f - (_distanceToNearestAvatar / PERIPERSONAL_RADIUS);
        float drag = 1.0f - closeness * AVATAR_BRAKING_STRENGTH * deltaTime;
        if ( drag > 0.0f ) {
            _velocity *= drag;
        } else {
            _velocity = glm::vec3( 0.0f, 0.0f, 0.0f );
        }
    }
    
    //  Compute instantaneous acceleration 
    float acceleration = glm::distance(getVelocity(), oldVelocity) / deltaTime;
    const float ACCELERATION_PITCH_DECAY = 0.4f;
    const float ACCELERATION_YAW_DECAY = 0.4f;
    
    const float OCULUS_ACCELERATION_PULL_THRESHOLD = 1.0f;
    const int OCULUS_YAW_OFFSET_THRESHOLD = 10;
    
    // Decay HeadPitch as a function of acceleration, so that you look straight ahead when
    // you start moving, but don't do this with an HMD like the Oculus. 
    if (!OculusManager::isConnected()) {
        _head.setPitch(_head.getPitch() * (1.f - acceleration * ACCELERATION_PITCH_DECAY * deltaTime));
        _head.setYaw(_head.getYaw() * (1.f - acceleration * ACCELERATION_YAW_DECAY * deltaTime));
    } else if (fabsf(acceleration) > OCULUS_ACCELERATION_PULL_THRESHOLD
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

    //apply the head lean values to the springy position...
    if (USING_HEAD_LEAN) {
        if (fabs(_head.getLeanSideways() + _head.getLeanForward()) > 0.0f) {
            glm::vec3 headLean = 
                right * _head.getLeanSideways() +
                front * _head.getLeanForward();
                        
            _joint[ AVATAR_JOINT_TORSO            ].springyPosition += headLean * 0.1f;
            _joint[ AVATAR_JOINT_CHEST            ].springyPosition += headLean * 0.4f;
            _joint[ AVATAR_JOINT_NECK_BASE        ].springyPosition += headLean * 0.7f;
            _joint[ AVATAR_JOINT_HEAD_BASE        ].springyPosition += headLean * 1.0f;
            
            _joint[ AVATAR_JOINT_LEFT_COLLAR      ].springyPosition += headLean * 0.6f;
            _joint[ AVATAR_JOINT_LEFT_SHOULDER    ].springyPosition += headLean * 0.6f;
            _joint[ AVATAR_JOINT_LEFT_ELBOW       ].springyPosition += headLean * 0.2f;
            _joint[ AVATAR_JOINT_LEFT_WRIST       ].springyPosition += headLean * 0.1f;
            _joint[ AVATAR_JOINT_LEFT_FINGERTIPS  ].springyPosition += headLean * 0.0f;
            
            _joint[ AVATAR_JOINT_RIGHT_COLLAR     ].springyPosition += headLean * 0.6f;
            _joint[ AVATAR_JOINT_RIGHT_SHOULDER   ].springyPosition += headLean * 0.6f;
            _joint[ AVATAR_JOINT_RIGHT_ELBOW      ].springyPosition += headLean * 0.2f;
            _joint[ AVATAR_JOINT_RIGHT_WRIST      ].springyPosition += headLean * 0.1f;
            _joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].springyPosition += headLean * 0.0f;
        }
     }

    // set head lookat position 
    if (!_owningAgent) {
        if (_interactingOther) {
            _head.setLookAtPosition(_interactingOther->caclulateAverageEyePosition());
        } else {
            _head.setLookAtPosition(glm::vec3(0.0f, 0.0f, 0.0f)); // 0,0,0 represents NOT looking at anything
        }
    }    

    _head.setBodyRotation(glm::vec3(_bodyPitch, _bodyYaw, _bodyRoll));
    _head.setPosition(_joint[ AVATAR_JOINT_HEAD_BASE ].springyPosition);
    _head.setScale   (_joint[ AVATAR_JOINT_HEAD_BASE ].radius);
    _head.setSkinColor(glm::vec3(SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]));
    _head.simulate(deltaTime, !_owningAgent);
    
    // use speed and angular velocity to determine walking vs. standing                                
    if (_speed + fabs(_bodyYawDelta) > 0.2) {
        _mode = AVATAR_MODE_WALKING;
    } else {
        _mode = AVATAR_MODE_INTERACTING;
    }
}

void Avatar::checkForMouseRayTouching() {

    for (int b = 0; b < NUM_AVATAR_JOINTS; b++) {
    
        glm::vec3 directionToBodySphere = glm::normalize(_joint[b].springyPosition - _mouseRayOrigin);
        float dot = glm::dot(directionToBodySphere, _mouseRayDirection);

        if (dot > (1.0f - JOINT_TOUCH_RANGE)) {
            _joint[b].touchForce = (dot - (1.0f - JOINT_TOUCH_RANGE)) / JOINT_TOUCH_RANGE;
        } else {
            _joint[b].touchForce = 0.0;
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
    glm::vec3 right = orientation * AVATAR_RIGHT;
    glm::vec3 up = orientation * AVATAR_UP;
    glm::vec3 front = orientation * AVATAR_FRONT;

    glm::vec3 transformedHandMovement
    = right *  _movedHandOffset.x * 2.0f
    + up	  * -_movedHandOffset.y * 2.0f
    + front * -_movedHandOffset.z * 2.0f;
    
    _joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position += transformedHandMovement;
            
    if (!_owningAgent) {
        _avatarTouch.setMyBodyPosition(_position);
        _avatarTouch.setMyOrientation(orientation);
                
        float closestDistance = std::numeric_limits<float>::max();
        
        _interactingOther = NULL;

        //loop through all the other avatars for potential interactions...
        AgentList* agentList = AgentList::getInstance();
        for (AgentList::iterator agent = agentList->begin(); agent != agentList->end(); agent++) {
            if (agent->getLinkedData() != NULL && agent->getType() == AGENT_TYPE_AVATAR) {
                Avatar *otherAvatar = (Avatar *)agent->getLinkedData();
                 
                //Test:  Show angle between your fwd vector and nearest avatar
                //glm::vec3 vectorBetweenUs = otherAvatar->getJointPosition(AVATAR_JOINT_PELVIS) -
                //                getJointPosition(AVATAR_JOINT_PELVIS);
                //printLog("Angle between: %f\n", angleBetween(vectorBetweenUs, getBodyFrontDirection()));
                
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
            _avatarTouch.setYourOrientation (_interactingOther->getOrientation());
            _avatarTouch.setYourHandPosition(_interactingOther->_joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].springyPosition);   
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
                _interactingOther->_joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position - 
                _joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position
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
                _joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position += 
                ( 
                    _interactingOther->_joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position 
                    - _joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position
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
        setHandPosition(_joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position);
     
        if (_mousePressed) {
            _handState = HAND_STATE_GRASPING;
        } else {
            _handState = HAND_STATE_NULL;
        }
        
        _avatarTouch.setMyHandState(_handState);
        _avatarTouch.setMyHandPosition(_joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].springyPosition);
    }
}

void Avatar::updateCollisionWithSphere(glm::vec3 position, float radius, float deltaTime) {
    float myBodyApproximateBoundingRadius = 1.0f;
    glm::vec3 vectorFromMyBodyToBigSphere(_position - position);
    bool jointCollision = false;
    
    float distanceToBigSphere = glm::length(vectorFromMyBodyToBigSphere);
    if (distanceToBigSphere < myBodyApproximateBoundingRadius + radius) {
        for (int b = 0; b < NUM_AVATAR_JOINTS; b++) {
            glm::vec3 vectorFromJointToBigSphereCenter(_joint[b].springyPosition - position);
            float distanceToBigSphereCenter = glm::length(vectorFromJointToBigSphereCenter);
            float combinedRadius = _joint[b].radius + radius;
            
            if (distanceToBigSphereCenter < combinedRadius)  {
                jointCollision = true;
                if (distanceToBigSphereCenter > 0.0) {
                    glm::vec3 directionVector = vectorFromJointToBigSphereCenter / distanceToBigSphereCenter;
                    
                    float penetration = 1.0 - (distanceToBigSphereCenter / combinedRadius);
                    glm::vec3 collisionForce = vectorFromJointToBigSphereCenter * penetration;
                    
                    _joint[b].springyVelocity += collisionForce * 0.0f * deltaTime;
                    _velocity                 += collisionForce * 40.0f * deltaTime;
                    _joint[b].springyPosition  = position + directionVector * combinedRadius;
                }
            }
        }
    }
}

void Avatar::updateCollisionWithEnvironment() {
    glm::vec3 up = getBodyUpDirection();
    float radius = _height * 0.125f;
    glm::vec3 penetration;
    if (Application::getInstance()->getEnvironment()->findCapsulePenetration(
            _position - up * (_pelvisFloatingHeight - radius),
            _position + up * (_height - _pelvisFloatingHeight - radius), radius, penetration)) {
        applyCollisionWithScene(penetration);
    }
}

void Avatar::updateCollisionWithVoxels() {
    float radius = _height * 0.125f;
    glm::vec3 penetration;
    if (Application::getInstance()->getVoxels()->findCapsulePenetration(
            _position - glm::vec3(0.0f, _pelvisFloatingHeight - radius, 0.0f),
            _position + glm::vec3(0.0f, _height - _pelvisFloatingHeight - radius, 0.0f), radius, penetration)) {
        applyCollisionWithScene(penetration);
    }
}

void Avatar::applyCollisionWithScene(const glm::vec3& penetration) {
    _position -= penetration;
        
    // reflect the velocity component in the direction of penetration
    float penetrationLength = glm::length(penetration);
    if (penetrationLength > EPSILON) {
        glm::vec3 direction = penetration / penetrationLength;
        _velocity -= 2.0f * glm::dot(_velocity, direction) * direction * BOUNCE;
    }
}

void Avatar::updateAvatarCollisions(float deltaTime) {
        
    //  Reset detector for nearest avatar
    _distanceToNearestAvatar = std::numeric_limits<float>::max();

    //loop through all the other avatars for potential interactions...
    AgentList* agentList = AgentList::getInstance();
    for (AgentList::iterator agent = agentList->begin(); agent != agentList->end(); agent++) {
        if (agent->getLinkedData() != NULL && agent->getType() == AGENT_TYPE_AVATAR) {
            Avatar *otherAvatar = (Avatar *)agent->getLinkedData();
            
            // check if the bounding spheres of the two avatars are colliding
            glm::vec3 vectorBetweenBoundingSpheres(_position - otherAvatar->_position);
            if (glm::length(vectorBetweenBoundingSpheres) < _height * ONE_HALF + otherAvatar->_height * ONE_HALF) {
                //apply forces from collision
                applyCollisionWithOtherAvatar(otherAvatar, deltaTime);
            }            

            // test other avatar hand position for proximity
            glm::vec3 v(_joint[ AVATAR_JOINT_RIGHT_SHOULDER ].position);
            v -= otherAvatar->getPosition();
            
            float distance = glm::length(v);
            if (distance < _distanceToNearestAvatar) {
                _distanceToNearestAvatar = distance;
            }
        }
    }
}

//detect collisions with other avatars and respond
void Avatar::applyCollisionWithOtherAvatar(Avatar * otherAvatar, float deltaTime) {
        
    float bodyMomentum = 1.0f;
    glm::vec3 bodyPushForce = glm::vec3(0.0f, 0.0f, 0.0f);
        
    // loop through the joints of each avatar to check for every possible collision
    for (int b=1; b<NUM_AVATAR_JOINTS; b++) {
        if (_joint[b].isCollidable) {

            for (int o=b+1; o<NUM_AVATAR_JOINTS; o++) {
                if (otherAvatar->_joint[o].isCollidable) {
                
                    glm::vec3 vectorBetweenJoints(_joint[b].springyPosition - otherAvatar->_joint[o].springyPosition);
                    float distanceBetweenJoints = glm::length(vectorBetweenJoints);
                    
                    if (distanceBetweenJoints > 0.0) { // to avoid divide by zero
                        float combinedRadius = _joint[b].radius + otherAvatar->_joint[o].radius;

                        // check for collision
                        if (distanceBetweenJoints < combinedRadius * COLLISION_RADIUS_SCALAR)  {
                            glm::vec3 directionVector = vectorBetweenJoints / distanceBetweenJoints;

                            // push balls away from each other and apply friction
                            glm::vec3 ballPushForce = directionVector * COLLISION_BALL_FORCE * deltaTime;
                                                            
                            float ballMomentum = 1.0 - COLLISION_BALL_FRICTION * deltaTime;
                            if (ballMomentum < 0.0) { ballMomentum = 0.0;}
                                                            
                                         _joint[b].springyVelocity += ballPushForce;
                            otherAvatar->_joint[o].springyVelocity -= ballPushForce;
                            
                                         _joint[b].springyVelocity *= ballMomentum;
                            otherAvatar->_joint[o].springyVelocity *= ballMomentum;
                            
                            // accumulate forces and frictions to apply to the velocities of avatar bodies
                            bodyPushForce += directionVector * COLLISION_BODY_FORCE * deltaTime;                                
                            bodyMomentum -= COLLISION_BODY_FRICTION * deltaTime;
                            if (bodyMomentum < 0.0) { bodyMomentum = 0.0;}
                                                            
                        }// check for collision
                    }   // to avoid divide by zero
                }      // o loop
            }         // collidable
        }            // b loop
    }               // collidable
    
    
    //apply forces and frictions on the bodies of both avatars 
                 _velocity += bodyPushForce;
    otherAvatar->_velocity -= bodyPushForce;
                 _velocity *= bodyMomentum;
    otherAvatar->_velocity *= bodyMomentum;        
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
    }
}

void Avatar::render(bool lookingInMirror, glm::vec3 cameraPosition) {
    
    _cameraPosition = cameraPosition;
    
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

    //render body
    renderBody(lookingInMirror);
    
    // if this is my avatar, then render my interactions with the other avatar
    if (!_owningAgent) {
        _avatarTouch.render(getCameraPosition());
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
        
        // extract the view direction from the modelview matrix: transform (0, 0, 1) by the
        // transpose of the modelview to get its direction in world space, then use the X/Z
        // components to determine the angle
        float modelview[16];
        glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
        
        glTranslatef(_joint[AVATAR_JOINT_HEAD_BASE].springyPosition.x,
                     _joint[AVATAR_JOINT_HEAD_BASE].springyPosition.y + chatMessageHeight,
                     _joint[AVATAR_JOINT_HEAD_BASE].springyPosition.z);
        glRotatef(atan2(-modelview[2], -modelview[10]) * 180 / PI, 0, 1, 0);
        
        glColor3f(0, 0.8, 0);
        glRotatef(180, 0, 0, 1);
        glScalef(chatMessageScale, chatMessageScale, 1.0f);

        glDisable(GL_LIGHTING);
        glDepthMask(false);
        if (_keyState == NO_KEY_DOWN) {
            textRenderer()->draw(-width/2, 0, _chatMessage.c_str());
            
        } else {
            // rather than using substr and allocating a new string, just replace the last
            // character with a null, then restore it
            int lastIndex = _chatMessage.size() - 1;
            char lastChar = _chatMessage[lastIndex];
            _chatMessage[lastIndex] = '\0';
            textRenderer()->draw(-width/2, 0, _chatMessage.c_str());
            _chatMessage[lastIndex] = lastChar;
            glColor3f(0, 1, 0);
            textRenderer()->draw(width/2 - lastWidth, 0, _chatMessage.c_str() + lastIndex);                        
        }
        glEnable(GL_LIGHTING);
        glDepthMask(true);
        
        glPopMatrix();
    }
}

void Avatar::initializeSkeleton() {
    
    for (int b=0; b<NUM_AVATAR_JOINTS; b++) {
        _joint[b].isCollidable        = true;
        _joint[b].parent              = AVATAR_JOINT_NULL;
        _joint[b].position            = glm::vec3(0.0, 0.0, 0.0);
        _joint[b].defaultPosePosition = glm::vec3(0.0, 0.0, 0.0);
        _joint[b].springyPosition     = glm::vec3(0.0, 0.0, 0.0);
        _joint[b].springyVelocity     = glm::vec3(0.0, 0.0, 0.0);
        _joint[b].orientation         = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
        _joint[b].length              = 0.0;
        _joint[b].radius              = 0.0;
        _joint[b].touchForce          = 0.0;
        _joint[b].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS;
    }
    
    // specify the parental hierarchy
    _joint[ AVATAR_JOINT_PELVIS              ].parent = AVATAR_JOINT_NULL;
    _joint[ AVATAR_JOINT_TORSO            ].parent = AVATAR_JOINT_PELVIS;
    _joint[ AVATAR_JOINT_CHEST              ].parent = AVATAR_JOINT_TORSO;
    _joint[ AVATAR_JOINT_NECK_BASE          ].parent = AVATAR_JOINT_CHEST;
    _joint[ AVATAR_JOINT_HEAD_BASE        ].parent = AVATAR_JOINT_NECK_BASE;
    _joint[ AVATAR_JOINT_HEAD_TOP         ].parent = AVATAR_JOINT_HEAD_BASE;
    _joint[ AVATAR_JOINT_LEFT_COLLAR      ].parent = AVATAR_JOINT_CHEST;
    _joint[ AVATAR_JOINT_LEFT_SHOULDER    ].parent = AVATAR_JOINT_LEFT_COLLAR;
    _joint[ AVATAR_JOINT_LEFT_ELBOW          ].parent = AVATAR_JOINT_LEFT_SHOULDER;
    _joint[ AVATAR_JOINT_LEFT_WRIST          ].parent = AVATAR_JOINT_LEFT_ELBOW;
    _joint[ AVATAR_JOINT_LEFT_FINGERTIPS  ].parent = AVATAR_JOINT_LEFT_WRIST;
    _joint[ AVATAR_JOINT_RIGHT_COLLAR     ].parent = AVATAR_JOINT_CHEST;
    _joint[ AVATAR_JOINT_RIGHT_SHOULDER      ].parent = AVATAR_JOINT_RIGHT_COLLAR;
    _joint[ AVATAR_JOINT_RIGHT_ELBOW      ].parent = AVATAR_JOINT_RIGHT_SHOULDER;
    _joint[ AVATAR_JOINT_RIGHT_WRIST      ].parent = AVATAR_JOINT_RIGHT_ELBOW;
    _joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].parent = AVATAR_JOINT_RIGHT_WRIST;
    _joint[ AVATAR_JOINT_LEFT_HIP          ].parent = AVATAR_JOINT_PELVIS;
    _joint[ AVATAR_JOINT_LEFT_KNEE          ].parent = AVATAR_JOINT_LEFT_HIP;
    _joint[ AVATAR_JOINT_LEFT_HEEL          ].parent = AVATAR_JOINT_LEFT_KNEE;
    _joint[ AVATAR_JOINT_LEFT_TOES          ].parent = AVATAR_JOINT_LEFT_HEEL;
    _joint[ AVATAR_JOINT_RIGHT_HIP          ].parent = AVATAR_JOINT_PELVIS;
    _joint[ AVATAR_JOINT_RIGHT_KNEE          ].parent = AVATAR_JOINT_RIGHT_HIP;
    _joint[ AVATAR_JOINT_RIGHT_HEEL          ].parent = AVATAR_JOINT_RIGHT_KNEE;
    _joint[ AVATAR_JOINT_RIGHT_TOES          ].parent = AVATAR_JOINT_RIGHT_HEEL;
    
    // specify the default pose position
    _joint[ AVATAR_JOINT_PELVIS           ].defaultPosePosition = glm::vec3(  0.0,   0.0,  0.0 );
    _joint[ AVATAR_JOINT_TORSO            ].defaultPosePosition = glm::vec3(  0.0,   0.09,  -0.01 );
    _joint[ AVATAR_JOINT_CHEST            ].defaultPosePosition = glm::vec3(  0.0,   0.09,  -0.01  );
    _joint[ AVATAR_JOINT_NECK_BASE        ].defaultPosePosition = glm::vec3(  0.0,   0.14,  0.01 );
    _joint[ AVATAR_JOINT_HEAD_BASE        ].defaultPosePosition = glm::vec3(  0.0,   0.04,  0.00 );
    
    _joint[ AVATAR_JOINT_LEFT_COLLAR      ].defaultPosePosition = glm::vec3( -0.06,  0.04, 0.01 );
    _joint[ AVATAR_JOINT_LEFT_SHOULDER      ].defaultPosePosition = glm::vec3( -0.05,  0.0,  0.01 );
    _joint[ AVATAR_JOINT_LEFT_ELBOW       ].defaultPosePosition = glm::vec3(  0.0,  -0.16,  0.0  );
    _joint[ AVATAR_JOINT_LEFT_WRIST          ].defaultPosePosition = glm::vec3(  0.0,  -0.117,  0.0  );
    _joint[ AVATAR_JOINT_LEFT_FINGERTIPS  ].defaultPosePosition = glm::vec3(  0.0,  -0.1,  0.0  );
    
    _joint[ AVATAR_JOINT_RIGHT_COLLAR     ].defaultPosePosition = glm::vec3( 0.06,  0.04, 0.01 );
    _joint[ AVATAR_JOINT_RIGHT_SHOULDER      ].defaultPosePosition = glm::vec3( 0.05,  0.0,  0.01 );
    _joint[ AVATAR_JOINT_RIGHT_ELBOW      ].defaultPosePosition = glm::vec3(  0.0,  -0.16,  0.0  );
    _joint[ AVATAR_JOINT_RIGHT_WRIST      ].defaultPosePosition = glm::vec3(  0.0,  -0.117,  0.0  );
    _joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].defaultPosePosition = glm::vec3(  0.0,  -0.1,  0.0  );
    
    _joint[ AVATAR_JOINT_LEFT_HIP          ].defaultPosePosition = glm::vec3(  -0.05,  0.0,  0.02 );
    _joint[ AVATAR_JOINT_LEFT_KNEE          ].defaultPosePosition = glm::vec3( 0.01, -0.25,  -0.03 );
    _joint[ AVATAR_JOINT_LEFT_HEEL          ].defaultPosePosition = glm::vec3( 0.01, -0.22, 0.08 );
    _joint[ AVATAR_JOINT_LEFT_TOES          ].defaultPosePosition = glm::vec3(  0.00, -0.03,  -0.05 );
    
    _joint[ AVATAR_JOINT_RIGHT_HIP          ].defaultPosePosition = glm::vec3(  0.05,  0.0,  0.02 );
    _joint[ AVATAR_JOINT_RIGHT_KNEE          ].defaultPosePosition = glm::vec3(  -0.01, -0.25,  -0.03 );
    _joint[ AVATAR_JOINT_RIGHT_HEEL          ].defaultPosePosition = glm::vec3(  -0.01, -0.22, 0.08 );
    _joint[ AVATAR_JOINT_RIGHT_TOES          ].defaultPosePosition = glm::vec3(  0.00, -0.03,  -0.05 );
    
    // specify the radii of the joints
    _joint[ AVATAR_JOINT_PELVIS           ].radius = 0.07;
    _joint[ AVATAR_JOINT_TORSO            ].radius = 0.065;
    _joint[ AVATAR_JOINT_CHEST            ].radius = 0.08;
    _joint[ AVATAR_JOINT_NECK_BASE        ].radius = 0.03;
    _joint[ AVATAR_JOINT_HEAD_BASE        ].radius = 0.07;
    
    _joint[ AVATAR_JOINT_LEFT_COLLAR      ].radius = 0.04;
    _joint[ AVATAR_JOINT_LEFT_SHOULDER    ].radius = 0.03;
    _joint[ AVATAR_JOINT_LEFT_ELBOW          ].radius = 0.02;
    _joint[ AVATAR_JOINT_LEFT_WRIST       ].radius = 0.02;
    _joint[ AVATAR_JOINT_LEFT_FINGERTIPS  ].radius = 0.01;
    
    _joint[ AVATAR_JOINT_RIGHT_COLLAR     ].radius = 0.04;
    _joint[ AVATAR_JOINT_RIGHT_SHOULDER      ].radius = 0.03;
    _joint[ AVATAR_JOINT_RIGHT_ELBOW      ].radius = 0.02;
    _joint[ AVATAR_JOINT_RIGHT_WRIST      ].radius = 0.02;
    _joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].radius = 0.01;
    
    _joint[ AVATAR_JOINT_LEFT_HIP          ].radius = 0.04;
    _joint[ AVATAR_JOINT_LEFT_KNEE          ].radius = 0.025;
    _joint[ AVATAR_JOINT_LEFT_HEEL          ].radius = 0.025;
    _joint[ AVATAR_JOINT_LEFT_TOES          ].radius = 0.025;
    
    _joint[ AVATAR_JOINT_RIGHT_HIP          ].radius = 0.04;
    _joint[ AVATAR_JOINT_RIGHT_KNEE          ].radius = 0.025;
    _joint[ AVATAR_JOINT_RIGHT_HEEL          ].radius = 0.025;
    _joint[ AVATAR_JOINT_RIGHT_TOES          ].radius = 0.025;
    
    // to aid in hand-shaking and hand-holding, the right hand is not collidable
    _joint[ AVATAR_JOINT_RIGHT_ELBOW      ].isCollidable = false;
    _joint[ AVATAR_JOINT_RIGHT_WRIST      ].isCollidable = false;
    _joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].isCollidable = false; 
       
    // calculate bone length
    calculateBoneLengths();
    
    _pelvisStandingHeight = 
    _joint[ AVATAR_JOINT_LEFT_HEEL ].radius +
    _joint[ AVATAR_JOINT_LEFT_HEEL ].length +
    _joint[ AVATAR_JOINT_LEFT_KNEE ].length;
    //printf("_pelvisStandingHeight = %f\n", _pelvisStandingHeight);
    
    _pelvisFloatingHeight = _pelvisStandingHeight + FLOATING_HEIGHT;
    
    _height = 
    (
        _pelvisStandingHeight +
        _joint[ AVATAR_JOINT_LEFT_HEEL ].radius +
        _joint[ AVATAR_JOINT_LEFT_HEEL ].length +
        _joint[ AVATAR_JOINT_LEFT_KNEE ].length +
        _joint[ AVATAR_JOINT_PELVIS    ].length +
        _joint[ AVATAR_JOINT_TORSO     ].length +
        _joint[ AVATAR_JOINT_CHEST     ].length +
        _joint[ AVATAR_JOINT_NECK_BASE ].length +
        _joint[ AVATAR_JOINT_HEAD_BASE ].length +
        _joint[ AVATAR_JOINT_HEAD_BASE ].radius
    );
    //printf("_height = %f\n", _height);
    
    // generate joint positions by updating the skeleton
    updateSkeleton();
    
    //set spring positions to be in the skeleton bone positions
    initializeBodySprings();
}

void Avatar::calculateBoneLengths() {
    for (int b = 0; b < NUM_AVATAR_JOINTS; b++) {
        _joint[b].length = glm::length(_joint[b].defaultPosePosition);
    }
    
    _maxArmLength
    = _joint[ AVATAR_JOINT_RIGHT_ELBOW      ].length
    + _joint[ AVATAR_JOINT_RIGHT_WRIST        ].length
    + _joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].length;
}

void Avatar::updateSkeleton() {
    
    // rotate body...
    glm::quat orientation = getOrientation();
    
    // calculate positions of all bones by traversing the skeleton tree:
    for (int b = 0; b < NUM_AVATAR_JOINTS; b++) {
        if (_joint[b].parent == AVATAR_JOINT_NULL) {
            _joint[b].orientation = orientation;
            _joint[b].position = _position;
        }
        else {
            _joint[b].orientation = _joint[ _joint[b].parent ].orientation;
            _joint[b].position = _joint[ _joint[b].parent ].position;
        }
        
        // if this is not my avatar, then hand position comes from transmitted data
        if (_owningAgent) {
            _joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position = _handPosition;
        }
        
        // the following will be replaced by a proper rotation...close
        glm::vec3 rotatedJointVector = _joint[b].orientation * _joint[b].defaultPosePosition;
        
        //glm::vec3 myEuler (0.0f, 0.0f, 0.0f);
        //glm::quat myQuat (myEuler);
        
        _joint[b].position += rotatedJointVector;
    }
}

void Avatar::initializeBodySprings() {
    for (int b = 0; b < NUM_AVATAR_JOINTS; b++) {
        _joint[b].springyPosition = _joint[b].position;
        _joint[b].springyVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
    }
}

void Avatar::updateBodySprings(float deltaTime) {
    //  Check for a large repositioning, and re-initialize body springs if this has happened
    const float BEYOND_BODY_SPRING_RANGE = 2.f;
    if (glm::length(_position - _joint[AVATAR_JOINT_PELVIS].springyPosition) > BEYOND_BODY_SPRING_RANGE) {
        initializeBodySprings();
    }
    for (int b = 0; b < NUM_AVATAR_JOINTS; b++) {
        glm::vec3 springVector(_joint[b].springyPosition);
        
        if (_joint[b].parent == AVATAR_JOINT_NULL) {
            springVector -= _position;
        }
        else {
            springVector -= _joint[ _joint[b].parent ].springyPosition;
        }
        
        float length = glm::length(springVector);
        
        if (length > 0.0f) { // to avoid divide by zero
            glm::vec3 springDirection = springVector / length;
            
            float force = (length - _joint[b].length) * BODY_SPRING_FORCE * deltaTime;
            
            _joint[b].springyVelocity -= springDirection * force;
            
            if (_joint[b].parent != AVATAR_JOINT_NULL) {
                _joint[_joint[b].parent].springyVelocity += springDirection * force;
            }
        }
        
        // apply tightness force - (causing springy position to be close to rigid body position)
        _joint[b].springyVelocity += (_joint[b].position - _joint[b].springyPosition) * _joint[b].springBodyTightness * deltaTime;
        
        // apply decay
        float decay = 1.0 - BODY_SPRING_DECAY * deltaTime;
        if (decay > 0.0) {
            _joint[b].springyVelocity *= decay;
        }
        else {
            _joint[b].springyVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
        }
        
        /*
        //apply forces from touch...
        if (_joint[b].touchForce > 0.0) {
            _joint[b].springyVelocity += _mouseRayDirection * _joint[b].touchForce * 0.7f;
        }
        */
        
        //update position by velocity...
        _joint[b].springyPosition += _joint[b].springyVelocity * deltaTime;
    }
}

void Avatar::updateArmIKAndConstraints(float deltaTime) {
    
    // determine the arm vector
    glm::vec3 armVector = _joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position;
    armVector -= _joint[ AVATAR_JOINT_RIGHT_SHOULDER ].position;
    
    // test to see if right hand is being dragged beyond maximum arm length
    float distance = glm::length(armVector);
    
    // don't let right hand get dragged beyond maximum arm length...
    if (distance > _maxArmLength) {
        // reset right hand to be constrained to maximum arm length
        _joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position = _joint[ AVATAR_JOINT_RIGHT_SHOULDER ].position;
        glm::vec3 armNormal = armVector / distance;
        armVector = armNormal * _maxArmLength;
        distance = _maxArmLength;
        glm::vec3 constrainedPosition = _joint[ AVATAR_JOINT_RIGHT_SHOULDER ].position;
        constrainedPosition += armVector;
        _joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position = constrainedPosition;
    }
    
    // set elbow position
    glm::vec3 newElbowPosition = _joint[ AVATAR_JOINT_RIGHT_SHOULDER ].position;
    newElbowPosition += armVector * ONE_HALF;

    glm::vec3 perpendicular = glm::cross(getBodyFrontDirection(),  armVector);
    
    newElbowPosition += perpendicular * (1.0f - (_maxArmLength / distance)) * ONE_HALF;
    _joint[ AVATAR_JOINT_RIGHT_ELBOW ].position = newElbowPosition;
    
    // set wrist position
    glm::vec3 vv(_joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position);
    vv -= _joint[ AVATAR_JOINT_RIGHT_ELBOW ].position;
    glm::vec3 newWristPosition = _joint[ AVATAR_JOINT_RIGHT_ELBOW ].position + vv * 0.7f;
    _joint[ AVATAR_JOINT_RIGHT_WRIST ].position = newWristPosition;
}

glm::quat Avatar::computeRotationFromBodyToWorldUp(float proportion) const {
    glm::quat orientation = getOrientation();
    glm::vec3 currentUp = orientation * AVATAR_UP;
    float angle = glm::degrees(acosf(glm::clamp(glm::dot(currentUp, _worldUpDirection), -1.0f, 1.0f)));
    if (angle < EPSILON) {
        return glm::quat();
    }
    glm::vec3 axis;
    if (angle > 179.99f) { // 180 degree rotation; must use another axis
        axis = orientation * AVATAR_RIGHT;
    } else {
        axis = glm::normalize(glm::cross(currentUp, _worldUpDirection));
    }
    return glm::angleAxis(angle * proportion, axis);
}

void Avatar::renderBody(bool lookingInMirror) {
    
    const float RENDER_OPAQUE_BEYOND = 1.0f;        //  Meters beyond which body is shown opaque
    const float RENDER_TRANSLUCENT_BEYOND = 0.5f;
    
    //  Render the body as balls and cones 
    for (int b = 0; b < NUM_AVATAR_JOINTS; b++) {
        float distanceToCamera = glm::length(_cameraPosition - _joint[b].position);
        
        float alpha = lookingInMirror ? 1.0f : glm::clamp((distanceToCamera - RENDER_TRANSLUCENT_BEYOND) /
            (RENDER_OPAQUE_BEYOND - RENDER_TRANSLUCENT_BEYOND), 0.f, 1.f);
        
        if (lookingInMirror || _owningAgent) {
            alpha = 1.0f;
        }
    
        //  Always render other people, and render myself when beyond threshold distance
        if (b == AVATAR_JOINT_HEAD_BASE) { // the head is rendered as a special
            if (lookingInMirror || _owningAgent || distanceToCamera > RENDER_OPAQUE_BEYOND * 0.5) {
                _head.render(lookingInMirror, _cameraPosition, alpha);
            }
        } else if (_owningAgent || distanceToCamera > RENDER_TRANSLUCENT_BEYOND
                   || b == AVATAR_JOINT_RIGHT_ELBOW
                   || b == AVATAR_JOINT_RIGHT_WRIST
                   || b == AVATAR_JOINT_RIGHT_FINGERTIPS ) {
            //  Render the sphere at the joint
            if (_owningAgent || b == AVATAR_JOINT_RIGHT_ELBOW
                             || b == AVATAR_JOINT_RIGHT_WRIST
                             || b == AVATAR_JOINT_RIGHT_FINGERTIPS ) {
                glColor3f(SKIN_COLOR[0] + _joint[b].touchForce * 0.3f,
                          SKIN_COLOR[1] - _joint[b].touchForce * 0.2f,
                          SKIN_COLOR[2] - _joint[b].touchForce * 0.1f);
            } else {
                glColor4f(SKIN_COLOR[0] + _joint[b].touchForce * 0.3f,
                          SKIN_COLOR[1] - _joint[b].touchForce * 0.2f,
                          SKIN_COLOR[2] - _joint[b].touchForce * 0.1f,
                          alpha);
            }
            
            if ((b != AVATAR_JOINT_HEAD_TOP  )
            &&  (b != AVATAR_JOINT_HEAD_BASE )) {
                glPushMatrix();
                glTranslatef(_joint[b].springyPosition.x, _joint[b].springyPosition.y, _joint[b].springyPosition.z);
                glutSolidSphere(_joint[b].radius, 20.0f, 20.0f);
                glPopMatrix();
            }
            
            //  Render the cone connecting this joint to its parent
            if (_joint[b].parent != AVATAR_JOINT_NULL) {
                if ((b != AVATAR_JOINT_HEAD_TOP      )
                &&  (b != AVATAR_JOINT_HEAD_BASE     )
                &&  (b != AVATAR_JOINT_PELVIS        )
                &&  (b != AVATAR_JOINT_TORSO         )
                &&  (b != AVATAR_JOINT_CHEST         )
                &&  (b != AVATAR_JOINT_LEFT_COLLAR   )
                &&  (b != AVATAR_JOINT_LEFT_SHOULDER )
                &&  (b != AVATAR_JOINT_RIGHT_COLLAR  )
                &&  (b != AVATAR_JOINT_RIGHT_SHOULDER)) {
                    glColor3fv(DARK_SKIN_COLOR);
                    
                    float r1 = _joint[_joint[b].parent ].radius * 0.8;
                    float r2 = _joint[b                ].radius * 0.8;
                    if (b == AVATAR_JOINT_HEAD_BASE) {
                        r1 *= 0.5f;
                    }
                    renderJointConnectingCone
                    (
                     _joint[_joint[b].parent ].springyPosition,
                     _joint[b                ].springyPosition, r2, r2
                     );
                }
            } 
        }
    }
}



void Avatar::setHeadFromGyros(glm::vec3* eulerAngles, glm::vec3* angularVelocity, float deltaTime, float smoothingTime) {
    //
    //  Given absolute position and angular velocity information, update the avatar's head angles
    //  with the goal of fast instantaneous updates that gradually follow the absolute data.
    //
    //  Euler Angle format is (Yaw, Pitch, Roll) in degrees
    //
    //  Angular Velocity is (Yaw, Pitch, Roll) in degrees per second
    //
    //  SMOOTHING_TIME is the time is seconds over which the head should average to the
    //  absolute eulerAngles passed.
    //  
    //
    
    if (deltaTime == 0.f) {
        //  On first sample, set head to absolute position
        _head.setYaw  (eulerAngles->x);
        _head.setPitch(eulerAngles->y);
        _head.setRoll (eulerAngles->z);
    } else { 
        glm::vec3 angles(_head.getYaw(), _head.getPitch(), _head.getRoll());
        //  Increment by detected velocity 
        angles += (*angularVelocity) * deltaTime;
        //  Smooth to slowly follow absolute values
        angles = ((1.f - deltaTime / smoothingTime) * angles) + (deltaTime / smoothingTime) * (*eulerAngles);
        _head.setYaw  (angles.x);
        _head.setPitch(angles.y);
        _head.setRoll (angles.z);
        //printLog("Y/P/R: %3.1f, %3.1f, %3.1f\n", angles.x, angles.y, angles.z);
    }
}

const char AVATAR_DATA_FILENAME[] = "avatar.ifd";

void Avatar::writeAvatarDataToFile() {
    // write the avatar position and yaw to a local file
    FILE* avatarFile = fopen(AVATAR_DATA_FILENAME, "w");
    
    if (avatarFile) {
        fprintf(avatarFile, "%f,%f,%f %f", _position.x, _position.y, _position.z, _bodyYaw);
        fclose(avatarFile);
    }
}

void Avatar::readAvatarDataFromFile() {
    FILE* avatarFile = fopen(AVATAR_DATA_FILENAME, "r");
    
    if (avatarFile) {
        glm::vec3 readPosition;
        float readYaw;
        fscanf(avatarFile, "%f,%f,%f %f", &readPosition.x, &readPosition.y, &readPosition.z, &readYaw);

        // make sure these values are sane
        if (!isnan(readPosition.x) && !isnan(readPosition.y) && !isnan(readPosition.z) && !isnan(readYaw)) {
            _position = readPosition;
            _bodyYaw = readYaw;
        }
        fclose(avatarFile);
    }
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
            angleb = ((float)(i+1) / (float)NUM_BODY_CONE_SIDES) * PI * 2.0f;
            
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




