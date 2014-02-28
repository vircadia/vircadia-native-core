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
const float DISPLAYNAME_FADE_TIME = 0.5f;
const float DISPLAYNAME_FADE_FACTOR = pow(0.01f, 1.0f / DISPLAYNAME_FADE_TIME);
const float DISPLAYNAME_ALPHA = 0.95f;

Avatar::Avatar() :
    AvatarData(),
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
    _collisionFlags(0),
    _initialized(false)
{
    // we may have been created in the network thread, but we live in the main thread
    moveToThread(Application::getInstance()->thread());
    
    // give the pointer to our head to inherited _headData variable from AvatarData
    _headData = static_cast<HeadData*>(new Head(this));
    _handData = static_cast<HandData*>(new Hand(this));
}

Avatar::~Avatar() {
}

void Avatar::init() {
    getHead()->init();
    getHand()->init();
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
    
    getHand()->simulate(deltaTime, false);
    _skeletonModel.simulate(deltaTime);
    Head* head = getHead();
    head->setBodyRotation(glm::vec3(_bodyPitch, _bodyYaw, _bodyRoll));
    glm::vec3 headPosition;
    if (!_skeletonModel.getHeadPosition(headPosition)) {
        headPosition = _position;
    }
    head->setPosition(headPosition);
    head->setScale(_scale);
    getHead()->simulate(deltaTime, false);
    
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

    // update animation for display name fade in/out
    if ( _displayNameTargetAlpha != _displayNameAlpha) {
        // the alpha function is 
        // Fade out => alpha(t) = factor ^ t => alpha(t+dt) = alpha(t) * factor^(dt)
        // Fade in  => alpha(t) = 1 - factor^t => alpha(t+dt) = 1-(1-alpha(t))*coef^(dt)
        // factor^(dt) = coef
        float coef = pow(DISPLAYNAME_FADE_FACTOR, deltaTime);
        if (_displayNameTargetAlpha < _displayNameAlpha) {
            // Fading out
            _displayNameAlpha *= coef;
        } else {
            // Fading in
            _displayNameAlpha = 1 - (1 - _displayNameAlpha) * coef;
        }
        _displayNameAlpha = abs(_displayNameAlpha - _displayNameTargetAlpha) < 0.01? _displayNameTargetAlpha : _displayNameAlpha;
    }
}

void Avatar::setMouseRay(const glm::vec3 &origin, const glm::vec3 &direction) {
    _mouseRayOrigin = origin;
    _mouseRayDirection = direction;
}

enum TextRendererType {
    CHAT, 
    DISPLAYNAME
};

static TextRenderer* textRenderer(TextRendererType type) {
    static TextRenderer* chatRenderer = new TextRenderer(SANS_FONT_FAMILY, 24, -1, false, TextRenderer::SHADOW_EFFECT);
    static TextRenderer* displayNameRenderer = new TextRenderer(SANS_FONT_FAMILY, 12, -1, false, TextRenderer::NO_EFFECT);

    switch(type) {
    case CHAT:
        return chatRenderer;
    case DISPLAYNAME:
        return displayNameRenderer;
    }

    return displayNameRenderer;
}

