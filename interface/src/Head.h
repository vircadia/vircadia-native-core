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

    //void setLooking(bool looking);
    
    void setScale          (float     scale             ) { _scale              = scale;              }
    void setPosition       (glm::vec3 position          ) { _position           = position;           }
    void setBodyRotation   (glm::vec3 bodyRotation      ) { _bodyRotation       = bodyRotation;       }
    void setRotationOffBody(glm::vec3 headRotation      ) { _headRotation       = headRotation;       }
    void setGravity        (glm::vec3 gravity           ) { _gravity            = gravity;            }
    void setSkinColor      (glm::vec3 skinColor         ) { _skinColor          = skinColor;          }
    void setSpringScale    (float     returnSpringScale ) { _returnSpringScale  = returnSpringScale;  }
    void setAverageLoudness(float     averageLoudness   ) { _averageLoudness    = averageLoudness;    }
    void setAudioLoudness  (float     audioLoudness     ) { _audioLoudness      = audioLoudness;      }
    void setReturnToCenter (bool      returnHeadToCenter) { _returnHeadToCenter = returnHeadToCenter; }

    void setLookAtPosition (const glm::vec3& lookAtPosition); // overrides method in HeadData
        
    const bool getReturnToCenter() const { return _returnHeadToCenter; } // Do you want head to try to return to center (depends on interface detected)
    float getAverageLoudness() {return _averageLoudness;};
    
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
    glm::vec3   _leftEyeBrowPosition;
    glm::vec3   _rightEyeBrowPosition; 
    glm::vec3   _leftEarPosition;
    glm::vec3   _rightEarPosition; 
    glm::vec3   _mouthPosition; 
    float       _scale;
    float       _browAudioLift;
    bool        _lookingAtSomething;
    glm::vec3   _gravity;
    float       _lastLoudness;
    float       _averageLoudness;
    float       _audioAttack;
    float       _returnSpringScale; //strength of return springs
    Orientation _orientation;
    glm::vec3   _bodyRotation;
    glm::vec3   _headRotation;
    
    // private methods
    void renderHeadSphere();
    void renderEyeBalls();
    void renderEyeBrows();
    void renderEars();
    void renderMouth();
    void debugRenderLookatVectors(glm::vec3 leftEyePosition, glm::vec3 rightEyePosition, glm::vec3 lookatPosition);
    void calculateGeometry( bool lookingInMirror);
};

#endif
