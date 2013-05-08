//
//  Avatar.cpp
//  interface
//
//  Created by Philip Rosedale on 9/11/12.
//	adapted by Jeffrey Ventrella
//  Copyright (c) 2013 Physical, Inc.. All rights reserved.
//

#include <glm/glm.hpp>
#include <vector>
#include <lodepng.h>
#include <SharedUtil.h>
#include "Avatar.h"
#include "Log.h"
#include "ui/TextRenderer.h"
#include <AgentList.h>
#include <AgentTypes.h>
#include <PacketHeaders.h>

using namespace std;

const bool  BALLS_ON                      = false;
const bool  USING_AVATAR_GRAVITY          = true;
const float GRAVITY_SCALE                 = 10.0f;
const float BOUNCE                        = 0.3f;
const float DECAY                         = 0.1;
const float THRUST_MAG                    = 1200.0;
const float YAW_MAG                       = 500.0;
const float BODY_SPIN_FRICTION            = 5.0;
const float BODY_UPRIGHT_FORCE            = 10.0;
const float BODY_PITCH_WHILE_WALKING      = 30.0;
const float BODY_ROLL_WHILE_TURNING       = 0.1;
const float LIN_VEL_DECAY                 = 5.0;
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

float skinColor [] = {1.0, 0.84, 0.66};
float lightBlue [] = {0.7, 0.8, 1.0};
float browColor [] = {210.0/255.0, 105.0/255.0, 30.0/255.0};
float mouthColor[] = {1, 0, 0};

float BrowRollAngle [5] = {0, 15, 30, -30, -15};
float BrowPitchAngle[3] = {-70, -60, -50};
float eyeColor      [3] = {1,1,1};

float MouthWidthChoices[3] = {0.5, 0.77, 0.3};

float browWidth = 0.8;
float browThickness = 0.16;

bool usingBigSphereCollisionTest = true;

char iris_texture_file[] = "resources/images/green_eye.png";

float chatMessageScale = 0.0015;
float chatMessageHeight = 0.45;

vector<unsigned char> iris_texture;
unsigned int iris_texture_width = 512;
unsigned int iris_texture_height = 256;

Avatar::Avatar(bool isMine) {
    
    _orientation.setToIdentity();
    
    _velocity                   = glm::vec3(0.0f, 0.0f, 0.0f);
    _thrust                     = glm::vec3(0.0f, 0.0f, 0.0f);
    _rotation                   = glm::quat(0.0f, 0.0f, 0.0f, 0.0f);
    _bodyYaw                    = -90.0;
    _bodyPitch                  = 0.0;
    _bodyRoll                   = 0.0;
    _bodyPitchDelta             = 0.0;
    _bodyYawDelta               = 0.0;
    _bodyRollDelta              = 0.0;
    _mousePressed               = false;
    _mode                       = AVATAR_MODE_STANDING;
    _isMine                     = isMine;
    _maxArmLength               = 0.0;
    _transmitterHz              = 0.0;
    _transmitterPackets         = 0;
    _transmitterIsFirstData     = true;
    _transmitterInitialReading  = glm::vec3(0.f, 0.f, 0.f);
    _speed                      = 0.0;
    _pelvisStandingHeight       = 0.0f;
    _displayingHead             = true;
    _TEST_bigSphereRadius       = 0.4f;
    _TEST_bigSpherePosition     = glm::vec3(5.0f, _TEST_bigSphereRadius, 5.0f);
    
    for (int i = 0; i < MAX_DRIVE_KEYS; i++) _driveKeys[i] = false;
    
    _head.pupilSize             = 0.10;
    _head.interPupilDistance    = 0.6;
    _head.interBrowDistance     = 0.75;
    _head.nominalPupilSize      = 0.10;
    _head.pitchRate             = 0.0;
    _head.yawRate               = 0.0;
    _head.rollRate              = 0.0;
    _head.eyebrowPitch[0]       = -30;
    _head.eyebrowPitch[1]       = -30;
    _head.eyebrowRoll [0]       = 20;
    _head.eyebrowRoll [1]       = -20;
    _head.mouthPitch            = 0;
    _head.mouthYaw              = 0;
    _head.mouthWidth            = 1.0;
    _head.mouthHeight           = 0.2;
    _head.eyeballPitch[0]       = 0;
    _head.eyeballPitch[1]       = 0;
    _head.eyeballScaleX         = 1.2;
    _head.eyeballScaleY         = 1.5;
    _head.eyeballScaleZ         = 1.0;
    _head.eyeballYaw[0]         = 0;
    _head.eyeballYaw[1]         = 0;
    _head.pitchTarget           = 0;
    _head.yawTarget             = 0;
    _head.noiseEnvelope         = 1.0;
    _head.pupilConverge         = 10.0;
    _head.leanForward           = 0.0;
    _head.leanSideways          = 0.0;
    _head.eyeContact            = 1;
    _head.eyeContactTarget      = LEFT_EYE;
    _head.scale                 = 1.0;
    _head.audioAttack           = 0.0;
    _head.averageLoudness       = 0.0;
    _head.lastLoudness          = 0.0;
    _head.browAudioLift         = 0.0;
    _head.noise                 = 0;
    _head.returnSpringScale     = 1.0;
    _movedHandOffset            = glm::vec3(0.0f, 0.0f, 0.0f);
    _renderYaw                  = 0.0;
    _renderPitch                = 0.0;
    _sphere                     = NULL;
    _handHoldingPosition        = glm::vec3(0.0f, 0.0f, 0.0f);
    _distanceToNearestAvatar    = std::numeric_limits<float>::max();
    _gravity                    = glm::vec3(0.0f, -1.0f, 0.0f); // default

    initializeSkeleton();
    
    _avatarTouch.setReachableRadius(0.6);
    
    if (iris_texture.size() == 0) {
        switchToResourcesParentIfRequired();
        unsigned error = lodepng::decode(iris_texture, iris_texture_width, iris_texture_height, iris_texture_file);
        if (error != 0) {
            printLog("error %u: %s\n", error, lodepng_error_text(error));
        }
    }
    
    if (BALLS_ON)   { _balls = new Balls(100); }
    else            { _balls = NULL; }
}

