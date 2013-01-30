//
//  Head.cpp
//  interface
//
//  Created by Philip Rosedale on 9/11/12.
//  Copyright (c) 2012 Physical, Inc.. All rights reserved.
//

#include <iostream>
#include "Head.h"
#include "Util.h"
#include "vector_angle.hpp"

float skinColor[] = {1.0, 0.84, 0.66};
float browColor[] = {210.0/255.0, 105.0/255.0, 30.0/255.0};
float mouthColor[] = {1, 0, 0};

float BrowRollAngle[5] = {0, 15, 30, -30, -15};
float BrowPitchAngle[3] = {-70, -60, -50};
float eyeColor[3] = {1,1,1};

float MouthWidthChoices[3] = {0.5, 0.77, 0.3};

float browWidth = 0.8;
float browThickness = 0.16;

const float DECAY = 0.1;

Head::Head()
{
    position.x = position.y = position.z = 0;
    PupilSize = 0.10;
    interPupilDistance = 0.6;
    interBrowDistance = 0.75;
    NominalPupilSize = 0.10;
    Yaw = 0.0;
    EyebrowPitch[0] = EyebrowPitch[1] = BrowPitchAngle[0];
    EyebrowRoll[0] = 30;
    EyebrowRoll[1] = -30;
    MouthPitch = 0;
    MouthYaw = 0;
    MouthWidth = 1.0;
    MouthHeight = 0.2;
    EyeballPitch[0] = EyeballPitch[1] = 0;
    EyeballScaleX = 1.2;  EyeballScaleY = 1.5; EyeballScaleZ = 1.0;
    EyeballYaw[0] = EyeballYaw[1] = 0;
    PitchTarget = YawTarget = 0; 
    NoiseEnvelope = 1.0;
    PupilConverge = 5.0;
    leanForward = 0.0;
    leanSideways = 0.0;
    setNoise(0);
}

void Head::reset()
{
    Pitch = Yaw = Roll = 0;
    leanForward = leanSideways = 0;
}

void Head::UpdatePos(float frametime, int * adc_channels, float * avg_adc_channels, int head_mirror, glm::vec3 * gravity)
//  Using serial data, update avatar/render position and angles
{
    float measured_pitch_rate = adc_channels[PITCH_RATE] - avg_adc_channels[PITCH_RATE];
    float measured_yaw_rate = adc_channels[YAW_RATE] - avg_adc_channels[YAW_RATE];
    float measured_lateral_accel = adc_channels[ACCEL_X] - avg_adc_channels[ACCEL_X];
    float measured_fwd_accel = avg_adc_channels[ACCEL_Z] - adc_channels[ACCEL_Z];
    float measured_roll_rate = adc_channels[ROLL_RATE] - avg_adc_channels[ROLL_RATE];
    
    //  Update avatar head position based on measured gyro rates
    const float HEAD_ROTATION_SCALE = 0.20;
    const float HEAD_ROLL_SCALE = 0.50;
    const float HEAD_LEAN_SCALE = 0.02;
    if (head_mirror) {
        addYaw(measured_yaw_rate * HEAD_ROTATION_SCALE * frametime);
        addPitch(measured_pitch_rate * -HEAD_ROTATION_SCALE * frametime);
        addRoll(measured_roll_rate * HEAD_ROLL_SCALE * frametime);
        addLean(measured_lateral_accel * frametime * HEAD_LEAN_SCALE, measured_fwd_accel*frametime * HEAD_LEAN_SCALE);
    } else {
        addYaw(measured_yaw_rate * -HEAD_ROTATION_SCALE * frametime);
        addPitch(measured_pitch_rate * -HEAD_ROTATION_SCALE * frametime);
        addRoll(measured_roll_rate * HEAD_ROLL_SCALE * frametime);
        addLean(measured_lateral_accel * frametime * -HEAD_LEAN_SCALE, measured_fwd_accel*frametime * HEAD_LEAN_SCALE);        
    } 
    
    
    //  Try to measure absolute roll from sensors 
    const float MIN_ROLL = 3.0;
    glm::vec3 v1(gravity->x, gravity->y, 0);
    glm::vec3 v2(adc_channels[ACCEL_X], adc_channels[ACCEL_Y], 0);
    float newRoll = acos(glm::dot(glm::normalize(v1), glm::normalize(v2))) ;
    if (newRoll != NAN) {
        newRoll *= 1000.0;
        if (newRoll > MIN_ROLL) {
            if (adc_channels[ACCEL_X] > gravity->x) newRoll *= -1.0;
            //SetRoll(newRoll);
        }
    }
}

