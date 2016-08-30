//
//  Head.cpp
//  interface/src/avatar
//
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/quaternion.hpp>
#include <gpu/Batch.h>

#include <NodeList.h>
#include <recording/Deck.h>

#include "Application.h"
#include "Avatar.h"
#include "DependencyManager.h"
#include "GeometryUtil.h"
#include "Head.h"
#include "Menu.h"
#include "Util.h"
#include "devices/DdeFaceTracker.h"
#include "devices/EyeTracker.h"
#include "devices/Faceshift.h"
#include <Rig.h>

using namespace std;

Head::Head(Avatar* owningAvatar) :
    HeadData((AvatarData*)owningAvatar),
    _returnHeadToCenter(false),
    _position(0.0f, 0.0f, 0.0f),
    _rotation(0.0f, 0.0f, 0.0f),
    _leftEyePosition(0.0f, 0.0f, 0.0f),
    _rightEyePosition(0.0f, 0.0f, 0.0f),
    _eyePosition(0.0f, 0.0f, 0.0f),
    _scale(1.0f),
    _lastLoudness(0.0f),
    _longTermAverageLoudness(-1.0f),
    _audioAttack(0.0f),
    _audioJawOpen(0.0f),
    _trailingAudioJawOpen(0.0f),
    _mouth2(0.0f),
    _mouth3(0.0f),
    _mouth4(0.0f),
    _mouthTime(0.0f),
    _saccade(0.0f, 0.0f, 0.0f),
    _saccadeTarget(0.0f, 0.0f, 0.0f),
    _leftEyeBlinkVelocity(0.0f),
    _rightEyeBlinkVelocity(0.0f),
    _timeWithoutTalking(0.0f),
    _deltaPitch(0.0f),
    _deltaYaw(0.0f),
    _deltaRoll(0.0f),
    _isCameraMoving(false),
    _isLookingAtMe(false),
    _lookingAtMeStarted(0),
    _wasLastLookingAtMe(0),
    _leftEyeLookAtID(DependencyManager::get<GeometryCache>()->allocateID()),
    _rightEyeLookAtID(DependencyManager::get<GeometryCache>()->allocateID())
{
}

void Head::init() {
}

void Head::reset() {
    _baseYaw = _basePitch = _baseRoll = 0.0f;
}