Avatar::Avatar(const Avatar &otherAvatar) {
    
    _velocity                    = otherAvatar._velocity;
    _thrust                      = otherAvatar._thrust;
    _rotation                    = otherAvatar._rotation;
    _bodyYaw                     = otherAvatar._bodyYaw;
    _bodyPitch                   = otherAvatar._bodyPitch;
    _bodyRoll                    = otherAvatar._bodyRoll;
    _bodyPitchDelta              = otherAvatar._bodyPitchDelta;
    _bodyYawDelta                = otherAvatar._bodyYawDelta;
    _bodyRollDelta               = otherAvatar._bodyRollDelta;
    _mousePressed                = otherAvatar._mousePressed;
    _mode                        = otherAvatar._mode;
    _isMine                      = otherAvatar._isMine;
    _renderYaw                   = otherAvatar._renderYaw;
    _renderPitch                 = otherAvatar._renderPitch;
    _maxArmLength                = otherAvatar._maxArmLength;
    _transmitterTimer            = otherAvatar._transmitterTimer;
    _transmitterIsFirstData      = otherAvatar._transmitterIsFirstData;
    _transmitterTimeLastReceived = otherAvatar._transmitterTimeLastReceived;
    _transmitterHz               = otherAvatar._transmitterHz;
    _transmitterInitialReading   = otherAvatar._transmitterInitialReading;
    _transmitterPackets          = otherAvatar._transmitterPackets;
    _TEST_bigSphereRadius        = otherAvatar._TEST_bigSphereRadius;
    _TEST_bigSpherePosition      = otherAvatar._TEST_bigSpherePosition;
    _movedHandOffset             = otherAvatar._movedHandOffset;
    
    _orientation.set(otherAvatar._orientation);
    
    _sphere = NULL;
    
    initializeSkeleton();
    
    for (int i = 0; i < MAX_DRIVE_KEYS; i++) _driveKeys[i] = otherAvatar._driveKeys[i];
    
    _head.pupilSize          = otherAvatar._head.pupilSize;
    _head.interPupilDistance = otherAvatar._head.interPupilDistance;
    _head.interBrowDistance  = otherAvatar._head.interBrowDistance;
    _head.nominalPupilSize   = otherAvatar._head.nominalPupilSize;
    _head.yawRate            = otherAvatar._head.yawRate;
    _head.pitchRate          = otherAvatar._head.pitchRate;
    _head.rollRate           = otherAvatar._head.rollRate;
    _head.eyebrowPitch[0]    = otherAvatar._head.eyebrowPitch[0];
    _head.eyebrowPitch[1]    = otherAvatar._head.eyebrowPitch[1];
    _head.eyebrowRoll [0]    = otherAvatar._head.eyebrowRoll [0];
    _head.eyebrowRoll [1]    = otherAvatar._head.eyebrowRoll [1];
    _head.mouthPitch         = otherAvatar._head.mouthPitch;
    _head.mouthYaw           = otherAvatar._head.mouthYaw;
    _head.mouthWidth         = otherAvatar._head.mouthWidth;
    _head.mouthHeight        = otherAvatar._head.mouthHeight;
    _head.eyeballPitch[0]    = otherAvatar._head.eyeballPitch[0];
    _head.eyeballPitch[1]    = otherAvatar._head.eyeballPitch[1];
    _head.eyeballScaleX      = otherAvatar._head.eyeballScaleX;
    _head.eyeballScaleY      = otherAvatar._head.eyeballScaleY;
    _head.eyeballScaleZ      = otherAvatar._head.eyeballScaleZ;
    _head.eyeballYaw[0]      = otherAvatar._head.eyeballYaw[0];
    _head.eyeballYaw[1]      = otherAvatar._head.eyeballYaw[1];
    _head.pitchTarget        = otherAvatar._head.pitchTarget;
    _head.yawTarget          = otherAvatar._head.yawTarget;
    _head.noiseEnvelope      = otherAvatar._head.noiseEnvelope;
    _head.pupilConverge      = otherAvatar._head.pupilConverge;
    _head.leanForward        = otherAvatar._head.leanForward;
    _head.leanSideways       = otherAvatar._head.leanSideways;
    _head.eyeContact         = otherAvatar._head.eyeContact;
    _head.eyeContactTarget   = otherAvatar._head.eyeContactTarget;
    _head.scale              = otherAvatar._head.scale;
    _head.audioAttack        = otherAvatar._head.audioAttack;
    _head.averageLoudness    = otherAvatar._head.averageLoudness;
    _head.lastLoudness       = otherAvatar._head.lastLoudness;
    _head.browAudioLift      = otherAvatar._head.browAudioLift;
    _head.noise              = otherAvatar._head.noise;
    _distanceToNearestAvatar = otherAvatar._distanceToNearestAvatar;
    
    initializeSkeleton();
    
    if (iris_texture.size() == 0) {
        switchToResourcesParentIfRequired();
        unsigned error = lodepng::decode(iris_texture, iris_texture_width, iris_texture_height, iris_texture_file);
        if (error != 0) {
            printLog("error %u: %s\n", error, lodepng_error_text(error));
        }
    }
}

