//
//  main.cpp
//  Animation Server
//
//  Created by Brad Hefta-Gaub on 05/16/2013
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <iostream>

#include <EnvironmentData.h>
#include <NodeList.h>
#include <NodeTypes.h>
#include <OctalCode.h>
#include <PacketHeaders.h>
#include <SceneUtils.h>
#include <SharedUtil.h>
#include <VoxelTree.h>

#ifdef _WIN32
#include "Syssocket.h"
#include "Systime.h"
#else
#include <sys/time.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#endif

bool shouldShowPacketsPerSecond = false; // do we want to debug packets per second
bool includeBillboard = true; 
bool includeBorderTracer = true;
bool includeMovingBug = true;
bool includeBlinkingVoxel = false;
bool includeDanceFloor = true;


const int ANIMATION_LISTEN_PORT = 40107;
const int ACTUAL_FPS = 60;
const double OUR_FPS_IN_MILLISECONDS = 1000.0/ACTUAL_FPS; // determines FPS from our desired FPS
const int ANIMATE_VOXELS_INTERVAL_USECS = OUR_FPS_IN_MILLISECONDS * 1000.0; // converts from milliseconds to usecs

bool wantLocalDomain = false;

unsigned long packetsSent = 0;
unsigned long bytesSent = 0;

static void sendVoxelEditMessage(PACKET_HEADER header, VoxelDetail& detail) {
    unsigned char* bufferOut;
    int sizeOut;
    
    if (createVoxelEditMessage(header, 0, 1, &detail, bufferOut, sizeOut)){

        ::packetsSent++;
        ::bytesSent += sizeOut;

        if (::shouldShowPacketsPerSecond) {
            printf("sending packet of size=%d\n",sizeOut);
        }

        NodeList::getInstance()->broadcastToNodes(bufferOut, sizeOut, &NODE_TYPE_VOXEL_SERVER, 1);
        delete[] bufferOut;
    }
}

glm::vec3 rotatePoint(glm::vec3 point, float angle) {
    //  First, create the quaternion based on this angle of rotation
    glm::quat q(glm::vec3(0, -angle, 0));

    //  Next, create a rotation matrix from that quaternion
    glm::mat4 rotation = glm::mat4_cast(q);
    
    //  Transform the original vectors by the rotation matrix to get the new vectors
    glm::vec4 quatPoint(point.x, point.y, point.z, 0);
    glm::vec4 newPoint = quatPoint * rotation;
    
    return glm::vec3(newPoint.x, newPoint.y, newPoint.z);
}


const float BUG_VOXEL_SIZE = 0.0625f / TREE_SCALE; 
glm::vec3 bugPosition  = glm::vec3(BUG_VOXEL_SIZE * 20.0, BUG_VOXEL_SIZE * 30.0, BUG_VOXEL_SIZE * 20.0);
glm::vec3 bugDirection = glm::vec3(0, 0, 1);
const int VOXELS_PER_BUG = 18;
glm::vec3 bugPathCenter = glm::vec3(BUG_VOXEL_SIZE * 150.0, BUG_VOXEL_SIZE * 30.0, BUG_VOXEL_SIZE * 150.0);
float bugPathRadius = BUG_VOXEL_SIZE * 140.0;
float bugPathTheta = 0.0 * PI_OVER_180;
float bugRotation = 0.0 * PI_OVER_180;
float bugAngleDelta = 0.2 * PI_OVER_180;
bool moveBugInLine = false;

class BugPart {
public:
    glm::vec3       partLocation;
    unsigned char   partColor[3];
    
    BugPart(const glm::vec3& location, unsigned char red, unsigned char green, unsigned char blue ) {
        partLocation = location;
        partColor[0] = red;
        partColor[1] = green;
        partColor[2] = blue;
    }
};

