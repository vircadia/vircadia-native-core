//
//  Hand.cpp
//  interface
//
//  Created by Philip Rosedale on 10/13/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include "Hand.h"

#ifdef _WIN32
#include "Systime.h"
#else
#include <sys/time.h>
#endif

const float PHI = 1.618f;

const float DEFAULT_X = 0;
const float DEFAULT_Y = -1.5;
const float DEFAULT_Z = 2.0;
const float DEFAULT_TRANSMITTER_HZ = 60.0;

Hand::Hand(glm::vec3 initcolor)
{
    color = initcolor;
    reset();
    noise = 0.0;  //0.2;
    scale.x = 0.07f;
    scale.y = scale.x * 5.0f;
    scale.z = scale.y * 1.0f;
    renderPointer = true;
}

Hand::Hand(const Hand &otherHand) {
    color = otherHand.color;
    noise = otherHand.noise;
    scale = otherHand.scale;
    position = otherHand.position;
    target = otherHand.target;
    velocity = otherHand.color;
    pitch = otherHand.pitch;
    yaw = otherHand.yaw;
    roll = otherHand.roll;
    pitchRate = otherHand.pitchRate;
    yawRate = otherHand.yawRate;
    rollRate = otherHand.rollRate;
    transmitterTimer = otherHand.transmitterTimer;
    transmitterHz = otherHand.transmitterHz;
    transmitterPackets = otherHand.transmitterPackets;
    renderPointer = otherHand.renderPointer;
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
    transmitterHz = DEFAULT_TRANSMITTER_HZ;
}

void Hand::render(int isMine)
{
    const float POINTER_LENGTH = 20.0;
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);
    glRotatef(yaw, 0, 1, 0);
    glRotatef(pitch, 1, 0, 0);
    glRotatef(roll, 0, 0, 1);
    glColor3f(color.x, color.y, color.z);
    glScalef(scale.x, scale.y, scale.z);
    //glutSolidSphere(1.5, 20, 20);
    glutSolidCube(1.0);
    if (renderPointer) {
        glBegin(GL_TRIANGLES);
        glColor3f(1,0,0);
        glNormal3f(0,-1,0);
        glVertex3f(-0.4f,0,0);
        glVertex3f(0.4f,0,0);
        glVertex3f(0,0,-POINTER_LENGTH);
        glEnd();
        glPushMatrix();
        glTranslatef(0,0,-POINTER_LENGTH);
        glutSolidCube(1.0);
        glPopMatrix();
    }
    glPopMatrix();
    
    if (1) {
        //  Render debug info from the transmitter
        /*
        glPushMatrix();
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        //gluOrtho2D(0, WIDTH, HEIGHT, 0);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        glPopMatrix();
         */

    }
    
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
        transmitterHz = static_cast<float>( (double)TRANSMITTER_COUNT/(msecsElapsed/1000.0) );
        //std::cout << "Transmitter Hz: " << (float)TRANSMITTER_COUNT/(msecsElapsed/1000.0) << "\n";
        //memcpy(&transmitterTimer, &now, sizeof(timeval));
        transmitterTimer = now; 
    }
    //  Add rotational forces to the hand
    const float ANG_VEL_SENSITIVITY = 4.0;
    const float ANG_VEL_THRESHOLD = 0.0;
    float angVelScale = ANG_VEL_SENSITIVITY*(1.0f/getTransmitterHz());
    //addAngularVelocity(gyrX*angVelScale,gyrZ*angVelScale,-gyrY*angVelScale);
    addAngularVelocity(fabs(gyrX*angVelScale)>ANG_VEL_THRESHOLD?gyrX*angVelScale:0,
                       fabs(gyrZ*angVelScale)>ANG_VEL_THRESHOLD?gyrZ*angVelScale:0,
                       fabs(-gyrY*angVelScale)>ANG_VEL_THRESHOLD?-gyrY*angVelScale:0);
    
    //  Add linear forces to the hand
    //const float LINEAR_VEL_SENSITIVITY = 50.0;
    const float LINEAR_VEL_SENSITIVITY = 5.0;
    float linVelScale = LINEAR_VEL_SENSITIVITY*(1.0f/getTransmitterHz());
    glm::vec3 linVel(linX*linVelScale, linZ*linVelScale, -linY*linVelScale);
    addVelocity(linVel);
    
}