Avatar::~Avatar()  {
    if (_sphere != NULL) {
        gluDeleteQuadric(_sphere);
    }
}

Avatar* Avatar::clone() const {
    return new Avatar(*this);
}

void Avatar::reset() {
    _headPitch = _headYaw = _headRoll = 0;
    _head.leanForward = _head.leanSideways = 0;
}


//  Update avatar head rotation with sensor data
void Avatar::UpdateGyros(float frametime, SerialInterface* serialInterface, glm::vec3* gravity) {
    float measuredPitchRate = 0.0f;
    float measuredRollRate = 0.0f;
    float measuredYawRate = 0.0f;
    
    if (serialInterface->active && USING_INVENSENSE_MPU9150) {
        measuredPitchRate = serialInterface->getLastPitchRate();
        measuredYawRate = serialInterface->getLastYawRate();
        measuredRollRate = serialInterface->getLastRollRate();
    } else {
        measuredPitchRate = serialInterface->getRelativeValue(HEAD_PITCH_RATE);
        measuredYawRate = serialInterface->getRelativeValue(HEAD_YAW_RATE);
        measuredRollRate = serialInterface->getRelativeValue(HEAD_ROLL_RATE);
    }
    
    //  Update avatar head position based on measured gyro rates
    const float MAX_PITCH = 45;
    const float MIN_PITCH = -45;
    const float MAX_YAW = 85;
    const float MIN_YAW = -85;
    const float MAX_ROLL = 50;
    const float MIN_ROLL = -50;
    
    addHeadPitch(measuredPitchRate * frametime);
    addHeadYaw(measuredYawRate * frametime);
    addHeadRoll(measuredRollRate * frametime);
    
    setHeadPitch(glm::clamp(getHeadPitch(), MIN_PITCH, MAX_PITCH));
    setHeadYaw(glm::clamp(getHeadYaw(), MIN_YAW, MAX_YAW));
    setHeadRoll(glm::clamp(getHeadRoll(), MIN_ROLL, MAX_ROLL));
}

float Avatar::getAbsoluteHeadYaw() const {
    return _bodyYaw + _headYaw;
}

void Avatar::addLean(float x, float z) {
    //Add lean as impulse
    _head.leanSideways += x;
    _head.leanForward  += z;
}

void Avatar::setLeanForward(float dist){
    _head.leanForward = dist;
}

void Avatar::setLeanSideways(float dist){
    _head.leanSideways = dist;
}

void Avatar::setMousePressed(bool mousePressed) {
	_mousePressed = mousePressed;
}

bool Avatar::getIsNearInteractingOther() { 
    return _avatarTouch.getAbleToReachOtherAvatar(); 
}

