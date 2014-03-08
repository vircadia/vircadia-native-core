//
//  Hand.cpp
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <QImage>

#include <NodeList.h>

#include <GeometryUtil.h>

#include "Application.h"
#include "Avatar.h"
#include "Hand.h"
#include "Menu.h"
#include "Util.h"
#include "renderer/ProgramObject.h"

using namespace std;

const float FINGERTIP_COLLISION_RADIUS = 0.01f;
const float PALM_COLLISION_RADIUS = 0.03f;


Hand::Hand(Avatar* owningAvatar) :
    HandData((AvatarData*)owningAvatar),

    _owningAvatar(owningAvatar),
    _renderAlpha(1.0),
    _collisionCenter(0,0,0),
    _collisionAge(0),
    _collisionDuration(0)
{
}

void Hand::init() {
}

void Hand::reset() {
}

void Hand::simulate(float deltaTime, bool isMine) {
    
    if (_collisionAge > 0.f) {
        _collisionAge += deltaTime;
    }
    
    calculateGeometry();
    
    if (isMine) {
        //  Iterate hand controllers, take actions as needed
        for (size_t i = 0; i < getNumPalms(); ++i) {
            PalmData& palm = getPalms()[i];
            palm.setLastControllerButtons(palm.getControllerButtons());
        }
    }
}

void Hand::playSlaps(PalmData& palm, Avatar* avatar) {
    //  Check for palm collisions
    glm::vec3 myPalmPosition = palm.getPosition();
    float palmCollisionDistance = 0.1f;
    bool wasColliding = palm.getIsCollidingWithPalm();
    palm.setIsCollidingWithPalm(false);
    //  If 'Play Slaps' is enabled, look for palm-to-palm collisions and make sound
    for (size_t j = 0; j < avatar->getHand()->getNumPalms(); j++) {
        PalmData& otherPalm = avatar->getHand()->getPalms()[j];
        if (!otherPalm.isActive()) {
            continue;
        }
        glm::vec3 otherPalmPosition = otherPalm.getPosition();
        if (glm::length(otherPalmPosition - myPalmPosition) < palmCollisionDistance) {
            palm.setIsCollidingWithPalm(true);
            if (!wasColliding) {
                const float PALM_COLLIDE_VOLUME = 1.f;
                const float PALM_COLLIDE_FREQUENCY = 1000.f;
                const float PALM_COLLIDE_DURATION_MAX = 0.75f;
                const float PALM_COLLIDE_DECAY_PER_SAMPLE = 0.01f;
                Application::getInstance()->getAudio()->startDrumSound(PALM_COLLIDE_VOLUME,
                                                                    PALM_COLLIDE_FREQUENCY,
                                                                    PALM_COLLIDE_DURATION_MAX,
                                                                    PALM_COLLIDE_DECAY_PER_SAMPLE);
                //  If the other person's palm is in motion, move mine downward to show I was hit
                const float MIN_VELOCITY_FOR_SLAP = 0.05f;
                if (glm::length(otherPalm.getVelocity()) > MIN_VELOCITY_FOR_SLAP) {
                    // add slapback here
                }
            }
        }
    }
}

// We create a static CollisionList that is recycled for each collision test.
const float MAX_COLLISIONS_PER_AVATAR = 32;
static CollisionList handCollisions(MAX_COLLISIONS_PER_AVATAR);

void Hand::collideAgainstAvatarOld(Avatar* avatar, bool isMyHand) {
    if (!avatar || avatar == _owningAvatar) {
        // don't collide with our own hands (that is done elsewhere)
        return;
    }
    float scaledPalmRadius = PALM_COLLISION_RADIUS * _owningAvatar->getScale();
    for (size_t i = 0; i < getNumPalms(); i++) {
        PalmData& palm = getPalms()[i];
        if (!palm.isActive()) {
            continue;
        }
        if (isMyHand && Menu::getInstance()->isOptionChecked(MenuOption::PlaySlaps)) {
            playSlaps(palm, avatar);
        }

        glm::vec3 totalPenetration;
        handCollisions.clear();
        if (avatar->findSphereCollisions(palm.getPosition(), scaledPalmRadius, handCollisions)) {
            for (int j = 0; j < handCollisions.size(); ++j) {
                CollisionInfo* collision = handCollisions.getCollision(j);
                if (isMyHand) {
                    // we resolve the hand from collision when it belongs to MyAvatar AND the other Avatar is 
                    // not expected to respond to the collision (hand hit unmovable part of their Avatar)
                    totalPenetration = addPenetrations(totalPenetration, collision->_penetration);
                }
            }
        }
        if (isMyHand) {
            // resolve penetration
            palm.addToPosition(-totalPenetration);
        }
    }
}

