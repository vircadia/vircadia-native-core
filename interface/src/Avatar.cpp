//
//  Avatar.cpp
//  interface
//
//  Created by Philip Rosedale on 9/11/12.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <glm/glm.hpp>
#include <vector>
#include <lodepng.h>
#include <SharedUtil.h>
#include "world.h"
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
const float BODY_SPRING_DEFAULT_TIGHTNESS = 1500.0f;
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

float skinColor    [] = {1.0, 0.84, 0.66};
float darkSkinColor[] = {0.9, 0.78, 0.63};
float lightBlue    [] = {0.7, 0.8,  1.0 };

bool usingBigSphereCollisionTest = true;

float chatMessageScale = 0.0015;
float chatMessageHeight = 0.45;

Avatar::Avatar(bool isMine) :
    _isMine(isMine),
    _TEST_bigSphereRadius(0.4f),
    _TEST_bigSpherePosition(5.0f, _TEST_bigSphereRadius, 5.0f),
    _mousePressed(false),
    _bodyPitchDelta(0.0f),
    _bodyYawDelta(0.0f),
    _bodyRollDelta(0.0f),
    _movedHandOffset(0.0f, 0.0f, 0.0f),
    _rotation(0.0f, 0.0f, 0.0f, 0.0f),
    _mode(AVATAR_MODE_STANDING),
    _handHoldingPosition(0.0f, 0.0f, 0.0f),
    _velocity(0.0f, 0.0f, 0.0f),
    _thrust(0.0f, 0.0f, 0.0f),
    _speed(0.0f),
    _maxArmLength(0.0f),
    _orientation(),
    _transmitterIsFirstData(true),
    _transmitterHz(0.0f),
    _transmitterPackets(0),
    _transmitterInitialReading(0.0f, 0.0f, 0.0f),
    _isTransmitterV2Connected(false),
    _pelvisStandingHeight(0.0f),
    _displayingHead(true),
    _distanceToNearestAvatar(std::numeric_limits<float>::max()),
    _gravity(0.0f, -1.0f, 0.0f),
    _mouseRayOrigin(0.0f, 0.0f, 0.0f),
    _mouseRayDirection(0.0f, 0.0f, 0.0f),
    _cameraPosition(0.0f, 0.0f, 0.0f),
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
    float measuredPitchRate = 0.0f;
    float measuredRollRate = 0.0f;
    float measuredYawRate = 0.0f;
    
    measuredPitchRate = serialInterface->getLastPitchRate();
    measuredYawRate = serialInterface->getLastYawRate();
    measuredRollRate = serialInterface->getLastRollRate();
   
    //  Update avatar head position based on measured gyro rates
    
    _head.addPitch(measuredPitchRate * deltaTime);
    _head.addYaw(measuredYawRate * deltaTime);
    _head.addRoll(measuredRollRate * deltaTime);
    
    //  Update head lean distance based on accelerometer data
    const float LEAN_SENSITIVITY = 0.15;
    const float LEAN_MAX = 0.45;
    const float LEAN_AVERAGING = 10.0;
    glm::vec3 headRotationRates(_head.getPitch(), _head.getYaw(), _head.getRoll());
    float headRateMax = 50.f;
    
    
    glm::vec3 leaning = (serialInterface->getLastAcceleration() -  serialInterface->getGravity())
                        * LEAN_SENSITIVITY
                        * (1.f - fminf(glm::length(headRotationRates), headRateMax) / headRateMax);
    leaning.y = 0.f;
    if (glm::length(leaning) < LEAN_MAX) {
        _head.setLeanForward(_head.getLeanForward() * (1.f - LEAN_AVERAGING * deltaTime) +
                             (LEAN_AVERAGING * deltaTime) * leaning.z * LEAN_SENSITIVITY);
        _head.setLeanSideways(_head.getLeanSideways() * (1.f - LEAN_AVERAGING * deltaTime) +
                              (LEAN_AVERAGING * deltaTime) * leaning.x * LEAN_SENSITIVITY);
    }
}

float Avatar::getAbsoluteHeadYaw() const {
    return _bodyYaw + _head.getYaw();
}

float Avatar::getAbsoluteHeadPitch() const {
    return _bodyPitch + _head.getPitch();
}

void Avatar::setMousePressed(bool mousePressed) {
	_mousePressed = mousePressed;
}