void Avatar::render(bool forceRenderHead) {
    glm::vec3 toTarget = _position - Application::getInstance()->getAvatar()->getPosition();
    float lengthToTarget = glm::length(toTarget);
   
    {
        // glow when moving in the distance
        
        const float GLOW_DISTANCE = 5.0f;
        Glower glower(_moving && lengthToTarget > GLOW_DISTANCE ? 1.0f : 0.0f);

        // render body
        if (Menu::getInstance()->isOptionChecked(MenuOption::RenderSkeletonCollisionProxies)) {
            _skeletonModel.renderCollisionProxies(0.7f);
        }
        if (Menu::getInstance()->isOptionChecked(MenuOption::RenderHeadCollisionProxies)) {
            getHead()->getFaceModel().renderCollisionProxies(0.7f);
        }
        if (Menu::getInstance()->isOptionChecked(MenuOption::Avatars)) {
            renderBody(forceRenderHead);
        }

        // render sphere when far away
        const float MAX_ANGLE = 10.f;
        float height = getSkeletonHeight();
        glm::vec3 delta = height * (getHead()->getCameraOrientation() * IDENTITY_UP) / 2.f;
        float angle = abs(angleBetween(toTarget + delta, toTarget - delta));

        if (angle < MAX_ANGLE) {
            glColor4f(0.5f, 0.8f, 0.8f, 1.f - angle / MAX_ANGLE);
            glPushMatrix();
            glTranslatef(_position.x, _position.y, _position.z);
            glScalef(height / 2.f, height / 2.f, height / 2.f);
            glutSolidSphere(1.2f + getHead()->getAverageLoudness() * .0005f, 20, 20);
            glPopMatrix();
        }
    }
    const float DISPLAYNAME_DISTANCE = 4.0f;
    setShowDisplayName(lengthToTarget < DISPLAYNAME_DISTANCE);
    renderDisplayName();
    
    if (!_chatMessage.empty()) {
        int width = 0;
        int lastWidth = 0;
        for (string::iterator it = _chatMessage.begin(); it != _chatMessage.end(); it++) {
            width += (lastWidth = textRenderer(CHAT)->computeWidth(*it));
        }
        glPushMatrix();
        
        glm::vec3 chatPosition = getHead()->getEyePosition() + getBodyUpDirection() * CHAT_MESSAGE_HEIGHT * _scale;
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
            textRenderer(CHAT)->draw(-width / 2.0f, 0, _chatMessage.c_str());
            
        } else {
            // rather than using substr and allocating a new string, just replace the last
            // character with a null, then restore it
            int lastIndex = _chatMessage.size() - 1;
            char lastChar = _chatMessage[lastIndex];
            _chatMessage[lastIndex] = '\0';
            textRenderer(CHAT)->draw(-width / 2.0f, 0, _chatMessage.c_str());
            _chatMessage[lastIndex] = lastChar;
            glColor3f(0, 1, 0);
            textRenderer(CHAT)->draw(width / 2.0f - lastWidth, 0, _chatMessage.c_str() + lastIndex);
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
        getHead()->render(1.0f);
    }
    getHand()->render(false);
}

void Avatar::renderDisplayName() {

    if (_displayName.isEmpty() || _displayNameAlpha == 0.0f) {
        return;
    }
       
    glDisable(GL_LIGHTING);
    
    glPushMatrix();
    glm::vec3 textPosition = getPosition() + getBodyUpDirection() * ((getSkeletonHeight() + getHeadHeight()) / 1.5f);
    glTranslatef(textPosition.x, textPosition.y, textPosition.z); 

    // we need "always facing camera": we must remove the camera rotation from the stack
    glm::quat rotation = Application::getInstance()->getCamera()->getRotation();
    glm::vec3 axis = glm::axis(rotation);
    glRotatef(glm::angle(rotation), axis.x, axis.y, axis.z);

    // We need to compute the scale factor such as the text remains with fixed size respect to window coordinates
    // We project a unit vector and check the difference in screen coordinates, to check which is the 
    // correction scale needed
    // save the matrices for later scale correction factor 
    glm::dmat4 modelViewMatrix;
    glm::dmat4 projectionMatrix;
    GLint viewportMatrix[4];
    Application::getInstance()->getModelViewMatrix(&modelViewMatrix);
    Application::getInstance()->getProjectionMatrix(&projectionMatrix);
    glGetIntegerv(GL_VIEWPORT, viewportMatrix);
    GLdouble result0[3], result1[3];

    glm::dvec3 upVector(modelViewMatrix[1]);
    
    glm::dvec3 testPoint0 = glm::dvec3(textPosition);
    glm::dvec3 testPoint1 = glm::dvec3(textPosition) + upVector;
    
    bool success;
    success = gluProject(testPoint0.x, testPoint0.y, testPoint0.z,
        (GLdouble*)&modelViewMatrix, (GLdouble*)&projectionMatrix, viewportMatrix, 
        &result0[0], &result0[1], &result0[2]);
    success = success && 
        gluProject(testPoint1.x, testPoint1.y, testPoint1.z,
        (GLdouble*)&modelViewMatrix, (GLdouble*)&projectionMatrix, viewportMatrix, 
        &result1[0], &result1[1], &result1[2]);

    if (success) {
        double textWindowHeight = abs(result1[1] - result0[1]);
        float scaleFactor = (textWindowHeight > EPSILON) ? 1.0f / textWindowHeight : 1.0f;
        glScalef(scaleFactor, scaleFactor, 1.0);  

        // draw a gray background
        QFontMetrics metrics = textRenderer(DISPLAYNAME)->metrics();
        int bottom = -metrics.descent(), top = bottom + metrics.height();
        int left = -_displayNameWidth/2, right = _displayNameWidth/2;
        const int border = 5;
        bottom -= border;
        left -= border;
        top += border;
        right += border;

        // We are drawing coplanar textures with depth: need the polygon offset
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);

        glColor4f(0.2f, 0.2f, 0.2f, _displayNameAlpha);
        glBegin(GL_QUADS);
        glVertex2f(left, bottom);
        glVertex2f(right, bottom);
        glVertex2f(right, top);
        glVertex2f(left, top);
        glEnd();
        
        glScalef(1.0f, -1.0f, 1.0f);  // TextRenderer::draw paints the text upside down in y axis
        glColor4f(0.93f, 0.93f, 0.93f, _displayNameAlpha);
               
        QByteArray ba = _displayName.toLocal8Bit();
        const char* text = ba.data();
        
        glDisable(GL_POLYGON_OFFSET_FILL);
        textRenderer(DISPLAYNAME)->draw(-_displayNameWidth / 2, 0, text); 
     

    }

    glPopMatrix();

    glEnable(GL_LIGHTING);
}