void Head::addLean(float x, float z) {
    //  Add Body lean as impulse 
    leanSideways += x;
    leanForward += z;
}

void Head::setLeanForward(float dist){
    leanForward = dist;
}

void Head::setLeanSideways(float dist){
    leanSideways = dist;
}

//  Simulate the head over time 
void Head::simulate(float deltaTime)
{
    if (!noise)
    {
        //  Decay back toward center 
        Pitch *= (1.f - DECAY*deltaTime); 
        Yaw *= (1.f - DECAY*deltaTime);
        Roll *= (1.f - DECAY*deltaTime);
    }
    else {
        //  Move toward new target  
        Pitch += (PitchTarget - Pitch)*22*deltaTime;   // (1.f - DECAY*deltaTime)*Pitch + ; 
        Yaw += (YawTarget - Yaw)*22*deltaTime; //  (1.f - DECAY*deltaTime);
        Roll *= (1.f - DECAY*deltaTime);
    }
    
    leanForward *= (1.f - DECAY*30.f*deltaTime);
    leanSideways *= (1.f - DECAY*30.f*deltaTime);
    
    if (noise)
    {
        Pitch += (randFloat() - 0.5)*0.05*NoiseEnvelope;
        Yaw += (randFloat() - 0.5)*0.1*NoiseEnvelope;
        PupilSize += (randFloat() - 0.5)*0.001*NoiseEnvelope;
        
        if (randFloat() < 0.005) MouthWidth = MouthWidthChoices[rand()%3];
        
        //if (randFloat() < 0.005)  Pitch = (randFloat() - 0.5)*45;
        //if (randFloat() < 0.005)  Yaw = (randFloat() - 0.5)*45;
        //if (randFloat() < 0.001)  Roll = (randFloat() - 0.5)*45;
        //if (randFloat() < 0.003)  PupilSize = ((randFloat() - 0.5)*0.25+1)*NominalPupilSize;
        if (randFloat() < 0.01)  EyeballPitch[0] = EyeballPitch[1] = (randFloat() - 0.5)*20;
        if (randFloat() < 0.01)  EyeballYaw[0] = EyeballYaw[1] = (randFloat()- 0.5)*10;
        
        if ((randFloat() < 0.005) && (fabs(PitchTarget - Pitch) < 1.0) && (fabs(YawTarget - Yaw) < 1.0))
        {
            SetNewHeadTarget((randFloat()-0.5)*20.0, (randFloat()-0.5)*45.0);
        }
        
        if (0)
        {
            
            //  Pick new target 
            PitchTarget = (randFloat() - 0.5)*45;
            YawTarget = (randFloat() - 0.5)*22;
        }
        if (randFloat() < 0.01)
        {
            EyebrowPitch[0] = EyebrowPitch[1] = BrowPitchAngle[rand()%3];
            EyebrowRoll[0] = EyebrowRoll[1] = BrowRollAngle[rand()%5];
            EyebrowRoll[1]*=-1;
        }
                         
    }
                         
                         
}
      
