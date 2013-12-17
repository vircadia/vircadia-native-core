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

const float FINGERTIP_VOXEL_SIZE = 0.05;
const int TOY_BALL_HAND = 1;
const float TOY_BALL_RADIUS = 0.05f;
const float TOY_BALL_DAMPING = 0.99f;
const glm::vec3 NO_GRAVITY = glm::vec3(0,0,0);
const glm::vec3 NO_VELOCITY = glm::vec3(0,0,0);
const glm::vec3 TOY_BALL_GRAVITY = glm::vec3(0,-1,0);
const QString TOY_BALL_UPDATE_SCRIPT("");
const QString TOY_BALL_DONT_DIE_SCRIPT("Particle.setShouldDie(false);");
const float PALM_COLLISION_RADIUS = 0.03f;
const xColor TOY_BALL_ON_SERVER_COLOR[] = 
    {
        { 255, 0, 0 },
        { 0, 255, 0 },
        { 0, 0, 255 },
        { 255, 255, 0 },
        { 0, 255, 255 },
        { 255, 0, 255 },
    };

Hand::Hand(Avatar* owningAvatar) :
    HandData((AvatarData*)owningAvatar),

    _owningAvatar(owningAvatar),
    _renderAlpha(1.0),
    _ballColor(0.0, 0.0, 0.4),
    _collisionCenter(0,0,0),
    _collisionAge(0),
    _collisionDuration(0),
    _pitchUpdate(0),
    _grabDelta(0, 0, 0),
    _grabDeltaVelocity(0, 0, 0)
{
    for (int i = 0; i < MAX_HANDS; i++) {
        _toyBallInHand[i] = false;
        _ballParticleEditHandles[i] = NULL;
        _whichBallColor[i] = 0;
    }
    _lastControllerButtons = 0;
}

void Hand::init() {
    // Different colors for my hand and others' hands
    if (_owningAvatar && _owningAvatar->getOwningNode() == NULL) {
        _ballColor = glm::vec3(0.0, 0.4, 0.0);
    }
    else {
        _ballColor = glm::vec3(0.0, 0.0, 0.4);
    }
}

void Hand::reset() {
}