void Hand::collideAgainstAvatar(Avatar* avatar, bool isMyHand) {
    if (!avatar || avatar == _owningAvatar) {
        // don't collide hands against ourself (that is done elsewhere)
        return;
    }

    // 2 = NUM_HANDS
    int palmIndices[2];
    getLeftRightPalmIndices(*palmIndices, *(palmIndices + 1));

    const SkeletonModel& skeletonModel = _owningAvatar->getSkeletonModel();
    int jointIndices[2];
    jointIndices[0] = skeletonModel.getLeftHandJointIndex();
    jointIndices[1] = skeletonModel.getRightHandJointIndex();

    palmIndices[1] = -1;    // adebug temporarily disable right hand
    jointIndices[1] = -1;   // adebug temporarily disable right hand

    for (size_t i = 0; i < 1; i++) {
        int palmIndex = palmIndices[i];
        int jointIndex = jointIndices[i];
        if (palmIndex == -1 || jointIndex == -1) {
            continue;
        }
        PalmData& palm = _palms[palmIndex];
        if (!palm.isActive()) {
            continue;
        }
        if (isMyHand && Menu::getInstance()->isOptionChecked(MenuOption::PlaySlaps)) {
            playSlaps(palm, avatar);
        }

        handCollisions.clear();
        QVector<const Shape*> shapes;
        skeletonModel.getHandShapes(jointIndex, shapes);
        bool collided = isMyHand ? avatar->findCollisions(shapes, handCollisions) : avatar->findCollisions(shapes, handCollisions);
        if (collided) {
        //if (avatar->findCollisions(shapes, handCollisions)) {
            glm::vec3 averagePenetration;
            glm::vec3 averageContactPoint;
            for (int j = 0; j < handCollisions.size(); ++j) {
                CollisionInfo* collision = handCollisions.getCollision(j);
                averagePenetration += collision->_penetration;
                averageContactPoint += collision->_contactPoint;
            }
            averagePenetration /= float(handCollisions.size());
            if (isMyHand) {
                // our hand against other avatar 
                // for now we resolve it to test shapes/collisions
                // TODO: only partially resolve this penetration
                palm.addToPosition(-averagePenetration);
            } else {
                // someone else's hand against MyAvatar
                // TODO: submit collision info to MyAvatar which should lean accordingly
                averageContactPoint /= float(handCollisions.size());
            }
        }
    }
}

void Hand::collideAgainstOurself() {
    if (!Menu::getInstance()->isOptionChecked(MenuOption::HandsCollideWithSelf)) {
        return;
    }

    int leftPalmIndex, rightPalmIndex;   
    getLeftRightPalmIndices(leftPalmIndex, rightPalmIndex);
    float scaledPalmRadius = PALM_COLLISION_RADIUS * _owningAvatar->getScale();
    
    const Model& skeletonModel = _owningAvatar->getSkeletonModel();
    for (size_t i = 0; i < getNumPalms(); i++) {
        PalmData& palm = getPalms()[i];
        if (!palm.isActive()) {
            continue;
        }
        // ignoring everything below the parent of the parent of the last free joint
        int skipIndex = skeletonModel.getParentJointIndex(skeletonModel.getParentJointIndex(
            skeletonModel.getLastFreeJointIndex((i == leftPalmIndex) ? skeletonModel.getLeftHandJointIndex() :
                (i == rightPalmIndex) ? skeletonModel.getRightHandJointIndex() : -1)));

        handCollisions.clear();
        if (_owningAvatar->findSphereCollisions(palm.getPosition(), scaledPalmRadius, handCollisions, skipIndex)) {
            glm::vec3 totalPenetration;
            for (int j = 0; j < handCollisions.size(); ++j) {
                CollisionInfo* collision = handCollisions.getCollision(j);
                totalPenetration = addPenetrations(totalPenetration, collision->_penetration);
            }
            // resolve penetration
            palm.addToPosition(-totalPenetration);
        }
    }
}