const BugPart bugParts[VOXELS_PER_BUG] = {

    // tail
    BugPart(glm::vec3( 0, 0, -3), 51, 51, 153) ,
    BugPart(glm::vec3( 0, 0, -2), 51, 51, 153) ,
    BugPart(glm::vec3( 0, 0, -1), 51, 51, 153) ,

    // body
    BugPart(glm::vec3( 0, 0,  0), 255, 200, 0) ,
    BugPart(glm::vec3( 0, 0,  1), 255, 200, 0) ,

    // head
    BugPart(glm::vec3( 0, 0,  2), 200, 0, 0) ,

    // eyes
    BugPart(glm::vec3( 1, 0,  3), 64, 64, 64) ,
    BugPart(glm::vec3(-1, 0,  3), 64, 64, 64) ,

    // wings
    BugPart(glm::vec3( 3, 1,  1), 0, 153, 0) ,
    BugPart(glm::vec3( 2, 1,  1), 0, 153, 0) ,
    BugPart(glm::vec3( 1, 0,  1), 0, 153, 0) ,
    BugPart(glm::vec3(-1, 0,  1), 0, 153, 0) ,
    BugPart(glm::vec3(-2, 1,  1), 0, 153, 0) ,
    BugPart(glm::vec3(-3, 1,  1), 0, 153, 0) ,


    BugPart(glm::vec3( 2, -1,  0), 153, 200, 0) ,
    BugPart(glm::vec3( 1, -1,  0), 153, 200, 0) ,
    BugPart(glm::vec3(-1, -1,  0), 153, 200, 0) ,
    BugPart(glm::vec3(-2, -1,  0), 153, 200, 0) ,
};

static void renderMovingBug() {
    VoxelDetail details[VOXELS_PER_BUG];
    unsigned char* bufferOut;
    int sizeOut;
    
    // Generate voxels for where bug used to be
    for (int i = 0; i < VOXELS_PER_BUG; i++) {
        details[i].s = BUG_VOXEL_SIZE;
        
        glm::vec3 partAt = bugParts[i].partLocation * BUG_VOXEL_SIZE * (bugDirection.x < 0 ? -1.0f : 1.0f);
        glm::vec3 rotatedPartAt = rotatePoint(partAt, bugRotation);
        glm::vec3 offsetPartAt = rotatedPartAt + bugPosition;

        details[i].x = offsetPartAt.x;
        details[i].y = offsetPartAt.y;
        details[i].z = offsetPartAt.z;
        
        details[i].red   = bugParts[i].partColor[0];
        details[i].green = bugParts[i].partColor[1];
        details[i].blue  = bugParts[i].partColor[2];
    }
    
    // send the "erase message" first...
    PACKET_HEADER message = PACKET_HEADER_ERASE_VOXEL;
    if (createVoxelEditMessage(message, 0, VOXELS_PER_BUG, (VoxelDetail*)&details, bufferOut, sizeOut)){

        ::packetsSent++;
        ::bytesSent += sizeOut;

        if (::shouldShowPacketsPerSecond) {
            printf("sending packet of size=%d\n", sizeOut);
        }
        NodeList::getInstance()->broadcastToNodes(bufferOut, sizeOut, &NODE_TYPE_VOXEL_SERVER, 1);
        delete[] bufferOut;
    }

    // Move the bug...
    if (moveBugInLine) {
        bugPosition.x += (bugDirection.x * BUG_VOXEL_SIZE);
        bugPosition.y += (bugDirection.y * BUG_VOXEL_SIZE);
        bugPosition.z += (bugDirection.z * BUG_VOXEL_SIZE);
    
        // Check boundaries
        if (bugPosition.z > 1.0) {
            bugDirection.z = -1;
        }
        if (bugPosition.z < BUG_VOXEL_SIZE) {
            bugDirection.z = 1;
        }
    } else {

        //printf("bugPathCenter=(%f,%f,%f)\n", bugPathCenter.x, bugPathCenter.y, bugPathCenter.z);

        bugPathTheta += bugAngleDelta; // move slightly
        bugRotation  -= bugAngleDelta; // rotate slightly
        
        // If we loop past end of circle, just reset back into normal range
        if (bugPathTheta > (360.0f * PI_OVER_180)) {
            bugPathTheta = 0;
            bugRotation  = 0;
        }

        float x = bugPathCenter.x + bugPathRadius * cos(bugPathTheta);
        float z = bugPathCenter.z + bugPathRadius * sin(bugPathTheta);
        float y = bugPathCenter.y;
        
        bugPosition = glm::vec3(x, y, z);
        //printf("bugPathTheta=%f\n", bugPathTheta);
        //printf("bugRotation=%f\n", bugRotation);
    }
    
    //printf("bugPosition=(%f,%f,%f)\n", bugPosition.x, bugPosition.y, bugPosition.z);
    //printf("bugDirection=(%f,%f,%f)\n", bugDirection.x, bugDirection.y, bugDirection.z);
    // would be nice to add some randomness here...

    // Generate voxels for where bug is going to
    for (int i = 0; i < VOXELS_PER_BUG; i++) {
        details[i].s = BUG_VOXEL_SIZE;
        
        glm::vec3 partAt = bugParts[i].partLocation * BUG_VOXEL_SIZE * (bugDirection.x < 0 ? -1.0f : 1.0f);
        glm::vec3 rotatedPartAt = rotatePoint(partAt, bugRotation);
        glm::vec3 offsetPartAt = rotatedPartAt + bugPosition;

        details[i].x = offsetPartAt.x;
        details[i].y = offsetPartAt.y;
        details[i].z = offsetPartAt.z;
        
        details[i].red   = bugParts[i].partColor[0];
        details[i].green = bugParts[i].partColor[1];
        details[i].blue  = bugParts[i].partColor[2];
    }
    
    // send the "create message" ...
    message = PACKET_HEADER_SET_VOXEL_DESTRUCTIVE;
    if (createVoxelEditMessage(message, 0, VOXELS_PER_BUG, (VoxelDetail*)&details, bufferOut, sizeOut)){

        ::packetsSent++;
        ::bytesSent += sizeOut;

        if (::shouldShowPacketsPerSecond) {
            printf("sending packet of size=%d\n", sizeOut);
        }
        NodeList::getInstance()->broadcastToNodes(bufferOut, sizeOut, &NODE_TYPE_VOXEL_SERVER, 1);
        delete[] bufferOut;
    }
}


