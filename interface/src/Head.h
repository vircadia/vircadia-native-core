//
//  Head.h
//  hifi
//
//  Created by Jeffrey on May, 10, 2013
//
// To avoid code bloat in Avatar.cpp, and to keep all head-related code in one place, 
// I have started moving the head code over to its own class. But it's gonna require 
// some work - so this is not done yet. 

#ifndef hifi_Head_h
#define hifi_Head_h

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <AvatarData.h>
#include "world.h"
#include "InterfaceConfig.h"
#include "SerialInterface.h"


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

    
    //  Do you want head to try to return to center (depends on interface detected)
    void setReturnToCenter(bool r) { returnHeadToCenter = r; }
    const bool getReturnToCenter() const { return returnHeadToCenter; }
    
//private:
// I am making these public for now - just to get the code moved over quickly!

        bool returnHeadToCenter;
        float audioLoudness;
        glm::vec3 skinColor;
        glm::vec3 position;
        glm::vec3 rotation;
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
        
        GLUquadric* sphere;
        
        
        //  Strength of return springs
        float returnSpringScale;
};

#endif