void Hand::handleVoxelCollision(PalmData* palm, const glm::vec3& fingerTipPosition, VoxelTreeElement* voxel, float deltaTime) {
    //  Collision between finger and a voxel plays sound
    const float LOWEST_FREQUENCY = 100.f;
    const float HERTZ_PER_RGB = 3.f;
    const float DECAY_PER_SAMPLE = 0.0005f;
    const float DURATION_MAX = 2.0f;
    const float MIN_VOLUME = 0.1f;
    float volume = MIN_VOLUME + glm::clamp(glm::length(palm->getRawVelocity()), 0.f, (1.f - MIN_VOLUME));
    float duration = volume;
    _collisionCenter = fingerTipPosition;
    _collisionAge = deltaTime;
    _collisionDuration = duration;
    int voxelBrightness = voxel->getColor()[0] + voxel->getColor()[1] + voxel->getColor()[2];
    float frequency = LOWEST_FREQUENCY + (voxelBrightness * HERTZ_PER_RGB);
    Application::getInstance()->getAudio()->startDrumSound(volume,
                                                           frequency,
                                                           DURATION_MAX,
                                                           DECAY_PER_SAMPLE);
}

void Hand::calculateGeometry() {
    // generate finger tip balls....
    _leapFingerTipBalls.clear();
    for (size_t i = 0; i < getNumPalms(); ++i) {
        PalmData& palm = getPalms()[i];
        if (palm.isActive()) {
            for (size_t f = 0; f < palm.getNumFingers(); ++f) {
                FingerData& finger = palm.getFingers()[f];
                if (finger.isActive()) {
                    const float standardBallRadius = FINGERTIP_COLLISION_RADIUS;
                    HandBall ball;
                    ball.rotation = getBaseOrientation();
                    ball.position = finger.getTipPosition();
                    ball.radius         = standardBallRadius;
                    ball.touchForce     = 0.0;
                    ball.isCollidable   = true;
                    ball.isColliding    = false;
                    _leapFingerTipBalls.push_back(ball);
                }
            }
        }
    }

    // generate finger root balls....
    _leapFingerRootBalls.clear();
    for (size_t i = 0; i < getNumPalms(); ++i) {
        PalmData& palm = getPalms()[i];
        if (palm.isActive()) {
            for (size_t f = 0; f < palm.getNumFingers(); ++f) {
                FingerData& finger = palm.getFingers()[f];
                if (finger.isActive()) {
                    const float standardBallRadius = 0.005f;
                    HandBall ball;
                    ball.rotation = getBaseOrientation();
                    ball.position = finger.getRootPosition();
                    ball.radius         = standardBallRadius;
                    ball.touchForce     = 0.0;
                    ball.isCollidable   = true;
                    ball.isColliding    = false;
                    _leapFingerRootBalls.push_back(ball);
                }
            }
        }
    }
}

void Hand::render(bool isMine) {
    
    _renderAlpha = 1.0;
    
    if (Menu::getInstance()->isOptionChecked(MenuOption::RenderSkeletonCollisionProxies)) {
        // draw a green sphere at hand joint location, which is actually near the wrist)
        for (size_t i = 0; i < getNumPalms(); i++) {
            PalmData& palm = getPalms()[i];
            if (!palm.isActive()) {
                continue;
            }
            glm::vec3 position = palm.getPosition();
            glPushMatrix();
            glTranslatef(position.x, position.y, position.z);
            glColor3f(0.0f, 1.0f, 0.0f);
            glutSolidSphere(PALM_COLLISION_RADIUS * _owningAvatar->getScale(), 10, 10);
            glPopMatrix();
        }
    }
    
    if (Menu::getInstance()->isOptionChecked(MenuOption::DisplayHands)) {
        renderLeapHands(isMine);
    }

    if (isMine) {
        //  If hand/voxel collision has happened, render a little expanding sphere
        if (_collisionAge > 0.f) {
            float opacity = glm::clamp(1.f - (_collisionAge / _collisionDuration), 0.f, 1.f);
            glColor4f(1, 0, 0, 0.5 * opacity);
            glPushMatrix();
            glTranslatef(_collisionCenter.x, _collisionCenter.y, _collisionCenter.z);
            glutSolidSphere(_collisionAge * 0.25f, 20, 20);
            glPopMatrix();
            if (_collisionAge > _collisionDuration) {
                _collisionAge = 0.f;
            }
        }
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_RESCALE_NORMAL);
    
}

