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
#include <SharedUtil.h>

enum eyeContactTargets 
{
    LEFT_EYE, 
    RIGHT_EYE, 
    MOUTH
};

const int NUM_HAIR_TUFTS    = 4;

class Avatar;
class ProgramObject;

class Head : public HeadData {
public:
    Head(Avatar* owningAvatar);
    
    void init();
    void reset();
    void simulate(float deltaTime, bool isMine);
    void render(float alpha);
    void renderMohawk();

    void setScale          (float     scale             ) { _scale              = scale;              }
    void setPosition       (glm::vec3 position          ) { _position           = position;           }
    void setBodyRotation   (glm::vec3 bodyRotation      ) { _bodyRotation       = bodyRotation;       }
    void setGravity        (glm::vec3 gravity           ) { _gravity            = gravity;            }
    void setSkinColor      (glm::vec3 skinColor         ) { _skinColor          = skinColor;          }
    void setSpringScale    (float     returnSpringScale ) { _returnSpringScale  = returnSpringScale;  }
    void setAverageLoudness(float     averageLoudness   ) { _averageLoudness    = averageLoudness;    }
    void setReturnToCenter (bool      returnHeadToCenter) { _returnHeadToCenter = returnHeadToCenter; }
    void setRenderLookatVectors(bool onOff ) { _renderLookatVectors = onOff; }
    
    void setCameraFollowsHead(bool cameraFollowsHead) { _cameraFollowsHead = cameraFollowsHead; }
    
    glm::quat getOrientation() const;
    glm::quat getCameraOrientation () const;
    
    glm::vec3 getPosition()       const { return _position; }
    glm::vec3 getEyeLevelPosition() const { return _eyeLevelPosition; }
    glm::vec3 getRightDirection() const { return getOrientation() * IDENTITY_RIGHT; }
    glm::vec3 getUpDirection   () const { return getOrientation() * IDENTITY_UP;    }
    glm::vec3 getFrontDirection() const { return getOrientation() * IDENTITY_FRONT; }
    
    const bool getReturnToCenter() const { return _returnHeadToCenter; } // Do you want head to try to return to center (depends on interface detected)
    float getAverageLoudness() {return _averageLoudness;};
    glm::vec3 calculateAverageEyePosition() { return _leftEyePosition + (_rightEyePosition - _leftEyePosition ) * ONE_HALF; }
    
    float yawRate;
    float noise;

private:
    // disallow copies of the Head, copy of owning Avatar is disallowed too
    Head(const Head&);
    Head& operator= (const Head&);

    struct HairTuft
    {
        float length;
        float thickness;
        
        glm::vec3 basePosition;				
        glm::vec3 midPosition;          
        glm::vec3 endPosition;          
        glm::vec3 midVelocity;          
        glm::vec3 endVelocity;  
    };

    float       _renderAlpha;
    bool        _returnHeadToCenter;
    glm::vec3   _skinColor;
    glm::vec3   _position;
    glm::vec3   _rotation;
    glm::vec3   _leftEyePosition;
    glm::vec3   _rightEyePosition;
    glm::vec3   _eyeLevelPosition; 
    glm::vec3   _leftEyeBrowPosition;
    glm::vec3   _rightEyeBrowPosition; 
    glm::vec3   _leftEarPosition;
    glm::vec3   _rightEarPosition; 
    glm::vec3   _mouthPosition; 
    float       _scale;
    float       _browAudioLift;
    glm::vec3   _gravity;
    float       _lastLoudness;
    float       _averageLoudness;
    float       _audioAttack;
    float       _returnSpringScale; //strength of return springs
    glm::vec3   _bodyRotation;
    bool        _renderLookatVectors;
    HairTuft    _hairTuft[NUM_HAIR_TUFTS];
    glm::vec3*  _mohawkTriangleFan;
    glm::vec3*  _mohawkColors;
    glm::vec3   _saccade;
    glm::vec3   _saccadeTarget;
    float       _leftEyeBlink;
    float       _rightEyeBlink;
    float       _leftEyeBlinkVelocity;
    float       _rightEyeBlinkVelocity;
    float       _timeWithoutTalking;
    float       _cameraPitch;                   //  Used to position the camera differently from the head
    float       _cameraYaw;
    bool        _isCameraMoving;
    bool        _cameraFollowsHead;
    float       _cameraFollowHeadRate;
    
    static ProgramObject* _irisProgram;
    static GLuint _irisTextureID;
    static int _eyePositionLocation;
    
    // private methods
    void createMohawk();
    void renderHeadSphere();
    void renderEyeBalls();
    void renderEyeBrows();
    void renderEars();
    void renderMouth();
    void renderLookatVectors(glm::vec3 leftEyePosition, glm::vec3 rightEyePosition, glm::vec3 lookatPosition);
    void calculateGeometry();
    void resetHairPhysics();
    void updateHairPhysics(float deltaTime);
};

#endif