void Hand::simulateToyBall(PalmData& palm, const glm::vec3& fingerTipPosition, float deltaTime) {
    bool ballFromHand = Menu::getInstance()->isOptionChecked(MenuOption::BallFromHand);
    int handID = palm.getSixenseID();
    glm::vec3 targetPosition = palm.getPosition() / (float)TREE_SCALE;
    float targetRadius = (TOY_BALL_RADIUS * 4.0f) / (float)TREE_SCALE;
    const Particle* closestParticle = Application::getInstance()->getParticles()
                                                ->getTree()->findClosestParticle(targetPosition, targetRadius);

    if (closestParticle) {
        //printf("potentially caught... particle ID:%d\n", closestParticle->getID());
        
        if (!_toyBallInHand[handID]) {
            printf("particle ID:%d NOT IN HAND\n", closestParticle->getID());

            // you can create a ParticleEditHandle by doing this...
            ParticleEditHandle* caughtParticle = Application::getInstance()->newParticleEditHandle(closestParticle->getID());
        
            // reflect off the hand...
            printf("particle ID:%d old velocity=%f,%f,%f\n", closestParticle->getID(), 
                    closestParticle->getVelocity().x, closestParticle->getVelocity().y, closestParticle->getVelocity().z);
            glm::vec3 newVelocity = glm::reflect(closestParticle->getVelocity(), palm.getNormal());

            printf("particle ID:%d REFLECT velocity=%f,%f,%f\n", closestParticle->getID(), 
                    newVelocity.x, newVelocity.y, newVelocity.z);

            newVelocity += palm.getTipVelocity() / (float)TREE_SCALE;

            printf("particle ID:%d with TIP velocity=%f,%f,%f\n", closestParticle->getID(), 
                    newVelocity.x, newVelocity.y, newVelocity.z);

            
            printf("particle ID:%d OLD position=%f,%f,%f\n", closestParticle->getID(), 
                    closestParticle->getPosition().x, closestParticle->getPosition().y, closestParticle->getPosition().z);
            glm::vec3 newPosition = closestParticle->getPosition();

            newPosition += newVelocity; // move it as if it's already been moving in new direction

            printf("particle ID:%d NEW position=%f,%f,%f\n", closestParticle->getID(), 
                    newPosition.x, newPosition.y, newPosition.z);
        
            caughtParticle->updateParticle(newPosition,
                                            closestParticle->getRadius(),
                                            closestParticle->getXColor(),
                                            newVelocity,
                                            closestParticle->getGravity(), 
                                            closestParticle->getDamping(), 
                                            closestParticle->getUpdateScript());
        
            // but make sure you clean it up, when you're done
            delete caughtParticle;
        }
    }
    
    // If there's a ball in hand, and the user presses the skinny button, then change the color of the ball
    int currentControllerButtons = palm.getControllerButtons();
    
    if (currentControllerButtons != _lastControllerButtons && (currentControllerButtons & BUTTON_0)) {
        _whichBallColor[handID]++;
        if (_whichBallColor[handID] >= sizeof(TOY_BALL_ON_SERVER_COLOR)/sizeof(TOY_BALL_ON_SERVER_COLOR[0])) {
            _whichBallColor[handID] = 0;
        }
    }

    // Is the controller button being held down....
    if (palm.getControllerButtons() & BUTTON_FWD) {
        // If we don't currently have a ball in hand, then create it...
        if (!_toyBallInHand[handID]) {
            //  Test for whether close enough to catch and catch....
            
            // isCaught is also used as "creating" a new ball... for now, this section is the
            // create new ball portion of the code...
            bool isCaught = false;

            // If we didn't catch something, then create a new ball....
            if (!isCaught) {
                _toyBallInHand[handID] = true;
                
                // create the ball, call MakeParticle, and use the resulting ParticleEditHandle to
                // manage the newly created particle.
                //  Create a particle on the particle server
                glm::vec3 ballPosition = ballFromHand ? palm.getPosition() : fingerTipPosition;
                _ballParticleEditHandles[handID] = Application::getInstance()->makeParticle(
                                                                    ballPosition / (float)TREE_SCALE,
                                                                     TOY_BALL_RADIUS / (float) TREE_SCALE,
                                                                     TOY_BALL_ON_SERVER_COLOR[_whichBallColor[handID]],
                                                                     NO_VELOCITY / (float)TREE_SCALE,
                                                                     NO_GRAVITY / (float) TREE_SCALE, 
                                                                     TOY_BALL_DAMPING, 
                                                                     TOY_BALL_DONT_DIE_SCRIPT);
            }
        } else {
            //  Ball is in hand
            glm::vec3 ballPosition = ballFromHand ? palm.getPosition() : fingerTipPosition;
            _ballParticleEditHandles[handID]->updateParticle(ballPosition / (float)TREE_SCALE,
                                                         TOY_BALL_RADIUS / (float) TREE_SCALE,
                                                         TOY_BALL_ON_SERVER_COLOR[_whichBallColor[handID]],
                                                         NO_VELOCITY / (float)TREE_SCALE,
                                                         NO_GRAVITY / (float) TREE_SCALE, 
                                                         TOY_BALL_DAMPING, 
                                                         TOY_BALL_DONT_DIE_SCRIPT);
        }
    } else {
        //  If toy ball just released, add velocity to it!
        if (_toyBallInHand[handID]) {
        
            _toyBallInHand[handID] = false;
            glm::vec3 ballPosition = ballFromHand ? palm.getPosition() : fingerTipPosition;
            glm::vec3 handVelocity = palm.getRawVelocity();
            glm::vec3 fingerTipVelocity = palm.getTipVelocity();
            glm::quat avatarRotation = _owningAvatar->getOrientation();
            glm::vec3 toyBallVelocity = avatarRotation * fingerTipVelocity;

            // ball is no longer in hand...
            _ballParticleEditHandles[handID]->updateParticle(ballPosition / (float)TREE_SCALE,
                                                         TOY_BALL_RADIUS / (float) TREE_SCALE,
                                                         TOY_BALL_ON_SERVER_COLOR[_whichBallColor[handID]],
                                                         toyBallVelocity / (float)TREE_SCALE,
                                                         TOY_BALL_GRAVITY / (float) TREE_SCALE, 
                                                         TOY_BALL_DAMPING, 
                                                         TOY_BALL_UPDATE_SCRIPT);

            // after releasing the ball, we free our ParticleEditHandle so we can't edit it further
            // note: deleting the edit handle doesn't effect the actual particle
            delete _ballParticleEditHandles[handID];
            _ballParticleEditHandles[handID] = NULL;

        }
    }
    
    // remember the last pressed button state
    if (currentControllerButtons != 0) {
        _lastControllerButtons = currentControllerButtons;
    }
}