void Avatar::simulate(float deltaTime) {
    
    // update balls
    if (_balls) { _balls->simulate(deltaTime); }
    
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
    if (USING_AVATAR_GRAVITY) {
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
    float forwardComponentOfVelocity = glm::dot(_orientation.getFront(), _velocity);
    _bodyPitch += BODY_PITCH_WHILE_WALKING * deltaTime * forwardComponentOfVelocity;
    _bodyRoll  += BODY_ROLL_WHILE_TURNING  * deltaTime * _speed * _bodyYawDelta;
        
	// these forces keep the body upright...     
    float tiltDecay = 1.0 - BODY_UPRIGHT_FORCE * deltaTime;
    if  (tiltDecay < 0.0f) {tiltDecay = 0.0f;}     
    _bodyPitch *= tiltDecay;
    _bodyRoll  *= tiltDecay;

    // update position by velocity
    _position += _velocity * deltaTime;

	// decay velocity
    _velocity *= (1.0 - LIN_VEL_DECAY * deltaTime);
	
    // If someone is near, damp velocity as a function of closeness
    const float AVATAR_BRAKING_RANGE = 1.2f;
    const float AVATAR_BRAKING_STRENGTH = 25.f;
    if (_isMine && (_distanceToNearestAvatar < AVATAR_BRAKING_RANGE)) {
        _velocity *=
        (1.f - deltaTime * AVATAR_BRAKING_STRENGTH *
         (AVATAR_BRAKING_RANGE - _distanceToNearestAvatar));
    }

    // update head information
    updateHead(deltaTime);
    
    // use speed and angular velocity to determine walking vs. standing                                
	if (_speed + fabs(_bodyYawDelta) > 0.2) {
		_mode = AVATAR_MODE_WALKING;
	} else {
		_mode = AVATAR_MODE_INTERACTING;
	}
}


void Avatar::updateHandMovementAndTouching(float deltaTime) {

    // reset hand and arm positions according to hand movement
    glm::vec3 transformedHandMovement
    = _orientation.getRight() *  _movedHandOffset.x * 2.0f
    + _orientation.getUp()	  * -_movedHandOffset.y * 1.0f
    + _orientation.getFront() * -_movedHandOffset.y * 1.0f;
    
    _joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position += transformedHandMovement;
            
    if (_isMine)
    {
        _avatarTouch.setMyBodyPosition(_position);
        
        Avatar * _interactingOther = NULL;
        float closestDistance = std::numeric_limits<float>::max();
    
        //loop through all the other avatars for potential interactions...
        AgentList* agentList = AgentList::getInstance();
        for (AgentList::iterator agent = agentList->begin(); agent != agentList->end(); agent++) {
            if (agent->getLinkedData() != NULL && agent->getType() == AGENT_TYPE_AVATAR) {
                Avatar *otherAvatar = (Avatar *)agent->getLinkedData();
                 
                /*
                //  Test:  Show angle between your fwd vector and nearest avatar
                glm::vec3 vectorBetweenUs = otherAvatar->getJointPosition(AVATAR_JOINT_PELVIS) -
                                getJointPosition(AVATAR_JOINT_PELVIS);
                glm::vec3 myForwardVector = _orientation.getFront();
                printLog("Angle between: %f\n", angleBetween(&vectorBetweenUs, &myForwardVector));
                */
                
                // test whether shoulders are close enough to allow for reaching to touch hands
                glm::vec3 v(_position - otherAvatar->_position);
                float distance = glm::length(v);
                if (distance < closestDistance) {
                    closestDistance = distance;
                    _interactingOther = otherAvatar;
                }
            }
        }

        if (_interactingOther) {
            _avatarTouch.setYourBodyPosition(_interactingOther->_position);   
            _avatarTouch.setYourHandPosition(_joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].springyPosition);   
            _avatarTouch.setYourHandState   (_interactingOther->_handState);   
        }
        
    }//if (_isMine)
    
    //constrain right arm length and re-adjust elbow position as it bends
    // NOTE - the following must be called on all avatars - not just _isMine
    updateArmIKAndConstraints(deltaTime);
    
    if (_isMine) {
        //Set the vector we send for hand position to other people to be our right hand
        setHandPosition(_joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position);
     
        if (_mousePressed) {
            _handState = 1;
        } else {
            _handState = 0;
        }
        
        _avatarTouch.setMyHandState(_handState);
        
        if (_handState == 1) {
            _avatarTouch.setMyHandPosition(_joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].springyPosition);
        }
    }
}