bool Avatar::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const {
    float minDistance = FLT_MAX;
    float modelDistance;
    if (_skeletonModel.findRayIntersection(origin, direction, modelDistance)) {
        minDistance = qMin(minDistance, modelDistance);
    }
    if (getHead()->getFaceModel().findRayIntersection(origin, direction, modelDistance)) {
        minDistance = qMin(minDistance, modelDistance);
    }
    if (minDistance < FLT_MAX) {
        distance = minDistance;
        return true;
    }
    return false;
}

bool Avatar::findSphereCollisions(const glm::vec3& penetratorCenter, float penetratorRadius,
        CollisionList& collisions, int skeletonSkipIndex) {
    return _skeletonModel.findSphereCollisions(penetratorCenter, penetratorRadius, collisions, skeletonSkipIndex);
    // Temporarily disabling collisions against the head because most of its collision proxies are bad.
    //return getHead()->getFaceModel().findSphereCollisions(penetratorCenter, penetratorRadius, collisions);
}

bool Avatar::findParticleCollisions(const glm::vec3& particleCenter, float particleRadius, CollisionList& collisions) {
    if (_collisionFlags & COLLISION_GROUP_PARTICLES) {
        return false;
    }
    bool collided = false;
    // first do the hand collisions
    const HandData* handData = getHandData();
    if (handData) {
        for (int i = 0; i < NUM_HANDS; i++) {
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

                int jointIndex = -1;
                glm::vec3 handPosition;
                if (i == 0) {
                    _skeletonModel.getLeftHandPosition(handPosition);
                    jointIndex = _skeletonModel.getLeftHandJointIndex();
                }
                else {
                    _skeletonModel.getRightHandPosition(handPosition);
                    jointIndex = _skeletonModel.getRightHandJointIndex();
                }
                glm::vec3 diskCenter = handPosition + HAND_PADDLE_OFFSET * fingerAxis;
                glm::vec3 diskNormal = palm->getNormal();
                const float DISK_THICKNESS = 0.08f;

                // collide against the disk
                glm::vec3 penetration;
                if (findSphereDiskPenetration(particleCenter, particleRadius, 
                            diskCenter, HAND_PADDLE_RADIUS, DISK_THICKNESS, diskNormal,
                            penetration)) {
                    CollisionInfo* collision = collisions.getNewCollision();
                    if (collision) {
                        collision->_type = PADDLE_HAND_COLLISION;
                        collision->_flags = jointIndex;
                        collision->_penetration = penetration;
                        collision->_addedVelocity = palm->getVelocity();
                        collided = true;
                    } else {
                        // collisions are full, so we might as well bail now
                        return collided;
                    }
                }
            }
        }
    }
    // then collide against the models
    int preNumCollisions = collisions.size();
    if (_skeletonModel.findSphereCollisions(particleCenter, particleRadius, collisions)) {
        // the Model doesn't have velocity info, so we have to set it for each new collision
        int postNumCollisions = collisions.size();
        for (int i = preNumCollisions; i < postNumCollisions; ++i) {
            CollisionInfo* collision = collisions.getCollision(i);
            collision->_penetration /= (float)(TREE_SCALE);
            collision->_addedVelocity = getVelocity();
        }
        collided = true;
    }
    return collided;
}

