//
//  Avatar.cpp
//  interface
//
//  Created by Philip Rosedale on 9/11/12.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <NodeList.h>
#include <NodeTypes.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>

#include "Application.h"
#include "Avatar.h"
#include "DataServerClient.h"
#include "Hand.h"
#include "Head.h"
#include "Physics.h"
#include "world.h"
#include "devices/OculusManager.h"
#include "ui/TextRenderer.h"

using namespace std;

const bool BALLS_ON = false;
const glm::vec3 DEFAULT_UP_DIRECTION(0.0f, 1.0f, 0.0f);
const float YAW_MAG = 500.0;
const float MY_HAND_HOLDING_PULL = 0.2;
const float YOUR_HAND_HOLDING_PULL = 1.0;
const float BODY_SPRING_DEFAULT_TIGHTNESS = 1000.0f;
const float BODY_SPRING_FORCE = 300.0f;
const float BODY_SPRING_DECAY = 16.0f;
const float COLLISION_RADIUS_SCALAR = 1.2; // pertains to avatar-to-avatar collisions
const float COLLISION_BALL_FORCE = 200.0; // pertains to avatar-to-avatar collisions
const float COLLISION_BODY_FORCE = 30.0; // pertains to avatar-to-avatar collisions
const float HEAD_ROTATION_SCALE = 0.70;
const float HEAD_ROLL_SCALE = 0.40;
const float HEAD_MAX_PITCH = 45;
const float HEAD_MIN_PITCH = -45;
const float HEAD_MAX_YAW = 85;
const float HEAD_MIN_YAW = -85;
const float PERIPERSONAL_RADIUS = 1.0f;
const float AVATAR_BRAKING_STRENGTH = 40.0f;
const float MOUSE_RAY_TOUCH_RANGE = 0.01f;
const float FLOATING_HEIGHT = 0.13f;
const bool  USING_HEAD_LEAN = false;
const float LEAN_SENSITIVITY = 0.15;
const float LEAN_MAX = 0.45;
const float LEAN_AVERAGING = 10.0;
const float HEAD_RATE_MAX = 50.f;
const float SKIN_COLOR[] = {1.0, 0.84, 0.66};
const float DARK_SKIN_COLOR[] = {0.9, 0.78, 0.63};
const int   NUM_BODY_CONE_SIDES = 9;
const float chatMessageScale = 0.0015;
const float chatMessageHeight = 0.20;

void Avatar::sendAvatarURLsMessage(const QUrl& voxelURL) {
    QByteArray message;
    
    char packetHeader[MAX_PACKET_HEADER_BYTES];
    int numBytesPacketHeader = populateTypeAndVersion((unsigned char*) packetHeader, PACKET_TYPE_AVATAR_URLS);
    
    message.append(packetHeader, numBytesPacketHeader);
    message.append(NodeList::getInstance()->getOwnerUUID().toRfc4122());
    
    QDataStream out(&message, QIODevice::WriteOnly | QIODevice::Append);
    out << voxelURL;
    
    Application::controlledBroadcastToNodes((unsigned char*)message.data(), message.size(), &NODE_TYPE_AVATAR_MIXER, 1);
}