glm::vec3 Hand::getAndResetGrabDelta() {
    const float HAND_GRAB_SCALE_DISTANCE = 5.f;
    glm::vec3 delta = _grabDelta * _owningAvatar->getScale() * HAND_GRAB_SCALE_DISTANCE;
    _grabDelta = glm::vec3(0,0,0);
    glm::quat avatarRotation = _owningAvatar->getOrientation();
    return avatarRotation * -delta;
}

glm::vec3 Hand::getAndResetGrabDeltaVelocity() {
    const float HAND_GRAB_SCALE_VELOCITY = 5.f;
    glm::vec3 delta = _grabDeltaVelocity * _owningAvatar->getScale() * HAND_GRAB_SCALE_VELOCITY;
    _grabDeltaVelocity = glm::vec3(0,0,0);
    glm::quat avatarRotation = _owningAvatar->getOrientation();
    return avatarRotation * -delta;
    
}

void Hand::simulate(float deltaTime, bool isMine) {
    
    if (_collisionAge > 0.f) {
        _collisionAge += deltaTime;
    }
    
    const glm::vec3 leapHandsOffsetFromFace(0.0, -0.2, -0.3);  // place the hand in front of the face where we can see it
    
    Head& head = _owningAvatar->getHead();
    _baseOrientation = _owningAvatar->getOrientation();
    _basePosition = head.calculateAverageEyePosition() + _baseOrientation * leapHandsOffsetFromFace * head.getScale();
    
    if (isMine) {
        updateCollisions();
    }
    
    calculateGeometry();
    
    if (isMine) {
        
        //  Iterate hand controllers, take actions as needed
        
        for (size_t i = 0; i < getNumPalms(); ++i) {
            PalmData& palm = getPalms()[i];
            if (palm.isActive()) {
                FingerData& finger = palm.getFingers()[0];   //  Sixense has only one finger
                glm::vec3 fingerTipPosition = finger.getTipPosition();
                
                simulateToyBall(palm, fingerTipPosition, deltaTime);
                
                if (palm.getControllerButtons() & BUTTON_4) {
                    _grabDelta +=  palm.getRawVelocity() * deltaTime;
                }
                if ((palm.getLastControllerButtons() & BUTTON_4) && !(palm.getControllerButtons() & BUTTON_4)) {
                    _grabDeltaVelocity = palm.getRawVelocity();
                }
                
                if (palm.getControllerButtons() & BUTTON_1) {
                    if (glm::length(fingerTipPosition - _lastFingerAddVoxel) > (FINGERTIP_VOXEL_SIZE / 2.f)) {
                        QColor paintColor = Menu::getInstance()->getActionForOption(MenuOption::VoxelPaintColor)->data().value<QColor>();
                        Application::getInstance()->makeVoxel(fingerTipPosition,
                                                              FINGERTIP_VOXEL_SIZE,
                                                              paintColor.red(),
                                                              paintColor.green(),
                                                              paintColor.blue(),
                                                              true);
                        _lastFingerAddVoxel = fingerTipPosition;
                    }
                } else if (palm.getControllerButtons() & BUTTON_2) {
                    if (glm::length(fingerTipPosition - _lastFingerDeleteVoxel) > (FINGERTIP_VOXEL_SIZE / 2.f)) {
                        Application::getInstance()->removeVoxel(fingerTipPosition, FINGERTIP_VOXEL_SIZE);
                        _lastFingerDeleteVoxel = fingerTipPosition;
                    }
                }
                //  Check if the finger is intersecting with a voxel in the client voxel tree
                VoxelTreeElement* fingerNode = Application::getInstance()->getVoxels()->getVoxelEnclosing(
                                                                            glm::vec3(fingerTipPosition / (float)TREE_SCALE));
                if (fingerNode) {
                    if (!palm.getIsCollidingWithVoxel()) {
                        //  Collision has just started
                        palm.setIsCollidingWithVoxel(true);
                        handleVoxelCollision(&palm, fingerTipPosition, fingerNode, deltaTime);
                        //  Set highlight voxel
                        VoxelDetail voxel;
                        glm::vec3 pos = fingerNode->getCorner();
                        voxel.x = pos.x;
                        voxel.y = pos.y;
                        voxel.z = pos.z;
                        voxel.s = fingerNode->getScale();
                        voxel.red = fingerNode->getColor()[0];
                        voxel.green = fingerNode->getColor()[1];
                        voxel.blue = fingerNode->getColor()[2];
                        Application::getInstance()->setHighlightVoxel(voxel);
                        Application::getInstance()->setIsHighlightVoxel(true);
                    }
                } else {
                    if (palm.getIsCollidingWithVoxel()) {
                        //  Collision has just ended
                        palm.setIsCollidingWithVoxel(false);
                        Application::getInstance()->setIsHighlightVoxel(false);
                    }
                }
            }
            palm.setLastControllerButtons(palm.getControllerButtons());
        }
    }
}

