//
//  Head.h
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef hifi_Head_h
#define hifi_Head_h

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <AvatarData.h>
#include "world.h"
#include "InterfaceConfig.h"
#include "SerialInterface.h"
#include "Orientation.h"

enum eyeContactTargets {LEFT_EYE, RIGHT_EYE, MOUTH};

class Head {
public:
    Head();
    
    void simulate(float deltaTime, bool isMine);
    void setPositionRotationAndScale(glm::vec3 position, glm::vec3 rotation, float scale);
    void render(bool lookingInMirror);
    void setNewTarget(float, float);
    void setSpringScale(float s) { _returnSpringScale = s; }
    void setLookatPosition(glm::vec3 l ) { _lookatPosition = l; }
    void setLooking(bool looking);
    void setGravity(glm::vec3 gravity) { _gravity = gravity; }
    void setBodyYaw(float y) { _bodyYaw = y; }
        
    glm::vec3 getApproximateEyePosition(); 
    
    //  Do you want head to try to return to center (depends on interface detected)
    void setReturnToCenter(bool returnHeadToCenter) { _returnHeadToCenter = returnHeadToCenter; }
    const bool getReturnToCenter() const { return _returnHeadToCenter; }
    
    float getAverageLoudness() {return _averageLoudness;};
    void  setAverageLoudness(float averageLoudness) { _averageLoudness = averageLoudness; }
    
    void setSkinColor(glm::vec3 skinColor) { _skinColor = skinColor; }
    void setAudioLoudness(float audioLoudness) { _audioLoudness = audioLoudness; }

    float yawRate;
    float noise;
    float leanForward;
    float leanSideways;

private: 

    bool _returnHeadToCenter;
    float _audioLoudness;
    glm::vec3 _skinColor;
    glm::vec3 _position;
    glm::vec3 _rotation;
    glm::vec3 _lookatPosition;
    
    glm::vec3 _leftEyePosition;
    glm::vec3 _rightEyePosition; 
    
    float _yaw;
    float _pitch;
    float _roll;
    float _eyeballPitch[2];
    float _eyeballYaw  [2];
    float _eyebrowPitch[2];
    float _eyebrowRoll [2];
    float _interBrowDistance;
    float _mouthPitch;
    float _mouthYaw;
    float _mouthWidth;
    float _mouthHeight;
    float _pitchTarget; 
    float _yawTarget; 
    float _noiseEnvelope;
    float _scale;
    int   _eyeContact;
    float _browAudioLift;
    eyeContactTargets _eyeContactTarget;
    Orientation _orientation;
    float _bodyYaw;
    
    //  Sound loudness information
    float _lastLoudness;
    float _averageLoudness;
    float _audioAttack;
    
    bool _looking;
    glm::vec3 _gravity;

    //  Strength of return springs
    float _returnSpringScale;
    
    // private methods
    void renderEyeBalls();
    void debugRenderLookatVectors(glm::vec3 leftEyePosition, glm::vec3 rightEyePosition, glm::vec3 lookatPosition);
    void updateEyePositions();
};

#endif
