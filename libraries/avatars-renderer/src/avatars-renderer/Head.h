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

#include <GLMHelpers.h>
#include <SharedUtil.h>
#include <HeadData.h>

const float EYE_EAR_GAP = 0.08f;

class Avatar;

class Head : public HeadData {
public:
    explicit Head(Avatar* owningAvatar);

    void init();
    void reset();
    virtual void simulate(float deltaTime);
    void setScale(float scale);
    void setPosition(const glm::vec3& position) { _position = position; }
    void setAverageLoudness(float averageLoudness) { _averageLoudness = averageLoudness; }
    void setReturnToCenter (bool returnHeadToCenter) { _returnHeadToCenter = returnHeadToCenter; }

    /// \return orientationBase+Delta
    glm::quat getFinalOrientationInLocalFrame() const;

    /// \return orientationBody * (orientationBase+Delta)
    glm::quat getFinalOrientationInWorldFrame() const;

    void setCorrectedLookAtPosition(const glm::vec3& correctedLookAtPosition);
    glm::vec3 getCorrectedLookAtPosition();
    void clearCorrectedLookAtPosition() { _isLookingAtMe = false; }
    bool isLookingAtMe();
    quint64 getLookingAtMeStarted() { return _lookingAtMeStarted; }

    float getScale() const { return _scale; }
    const glm::vec3& getPosition() const { return _position; }
    const glm::vec3& getEyePosition() const { return _eyePosition; }
    const glm::vec3& getSaccade() const { return _saccade; }
    glm::vec3 getRightDirection() const { return getOrientation() * IDENTITY_RIGHT; }
    glm::vec3 getUpDirection() const { return getOrientation() * IDENTITY_UP; }
    glm::vec3 getForwardDirection() const { return getOrientation() * IDENTITY_FORWARD; }

    glm::quat getEyeRotation(const glm::vec3& eyePosition) const;

    const glm::vec3& getRightEyePosition() const { return _rightEyePosition; }
    const glm::vec3& getLeftEyePosition() const { return _leftEyePosition; }
    glm::vec3 getRightEarPosition() const { return _rightEyePosition + (getRightDirection() * EYE_EAR_GAP) + (getForwardDirection() * -EYE_EAR_GAP); }
    glm::vec3 getLeftEarPosition() const { return _leftEyePosition + (getRightDirection() * -EYE_EAR_GAP) + (getForwardDirection() * -EYE_EAR_GAP); }
    glm::vec3 getMouthPosition() const { return _eyePosition - getUpDirection() * glm::length(_rightEyePosition - _leftEyePosition); }

    bool getReturnToCenter() const { return _returnHeadToCenter; } // Do you want head to try to return to center (depends on interface detected)
    float getAverageLoudness() const { return _averageLoudness; }

    void setDeltaPitch(float pitch) { _deltaPitch = pitch; }
    float getDeltaPitch() const { return _deltaPitch; }

    void setDeltaYaw(float yaw) { _deltaYaw = yaw; }
    float getDeltaYaw() const { return _deltaYaw; }

    void setDeltaRoll(float roll) { _deltaRoll = roll; }
    float getDeltaRoll() const { return _deltaRoll; }

    virtual float getFinalPitch() const override;
    virtual float getFinalYaw() const override;
    virtual float getFinalRoll() const override;

    void relax(float deltaTime);

    float getTimeWithoutTalking() const { return _timeWithoutTalking; }

    virtual void setLookAtPosition(const glm::vec3& lookAtPosition) override;
    void updateEyeLookAt();

protected:
    // disallow copies of the Head, copy of owning Avatar is disallowed too
    Head(const Head&);
    Head& operator= (const Head&);

    bool _returnHeadToCenter { false };
    glm::vec3 _position;
    glm::vec3 _rotation;
    glm::vec3 _leftEyePosition;
    glm::vec3 _rightEyePosition;
    glm::vec3 _eyePosition;

    float _scale { 1.0f };
    float _lastLoudness { 0.0f };
    float _longTermAverageLoudness { -1.0f };
    float _audioAttack { 0.0f };
    float _audioJawOpen { 0.0f };
    float _trailingAudioJawOpen { 0.0f };
    float _mouth2 { 0.0f };
    float _mouth3 { 0.0f };
    float _mouth4 { 0.0f };
    float _mouthTime { 0.0f };

    glm::vec3 _saccade;
    glm::vec3 _saccadeTarget;
    float _leftEyeBlinkVelocity { 0.0f };
    float _rightEyeBlinkVelocity { 0.0f };
    float _timeWithoutTalking { 0.0f };

    // delta angles for local head rotation (driven by hardware input)
    float _deltaPitch { 0.0f };
    float _deltaYaw { 0.0f };
    float _deltaRoll { 0.0f };

    bool _isCameraMoving { false };
    bool _isLookingAtMe { false };
    quint64 _lookingAtMeStarted { 0 };
    quint64 _wasLastLookingAtMe { 0 };

    glm::vec3 _correctedLookAtPosition;

    int _leftEyeLookAtID;
    int _rightEyeLookAtID;

    glm::vec3 _requestLookAtPosition;
    bool _forceBlinkToRetarget { false };
    bool _isEyeLookAtUpdated { false };

    // private methods
    void calculateMouthShapes(float timeRatio);
    void applyEyelidOffset(glm::quat headOrientation);
};

#endif // hifi_Head_h