void Hand::updateCollisions() {
    // use position to obtain the left and right palm indices
    int leftPalmIndex, rightPalmIndex;   
    getLeftRightPalmIndices(leftPalmIndex, rightPalmIndex);
    
    // check for collisions
    for (int i = 0; i < getNumPalms(); i++) {
        PalmData& palm = getPalms()[i];
        if (!palm.isActive()) {
            continue;
        }        
        float scaledPalmRadius = PALM_COLLISION_RADIUS * _owningAvatar->getScale();
        glm::vec3 totalPenetration;
        
        // check other avatars
        NodeList* nodeList = NodeList::getInstance();
        for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
            if (node->getLinkedData() && node->getType() == NODE_TYPE_AGENT) {
                Avatar* otherAvatar = (Avatar*)node->getLinkedData();
                //  Check for palm collisions
                glm::vec3 myPalmPosition = palm.getPosition();
                float palmCollisionDistance = 0.1f;
                palm.setIsCollidingWithPalm(false);
                for (int j = 0; j < otherAvatar->getHand().getNumPalms(); j++) {
                    PalmData& otherPalm = otherAvatar->getHand().getPalms()[j];
                    if (!otherPalm.isActive()) {
                        continue;
                    }
                    glm::vec3 otherPalmPosition = otherPalm.getPosition();
                    if (glm::length(otherPalmPosition - myPalmPosition) < palmCollisionDistance) {
                        palm.setIsCollidingWithPalm(true);
                        const float PALM_COLLIDE_VOLUME = 1.f;
                        const float PALM_COLLIDE_FREQUENCY = 150.f;
                        const float PALM_COLLIDE_DURATION_MAX = 2.f;
                        const float PALM_COLLIDE_DECAY_PER_SAMPLE = 0.005f;
                        Application::getInstance()->getAudio()->startDrumSound(PALM_COLLIDE_VOLUME,
                                                                               PALM_COLLIDE_FREQUENCY,
                                                                               PALM_COLLIDE_DURATION_MAX,
                                                                               PALM_COLLIDE_DECAY_PER_SAMPLE);

                        
                    }
                }
                glm::vec3 avatarPenetration;
                if (otherAvatar->findSpherePenetration(palm.getPosition(), scaledPalmRadius, avatarPenetration)) {
                    totalPenetration = addPenetrations(totalPenetration, avatarPenetration);
                    //  Check for collisions with the other avatar's leap palms
                }
            }
        }
            
        // and the current avatar (ignoring everything below the parent of the parent of the last free joint)
        glm::vec3 owningPenetration;
        const Model& skeletonModel = _owningAvatar->getSkeletonModel();
        int skipIndex = skeletonModel.getParentJointIndex(skeletonModel.getParentJointIndex(
            skeletonModel.getLastFreeJointIndex((i == leftPalmIndex) ? skeletonModel.getLeftHandJointIndex() :
                (i == rightPalmIndex) ? skeletonModel.getRightHandJointIndex() : -1)));
        if (_owningAvatar->findSpherePenetration(palm.getPosition(), scaledPalmRadius, owningPenetration, skipIndex)) {
            totalPenetration = addPenetrations(totalPenetration, owningPenetration);
        }
        
        // un-penetrate
        palm.addToPosition(-totalPenetration);
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
                    const float standardBallRadius = 0.010f;
                    _leapFingerTipBalls.resize(_leapFingerTipBalls.size() + 1);
                    HandBall& ball = _leapFingerTipBalls.back();
                    ball.rotation = _baseOrientation;
                    ball.position = finger.getTipPosition();
                    ball.radius         = standardBallRadius;
                    ball.touchForce     = 0.0;
                    ball.isCollidable   = true;
                    ball.isColliding    = false;
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
                    _leapFingerRootBalls.resize(_leapFingerRootBalls.size() + 1);
                    HandBall& ball = _leapFingerRootBalls.back();
                    ball.rotation = _baseOrientation;
                    ball.position = finger.getRootPosition();
                    ball.radius         = standardBallRadius;
                    ball.touchForce     = 0.0;
                    ball.isCollidable   = true;
                    ball.isColliding    = false;
                }
            }
        }
    }
}

