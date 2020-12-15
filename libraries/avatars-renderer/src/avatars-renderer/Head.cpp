//
//  Head.cpp
//  interface/src/avatar
//
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Head.h"

#include <glm/gtx/quaternion.hpp>
#include <gpu/Batch.h>

#include <NodeList.h>
#include <DependencyManager.h>
#include <GeometryUtil.h>
#include <Rig.h>
#include "Logging.h"

#include "Avatar.h"

using namespace std;

static bool disableEyelidAdjustment { false };

static void updateFakeCoefficients(float leftBlink, float rightBlink, float browUp,
    float jawOpen, float mouth2, float mouth3, float mouth4, QVector<float>& coefficients) {

    coefficients.resize(std::max((int)coefficients.size(), (int)Blendshapes::BlendshapeCount));
    qFill(coefficients.begin(), coefficients.end(), 0.0f);
    coefficients[(int)Blendshapes::EyeBlink_L] = leftBlink;
    coefficients[(int)Blendshapes::EyeBlink_R] = rightBlink;
    coefficients[(int)Blendshapes::BrowsU_C] = browUp;
    coefficients[(int)Blendshapes::BrowsU_L] = browUp;
    coefficients[(int)Blendshapes::BrowsU_R] = browUp;
    coefficients[(int)Blendshapes::JawOpen] = jawOpen;
    coefficients[(int)Blendshapes::MouthSmile_L] = coefficients[(int)Blendshapes::MouthSmile_R] = mouth4;
    coefficients[(int)Blendshapes::LipsUpperClose] = mouth2;
    coefficients[(int)Blendshapes::LipsFunnel] = mouth3;
}

Head::Head(Avatar* owningAvatar) :
    HeadData(owningAvatar),
    _leftEyeLookAtID(DependencyManager::get<GeometryCache>()->allocateID()),
    _rightEyeLookAtID(DependencyManager::get<GeometryCache>()->allocateID())
{
}

void Head::init() {
}

void Head::reset() {
    _baseYaw = _basePitch = _baseRoll = 0.0f;
}