bool Avatar::getIsNearInteractingOther() { 
    return _avatarTouch.getAbleToReachOtherAvatar(); 
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

void Avatar::simulate(float deltaTime) {

    //figure out if the mouse cursor is over any body spheres... 
    //if (_isMine) {
    {
        checkForMouseRayTouching();
    }
    // copy velocity so we can use it later for acceleration
    glm::vec3 oldVelocity = getVelocity();
        
    // update balls
    if (_balls) { _balls->simulate(deltaTime); }
    
    // if other avatar, update head position from network data
    
	// update avatar skeleton
	updateSkeleton();
	
    //detect and respond to collisions with other avatars... 
    if (_isMine) {
        updateAvatarCollisions(deltaTime);
    }
    
    //update the movement of the hand and process handshaking with other avatars... 
    updateHandMovementAndTouching(deltaTime);
    
    _avatarTouch.simulate(deltaTime);        
    
    // apply gravity and collision with the ground/floor
    if (_isMine && USING_AVATAR_GRAVITY) {
        if (_position.y > _pelvisStandingHeight + 0.01f) {
            _velocity += _gravity * (GRAVITY_SCALE * deltaTime);
        } else if (_position.y < _pelvisStandingHeight) {
            _position.y = _pelvisStandingHeight;
            _velocity.y = -_velocity.y * BOUNCE;
        }
    }
    
	// update body springs
    updateBodySprings(deltaTime);
    
    // test for avatar collision response with the big sphere
    if (usingBigSphereCollisionTest) {
        updateCollisionWithSphere(_TEST_bigSpherePosition, _TEST_bigSphereRadius, deltaTime);
    }
    
    // driving the avatar around should only apply if this is my avatar (as opposed to an avatar being driven remotely)
    if (_isMine) {
        
        _thrust = glm::vec3(0.0f, 0.0f, 0.0f);
             
        if (_driveKeys[FWD      ]) {_thrust       += THRUST_MAG * deltaTime * _orientation.getFront();}
        if (_driveKeys[BACK     ]) {_thrust       -= THRUST_MAG * deltaTime * _orientation.getFront();}
        if (_driveKeys[RIGHT    ]) {_thrust       += THRUST_MAG * deltaTime * _orientation.getRight();}
        if (_driveKeys[LEFT     ]) {_thrust       -= THRUST_MAG * deltaTime * _orientation.getRight();}
        if (_driveKeys[UP       ]) {_thrust       += THRUST_MAG * deltaTime * _orientation.getUp();}
        if (_driveKeys[DOWN     ]) {_thrust       -= THRUST_MAG * deltaTime * _orientation.getUp();}
        if (_driveKeys[ROT_RIGHT]) {_bodyYawDelta -= YAW_MAG    * deltaTime;}
        if (_driveKeys[ROT_LEFT ]) {_bodyYawDelta += YAW_MAG    * deltaTime;}
	}
        
    // update body yaw by body yaw delta
    if (_isMine) {
        _bodyPitch += _bodyPitchDelta * deltaTime;
        _bodyYaw   += _bodyYawDelta   * deltaTime;
        _bodyRoll  += _bodyRollDelta  * deltaTime;
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
    float forwardComponentOfVelocity = glm::dot(_orientation.getFront(), _velocity);
    _bodyPitch += BODY_PITCH_WHILE_WALKING * deltaTime * forwardComponentOfVelocity;
    _bodyRoll  += BODY_ROLL_WHILE_TURNING  * deltaTime * _speed * _bodyYawDelta;
        
	// these forces keep the body upright...     
    float tiltDecay = 1.0 - BODY_UPRIGHT_FORCE * deltaTime;
    if  (tiltDecay < 0.0f) {tiltDecay = 0.0f;}     
    _bodyPitch *= tiltDecay;
    _bodyRoll  *= tiltDecay;
    
    //the following will be used to make the avatar upright no matter what gravity is
    //float f = angleBetween(_orientation.getUp(), _gravity);
    
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
    if (_isMine && (_distanceToNearestAvatar < PERIPERSONAL_RADIUS)) {    
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
    
    // Decay HeadPitch as a function of acceleration, so that you look straight ahead when
    // you start moving, but don't do this with an HMD like the Oculus. 
    if (!OculusManager::isConnected()) {
        _head.setPitch(_head.getPitch() * (1.f - acceleration * ACCELERATION_PITCH_DECAY * deltaTime));
    }

    
    //apply the head lean values to the springy position...
    if (fabs(_head.getLeanSideways() + _head.getLeanForward()) > 0.0f) {
        glm::vec3 headLean = 
            _orientation.getRight() * _head.getLeanSideways() +
            _orientation.getFront() * _head.getLeanForward();

        // this is not a long-term solution, but it works ok for initial purposes of making the avatar lean
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

    // set head lookat position 
    //setLookatPosition(glm::vec3(0.0f, 0.0f, 0.0f)); //default lookat position is 0,0,0   
    if (_interactingOther) {
        _head.setLooking(true);
        
        if (_isMine) {
            _head.setLookAtPosition(_interactingOther->getSpringyHeadPosition());
        }        
    } else {
        _head.setLooking(false);
    }    
    
    //_head.setLookatPosition(_lookatPosition); // this comes from Avatar Data
    _head.setBodyRotation   (glm::vec3(_bodyPitch, _bodyYaw, _bodyRoll));
    //_head.setRotationOffBody(glm::vec3(_headPitch, _headYaw, _headRoll));
    
    //_head.setRotationOffBody(glm::vec3(0.0f, 0.0f, 0.0f));

    _head.setPosition(_joint[ AVATAR_JOINT_HEAD_BASE ].springyPosition);
    _head.setScale   (_joint[ AVATAR_JOINT_HEAD_BASE ].radius);
    _head.setAudioLoudness(_audioLoudness);
    _head.setSkinColor(glm::vec3(skinColor[0], skinColor[1], skinColor[2]));
    _head.simulate(deltaTime, _isMine);
    
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
    _mouseRayOrigin = origin; _mouseRayDirection = direction;    
}

void Avatar::updateHandMovementAndTouching(float deltaTime) {

    // reset hand and arm positions according to hand movement
    glm::vec3 transformedHandMovement
    = _orientation.getRight() *  _movedHandOffset.x * 2.0f
    + _orientation.getUp()	  * -_movedHandOffset.y * 1.0f
    + _orientation.getFront() * -_movedHandOffset.y * 1.0f;
    
    _joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position += transformedHandMovement;
            
    if (_isMine) {
        _avatarTouch.setMyBodyPosition(_position);
                
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
                //printLog("Angle between: %f\n", angleBetween(vectorBetweenUs, _orientation.getFront()));
                
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

            _avatarTouch.setYourBodyPosition(_interactingOther->_position);   
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
        }
    }//if (_isMine)
    
    //constrain right arm length and re-adjust elbow position as it bends
    // NOTE - the following must be called on all avatars - not just _isMine
    updateArmIKAndConstraints(deltaTime);
    
    //Set right hand position and state to be transmitted, and also tell AvatarTouch about it
    if (_isMine) {
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
    
        /*
        if (jointCollision) {
            if (!_usingBodySprings) {
                _usingBodySprings = true;
                initializeBodySprings();
            }
        }
        */
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

void Avatar::setDisplayingHead(bool displayingHead) {
    _displayingHead = displayingHead;
}

static TextRenderer* textRenderer() {
    static TextRenderer* renderer = new TextRenderer(SANS_FONT_FAMILY, 24, -1, false, TextRenderer::SHADOW_EFFECT);
    return renderer;
}

void Avatar::setGravity(glm::vec3 gravity) {
    _gravity = gravity;
    _head.setGravity(_gravity);
}

void Avatar::render(bool lookingInMirror, glm::vec3 cameraPosition) {

    _cameraPosition = cameraPosition; // store this for use in various parts of the code

    if (usingBigSphereCollisionTest) {
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
    if (_isMine) {			
        _avatarTouch.render(_cameraPosition);
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
        
        glTranslatef(_position.x, _position.y + chatMessageHeight, _position.z);
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
        _joint[b].rotation            = glm::quat(0.0f, 0.0f, 0.0f, 0.0f);
        _joint[b].yaw                 = 0.0;
        _joint[b].pitch               = 0.0;
        _joint[b].roll                = 0.0;
        _joint[b].length              = 0.0;
        _joint[b].radius              = 0.0;
        _joint[b].touchForce          = 0.0;
        _joint[b].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS;
        _joint[b].orientation.setToIdentity();
    }
    
    // specify the parental hierarchy
    _joint[ AVATAR_JOINT_PELVIS		      ].parent = AVATAR_JOINT_NULL;
    _joint[ AVATAR_JOINT_TORSO            ].parent = AVATAR_JOINT_PELVIS;
    _joint[ AVATAR_JOINT_CHEST		      ].parent = AVATAR_JOINT_TORSO;
    _joint[ AVATAR_JOINT_NECK_BASE	      ].parent = AVATAR_JOINT_CHEST;
    _joint[ AVATAR_JOINT_HEAD_BASE        ].parent = AVATAR_JOINT_NECK_BASE;
    _joint[ AVATAR_JOINT_HEAD_TOP         ].parent = AVATAR_JOINT_HEAD_BASE;
    _joint[ AVATAR_JOINT_LEFT_COLLAR      ].parent = AVATAR_JOINT_CHEST;
    _joint[ AVATAR_JOINT_LEFT_SHOULDER    ].parent = AVATAR_JOINT_LEFT_COLLAR;
    _joint[ AVATAR_JOINT_LEFT_ELBOW	      ].parent = AVATAR_JOINT_LEFT_SHOULDER;
    _joint[ AVATAR_JOINT_LEFT_WRIST		  ].parent = AVATAR_JOINT_LEFT_ELBOW;
    _joint[ AVATAR_JOINT_LEFT_FINGERTIPS  ].parent = AVATAR_JOINT_LEFT_WRIST;
    _joint[ AVATAR_JOINT_RIGHT_COLLAR     ].parent = AVATAR_JOINT_CHEST;
    _joint[ AVATAR_JOINT_RIGHT_SHOULDER	  ].parent = AVATAR_JOINT_RIGHT_COLLAR;
    _joint[ AVATAR_JOINT_RIGHT_ELBOW	  ].parent = AVATAR_JOINT_RIGHT_SHOULDER;
    _joint[ AVATAR_JOINT_RIGHT_WRIST	  ].parent = AVATAR_JOINT_RIGHT_ELBOW;
    _joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].parent = AVATAR_JOINT_RIGHT_WRIST;
    _joint[ AVATAR_JOINT_LEFT_HIP		  ].parent = AVATAR_JOINT_PELVIS;
    _joint[ AVATAR_JOINT_LEFT_KNEE		  ].parent = AVATAR_JOINT_LEFT_HIP;
    _joint[ AVATAR_JOINT_LEFT_HEEL		  ].parent = AVATAR_JOINT_LEFT_KNEE;
    _joint[ AVATAR_JOINT_LEFT_TOES		  ].parent = AVATAR_JOINT_LEFT_HEEL;
    _joint[ AVATAR_JOINT_RIGHT_HIP		  ].parent = AVATAR_JOINT_PELVIS;
    _joint[ AVATAR_JOINT_RIGHT_KNEE		  ].parent = AVATAR_JOINT_RIGHT_HIP;
    _joint[ AVATAR_JOINT_RIGHT_HEEL		  ].parent = AVATAR_JOINT_RIGHT_KNEE;
    _joint[ AVATAR_JOINT_RIGHT_TOES		  ].parent = AVATAR_JOINT_RIGHT_HEEL;
    
    // specify the default pose position
    _joint[ AVATAR_JOINT_PELVIS           ].defaultPosePosition = glm::vec3(  0.0,   0.0,  0.0 );
    _joint[ AVATAR_JOINT_TORSO            ].defaultPosePosition = glm::vec3(  0.0,   0.09,  0.01 );
    _joint[ AVATAR_JOINT_CHEST            ].defaultPosePosition = glm::vec3(  0.0,   0.09,  0.01  );
    _joint[ AVATAR_JOINT_NECK_BASE        ].defaultPosePosition = glm::vec3(  0.0,   0.12,  -0.01 );
    _joint[ AVATAR_JOINT_HEAD_BASE        ].defaultPosePosition = glm::vec3(  0.0,   0.08,  0.00 );
    
    _joint[ AVATAR_JOINT_LEFT_COLLAR      ].defaultPosePosition = glm::vec3( -0.06,  0.04, -0.01 );
    _joint[ AVATAR_JOINT_LEFT_SHOULDER	  ].defaultPosePosition = glm::vec3( -0.05,  0.0,  -0.01 );
    _joint[ AVATAR_JOINT_LEFT_ELBOW       ].defaultPosePosition = glm::vec3(  0.0,  -0.16,  0.0  );
    _joint[ AVATAR_JOINT_LEFT_WRIST		  ].defaultPosePosition = glm::vec3(  0.0,  -0.117,  0.0  );
    _joint[ AVATAR_JOINT_LEFT_FINGERTIPS  ].defaultPosePosition = glm::vec3(  0.0,  -0.1,  0.0  );
    
    _joint[ AVATAR_JOINT_RIGHT_COLLAR     ].defaultPosePosition = glm::vec3(  0.06,  0.04, -0.01 );
    _joint[ AVATAR_JOINT_RIGHT_SHOULDER	  ].defaultPosePosition = glm::vec3(  0.05,  0.0,  -0.01 );
    _joint[ AVATAR_JOINT_RIGHT_ELBOW      ].defaultPosePosition = glm::vec3(  0.0,  -0.16,  0.0  );
    _joint[ AVATAR_JOINT_RIGHT_WRIST      ].defaultPosePosition = glm::vec3(  0.0,  -0.117,  0.0  );
    _joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].defaultPosePosition = glm::vec3(  0.0,  -0.1,  0.0  );
    
    _joint[ AVATAR_JOINT_LEFT_HIP		  ].defaultPosePosition = glm::vec3( -0.05,  0.0,  -0.02 );
    _joint[ AVATAR_JOINT_LEFT_KNEE		  ].defaultPosePosition = glm::vec3(  0.0,  -0.27,  0.02 );
    _joint[ AVATAR_JOINT_LEFT_HEEL		  ].defaultPosePosition = glm::vec3(  0.0,  -0.27, -0.01 );
    _joint[ AVATAR_JOINT_LEFT_TOES		  ].defaultPosePosition = glm::vec3(  0.0,   0.0,   0.05 );
    
    _joint[ AVATAR_JOINT_RIGHT_HIP		  ].defaultPosePosition = glm::vec3(  0.05,  0.0,  -0.02 );
    _joint[ AVATAR_JOINT_RIGHT_KNEE		  ].defaultPosePosition = glm::vec3(  0.0,  -0.27,  0.02 );
    _joint[ AVATAR_JOINT_RIGHT_HEEL		  ].defaultPosePosition = glm::vec3(  0.0,  -0.27, -0.01 );
    _joint[ AVATAR_JOINT_RIGHT_TOES		  ].defaultPosePosition = glm::vec3(  0.0,   0.0,   0.05 );
    
    // specify the radii of the joints
    _joint[ AVATAR_JOINT_PELVIS           ].radius = 0.07;
    _joint[ AVATAR_JOINT_TORSO            ].radius = 0.065;
    _joint[ AVATAR_JOINT_CHEST            ].radius = 0.08;
    _joint[ AVATAR_JOINT_NECK_BASE        ].radius = 0.03;
    _joint[ AVATAR_JOINT_HEAD_BASE        ].radius = 0.07;
    
    _joint[ AVATAR_JOINT_LEFT_COLLAR      ].radius = 0.04;
    _joint[ AVATAR_JOINT_LEFT_SHOULDER    ].radius = 0.03;
    _joint[ AVATAR_JOINT_LEFT_ELBOW	      ].radius = 0.02;
    _joint[ AVATAR_JOINT_LEFT_WRIST       ].radius = 0.02;
    _joint[ AVATAR_JOINT_LEFT_FINGERTIPS  ].radius = 0.01;
    
    _joint[ AVATAR_JOINT_RIGHT_COLLAR     ].radius = 0.04;
    _joint[ AVATAR_JOINT_RIGHT_SHOULDER	  ].radius = 0.03;
    _joint[ AVATAR_JOINT_RIGHT_ELBOW	  ].radius = 0.02;
    _joint[ AVATAR_JOINT_RIGHT_WRIST	  ].radius = 0.02;
    _joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].radius = 0.01;
    
    _joint[ AVATAR_JOINT_LEFT_HIP		  ].radius = 0.04;
    _joint[ AVATAR_JOINT_LEFT_KNEE		  ].radius = 0.025;
    _joint[ AVATAR_JOINT_LEFT_HEEL		  ].radius = 0.025;
    _joint[ AVATAR_JOINT_LEFT_TOES		  ].radius = 0.027;
    
    _joint[ AVATAR_JOINT_RIGHT_HIP		  ].radius = 0.04;
    _joint[ AVATAR_JOINT_RIGHT_KNEE		  ].radius = 0.025;
    _joint[ AVATAR_JOINT_RIGHT_HEEL		  ].radius = 0.025;
    _joint[ AVATAR_JOINT_RIGHT_TOES		  ].radius = 0.027;
    
    // specify the tightness of the springy positions as far as attraction to rigid body
    _joint[ AVATAR_JOINT_PELVIS           ].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS * 1.0;
    _joint[ AVATAR_JOINT_TORSO            ].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS * 0.8;	
    _joint[ AVATAR_JOINT_CHEST            ].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS * 0.5;
    _joint[ AVATAR_JOINT_NECK_BASE        ].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS * 0.4;
    _joint[ AVATAR_JOINT_HEAD_BASE        ].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS * 0.3;
    _joint[ AVATAR_JOINT_LEFT_COLLAR      ].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS * 0.5;
    _joint[ AVATAR_JOINT_LEFT_SHOULDER    ].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS * 0.5;
    _joint[ AVATAR_JOINT_LEFT_ELBOW       ].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS * 0.5;
    _joint[ AVATAR_JOINT_LEFT_WRIST       ].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS * 0.3;
    _joint[ AVATAR_JOINT_LEFT_FINGERTIPS  ].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS * 0.3;
    _joint[ AVATAR_JOINT_RIGHT_COLLAR     ].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS * 0.5;
    _joint[ AVATAR_JOINT_RIGHT_SHOULDER   ].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS * 0.5;
    _joint[ AVATAR_JOINT_RIGHT_ELBOW      ].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS * 0.5;
    _joint[ AVATAR_JOINT_RIGHT_WRIST      ].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS * 0.3;
	_joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS * 0.3;
    _joint[ AVATAR_JOINT_LEFT_HIP         ].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS;
    _joint[ AVATAR_JOINT_LEFT_KNEE        ].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS;
    _joint[ AVATAR_JOINT_LEFT_HEEL        ].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS;
    _joint[ AVATAR_JOINT_LEFT_TOES        ].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS;
    _joint[ AVATAR_JOINT_RIGHT_HIP        ].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS;
    _joint[ AVATAR_JOINT_RIGHT_KNEE       ].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS;
    _joint[ AVATAR_JOINT_RIGHT_HEEL       ].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS;
    _joint[ AVATAR_JOINT_RIGHT_TOES       ].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS;
    
    // to aid in hand-shaking and hand-holding, the right hand is not collidable
    _joint[ AVATAR_JOINT_RIGHT_ELBOW	  ].isCollidable = false;
    _joint[ AVATAR_JOINT_RIGHT_WRIST	  ].isCollidable = false;
    _joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].isCollidable = false; 
       
    // calculate bone length
    calculateBoneLengths();
    
    _pelvisStandingHeight = 
    _joint[ AVATAR_JOINT_LEFT_HEEL ].radius +
    _joint[ AVATAR_JOINT_LEFT_HEEL ].length +
    _joint[ AVATAR_JOINT_LEFT_KNEE ].length;
    //printf("_pelvisStandingHeight = %f\n", _pelvisStandingHeight);
    
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
    + _joint[ AVATAR_JOINT_RIGHT_WRIST	    ].length
    + _joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].length;
}

void Avatar::updateSkeleton() {
	
    // rotate body...
    _orientation.setToIdentity();
    _orientation.yaw  (_bodyYaw  );
    _orientation.pitch(_bodyPitch);
    _orientation.roll (_bodyRoll );
    
    // calculate positions of all bones by traversing the skeleton tree:
    for (int b = 0; b < NUM_AVATAR_JOINTS; b++) {
        if (_joint[b].parent == AVATAR_JOINT_NULL) {
            _joint[b].orientation.set(_orientation);
            _joint[b].position = _position;
        }
        else {
            _joint[b].orientation.set(_joint[ _joint[b].parent ].orientation);
            _joint[b].position = _joint[ _joint[b].parent ].position;
        }
        
        // if this is not my avatar, then hand position comes from transmitted data
        if (! _isMine) {
            _joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position = _handPosition;
        }
        
        // the following will be replaced by a proper rotation...close
        float xx = glm::dot(_joint[b].defaultPosePosition, _joint[b].orientation.getRight());
        float yy = glm::dot(_joint[b].defaultPosePosition, _joint[b].orientation.getUp	());
        float zz = glm::dot(_joint[b].defaultPosePosition, _joint[b].orientation.getFront());
        
        glm::vec3 rotatedJointVector(xx, yy, zz);
        
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


const glm::vec3& Avatar::getSpringyHeadPosition() const {
    return _joint[ AVATAR_JOINT_HEAD_BASE ].springyPosition;
}

const glm::vec3& Avatar::getHeadPosition() const {
    return _joint[ AVATAR_JOINT_HEAD_BASE ].position;
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

    glm::vec3 perpendicular = glm::cross(_orientation.getFront(),  armVector);
    
    newElbowPosition += perpendicular * (1.0f - (_maxArmLength / distance)) * ONE_HALF;
    _joint[ AVATAR_JOINT_RIGHT_ELBOW ].position = newElbowPosition;
    
    // set wrist position
    glm::vec3 vv(_joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position);
    vv -= _joint[ AVATAR_JOINT_RIGHT_ELBOW ].position;
    glm::vec3 newWristPosition = _joint[ AVATAR_JOINT_RIGHT_ELBOW ].position + vv * 0.7f;
    _joint[ AVATAR_JOINT_RIGHT_WRIST ].position = newWristPosition;
}


void Avatar::renderBody(bool lookingInMirror) {
    
    //  Render joint positions as spheres
    for (int b = 0; b < NUM_AVATAR_JOINTS; b++) {
        
        if (b == AVATAR_JOINT_HEAD_BASE) { // the head is rendered as a special case
            if (_displayingHead) {
                _head.render(lookingInMirror);
            }
        } else {
    
            //show direction vectors of the bone orientation
            //renderOrientationDirections(_joint[b].springyPosition, _joint[b].orientation, _joint[b].radius * 2.0);
            
            glColor3fv(skinColor);
            glPushMatrix();
            glTranslatef(_joint[b].springyPosition.x, _joint[b].springyPosition.y, _joint[b].springyPosition.z);
            glutSolidSphere(_joint[b].radius, 20.0f, 20.0f);
            glPopMatrix();
        }
        
        if (_joint[b].touchForce > 0.0f) {
        
            float alpha = _joint[b].touchForce * 0.2;
            float r = _joint[b].radius * 1.1f + 0.005f;
            glColor4f(0.5f, 0.2f, 0.2f, alpha);
            glPushMatrix();
            glTranslatef(_joint[b].springyPosition.x, _joint[b].springyPosition.y, _joint[b].springyPosition.z);
            glScalef(r, r, r);
            glutSolidSphere(1, 20, 20);
            glPopMatrix();
        }
    }
    
    for (int b = 1; b < NUM_AVATAR_JOINTS; b++) {
    if (_joint[b].parent != AVATAR_JOINT_NULL) 
        if (b != AVATAR_JOINT_HEAD_TOP) {
                
            // Render cone sections connecting the joint positions 
            glColor3fv(darkSkinColor);
            renderJointConnectingCone
            (
                _joint[_joint[b].parent ].springyPosition, 
                _joint[b                ].springyPosition, 
                _joint[_joint[b].parent ].radius * 0.8, 
                _joint[b                ].radius * 0.8
            );
        
            /*
            // Render lines connecting the joint positions
            glColor3f(0.4f, 0.5f, 0.6f);
            glLineWidth(3.0);
            glBegin(GL_LINE_STRIP);
            glVertex3fv(&_joint[ _joint[ b ].parent ].springyPosition.x);
            glVertex3fv(&_joint[ b ].springyPosition.x);
            glEnd();
            */
        }
    }
}

//
// Process UDP interface data from Android transmitter or Google Glass
//
void Avatar::processTransmitterData(unsigned char* packetData, int numBytes) {
    //  Read a packet from a transmitter app, process the data
    float
    accX, accY, accZ,           //  Measured acceleration
    graX, graY, graZ,           //  Gravity
    gyrX, gyrY, gyrZ,           //  Gyro velocity in radians/sec as (pitch, roll, yaw)
    linX, linY, linZ,           //  Linear Acceleration (less gravity)
    rot1, rot2, rot3, rot4;     //  Rotation of device:
                                //    rot1 = roll, ranges from -1 to 1, 0 = flat on table
                                //    rot2 = pitch, ranges from -1 to 1, 0 = flat on table
                                //    rot3 = yaw, ranges from -1 to 1
    char device[100];           //  Device ID
    
    enum deviceTypes            { DEVICE_GLASS, DEVICE_ANDROID, DEVICE_IPHONE, DEVICE_UNKNOWN };

    sscanf((char *)packetData,
           "tacc %f %f %f gra %f %f %f gyr %f %f %f lin %f %f %f rot %f %f %f %f dna \"%s",
           &accX, &accY, &accZ,
           &graX, &graY, &graZ,
           &gyrX, &gyrY, &gyrZ,
           &linX, &linY, &linZ,
           &rot1, &rot2, &rot3, &rot4, (char *)&device);
    
    // decode transmitter device type
    deviceTypes deviceType = DEVICE_UNKNOWN;
    if (strcmp(device, "ADR")) {
        deviceType = DEVICE_ANDROID;
    } else {
        deviceType = DEVICE_GLASS;
    }
    
    if (_transmitterPackets++ == 0) {
        // If first packet received, note time, turn head spring return OFF, get start rotation
        gettimeofday(&_transmitterTimer, NULL);
        if (deviceType == DEVICE_GLASS) {
            _head.setReturnToCenter(true);
            _head.setSpringScale(10.f);
            printLog("Using Google Glass to drive head, springs ON.\n");

        } else {
            _head.setReturnToCenter(false);
            printLog("Using Transmitter %s to drive head, springs OFF.\n", device);

        }
        //printLog("Packet: [%s]\n", packetData);
        //printLog("Version:  %s\n", device);
        
        _transmitterInitialReading = glm::vec3(rot3, rot2, rot1);
    }
    
    const int TRANSMITTER_COUNT = 100;
    if (_transmitterPackets % TRANSMITTER_COUNT == 0) {
        // Every 100 packets, record the observed Hz of the transmitter data
        timeval now;
        gettimeofday(&now, NULL);
        double msecsElapsed = diffclock(&_transmitterTimer, &now);
        _transmitterHz = static_cast<float>((double)TRANSMITTER_COUNT / (msecsElapsed / 1000.0));
        _transmitterTimer = now;
        printLog("Transmitter Hz: %3.1f\n", _transmitterHz);
    }
    //printLog("Gyr: %3.1f, %3.1f, %3.1f\n", glm::degrees(gyrZ), glm::degrees(-gyrX), glm::degrees(gyrY));
    //printLog("Rot: %3.1f, %3.1f, %3.1f, %3.1f\n", rot1, rot2, rot3, rot4);
    
    //  Update the head with the transmitter data
    glm::vec3 eulerAngles((rot3 - _transmitterInitialReading.x) * 180.f,
                          -(rot2 - _transmitterInitialReading.y) * 180.f,
                          (rot1 - _transmitterInitialReading.z) * 180.f);
    if (eulerAngles.x > 180.f) { eulerAngles.x -= 360.f; }
    if (eulerAngles.x < -180.f) { eulerAngles.x += 360.f; }
    
    glm::vec3 angularVelocity;
    if (deviceType != DEVICE_GLASS) {
        angularVelocity = glm::vec3(glm::degrees(gyrZ), glm::degrees(-gyrX), glm::degrees(gyrY));
        setHeadFromGyros(&eulerAngles, &angularVelocity,
                         (_transmitterHz == 0.f) ? 0.f : 1.f / _transmitterHz, 1.0);

    } else {
        angularVelocity = glm::vec3(glm::degrees(gyrY), glm::degrees(-gyrX), glm::degrees(-gyrZ));
        setHeadFromGyros(&eulerAngles, &angularVelocity,
                         (_transmitterHz == 0.f) ? 0.f : 1.f / _transmitterHz, 1000.0);
    }
}
//
// Process UDP data from version 2 Transmitter acting as Hand 
//
void Avatar::processTransmitterDataV2(unsigned char* packetData, int numBytes) {
    if (numBytes == 3 + sizeof(_transmitterHandLastRotationRates) +
                        sizeof(_transmitterHandLastAcceleration)) {
        memcpy(_transmitterHandLastRotationRates, packetData + 2,
               sizeof(_transmitterHandLastRotationRates));
        memcpy(_transmitterHandLastAcceleration, packetData + 3 +
               sizeof(_transmitterHandLastRotationRates),
               sizeof(_transmitterHandLastAcceleration));
        //  Convert from transmitter units to internal units
        for (int i = 0; i < 3; i++) {
            _transmitterHandLastRotationRates[i] *= 180.f / PI;
            _transmitterHandLastAcceleration[i] *= GRAVITY_EARTH;
        }
        if (!_isTransmitterV2Connected) {
            printf("Transmitter V2 Connected.\n");
            _isTransmitterV2Connected = true;
        }
    } else {
        printf("Transmitter V2 packet read error.\n");
    }
}

void Avatar::transmitterV2RenderLevels(int width, int height) {
    
    char val[50];
    const int LEVEL_CORNER_X = 10;
    const int LEVEL_CORNER_Y = 400;
    
    // Draw the numeric degree/sec values from the gyros
    sprintf(val, "Yaw   %4.1f", _transmitterHandLastRotationRates[1]);
    drawtext(LEVEL_CORNER_X, LEVEL_CORNER_Y, 0.10, 0, 1.0, 1, val, 0, 1, 0);
    sprintf(val, "Pitch %4.1f", _transmitterHandLastRotationRates[0]);
    drawtext(LEVEL_CORNER_X, LEVEL_CORNER_Y + 15, 0.10, 0, 1.0, 1, val, 0, 1, 0);
    sprintf(val, "Roll  %4.1f", _transmitterHandLastRotationRates[2]);
    drawtext(LEVEL_CORNER_X, LEVEL_CORNER_Y + 30, 0.10, 0, 1.0, 1, val, 0, 1, 0);
    sprintf(val, "X     %4.3f", _transmitterHandLastAcceleration[0]);
    drawtext(LEVEL_CORNER_X, LEVEL_CORNER_Y + 45, 0.10, 0, 1.0, 1, val, 0, 1, 0);
    sprintf(val, "Y     %4.3f", _transmitterHandLastAcceleration[1]);
    drawtext(LEVEL_CORNER_X, LEVEL_CORNER_Y + 60, 0.10, 0, 1.0, 1, val, 0, 1, 0);
    sprintf(val, "Z     %4.3f", _transmitterHandLastAcceleration[2]);
    drawtext(LEVEL_CORNER_X, LEVEL_CORNER_Y + 75, 0.10, 0, 1.0, 1, val, 0, 1, 0);
    
    //  Draw the levels as horizontal lines
    const int LEVEL_CENTER = 150;
    const float ACCEL_VIEW_SCALING = 50.f;
    glLineWidth(2.0);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_LINES);
    // Gyro rates
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y - 3);
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + _transmitterHandLastRotationRates[1], LEVEL_CORNER_Y - 3);
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 12);
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + _transmitterHandLastRotationRates[0], LEVEL_CORNER_Y + 12);
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 27);
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + _transmitterHandLastRotationRates[2], LEVEL_CORNER_Y + 27);
    // Acceleration
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 42);
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + (int)(_transmitterHandLastAcceleration[0] * ACCEL_VIEW_SCALING),
               LEVEL_CORNER_Y + 42);
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 57);
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + (int)(_transmitterHandLastAcceleration[1] * ACCEL_VIEW_SCALING),
               LEVEL_CORNER_Y + 57);
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 72);
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + (int)(_transmitterHandLastAcceleration[2] * ACCEL_VIEW_SCALING),
               LEVEL_CORNER_Y + 72);
    
    glEnd();
    //  Draw green vertical centerline
    glColor4f(0, 1, 0, 0.5);
    glBegin(GL_LINES);
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y - 6);
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 30);
    glEnd();
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
        _head.setYaw(eulerAngles->x);
        _head.setPitch(eulerAngles->y);
        _head.setRoll(eulerAngles->z);
    } else { 
        glm::vec3 angles(_head.getYaw(), _head.getPitch(), _head.getRoll());
        //  Increment by detected velocity 
        angles += (*angularVelocity) * deltaTime;
        //  Smooth to slowly follow absolute values
        angles = ((1.f - deltaTime / smoothingTime) * angles) + (deltaTime / smoothingTime) * (*eulerAngles);
        _head.setYaw(angles.x);
        _head.setPitch(angles.y);
        _head.setRoll(angles.z);
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
    
    int num = 10;
    
    glm::vec3 axis = glm::normalize(position2 - position1);
    float length = glm::length(axis);

    if (length > 0.0f) {
    
        glm::vec3 perpSin = glm::vec3(axis.y, axis.z, axis.x);
        glm::vec3 perpCos = glm::vec3(axis.z, axis.x, axis.y);

        float angle1 = 0.0;
        float angle2 = 0.0;

        for (int i = 0; i < num; i ++) {
        
            angle1 = angle2;
            angle2 = ((float)(i+1) / (float)num) * PI * 2.0;
            
            float s1 = sin(angle1);
            float s2 = sin(angle2);
            float c1 = cos(angle1);
            float c2 = cos(angle2);
        
            glm::vec3 p1a = position1 + perpSin * s1 * radius1 + perpCos * c1 * radius1;  
            glm::vec3 p1b = position1 + perpSin * s2 * radius1 + perpCos * c2 * radius1; 
            glm::vec3 p2a = position2 + perpSin * s1 * radius2 + perpCos * c1 * radius2;   
            glm::vec3 p2b = position2 + perpSin * s2 * radius2 + perpCos * c2 * radius2;  

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