void Head::simulate(float deltaTime, bool isMine, bool billboard) {
    //  Update audio trailing average for rendering facial animations
    const float AUDIO_AVERAGING_SECS = 0.05f;
    const float AUDIO_LONG_TERM_AVERAGING_SECS = 30.0f;
    _averageLoudness = glm::mix(_averageLoudness, _audioLoudness, glm::min(deltaTime / AUDIO_AVERAGING_SECS, 1.0f));

    if (_longTermAverageLoudness == -1.0f) {
        _longTermAverageLoudness = _averageLoudness;
    } else {
        _longTermAverageLoudness = glm::mix(_longTermAverageLoudness, _averageLoudness, glm::min(deltaTime / AUDIO_LONG_TERM_AVERAGING_SECS, 1.0f));
    }

    if (isMine) {
        auto player = DependencyManager::get<recording::Deck>();
        // Only use face trackers when not playing back a recording.
        if (!player->isPlaying()) {
            FaceTracker* faceTracker = qApp->getActiveFaceTracker();
            _isFaceTrackerConnected = faceTracker != NULL && !faceTracker->isMuted();
            if (_isFaceTrackerConnected) {
                _blendshapeCoefficients = faceTracker->getBlendshapeCoefficients();

                if (typeid(*faceTracker) == typeid(DdeFaceTracker)) {

                    if (Menu::getInstance()->isOptionChecked(MenuOption::UseAudioForMouth)) {
                        calculateMouthShapes();

                        const int JAW_OPEN_BLENDSHAPE = 21;
                        const int MMMM_BLENDSHAPE = 34;
                        const int FUNNEL_BLENDSHAPE = 40;
                        const int SMILE_LEFT_BLENDSHAPE = 28;
                        const int SMILE_RIGHT_BLENDSHAPE = 29;
                        _blendshapeCoefficients[JAW_OPEN_BLENDSHAPE] += _audioJawOpen;
                        _blendshapeCoefficients[SMILE_LEFT_BLENDSHAPE] += _mouth4;
                        _blendshapeCoefficients[SMILE_RIGHT_BLENDSHAPE] += _mouth4;
                        _blendshapeCoefficients[MMMM_BLENDSHAPE] += _mouth2;
                        _blendshapeCoefficients[FUNNEL_BLENDSHAPE] += _mouth3;
                    }

                    applyEyelidOffset(getFinalOrientationInWorldFrame());
                }
            }

            auto eyeTracker = DependencyManager::get<EyeTracker>();
            _isEyeTrackerConnected = eyeTracker->isTracking();
        }
    }
   
    if (!(_isFaceTrackerConnected || billboard)) {

        if (!_isEyeTrackerConnected) {
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

        //  Detect transition from talking to not; force blink after that and a delay
        bool forceBlink = false;
        const float TALKING_LOUDNESS = 100.0f;
        const float BLINK_AFTER_TALKING = 0.25f;
        _timeWithoutTalking += deltaTime;
        if ((_averageLoudness - _longTermAverageLoudness) > TALKING_LOUDNESS) {
            _timeWithoutTalking = 0.0f;
        
        } else if (_timeWithoutTalking < BLINK_AFTER_TALKING && _timeWithoutTalking >= BLINK_AFTER_TALKING) {
            forceBlink = true;
        }
                                 
        //  Update audio attack data for facial animation (eyebrows and mouth)
        const float AUDIO_ATTACK_AVERAGING_RATE = 0.9f;
        _audioAttack = AUDIO_ATTACK_AVERAGING_RATE * _audioAttack + (1.0f - AUDIO_ATTACK_AVERAGING_RATE) * fabs((_audioLoudness - _longTermAverageLoudness) - _lastLoudness);
        _lastLoudness = (_audioLoudness - _longTermAverageLoudness);
        
        const float BROW_LIFT_THRESHOLD = 100.0f;
        if (_audioAttack > BROW_LIFT_THRESHOLD) {
            _browAudioLift += sqrtf(_audioAttack) * 0.01f;
        }
        _browAudioLift = glm::clamp(_browAudioLift *= 0.7f, 0.0f, 1.0f);
        
        const float BLINK_SPEED = 10.0f;
        const float BLINK_SPEED_VARIABILITY = 1.0f;
        const float BLINK_START_VARIABILITY = 0.25f;
        const float FULLY_OPEN = 0.0f;
        const float FULLY_CLOSED = 1.0f;
        if (_leftEyeBlinkVelocity == 0.0f && _rightEyeBlinkVelocity == 0.0f) {
            // no blinking when brows are raised; blink less with increasing loudness
            const float BASE_BLINK_RATE = 15.0f / 60.0f;
            const float ROOT_LOUDNESS_TO_BLINK_INTERVAL = 0.25f;
            if (forceBlink || (_browAudioLift < EPSILON && shouldDo(glm::max(1.0f, sqrt(fabs(_averageLoudness - _longTermAverageLoudness)) *
                    ROOT_LOUDNESS_TO_BLINK_INTERVAL) / BASE_BLINK_RATE, deltaTime))) {
                _leftEyeBlinkVelocity = BLINK_SPEED + randFloat() * BLINK_SPEED_VARIABILITY;
                _rightEyeBlinkVelocity = BLINK_SPEED + randFloat() * BLINK_SPEED_VARIABILITY;
                if (randFloat() < 0.5f) {
                    _leftEyeBlink = BLINK_START_VARIABILITY;
                } else {
                    _rightEyeBlink = BLINK_START_VARIABILITY;
                }
            }
        } else {
            _leftEyeBlink = glm::clamp(_leftEyeBlink + _leftEyeBlinkVelocity * deltaTime, FULLY_OPEN, FULLY_CLOSED);
            _rightEyeBlink = glm::clamp(_rightEyeBlink + _rightEyeBlinkVelocity * deltaTime, FULLY_OPEN, FULLY_CLOSED);
            
            if (_leftEyeBlink == FULLY_CLOSED) {
                _leftEyeBlinkVelocity = -BLINK_SPEED;
            
            } else if (_leftEyeBlink == FULLY_OPEN) {
                _leftEyeBlinkVelocity = 0.0f;
            }
            if (_rightEyeBlink == FULLY_CLOSED) {
                _rightEyeBlinkVelocity = -BLINK_SPEED;
            
            } else if (_rightEyeBlink == FULLY_OPEN) {
                _rightEyeBlinkVelocity = 0.0f;
            }
        }
        
        // use data to update fake Faceshift blendshape coefficients
        calculateMouthShapes();
        DependencyManager::get<Faceshift>()->updateFakeCoefficients(_leftEyeBlink,
                                                                    _rightEyeBlink,
                                                                    _browAudioLift,
                                                                    _audioJawOpen,
                                                                    _mouth2,
                                                                    _mouth3,
                                                                    _mouth4,
                                                                    _blendshapeCoefficients);

        applyEyelidOffset(getOrientation());

    } else {
        _saccade = glm::vec3();
    }
    if (Menu::getInstance()->isOptionChecked(MenuOption::FixGaze)) { // if debug menu turns off, use no saccade
        _saccade = glm::vec3();
    }
    
    _leftEyePosition = _rightEyePosition = getPosition();
    _eyePosition = getPosition();

    if (!billboard && _owningAvatar) {
        auto skeletonModel = static_cast<Avatar*>(_owningAvatar)->getSkeletonModel();
        if (skeletonModel) {
            skeletonModel->getEyePositions(_leftEyePosition, _rightEyePosition);
        }
    }

    _eyePosition = calculateAverageEyePosition();
}

void Head::calculateMouthShapes() {
    const float JAW_OPEN_SCALE = 0.015f;
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

    // From the change in loudness, decide how much to open or close the jaw
    float audioDelta = sqrtf(glm::max(_averageLoudness - _longTermAverageLoudness, 0.0f)) * JAW_OPEN_SCALE;
    if (audioDelta > _audioJawOpen) {
        _audioJawOpen += (audioDelta - _audioJawOpen) * JAW_OPEN_RATE;
    } else {
        _audioJawOpen *= JAW_CLOSE_RATE;
    }
    _audioJawOpen = glm::clamp(_audioJawOpen, 0.0f, 1.0f);
    _trailingAudioJawOpen = glm::mix(_trailingAudioJawOpen, _audioJawOpen, 0.99f); 

    // Advance time at a rate proportional to loudness, and move the mouth shapes through 
    // a cycle at differing speeds to create a continuous random blend of shapes.
    _mouthTime += sqrtf(_averageLoudness) * TIMESTEP_CONSTANT;
    _mouth2 = (sinf(_mouthTime * MMMM_SPEED) + 1.0f) * MMMM_POWER * glm::min(1.0f, _trailingAudioJawOpen * STOP_GAIN);
    _mouth3 = (sinf(_mouthTime * FUNNEL_SPEED) + 1.0f) * FUNNEL_POWER * glm::min(1.0f, _trailingAudioJawOpen * STOP_GAIN);
    _mouth4 = (sinf(_mouthTime * SMILE_SPEED) + 1.0f) * SMILE_POWER * glm::min(1.0f, _trailingAudioJawOpen * STOP_GAIN);
}

void Head::applyEyelidOffset(glm::quat headOrientation) {
    // Adjusts the eyelid blendshape coefficients so that the eyelid follows the iris as the head pitches.

    if (Menu::getInstance()->isOptionChecked(MenuOption::DisableEyelidAdjustment)) {
        return;
    }

    glm::quat eyeRotation = rotationBetween(headOrientation * IDENTITY_FRONT, getLookAtPosition() - _eyePosition);
    eyeRotation = eyeRotation * glm::angleAxis(safeEulerAngles(headOrientation).y, IDENTITY_UP);  // Rotation w.r.t. head
    float eyePitch = safeEulerAngles(eyeRotation).x;

    const float EYE_PITCH_TO_COEFFICIENT = 1.6f;  // Empirically determined
    const float MAX_EYELID_OFFSET = 0.8f;  // So that don't fully close eyes when looking way down
    float eyelidOffset = glm::clamp(-eyePitch * EYE_PITCH_TO_COEFFICIENT, -1.0f, MAX_EYELID_OFFSET);

    for (int i = 0; i < 2; i++) {
        const int LEFT_EYE = 8;
        float eyeCoefficient = _blendshapeCoefficients[i] - _blendshapeCoefficients[LEFT_EYE + i];  // Raw value
        eyeCoefficient = glm::clamp(eyelidOffset + eyeCoefficient * (1.0f - eyelidOffset), -1.0f, 1.0f);
        if (eyeCoefficient > 0.0f) {
            _blendshapeCoefficients[i] = eyeCoefficient;
            _blendshapeCoefficients[LEFT_EYE + i] = 0.0f;

        } else {
            _blendshapeCoefficients[i] = 0.0f;
            _blendshapeCoefficients[LEFT_EYE + i] = -eyeCoefficient;
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
    return _owningAvatar->getOrientation() * getFinalOrientationInLocalFrame();
}

glm::quat Head::getFinalOrientationInLocalFrame() const {
    return glm::quat(glm::radians(glm::vec3(getFinalPitch(), getFinalYaw(), getFinalRoll() )));
}

// Everyone else's head keeps track of a lookAtPosition that everybody sees the same, and refers to where that head
// is looking in model space -- e.g., at someone's eyeball, or between their eyes, or mouth, etc. Everyon's Interface
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

void Head::setCorrectedLookAtPosition(glm::vec3 correctedLookAtPosition) {
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

glm::quat Head::getCameraOrientation() const {
    // NOTE: Head::getCameraOrientation() is not used for orienting the camera "view" while in Oculus mode, so
    // you may wonder why this code is here. This method will be called while in Oculus mode to determine how
    // to change the driving direction while in Oculus mode. It is used to support driving toward where you're
    // head is looking. Note that in oculus mode, your actual camera view and where your head is looking is not
    // always the same.
    if (qApp->isHMDMode()) {
        MyAvatar* myAvatar = dynamic_cast<MyAvatar*>(_owningAvatar);
        if (myAvatar) {
            return glm::quat_cast(myAvatar->getSensorToWorldMatrix()) * myAvatar->getHMDSensorOrientation();
        } else {
            return getOrientation();
        }
    } else {
        Avatar* owningAvatar = static_cast<Avatar*>(_owningAvatar);
        return owningAvatar->getWorldAlignedOrientation() * glm::quat(glm::radians(glm::vec3(_basePitch, 0.0f, 0.0f)));
    }
}

glm::quat Head::getEyeRotation(const glm::vec3& eyePosition) const {
    glm::quat orientation = getOrientation();
    glm::vec3 lookAtDelta = _lookAtPosition - eyePosition;
    return rotationBetween(orientation * IDENTITY_FRONT, lookAtDelta + glm::length(lookAtDelta) * _saccade) * orientation;
}

glm::vec3 Head::getScalePivot() const {
    return _position;
}

void Head::setFinalPitch(float finalPitch) {
    _deltaPitch = glm::clamp(finalPitch, MIN_HEAD_PITCH, MAX_HEAD_PITCH) - _basePitch;
}

void Head::setFinalYaw(float finalYaw) {
    _deltaYaw = glm::clamp(finalYaw, MIN_HEAD_YAW, MAX_HEAD_YAW) - _baseYaw;
}

void Head::setFinalRoll(float finalRoll) {
    _deltaRoll = glm::clamp(finalRoll, MIN_HEAD_ROLL, MAX_HEAD_ROLL) - _baseRoll;
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