void Hand::render( bool isMine) {
    
    _renderAlpha = 1.0;
    
    if (Menu::getInstance()->isOptionChecked(MenuOption::CollisionProxies)) {
        for (int i = 0; i < getNumPalms(); i++) {
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
    
    if (Menu::getInstance()->isOptionChecked(MenuOption::DisplayLeapHands)) {
        renderLeapHands();
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

void Hand::renderLeapHands() {

    const float alpha = 1.0f;
    //const glm::vec3 handColor = _ballColor;
    const glm::vec3 handColor(1.0, 0.84, 0.66); // use the skin color
    

    
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    if (Menu::getInstance()->isOptionChecked(MenuOption::DisplayHandTargets)) {
        for (size_t i = 0; i < getNumPalms(); ++i) {
            PalmData& palm = getPalms()[i];
            if (!palm.isActive()) {
                continue;
            }
            glm::vec3 targetPosition = palm.getPosition();
            float targetRadius = (TOY_BALL_RADIUS * 4.0f);
            glPushMatrix();
        
            const Particle* closestParticle = Application::getInstance()->getParticles()
                                                        ->getTree()->findClosestParticle(targetPosition / (float)TREE_SCALE, 
                                                                                         targetRadius / (float)TREE_SCALE);
                                                                                         
            // If we are hitting a particle then draw the target green, otherwise yellow
            if (closestParticle) {
                glColor4f(0,1,0, alpha);
            } else {
                glColor4f(1,1,0, alpha);
            }
            glTranslatef(targetPosition.x, targetPosition.y, targetPosition.z);
            glutWireSphere(targetRadius, 20.0f, 20.0f);
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
                    Avatar::renderJointConnectingCone(root, tip, 0.001, 0.003);
                }
            }
        }
    }

    // Draw the palms
    for (size_t i = 0; i < getNumPalms(); ++i) {
        PalmData& palm = getPalms()[i];
        if (palm.isActive()) {
            const float palmThickness = 0.02f;
            if (palm.getIsCollidingWithPalm()) {
                glColor4f(1, 0, 0, 0.50);
            } else {
                glColor4f(handColor.r, handColor.g, handColor.b, 0.25);
            }
            glm::vec3 tip = palm.getPosition();
            glm::vec3 root = palm.getPosition() + palm.getNormal() * palmThickness;
            const float radiusA = 0.05f;
            const float radiusB = 0.03f;
            Avatar::renderJointConnectingCone(root, tip, radiusA, radiusB);
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