Avatar::Avatar(Node* owningNode) :
    AvatarData(owningNode),
    _head(this),
    _hand(this),
    _skeletonModel(this),
    _ballSpringsInitialized(false),
    _bodyYawDelta(0.0f),
    _mode(AVATAR_MODE_STANDING),
    _velocity(0.0f, 0.0f, 0.0f),
    _thrust(0.0f, 0.0f, 0.0f),
    _speed(0.0f),
    _leanScale(0.5f),
    _pelvisFloatingHeight(0.0f),
    _scale(1.0f),
    _worldUpDirection(DEFAULT_UP_DIRECTION),
    _mouseRayOrigin(0.0f, 0.0f, 0.0f),
    _mouseRayDirection(0.0f, 0.0f, 0.0f),
    _isCollisionsOn(true),
    _leadingAvatar(NULL),
    _voxels(this),
    _moving(false),
    _hoverOnDuration(0.0f),
    _hoverOffDuration(0.0f),
    _initialized(false),
    _handHoldingPosition(0.0f, 0.0f, 0.0f),
    _maxArmLength(0.0f),
    _pelvisStandingHeight(0.0f)
{
    // we may have been created in the network thread, but we live in the main thread
    moveToThread(Application::getInstance()->thread());
    
    // give the pointer to our head to inherited _headData variable from AvatarData
    _headData = &_head;
    _handData = &_hand;
    
    _skeleton.initialize();
    
    initializeBodyBalls();
        
    _height = _skeleton.getHeight() + _bodyBall[BODY_BALL_LEFT_HEEL].radius + _bodyBall[BODY_BALL_HEAD_BASE].radius;

    _maxArmLength = _skeleton.getArmLength();
    _pelvisStandingHeight = _skeleton.getPelvisStandingHeight() + _bodyBall[BODY_BALL_LEFT_HEEL].radius;
    _pelvisFloatingHeight = _skeleton.getPelvisFloatingHeight() + _bodyBall[BODY_BALL_LEFT_HEEL].radius;
    _pelvisToHeadLength = _skeleton.getPelvisToHeadLength();
    
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
        _bodyBall[b].parentJoint = AVATAR_JOINT_NULL;
        _bodyBall[b].parentOffset = glm::vec3(0.0, 0.0, 0.0);
        _bodyBall[b].position = glm::vec3(0.0, 0.0, 0.0);
        _bodyBall[b].velocity = glm::vec3(0.0, 0.0, 0.0);
        _bodyBall[b].radius = 0.0;
        _bodyBall[b].touchForce = 0.0;
        _bodyBall[b].isCollidable = true;
        _bodyBall[b].jointTightness = BODY_SPRING_DEFAULT_TIGHTNESS;
    }
    
    // specify the radius of each ball
    _bodyBall[BODY_BALL_PELVIS].radius = BODY_BALL_RADIUS_PELVIS;
    _bodyBall[BODY_BALL_TORSO].radius = BODY_BALL_RADIUS_TORSO;
    _bodyBall[BODY_BALL_CHEST].radius = BODY_BALL_RADIUS_CHEST;
    _bodyBall[BODY_BALL_NECK_BASE].radius = BODY_BALL_RADIUS_NECK_BASE;
    _bodyBall[BODY_BALL_HEAD_BASE].radius = BODY_BALL_RADIUS_HEAD_BASE;
    _bodyBall[BODY_BALL_LEFT_COLLAR].radius = BODY_BALL_RADIUS_LEFT_COLLAR;
    _bodyBall[BODY_BALL_LEFT_SHOULDER].radius = BODY_BALL_RADIUS_LEFT_SHOULDER;
    _bodyBall[BODY_BALL_LEFT_ELBOW].radius = BODY_BALL_RADIUS_LEFT_ELBOW;
    _bodyBall[BODY_BALL_LEFT_WRIST].radius = BODY_BALL_RADIUS_LEFT_WRIST;
    _bodyBall[BODY_BALL_LEFT_FINGERTIPS].radius = BODY_BALL_RADIUS_LEFT_FINGERTIPS;
    _bodyBall[BODY_BALL_RIGHT_COLLAR].radius = BODY_BALL_RADIUS_RIGHT_COLLAR;
    _bodyBall[BODY_BALL_RIGHT_SHOULDER].radius = BODY_BALL_RADIUS_RIGHT_SHOULDER;
    _bodyBall[BODY_BALL_RIGHT_ELBOW].radius = BODY_BALL_RADIUS_RIGHT_ELBOW;
    _bodyBall[BODY_BALL_RIGHT_WRIST].radius = BODY_BALL_RADIUS_RIGHT_WRIST;
    _bodyBall[BODY_BALL_RIGHT_FINGERTIPS].radius = BODY_BALL_RADIUS_RIGHT_FINGERTIPS;
    _bodyBall[BODY_BALL_LEFT_HIP].radius = BODY_BALL_RADIUS_LEFT_HIP;
    _bodyBall[BODY_BALL_LEFT_KNEE].radius = BODY_BALL_RADIUS_LEFT_KNEE;
    _bodyBall[BODY_BALL_LEFT_HEEL].radius = BODY_BALL_RADIUS_LEFT_HEEL;
    _bodyBall[BODY_BALL_LEFT_TOES].radius = BODY_BALL_RADIUS_LEFT_TOES;
    _bodyBall[BODY_BALL_RIGHT_HIP].radius = BODY_BALL_RADIUS_RIGHT_HIP;
    _bodyBall[BODY_BALL_RIGHT_KNEE].radius = BODY_BALL_RADIUS_RIGHT_KNEE;
    _bodyBall[BODY_BALL_RIGHT_HEEL].radius = BODY_BALL_RADIUS_RIGHT_HEEL;
    _bodyBall[BODY_BALL_RIGHT_TOES].radius = BODY_BALL_RADIUS_RIGHT_TOES;
    
    
    // specify the parent joint for each ball
    _bodyBall[BODY_BALL_PELVIS].parentJoint = AVATAR_JOINT_PELVIS;
    _bodyBall[BODY_BALL_TORSO].parentJoint = AVATAR_JOINT_TORSO;
    _bodyBall[BODY_BALL_CHEST].parentJoint = AVATAR_JOINT_CHEST;
    _bodyBall[BODY_BALL_NECK_BASE].parentJoint = AVATAR_JOINT_NECK_BASE;
    _bodyBall[BODY_BALL_HEAD_BASE].parentJoint = AVATAR_JOINT_HEAD_BASE;
    _bodyBall[BODY_BALL_HEAD_TOP].parentJoint = AVATAR_JOINT_HEAD_TOP;
    _bodyBall[BODY_BALL_LEFT_COLLAR].parentJoint = AVATAR_JOINT_LEFT_COLLAR;
    _bodyBall[BODY_BALL_LEFT_SHOULDER].parentJoint = AVATAR_JOINT_LEFT_SHOULDER;
    _bodyBall[BODY_BALL_LEFT_ELBOW].parentJoint = AVATAR_JOINT_LEFT_ELBOW;
    _bodyBall[BODY_BALL_LEFT_WRIST].parentJoint = AVATAR_JOINT_LEFT_WRIST;
    _bodyBall[BODY_BALL_LEFT_FINGERTIPS].parentJoint = AVATAR_JOINT_LEFT_FINGERTIPS;
    _bodyBall[BODY_BALL_RIGHT_COLLAR].parentJoint = AVATAR_JOINT_RIGHT_COLLAR;
    _bodyBall[BODY_BALL_RIGHT_SHOULDER].parentJoint = AVATAR_JOINT_RIGHT_SHOULDER;
    _bodyBall[BODY_BALL_RIGHT_ELBOW].parentJoint = AVATAR_JOINT_RIGHT_ELBOW;
    _bodyBall[BODY_BALL_RIGHT_WRIST].parentJoint = AVATAR_JOINT_RIGHT_WRIST;
    _bodyBall[BODY_BALL_RIGHT_FINGERTIPS].parentJoint = AVATAR_JOINT_RIGHT_FINGERTIPS;
    _bodyBall[BODY_BALL_LEFT_HIP].parentJoint = AVATAR_JOINT_LEFT_HIP;
    _bodyBall[BODY_BALL_LEFT_KNEE].parentJoint = AVATAR_JOINT_LEFT_KNEE;
    _bodyBall[BODY_BALL_LEFT_HEEL].parentJoint = AVATAR_JOINT_LEFT_HEEL;
    _bodyBall[BODY_BALL_LEFT_TOES].parentJoint = AVATAR_JOINT_LEFT_TOES;
    _bodyBall[BODY_BALL_RIGHT_HIP].parentJoint = AVATAR_JOINT_RIGHT_HIP;
    _bodyBall[BODY_BALL_RIGHT_KNEE].parentJoint = AVATAR_JOINT_RIGHT_KNEE;
    _bodyBall[BODY_BALL_RIGHT_HEEL].parentJoint = AVATAR_JOINT_RIGHT_HEEL;
    _bodyBall[BODY_BALL_RIGHT_TOES].parentJoint = AVATAR_JOINT_RIGHT_TOES;

    // specify the parent offset for each ball
    _bodyBall[BODY_BALL_PELVIS].parentOffset = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[BODY_BALL_TORSO].parentOffset = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[BODY_BALL_CHEST].parentOffset = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[BODY_BALL_NECK_BASE].parentOffset = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[BODY_BALL_HEAD_BASE].parentOffset = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[BODY_BALL_HEAD_TOP].parentOffset = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[BODY_BALL_LEFT_COLLAR].parentOffset = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[BODY_BALL_LEFT_SHOULDER].parentOffset = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[BODY_BALL_LEFT_ELBOW].parentOffset = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[BODY_BALL_LEFT_WRIST].parentOffset = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[BODY_BALL_LEFT_FINGERTIPS].parentOffset = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[BODY_BALL_RIGHT_COLLAR].parentOffset = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[BODY_BALL_RIGHT_SHOULDER].parentOffset = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[BODY_BALL_RIGHT_ELBOW].parentOffset = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[BODY_BALL_RIGHT_WRIST].parentOffset = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[BODY_BALL_RIGHT_FINGERTIPS].parentOffset  = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[BODY_BALL_LEFT_HIP].parentOffset = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[BODY_BALL_LEFT_KNEE].parentOffset = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[BODY_BALL_LEFT_HEEL].parentOffset = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[BODY_BALL_LEFT_TOES].parentOffset = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[BODY_BALL_RIGHT_HIP].parentOffset = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[BODY_BALL_RIGHT_KNEE].parentOffset = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[BODY_BALL_RIGHT_HEEL].parentOffset = glm::vec3(0.0, 0.0, 0.0);
    _bodyBall[BODY_BALL_RIGHT_TOES].parentOffset = glm::vec3(0.0, 0.0, 0.0);

    // specify the parent BALL for each ball
    _bodyBall[BODY_BALL_PELVIS].parentBall = BODY_BALL_NULL;
    _bodyBall[BODY_BALL_TORSO].parentBall = BODY_BALL_PELVIS;
    _bodyBall[BODY_BALL_CHEST].parentBall = BODY_BALL_TORSO;
    _bodyBall[BODY_BALL_NECK_BASE].parentBall = BODY_BALL_CHEST;
    _bodyBall[BODY_BALL_HEAD_BASE].parentBall = BODY_BALL_NECK_BASE;
    _bodyBall[BODY_BALL_HEAD_TOP].parentBall = BODY_BALL_HEAD_BASE;
    _bodyBall[BODY_BALL_LEFT_COLLAR].parentBall = BODY_BALL_CHEST;
    _bodyBall[BODY_BALL_LEFT_SHOULDER].parentBall = BODY_BALL_LEFT_COLLAR;
    _bodyBall[BODY_BALL_LEFT_ELBOW].parentBall = BODY_BALL_LEFT_SHOULDER;
    _bodyBall[BODY_BALL_LEFT_WRIST].parentBall = BODY_BALL_LEFT_ELBOW;
    _bodyBall[BODY_BALL_LEFT_FINGERTIPS].parentBall = BODY_BALL_LEFT_WRIST;
    _bodyBall[BODY_BALL_RIGHT_COLLAR].parentBall = BODY_BALL_CHEST;
    _bodyBall[BODY_BALL_RIGHT_SHOULDER].parentBall = BODY_BALL_RIGHT_COLLAR;
    _bodyBall[BODY_BALL_RIGHT_ELBOW].parentBall = BODY_BALL_RIGHT_SHOULDER;
    _bodyBall[BODY_BALL_RIGHT_WRIST].parentBall = BODY_BALL_RIGHT_ELBOW;
    _bodyBall[BODY_BALL_RIGHT_FINGERTIPS].parentBall = BODY_BALL_RIGHT_WRIST;
    _bodyBall[BODY_BALL_LEFT_HIP].parentBall = BODY_BALL_PELVIS;
    _bodyBall[BODY_BALL_LEFT_KNEE].parentBall = BODY_BALL_LEFT_HIP;
    _bodyBall[BODY_BALL_LEFT_HEEL].parentBall = BODY_BALL_LEFT_KNEE;
    _bodyBall[BODY_BALL_LEFT_TOES].parentBall = BODY_BALL_LEFT_HEEL;
    _bodyBall[BODY_BALL_RIGHT_HIP].parentBall = BODY_BALL_PELVIS;
    _bodyBall[BODY_BALL_RIGHT_KNEE].parentBall = BODY_BALL_RIGHT_HIP;
    _bodyBall[BODY_BALL_RIGHT_HEEL].parentBall = BODY_BALL_RIGHT_KNEE;
    _bodyBall[BODY_BALL_RIGHT_TOES].parentBall = BODY_BALL_RIGHT_HEEL;
}

