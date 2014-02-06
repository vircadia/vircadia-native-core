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
#include <PacketHeaders.h>
#include <SharedUtil.h>

#include <GeometryUtil.h>

#include "Application.h"
#include "Avatar.h"
#include "DataServerClient.h"
#include "Hand.h"
#include "Head.h"
#include "Menu.h"
#include "Physics.h"
#include "world.h"
#include "devices/OculusManager.h"
#include "ui/TextRenderer.h"

using namespace std;

const glm::vec3 DEFAULT_UP_DIRECTION(0.0f, 1.0f, 0.0f);
const float YAW_MAG = 500.0f;
const float MY_HAND_HOLDING_PULL = 0.2f;
const float YOUR_HAND_HOLDING_PULL = 1.0f;
const float BODY_SPRING_DEFAULT_TIGHTNESS = 1000.0f;
const float BODY_SPRING_FORCE = 300.0f;
const float BODY_SPRING_DECAY = 16.0f;
const float COLLISION_RADIUS_SCALAR = 1.2f; // pertains to avatar-to-avatar collisions
const float COLLISION_BODY_FORCE = 30.0f; // pertains to avatar-to-avatar collisions
const float HEAD_ROTATION_SCALE = 0.70f;
const float HEAD_ROLL_SCALE = 0.40f;
const float HEAD_MAX_PITCH = 45;
const float HEAD_MIN_PITCH = -45;
const float HEAD_MAX_YAW = 85;
const float HEAD_MIN_YAW = -85;
const float AVATAR_BRAKING_STRENGTH = 40.0f;
const float MOUSE_RAY_TOUCH_RANGE = 0.01f;
const float FLOATING_HEIGHT = 0.13f;
const bool  USING_HEAD_LEAN = false;
const float LEAN_SENSITIVITY = 0.15f;
const float LEAN_MAX = 0.45f;
const float LEAN_AVERAGING = 10.0f;
const float HEAD_RATE_MAX = 50.f;
const int   NUM_BODY_CONE_SIDES = 9;
const float CHAT_MESSAGE_SCALE = 0.0015f;
const float CHAT_MESSAGE_HEIGHT = 0.1f;

Avatar::Avatar() :
    AvatarData(),
    _head(this),
    _hand(this),
    _skeletonModel(this),
    _bodyYawDelta(0.0f),
    _mode(AVATAR_MODE_STANDING),
    _velocity(0.0f, 0.0f, 0.0f),
    _thrust(0.0f, 0.0f, 0.0f),
    _speed(0.0f),
    _leanScale(0.5f),
    _scale(1.0f),
    _worldUpDirection(DEFAULT_UP_DIRECTION),
    _mouseRayOrigin(0.0f, 0.0f, 0.0f),
    _mouseRayDirection(0.0f, 0.0f, 0.0f),
    _moving(false),
    _owningAvatarMixer(),
    _initialized(false)
{
    // we may have been created in the network thread, but we live in the main thread
    moveToThread(Application::getInstance()->thread());
    
    // give the pointer to our head to inherited _headData variable from AvatarData
    _headData = &_head;
    _handData = &_hand;
}

Avatar::~Avatar() {
    _headData = NULL;
    _handData = NULL;
}

void Avatar::init() {
    _head.init();
    _hand.init();
    _skeletonModel.init();
    _initialized = true;
}

glm::vec3 Avatar::getChestPosition() const {
    // for now, let's just assume that the "chest" is halfway between the root and the neck
    glm::vec3 neckPosition;
    return _skeletonModel.getNeckPosition(neckPosition) ? (_position + neckPosition) * 0.5f : _position;
}

glm::quat Avatar::getWorldAlignedOrientation () const {
    return computeRotationFromBodyToWorldUp() * getOrientation();
}