void Head::simulate(float deltaTime) {
    const float NORMAL_HZ = 60.0f; // the update rate the constant values were tuned for

    // grab the audio loudness from the owning avatar, if we have one
    float audioLoudness = _owningAvatar ? _owningAvatar->getAudioLoudness() : 0.0f;

    // Update audio trailing average for rendering facial animations
    const float AUDIO_AVERAGING_SECS = 0.05f;
    const float AUDIO_LONG_TERM_AVERAGING_SECS = 15.0f;
    _averageLoudness = glm::mix(_averageLoudness, audioLoudness, glm::min(deltaTime / AUDIO_AVERAGING_SECS, 1.0f));

    if (_longTermAverageLoudness == -1.0f) {
        _longTermAverageLoudness = _averageLoudness;
    } else {
        _longTermAverageLoudness = glm::mix(_longTermAverageLoudness, _averageLoudness, glm::min(deltaTime / AUDIO_LONG_TERM_AVERAGING_SECS, 1.0f));
    }

    if (getProceduralAnimationFlag(HeadData::SaccadeProceduralEyeJointAnimation) &&
        !getSuppressProceduralAnimationFlag(HeadData::SaccadeProceduralEyeJointAnimation)) {
        // Update eye saccades
        const float AVERAGE_MICROSACCADE_INTERVAL = 1.0f;
        const float AVERAGE_SACCADE_INTERVAL = 6.0f;
        const float MICROSACCADE_MAGNITUDE = 0.002f;
        const float SACCADE_MAGNITUDE = 0.04f;
        const float NOMINAL_FRAME_RATE = 60.0f;

        if (randFloat() < deltaTime / AVERAGE_MICROSACCADE_INTERVAL) {
            _saccadeTarget = MICROSACCADE_MAGNITUDE * randVector();
        } else if (randFloat() < deltaTime / AVERAGE_SACCADE_INTERVAL) {
            _saccadeTarget = SACCADE_MAGNITUDE * randVector();
        }
        _saccade += (_saccadeTarget - _saccade) * pow(0.5f, NOMINAL_FRAME_RATE * deltaTime);
    } else {
        _saccade = glm::vec3();
    }

    const float BLINK_SPEED = 10.0f;
    const float BLINK_SPEED_VARIABILITY = 1.0f;
    const float BLINK_START_VARIABILITY = 0.25f;
    const float FULLY_OPEN = 0.0f;
    const float FULLY_CLOSED = 1.0f;
    const float TALKING_LOUDNESS = 150.0f;

    _timeWithoutTalking += deltaTime;
    if ((_averageLoudness - _longTermAverageLoudness) > TALKING_LOUDNESS) {
        _timeWithoutTalking = 0.0f;
    }

    if (getProceduralAnimationFlag(HeadData::BlinkProceduralBlendshapeAnimation) &&
        !getSuppressProceduralAnimationFlag(HeadData::BlinkProceduralBlendshapeAnimation)) {

        // handle automatic blinks
        // Detect transition from talking to not; force blink after that and a delay
        bool forceBlink = false;
        const float BLINK_AFTER_TALKING = 0.25f;
        if (_timeWithoutTalking - deltaTime < BLINK_AFTER_TALKING && _timeWithoutTalking >= BLINK_AFTER_TALKING) {
            forceBlink = true;
        }
        if (_leftEyeBlinkVelocity == 0.0f && _rightEyeBlinkVelocity == 0.0f) {
            // no blinking when brows are raised; blink less with increasing loudness
            const float BASE_BLINK_RATE = 15.0f / 60.0f;
            const float ROOT_LOUDNESS_TO_BLINK_INTERVAL = 0.25f;
            if (_forceBlinkToRetarget || forceBlink ||
                (_browAudioLift < EPSILON && shouldDo(glm::max(1.0f, sqrt(fabs(_averageLoudness - _longTermAverageLoudness)) *
                ROOT_LOUDNESS_TO_BLINK_INTERVAL) / BASE_BLINK_RATE, deltaTime))) {
                float randSpeedVariability = randFloat();
                float eyeBlinkVelocity = BLINK_SPEED + randSpeedVariability * BLINK_SPEED_VARIABILITY;
                if (_forceBlinkToRetarget) {
                    // Slow down by half the blink if reseting eye target
                    eyeBlinkVelocity = 0.5f * eyeBlinkVelocity;
                    _forceBlinkToRetarget = false;
                }
                _leftEyeBlinkVelocity = eyeBlinkVelocity;
                _rightEyeBlinkVelocity = eyeBlinkVelocity;
                if (randFloat() < 0.5f) {
                    _leftEyeBlink = BLINK_START_VARIABILITY;
                    _rightEyeBlink = BLINK_START_VARIABILITY;
                }
            }
        } else {

            _leftEyeBlink = glm::clamp(_leftEyeBlink + _leftEyeBlinkVelocity * deltaTime, FULLY_OPEN, FULLY_CLOSED);
            _rightEyeBlink = glm::clamp(_rightEyeBlink + _rightEyeBlinkVelocity * deltaTime, FULLY_OPEN, FULLY_CLOSED);

            if (_leftEyeBlink == FULLY_CLOSED) {
                _leftEyeBlinkVelocity = -BLINK_SPEED;
                updateEyeLookAt();
            } else if (_leftEyeBlink == FULLY_OPEN) {
                _leftEyeBlinkVelocity = 0.0f;
            }
            if (_rightEyeBlink == FULLY_CLOSED) {
                _rightEyeBlinkVelocity = -BLINK_SPEED;
            } else if (_rightEyeBlink == FULLY_OPEN) {
                _rightEyeBlinkVelocity = 0.0f;
            }
        }
    } else {
        _rightEyeBlink = FULLY_OPEN;
        _leftEyeBlink = FULLY_OPEN;
        updateEyeLookAt();
    }

    // Use data to update fake blendshape coefficients.
    if (getProceduralAnimationFlag(HeadData::AudioProceduralBlendshapeAnimation) &&
        !getSuppressProceduralAnimationFlag(HeadData::AudioProceduralBlendshapeAnimation)) {

        // Update audio attack data for facial animation (eyebrows and mouth)
        float audioAttackAveragingRate = (10.0f - deltaTime * NORMAL_HZ) / 10.0f; // --> 0.9 at 60 Hz
        _audioAttack = audioAttackAveragingRate * _audioAttack +
            (1.0f - audioAttackAveragingRate) * fabs((audioLoudness - _longTermAverageLoudness) - _lastLoudness);
        _lastLoudness = (audioLoudness - _longTermAverageLoudness);
        const float BROW_LIFT_THRESHOLD = 100.0f;
        if (_audioAttack > BROW_LIFT_THRESHOLD) {
            _browAudioLift += sqrtf(_audioAttack) * 0.01f;
        }
        _browAudioLift = glm::clamp(_browAudioLift *= 0.7f, 0.0f, 1.0f);
        calculateMouthShapes(deltaTime);

    } else {
        _audioJawOpen = 0.0f;
        _browAudioLift = 0.0f;
        _mouth2 = 0.0f;
        _mouth3 = 0.0f;
        _mouth4 = 0.0f;
        _mouthTime = 0.0f;
    }

    updateFakeCoefficients(
        _leftEyeBlink,
        _rightEyeBlink,
        _browAudioLift,
        _audioJawOpen,
        _mouth2,
        _mouth3,
        _mouth4,
        _transientBlendshapeCoefficients);

    if (getProceduralAnimationFlag(HeadData::LidAdjustmentProceduralBlendshapeAnimation) &&
        !getSuppressProceduralAnimationFlag(HeadData::LidAdjustmentProceduralBlendshapeAnimation)) {

        // This controls two things, the eye brow and the upper eye lid, it is driven by the vertical up/down angle of the
        // eyes relative to the head.  This is to try to help prevent sleepy eyes/crazy eyes.
        applyEyelidOffset(getOrientation());
    }

    _leftEyePosition = _rightEyePosition = getPosition();
    if (_owningAvatar) {
        auto skeletonModel = static_cast<Avatar*>(_owningAvatar)->getSkeletonModel();
        if (skeletonModel) {
            skeletonModel->getEyePositions(_leftEyePosition, _rightEyePosition);
        }
    }
    _eyePosition = 0.5f * (_leftEyePosition + _rightEyePosition);
}