Avatar::~Avatar() {
    _headData = NULL;
    _handData = NULL;
    delete _balls;
}

void Avatar::init() {
    _head.init();
    _hand.init();
    _skeletonModel.init();
    _voxels.init();
    _initialized = true;
}

glm::quat Avatar::getOrientation() const {
    return glm::quat(glm::radians(glm::vec3(_bodyPitch, _bodyYaw, _bodyRoll)));
}

glm::quat Avatar::getWorldAlignedOrientation () const {
    return computeRotationFromBodyToWorldUp() * getOrientation();
}

void Avatar::follow(Avatar* leadingAvatar) {
    const float MAX_STRING_LENGTH = 2;

    _leadingAvatar = leadingAvatar;
    if (_leadingAvatar != NULL) {
        _leaderUUID = leadingAvatar->getOwningNode()->getUUID();
        _stringLength = glm::length(_position - _leadingAvatar->getPosition()) / _scale;
        if (_stringLength > MAX_STRING_LENGTH) {
            _stringLength = MAX_STRING_LENGTH;
        }
    } else {
        _leaderUUID = QUuid();
    }
}

void Avatar::simulate(float deltaTime, Transmitter* transmitter) {

    glm::quat orientation = getOrientation();
    glm::vec3 front = orientation * IDENTITY_FRONT;
    glm::vec3 right = orientation * IDENTITY_RIGHT;

    if (_leadingAvatar && !_leadingAvatar->getOwningNode()->isAlive()) {
        follow(NULL);
    }

    if (_scale != _newScale) {
        setScale(_newScale);
    }
    
    // copy velocity so we can use it later for acceleration
    glm::vec3 oldVelocity = getVelocity();

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
    _skeleton.joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position = _handPosition;
        
    //update the movement of the hand and process handshaking with other avatars...
    updateHandMovementAndTouching(deltaTime, enableHandMovement);
    _avatarTouch.simulate(deltaTime);
    
    // update body balls
    updateBodyBalls(deltaTime);
    
    //apply the head lean values to the ball positions...
    if (USING_HEAD_LEAN) {
        if (fabs(_head.getLeanSideways() + _head.getLeanForward()) > 0.0f) {
            glm::vec3 headLean =
            right * _head.getLeanSideways() +
            front * _head.getLeanForward();
            
            _bodyBall[BODY_BALL_TORSO].position += headLean * 0.1f;
            _bodyBall[BODY_BALL_CHEST].position += headLean * 0.4f;
            _bodyBall[BODY_BALL_NECK_BASE].position += headLean * 0.7f;
            _bodyBall[BODY_BALL_HEAD_BASE].position += headLean * 1.0f;
            
            _bodyBall[BODY_BALL_LEFT_COLLAR].position += headLean * 0.6f;
            _bodyBall[BODY_BALL_LEFT_SHOULDER].position += headLean * 0.6f;
            _bodyBall[BODY_BALL_LEFT_ELBOW].position += headLean * 0.2f;
            _bodyBall[BODY_BALL_LEFT_WRIST].position += headLean * 0.1f;
            _bodyBall[BODY_BALL_LEFT_FINGERTIPS].position += headLean * 0.0f;
            
            _bodyBall[BODY_BALL_RIGHT_COLLAR].position += headLean * 0.6f;
            _bodyBall[BODY_BALL_RIGHT_SHOULDER].position += headLean * 0.6f;
            _bodyBall[BODY_BALL_RIGHT_ELBOW].position += headLean * 0.2f;
            _bodyBall[BODY_BALL_RIGHT_WRIST].position += headLean * 0.1f;
            _bodyBall[BODY_BALL_RIGHT_FINGERTIPS].position += headLean * 0.0f;
        }
    }
    
    // head scale grows when avatar is looked at
    const float BASE_MAX_SCALE = 3.0f;
    float maxScale = BASE_MAX_SCALE * glm::distance(_position, Application::getInstance()->getCamera()->getPosition());
    if (Application::getInstance()->getLookatTargetAvatar() == this) {
        _hoverOnDuration += deltaTime;
        _hoverOffDuration = 0.0f;
    
        const float GROW_DELAY = 1.0f;
        const float GROW_RATE = 0.25f;
        if (_hoverOnDuration > GROW_DELAY) {
            _head.setScale(glm::mix(_head.getScale(), maxScale, GROW_RATE));
        }
        
    } else {
        _hoverOnDuration = 0.0f;
        _hoverOffDuration += deltaTime;
    
        const float SHRINK_DELAY = 1.0f;
        const float SHRINK_RATE = 0.25f;
        if (_hoverOffDuration > SHRINK_DELAY) {
            _head.setScale(glm::mix(_head.getScale(), 1.0f, SHRINK_RATE));
        }
    }
    
    _skeletonModel.simulate(deltaTime);
    _head.setBodyRotation(glm::vec3(_bodyPitch, _bodyYaw, _bodyRoll));
    glm::vec3 headPosition;
    if (!_skeletonModel.getHeadPosition(headPosition)) {
        headPosition = _bodyBall[BODY_BALL_HEAD_BASE].position;
    }
    _head.setPosition(headPosition);
    _head.setSkinColor(glm::vec3(SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]));
    _head.simulate(deltaTime, false);
    _hand.simulate(deltaTime, false);

    // use speed and angular velocity to determine walking vs. standing
    if (_speed + fabs(_bodyYawDelta) > 0.2) {
        _mode = AVATAR_MODE_WALKING;
    } else {
        _mode = AVATAR_MODE_INTERACTING;
    }
    
    // update position by velocity, and subtract the change added earlier for gravity 
    _position += _velocity * deltaTime;
    
    // Zero thrust out now that we've added it to velocity in this frame
    _thrust = glm::vec3(0, 0, 0);

}

