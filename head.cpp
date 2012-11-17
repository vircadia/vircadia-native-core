//
//  head.cpp
//  interface
//
//  Created by Philip Rosedale on 9/11/12.
//  Copyright (c) 2012 Physical, Inc.. All rights reserved.
//

#include <iostream>
#include "head.h"
#include "util.h"

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
    PupilSize = 0.10;
    interPupilDistance = 0.5;
    interBrowDistance = 0.75;
    NominalPupilSize = 0.10;
    EyebrowPitch[0] = EyebrowPitch[1] = BrowPitchAngle[0];
    EyebrowRoll[0] = 30;
    EyebrowRoll[1] = -30;
    MouthPitch = 0;
    MouthYaw = 0;
    MouthWidth = 1.0;
    MouthHeight = 0.2;
    EyeballPitch[0] = EyeballPitch[1] = 0;
    EyeballYaw[0] = EyeballYaw[1] = 0;
    PitchTarget = YawTarget = 0; 
    NoiseEnvelope = 1.0;
    PupilConverge = 2.1;
    leanForward = 0.0;
    leanSideways = 0.0;
    setNoise(0);
}

void Head::reset()
{
    position = glm::vec3(0,0,0);
    Pitch = 0;
    Yaw = 0;
    leanForward = leanSideways = 0;
}

//  Read the sensors
void readSensors()
{
    
}

void Head::addLean(float x, float z) {
    //  Add Body lean as impulse 
    leanSideways += x;
    leanForward += z;
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
    glPushMatrix();
        glLoadIdentity();
        glTranslatef(0.f, 0.f, -7.f);
        glTranslatef(leanSideways, 0.f, leanForward);
        glRotatef(Yaw/2.0, 0, 1, 0);
        glRotatef(Pitch/2.0, 1, 0, 0);
        glRotatef(Roll/2.0, 0, 0, 1);
        
        // Overall scale of head
        glScalef(2.0, 2.0, 2.0);
        glColor3fv(skinColor);
        
        //  Head
        glutSolidSphere(1, 15, 15);           
        
        //  Ears
        glPushMatrix();
            glTranslatef(1, 0, 0);
            for(side = 0; side < 2; side++)
            {
                glPushMatrix();
                    glScalef(0.5, 0.75, 1.0);
                    glutSolidSphere(0.5, 15, 15);  
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

        // Right Eye
        glTranslatef(-0.25,-0.5,0.7);
        glColor3fv(eyeColor);
        glutSolidSphere(0.25, 15, 15);         
        // Right Pupil
        glPushMatrix();                         
            glRotatef(EyeballPitch[1], 1, 0, 0);
            glRotatef(EyeballYaw[1] + PupilConverge, 0, 1, 0);
            glTranslatef(0,0,.25);
            glColor3f(0,0,0);
            glutSolidSphere(PupilSize, 10, 10);          
        glPopMatrix();
        // Left Eye
        glColor3fv(eyeColor);
        glTranslatef(interPupilDistance, 0, 0);
        glutSolidSphere(0.25f, 15, 15);         
        // Left Pupil
        glPushMatrix();
            glRotatef(EyeballPitch[0], 1, 0, 0);
            glRotatef(EyeballYaw[0] - PupilConverge, 0, 1, 0);
            glTranslatef(0,0,.25);
            glColor3f(0,0,0);
            glutSolidSphere(PupilSize, 10, 10);          
        glPopMatrix();

        
    glPopMatrix();
    
}


//  Transmit data to agents requesting it 

int Head::transmit(char* data)
{
    // Copy data for transmission to the buffer, return length of data
    sprintf(data, "%f6.2", Pitch);
    return strlen(data);
}

void Head::SetNewHeadTarget(float pitch, float yaw)
{
    PitchTarget = pitch;
    YawTarget = yaw;
}