float intensity = 0.5f;
float intensityIncrement = 0.1f;
const float MAX_INTENSITY = 1.0f;
const float MIN_INTENSITY = 0.5f;
const float BEACON_SIZE = 0.25f / TREE_SCALE; // approximately 1/4th meter

static void sendVoxelBlinkMessage() {
    VoxelDetail detail;
    detail.s = BEACON_SIZE;
    
    glm::vec3 position = glm::vec3(0, 0, detail.s);
    
    detail.x = detail.s * floor(position.x / detail.s);
    detail.y = detail.s * floor(position.y / detail.s);
    detail.z = detail.s * floor(position.z / detail.s);
    
    ::intensity += ::intensityIncrement;
    if (::intensity >= MAX_INTENSITY) {
        ::intensity = MAX_INTENSITY;
        ::intensityIncrement = -::intensityIncrement;
    }
    if (::intensity <= MIN_INTENSITY) {
        ::intensity = MIN_INTENSITY;
        ::intensityIncrement = -::intensityIncrement;
    }

    detail.red   = 255 * ::intensity;
    detail.green = 0   * ::intensity;
    detail.blue  = 0   * ::intensity;
    
    PACKET_HEADER message = PACKET_HEADER_SET_VOXEL_DESTRUCTIVE;

    sendVoxelEditMessage(message, detail);
}

bool stringOfLightsInitialized = false;
int currentLight = 0;
int lightMovementDirection = 1;
const int SEGMENT_COUNT = 4;
const int LIGHTS_PER_SEGMENT = 80;
const int LIGHT_COUNT = LIGHTS_PER_SEGMENT * SEGMENT_COUNT;
glm::vec3 stringOfLights[LIGHT_COUNT];
unsigned char offColor[3] = { 240, 240, 240 };
unsigned char onColor[3]  = {   0, 255, 255 };
const float STRING_OF_LIGHTS_SIZE = 0.125f / TREE_SCALE; // approximately 1/8th meter