void Avatar::setMouseRay(const glm::vec3 &origin, const glm::vec3 &direction) {
    _mouseRayOrigin = origin;
    _mouseRayDirection = direction;
}

void Avatar::updateHandMovementAndTouching(float deltaTime, bool enableHandMovement) {
    
    glm::quat orientation = getOrientation();
    
    // reset hand and arm positions according to hand movement
    glm::vec3 right = orientation * IDENTITY_RIGHT;
    glm::vec3 up = orientation * IDENTITY_UP;
    glm::vec3 front = orientation * IDENTITY_FRONT;
    
    enableHandMovement |= updateLeapHandPositions();
    
    //constrain right arm length and re-adjust elbow position as it bends
    // NOTE - the following must be called on all avatars - not just _isMine
    if (enableHandMovement) {
        updateArmIKAndConstraints(deltaTime, AVATAR_JOINT_RIGHT_FINGERTIPS);
        updateArmIKAndConstraints(deltaTime, AVATAR_JOINT_LEFT_FINGERTIPS);
    }
}

static TextRenderer* textRenderer() {
    static TextRenderer* renderer = new TextRenderer(SANS_FONT_FAMILY, 24, -1, false, TextRenderer::SHADOW_EFFECT);
    return renderer;
}

void Avatar::render(bool lookingInMirror, bool renderAvatarBalls) {

    if (Application::getInstance()->getAvatar()->getHand().isRaveGloveActive()) {
        _hand.setRaveLights(RAVE_LIGHTS_AVATAR);
    }
    
    // render a simple round on the ground projected down from the avatar's position
    renderDiskShadow(_position, glm::vec3(0.0f, 1.0f, 0.0f), _scale * 0.1f, 0.2f);
    
    {
        // glow when moving in the distance
        glm::vec3 toTarget = _position - Application::getInstance()->getAvatar()->getPosition();
        const float GLOW_DISTANCE = 5.0f;
        Glower glower(_moving && glm::length(toTarget) > GLOW_DISTANCE ? 1.0f : 0.0f);
        
        // render body
        renderBody(lookingInMirror, renderAvatarBalls);
    
        // render sphere when far away
        const float MAX_ANGLE = 10.f;
        glm::vec3 delta = _height * (_head.getCameraOrientation() * IDENTITY_UP) / 2.f;
        float angle = abs(angleBetween(toTarget + delta, toTarget - delta));

        if (angle < MAX_ANGLE) {
            glColor4f(0.5f, 0.8f, 0.8f, 1.f - angle / MAX_ANGLE);
            glPushMatrix();
            glTranslatef(_position.x, _position.y, _position.z);
            glScalef(_height / 2.f, _height / 2.f, _height / 2.f);
            glutSolidSphere(1.2f + _head.getAverageLoudness() * .0005f, 20, 20);
            glPopMatrix();
        }
    }
    
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
        
        glm::vec3 chatPosition = _bodyBall[BODY_BALL_HEAD_BASE].position + getBodyUpDirection() * chatMessageHeight * _scale;
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
    const float BEYOND_BODY_SPRING_RANGE = _scale * 2.f;
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

// returns true if the Leap controls any of the avatar's hands.
bool Avatar::updateLeapHandPositions() {
    bool returnValue = false;
    // If there are leap-interaction hands visible, see if we can use them as the endpoints for IK
    if (getHand().getPalms().size() > 0) {
        PalmData const* leftLeapHand = NULL;
        PalmData const* rightLeapHand = NULL;
        // Look through all of the palms available (there may be more than two), and pick
        // the leftmost and rightmost. If there's only one, we'll use a heuristic below
        // to decode whether it's the left or right.
        for (size_t i = 0; i < getHand().getPalms().size(); ++i) {
            PalmData& palm = getHand().getPalms()[i];
            if (palm.isActive()) {
                if (!rightLeapHand || !leftLeapHand) {
                    rightLeapHand = leftLeapHand = &palm;
                }
                else if (palm.getRawPosition().x > rightLeapHand->getRawPosition().x) {
                    rightLeapHand = &palm;
                }
                else if (palm.getRawPosition().x < leftLeapHand->getRawPosition().x) {
                    leftLeapHand = &palm;
                }
            }
        }
        // If there's only one palm visible. Decide if it's the left or right
        if (leftLeapHand == rightLeapHand && leftLeapHand) {
            if (leftLeapHand->getRawPosition().x > 0) {
                leftLeapHand = NULL;
            }
            else {
                rightLeapHand = NULL;
            }
        }
        if (leftLeapHand) {
            _skeleton.joint[ AVATAR_JOINT_LEFT_FINGERTIPS ].position = leftLeapHand->getPosition();
            returnValue = true;
        }
        if (rightLeapHand) {
            _skeleton.joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].position = rightLeapHand->getPosition();
            returnValue = true;
        }
    }
    return returnValue;
}

