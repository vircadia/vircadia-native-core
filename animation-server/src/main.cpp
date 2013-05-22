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
#include <OctalCode.h>
#include <AgentList.h>
#include <AgentTypes.h>
#include <EnvironmentData.h>
#include <VoxelTree.h>
#include <SharedUtil.h>
#include <PacketHeaders.h>
#include <SceneUtils.h>

#ifdef _WIN32
#include "Syssocket.h"
#include "Systime.h"
#else
#include <sys/time.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#endif

bool shouldShowPacketsPerSecond = false; // do we want to debug packets per second

const int ANIMATION_LISTEN_PORT = 40107;
const int ACTUAL_FPS = 60;
const double OUR_FPS_IN_MILLISECONDS = 1000.0/ACTUAL_FPS; // determines FPS from our desired FPS
const int ANIMATE_VOXELS_INTERVAL_USECS = OUR_FPS_IN_MILLISECONDS * 1000.0; // converts from milliseconds to usecs

bool wantLocalDomain = false;

static void sendVoxelServerZMessage() {
    char message[100];
    sprintf(message,"%c%s",'Z',"a message");
    int messageSize = strlen(message) + 1;
    AgentList::getInstance()->broadcastToAgents((unsigned char*) message, messageSize, &AGENT_TYPE_VOXEL, 1);
}

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

        AgentList::getInstance()->broadcastToAgents(bufferOut, sizeOut, &AGENT_TYPE_VOXEL, 1);
        delete[] bufferOut;
    }
}


const float BUG_VOXEL_SIZE = 0.0625f / TREE_SCALE;
glm::vec3 bugPosition  = glm::vec3(BUG_VOXEL_SIZE*10.0,0,BUG_VOXEL_SIZE*10.0); ///BUG_VOXEL_SIZE * 10.f, BUG_VOXEL_SIZE * 10.f, BUG_VOXEL_SIZE * 10.f);
glm::vec3 bugDirection = glm::vec3(0,0,1);


const unsigned char bugColor[3] = { 0, 255, 255};

const int VOXELS_PER_BUG = 14;
const glm::vec3 bugParts[VOXELS_PER_BUG] = {

    // body
    glm::vec3(0,0,-3), 
    glm::vec3(0,0,-2), 
    glm::vec3(0,0,-1), 
    glm::vec3(0,0,0), 
    glm::vec3(0,0,1), 
    glm::vec3(0,0,2), 
    
    // eyes
    glm::vec3(1,0,3), 
    glm::vec3(-1,0,3), 

    // wings
    glm::vec3(1,0,1), 
    glm::vec3(2,0,1), 
    glm::vec3(3,0,1), 
    glm::vec3(-1,0,1), 
    glm::vec3(-2,0,1), 
    glm::vec3(-3,0,1), 
};