void Head::calculateMouthShapes(float deltaTime) {
    const float JAW_OPEN_SCALE = 0.35f;
    const float JAW_OPEN_RATE = 0.9f;
    const float JAW_CLOSE_RATE = 0.90f;
    const float TIMESTEP_CONSTANT = 0.0032f;
    const float MMMM_POWER = 0.10f;
    const float SMILE_POWER = 0.10f;
    const float FUNNEL_POWER = 0.35f;
    const float MMMM_SPEED = 2.685f;
    const float SMILE_SPEED = 1.0f;
    const float FUNNEL_SPEED = 2.335f;
    const float STOP_GAIN = 5.0f;
    const float NORMAL_HZ = 60.0f; // the update rate the constant values were tuned for
    const float MAX_DELTA_LOUDNESS = 100.0f;

    float deltaTimeRatio = deltaTime / (1.0f / NORMAL_HZ);

    // From the change in loudness, decide how much to open or close the jaw
    float deltaLoudness = glm::max(glm::min(_averageLoudness - _longTermAverageLoudness, MAX_DELTA_LOUDNESS), 0.0f) / MAX_DELTA_LOUDNESS;
    float audioDelta = powf(deltaLoudness, 2.0f) * JAW_OPEN_SCALE;
    if (audioDelta > _audioJawOpen) {
        _audioJawOpen += (audioDelta - _audioJawOpen) * JAW_OPEN_RATE * deltaTimeRatio;
    } else {
        _audioJawOpen *= powf(JAW_CLOSE_RATE, deltaTimeRatio);
    }
    _audioJawOpen = glm::clamp(_audioJawOpen, 0.0f, 1.0f);
    float trailingAudioJawOpenRatio = (100.0f - deltaTime * NORMAL_HZ) / 100.0f; // --> 0.99 at 60 Hz
    _trailingAudioJawOpen = glm::mix(_trailingAudioJawOpen, _audioJawOpen, trailingAudioJawOpenRatio);

    // truncate _mouthTime when mouth goes quiet to prevent floating point error on increment
    const float SILENT_TRAILING_JAW_OPEN = 0.0002f;
    const float MAX_SILENT_MOUTH_TIME = 10.0f;
    if (_trailingAudioJawOpen < SILENT_TRAILING_JAW_OPEN && _mouthTime > MAX_SILENT_MOUTH_TIME) {
        _mouthTime = 0.0f;
    }

    // Advance time at a rate proportional to loudness, and move the mouth shapes through
    // a cycle at differing speeds to create a continuous random blend of shapes.
    _mouthTime += sqrtf(_averageLoudness) * TIMESTEP_CONSTANT * deltaTimeRatio;
    _mouth2 = (sinf(_mouthTime * MMMM_SPEED) + 1.0f) * MMMM_POWER * glm::min(1.0f, _trailingAudioJawOpen * STOP_GAIN);
    _mouth3 = (sinf(_mouthTime * FUNNEL_SPEED) + 1.0f) * FUNNEL_POWER * glm::min(1.0f, _trailingAudioJawOpen * STOP_GAIN);
    _mouth4 = (sinf(_mouthTime * SMILE_SPEED) + 1.0f) * SMILE_POWER * glm::min(1.0f, _trailingAudioJawOpen * STOP_GAIN);
}