void Head::render()
{
    int side = 0; 
    
    glEnable(GL_DEPTH_TEST);
    glTranslatef(leanSideways, 0.f, leanForward);

    glRotatef(Yaw/2.0, 0, 1, 0);
    glRotatef(Pitch/2.0, 1, 0, 0);
    glRotatef(Roll/2.0, 0, 0, 1);

    
    // Overall scale of head
    glScalef(1.5, 2.0, 2.0);
    glColor3fv(skinColor);
    
    //  Head
    glutSolidSphere(1, 30, 30);           
    
    //  Ears
    glPushMatrix();
        glTranslatef(1, 0, 0);
        for(side = 0; side < 2; side++)
        {
            glPushMatrix();
                glScalef(0.5, 0.75, 1.0);
                glutSolidSphere(0.5, 30, 30);  
            glPopMatrix();
            glTranslatef(-2, 0, 0);
        }
    glPopMatrix();
    

    // Eyebrows
    glPushMatrix();
        glTranslatef(-interBrowDistance/2.0,0.4,0.45);
        for(side = 0; side < 2; side++)
        {
            glColor3fv(browColor);
            glPushMatrix();
                glTranslatef(0, 0.4, 0);
                glRotatef(EyebrowPitch[side]/2.0, 1, 0, 0);
                glRotatef(EyebrowRoll[side]/2.0, 0, 0, 1);
                glScalef(browWidth, browThickness, 1);
                glutSolidCube(0.5);
            glPopMatrix();
            glTranslatef(interBrowDistance, 0, 0);
        }
    glPopMatrix();
    
    
    // Mouth
    glPushMatrix();
        glTranslatef(0,-0.3,0.75);
        glColor3fv(mouthColor);
        glRotatef(MouthPitch, 1, 0, 0);
        glRotatef(MouthYaw, 0, 0, 1);
        glScalef(MouthWidth, MouthHeight, 1);
        glutSolidCube(0.5);
    glPopMatrix();
    
    glTranslatef(0, 1.0, 0);

   
    glTranslatef(-interPupilDistance/2.0,-0.68,0.7);
    // Right Eye
    glRotatef(-10, 1, 0, 0);
    glColor3fv(eyeColor);
    glPushMatrix(); 
    {
        glTranslatef(interPupilDistance/10.0, 0, 0.05);
        glRotatef(20, 0, 0, 1);
        glScalef(EyeballScaleX, EyeballScaleY, EyeballScaleZ);
        glutSolidSphere(0.25, 30, 30);
    }
    glPopMatrix();
    // Right Pupil
    glPushMatrix();                         
        glRotatef(EyeballPitch[1], 1, 0, 0);
        glRotatef(EyeballYaw[1] + PupilConverge, 0, 1, 0);
        glTranslatef(0,0,.25);
        glColor3f(0,0,0);
        glutSolidSphere(PupilSize, 15, 15);          
    glPopMatrix();
    // Left Eye
    glColor3fv(eyeColor);
    glTranslatef(interPupilDistance, 0, 0);
    glPushMatrix(); 
    {
        glTranslatef(-interPupilDistance/10.0, 0, .05);
        glRotatef(-20, 0, 0, 1);
        glScalef(EyeballScaleX, EyeballScaleY, EyeballScaleZ);
        glutSolidSphere(0.25, 30, 30);
    }
    glPopMatrix();
    // Left Pupil
    glPushMatrix();
        glRotatef(EyeballPitch[0], 1, 0, 0);
        glRotatef(EyeballYaw[0] - PupilConverge, 0, 1, 0);
        glTranslatef(0,0,.25);
        glColor3f(0,0,0);
        glutSolidSphere(PupilSize, 15, 15);          
    glPopMatrix();
 }


//  Transmit data to agents requesting it 

int Head::getBroadcastData(char* data)
{
    // Copy data for transmission to the buffer, return length of data
    sprintf(data, "H%f,%f,%f,%f,%f,%f", Pitch, Yaw, Roll, position.x, position.y, position.z);
    return strlen(data);
}

void Head::recvBroadcastData(char * data, int size)
{
    sscanf(data, "H%f,%f,%f,%f,%f,%f", &Pitch, &Yaw, &Roll, &position.x, &position.y, &position.z);
}

void Head::SetNewHeadTarget(float pitch, float yaw)
{
    PitchTarget = pitch;
    YawTarget = yaw;
}