void Avatar::setFaceModelURL(const QUrl &faceModelURL) {
    AvatarData::setFaceModelURL(faceModelURL);
    const QUrl DEFAULT_FACE_MODEL_URL = QUrl::fromLocalFile("resources/meshes/defaultAvatar_head.fst");
    getHead()->getFaceModel().setURL(_faceModelURL, DEFAULT_FACE_MODEL_URL);
}

void Avatar::setSkeletonModelURL(const QUrl &skeletonModelURL) {
    AvatarData::setSkeletonModelURL(skeletonModelURL);
    const QUrl DEFAULT_SKELETON_MODEL_URL = QUrl::fromLocalFile("resources/meshes/defaultAvatar_body.fst");
    _skeletonModel.setURL(_skeletonModelURL, DEFAULT_SKELETON_MODEL_URL);
}

void Avatar::setDisplayName(const QString& displayName) {
    AvatarData::setDisplayName(displayName);
    int width = 0;
    for (int i = 0; i < displayName.size(); i++) {
        width += (textRenderer(DISPLAYNAME)->computeWidth(displayName[i].toLatin1()));
    }
    _displayNameWidth = width;
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

void Avatar::updateCollisionFlags() {
    _collisionFlags = 0;
    if (Menu::getInstance()->isOptionChecked(MenuOption::CollideWithEnvironment)) {
        _collisionFlags |= COLLISION_GROUP_ENVIRONMENT;
    }
    if (Menu::getInstance()->isOptionChecked(MenuOption::CollideWithAvatars)) {
        _collisionFlags |= COLLISION_GROUP_AVATARS;
    }
    if (Menu::getInstance()->isOptionChecked(MenuOption::CollideWithVoxels)) {
        _collisionFlags |= COLLISION_GROUP_VOXELS;
    }
    if (Menu::getInstance()->isOptionChecked(MenuOption::CollideWithParticles)) {
        _collisionFlags |= COLLISION_GROUP_PARTICLES;
    }
}

void Avatar::setScale(float scale) {
    _scale = scale;

    if (_targetScale * (1.f - RESCALING_TOLERANCE) < _scale &&
            _scale < _targetScale * (1.f + RESCALING_TOLERANCE)) {
        _scale = _targetScale;
    }
}

float Avatar::getSkeletonHeight() const {
    Extents extents = _skeletonModel.getBindExtents();
    return extents.maximum.y - extents.minimum.y;
}

float Avatar::getHeadHeight() const {
    Extents extents = getHead()->getFaceModel().getBindExtents();
    return extents.maximum.y - extents.minimum.y;
}

bool Avatar::collisionWouldMoveAvatar(CollisionInfo& collision) const {
    if (!collision._data || collision._type != MODEL_COLLISION) {
        return false;
    }
    Model* model = static_cast<Model*>(collision._data);
    int jointIndex = collision._flags;

    if (model == &(_skeletonModel) && jointIndex != -1) {
        // collision response of skeleton is temporarily disabled
        return false;
        //return _skeletonModel.collisionHitsMoveableJoint(collision);
    }
    if (model == &(getHead()->getFaceModel())) {
        // ATM we always handle MODEL_COLLISIONS against the face.
        return true;
    }
    return false;
}

void Avatar::applyCollision(CollisionInfo& collision) {
    if (!collision._data || collision._type != MODEL_COLLISION) {
        return;
    }
    // TODO: make skeleton also respond to collisions
    Model* model = static_cast<Model*>(collision._data);
    if (model == &(getHead()->getFaceModel())) {
        getHead()->applyCollision(collision);
    }
}

float Avatar::getPelvisFloatingHeight() const {
    return -_skeletonModel.getBindExtents().minimum.y;
}

float Avatar::getPelvisToHeadLength() const {
    return glm::distance(_position, getHead()->getPosition());
}

void Avatar::setShowDisplayName(bool showDisplayName) {
    // For myAvatar, the alpha update is not done (called in simulate for other avatars)
    if (Application::getInstance()->getAvatar() == this) {
        if (showDisplayName) {
            _displayNameAlpha = DISPLAYNAME_ALPHA;
        } else {
            _displayNameAlpha = 0.0f;
        }
    } 

    if (showDisplayName) {
        _displayNameTargetAlpha = DISPLAYNAME_ALPHA;
    } else {
        _displayNameTargetAlpha = 0.0f;
    }

}