static void sendBlinkingStringOfLights() {
    PACKET_HEADER message = PACKET_HEADER_SET_VOXEL_DESTRUCTIVE; // we're a bully!
    float lightScale = STRING_OF_LIGHTS_SIZE;
    static VoxelDetail details[LIGHTS_PER_SEGMENT];
    unsigned char* bufferOut;
    int sizeOut;

    // first initialized the string of lights if needed...
    if (!stringOfLightsInitialized) {
        for (int segment = 0; segment < SEGMENT_COUNT; segment++) {
            for (int indexInSegment = 0; indexInSegment < LIGHTS_PER_SEGMENT; indexInSegment++) {

                int i = (segment * LIGHTS_PER_SEGMENT) + indexInSegment;

                // four different segments on sides of initial platform
                switch (segment) {
                    case 0:
                        // along x axis
                        stringOfLights[i] = glm::vec3(indexInSegment * lightScale, 0, 0);
                        break;
                    case 1:
                        // parallel to Z axis at outer X edge
                        stringOfLights[i] = glm::vec3(LIGHTS_PER_SEGMENT * lightScale, 0, indexInSegment * lightScale);
                        break;
                    case 2:
                        // parallel to X axis at outer Z edge
                        stringOfLights[i] = glm::vec3((LIGHTS_PER_SEGMENT-indexInSegment) * lightScale, 0, 
                                                      LIGHTS_PER_SEGMENT * lightScale);
                        break;
                    case 3:
                        // on Z axis
                        stringOfLights[i] = glm::vec3(0, 0, (LIGHTS_PER_SEGMENT-indexInSegment) * lightScale);
                        break;
                }

                details[indexInSegment].s = STRING_OF_LIGHTS_SIZE;
                details[indexInSegment].x = stringOfLights[i].x;
                details[indexInSegment].y = stringOfLights[i].y;
                details[indexInSegment].z = stringOfLights[i].z;

                details[indexInSegment].red   = offColor[0];
                details[indexInSegment].green = offColor[1];
                details[indexInSegment].blue  = offColor[2];
            }

            // send entire segment at once
            if (createVoxelEditMessage(message, 0, LIGHTS_PER_SEGMENT, (VoxelDetail*)&details, bufferOut, sizeOut)){

                ::packetsSent++;
                ::bytesSent += sizeOut;

                if (::shouldShowPacketsPerSecond) {
                    printf("sending packet of size=%d\n",sizeOut);
                }
                NodeList::getInstance()->broadcastToNodes(bufferOut, sizeOut, &NODE_TYPE_VOXEL_SERVER, 1);
                delete[] bufferOut;
            }

        }
        stringOfLightsInitialized = true;
    } else {
        // turn off current light
        details[0].x = stringOfLights[currentLight].x;
        details[0].y = stringOfLights[currentLight].y;
        details[0].z = stringOfLights[currentLight].z;
        details[0].red   = offColor[0];
        details[0].green = offColor[1];
        details[0].blue  = offColor[2];

        // move current light...
        // if we're at the end of our string, then change direction
        if (currentLight == LIGHT_COUNT-1) {
            lightMovementDirection = -1;
        }
        if (currentLight == 0) {
            lightMovementDirection = 1;
        }
        currentLight += lightMovementDirection;
        
        // turn on new current light
        details[1].x = stringOfLights[currentLight].x;
        details[1].y = stringOfLights[currentLight].y;
        details[1].z = stringOfLights[currentLight].z;
        details[1].red   = onColor[0];
        details[1].green = onColor[1];
        details[1].blue  = onColor[2];

        // send both changes in same message
        if (createVoxelEditMessage(message, 0, 2, (VoxelDetail*)&details, bufferOut, sizeOut)){

            ::packetsSent++;
            ::bytesSent += sizeOut;

            if (::shouldShowPacketsPerSecond) {
                printf("sending packet of size=%d\n",sizeOut);
            }
            NodeList::getInstance()->broadcastToNodes(bufferOut, sizeOut, &NODE_TYPE_VOXEL_SERVER, 1);
            delete[] bufferOut;
        }
    }
}

bool danceFloorInitialized = false;
const float DANCE_FLOOR_LIGHT_SIZE = 1.0f / TREE_SCALE; // approximately 1 meter
const int DANCE_FLOOR_LENGTH = 10;
const int DANCE_FLOOR_WIDTH  = 10;
glm::vec3 danceFloorPosition(100.0f / TREE_SCALE, 30.0f / TREE_SCALE, 10.0f / TREE_SCALE); 
glm::vec3 danceFloorLights[DANCE_FLOOR_LENGTH][DANCE_FLOOR_WIDTH];
unsigned char danceFloorOffColor[3] = { 240, 240, 240 };
const int DANCE_FLOOR_COLORS = 6;

