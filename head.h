//
//  head.h
//  interface
//
//  Created by Philip Rosedale on 9/11/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef interface_head_h
#define interface_head_h

#include <iostream>
#include "field.h"
#include "world.h"
#include <GLUT/glut.h>
#include "SerialInterface.h"

class Head {
    float noise;
    float Pitch;
    float Yaw;
    float Roll;
    float PitchRate;
    float YawRate;
    float RollRate;
    float EyeballPitch[2];
    float EyeballYaw[2];
    float EyebrowPitch[2];
    float EyebrowRoll[2];
    float EyeballScaleX, EyeballScaleY, EyeballScaleZ;
    float interPupilDistance;
    float interBrowDistance;
    float NominalPupilSize;
    float PupilSize;
    float MouthPitch;
    float MouthYaw;
    float MouthWidth;
    float MouthHeight;
    float leanForward;
    float leanSideways;
    
    float PitchTarget; 
    float YawTarget; 
    
    float NoiseEnvelope;
    
    float PupilConverge;
    
    glm::vec3 position;
    
    void readSensors();
    
public:
    Head(void);
    void reset();
    void UpdatePos(float frametime, int * adc_channels, float * avg_adc_channels, 
                   int head_mirror, glm::vec3 * gravity);
    void setNoise (float mag) { noise = mag; }
    void setPitch(float p) {Pitch = p; }
    void setYaw(float y) {Yaw = y; }
    void SetRoll(float r) {Roll = r; };
    void addPitch(float p) {Pitch -= p; }
    void addYaw(float y){Yaw -= y; }
    void addRoll(float r){Roll += r; }
    void addLean(float x, float z);
    void getPitch(float);
    void render();
    void simulate(float);
    int transmit(char*);
    void receive(float);
    void SetNewHeadTarget(float, float);
    glm::vec3 getPos() { return position; };
    void setPos(glm::vec3 newpos) { position = newpos; };
};

#endif