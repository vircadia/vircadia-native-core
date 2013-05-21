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

enum eyeContactTargets 
{
    LEFT_EYE, 
    RIGHT_EYE, 
    MOUTH
};

class Head : public HeadData {
public:
    Head();
    
    void reset();
    void simulate(float deltaTime, bool isMine);
    void render(bool lookingInMirror);

    void setLooking(bool looking);
    void setPositionAndScale(glm::vec3 position, float scale);
    void setNewTarget(float, float);
    
    void setGravity        (glm::vec3 gravity           ) { _gravity            = gravity;            }
    void setSkinColor      (glm::vec3 skinColor         ) { _skinColor          = skinColor;          }
    void setBodyYaw        (float     bodyYaw           ) { _bodyYaw            = bodyYaw;            }
    void setSpringScale    (float     returnSpringScale ) { _returnSpringScale  = returnSpringScale;  }
    void setAverageLoudness(float     averageLoudness   ) { _averageLoudness    = averageLoudness;    }
    void setAudioLoudness  (float     audioLoudness     ) { _audioLoudness      = audioLoudness;      }
    void setReturnToCenter (bool      returnHeadToCenter) { _returnHeadToCenter = returnHeadToCenter; }
        
    glm::vec3  getApproximateEyePosition(); 
    const bool getReturnToCenter() const { return _returnHeadToCenter; } // Do you want head to try to return to center (depends on interface detected)
    float      getAverageLoudness() {return _averageLoudness;};
    
    //some public members (left-over from pulling Head out of Avatar - I may see about privatizing these later).
    float yawRate;
    float noise;

private: 

    bool        _returnHeadToCenter;
    float       _audioLoudness;
    glm::vec3   _skinColor;
    glm::vec3   _position;
    glm::vec3   _rotation;
    glm::vec3   _leftEyePosition;
    glm::vec3   _rightEyePosition; 
    float       _eyeballPitch[2];
    float       _eyeballYaw  [2];
    float       _eyebrowPitch[2];
    float       _eyebrowRoll [2];
    float       _interBrowDistance;
    float       _mouthPitch;
    float       _mouthYaw;
    float       _mouthWidth;
    float       _mouthHeight;
    float       _pitchTarget; 
    float       _yawTarget; 
    float       _noiseEnvelope;
    float       _scale;
    int         _eyeContact;
    float       _browAudioLift;
    bool        _lookingAtSomething;
    glm::vec3   _gravity;
    float       _lastLoudness;
    float       _averageLoudness;
    float       _audioAttack;
    float       _returnSpringScale; //strength of return springs
    Orientation _orientation;
    float       _bodyYaw;
    eyeContactTargets _eyeContactTarget;
    
    // private methods
    void renderEyeBalls();
    void debugRenderLookatVectors(glm::vec3 leftEyePosition, glm::vec3 rightEyePosition, glm::vec3 lookatPosition);
    void updateEyePositions();
};

#endif