void Avatar::updateHead(float deltaTime) {

    //apply the head lean values to the springy position...
    if (fabs(_head.leanSideways + _head.leanForward) > 0.0f) {
        glm::vec3 headLean = 
        _orientation.getRight() * _head.leanSideways +
        _orientation.getFront() * _head.leanForward;
        _joint[ AVATAR_JOINT_HEAD_BASE ].springyPosition += headLean;
    }
    
    //  Decay head back to center if turned on
    if (_returnHeadToCenter) {
        //  Decay back toward center
        _headPitch *= (1.0f - DECAY * _head.returnSpringScale * 2 * deltaTime);
        _headYaw   *= (1.0f - DECAY * _head.returnSpringScale * 2 * deltaTime);
        _headRoll  *= (1.0f - DECAY * _head.returnSpringScale * 2 * deltaTime);
    }
    
    //  For invensense gyro, decay only slightly when roughly centered
    if (USING_INVENSENSE_MPU9150) {
        const float RETURN_RANGE = 5.0;
        const float RETURN_STRENGTH = 1.0;
        if (fabs(_headPitch) < RETURN_RANGE) { _headPitch *= (1.0f - RETURN_STRENGTH * deltaTime); }
        if (fabs(_headYaw) < RETURN_RANGE) { _headYaw *= (1.0f - RETURN_STRENGTH * deltaTime); }
        if (fabs(_headRoll) < RETURN_RANGE) { _headRoll *= (1.0f - RETURN_STRENGTH * deltaTime); }

    }
    
    if (_head.noise) {
        //  Move toward new target
        _headPitch += (_head.pitchTarget - _headPitch) * 10 * deltaTime; // (1.f - DECAY*deltaTime)*Pitch + ;
        _headYaw   += (_head.yawTarget   - _headYaw  ) * 10 * deltaTime; // (1.f - DECAY*deltaTime);
        _headRoll *= 1.f - (DECAY * deltaTime);
    }
    
    _head.leanForward  *= (1.f - DECAY * 30 * deltaTime);
    _head.leanSideways *= (1.f - DECAY * 30 * deltaTime);
        
    //  Update where the avatar's eyes are
    //
    //  First, decide if we are making eye contact or not
    if (randFloat() < 0.005) {
        _head.eyeContact = !_head.eyeContact;
        _head.eyeContact = 1;
        if (!_head.eyeContact) {
            //  If we just stopped making eye contact,move the eyes markedly away
            _head.eyeballPitch[0] = _head.eyeballPitch[1] = _head.eyeballPitch[0] + 5.0 + (randFloat() - 0.5) * 10;
            _head.eyeballYaw  [0] = _head.eyeballYaw  [1] = _head.eyeballYaw  [0] + 5.0 + (randFloat() - 0.5) * 5;
        } else {
            //  If now making eye contact, turn head to look right at viewer
            SetNewHeadTarget(0,0);
        }
    }
    
    const float DEGREES_BETWEEN_VIEWER_EYES = 3;
    const float DEGREES_TO_VIEWER_MOUTH = 7;
    
    if (_head.eyeContact) {
        //  Should we pick a new eye contact target?
        if (randFloat() < 0.01) {
            //  Choose where to look next
            if (randFloat() < 0.1) {
                _head.eyeContactTarget = MOUTH;
            } else {
                if (randFloat() < 0.5) _head.eyeContactTarget = LEFT_EYE; else _head.eyeContactTarget = RIGHT_EYE;
            }
        }
        //  Set eyeball pitch and yaw to make contact
        float eye_target_yaw_adjust = 0;
        float eye_target_pitch_adjust = 0;
        if (_head.eyeContactTarget == LEFT_EYE) eye_target_yaw_adjust = DEGREES_BETWEEN_VIEWER_EYES;
        if (_head.eyeContactTarget == RIGHT_EYE) eye_target_yaw_adjust = -DEGREES_BETWEEN_VIEWER_EYES;
        if (_head.eyeContactTarget == MOUTH) eye_target_pitch_adjust = DEGREES_TO_VIEWER_MOUTH;
        
        _head.eyeballPitch[0] = _head.eyeballPitch[1] = -_headPitch + eye_target_pitch_adjust;
        _head.eyeballYaw[0] = _head.eyeballYaw[1] = -_headYaw + eye_target_yaw_adjust;
    }
    
    if (_head.noise)
    {
        _headPitch += (randFloat() - 0.5) * 0.2 * _head.noiseEnvelope;
        _headYaw += (randFloat() - 0.5) * 0.3 *_head.noiseEnvelope;
        //PupilSize += (randFloat() - 0.5) * 0.001*NoiseEnvelope;
        
        if (randFloat() < 0.005) _head.mouthWidth = MouthWidthChoices[rand()%3];
        
        if (!_head.eyeContact) {
            if (randFloat() < 0.01)  _head.eyeballPitch[0] = _head.eyeballPitch[1] = (randFloat() - 0.5) * 20;
            if (randFloat() < 0.01)  _head.eyeballYaw[0] = _head.eyeballYaw[1] = (randFloat()- 0.5) * 10;
        }
        
        if ((randFloat() < 0.005) && (fabs(_head.pitchTarget - _headPitch) < 1.0) && (fabs(_head.yawTarget - _headYaw) < 1.0)) {
            SetNewHeadTarget((randFloat()-0.5) * 20.0, (randFloat()-0.5) * 45.0);
        }
        
        if (0) {
            
            //  Pick new target
            _head.pitchTarget = (randFloat() - 0.5) * 45;
            _head.yawTarget = (randFloat() - 0.5) * 22;
        }
        if (randFloat() < 0.01)
        {
            _head.eyebrowPitch[0] = _head.eyebrowPitch[1] = BrowPitchAngle[rand()%3];
            _head.eyebrowRoll [0] = _head.eyebrowRoll[1] = BrowRollAngle[rand()%5];
            _head.eyebrowRoll [1] *=-1;
        }
    }
    
    //  Update audio trailing average for rendering facial animations
    const float AUDIO_AVERAGING_SECS = 0.05;
    _head.averageLoudness = (1.f - deltaTime / AUDIO_AVERAGING_SECS) * _head.averageLoudness +
                            (deltaTime / AUDIO_AVERAGING_SECS) * _audioLoudness;
}





float Avatar::getHeight() {
    return _height;
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
    static TextRenderer* renderer = new TextRenderer(SANS_FONT_FAMILY, 24);
    return renderer;
}

void Avatar::setGravity(glm::vec3 gravity) {
    _gravity = gravity;
}

void Avatar::render(bool lookingInMirror, glm::vec3 cameraPosition) {

    // render a simple round on the ground projected down from the avatar's position
    renderDiskShadow(_position, glm::vec3(0.0f, 1.0f, 0.0f), 0.1f, 0.2f);

    /*
    // show avatar position
    glColor4f(0.5f, 0.5f, 0.5f, 0.6);
    glPushMatrix();
    glTranslatef(_position.x, _position.y, _position.z);
    glScalef(0.03, 0.03, 0.03);
    glutSolidSphere(1, 10, 10);
    glPopMatrix();
    */
    
    if (usingBigSphereCollisionTest) {
        // show TEST big sphere
        glColor4f(0.5f, 0.6f, 0.8f, 0.7);
        glPushMatrix();
        glTranslatef(_TEST_bigSpherePosition.x, _TEST_bigSpherePosition.y, _TEST_bigSpherePosition.z);
        glScalef(_TEST_bigSphereRadius, _TEST_bigSphereRadius, _TEST_bigSphereRadius);
        glutSolidSphere(1, 20, 20);
        glPopMatrix();
    }
    
    //render body
    renderBody();
    
    // render head
    if (_displayingHead) {
        renderHead(lookingInMirror);
    }
    
    // if this is my avatar, then render my interactions with the other avatar
    if (_isMine) {			
        _avatarTouch.render(cameraPosition);
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
        
        glPopMatrix();
    }
}

