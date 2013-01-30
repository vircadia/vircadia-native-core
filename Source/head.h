//
//  Head.h
//  interface
//
//  Created by Philip Rosedale on 9/11/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__head__
#define __interface__head__

#include <iostream>
#include "Field.h"
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
    float renderYaw;
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
    void setRoll(float r) {Roll = r; };
    void setRenderYaw(float y) {renderYaw = y;}
    void setLeanForward(float dist);
    void setLeanSideways(float dist);
    void addPitch(float p) {Pitch -= p; }
    void addYaw(float y){Yaw -= y; }
    void addRoll(float r){Roll += r; }
    void addLean(float x, float z);
    float getPitch() {return Pitch;}
    float getRoll() {return Roll;}
    float getYaw() {return Yaw;}
    float getRenderYaw() {return renderYaw;}
    void render();
    void simulate(float);
    //  Send and receive network data 
    int getBroadcastData(char* data);
    void recvBroadcastData(char * data, int size);
    void SetNewHeadTarget(float, float);
    glm::vec3 getPos() { return position; };
    void setPos(glm::vec3 newpos) { position = newpos; };
};

#endif