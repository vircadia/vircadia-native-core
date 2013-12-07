//
//  MyAvatar.h
//  interface
//
//  Created by Mark Peng on 8/16/13.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__myavatar__
#define __interface__myavatar__

#include <QSettings>

#include "Avatar.h"

enum AvatarHandState
{
    HAND_STATE_NULL = 0,
    HAND_STATE_OPEN,
    HAND_STATE_GRASPING,
    HAND_STATE_POINTING,
    NUM_HAND_STATES
};

class MyAvatar : public Avatar {
public:
	MyAvatar(Node* owningNode = NULL);

    void reset();
    void simulate(float deltaTime, Transmitter* transmitter);
    void updateFromGyrosAndOrWebcam(bool turnWithHead);
    void render(bool forceRenderHead);

    // setters
    void setMousePressed(bool mousePressed) { _mousePressed = mousePressed; }
    void setThrust(glm::vec3 newThrust) { _thrust = newThrust; }
    void setVelocity(const glm::vec3 velocity) { _velocity = velocity; }
    void setLeanScale(float scale) { _leanScale = scale; }
    void setGravity(glm::vec3 gravity);
    void setOrientation(const glm::quat& orientation);
    void setWantCollisionsOn(bool wantCollisionsOn) { _isCollisionsOn = wantCollisionsOn; }
    void setMoveTarget(const glm::vec3 moveTarget);

    // getters
    float getSpeed() const { return _speed; }
    AvatarMode getMode() const { return _mode; }
    float getLeanScale() const { return _leanScale; }
    float getElapsedTimeStopped() const { return _elapsedTimeStopped; }
    float getElapsedTimeMoving() const { return _elapsedTimeMoving; }
    float getAbsoluteHeadYaw() const;
    const glm::vec3& getMouseRayOrigin() const { return _mouseRayOrigin; }
    const glm::vec3& getMouseRayDirection() const { return _mouseRayDirection; }
    Avatar* getLeadingAvatar() const { return _leadingAvatar; }
    glm::vec3 getGravity() const { return _gravity; }
    glm::vec3 getUprightHeadPosition() const;
    glm::vec3 getEyeLevelPosition() const;
    
    // get/set avatar data
    void saveData(QSettings* settings);
    void loadData(QSettings* settings);

    //  Set what driving keys are being pressed to control thrust levels
    void setDriveKeys(int key, float val) { _driveKeys[key] = val; };
    bool getDriveKeys(int key) { return _driveKeys[key]; };
    void jump() { _shouldJump = true; };

    //  Set/Get update the thrust that will move the avatar around
    void addThrust(glm::vec3 newThrust) { _thrust += newThrust; };
    glm::vec3 getThrust() { return _thrust; };

private:
    bool _mousePressed;
    float _bodyPitchDelta;
    float _bodyRollDelta;
    bool _shouldJump;
    float _driveKeys[MAX_DRIVE_KEYS];
    glm::vec3 _gravity;
    float _distanceToNearestAvatar; // How close is the nearest avatar?
    float _elapsedTimeMoving; // Timers to drive camera transitions when moving
    float _elapsedTimeStopped;
    float _elapsedTimeSinceCollision;
    glm::vec3 _lastCollisionPosition;
    bool _speedBrakes;
    bool _isThrustOn;
    float _thrustMultiplier;
    float _collisionRadius;
    glm::vec3 _moveTarget;
    int _moveTargetStepCounter;

	// private methods
    void renderBody(bool forceRenderHead);
    void updateThrust(float deltaTime, Transmitter * transmitter);
    void updateHandMovementAndTouching(float deltaTime, bool enableHandMovement);
    void updateAvatarCollisions(float deltaTime);
    void updateCollisionWithEnvironment(float deltaTime);
    void updateCollisionWithVoxels(float deltaTime);
    void applyHardCollision(const glm::vec3& penetration, float elasticity, float damping);
    void updateCollisionSound(const glm::vec3& penetration, float deltaTime, float frequency);
    void updateChatCircle(float deltaTime);
};

#endif