unsigned char danceFloorOnColorA[DANCE_FLOOR_COLORS][3] = { 
    { 255,   0,   0 }, {    0, 255,   0 }, {    0,   0, 255 },
    {   0, 191, 255 }, {    0, 250, 154 }, {  255,  69,   0 },
};
unsigned char danceFloorOnColorB[DANCE_FLOOR_COLORS][3] = { 
    {   0,   0,   0 }, {    0,   0,   0 }, {    0,   0,   0 }  ,
    {   0,   0,   0 }, {    0,   0,   0 }, {    0,   0,   0 }  
};
float danceFloorGradient = 0.5f;
const float BEATS_PER_MINUTE = 118.0f;
const float SECONDS_PER_MINUTE = 60.0f;
const float FRAMES_PER_BEAT = (SECONDS_PER_MINUTE * ACTUAL_FPS) / BEATS_PER_MINUTE;
float danceFloorGradientIncrement = 1.0f / FRAMES_PER_BEAT;
const float DANCE_FLOOR_MAX_GRADIENT = 1.0f;
const float DANCE_FLOOR_MIN_GRADIENT = 0.0f;
const int DANCE_FLOOR_VOXELS_PER_PACKET = 100;
const int PACKETS_PER_DANCE_FLOOR = DANCE_FLOOR_VOXELS_PER_PACKET / (DANCE_FLOOR_WIDTH * DANCE_FLOOR_LENGTH);
int danceFloorColors[DANCE_FLOOR_WIDTH][DANCE_FLOOR_LENGTH];

void sendDanceFloor() {
    PACKET_HEADER message = PACKET_HEADER_SET_VOXEL_DESTRUCTIVE; // we're a bully!
    float lightScale = DANCE_FLOOR_LIGHT_SIZE;
    static VoxelDetail details[DANCE_FLOOR_VOXELS_PER_PACKET];
    unsigned char* bufferOut;
    int sizeOut;

    // first initialized the billboard of lights if needed...
    if (!::danceFloorInitialized) {
        for (int i = 0; i < DANCE_FLOOR_WIDTH; i++) {
            for (int j = 0; j < DANCE_FLOOR_LENGTH; j++) {

                int randomColorIndex = randIntInRange( -(DANCE_FLOOR_COLORS), (DANCE_FLOOR_COLORS + 1));
                ::danceFloorColors[i][j] = randomColorIndex;
                ::danceFloorLights[i][j] = ::danceFloorPosition + 
                                         glm::vec3(i * DANCE_FLOOR_LIGHT_SIZE, 0, j * DANCE_FLOOR_LIGHT_SIZE);
            }
        }
        ::danceFloorInitialized = true;
    }

    ::danceFloorGradient += ::danceFloorGradientIncrement;
    
    if (::danceFloorGradient >= DANCE_FLOOR_MAX_GRADIENT) {
        ::danceFloorGradient = DANCE_FLOOR_MAX_GRADIENT;
        ::danceFloorGradientIncrement = -::danceFloorGradientIncrement;
    }
    if (::danceFloorGradient <= DANCE_FLOOR_MIN_GRADIENT) {
        ::danceFloorGradient = DANCE_FLOOR_MIN_GRADIENT;
        ::danceFloorGradientIncrement = -::danceFloorGradientIncrement;
    }

    for (int i = 0; i < DANCE_FLOOR_LENGTH; i++) {
        for (int j = 0; j < DANCE_FLOOR_WIDTH; j++) {

            int nthVoxel = ((i * DANCE_FLOOR_WIDTH) + j);
            int item = nthVoxel % DANCE_FLOOR_VOXELS_PER_PACKET;
            
            ::danceFloorLights[i][j] = ::danceFloorPosition + 
                                        glm::vec3(i * DANCE_FLOOR_LIGHT_SIZE, 0, j * DANCE_FLOOR_LIGHT_SIZE);

            details[item].s = lightScale;
            details[item].x = ::danceFloorLights[i][j].x;
            details[item].y = ::danceFloorLights[i][j].y;
            details[item].z = ::danceFloorLights[i][j].z;

            if (danceFloorColors[i][j] > 0) {
                int color = danceFloorColors[i][j] - 1;
                details[item].red   = (::danceFloorOnColorA[color][0] + 
                                        ((::danceFloorOnColorB[color][0] - ::danceFloorOnColorA[color][0]) 
                                         * ::danceFloorGradient));
                details[item].green = (::danceFloorOnColorA[color][1] + 
                                        ((::danceFloorOnColorB[color][1] - ::danceFloorOnColorA[color][1]) 
                                         * ::danceFloorGradient));
                details[item].blue  = (::danceFloorOnColorA[color][2] + 
                                        ((::danceFloorOnColorB[color][2] - ::danceFloorOnColorA[color][2]) 
                                         * ::danceFloorGradient));
            } else if (::danceFloorColors[i][j] < 0) {
                int color = -(::danceFloorColors[i][j] + 1);
                details[item].red   = (::danceFloorOnColorB[color][0] + 
                                        ((::danceFloorOnColorA[color][0] - ::danceFloorOnColorB[color][0]) 
                                         * ::danceFloorGradient));
                details[item].green = (::danceFloorOnColorB[color][1] + 
                                        ((::danceFloorOnColorA[color][1] - ::danceFloorOnColorB[color][1]) 
                                         * ::danceFloorGradient));
                details[item].blue  = (::danceFloorOnColorB[color][2] + 
                                        ((::danceFloorOnColorA[color][2] - ::danceFloorOnColorB[color][2]) 
                                         * ::danceFloorGradient));
            } else {
                int color = 0;
                details[item].red   = (::danceFloorOnColorB[color][0] + 
                                        ((::danceFloorOnColorA[color][0] - ::danceFloorOnColorB[color][0]) 
                                         * ::danceFloorGradient));
                details[item].green = (::danceFloorOnColorB[color][1] + 
                                        ((::danceFloorOnColorA[color][1] - ::danceFloorOnColorB[color][1]) 
                                         * ::danceFloorGradient));
                details[item].blue  = (::danceFloorOnColorB[color][2] + 
                                        ((::danceFloorOnColorA[color][2] - ::danceFloorOnColorB[color][2]) 
                                         * ::danceFloorGradient));
            }
            
            if (item == DANCE_FLOOR_VOXELS_PER_PACKET - 1) {
                if (createVoxelEditMessage(message, 0, DANCE_FLOOR_VOXELS_PER_PACKET, 
                                            (VoxelDetail*)&details, bufferOut, sizeOut)){
                    ::packetsSent++;
                    ::bytesSent += sizeOut;
                    if (::shouldShowPacketsPerSecond) {
                        printf("sending packet of size=%d\n", sizeOut);
                    }
                    NodeList::getInstance()->broadcastToNodes(bufferOut, sizeOut, &NODE_TYPE_VOXEL_SERVER, 1);
                    delete[] bufferOut;
                }
            }
        }
    }
}