void Avatar::renderHead(bool lookingInMirror) {
    int side = 0;
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_RESCALE_NORMAL);
    
	// show head orientation
	//renderOrientationDirections(_joint[ AVATAR_JOINT_HEAD_BASE ].springyPosition, _joint[ AVATAR_JOINT_HEAD_BASE ].orientation, 0.2f);
    
    glPushMatrix();
    
        glTranslatef(_joint[ AVATAR_JOINT_HEAD_BASE ].springyPosition.x,
                     _joint[ AVATAR_JOINT_HEAD_BASE ].springyPosition.y,
                     _joint[ AVATAR_JOINT_HEAD_BASE ].springyPosition.z);
	
    glScalef
    (
        _joint[ AVATAR_JOINT_HEAD_BASE ].radius,
        _joint[ AVATAR_JOINT_HEAD_BASE ].radius,
        _joint[ AVATAR_JOINT_HEAD_BASE ].radius
    );
    
    
    if (lookingInMirror) {
        glRotatef(_bodyYaw   - _headYaw,   0, 1, 0);
        //glRotatef(_bodyPitch + _headPitch, 1, 0, 0);
        //glRotatef(_bodyRoll  - _headRoll,  0, 0, 1);
        // don't let body pitch and roll affect the head..
        glRotatef(_headPitch, 1, 0, 0);   
        glRotatef(-_headRoll,  0, 0, 1);
    } else {
        glRotatef(_bodyYaw   + _headYaw,   0, 1, 0);
        //glRotatef(_bodyPitch + _headPitch, 1, 0, 0);
        //glRotatef(_bodyRoll  + _headRoll,  0, 0, 1);
        // don't let body pitch and roll affect the head..
        glRotatef(_headPitch, 1, 0, 0);
        glRotatef(_headRoll,  0, 0, 1);
    }
    
    //glScalef(2.0, 2.0, 2.0);
    glColor3fv(skinColor);
    
    glutSolidSphere(1, 30, 30);
    
    //  Ears
    glPushMatrix();
    glTranslatef(1.0, 0, 0);
    for(side = 0; side < 2; side++) {
        glPushMatrix();
        glScalef(0.3, 0.65, .65);
        glutSolidSphere(0.5, 30, 30);
        glPopMatrix();
        glTranslatef(-2.0, 0, 0);
    }
    glPopMatrix();
    
    
    //  Update audio attack data for facial animation (eyebrows and mouth)
    _head.audioAttack = 0.9 * _head.audioAttack + 0.1 * fabs(_audioLoudness - _head.lastLoudness);
    _head.lastLoudness = _audioLoudness;
    
    
    const float BROW_LIFT_THRESHOLD = 100;
    if (_head.audioAttack > BROW_LIFT_THRESHOLD)
        _head.browAudioLift += sqrt(_head.audioAttack) / 1000.0;
    
    _head.browAudioLift *= .90;
    
    
    //  Render Eyebrows
    glPushMatrix();
    glTranslatef(-_head.interBrowDistance / 2.0,0.4,0.45);
    for(side = 0; side < 2; side++) {
        glColor3fv(browColor);
        glPushMatrix();
        glTranslatef(0, 0.35 + _head.browAudioLift, 0);
        glRotatef(_head.eyebrowPitch[side]/2.0, 1, 0, 0);
        glRotatef(_head.eyebrowRoll[side]/2.0, 0, 0, 1);
        glScalef(browWidth, browThickness, 1);
        glutSolidCube(0.5);
        glPopMatrix();
        glTranslatef(_head.interBrowDistance, 0, 0);
    }
    glPopMatrix();
    
    // Mouth
    glPushMatrix();
    glTranslatef(0,-0.35,0.75);
    glColor3f(0,0,0);
    glRotatef(_head.mouthPitch, 1, 0, 0);
    glRotatef(_head.mouthYaw, 0, 0, 1);
    glScalef(_head.mouthWidth*(.7 + sqrt(_head.averageLoudness)/60.0), _head.mouthHeight*(1.0 + sqrt(_head.averageLoudness)/30.0), 1);
    glutSolidCube(0.5);
    glPopMatrix();
    
    glTranslatef(0, 1.0, 0);
    
    glTranslatef(-_head.interPupilDistance/2.0,-0.68,0.7);
    // Right Eye
    glRotatef(-10, 1, 0, 0);
    glColor3fv(eyeColor);
    glPushMatrix();
    {
        glTranslatef(_head.interPupilDistance/10.0, 0, 0.05);
        glRotatef(20, 0, 0, 1);
        glScalef(_head.eyeballScaleX, _head.eyeballScaleY, _head.eyeballScaleZ);
        glutSolidSphere(0.25, 30, 30);
    }
    glPopMatrix();
    
    // Right Pupil
    if (_sphere == NULL) {
        _sphere = gluNewQuadric();
        gluQuadricTexture(_sphere, GL_TRUE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gluQuadricOrientation(_sphere, GLU_OUTSIDE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, iris_texture_width, iris_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &iris_texture[0]);
    }
    
    glPushMatrix();
    {
        glRotatef(_head.eyeballPitch[1], 1, 0, 0);
        glRotatef(_head.eyeballYaw[1] + _headYaw + _head.pupilConverge, 0, 1, 0);
        glTranslatef(0,0,.35);
        glRotatef(-75,1,0,0);
        glScalef(1.0, 0.4, 1.0);
        
        glEnable(GL_TEXTURE_2D);
        gluSphere(_sphere, _head.pupilSize, 15, 15);
        glDisable(GL_TEXTURE_2D);
    }
    
    glPopMatrix();
    // Left Eye
    glColor3fv(eyeColor);
    glTranslatef(_head.interPupilDistance, 0, 0);
    glPushMatrix();
    {
        glTranslatef(-_head.interPupilDistance/10.0, 0, .05);
        glRotatef(-20, 0, 0, 1);
        glScalef(_head.eyeballScaleX, _head.eyeballScaleY, _head.eyeballScaleZ);
        glutSolidSphere(0.25, 30, 30);
    }
    glPopMatrix();
    // Left Pupil
    glPushMatrix();
    {
        glRotatef(_head.eyeballPitch[0], 1, 0, 0);
        glRotatef(_head.eyeballYaw[0] + _headYaw - _head.pupilConverge, 0, 1, 0);
        glTranslatef(0, 0, .35);
        glRotatef(-75, 1, 0, 0);
        glScalef(1.0, 0.4, 1.0);
        
        glEnable(GL_TEXTURE_2D);
        gluSphere(_sphere, _head.pupilSize, 15, 15);
        glDisable(GL_TEXTURE_2D);
    }
    
    glPopMatrix();
    
    
    glPopMatrix();
 }