void Hand::simulate(float deltaTime)
{
    const float ANGULAR_SPRING_CONSTANT = 0.25;
    const float ANGULAR_DAMPING_COEFFICIENT = 5*2.0f*powf(ANGULAR_SPRING_CONSTANT,0.5f);
    const float LINEAR_SPRING_CONSTANT = 100;
    const float LINEAR_DAMPING_COEFFICIENT = 2.0f*powf(LINEAR_SPRING_CONSTANT,0.5f);
    
    //  If noise, add a bit of random velocity
    const float RNOISE = 0.0;
    const float VNOISE = 0.01f;
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
    
    //  The spring method
    if (0) {
    //  Use a linear spring to attempt to return the hand to the target position
    glm::vec3 springForce = target - position;
    springForce *= LINEAR_SPRING_CONSTANT;
    addVelocity(springForce * deltaTime);
    
    //  Critically damp the linear spring
    glm::vec3 dampingForce(velocity);
    dampingForce *= LINEAR_DAMPING_COEFFICIENT;
    addVelocity(-dampingForce * deltaTime);
    
    // Use angular spring to return hand to target rotation (0,0,0)
    addAngularVelocity(-pitch * ANGULAR_SPRING_CONSTANT * deltaTime,
                       -yaw * ANGULAR_SPRING_CONSTANT * deltaTime,
                       -roll * ANGULAR_SPRING_CONSTANT * deltaTime);

    //  Critically damp angular spring
    addAngularVelocity(-pitchRate*ANGULAR_DAMPING_COEFFICIENT*deltaTime,
                       -yawRate*ANGULAR_DAMPING_COEFFICIENT*deltaTime,
                       -rollRate*ANGULAR_DAMPING_COEFFICIENT*deltaTime);
    }
    
    //  The absolute limits method  (no springs)
    if (1) {
        //  Limit rotation 
        const float YAW_LIMIT = 20;
        const float PITCH_LIMIT = 20;
        
        if (yaw > YAW_LIMIT) { yaw = YAW_LIMIT; yawRate = 0.0; }
        if (yaw < -YAW_LIMIT) { yaw = -YAW_LIMIT; yawRate = 0.0; }
        if (pitch > PITCH_LIMIT) { pitch = PITCH_LIMIT; pitchRate = 0.0; }
        if (pitch < -PITCH_LIMIT) { pitch = -PITCH_LIMIT; pitchRate = 0.0; }
        
        //  Damp Rotation Rates
        yawRate *= 0.99f;
        pitchRate *= 0.99f;
        rollRate *= 0.99f;
        
        //  Limit position
        const float X_LIMIT = 1.0;
        const float Y_LIMIT = 1.0;
        const float Z_LIMIT = 1.0;
        
        if (position.x > DEFAULT_X + X_LIMIT) { position.x = DEFAULT_X + X_LIMIT; velocity.x = 0; }
        if (position.x < DEFAULT_X - X_LIMIT) { position.x = DEFAULT_X - X_LIMIT; velocity.x = 0; }
        if (position.y > DEFAULT_Y + Y_LIMIT) { position.y = DEFAULT_Y + Y_LIMIT; velocity.y = 0; }
        if (position.y < DEFAULT_Y - Y_LIMIT) { position.y = DEFAULT_Y - Y_LIMIT; velocity.y = 0; }
        if (position.z > DEFAULT_Z + Z_LIMIT) { position.z = DEFAULT_Z + Z_LIMIT; velocity.z = 0; }
        if (position.z < DEFAULT_Z - Z_LIMIT) { position.z = DEFAULT_Z - Z_LIMIT; velocity.z = 0; }
  
        //  Damp Velocity
        velocity *= 0.99;
        
    }
    
}