void Avatar::simulate(float deltaTime) {
    if (_scale != _targetScale) {
        setScale(_targetScale);
    }
    
    // copy velocity so we can use it later for acceleration
    glm::vec3 oldVelocity = getVelocity();
    
    _hand.simulate(deltaTime, false);
    _skeletonModel.simulate(deltaTime);
    _head.setBodyRotation(glm::vec3(_bodyPitch, _bodyYaw, _bodyRoll));
    glm::vec3 headPosition;
    if (!_skeletonModel.getHeadPosition(headPosition)) {
        headPosition = _position;
    }
    _head.setPosition(headPosition);
    _head.setScale(_scale);
    _head.simulate(deltaTime, false);
    
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

static TextRenderer* textRenderer() {
    static TextRenderer* renderer = new TextRenderer(SANS_FONT_FAMILY, 24, -1, false, TextRenderer::SHADOW_EFFECT);
    return renderer;
}

void Avatar::render(bool forceRenderHead) {
    
    {
        // glow when moving in the distance
        glm::vec3 toTarget = _position - Application::getInstance()->getAvatar()->getPosition();
        const float GLOW_DISTANCE = 5.0f;
        Glower glower(_moving && glm::length(toTarget) > GLOW_DISTANCE ? 1.0f : 0.0f);
        
        // render body
        renderBody(forceRenderHead);
    
        // render sphere when far away
        const float MAX_ANGLE = 10.f;
        float height = getHeight();
        glm::vec3 delta = height * (_head.getCameraOrientation() * IDENTITY_UP) / 2.f;
        float angle = abs(angleBetween(toTarget + delta, toTarget - delta));

        if (angle < MAX_ANGLE) {
            glColor4f(0.5f, 0.8f, 0.8f, 1.f - angle / MAX_ANGLE);
            glPushMatrix();
            glTranslatef(_position.x, _position.y, _position.z);
            glScalef(height / 2.f, height / 2.f, height / 2.f);
            glutSolidSphere(1.2f + _head.getAverageLoudness() * .0005f, 20, 20);
            glPopMatrix();
        }
    }
    
   
    if (!_chatMessage.empty()) {
        int width = 0;
        int lastWidth = 0;
        for (string::iterator it = _chatMessage.begin(); it != _chatMessage.end(); it++) {
            width += (lastWidth = textRenderer()->computeWidth(*it));
        }
        glPushMatrix();
        
        glm::vec3 chatPosition = getHead().getEyePosition() + getBodyUpDirection() * CHAT_MESSAGE_HEIGHT * _scale;
        glTranslatef(chatPosition.x, chatPosition.y, chatPosition.z);
        glm::quat chatRotation = Application::getInstance()->getCamera()->getRotation();
        glm::vec3 chatAxis = glm::axis(chatRotation);
        glRotatef(glm::angle(chatRotation), chatAxis.x, chatAxis.y, chatAxis.z);
        
        
        glColor3f(0, 0.8f, 0);
        glRotatef(180, 0, 1, 0);
        glRotatef(180, 0, 0, 1);
        glScalef(_scale * CHAT_MESSAGE_SCALE, _scale * CHAT_MESSAGE_SCALE, 1.0f);
        
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

void Avatar::renderBody(bool forceRenderHead) {
    //  Render the body's voxels and head
    glm::vec3 pos = getPosition();
    //printf("Render other at %.3f, %.2f, %.2f\n", pos.x, pos.y, pos.z);
    _skeletonModel.render(1.0f);
    if (forceRenderHead) {
        _head.render(1.0f);
    }
    _hand.render(false);
}

bool Avatar::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const {
    float minDistance = FLT_MAX;
    float modelDistance;
    if (_skeletonModel.findRayIntersection(origin, direction, modelDistance)) {
        minDistance = qMin(minDistance, modelDistance);
    }
    if (_head.getFaceModel().findRayIntersection(origin, direction, modelDistance)) {
        minDistance = qMin(minDistance, modelDistance);
    }
    if (minDistance < FLT_MAX) {
        distance = minDistance;
        return true;
    }
    return false;
}

bool Avatar::findSpherePenetration(const glm::vec3& penetratorCenter, float penetratorRadius,
        glm::vec3& penetration, int skeletonSkipIndex) const {
    bool didPenetrate = false;
    glm::vec3 totalPenetration;
    glm::vec3 skeletonPenetration;
    if (_skeletonModel.findSpherePenetration(penetratorCenter, penetratorRadius,
            skeletonPenetration, 1.0f, skeletonSkipIndex)) {
        totalPenetration = addPenetrations(totalPenetration, skeletonPenetration);
        didPenetrate = true; 
    }
    glm::vec3 facePenetration;
    if (_head.getFaceModel().findSpherePenetration(penetratorCenter, penetratorRadius, facePenetration)) {
        totalPenetration = addPenetrations(totalPenetration, facePenetration);
        didPenetrate = true; 
    }
    if (didPenetrate) {
        penetration = totalPenetration;
        return true;
    }
    return false;
}

bool Avatar::findSphereCollision(const glm::vec3& sphereCenter, float sphereRadius, CollisionInfo& collision) {
    // TODO: provide an early exit using bounding sphere of entire avatar

    const HandData* handData = getHandData();
    if (handData) {
        for (int i = 0; i < 2; i++) {
            const PalmData* palm = handData->getPalm(i);
            if (palm && palm->hasPaddle()) {
                // create a disk collision proxy where the hand is
                glm::vec3 fingerAxis(0.f);
                for (size_t f = 0; f < palm->getNumFingers(); ++f) {
                    const FingerData& finger = (palm->getFingers())[f];
                    if (finger.isActive()) {
                        // compute finger axis
                        glm::vec3 fingerTip = finger.getTipPosition();
                        glm::vec3 fingerRoot = finger.getRootPosition();
                        fingerAxis = glm::normalize(fingerTip - fingerRoot);
                        break;
                    }
                }
                glm::vec3 handPosition;
                if (i == 0) {
                    _skeletonModel.getLeftHandPosition(handPosition);
                }
                else {
                    _skeletonModel.getRightHandPosition(handPosition);
                }
                glm::vec3 diskCenter = handPosition + HAND_PADDLE_OFFSET * fingerAxis;
                glm::vec3 diskNormal = palm->getNormal();
                float diskThickness = 0.08f;

                // collide against the disk
                if (findSphereDiskPenetration(sphereCenter, sphereRadius, 
                            diskCenter, HAND_PADDLE_RADIUS, diskThickness, diskNormal,
                            collision._penetration)) {
                    collision._addedVelocity = palm->getVelocity();
                    return true;
                }
            }
        }
    }

    if (_skeletonModel.findSpherePenetration(sphereCenter, sphereRadius, collision._penetration)) {
        collision._penetration /= (float)(TREE_SCALE);
        collision._addedVelocity = getVelocity();
        return true;
    }
    return false;
}

void Avatar::setFaceModelURL(const QUrl &faceModelURL) {
    AvatarData::setFaceModelURL(faceModelURL);
    _head.getFaceModel().setURL(faceModelURL);
}

void Avatar::setSkeletonModelURL(const QUrl &skeletonModelURL) {
    AvatarData::setSkeletonModelURL(skeletonModelURL);
    _skeletonModel.setURL(skeletonModelURL);
}

int Avatar::parseData(const QByteArray& packet) {
    // change in position implies movement
    glm::vec3 oldPosition = _position;
    
    int bytesRead = AvatarData::parseData(packet);
    
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

void Avatar::setScale(float scale) {
    _scale = scale;

    if (_targetScale * (1.f - RESCALING_TOLERANCE) < _scale &&
            _scale < _targetScale * (1.f + RESCALING_TOLERANCE)) {
        _scale = _targetScale;
    }
}

float Avatar::getHeight() const {
    Extents extents = _skeletonModel.getBindExtents();
    return extents.maximum.y - extents.minimum.y;
}

float Avatar::getPelvisFloatingHeight() const {
    return -_skeletonModel.getBindExtents().minimum.y;
}

float Avatar::getPelvisToHeadLength() const {
    return glm::distance(_position, _head.getPosition());
}

