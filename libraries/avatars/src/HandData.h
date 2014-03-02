//
//  HandData.h
//  hifi
//
//  Created by Eric Johnston on 6/26/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__HandData__
#define __hifi__HandData__

#include <iostream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "SharedUtil.h"

class AvatarData;
class FingerData;
class PalmData;

const int NUM_HANDS = 2;
const int NUM_FINGERS_PER_HAND = 5;
const int NUM_FINGERS = NUM_HANDS * NUM_FINGERS_PER_HAND;

const int LEAPID_INVALID = -1;
const int SIXENSEID_INVALID = -1;

const int BUTTON_0 = 1; // the skinny button between 1 and 2
const int BUTTON_1 = 32;
const int BUTTON_2 = 64;
const int BUTTON_3 = 8;
const int BUTTON_4 = 16;
const int BUTTON_FWD = 128;

const float LEAP_UNIT_SCALE = 0.001f; ///< convert mm to meters

const int SIXENSE_CONTROLLER_ID_LEFT_HAND = 0;
const int SIXENSE_CONTROLLER_ID_RIGHT_HAND = 1;

class HandData {
public:
    HandData(AvatarData* owningAvatar);
    virtual ~HandData() {}
    
    // These methods return the positions in Leap-relative space.
    // To convert to world coordinates, use Hand::leapPositionToWorldPosition.

    // position conversion
    glm::vec3 leapPositionToWorldPosition(const glm::vec3& leapPosition) {
        return getBasePosition() + getBaseOrientation() * (leapPosition * LEAP_UNIT_SCALE);
    }
    glm::vec3 leapDirectionToWorldDirection(const glm::vec3& leapDirection) {
        return getBaseOrientation() * leapDirection;
    }
    glm::vec3 worldPositionToLeapPosition(const glm::vec3& worldPosition) const;
    glm::vec3 worldVectorToLeapVector(const glm::vec3& worldVector) const;

    std::vector<PalmData>& getPalms() { return _palms; }
    const std::vector<PalmData>& getPalms() const { return _palms; }
    const PalmData* getPalm(int sixSenseID) const;
    size_t getNumPalms() const { return _palms.size(); }
    PalmData& addNewPalm();

    /// Finds the indices of the left and right palms according to their locations, or -1 if either or
    /// both is not found.
    void getLeftRightPalmIndices(int& leftPalmIndex, int& rightPalmIndex) const;

    void setFingerTrailLength(unsigned int length);
    void updateFingerTrails();

    static int encodeData(HandData* hand, unsigned char* destinationBuffer);

    // Use these for sending and receiving hand data
    int encodeRemoteData(unsigned char* destinationBuffer);
    int decodeRemoteData(const QByteArray& dataByteArray);

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
    std::vector<PalmData>  _palms;
    
    glm::quat getBaseOrientation() const;
    glm::vec3 getBasePosition() const;
    
private:
    // privatize copy ctor and assignment operator so copies of this object cannot be made
    HandData(const HandData&);
    HandData& operator= (const HandData&);
};

class FingerData {
public:
    FingerData(PalmData* owningPalmData, HandData* owningHandData);

    glm::vec3        getTipPosition()     const { return _owningHandData->leapPositionToWorldPosition(_tipRawPosition); }
    glm::vec3        getRootPosition()    const { return _owningHandData->leapPositionToWorldPosition(_rootRawPosition); }
    const glm::vec3& getTipRawPosition()  const { return _tipRawPosition; }
    const glm::vec3& getRootRawPosition() const { return _rootRawPosition; }
    bool             isActive()           const { return _isActive; }
    int              getLeapID()          const { return _leapID; }

    void setActive(bool active)                   { _isActive = active; }
    void setLeapID(int id)                        { _leapID = id; }
    void setRawTipPosition(const glm::vec3& pos)  { _tipRawPosition = pos; }
    void setRawRootPosition(const glm::vec3& pos) { _rootRawPosition = pos; }

    void setTrailLength(unsigned int length);
    void updateTrail();

    int              getTrailNumPositions();
    const glm::vec3& getTrailPosition(int index);

