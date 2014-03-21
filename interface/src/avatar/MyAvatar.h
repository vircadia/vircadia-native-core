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
    Q_OBJECT
    Q_PROPERTY(bool shouldRenderLocally READ getShouldRenderLocally WRITE setShouldRenderLocally)

public:
	MyAvatar();
    ~MyAvatar();
    
    void reset();
    void update(float deltaTime);
    void simulate(float deltaTime);
    void updateFromGyros(float deltaTime);

    void render(const glm::vec3& cameraPosition, bool forShadowMapOrMirror = false);
    void renderBody(bool forceRenderHead);
    void renderDebugBodyPoints();
    void renderHeadMouse() const;

    // setters
    void setMousePressed(bool mousePressed) { _mousePressed = mousePressed; }
    void setVelocity(const glm::vec3 velocity) { _velocity = velocity; }
    void setLeanScale(float scale) { _leanScale = scale; }
    void setGravity(glm::vec3 gravity);
    void setMoveTarget(const glm::vec3 moveTarget);
    void setShouldRenderLocally(bool shouldRender) { _shouldRender = shouldRender; }

    // getters
    float getSpeed() const { return _speed; }
    AvatarMode getMode() const { return _mode; }
    float getLeanScale() const { return _leanScale; }
    float getElapsedTimeStopped() const { return _elapsedTimeStopped; }
    float getElapsedTimeMoving() const { return _elapsedTimeMoving; }
    float getAbsoluteHeadYaw() const;   // degrees
    const glm::vec3& getMouseRayOrigin() const { return _mouseRayOrigin; }
    const glm::vec3& getMouseRayDirection() const { return _mouseRayDirection; }
    glm::vec3 getGravity() const { return _gravity; }
    glm::vec3 getUprightHeadPosition() const;
    bool getShouldRenderLocally() const { return _shouldRender; }
    
    // get/set avatar data
    void saveData(QSettings* settings);
    void loadData(QSettings* settings);

    //  Set what driving keys are being pressed to control thrust levels
    void setDriveKeys(int key, float val) { _driveKeys[key] = val; };
    bool getDriveKeys(int key) { return _driveKeys[key] != 0.f; };
    void jump() { _shouldJump = true; };
    
    bool isMyAvatar() { return true; }
    
    static void sendKillAvatar();

    void orbit(const glm::vec3& position, int deltaX, int deltaY);

    Q_INVOKABLE glm::vec3 getTargetAvatarPosition() const { return _targetAvatarPosition; }
    AvatarData* getLookAtTargetAvatar() const { return _lookAtTargetAvatar.data(); }
    void updateLookAtTargetAvatar();
    void clearLookAtTargetAvatar();
    
    virtual void setJointData(int index, const glm::quat& rotation);
    virtual void clearJointData(int index);
    virtual void setFaceModelURL(const QUrl& faceModelURL);
    virtual void setSkeletonModelURL(const QUrl& skeletonModelURL);
public slots:
    void goHome();
    void increaseSize();
    void decreaseSize();
    void resetSize();
    
    void updateLocationInDataServer();
    void goToLocationFromResponse(const QJsonObject& jsonObject);

    //  Set/Get update the thrust that will move the avatar around
    void addThrust(glm::vec3 newThrust) { _thrust += newThrust; };
    glm::vec3 getThrust() { return _thrust; };
    void setThrust(glm::vec3 newThrust) { _thrust = newThrust; }

private:
    bool _mousePressed;
    float _bodyPitchDelta;  // degrees
    float _bodyRollDelta;   // degrees
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
    glm::vec3 _moveTarget;
    int _moveTargetStepCounter;
    QWeakPointer<AvatarData> _lookAtTargetAvatar;
    glm::vec3 _targetAvatarPosition;
    bool _shouldRender;

    bool _billboardValid;

	// private methods
    void updateThrust(float deltaTime);
    void updateHandMovementAndTouching(float deltaTime);
    void updateCollisionWithAvatars(float deltaTime);
    void updateCollisionWithEnvironment(float deltaTime, float radius);
    void updateCollisionWithVoxels(float deltaTime, float radius);
    void applyHardCollision(const glm::vec3& penetration, float elasticity, float damping);
    void updateCollisionSound(const glm::vec3& penetration, float deltaTime, float frequency);
    void updateChatCircle(float deltaTime);
    void maybeUpdateBillboard();
};

#endif