void Avatar::updateArmIKAndConstraints(float deltaTime, AvatarJointID fingerTipJointID) {
    Skeleton::AvatarJoint& fingerJoint = _skeleton.joint[fingerTipJointID];
    Skeleton::AvatarJoint& wristJoint = _skeleton.joint[fingerJoint.parent];
    Skeleton::AvatarJoint& elbowJoint = _skeleton.joint[wristJoint.parent];
    Skeleton::AvatarJoint& shoulderJoint = _skeleton.joint[elbowJoint.parent];
    
    // determine the arm vector
    glm::vec3 armVector = fingerJoint.position;
    armVector -= shoulderJoint.position;
    
    // test to see if right hand is being dragged beyond maximum arm length
    float distance = glm::length(armVector);
    
    // don't let right hand get dragged beyond maximum arm length...
    if (distance > _maxArmLength) {
        // reset right hand to be constrained to maximum arm length
        fingerJoint.position = shoulderJoint.position;
        glm::vec3 armNormal = armVector / distance;
        armVector = armNormal * _maxArmLength;
        distance = _maxArmLength;
        glm::vec3 constrainedPosition = shoulderJoint.position;
        constrainedPosition += armVector;
        fingerJoint.position = constrainedPosition;
    }
    
    // set elbow position
    glm::vec3 newElbowPosition = shoulderJoint.position + armVector * ONE_HALF;
    
    glm::vec3 perpendicular = glm::cross(getBodyRightDirection(),  armVector);
    
    newElbowPosition += perpendicular * (1.0f - (_maxArmLength / distance)) * ONE_HALF;
    elbowJoint.position = newElbowPosition;
    
    // set wrist position
    const float wristPosRatio = 0.7f;
    wristJoint.position = elbowJoint.position + (fingerJoint.position - elbowJoint.position) * wristPosRatio;
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
    return 1.0f;
}

