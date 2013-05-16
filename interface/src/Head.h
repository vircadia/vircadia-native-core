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
    
    void initialize();
    void simulate(float deltaTime, bool isMine);
    void setPositionRotationAndScale(glm::vec3 position, glm::vec3 rotation, float scale);
    void setSkinColor(glm::vec3 color);
    void setAudioLoudness(float loudness);
    void render(bool lookingInMirror, float bodyYaw);
    void setNewTarget(float, float);
    void setSpringScale(float s) { returnSpringScale = s; }
    void setLookatPosition(glm::vec3 lookatPosition);
    void setLooking(bool looking);
    
    //  Do you want head to try to return to center (depends on interface detected)
    void setReturnToCenter(bool r) { returnHeadToCenter = r; }
    const bool getReturnToCenter() const { return returnHeadToCenter; }
    
    float getAverageLoudness() {return averageLoudness;};
    void  setAverageLoudness(float al) { averageLoudness = al;};

    bool returnHeadToCenter;
    float audioLoudness;
    glm::vec3 skinColor;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 lookatPosition;
    float yaw;
    float pitch;
    float roll;
    float pitchRate;
    float yawRate;
    float rollRate;
    float noise;
    float eyeballPitch[2];
    float eyeballYaw  [2];
    float eyebrowPitch[2];
    float eyebrowRoll [2];
    float eyeballScaleX;
    float eyeballScaleY;
    float eyeballScaleZ;
    float interPupilDistance;
    float interBrowDistance;
    float nominalPupilSize;
    float pupilSize;
    float mouthPitch;
    float mouthYaw;
    float mouthWidth;
    float mouthHeight;
    float leanForward;
    float leanSideways;
    float pitchTarget; 
    float yawTarget; 
    float noiseEnvelope;
    float pupilConverge;
    float scale;
    int   eyeContact;
    float browAudioLift;
    eyeContactTargets eyeContactTarget;
    
    //  Sound loudness information
    float lastLoudness;
    float averageLoudness;
    float audioAttack;
    
    bool _looking;
    
    GLUquadric* sphere;

    //  Strength of return springs
    float returnSpringScale;
    
private: 
    void renderEyeBalls();
    void renderIrises(float yaw);
    void debugRenderLookatVectors(glm::vec3 leftEyePosition, glm::vec3 rightEyePosition, glm::vec3 lookatPosition);
    
};

#endif
