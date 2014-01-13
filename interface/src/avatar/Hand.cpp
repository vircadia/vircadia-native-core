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
const float FINGERTIP_VOXEL_SIZE = 0.05f;
const int TOY_BALL_HAND = 1;
const float TOY_BALL_RADIUS = 0.05f;
const float TOY_BALL_DAMPING = 0.1f;
const glm::vec3 NO_VELOCITY = glm::vec3(0,0,0);
const glm::vec3 NO_GRAVITY = glm::vec3(0,0,0);
const float NO_DAMPING = 0.f;
const glm::vec3 TOY_BALL_GRAVITY = glm::vec3(0,-2.0f,0);
const QString TOY_BALL_UPDATE_SCRIPT("");
const float PALM_COLLISION_RADIUS = 0.03f;
const float CATCH_RADIUS = 0.3f;
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
    _grabDeltaVelocity(0, 0, 0),
    _grabStartRotation(0, 0, 0, 1),
    _grabCurrentRotation(0, 0, 0, 1),
    _throwSound(QUrl("https://dl.dropboxusercontent.com/u/1864924/hifi-sounds/throw.raw")),
    _catchSound(QUrl("https://dl.dropboxusercontent.com/u/1864924/hifi-sounds/catch.raw"))
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
    Application* app = Application::getInstance();
    ParticleTree* particles = app->getParticles()->getTree();
    bool ballFromHand = Menu::getInstance()->isOptionChecked(MenuOption::BallFromHand);
    int handID = palm.getSixenseID();
    
    const int NEW_BALL_BUTTON = BUTTON_3;
    
    bool grabButtonPressed = ((palm.getControllerButtons() & BUTTON_FWD) ||
                              (palm.getControllerButtons() & BUTTON_3));
    
    bool ballAlreadyInHand = _toyBallInHand[handID];

    glm::vec3 targetPosition = (ballFromHand ? palm.getPosition() : fingerTipPosition) / (float)TREE_SCALE;
    float targetRadius = CATCH_RADIUS / (float)TREE_SCALE;

    // If I don't currently have a ball in my hand, then try to catch closest one
    if (!ballAlreadyInHand && grabButtonPressed) {

        const Particle* closestParticle = particles->findClosestParticle(targetPosition, targetRadius);

        if (closestParticle) {
            ParticleEditHandle* caughtParticle = app->newParticleEditHandle(closestParticle->getID());
            glm::vec3 newPosition = targetPosition;
            glm::vec3 newVelocity = NO_VELOCITY;
            
            // update the particle with it's new state...
            caughtParticle->updateParticle(newPosition,
                                            closestParticle->getRadius(),
                                            closestParticle->getXColor(),
                                            newVelocity,
                                            NO_GRAVITY,
                                            NO_DAMPING,
                                            IN_HAND, // we just grabbed it!
                                            closestParticle->getScript());
            
                                            
            // now tell our hand about us having caught it...
            _toyBallInHand[handID] = true;

            //printf(">>>>>>> caught... handID:%d particle ID:%d _toyBallInHand[handID] = true\n", handID, closestParticle->getID());
            _ballParticleEditHandles[handID] = caughtParticle;
            caughtParticle = NULL;
            
            // use the threadSound static method to inject the catch sound
            // pass an AudioInjectorOptions struct to set position and disable loopback
            AudioInjectorOptions injectorOptions;
            injectorOptions.setPosition(newPosition);
            injectorOptions.setLoopbackAudioInterface(app->getAudio());
    
            AudioScriptingInterface::playSound(&_catchSound, &injectorOptions);
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

    //  If '3' is pressed, and not holding a ball, make a new one
    if ((palm.getControllerButtons() & NEW_BALL_BUTTON) && (_toyBallInHand[handID] == false)) {
        _toyBallInHand[handID] = true;
        //  Create a particle on the particle server
        glm::vec3 ballPosition = ballFromHand ? palm.getPosition() : fingerTipPosition;
        _ballParticleEditHandles[handID] = app->makeParticle(
                                                             ballPosition / (float)TREE_SCALE,
                                                             TOY_BALL_RADIUS / (float) TREE_SCALE,
                                                             TOY_BALL_ON_SERVER_COLOR[_whichBallColor[handID]],
                                                             NO_VELOCITY / (float)TREE_SCALE,
                                                             TOY_BALL_GRAVITY / (float) TREE_SCALE,
                                                             TOY_BALL_DAMPING,
                                                             IN_HAND,
                                                             TOY_BALL_UPDATE_SCRIPT);
        // Play a new ball sound
        app->getAudio()->startDrumSound(1.0f, 2000, 0.5f, 0.02f);
    }

    if (grabButtonPressed) {
        // If we don't currently have a ball in hand, then create it...
        if (_toyBallInHand[handID]) {
            // Update ball that is in hand
            uint32_t particleInHandID = _ballParticleEditHandles[handID]->getID();
            const Particle* particleInHand = particles->findParticleByID(particleInHandID);
            xColor colorForParticleInHand = particleInHand ? particleInHand->getXColor() 
                                                    : TOY_BALL_ON_SERVER_COLOR[_whichBallColor[handID]];

            glm::vec3 ballPosition = ballFromHand ? palm.getPosition() : fingerTipPosition;
            _ballParticleEditHandles[handID]->updateParticle(ballPosition / (float)TREE_SCALE,
                                                         TOY_BALL_RADIUS / (float) TREE_SCALE,
                                                         colorForParticleInHand, 
                                                         NO_VELOCITY / (float)TREE_SCALE,
                                                         TOY_BALL_GRAVITY / (float) TREE_SCALE, 
                                                         TOY_BALL_DAMPING,
                                                         IN_HAND,
                                                         TOY_BALL_UPDATE_SCRIPT);
        }
    } else {
        //  If toy ball just released, add velocity to it!
        if (_toyBallInHand[handID]) {
        
            const float THROWN_VELOCITY_SCALING = 1.5f;
            _toyBallInHand[handID] = false;
            glm::vec3 ballPosition = ballFromHand ? palm.getPosition() : fingerTipPosition;
            glm::vec3 ballVelocity = ballFromHand ? palm.getRawVelocity() : palm.getTipVelocity();
            glm::quat avatarRotation = _owningAvatar->getOrientation();
            ballVelocity = avatarRotation * ballVelocity;
            ballVelocity *= THROWN_VELOCITY_SCALING;

            uint32_t particleInHandID = _ballParticleEditHandles[handID]->getID();
            const Particle* particleInHand = particles->findParticleByID(particleInHandID);
            xColor colorForParticleInHand = particleInHand ? particleInHand->getXColor() 
                                                    : TOY_BALL_ON_SERVER_COLOR[_whichBallColor[handID]];

            _ballParticleEditHandles[handID]->updateParticle(ballPosition / (float)TREE_SCALE,
                                                         TOY_BALL_RADIUS / (float) TREE_SCALE,
                                                         colorForParticleInHand,
                                                         ballVelocity / (float)TREE_SCALE,
                                                         TOY_BALL_GRAVITY / (float) TREE_SCALE, 
                                                         TOY_BALL_DAMPING,
                                                         NOT_IN_HAND,
                                                         TOY_BALL_UPDATE_SCRIPT);

            // after releasing the ball, we free our ParticleEditHandle so we can't edit it further
            // note: deleting the edit handle doesn't effect the actual particle
            delete _ballParticleEditHandles[handID];
            _ballParticleEditHandles[handID] = NULL;
            
            // use the threadSound static method to inject the throw sound
            // pass an AudioInjectorOptions struct to set position and disable loopback
            AudioInjectorOptions injectorOptions;
            injectorOptions.setPosition(ballPosition);
            injectorOptions.setLoopbackAudioInterface(app->getAudio());
            
            AudioScriptingInterface::playSound(&_throwSound, &injectorOptions);
        }
    }
    
    // remember the last pressed button state
    if (currentControllerButtons != 0) {
        _lastControllerButtons = currentControllerButtons;
    }
}

glm::vec3 Hand::getAndResetGrabDelta() {
    const float HAND_GRAB_SCALE_DISTANCE = 2.f;
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
glm::quat Hand::getAndResetGrabRotation() {
    glm::quat diff = _grabCurrentRotation * glm::inverse(_grabStartRotation);
    _grabStartRotation = _grabCurrentRotation;
    return diff;
}

void Hand::simulate(float deltaTime, bool isMine) {
    
    if (_collisionAge > 0.f) {
        _collisionAge += deltaTime;
    }
    
    if (isMine) {
        _buckyBalls.simulate(deltaTime);
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
                
                _buckyBalls.grab(palm, fingerTipPosition, _owningAvatar->getOrientation(), deltaTime);
                
                if (palm.getControllerButtons() & BUTTON_4) {
                    _grabDelta +=  palm.getRawVelocity() * deltaTime;
                    _grabCurrentRotation = palm.getRawRotation();
                }
                if ((palm.getLastControllerButtons() & BUTTON_4) && !(palm.getControllerButtons() & BUTTON_4)) {
                    // Just ending grab, capture velocity
                    _grabDeltaVelocity = palm.getRawVelocity();
                }
                if (!(palm.getLastControllerButtons() & BUTTON_4) && (palm.getControllerButtons() & BUTTON_4)) {
                    // Just starting grab, capture starting rotation
                    _grabStartRotation = palm.getRawRotation();
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
                
                //  Voxel Drumming with fingertips if enabled
                if (Menu::getInstance()->isOptionChecked(MenuOption::VoxelDrumming)) {
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
    for (size_t i = 0; i < getNumPalms(); i++) {
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
                if (Menu::getInstance()->isOptionChecked(MenuOption::PlaySlaps)) {
                    //  Check for palm collisions
                    glm::vec3 myPalmPosition = palm.getPosition();
                    float palmCollisionDistance = 0.1f;
                    bool wasColliding = palm.getIsCollidingWithPalm();
                    palm.setIsCollidingWithPalm(false);
                    //  If 'Play Slaps' is enabled, look for palm-to-palm collisions and make sound
                    for (size_t j = 0; j < otherAvatar->getHand().getNumPalms(); j++) {
                        PalmData& otherPalm = otherAvatar->getHand().getPalms()[j];
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
                    const float standardBallRadius = FINGERTIP_COLLISION_RADIUS;
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

void Hand::render(bool isMine) {
    
    _renderAlpha = 1.0;
    
    if (isMine) {
        _buckyBalls.render();
    }
    
    if (Menu::getInstance()->isOptionChecked(MenuOption::CollisionProxies)) {
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

    
    if (Menu::getInstance()->isOptionChecked(MenuOption::DisplayLeapHands)) {
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
    const float TARGET_ALPHA = 0.5f;
    
    //const glm::vec3 handColor = _ballColor;
    const glm::vec3 handColor(1.0, 0.84, 0.66); // use the skin color
    bool ballFromHand = Menu::getInstance()->isOptionChecked(MenuOption::BallFromHand);
    
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    if (isMine && Menu::getInstance()->isOptionChecked(MenuOption::DisplayHandTargets)) {
        for (size_t i = 0; i < getNumPalms(); ++i) {
            PalmData& palm = getPalms()[i];
            if (!palm.isActive()) {
                continue;
            }
            glm::vec3 targetPosition = ballFromHand ? palm.getPosition() : palm.getTipPosition();
            glPushMatrix();
        
            ParticleTree* particles = Application::getInstance()->getParticles()->getTree();
            const Particle* closestParticle = particles->findClosestParticle(targetPosition / (float)TREE_SCALE,
                                                                                         CATCH_RADIUS / (float)TREE_SCALE);
                                                                                         
            // If we are hitting a particle then draw the target green, otherwise yellow
            if (closestParticle) {
                glColor4f(0,1,0, TARGET_ALPHA);
            } else {
                glColor4f(1,1,0, TARGET_ALPHA);
            }
            glTranslatef(targetPosition.x, targetPosition.y, targetPosition.z);
            glutWireSphere(CATCH_RADIUS, 10.f, 10.f);
            
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