void Avatar::renderBody(bool lookingInMirror, bool renderAvatarBalls) {

    if (_head.getVideoFace().isFullFrame()) {
        //  Render the full-frame video
        float alpha = getBallRenderAlpha(BODY_BALL_HEAD_BASE, lookingInMirror);
        if (alpha > 0.0f) {
            _head.getVideoFace().render(1.0f);
        }
    } else if (renderAvatarBalls || !(_voxels.getVoxelURL().isValid() || _skeletonModel.isActive())) {
        //  Render the body as balls and cones
        glm::vec3 skinColor, darkSkinColor;
        getSkinColors(skinColor, darkSkinColor);
        for (int b = 0; b < NUM_AVATAR_BODY_BALLS; b++) {
            float alpha = getBallRenderAlpha(b, lookingInMirror);
            
            // When we have leap hands, hide part of the arms.
            if (_hand.getNumPalms() > 0) {
                if (b == BODY_BALL_LEFT_FINGERTIPS
                    || b == BODY_BALL_RIGHT_FINGERTIPS) {
                    continue;
                }
            }
            //  Always render other people, and render myself when beyond threshold distance
            if (b == BODY_BALL_HEAD_BASE) { // the head is rendered as a special
                if (alpha > 0.0f) {
                    _head.render(alpha, false);
                }
            } else if (alpha > 0.0f) {
                //  Render the body ball sphere
                glColor3f(skinColor.r + _bodyBall[b].touchForce * 0.3f,
                    skinColor.g - _bodyBall[b].touchForce * 0.2f,
                    skinColor.b - _bodyBall[b].touchForce * 0.1f);
                
                if (b == BODY_BALL_NECK_BASE && _head.getFaceModel().isActive()) {
                    continue; // don't render the neck if we have a face model
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
                    if ((b != BODY_BALL_HEAD_TOP)
                        && (b != BODY_BALL_HEAD_BASE)
                        && (b != BODY_BALL_PELVIS)
                        && (b != BODY_BALL_TORSO)
                        && (b != BODY_BALL_CHEST)
                        && (b != BODY_BALL_LEFT_COLLAR)
                        && (b != BODY_BALL_LEFT_SHOULDER)
                        && (b != BODY_BALL_RIGHT_COLLAR)
                        && (b != BODY_BALL_RIGHT_SHOULDER)) {
                        glColor3fv((const GLfloat*)&darkSkinColor);
                        
                        float r2 = _bodyBall[b].radius * 0.8;
                        renderJointConnectingCone(_bodyBall[_bodyBall[b].parentBall].position, _bodyBall[b].position, r2, r2);
                    }
                }
            }
        }
    } else {
        //  Render the body's voxels and head
        float alpha = getBallRenderAlpha(BODY_BALL_HEAD_BASE, lookingInMirror);
        if (alpha > 0.0f) {
            if (!_skeletonModel.render(alpha)) {
                _voxels.render(false);
            }
            _head.render(alpha, false);
        }
    }
    _hand.render(lookingInMirror);
}

