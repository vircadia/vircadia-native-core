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

#include <DependencyManager.h>
#include <DeferredLightingEffect.h>
#include <NodeList.h>

#include "Application.h"
#include "Avatar.h"
#include "GeometryUtil.h"
#include "Head.h"
#include "Menu.h"
#include "Util.h"
#include "devices/DdeFaceTracker.h"
#include "devices/Faceshift.h"

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
    _mouth2(0.0f),
    _mouth3(0.0f),
    _mouth4(0.0f),
    _renderLookatVectors(false),
    _saccade(0.0f, 0.0f, 0.0f),
    _saccadeTarget(0.0f, 0.0f, 0.0f),
    _leftEyeBlinkVelocity(0.0f),
    _rightEyeBlinkVelocity(0.0f),
    _timeWithoutTalking(0.0f),
    _deltaPitch(0.0f),
    _deltaYaw(0.0f),
    _deltaRoll(0.0f),
    _deltaLeanSideways(0.0f),
    _deltaLeanForward(0.0f),
    _isCameraMoving(false),
    _isLookingAtMe(false),
    _faceModel(this),
    _leftEyeLookAtID(DependencyManager::get<GeometryCache>()->allocateID()),
    _rightEyeLookAtID(DependencyManager::get<GeometryCache>()->allocateID())
{
  
}

void Head::init() {
    _faceModel.init();
}

