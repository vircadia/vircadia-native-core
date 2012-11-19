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

/*
void update_pos(float frametime)
//  Using serial data, update avatar/render position and angles
{
    float measured_pitch_rate = adc_channels[0] - avg_adc_channels[0];
    float measured_yaw_rate = adc_channels[1] - avg_adc_channels[1];
    float measured_lateral_accel = adc_channels[3] - avg_adc_channels[3];
    float measured_fwd_accel = avg_adc_channels[2] - adc_channels[2];
    
    //  Update avatar head position based on measured gyro rates
    const float HEAD_ROTATION_SCALE = 0.20;
    const float HEAD_LEAN_SCALE = 0.02;
    if (head_mirror) {
        myHead.addYaw(measured_yaw_rate * HEAD_ROTATION_SCALE * frametime);
        myHead.addPitch(measured_pitch_rate * -HEAD_ROTATION_SCALE * frametime);
        myHead.addLean(measured_lateral_accel * frametime * HEAD_LEAN_SCALE, measured_fwd_accel*frametime * HEAD_LEAN_SCALE);
    } else {
        myHead.addYaw(measured_yaw_rate * -HEAD_ROTATION_SCALE * frametime);
        myHead.addPitch(measured_pitch_rate * -HEAD_ROTATION_SCALE * frametime);
        myHead.addLean(measured_lateral_accel * frametime * -HEAD_LEAN_SCALE, measured_fwd_accel*frametime * HEAD_LEAN_SCALE);        
    }
    //  Decay avatar head back toward zero
    //pitch *= (1.f - 5.0*frametime); 
    //yaw *= (1.f - 7.0*frametime);
    
    //  Update head_mouse model 
    const float MIN_MOUSE_RATE = 30.0;
    const float MOUSE_SENSITIVITY = 0.1;
    if (powf(measured_yaw_rate*measured_yaw_rate + 
             measured_pitch_rate*measured_pitch_rate, 0.5) > MIN_MOUSE_RATE)
    {
        head_mouse_x -= measured_yaw_rate*MOUSE_SENSITIVITY;
        head_mouse_y += measured_pitch_rate*MOUSE_SENSITIVITY*(float)HEIGHT/(float)WIDTH; 
    }
    head_mouse_x = max(head_mouse_x, 0);
    head_mouse_x = min(head_mouse_x, WIDTH);
    head_mouse_y = max(head_mouse_y, 0);
    head_mouse_y = min(head_mouse_y, HEIGHT);
    
      //  Update render direction (pitch/yaw) based on measured gyro rates
    const int MIN_YAW_RATE = 300;
    const float YAW_SENSITIVITY = 0.03;
    const int MIN_PITCH_RATE = 300;
    const float PITCH_SENSITIVITY = 0.04;
    
    if (fabs(measured_yaw_rate) > MIN_YAW_RATE) 
    {   
        if (measured_yaw_rate > 0)
            render_yaw_rate -= (measured_yaw_rate - MIN_YAW_RATE) * YAW_SENSITIVITY * frametime;
        else 
            render_yaw_rate -= (measured_yaw_rate + MIN_YAW_RATE) * YAW_SENSITIVITY * frametime;
    }
    if (fabs(measured_pitch_rate) > MIN_PITCH_RATE) 
    {
        if (measured_pitch_rate > 0)
            render_pitch_rate += (measured_pitch_rate - MIN_PITCH_RATE) * PITCH_SENSITIVITY * frametime;
        else 
            render_pitch_rate += (measured_pitch_rate + MIN_PITCH_RATE) * PITCH_SENSITIVITY * frametime;
    }
    render_yaw += render_yaw_rate;
    render_pitch += render_pitch_rate;
    
    // Decay render_pitch toward zero because we never look constantly up/down 
    render_pitch *= (1.f - 2.0*frametime);
    
    //  Decay angular rates toward zero 
    render_pitch_rate *= (1.f - 5.0*frametime);
    render_yaw_rate *= (1.f - 7.0*frametime);
    
    //  Update slide left/right based on accelerometer reading
    const int MIN_LATERAL_ACCEL = 20;
    const float LATERAL_SENSITIVITY = 0.001;
    if (fabs(measured_lateral_accel) > MIN_LATERAL_ACCEL) 
    {
        if (measured_lateral_accel > 0)
            lateral_vel += (measured_lateral_accel - MIN_LATERAL_ACCEL) * LATERAL_SENSITIVITY * frametime;
        else 
            lateral_vel += (measured_lateral_accel + MIN_LATERAL_ACCEL) * LATERAL_SENSITIVITY * frametime;
    }
    
    //slide += lateral_vel;
    lateral_vel *= (1.f - 4.0*frametime);
    
    //  Update fwd/back based on accelerometer reading
    const int MIN_FWD_ACCEL = 20;
    const float FWD_SENSITIVITY = 0.001;
    
    if (fabs(measured_fwd_accel) > MIN_FWD_ACCEL) 
    {
        if (measured_fwd_accel > 0)
            fwd_vel += (measured_fwd_accel - MIN_FWD_ACCEL) * FWD_SENSITIVITY * frametime;
        else 
            fwd_vel += (measured_fwd_accel + MIN_FWD_ACCEL) * FWD_SENSITIVITY * frametime;
        
    }
    //  Decrease forward velocity
    fwd_vel *= (1.f - 4.0*frametime);
    
    //  Update forward vector based on pitch and yaw 
    fwd_vec[0] = -sinf(render_yaw*PI/180);
    fwd_vec[1] = sinf(render_pitch*PI/180);
    fwd_vec[2] = cosf(render_yaw*PI/180);
    
    //  Advance location forward
    location[0] += fwd_vec[0]*fwd_vel;
    location[1] += fwd_vec[1]*fwd_vel;
    location[2] += fwd_vec[2]*fwd_vel;
    
    //  Slide location sideways
    location[0] += fwd_vec[2]*-lateral_vel;
    location[2] += fwd_vec[0]*lateral_vel;
    
    //  Update head and manipulator objects with object with current location
    myHead.setPos(glm::vec3(location[0], location[1], location[2]));
    balls.updateHand(myHead.getPos() + myHand.getPos(), glm::vec3(0,0,0), myHand.getRadius());
}
*/


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