void Avatar::getSkinColors(glm::vec3& lighter, glm::vec3& darker) {
    lighter = glm::vec3(SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    darker = glm::vec3(DARK_SKIN_COLOR[0], DARK_SKIN_COLOR[1], DARK_SKIN_COLOR[2]);
    if (_head.getFaceModel().isActive()) {
        lighter = glm::vec3(_head.getFaceModel().computeAverageColor());
        const float SKIN_DARKENING = 0.9f;
        darker = lighter * SKIN_DARKENING;
    }
}

void Avatar::getBodyBallTransform(AvatarJointID jointID, glm::vec3& position, glm::quat& rotation) const {
    position = _bodyBall[jointID].position;
    rotation = _bodyBall[jointID].rotation;
}

bool Avatar::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const {
    float minDistance = FLT_MAX;
    for (int i = 0; i < NUM_AVATAR_BODY_BALLS; i++) {
        float distance;
        if (rayIntersectsSphere(origin, direction, _bodyBall[i].position, _bodyBall[i].radius, distance)) {
            minDistance = min(minDistance, distance);
        }
    }
    if (minDistance == FLT_MAX) {
        return false;
    }
    distance = minDistance;
    return true;
}

int Avatar::parseData(unsigned char* sourceBuffer, int numBytes) {
    // change in position implies movement
    glm::vec3 oldPosition = _position;
    int bytesRead = AvatarData::parseData(sourceBuffer, numBytes);
    const float MOVE_DISTANCE_THRESHOLD = 0.001f;
    _moving = glm::distance(oldPosition, _position) > MOVE_DISTANCE_THRESHOLD;
    return bytesRead;
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

void Avatar::goHome() {
    qDebug("Going Home!\n");
    setPosition(START_LOCATION);
}

void Avatar::increaseSize() {
    if ((1.f + SCALING_RATIO) * _newScale < MAX_SCALE) {
        _newScale *= (1.f + SCALING_RATIO);
        qDebug("Changed scale to %f\n", _newScale);
    }
}

void Avatar::decreaseSize() {
    if (MIN_SCALE < (1.f - SCALING_RATIO) * _newScale) {
        _newScale *= (1.f - SCALING_RATIO);
        qDebug("Changed scale to %f\n", _newScale);
    }
}

void Avatar::resetSize() {
    _newScale = 1.0f;
    qDebug("Reseted scale to %f\n", _newScale);
}

void Avatar::setScale(const float scale) {
    _scale = scale;

    if (_newScale * (1.f - RESCALING_TOLERANCE) < _scale &&
            _scale < _newScale * (1.f + RESCALING_TOLERANCE)) {
        _scale = _newScale;
    }
    
    _skeleton.setScale(_scale);
    
    // specify the new radius of each ball
    _bodyBall[BODY_BALL_PELVIS].radius = _scale * BODY_BALL_RADIUS_PELVIS;
    _bodyBall[BODY_BALL_TORSO].radius = _scale * BODY_BALL_RADIUS_TORSO;
    _bodyBall[BODY_BALL_CHEST].radius = _scale * BODY_BALL_RADIUS_CHEST;
    _bodyBall[BODY_BALL_NECK_BASE].radius = _scale * BODY_BALL_RADIUS_NECK_BASE;
    _bodyBall[BODY_BALL_HEAD_BASE].radius = _scale * BODY_BALL_RADIUS_HEAD_BASE;
    _bodyBall[BODY_BALL_LEFT_COLLAR].radius = _scale * BODY_BALL_RADIUS_LEFT_COLLAR;
    _bodyBall[BODY_BALL_LEFT_SHOULDER].radius = _scale * BODY_BALL_RADIUS_LEFT_SHOULDER;
    _bodyBall[BODY_BALL_LEFT_ELBOW].radius = _scale * BODY_BALL_RADIUS_LEFT_ELBOW;
    _bodyBall[BODY_BALL_LEFT_WRIST].radius = _scale * BODY_BALL_RADIUS_LEFT_WRIST;
    _bodyBall[BODY_BALL_LEFT_FINGERTIPS].radius = _scale * BODY_BALL_RADIUS_LEFT_FINGERTIPS;
    _bodyBall[BODY_BALL_RIGHT_COLLAR].radius = _scale * BODY_BALL_RADIUS_RIGHT_COLLAR;
    _bodyBall[BODY_BALL_RIGHT_SHOULDER].radius = _scale * BODY_BALL_RADIUS_RIGHT_SHOULDER;
    _bodyBall[BODY_BALL_RIGHT_ELBOW].radius = _scale * BODY_BALL_RADIUS_RIGHT_ELBOW;
    _bodyBall[BODY_BALL_RIGHT_WRIST].radius = _scale * BODY_BALL_RADIUS_RIGHT_WRIST;
    _bodyBall[BODY_BALL_RIGHT_FINGERTIPS].radius = _scale * BODY_BALL_RADIUS_RIGHT_FINGERTIPS;
    _bodyBall[BODY_BALL_LEFT_HIP].radius = _scale * BODY_BALL_RADIUS_LEFT_HIP;
    _bodyBall[BODY_BALL_LEFT_KNEE].radius = _scale * BODY_BALL_RADIUS_LEFT_KNEE;
    _bodyBall[BODY_BALL_LEFT_HEEL].radius = _scale * BODY_BALL_RADIUS_LEFT_HEEL;
    _bodyBall[BODY_BALL_LEFT_TOES].radius = _scale * BODY_BALL_RADIUS_LEFT_TOES;
    _bodyBall[BODY_BALL_RIGHT_HIP].radius = _scale * BODY_BALL_RADIUS_RIGHT_HIP;
    _bodyBall[BODY_BALL_RIGHT_KNEE].radius = _scale * BODY_BALL_RADIUS_RIGHT_KNEE;
    _bodyBall[BODY_BALL_RIGHT_HEEL].radius = _scale * BODY_BALL_RADIUS_RIGHT_HEEL;
    _bodyBall[BODY_BALL_RIGHT_TOES].radius = _scale * BODY_BALL_RADIUS_RIGHT_TOES;
    
    _height = _skeleton.getHeight() + _bodyBall[BODY_BALL_LEFT_HEEL].radius + _bodyBall[BODY_BALL_HEAD_BASE].radius;
    
    _maxArmLength = _skeleton.getArmLength();
    _pelvisStandingHeight = _skeleton.getPelvisStandingHeight() + _bodyBall[BODY_BALL_LEFT_HEEL].radius;
    _pelvisFloatingHeight = _skeleton.getPelvisFloatingHeight() + _bodyBall[BODY_BALL_LEFT_HEEL].radius;
    _pelvisToHeadLength = _skeleton.getPelvisToHeadLength();
    _avatarTouch.setReachableRadius(_scale * PERIPERSONAL_RADIUS);
}

