//
//  Hand.cpp
//  interface
//
//  Created by Philip Rosedale on 10/13/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include "Hand.h"
#include <sys/time.h>

const float PHI = 1.618;

const float DEFAULT_X = 0;
const float DEFAULT_Y = -1.5;
const float DEFAULT_Z = 2.0;

Hand::Hand(glm::vec3 initcolor)
{
    color = initcolor;
    reset();
    noise = 0.2;
    scale.x = 0.07;
    scale.y = scale.x * 5.0;
    scale.z = scale.y * 1.0;
}

void Hand::reset()
{
    position.x = DEFAULT_X;
    position.y = DEFAULT_Y;
    position.z = DEFAULT_Z;
    pitch = yaw = roll = 0;
    pitchRate = yawRate = rollRate = 0;
    setTarget(position);
    velocity.x = velocity.y = velocity.z = 0;
    transmitterPackets = 0;
}


void Hand::addAngularVelocity (float pRate, float yRate, float rRate) {
    pitchRate += pRate;
    yawRate += yRate;
    rollRate += rRate;
}

void Hand::processTransmitterData(char *packetData, int numBytes) {
    //  Read a packet from a transmitter app, process the data
    float accX, accY, accZ,
        graX, graY, graZ,
        gyrX, gyrY, gyrZ,
        linX, linY, linZ,
        rot1, rot2, rot3, rot4;
    sscanf((char *)packetData, "tacc %f %f %f gra %f %f %f gyr %f %f %f lin %f %f %f rot %f %f %f %f",
           &accX, &accY, &accZ,
           &graX, &graY, &graZ,
           &gyrX, &gyrY, &gyrZ,
           &linX, &linY, &linZ,
           &rot1, &rot2, &rot3, &rot4);
    
    if (transmitterPackets++ == 0) {
            gettimeofday(&transmitterTimer, NULL);
    }
    const int TRANSMITTER_COUNT = 100;
    if (transmitterPackets % TRANSMITTER_COUNT == 0) {
        // Every 100 packets, record the observed Hz of the transmitter data
        timeval now;
        gettimeofday(&now, NULL);
        double msecsElapsed = diffclock(&transmitterTimer, &now);
        std::cout << "Transmitter Hz: " << (float)TRANSMITTER_COUNT/(msecsElapsed/1000.0) << "\n";
        //memcpy(&transmitterTimer, &now, sizeof(timeval));
        transmitterTimer = now; 
    }
}

void Hand::render()
{
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);
    glRotatef(yaw, 0, 1, 0);
    glRotatef(pitch, 1, 0, 0);
    glRotatef(roll, 0, 0, 1);
    glColor3f(color.x, color.y, color.z);
    glScalef(scale.x, scale.y, scale.z);
    //glutSolidSphere(1.5, 20, 20);
    glutSolidCube(1.0);
    glPopMatrix();
}

void Hand::simulate(float deltaTime)
{
    const float VNOISE = 0.01;
    const float RSPRING = 0.01;
    const float LINEAR_SPRING_CONSTANT = 500;
    const float LINEAR_DAMPING_COEFFICIENT = 2.0*powf(LINEAR_SPRING_CONSTANT,0.5);
    const float RNOISE = 0.1;

    //  If noise, add a bit of random velocity
    if (noise) {
        glm::vec3 nVel(randFloat() - 0.5f, randFloat() - 0.5f, randFloat() - 0.5f);
        nVel *= VNOISE;
        addVelocity(nVel);
        
        addAngularVelocity(RNOISE*(randFloat() - 0.5f),
                           RNOISE*(randFloat() - 0.5f),
                           RNOISE*(randFloat() - 0.5f));        
    }
    position += velocity*deltaTime;
    
    pitch += pitchRate;
    yaw += yawRate;
    roll += rollRate;
    
    //  Use a spring to attempt to return the hand to the target position
    glm::vec3 springForce = target - position;
    springForce *= LINEAR_SPRING_CONSTANT;
    addVelocity(springForce * deltaTime);
    
    //  Critically damp the spring
    glm::vec3 dampingForce(velocity);
    dampingForce *= LINEAR_DAMPING_COEFFICIENT;
    addVelocity(-dampingForce * deltaTime);
    
    //  Decay Angular Velocity
    pitchRate *= 1.0 - deltaTime;
    yawRate *= 1.0 - deltaTime;
    rollRate *= 1.0 - deltaTime;
    
    //  Add spring effect to return hand rotation to zero
    pitchRate -= pitch * RSPRING;
    yawRate -= yaw * RSPRING;
    rollRate -= roll * RSPRING;
    
}