void Avatar::setHandMovementValues(glm::vec3 handOffset) {
	_movedHandOffset = handOffset;
}

AvatarMode Avatar::getMode() {
	return _mode;
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
    _joint[ AVATAR_JOINT_PELVIS           ].defaultPosePosition = glm::vec3(  0.0,   0.0,   0.0 );
    _joint[ AVATAR_JOINT_TORSO            ].defaultPosePosition = glm::vec3(  0.0,   0.08,  0.01 );
    _joint[ AVATAR_JOINT_CHEST            ].defaultPosePosition = glm::vec3(  0.0,   0.09,  0.0  );
    _joint[ AVATAR_JOINT_NECK_BASE        ].defaultPosePosition = glm::vec3(  0.0,   0.1,  -0.01 );
    _joint[ AVATAR_JOINT_HEAD_BASE        ].defaultPosePosition = glm::vec3(  0.0,   0.08,  0.01 );
    _joint[ AVATAR_JOINT_LEFT_COLLAR      ].defaultPosePosition = glm::vec3( -0.06,  0.04, -0.01 );
    _joint[ AVATAR_JOINT_LEFT_SHOULDER	  ].defaultPosePosition = glm::vec3( -0.03,  0.0,  -0.01 );
    _joint[ AVATAR_JOINT_LEFT_ELBOW       ].defaultPosePosition = glm::vec3(  0.0,  -0.13,  0.0  );
    _joint[ AVATAR_JOINT_LEFT_WRIST		  ].defaultPosePosition = glm::vec3(  0.0,  -0.11,  0.0  );
    _joint[ AVATAR_JOINT_LEFT_FINGERTIPS  ].defaultPosePosition = glm::vec3(  0.0,  -0.07,  0.0  );
    _joint[ AVATAR_JOINT_RIGHT_COLLAR     ].defaultPosePosition = glm::vec3(  0.06,  0.04, -0.01 );
    _joint[ AVATAR_JOINT_RIGHT_SHOULDER	  ].defaultPosePosition = glm::vec3(  0.03,  0.0,  -0.01 );
    _joint[ AVATAR_JOINT_RIGHT_ELBOW      ].defaultPosePosition = glm::vec3(  0.0,  -0.13,  0.0  );
    _joint[ AVATAR_JOINT_RIGHT_WRIST      ].defaultPosePosition = glm::vec3(  0.0,  -0.11,  0.0  );
    _joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].defaultPosePosition = glm::vec3(  0.0,  -0.07,  0.0  );
    _joint[ AVATAR_JOINT_LEFT_HIP		  ].defaultPosePosition = glm::vec3( -0.04,  0.0,  -0.02 );
    _joint[ AVATAR_JOINT_LEFT_KNEE		  ].defaultPosePosition = glm::vec3(  0.0,  -0.22,  0.02 );
    _joint[ AVATAR_JOINT_LEFT_HEEL		  ].defaultPosePosition = glm::vec3(  0.0,  -0.22, -0.01 );
    _joint[ AVATAR_JOINT_LEFT_TOES		  ].defaultPosePosition = glm::vec3(  0.0,   0.0,   0.05 );
    _joint[ AVATAR_JOINT_RIGHT_HIP		  ].defaultPosePosition = glm::vec3(  0.04,  0.0,  -0.02 );
    _joint[ AVATAR_JOINT_RIGHT_KNEE		  ].defaultPosePosition = glm::vec3(  0.0,  -0.22,  0.02 );
    _joint[ AVATAR_JOINT_RIGHT_HEEL		  ].defaultPosePosition = glm::vec3(  0.0,  -0.22, -0.01 );
    _joint[ AVATAR_JOINT_RIGHT_TOES		  ].defaultPosePosition = glm::vec3(  0.0,   0.0,   0.05 );
    
    // specify the radii of the bone positions
    _joint[ AVATAR_JOINT_PELVIS           ].radius = 0.06;
    _joint[ AVATAR_JOINT_TORSO            ].radius = 0.055;
    _joint[ AVATAR_JOINT_CHEST            ].radius = 0.075;
    _joint[ AVATAR_JOINT_NECK_BASE        ].radius = 0.03;
    _joint[ AVATAR_JOINT_HEAD_BASE        ].radius = 0.07;
    _joint[ AVATAR_JOINT_LEFT_COLLAR      ].radius = 0.029;
    _joint[ AVATAR_JOINT_LEFT_SHOULDER    ].radius = 0.023;
    _joint[ AVATAR_JOINT_LEFT_ELBOW	      ].radius = 0.017;
    _joint[ AVATAR_JOINT_LEFT_WRIST       ].radius = 0.017;
    _joint[ AVATAR_JOINT_LEFT_FINGERTIPS  ].radius = 0.01;
    _joint[ AVATAR_JOINT_RIGHT_COLLAR     ].radius = 0.029;
    _joint[ AVATAR_JOINT_RIGHT_SHOULDER	  ].radius = 0.023;
    _joint[ AVATAR_JOINT_RIGHT_ELBOW	  ].radius = 0.015;
    _joint[ AVATAR_JOINT_RIGHT_WRIST	  ].radius = 0.015;
    _joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].radius = 0.01;
    _joint[ AVATAR_JOINT_LEFT_HIP		  ].radius = 0.03;
    _joint[ AVATAR_JOINT_LEFT_KNEE		  ].radius = 0.02;
    _joint[ AVATAR_JOINT_LEFT_HEEL		  ].radius = 0.015;
    _joint[ AVATAR_JOINT_LEFT_TOES		  ].radius = 0.02;
    _joint[ AVATAR_JOINT_RIGHT_HIP		  ].radius = 0.03;
    _joint[ AVATAR_JOINT_RIGHT_KNEE		  ].radius = 0.02;
    _joint[ AVATAR_JOINT_RIGHT_HEEL		  ].radius = 0.015;
    _joint[ AVATAR_JOINT_RIGHT_TOES		  ].radius = 0.02;
    
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
    
    // generate world positions
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
        
		_joint[b].springyVelocity += (_joint[b].position - _joint[b].springyPosition) * _joint[b].springBodyTightness * deltaTime;
        
        float decay = 1.0 - BODY_SPRING_DECAY * deltaTime;
		
        if (decay > 0.0) {
            _joint[b].springyVelocity *= decay;
        }
        else {
            _joint[b].springyVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
        }
        
        _joint[b].springyPosition += _joint[b].springyVelocity * deltaTime;
    }
}