void Head::applyEyelidOffset(glm::quat headOrientation) {
    // Adjusts the eyelid blendshape coefficients so that the eyelid follows the iris as the head pitches.
    bool isBlinking = (_rightEyeBlinkVelocity != 0.0f && _rightEyeBlinkVelocity != 0.0f);
    if (disableEyelidAdjustment || isBlinking) {
        return;
    }

    const float EYE_PITCH_TO_COEFFICIENT = 3.5f;  // Empirically determined
    const float MAX_EYELID_OFFSET = 1.5f;
    const float BLINK_DOWN_MULTIPLIER = 0.25f;
    const float OPEN_DOWN_MULTIPLIER = 0.3f;
    const float BROW_UP_MULTIPLIER = 0.5f;

    glm::vec3 lookAtVector = getLookAtPosition() - _eyePosition;
    if (glm::length2(lookAtVector) == 0.0f) {
        return;
    }
    glm::vec3 lookAt = glm::normalize(lookAtVector);
    glm::vec3 headUp = headOrientation * Vectors::UNIT_Y;
    float eyePitch = (PI / 2.0f) - acos(glm::dot(lookAt, headUp));
    float eyelidOffset = glm::clamp(abs(eyePitch * EYE_PITCH_TO_COEFFICIENT), 0.0f, MAX_EYELID_OFFSET);

    float blinkUpCoefficient = -eyelidOffset;
    float blinkDownCoefficient = BLINK_DOWN_MULTIPLIER * eyelidOffset;

    float openUpCoefficient = eyelidOffset;
    float openDownCoefficient = OPEN_DOWN_MULTIPLIER * eyelidOffset;

    float browsUpCoefficient = BROW_UP_MULTIPLIER * eyelidOffset;
    float browsDownCoefficient = 0.0f;

    bool isLookingUp = (eyePitch > 0);

    if (isLookingUp) {
        for (int i = 0; i < 2; i++) {
            _transientBlendshapeCoefficients[(int)Blendshapes::EyeBlink_L + i] = blinkUpCoefficient;
            _transientBlendshapeCoefficients[(int)Blendshapes::EyeOpen_L + i] = openUpCoefficient;
            _transientBlendshapeCoefficients[(int)Blendshapes::BrowsU_L + i] = browsUpCoefficient;
        }
    } else {
        for (int i = 0; i < 2; i++) {
            _transientBlendshapeCoefficients[(int)Blendshapes::EyeBlink_L + i] = blinkDownCoefficient;
            _transientBlendshapeCoefficients[(int)Blendshapes::EyeOpen_L + i] = openDownCoefficient;
            _transientBlendshapeCoefficients[(int)Blendshapes::BrowsU_L + i] = browsDownCoefficient;
        }
    }
}

void Head::relax(float deltaTime) {
    // restore rotation, lean to neutral positions
    const float LEAN_RELAXATION_PERIOD = 0.25f;   // seconds
    float relaxationFactor = 1.0f - glm::min(deltaTime / LEAN_RELAXATION_PERIOD, 1.0f);
    _deltaYaw *= relaxationFactor;
    _deltaPitch *= relaxationFactor;
    _deltaRoll *= relaxationFactor;
}

void Head::setScale (float scale) {
    if (_scale == scale) {
        return;
    }
    _scale = scale;
}

glm::quat Head::getFinalOrientationInWorldFrame() const {
    return _owningAvatar->getWorldOrientation() * getFinalOrientationInLocalFrame();
}

