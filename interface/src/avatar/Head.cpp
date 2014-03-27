//
//  Head.cpp
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <glm/gtx/quaternion.hpp>

#include <NodeList.h>

#include "Application.h"
#include "Avatar.h"
#include "GeometryUtil.h"
#include "Head.h"
#include "Menu.h"
#include "Util.h"

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
    _gravity(0.0f, -1.0f, 0.0f),
    _lastLoudness(0.0f),
    _audioAttack(0.0f),
    _angularVelocity(0,0,0),
    _renderLookatVectors(false),
    _saccade(0.0f, 0.0f, 0.0f),
    _saccadeTarget(0.0f, 0.0f, 0.0f),
    _leftEyeBlinkVelocity(0.0f),
    _rightEyeBlinkVelocity(0.0f),
    _timeWithoutTalking(0.0f),
    _pitchTweak(0.f),
    _yawTweak(0.f),
    _rollTweak(0.f),
    _isCameraMoving(false),
    _faceModel(this)
{
  
}

void Head::init() {
    _faceModel.init();
}

void Head::reset() {
    _yaw = _pitch = _roll = 0.0f;
    _leanForward = _leanSideways = 0.0f;
    _faceModel.reset();
}




void Head::simulate(float deltaTime, bool isMine, bool billboard) {
    
    //  Update audio trailing average for rendering facial animations
    Faceshift* faceshift = Application::getInstance()->getFaceshift();
    Visage* visage = Application::getInstance()->getVisage();
    if (isMine) {
        _isFaceshiftConnected = false;
        if (faceshift->isActive()) {
            _blendshapeCoefficients = faceshift->getBlendshapeCoefficients();
            _isFaceshiftConnected = true;
            
        } else if (visage->isActive()) {
            _blendshapeCoefficients = visage->getBlendshapeCoefficients();
            _isFaceshiftConnected = true;
        }
    }
    
    if (!(_isFaceshiftConnected || billboard)) {
        // Update eye saccades
        const float AVERAGE_MICROSACCADE_INTERVAL = 0.50f;
        const float AVERAGE_SACCADE_INTERVAL = 4.0f;
        const float MICROSACCADE_MAGNITUDE = 0.002f;
        const float SACCADE_MAGNITUDE = 0.04f;
        
        if (randFloat() < deltaTime / AVERAGE_MICROSACCADE_INTERVAL) {
            _saccadeTarget = MICROSACCADE_MAGNITUDE * randVector();
        } else if (randFloat() < deltaTime / AVERAGE_SACCADE_INTERVAL) {
            _saccadeTarget = SACCADE_MAGNITUDE * randVector();
        }
        _saccade += (_saccadeTarget - _saccade) * 0.50f;
    
        const float AUDIO_AVERAGING_SECS = 0.05f;
        _averageLoudness = glm::mix(_averageLoudness, _audioLoudness, glm::min(deltaTime / AUDIO_AVERAGING_SECS, 1.0f));
        
        //  Detect transition from talking to not; force blink after that and a delay
        bool forceBlink = false;
        const float TALKING_LOUDNESS = 100.0f;
        const float BLINK_AFTER_TALKING = 0.25f;
        if (_averageLoudness > TALKING_LOUDNESS) {
            _timeWithoutTalking = 0.0f;
        
        } else if (_timeWithoutTalking < BLINK_AFTER_TALKING && (_timeWithoutTalking += deltaTime) >= BLINK_AFTER_TALKING) {
            forceBlink = true;
        }
                                 
        //  Update audio attack data for facial animation (eyebrows and mouth)
        _audioAttack = 0.9f * _audioAttack + 0.1f * fabs(_audioLoudness - _lastLoudness);
        _lastLoudness = _audioLoudness;
        
        const float BROW_LIFT_THRESHOLD = 100.0f;
        if (_audioAttack > BROW_LIFT_THRESHOLD) {
            _browAudioLift += sqrtf(_audioAttack) * 0.00005f;
        }
        
        const float CLAMP = 0.01f;
        if (_browAudioLift > CLAMP) {
            _browAudioLift = CLAMP;
        }
        
        _browAudioLift *= 0.7f;      

        const float BLINK_SPEED = 10.0f;
        const float FULLY_OPEN = 0.0f;
        const float FULLY_CLOSED = 1.0f;
        if (_leftEyeBlinkVelocity == 0.0f && _rightEyeBlinkVelocity == 0.0f) {
            // no blinking when brows are raised; blink less with increasing loudness
            const float BASE_BLINK_RATE = 15.0f / 60.0f;
            const float ROOT_LOUDNESS_TO_BLINK_INTERVAL = 0.25f;
            if (forceBlink || (_browAudioLift < EPSILON && shouldDo(glm::max(1.0f, sqrt(_averageLoudness) *
                    ROOT_LOUDNESS_TO_BLINK_INTERVAL) / BASE_BLINK_RATE, deltaTime))) {
                _leftEyeBlinkVelocity = BLINK_SPEED;
                _rightEyeBlinkVelocity = BLINK_SPEED;
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
        const float BROW_LIFT_SCALE = 500.0f;
        const float JAW_OPEN_SCALE = 0.01f;
        const float JAW_OPEN_DEAD_ZONE = 0.75f;
        faceshift->updateFakeCoefficients(_leftEyeBlink, _rightEyeBlink, min(1.0f, _browAudioLift * BROW_LIFT_SCALE),
            glm::clamp(sqrt(_averageLoudness * JAW_OPEN_SCALE) - JAW_OPEN_DEAD_ZONE, 0.0f, 1.0f), _blendshapeCoefficients);
    }
    
    if (!isMine) {
        _faceModel.setLODDistance(static_cast<Avatar*>(_owningAvatar)->getLODDistance());
    }
    _leftEyePosition = _rightEyePosition = getPosition();
    if (!billboard) {
        _faceModel.simulate(deltaTime);
        _faceModel.getEyePositions(_leftEyePosition, _rightEyePosition);
    }
    _eyePosition = calculateAverageEyePosition();
}

void Head::render(float alpha, bool forShadowMap) {
    if (_faceModel.render(alpha, forShadowMap) && _renderLookatVectors) {
        renderLookatVectors(_leftEyePosition, _rightEyePosition, _lookAtPosition);
    }
}

void Head::setScale (float scale) {
    if (_scale == scale) {
        return;
    }
    _scale = scale;
}

glm::quat Head::getTweakedOrientation() const {
    return _owningAvatar->getOrientation() * glm::quat(glm::radians(
                glm::vec3(getTweakedPitch(), getTweakedYaw(), getTweakedRoll() )));
}

glm::quat Head::getCameraOrientation () const {
    Avatar* owningAvatar = static_cast<Avatar*>(_owningAvatar);
    return owningAvatar->getWorldAlignedOrientation() * glm::quat(glm::radians(glm::vec3(_pitch, 0.f, 0.0f)));
}

glm::quat Head::getEyeRotation(const glm::vec3& eyePosition) const {
    glm::quat orientation = getOrientation();
    return rotationBetween(orientation * IDENTITY_FRONT, _lookAtPosition + _saccade - eyePosition) * orientation;
}

glm::vec3 Head::getScalePivot() const {
    return _faceModel.isActive() ? _faceModel.getTranslation() : _position;
}

float Head::getTweakedYaw() const {
    return glm::clamp(_yaw + _yawTweak, MIN_HEAD_YAW, MAX_HEAD_YAW);
}

float Head::getTweakedPitch() const {
    return glm::clamp(_pitch + _pitchTweak, MIN_HEAD_PITCH, MAX_HEAD_PITCH);
}

float Head::getTweakedRoll() const {
    return glm::clamp(_roll + _rollTweak, MIN_HEAD_ROLL, MAX_HEAD_ROLL);
}

void Head::renderLookatVectors(glm::vec3 leftEyePosition, glm::vec3 rightEyePosition, glm::vec3 lookatPosition) {

    Application::getInstance()->getGlowEffect()->begin();
    
    glLineWidth(2.0);
    glBegin(GL_LINES);
    glColor4f(0.2f, 0.2f, 0.2f, 1.f);
    glVertex3f(leftEyePosition.x, leftEyePosition.y, leftEyePosition.z);
    glColor4f(1.0f, 1.0f, 1.0f, 0.f);
    glVertex3f(lookatPosition.x, lookatPosition.y, lookatPosition.z);
    glColor4f(0.2f, 0.2f, 0.2f, 1.f);
    glVertex3f(rightEyePosition.x, rightEyePosition.y, rightEyePosition.z);
    glColor4f(1.0f, 1.0f, 1.0f, 0.f);
    glVertex3f(lookatPosition.x, lookatPosition.y, lookatPosition.z);
    glEnd();
    
    Application::getInstance()->getGlowEffect()->end();
}