bool billboardInitialized = false;
const int BILLBOARD_HEIGHT = 9;
const int BILLBOARD_WIDTH  = 45;
glm::vec3 billboardPosition((0.125f / TREE_SCALE),(0.125f / TREE_SCALE),0);
glm::vec3 billboardLights[BILLBOARD_HEIGHT][BILLBOARD_WIDTH];
unsigned char billboardOffColor[3] = { 240, 240, 240 };
unsigned char billboardOnColorA[3]  = {   0,   0, 255 };
unsigned char billboardOnColorB[3]  = {   0, 255,   0 };
float billboardGradient = 0.5f;
float billboardGradientIncrement = 0.01f;
const float BILLBOARD_MAX_GRADIENT = 1.0f;
const float BILLBOARD_MIN_GRADIENT = 0.0f;
const float BILLBOARD_LIGHT_SIZE   = 0.125f / TREE_SCALE; // approximately 1/8 meter per light
const int VOXELS_PER_PACKET = 81;
const int PACKETS_PER_BILLBOARD = VOXELS_PER_PACKET / (BILLBOARD_HEIGHT * BILLBOARD_WIDTH);


// top to bottom... 
bool billboardMessage[BILLBOARD_HEIGHT][BILLBOARD_WIDTH] = {
 { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
 { 0,0,1,0,0,1,0,1,0,0,0,0,0,1,0,0,0,0,1,1,1,1,0,0,0,0,0,1,0,1,1,1,0,1,0,1,0,0,1,0,0,0,0,0,0 },
 { 0,0,1,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,0,1,0,1,0,1,0,0,0,1,1,1,0,0,0,0,0 },
 { 0,0,1,1,1,1,0,1,0,1,1,1,0,1,1,1,0,0,1,1,1,0,0,0,0,1,1,1,0,1,1,1,0,1,0,1,0,0,1,0,0,1,0,1,0 },
 { 0,0,1,0,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1,0,0,0,0,1,0,1,0,1,0,1,0,0,0,1,0,1,0,0,1,0,0,1,0,1,0 },
 { 0,0,1,0,0,1,0,1,0,1,1,1,0,1,0,1,0,0,1,0,0,0,0,1,0,1,1,1,0,1,1,1,0,1,0,1,0,0,1,0,0,1,1,1,0 },
 { 0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0 },
 { 0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0 },
 { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }
};

static void sendBillboard() {
    PACKET_HEADER message = PACKET_HEADER_SET_VOXEL_DESTRUCTIVE; // we're a bully!
    float lightScale = BILLBOARD_LIGHT_SIZE;
    static VoxelDetail details[VOXELS_PER_PACKET];
    unsigned char* bufferOut;
    int sizeOut;

    // first initialized the billboard of lights if needed...
    if (!billboardInitialized) {
        for (int i = 0; i < BILLBOARD_HEIGHT; i++) {
            for (int j = 0; j < BILLBOARD_WIDTH; j++) {
                
                billboardLights[i][j] = billboardPosition + glm::vec3(j * lightScale, (float)((BILLBOARD_HEIGHT - i) * lightScale), 0);
            }
        }
        billboardInitialized = true;
    }

    ::billboardGradient += ::billboardGradientIncrement;
    
    if (::billboardGradient >= BILLBOARD_MAX_GRADIENT) {
        ::billboardGradient = BILLBOARD_MAX_GRADIENT;
        ::billboardGradientIncrement = -::billboardGradientIncrement;
    }
    if (::billboardGradient <= BILLBOARD_MIN_GRADIENT) {
        ::billboardGradient = BILLBOARD_MIN_GRADIENT;
        ::billboardGradientIncrement = -::billboardGradientIncrement;
    }

    for (int i = 0; i < BILLBOARD_HEIGHT; i++) {
        for (int j = 0; j < BILLBOARD_WIDTH; j++) {

            int nthVoxel = ((i * BILLBOARD_WIDTH) + j);
            int item = nthVoxel % VOXELS_PER_PACKET;
            
            billboardLights[i][j] = billboardPosition + glm::vec3(j * lightScale, (float)((BILLBOARD_HEIGHT - i) * lightScale), 0);

            details[item].s = lightScale;
            details[item].x = billboardLights[i][j].x;
            details[item].y = billboardLights[i][j].y;
            details[item].z = billboardLights[i][j].z;

            if (billboardMessage[i][j]) {
                details[item].red   = (billboardOnColorA[0] + ((billboardOnColorB[0] - billboardOnColorA[0]) * ::billboardGradient));
                details[item].green = (billboardOnColorA[1] + ((billboardOnColorB[1] - billboardOnColorA[1]) * ::billboardGradient));
                details[item].blue  = (billboardOnColorA[2] + ((billboardOnColorB[2] - billboardOnColorA[2]) * ::billboardGradient));
            } else {
                details[item].red   = billboardOffColor[0];
                details[item].green = billboardOffColor[1];
                details[item].blue  = billboardOffColor[2];
            }
            
            if (item == VOXELS_PER_PACKET - 1) {
                if (createVoxelEditMessage(message, 0, VOXELS_PER_PACKET, (VoxelDetail*)&details, bufferOut, sizeOut)){
                    ::packetsSent++;
                    ::bytesSent += sizeOut;
                    if (::shouldShowPacketsPerSecond) {
                        printf("sending packet of size=%d\n", sizeOut);
                    }
                    NodeList::getInstance()->broadcastToNodes(bufferOut, sizeOut, &NODE_TYPE_VOXEL_SERVER, 1);
                    delete[] bufferOut;
                }
            }
        }
    }
}

double start = 0;


void* animateVoxels(void* args) {
    
    timeval lastSendTime;
    
    while (true) {
        gettimeofday(&lastSendTime, NULL);

        // some animations
        //sendVoxelBlinkMessage();

        if (::includeBillboard) {
            sendBillboard();
        }
        if (::includeBorderTracer) {
            sendBlinkingStringOfLights();
        }
        if (::includeMovingBug) {
            renderMovingBug();
        }
        if (::includeBlinkingVoxel) {
            sendVoxelBlinkMessage();
        }
        if (::includeDanceFloor) {
            sendDanceFloor();
        }
        
        uint64_t end = usecTimestampNow();
        uint64_t elapsedSeconds = (end - ::start) / 1000000;
        if (::shouldShowPacketsPerSecond) {
            printf("packetsSent=%ld, bytesSent=%ld pps=%f bps=%f\n",packetsSent,bytesSent,
                (float)(packetsSent/elapsedSeconds),(float)(bytesSent/elapsedSeconds));
        }
        // dynamically sleep until we need to fire off the next set of voxels
        uint64_t usecToSleep =  ANIMATE_VOXELS_INTERVAL_USECS - (usecTimestampNow() - usecTimestamp(&lastSendTime));
        
        if (usecToSleep > 0) {
            usleep(usecToSleep);
        } else {
            std::cout << "Last send took too much time, not sleeping!\n";
        }
    }
    
    pthread_exit(0);
}


int main(int argc, const char * argv[])
{
    ::start = usecTimestampNow();

    NodeList* nodeList = NodeList::createInstance(NODE_TYPE_ANIMATION_SERVER, ANIMATION_LISTEN_PORT);
    setvbuf(stdout, NULL, _IOLBF, 0);

    // Handle Local Domain testing with the --local command line
    const char* NO_BILLBOARD = "--NoBillboard";
    ::includeBillboard = !cmdOptionExists(argc, argv, NO_BILLBOARD);

    const char* NO_BORDER_TRACER = "--NoBorderTracer";
    ::includeBorderTracer = !cmdOptionExists(argc, argv, NO_BORDER_TRACER);

    const char* NO_MOVING_BUG = "--NoMovingBug";
    ::includeMovingBug = !cmdOptionExists(argc, argv, NO_MOVING_BUG);

    const char* INCLUDE_BLINKING_VOXEL = "--includeBlinkingVoxel";
    ::includeBlinkingVoxel = cmdOptionExists(argc, argv, INCLUDE_BLINKING_VOXEL);

    const char* NO_DANCE_FLOOR = "--NoDanceFloor";
    ::includeDanceFloor = !cmdOptionExists(argc, argv, NO_DANCE_FLOOR);

    // Handle Local Domain testing with the --local command line
    const char* showPPS = "--showPPS";
    ::shouldShowPacketsPerSecond = cmdOptionExists(argc, argv, showPPS);

    // Handle Local Domain testing with the --local command line
    const char* local = "--local";
    ::wantLocalDomain = cmdOptionExists(argc, argv,local);
    if (::wantLocalDomain) {
        printf("Local Domain MODE!\n");
        int ip = getLocalAddress();
        sprintf(DOMAIN_IP,"%d.%d.%d.%d", (ip & 0xFF), ((ip >> 8) & 0xFF),((ip >> 16) & 0xFF), ((ip >> 24) & 0xFF));
    }

    nodeList->linkedDataCreateCallback = NULL; // do we need a callback?
    nodeList->startSilentNodeRemovalThread();
    
    srand((unsigned)time(0));

    pthread_t animateVoxelThread;
    pthread_create(&animateVoxelThread, NULL, animateVoxels, NULL);

    sockaddr nodePublicAddress;
    
    unsigned char* packetData = new unsigned char[MAX_PACKET_SIZE];
    ssize_t receivedBytes;
    
    timeval lastDomainServerCheckIn = {};
    NodeList::getInstance()->setNodeTypesOfInterest(&NODE_TYPE_VOXEL_SERVER, 1);

    // loop to send to nodes requesting data
    while (true) {
        // send a check in packet to the domain server if DOMAIN_SERVER_CHECK_IN_USECS has elapsed
        if (usecTimestampNow() - usecTimestamp(&lastDomainServerCheckIn) >= DOMAIN_SERVER_CHECK_IN_USECS) {
            gettimeofday(&lastDomainServerCheckIn, NULL);
            NodeList::getInstance()->sendDomainServerCheckIn();
        }
        
        // Nodes sending messages to us...
        if (nodeList->getNodeSocket()->receive(&nodePublicAddress, packetData, &receivedBytes)) {
            NodeList::getInstance()->processNodeData(&nodePublicAddress, packetData, receivedBytes);
        }
    }
    
    pthread_join(animateVoxelThread, NULL);

    return 0;
}
