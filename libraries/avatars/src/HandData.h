//
//  HandData.h
//  libraries/avatars/src
//
//  Created by Eric Johnston on 6/26/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HandData_h
#define hifi_HandData_h

#include <iostream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "SharedUtil.h"

class AvatarData;
class PalmData;

const int LEFT_HAND_INDEX = 0;
const int RIGHT_HAND_INDEX = 1;
const int NUM_HANDS = 2;

const int SIXENSEID_INVALID = -1;

class HandData {
public:
    HandData(AvatarData* owningAvatar);
    virtual ~HandData() {}
    
    // position conversion
    glm::vec3 localToWorldPosition(const glm::vec3& localPosition) {
        return getBasePosition() + getBaseOrientation() * localPosition;
    }

    glm::vec3 localToWorldDirection(const glm::vec3& localVector) {
        return getBaseOrientation() * localVector;
    }

    glm::vec3 worldToLocalVector(const glm::vec3& worldVector) const;

    std::vector<PalmData>& getPalms() { return _palms; }
    const std::vector<PalmData>& getPalms() const { return _palms; }
    const PalmData* getPalm(int sixSenseID) const;
    size_t getNumPalms() const { return _palms.size(); }
    PalmData& addNewPalm();

    /// Finds the indices of the left and right palms according to their locations, or -1 if either or
    /// both is not found.
    void getLeftRightPalmIndices(int& leftPalmIndex, int& rightPalmIndex) const;

    /// Checks for penetration between the described sphere and the hand.
    /// \param penetratorCenter the center of the penetration test sphere
    /// \param penetratorRadius the radius of the penetration test sphere
    /// \param penetration[out] the vector in which to store the penetration
    /// \param collidingPalm[out] a const PalmData* to the palm that was collided with
    /// \return whether or not the sphere penetrated
    bool findSpherePenetration(const glm::vec3& penetratorCenter, float penetratorRadius, glm::vec3& penetration, 
        const PalmData*& collidingPalm) const;

    friend class AvatarData;
protected:
    AvatarData* _owningAvatarData;
    std::vector<PalmData> _palms;
    
    glm::quat getBaseOrientation() const;
    glm::vec3 getBasePosition() const;
    
private:
    // privatize copy ctor and assignment operator so copies of this object cannot be made
    HandData(const HandData&);
    HandData& operator= (const HandData&);
};


class PalmData {
public:
    PalmData(HandData* owningHandData);
    glm::vec3 getPosition() const { return _owningHandData->localToWorldPosition(_rawPosition); }
    glm::vec3 getVelocity() const { return _owningHandData->localToWorldDirection(_rawVelocity); }

    const glm::vec3& getRawPosition() const { return _rawPosition; }
    bool isActive() const { return _isActive; }
    int getSixenseID() const { return _sixenseID; }

    void setActive(bool active) { _isActive = active; }
    void setSixenseID(int id) { _sixenseID = id; }

    void setRawRotation(const glm::quat rawRotation) { _rawRotation = rawRotation; };
    glm::quat getRawRotation() const { return _rawRotation; }
    void setRawPosition(const glm::vec3& pos)  { _rawPosition = pos; }
    void setRawVelocity(const glm::vec3& velocity) { _rawVelocity = velocity; }
    const glm::vec3& getRawVelocity()  const { return _rawVelocity; }
    void addToPosition(const glm::vec3& delta);

    void addToPenetration(const glm::vec3& penetration) { _totalPenetration += penetration; }
    void resolvePenetrations() { addToPosition(-_totalPenetration); _totalPenetration = glm::vec3(0.0f); }
    
    void setTipPosition(const glm::vec3& position) { _tipPosition = position; }
    const glm::vec3 getTipPosition() const { return _owningHandData->localToWorldPosition(_tipPosition); }
    const glm::vec3& getTipRawPosition() const { return _tipPosition; }

    void setTipVelocity(const glm::vec3& velocity) { _tipVelocity = velocity; }
    const glm::vec3 getTipVelocity() const { return _owningHandData->localToWorldDirection(_tipVelocity); }
    const glm::vec3& getTipRawVelocity() const { return _tipVelocity; }
    
    void incrementFramesWithoutData() { _numFramesWithoutData++; }
    void resetFramesWithoutData() { _numFramesWithoutData = 0; }
    int  getFramesWithoutData() const { return _numFramesWithoutData; }
    
    // Controller buttons
    void setControllerButtons(unsigned int controllerButtons) { _controllerButtons = controllerButtons; }
    void setLastControllerButtons(unsigned int controllerButtons) { _lastControllerButtons = controllerButtons; }

    unsigned int getControllerButtons() const { return _controllerButtons; }
    unsigned int getLastControllerButtons() const { return _lastControllerButtons; }
    
    void setTrigger(float trigger) { _trigger = trigger; }
    float getTrigger() const { return _trigger; }
    void setJoystick(float joystickX, float joystickY) { _joystickX = joystickX; _joystickY = joystickY; }
    float getJoystickX() const { return _joystickX; }
    float getJoystickY() const { return _joystickY; }
    
    bool getIsCollidingWithVoxel() const { return _isCollidingWithVoxel; }
    void setIsCollidingWithVoxel(bool isCollidingWithVoxel) { _isCollidingWithVoxel = isCollidingWithVoxel; }

    bool getIsCollidingWithPalm() const { return _isCollidingWithPalm; }
    void setIsCollidingWithPalm(bool isCollidingWithPalm) { _isCollidingWithPalm = isCollidingWithPalm; }

    bool hasPaddle() const { return _collisionlessPaddleExpiry < usecTimestampNow(); }
    void updateCollisionlessPaddleExpiry() { _collisionlessPaddleExpiry = usecTimestampNow() + USECS_PER_SECOND; }

    /// Store position where the palm holds the ball.
    void getBallHoldPosition(glm::vec3& position) const;

    // return world-frame:
    glm::vec3 getFingerDirection() const;
    glm::vec3 getNormal() const;

private:
    glm::quat _rawRotation;
    glm::vec3 _rawPosition;
    glm::vec3 _rawVelocity;
    glm::vec3 _rotationalVelocity;
    glm::quat _lastRotation;
    
    glm::vec3 _tipPosition;
    glm::vec3 _tipVelocity;
    glm::vec3 _totalPenetration;    // accumulator for per-frame penetrations
    unsigned int _controllerButtons;
    unsigned int _lastControllerButtons;
    float _trigger;
    float _joystickX, _joystickY;
    
    bool      _isActive;             // This has current valid data
    int       _sixenseID;            // Sixense controller ID for this palm
    int       _numFramesWithoutData; // after too many frames without data, this tracked object assumed lost.
    HandData* _owningHandData;
    
    bool      _isCollidingWithVoxel;  /// Whether the finger of this palm is inside a leaf voxel
    bool      _isCollidingWithPalm;
    quint64  _collisionlessPaddleExpiry; /// Timestamp after which paddle starts colliding
};

#endif // hifi_HandData_h