    void incrementFramesWithoutData()          { _numFramesWithoutData++; }
    void resetFramesWithoutData()              { _numFramesWithoutData = 0; }
    int  getFramesWithoutData()          const { return _numFramesWithoutData; }
    
private:
    glm::vec3 _tipRawPosition;
    glm::vec3 _rootRawPosition;
    bool      _isActive;            // This has current valid data
    int       _leapID;              // the Leap's serial id for this tracked object
    int       _numFramesWithoutData; // after too many frames without data, this tracked object assumed lost.
    std::vector<glm::vec3> _tipTrailPositions;
    int                    _tipTrailCurrentStartIndex;
    int                    _tipTrailCurrentValidLength;
    PalmData* _owningPalmData;
    HandData* _owningHandData;
};

class PalmData {
public:
    PalmData(HandData* owningHandData);
    glm::vec3 getPosition() const { return _owningHandData->leapPositionToWorldPosition(_rawPosition); }
    glm::vec3 getNormal() const { return _owningHandData->leapDirectionToWorldDirection(_rawNormal); }
    glm::vec3 getVelocity() const { return _owningHandData->leapDirectionToWorldDirection(_rawVelocity); }

    const glm::vec3& getRawPosition() const { return _rawPosition; }
    const glm::vec3& getRawNormal()   const { return _rawNormal; }
    bool isActive() const { return _isActive; }
    int getLeapID() const { return _leapID; }
    int getSixenseID() const { return _sixenseID; }


    std::vector<FingerData>& getFingers() { return _fingers; }
    const std::vector<FingerData>& getFingers() const { return _fingers; }
    size_t getNumFingers() const { return _fingers.size(); }

    void setActive(bool active) { _isActive = active; }
    void setLeapID(int id) { _leapID = id; }
    void setSixenseID(int id) { _sixenseID = id; }

    void setRawRotation(const glm::quat rawRotation) { _rawRotation = rawRotation; };
    glm::quat getRawRotation() const { return _rawRotation; }
    void setRawPosition(const glm::vec3& pos)  { _rawPosition = pos; }
    void setRawNormal(const glm::vec3& normal) { _rawNormal = normal; }
    void setRawVelocity(const glm::vec3& velocity) { _rawVelocity = velocity; }
    const glm::vec3& getRawVelocity()  const { return _rawVelocity; }
    void addToPosition(const glm::vec3& delta);
    
    void setTipPosition(const glm::vec3& position) { _tipPosition = position; }
    const glm::vec3 getTipPosition() const { return _owningHandData->leapPositionToWorldPosition(_tipPosition); }
    const glm::vec3& getTipRawPosition() const { return _tipPosition; }

    void setTipVelocity(const glm::vec3& velocity) { _tipVelocity = velocity; }
    const glm::vec3 getTipVelocity() const { return _owningHandData->leapDirectionToWorldDirection(_tipVelocity); }
    const glm::vec3& getTipRawVelocity() const { return _tipVelocity; }
    
    void incrementFramesWithoutData() { _numFramesWithoutData++; }
    void resetFramesWithoutData() { _numFramesWithoutData = 0; }
    int  getFramesWithoutData() const { return _numFramesWithoutData; }
    
    // Controller buttons
    void setControllerButtons(int controllerButtons) { _controllerButtons = controllerButtons; }
    void setLastControllerButtons(int controllerButtons) { _lastControllerButtons = controllerButtons; }

    int getControllerButtons() const { return _controllerButtons; }
    int getLastControllerButtons() const { return _lastControllerButtons; }
    
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

private:
    std::vector<FingerData> _fingers;
    glm::quat _rawRotation;
    glm::vec3 _rawPosition;
    glm::vec3 _rawNormal;
    glm::vec3 _rawVelocity;
    glm::vec3 _rotationalVelocity;
    glm::quat _lastRotation;
    
    glm::vec3 _tipPosition;
    glm::vec3 _tipVelocity;
    int _controllerButtons;
    int _lastControllerButtons;
    float _trigger;
    float _joystickX, _joystickY;
    
    bool      _isActive;             // This has current valid data
    int       _leapID;               // the Leap's serial id for this tracked object
    int       _sixenseID;            // Sixense controller ID for this palm
    int       _numFramesWithoutData; // after too many frames without data, this tracked object assumed lost.
    HandData* _owningHandData;
    
    bool      _isCollidingWithVoxel;  /// Whether the finger of this palm is inside a leaf voxel
    bool      _isCollidingWithPalm;
    quint64  _collisionlessPaddleExpiry; /// Timestamp after which paddle starts colliding
};

#endif /* defined(__hifi__HandData__) */