void Hand::renderLeapHands(bool isMine) {

    const float alpha = 1.0f;
    
    const glm::vec3 handColor(1.0, 0.84, 0.66); // use the skin color
    
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    if (isMine && Menu::getInstance()->isOptionChecked(MenuOption::DisplayHandTargets)) {
        for (size_t i = 0; i < getNumPalms(); ++i) {
            PalmData& palm = getPalms()[i];
            if (!palm.isActive()) {
                continue;
            }
            glm::vec3 targetPosition;
            palm.getBallHoldPosition(targetPosition);
            glPushMatrix();
        
            const float collisionRadius = 0.05f;
            glColor4f(0.5f,0.5f,0.5f, alpha);
            glutWireSphere(collisionRadius, 10.f, 10.f);
            glPopMatrix();
        }
    }
    
    glPushMatrix();
    // Draw the leap balls
    for (size_t i = 0; i < _leapFingerTipBalls.size(); i++) {
        if (alpha > 0.0f) {
            if (_leapFingerTipBalls[i].isColliding) {
                glColor4f(handColor.r, 0, 0, alpha);
            } else {
                glColor4f(handColor.r, handColor.g, handColor.b, alpha);
            }
            glPushMatrix();
            glTranslatef(_leapFingerTipBalls[i].position.x, _leapFingerTipBalls[i].position.y, _leapFingerTipBalls[i].position.z);
            glutSolidSphere(_leapFingerTipBalls[i].radius, 20.0f, 20.0f);
            glPopMatrix();
        }
    }
        
    // Draw the finger root cones
    for (size_t i = 0; i < getNumPalms(); ++i) {
        PalmData& palm = getPalms()[i];
        if (palm.isActive()) {
            for (size_t f = 0; f < palm.getNumFingers(); ++f) {
                FingerData& finger = palm.getFingers()[f];
                if (finger.isActive()) {
                    glColor4f(handColor.r, handColor.g, handColor.b, 0.5);
                    glm::vec3 tip = finger.getTipPosition();
                    glm::vec3 root = finger.getRootPosition();
                    Avatar::renderJointConnectingCone(root, tip, 0.001f, 0.003f);
                }
            }
        }
    }

    // Draw the hand paddles
    int MAX_NUM_PADDLES = 2; // one for left and one for right
    glColor4f(handColor.r, handColor.g, handColor.b, 0.3f);
    for (int i = 0; i < MAX_NUM_PADDLES; i++) {
        const PalmData* palm = getPalm(i);
        if (palm) {
            // compute finger axis
            glm::vec3 fingerAxis(0.f);
            for (size_t f = 0; f < palm->getNumFingers(); ++f) {
                const FingerData& finger = (palm->getFingers())[f];
                if (finger.isActive()) {
                    glm::vec3 fingerTip = finger.getTipPosition();
                    glm::vec3 fingerRoot = finger.getRootPosition();
                    fingerAxis = glm::normalize(fingerTip - fingerRoot);
                    break;
                }
            }
            // compute paddle position
            glm::vec3 handPosition;
            if (i == SIXENSE_CONTROLLER_ID_LEFT_HAND) {
                _owningAvatar->getSkeletonModel().getLeftHandPosition(handPosition);
            } else if (i == SIXENSE_CONTROLLER_ID_RIGHT_HAND) {
                _owningAvatar->getSkeletonModel().getRightHandPosition(handPosition);
            }
            glm::vec3 tip = handPosition + HAND_PADDLE_OFFSET * fingerAxis;
            glm::vec3 root = tip + palm->getNormal() * HAND_PADDLE_THICKNESS;
            // render a very shallow cone as the paddle
            Avatar::renderJointConnectingCone(root, tip, HAND_PADDLE_RADIUS, 0.f);
        }
    }

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
}


void Hand::setLeapHands(const std::vector<glm::vec3>& handPositions,
                          const std::vector<glm::vec3>& handNormals) {
    for (size_t i = 0; i < getNumPalms(); ++i) {
        PalmData& palm = getPalms()[i];
        if (i < handPositions.size()) {
            palm.setActive(true);
            palm.setRawPosition(handPositions[i]);
            palm.setRawNormal(handNormals[i]);
        }
        else {
            palm.setActive(false);
        }
    }
}