void Head::reset() {
    _baseYaw = _basePitch = _baseRoll = 0.0f;
    _leanForward = _leanSideways = 0.0f;
    _faceModel.reset();
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
        MyAvatar* myAvatar = static_cast<MyAvatar*>(_owningAvatar);
        
        // Only use face trackers when not playing back a recording.
        if (!myAvatar->isPlaying()) {
            FaceTracker* faceTracker = Application::getInstance()->getActiveFaceTracker();
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
        }
        //  Twist the upper body to follow the rotation of the head, but only do this with my avatar,
        //  since everyone else will see the full joint rotations for other people.  
        const float BODY_FOLLOW_HEAD_YAW_RATE = 0.1f;
        const float BODY_FOLLOW_HEAD_FACTOR = 0.66f;
        float currentTwist = getTorsoTwist();
        setTorsoTwist(currentTwist + (getFinalYaw() * BODY_FOLLOW_HEAD_FACTOR - currentTwist) * BODY_FOLLOW_HEAD_YAW_RATE);
    }
   
    if (!(_isFaceTrackerConnected || billboard)) {
        // Update eye saccades
        const float AVERAGE_MICROSACCADE_INTERVAL = 1.0f;
        const float AVERAGE_SACCADE_INTERVAL = 6.0f;
        const float MICROSACCADE_MAGNITUDE = 0.002f;
        const float SACCADE_MAGNITUDE = 0.04f;

        if (randFloat() < deltaTime / AVERAGE_MICROSACCADE_INTERVAL) {
            _saccadeTarget = MICROSACCADE_MAGNITUDE * randVector();
        } else if (randFloat() < deltaTime / AVERAGE_SACCADE_INTERVAL) {
            _saccadeTarget = SACCADE_MAGNITUDE * randVector();
        }
        _saccade += (_saccadeTarget - _saccade) * 0.5f;

        //  Detect transition from talking to not; force blink after that and a delay
        bool forceBlink = false;
        const float TALKING_LOUDNESS = 100.0f;
        const float BLINK_AFTER_TALKING = 0.25f;
        if ((_averageLoudness - _longTermAverageLoudness) > TALKING_LOUDNESS) {
            _timeWithoutTalking = 0.0f;
        
        } else if (_timeWithoutTalking < BLINK_AFTER_TALKING && (_timeWithoutTalking += deltaTime) >= BLINK_AFTER_TALKING) {
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
    
    if (!isMine) {
        _faceModel.setLODDistance(static_cast<Avatar*>(_owningAvatar)->getLODDistance());
    }
    _leftEyePosition = _rightEyePosition = getPosition();
    if (!billboard) {
        _faceModel.simulate(deltaTime);
        if (!_faceModel.getEyePositions(_leftEyePosition, _rightEyePosition)) {
            static_cast<Avatar*>(_owningAvatar)->getSkeletonModel().getEyePositions(_leftEyePosition, _rightEyePosition);
        }
    }
    _eyePosition = calculateAverageEyePosition();
}

void Head::calculateMouthShapes() {
    const float JAW_OPEN_SCALE = 0.015f;
    const float JAW_OPEN_RATE = 0.9f;
    const float JAW_CLOSE_RATE = 0.90f;
    float audioDelta = sqrtf(glm::max(_averageLoudness - _longTermAverageLoudness, 0.0f)) * JAW_OPEN_SCALE;
    if (audioDelta > _audioJawOpen) {
        _audioJawOpen += (audioDelta - _audioJawOpen) * JAW_OPEN_RATE;
    } else {
        _audioJawOpen *= JAW_CLOSE_RATE;
    }
    _audioJawOpen = glm::clamp(_audioJawOpen, 0.0f, 1.0f);

    // _mouth2 = "mmmm" shape
    // _mouth3 = "funnel" shape
    // _mouth4 = "smile" shape
    const float FUNNEL_PERIOD = 0.985f;
    const float FUNNEL_RANDOM_PERIOD = 0.01f;
    const float MMMM_POWER = 0.25f;
    const float MMMM_PERIOD = 0.91f;
    const float MMMM_RANDOM_PERIOD = 0.15f;
    const float SMILE_PERIOD = 0.925f;
    const float SMILE_RANDOM_PERIOD = 0.05f;

    _mouth3 = glm::mix(_audioJawOpen, _mouth3, FUNNEL_PERIOD + randFloat() * FUNNEL_RANDOM_PERIOD);
    _mouth2 = glm::mix(_audioJawOpen * MMMM_POWER, _mouth2, MMMM_PERIOD + randFloat() * MMMM_RANDOM_PERIOD);
    _mouth4 = glm::mix(_audioJawOpen, _mouth4, SMILE_PERIOD + randFloat() * SMILE_RANDOM_PERIOD);
}

void Head::applyEyelidOffset(glm::quat headOrientation) {
    // Adjusts the eyelid blendshape coefficients so that the eyelid follows the iris as the head pitches.

    glm::quat eyeRotation = rotationBetween(headOrientation * IDENTITY_FRONT, getCorrectedLookAtPosition() - _eyePosition);
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

void Head::relaxLean(float deltaTime) {
    // restore rotation, lean to neutral positions
    const float LEAN_RELAXATION_PERIOD = 0.25f;   // seconds
    float relaxationFactor = 1.0f - glm::min(deltaTime / LEAN_RELAXATION_PERIOD, 1.0f);
    _deltaYaw *= relaxationFactor;
    _deltaPitch *= relaxationFactor;
    _deltaRoll *= relaxationFactor;
    _leanSideways *= relaxationFactor;
    _leanForward *= relaxationFactor;
    _deltaLeanSideways *= relaxationFactor;
    _deltaLeanForward *= relaxationFactor;
}

void Head::render(RenderArgs* renderArgs, float alpha, ViewFrustum* renderFrustum, bool postLighting) {
    if (_renderLookatVectors) {
        renderLookatVectors(renderArgs, _leftEyePosition, _rightEyePosition, getCorrectedLookAtPosition());
    }
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

glm::vec3 Head::getCorrectedLookAtPosition() {
    if (_isLookingAtMe) {
        return _correctedLookAtPosition;
    } else {
        return getLookAtPosition();
    }
}

void Head::setCorrectedLookAtPosition(glm::vec3 correctedLookAtPosition) {
    _isLookingAtMe = true;
    _correctedLookAtPosition = correctedLookAtPosition;
}

glm::quat Head::getCameraOrientation() const {
    // NOTE: Head::getCameraOrientation() is not used for orienting the camera "view" while in Oculus mode, so
    // you may wonder why this code is here. This method will be called while in Oculus mode to determine how
    // to change the driving direction while in Oculus mode. It is used to support driving toward where you're 
    // head is looking. Note that in oculus mode, your actual camera view and where your head is looking is not
    // always the same.
    if (qApp->isHMDMode()) {
        return getOrientation();
    }
    Avatar* owningAvatar = static_cast<Avatar*>(_owningAvatar);
    return owningAvatar->getWorldAlignedOrientation() * glm::quat(glm::radians(glm::vec3(_basePitch, 0.0f, 0.0f)));
}

glm::quat Head::getEyeRotation(const glm::vec3& eyePosition) const {
    glm::quat orientation = getOrientation();
    return rotationBetween(orientation * IDENTITY_FRONT, _lookAtPosition + _saccade - eyePosition) * orientation;
}

glm::vec3 Head::getScalePivot() const {
    return _faceModel.isActive() ? _faceModel.getTranslation() : _position;
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

void Head::addLeanDeltas(float sideways, float forward) {
    _deltaLeanSideways += sideways;
    _deltaLeanForward += forward;
}

void Head::renderLookatVectors(RenderArgs* renderArgs, glm::vec3 leftEyePosition, glm::vec3 rightEyePosition, glm::vec3 lookatPosition) {
    auto& batch = *renderArgs->_batch;
    auto transform = Transform{};
    batch.setModelTransform(transform);
    batch._glLineWidth(2.0f);

    auto deferredLighting = DependencyManager::get<DeferredLightingEffect>();
    deferredLighting->bindSimpleProgram(batch);

    auto geometryCache = DependencyManager::get<GeometryCache>();
    glm::vec4 startColor(0.2f, 0.2f, 0.2f, 1.0f);
    glm::vec4 endColor(1.0f, 1.0f, 1.0f, 0.0f);
    geometryCache->renderLine(batch, leftEyePosition, lookatPosition, startColor, endColor, _leftEyeLookAtID);
    geometryCache->renderLine(batch, rightEyePosition, lookatPosition, startColor, endColor, _rightEyeLookAtID);
}