glm::quat Head::getFinalOrientationInLocalFrame() const {
    return glm::quat(glm::radians(glm::vec3(getFinalPitch(), getFinalYaw(), getFinalRoll() )));
}

// Everyone else's head keeps track of a lookAtPosition that everybody sees the same, and refers to where that head
// is looking in model space -- e.g., at someone's eyeball, or between their eyes, or mouth, etc. Everyone's Interface
// will have the same value for the lookAtPosition of any given head.
//
// Everyone else's head also keeps track of a correctedLookAtPosition that may be different for the same head within
// different Interfaces. If that head is not looking at me, the correctedLookAtPosition is the same as the lookAtPosition.
// However, if that head is looking at me, then I will attempt to adjust the lookAtPosition by the difference between
// my (singular) eye position and my actual camera position. This adjustment is used on their eyeballs during rendering
// (and also on any lookAt vector display for that head, during rendering). Note that:
// 1. this adjustment can be made directly to the other head's eyeball joints, because we won't be send their joint information to others.
// 2. the corrected position is a separate ivar, so the common/uncorrected value is still available
//
// There is a pun here: The two lookAtPositions will always be the same for my own avatar in my own Interface, because I
// will not be looking at myself. (Even in a mirror, I will be looking at the camera.)
glm::vec3 Head::getCorrectedLookAtPosition() {
    if (isLookingAtMe()) {
        return _correctedLookAtPosition;
    } else {
        return getLookAtPosition();
    }
}

void Head::setCorrectedLookAtPosition(const glm::vec3& correctedLookAtPosition) {
    if (!isLookingAtMe()) {
        _lookingAtMeStarted = usecTimestampNow();
    }
    _isLookingAtMe = true;
    _wasLastLookingAtMe = usecTimestampNow();
    _correctedLookAtPosition = correctedLookAtPosition;
}

bool Head::isLookingAtMe() {
    // Allow for outages such as may be encountered during avatar movement
    quint64 now = usecTimestampNow();
    const quint64 LOOKING_AT_ME_GAP_ALLOWED = (5 * 1000 * 1000) / 60; // n frames, in microseconds
    return _isLookingAtMe || (now - _wasLastLookingAtMe) < LOOKING_AT_ME_GAP_ALLOWED;
}

glm::quat Head::getEyeRotation(const glm::vec3& eyePosition) const {
    glm::quat orientation = getOrientation();
    glm::vec3 lookAtDelta = _lookAtPosition - eyePosition;
    return rotationBetween(orientation * IDENTITY_FORWARD, lookAtDelta + glm::length(lookAtDelta) * _saccade) * orientation;
}

float Head::getFinalYaw() const {
    return glm::clamp(_baseYaw + _deltaYaw, MIN_HEAD_YAW, MAX_HEAD_YAW);
}

float Head::getFinalPitch() const {
    return glm::clamp(_basePitch + _deltaPitch, MIN_HEAD_PITCH, MAX_HEAD_PITCH);
}

float Head::getFinalRoll() const {
    return glm::clamp(_baseRoll + _deltaRoll, MIN_HEAD_ROLL, MAX_HEAD_ROLL);
}

void Head::setLookAtPosition(const glm::vec3& lookAtPosition) {
    if (_isEyeLookAtUpdated && _requestLookAtPosition != lookAtPosition) {
        glm::vec3 oldAvatarLookAtVector = _requestLookAtPosition - _owningAvatar->getWorldPosition();
        glm::vec3 newAvatarLookAtVector = lookAtPosition - _owningAvatar->getWorldPosition();
        const float MIN_BLINK_ANGLE = 0.35f; // 20 degrees
        _forceBlinkToRetarget = angleBetween(oldAvatarLookAtVector, newAvatarLookAtVector) > MIN_BLINK_ANGLE;
        if (_forceBlinkToRetarget) {
            _isEyeLookAtUpdated = false;
        } else {
            _lookAtPosition = lookAtPosition;
        }
    }
    _lookAtPositionChanged = usecTimestampNow();
    _requestLookAtPosition = lookAtPosition;
}

void Head::updateEyeLookAt() {
    _lookAtPosition = _requestLookAtPosition;
    _isEyeLookAtUpdated = true;
}