const glm::vec3& Avatar::getHeadPosition() const {
    
    
    //if (_usingBodySprings) {
    //    return _joint[ AVATAR_JOINT_HEAD_BASE ].springyPosition;
    //}
    
    return _joint[ AVATAR_JOINT_HEAD_BASE ].springyPosition;
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


void Avatar::renderBody() {
    
    //  Render joint positions as spheres
    for (int b = 0; b < NUM_AVATAR_JOINTS; b++) {
        
        if (b != AVATAR_JOINT_HEAD_BASE) { // the head is rendered as a special case in "renderHead"
    
            //show direction vectors of the bone orientation
            //renderOrientationDirections(_joint[b].springyPosition, _joint[b].orientation, _joint[b].radius * 2.0);
            
            glColor3fv(skinColor);
            glPushMatrix();
            glTranslatef(_joint[b].springyPosition.x, _joint[b].springyPosition.y, _joint[b].springyPosition.z);
            glutSolidSphere(_joint[b].radius, 20.0f, 20.0f);
            glPopMatrix();
        }
    }
    
    // Render lines connecting the joint positions
    glColor3f(0.4f, 0.5f, 0.6f);
    glLineWidth(3.0);
    
    for (int b = 1; b < NUM_AVATAR_JOINTS; b++) {
    if (_joint[b].parent != AVATAR_JOINT_NULL) 
        if (b != AVATAR_JOINT_HEAD_TOP) {
            glBegin(GL_LINE_STRIP);
            glVertex3fv(&_joint[ _joint[ b ].parent ].springyPosition.x);
            glVertex3fv(&_joint[ b ].springyPosition.x);
            glEnd();
        }
    }
}

void Avatar::SetNewHeadTarget(float pitch, float yaw) {
    _head.pitchTarget = pitch;
    _head.yawTarget   = yaw;
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
            setHeadReturnToCenter(true);
            setHeadSpringScale(10.f);
            printLog("Using Google Glass to drive head, springs ON.\n");

        } else {
            setHeadReturnToCenter(false);
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
    float const MAX_YAW = 90.f;
    float const MIN_YAW = -90.f;
    float const MAX_PITCH = 85.f;
    float const MIN_PITCH = -85.f;
    float const MAX_ROLL = 90.f;
    float const MIN_ROLL = -90.f;
    
    if (deltaTime == 0.f) {
        //  On first sample, set head to absolute position
        setHeadYaw(eulerAngles->x);
        setHeadPitch(eulerAngles->y);
        setHeadRoll(eulerAngles->z);
    } else { 
        glm::vec3 angles(getHeadYaw(), getHeadPitch(), getHeadRoll());
        //  Increment by detected velocity 
        angles += (*angularVelocity) * deltaTime;
        //  Smooth to slowly follow absolute values
        angles = ((1.f - deltaTime / smoothingTime) * angles) + (deltaTime / smoothingTime) * (*eulerAngles);
        setHeadYaw(fmin(fmax(angles.x, MIN_YAW), MAX_YAW));
        setHeadPitch(fmin(fmax(angles.y, MIN_PITCH), MAX_PITCH));
        setHeadRoll(fmin(fmax(angles.z, MIN_ROLL), MAX_ROLL));
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