static void renderMovingBug() {
    VoxelDetail details[VOXELS_PER_BUG];
    PACKET_HEADER message = PACKET_HEADER_SET_VOXEL_DESTRUCTIVE;
    unsigned char* bufferOut;
    int sizeOut;
    
    // Generate voxels for where bug used to be
    for (int i = 0; i < VOXELS_PER_BUG; i++) {
        details[i].s = BUG_VOXEL_SIZE;
        details[i].x = bugPosition.x + (bugParts[i].x * BUG_VOXEL_SIZE);
        details[i].y = bugPosition.y + (bugParts[i].y * BUG_VOXEL_SIZE);
        details[i].z = bugPosition.z + (bugParts[i].z * BUG_VOXEL_SIZE);

        details[i].red   = bugColor[0];
        details[i].green = bugColor[1];
        details[i].blue  = bugColor[2];
    }
    
    // send the "erase message" first...
    message = PACKET_HEADER_ERASE_VOXEL;
    if (createVoxelEditMessage(message, 0, VOXELS_PER_BUG, (VoxelDetail*)&details, bufferOut, sizeOut)){

        ::packetsSent++;
        ::bytesSent += sizeOut;

        if (::shouldShowPacketsPerSecond) {
            printf("sending packet of size=%d\n",sizeOut);
        }
        AgentList::getInstance()->broadcastToAgents(bufferOut, sizeOut, &AGENT_TYPE_VOXEL, 1);
        delete[] bufferOut;
    }

    // Move the bug...
    bugPosition.x += (bugDirection.x * BUG_VOXEL_SIZE);
    bugPosition.y += (bugDirection.y * BUG_VOXEL_SIZE);
    bugPosition.z += (bugDirection.z * BUG_VOXEL_SIZE);
    
    // Check boundaries
    if (bugPosition.z > 0.25) {
        bugDirection.z = -1;
    }
    if (bugPosition.z < 0.01) {
        bugDirection.z = 1;
    }
    printf("bugPosition=(%f,%f,%f)\n",bugPosition.x,bugPosition.y,bugPosition.z);
    printf("bugDirection=(%f,%f,%f)\n",bugDirection.x,bugDirection.y,bugDirection.z);
    // would be nice to add some randomness here...

    // Generate voxels for where bug is going to
    for (int i = 0; i < VOXELS_PER_BUG; i++) {
        details[i].s = BUG_VOXEL_SIZE;
        details[i].x = bugPosition.x + (bugParts[i].x * BUG_VOXEL_SIZE);
        details[i].y = bugPosition.y + (bugParts[i].y * BUG_VOXEL_SIZE);
        details[i].z = bugPosition.z + (bugParts[i].z * BUG_VOXEL_SIZE);

        details[i].red   = bugColor[0];
        details[i].green = bugColor[1];
        details[i].blue  = bugColor[2];
    }
    
    // send the "create message" ...
    message = PACKET_HEADER_SET_VOXEL_DESTRUCTIVE;
    if (createVoxelEditMessage(message, 0, VOXELS_PER_BUG, (VoxelDetail*)&details, bufferOut, sizeOut)){

        ::packetsSent++;
        ::bytesSent += sizeOut;

        if (::shouldShowPacketsPerSecond) {
            printf("sending packet of size=%d\n",sizeOut);
        }
        AgentList::getInstance()->broadcastToAgents(bufferOut, sizeOut, &AGENT_TYPE_VOXEL, 1);
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
                AgentList::getInstance()->broadcastToAgents(bufferOut, sizeOut, &AGENT_TYPE_VOXEL, 1);
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
            AgentList::getInstance()->broadcastToAgents(bufferOut, sizeOut, &AGENT_TYPE_VOXEL, 1);
            delete[] bufferOut;
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
const int VOXELS_PER_PACKET = 135;
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
                    AgentList::getInstance()->broadcastToAgents(bufferOut, sizeOut, &AGENT_TYPE_VOXEL, 1);
                    delete[] bufferOut;
                }
            }
        }
    }
}

double start = 0;


void* animateVoxels(void* args) {
    
    AgentList* agentList = AgentList::getInstance();
    timeval lastSendTime;
    
    while (true) {
        gettimeofday(&lastSendTime, NULL);

        // some animations
        //sendVoxelBlinkMessage();
        //sendBlinkingStringOfLights();
        //sendBillboard();
        renderMovingBug();
        
        double end = usecTimestampNow();
        double elapsedSeconds = (end - ::start) / 1000000.0;
        if (::shouldShowPacketsPerSecond) {
            printf("packetsSent=%ld, bytesSent=%ld pps=%f bps=%f\n",packetsSent,bytesSent,
                (float)(packetsSent/elapsedSeconds),(float)(bytesSent/elapsedSeconds));
        }
        // dynamically sleep until we need to fire off the next set of voxels
        double usecToSleep =  ANIMATE_VOXELS_INTERVAL_USECS - (usecTimestampNow() - usecTimestamp(&lastSendTime));
        
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

    AgentList* agentList = AgentList::createInstance(AGENT_TYPE_ANIMATION_SERVER, ANIMATION_LISTEN_PORT);
    setvbuf(stdout, NULL, _IOLBF, 0);

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

    agentList->linkedDataCreateCallback = NULL; // do we need a callback?
    agentList->startSilentAgentRemovalThread();
    agentList->startDomainServerCheckInThread();
    
    srand((unsigned)time(0));

    pthread_t animateVoxelThread;
    pthread_create(&animateVoxelThread, NULL, animateVoxels, NULL);

    sockaddr agentPublicAddress;
    
    unsigned char* packetData = new unsigned char[MAX_PACKET_SIZE];
    ssize_t receivedBytes;

    // loop to send to agents requesting data
    while (true) {
        // Agents sending messages to us...    
        if (agentList->getAgentSocket()->receive(&agentPublicAddress, packetData, &receivedBytes)) {
            switch (packetData[0]) {
                default: {
                    AgentList::getInstance()->processAgentData(&agentPublicAddress, packetData, receivedBytes);
                } break;
            }
        }
    }
    
    pthread_join(animateVoxelThread, NULL);

    return 0;
}
