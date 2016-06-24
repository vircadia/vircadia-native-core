//
//  Head.h
//  interface/src/avatar
//
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Head_h
#define hifi_Head_h

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <SharedUtil.h>

#include <HeadData.h>

#include "world.h"


const float EYE_EAR_GAP = 0.08f;

class Avatar;

class Head : public HeadData {
public:
    explicit Head(Avatar* owningAvatar);

    void init();
    void reset();
    void simulate(float deltaTime, bool isMine, bool billboard = false);
    void setScale(float scale);
    void setPosition(glm::vec3 position) { _position = position; }
    void setAverageLoudness(float averageLoudness) { _averageLoudness = averageLoudness; }
    void setReturnToCenter (bool returnHeadToCenter) { _returnHeadToCenter = returnHeadToCenter; }

    /// \return orientationBase+Delta
    glm::quat getFinalOrientationInLocalFrame() const;

    /// \return orientationBody * (orientationBase+Delta)
    glm::quat getFinalOrientationInWorldFrame() const;

    /// \return orientationBody * orientationBasePitch
    glm::quat getCameraOrientation () const;

    void setCorrectedLookAtPosition(glm::vec3 correctedLookAtPosition);
    glm::vec3 getCorrectedLookAtPosition();
    void clearCorrectedLookAtPosition() { _isLookingAtMe = false; }
    bool isLookingAtMe();
    quint64 getLookingAtMeStarted() { return _lookingAtMeStarted; }

    float getScale() const { return _scale; }
    glm::vec3 getPosition() const { return _position; }
    const glm::vec3& getEyePosition() const { return _eyePosition; }
    const glm::vec3& getSaccade() const { return _saccade; }
    glm::vec3 getRightDirection() const { return getOrientation() * IDENTITY_RIGHT; }
    glm::vec3 getUpDirection() const { return getOrientation() * IDENTITY_UP; }
    glm::vec3 getFrontDirection() const { return getOrientation() * IDENTITY_FRONT; }

    glm::quat getEyeRotation(const glm::vec3& eyePosition) const;

    const glm::vec3& getRightEyePosition() const { return _rightEyePosition; }
    const glm::vec3& getLeftEyePosition() const { return _leftEyePosition; }
    glm::vec3 getRightEarPosition() const { return _rightEyePosition + (getRightDirection() * EYE_EAR_GAP) + (getFrontDirection() * -EYE_EAR_GAP); }
    glm::vec3 getLeftEarPosition() const { return _leftEyePosition + (getRightDirection() * -EYE_EAR_GAP) + (getFrontDirection() * -EYE_EAR_GAP); }
    glm::vec3 getMouthPosition() const { return _eyePosition - getUpDirection() * glm::length(_rightEyePosition - _leftEyePosition); }

    bool getReturnToCenter() const { return _returnHeadToCenter; } // Do you want head to try to return to center (depends on interface detected)
    float getAverageLoudness() const { return _averageLoudness; }
    /// \return the point about which scaling occurs.
    glm::vec3 getScalePivot() const;

    void setDeltaPitch(float pitch) { _deltaPitch = pitch; }
    float getDeltaPitch() const { return _deltaPitch; }

    void setDeltaYaw(float yaw) { _deltaYaw = yaw; }
    float getDeltaYaw() const { return _deltaYaw; }

    void setDeltaRoll(float roll) { _deltaRoll = roll; }
    float getDeltaRoll() const { return _deltaRoll; }

    virtual void setFinalYaw(float finalYaw);
    virtual void setFinalPitch(float finalPitch);
    virtual void setFinalRoll(float finalRoll);
    virtual float getFinalPitch() const;
    virtual float getFinalYaw() const;
    virtual float getFinalRoll() const;

    void relax(float deltaTime);

    float getTimeWithoutTalking() const { return _timeWithoutTalking; }

private:
    glm::vec3 calculateAverageEyePosition() const { return _leftEyePosition + (_rightEyePosition - _leftEyePosition ) * 0.5f; }

    // disallow copies of the Head, copy of owning Avatar is disallowed too
    Head(const Head&);
    Head& operator= (const Head&);

    bool _returnHeadToCenter;
    glm::vec3 _position;
    glm::vec3 _rotation;
    glm::vec3 _leftEyePosition;
    glm::vec3 _rightEyePosition;
    glm::vec3 _eyePosition;

    float _scale;
    float _lastLoudness;
    float _longTermAverageLoudness;
    float _audioAttack;
    float _audioJawOpen;
    float _trailingAudioJawOpen;
    float _mouth2;
    float _mouth3;
    float _mouth4;
    float _mouthTime;

    glm::vec3 _saccade;
    glm::vec3 _saccadeTarget;
    float _leftEyeBlinkVelocity;
    float _rightEyeBlinkVelocity;
    float _timeWithoutTalking;

    // delta angles for local head rotation (driven by hardware input)
    float _deltaPitch;
    float _deltaYaw;
    float _deltaRoll;

    bool _isCameraMoving;
    bool _isLookingAtMe;
    quint64 _lookingAtMeStarted;
    quint64 _wasLastLookingAtMe;

    glm::vec3 _correctedLookAtPosition;

    int _leftEyeLookAtID;
    int _rightEyeLookAtID;

    // private methods
    void calculateMouthShapes();
    void applyEyelidOffset(glm::quat headOrientation);
};

#endif // hifi_Head_h
