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

const int ANIMATION_LISTEN_PORT = 40107;
const int ANIMATE_VOXELS_INTERVAL_USECS = SIXTY_FPS_IN_MILLISECONDS * 1000.0 * 1;

bool wantLocalDomain = false;

static void sendVoxelServerZMessage() {
    char message[100];
    sprintf(message,"%c%s",'Z',"a message");
    int messageSize = strlen(message) + 1;
    AgentList::getInstance()->broadcastToAgents((unsigned char*) message, messageSize, &AGENT_TYPE_VOXEL, 1);
}

static void sendVoxelEditMessage(PACKET_HEADER header, VoxelDetail& detail) {
    unsigned char* bufferOut;
    int sizeOut;
    
    if (createVoxelEditMessage(header, 0, 1, &detail, bufferOut, sizeOut)){
        AgentList::getInstance()->broadcastToAgents(bufferOut, sizeOut, &AGENT_TYPE_VOXEL, 1);
        delete bufferOut;
    }
}

float intensity = 0.5f;
float intensityIncrement = 0.1f;
const float MAX_INTENSITY = 1.0f;
const float MIN_INTENSITY = 0.5f;

static void sendVoxelBlinkMessage() {
    VoxelDetail detail;
    detail.s = 0.25f / TREE_SCALE;
    
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
const int LIGHT_COUNT = 64;
glm::vec3 stringOfLights[LIGHT_COUNT];
unsigned char offColor[3] = { 240, 240, 240 };
unsigned char onColor[3]  = {   0, 255,   0 };

static void sendBlinkingStringOfLights() {
    PACKET_HEADER message = PACKET_HEADER_SET_VOXEL_DESTRUCTIVE; // we're a bully!
    float lightScale = 0.125f / TREE_SCALE;
    VoxelDetail detail;

    detail.s = lightScale;

    // first initialized the string of lights if needed...
    if (!stringOfLightsInitialized) {
        for (int i = 0; i < LIGHT_COUNT; i++) {
            stringOfLights[i] = glm::vec3(i * lightScale,0,0);

            detail.x = detail.s * floor(stringOfLights[i].x / detail.s);
            detail.y = detail.s * floor(stringOfLights[i].y / detail.s);
            detail.z = detail.s * floor(stringOfLights[i].z / detail.s);

            if (i == currentLight) {
                detail.red   = onColor[0];
                detail.green = onColor[1];
                detail.blue  = onColor[2];
            } else {
                detail.red   = offColor[0];
                detail.green = offColor[1];
                detail.blue  = offColor[2];
            }
            sendVoxelEditMessage(message, detail);
        }
        stringOfLightsInitialized = true;
    } else {
        // turn off current light
        detail.x = detail.s * floor(stringOfLights[currentLight].x / detail.s);
        detail.y = detail.s * floor(stringOfLights[currentLight].y / detail.s);
        detail.z = detail.s * floor(stringOfLights[currentLight].z / detail.s);
        detail.red   = offColor[0];
        detail.green = offColor[1];
        detail.blue  = offColor[2];
        sendVoxelEditMessage(message, detail);

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
        detail.x = detail.s * floor(stringOfLights[currentLight].x / detail.s);
        detail.y = detail.s * floor(stringOfLights[currentLight].y / detail.s);
        detail.z = detail.s * floor(stringOfLights[currentLight].z / detail.s);
        detail.red   = onColor[0];
        detail.green = onColor[1];
        detail.blue  = onColor[2];
        sendVoxelEditMessage(message, detail);
    }
}

bool billboardInitialized = false;
const int BILLBOARD_HEIGHT = 9;
const int BILLBOARD_WIDTH  = 44;
glm::vec3 billboardPosition((0.125f / TREE_SCALE),(0.125f / TREE_SCALE),0);
glm::vec3 billboardLights[BILLBOARD_HEIGHT][BILLBOARD_WIDTH];
unsigned char billboardOffColor[3] = { 240, 240, 240 };
unsigned char billboardOnColorA[3]  = {   0,   0, 255 };
unsigned char billboardOnColorB[3]  = {   0, 255,   0 };
float billboardGradient = 0.5f;
float billboardGradientIncrement = 0.01f;
const float BILLBOARD_MAX_GRADIENT = 1.0f;
const float BILLBOARD_MIN_GRADIENT = 0.0f;

// top to bottom... 
bool billBoardMessage[BILLBOARD_HEIGHT][BILLBOARD_WIDTH] = {
 { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
 { 0,1,0,0,1,0,1,0,0,0,0,0,1,0,0,0,0,1,1,1,1,0,0,0,0,0,1,0,1,1,1,0,1,0,1,0,0,1,0,0,0,0,0,0 },
 { 0,1,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,0,1,0,1,0,1,0,0,0,1,1,1,0,0,0,0,0 },
 { 0,1,1,1,1,0,1,0,1,1,1,0,1,1,1,0,0,1,1,1,0,0,0,0,1,1,1,0,1,1,1,0,1,0,1,0,0,1,0,0,1,0,1,0 },
 { 0,1,0,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1,0,0,0,0,1,0,1,0,1,0,1,0,0,0,1,0,1,0,0,1,0,0,1,0,1,0 },
 { 0,1,0,0,1,0,1,0,1,1,1,0,1,0,1,0,0,1,0,0,0,0,1,0,1,1,1,0,1,1,1,0,1,0,1,0,0,1,0,0,1,1,1,0 },
 { 0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0 },
 { 0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0 },
 { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }
};

static void sendBillboard() {
    PACKET_HEADER message = PACKET_HEADER_SET_VOXEL_DESTRUCTIVE; // we're a bully!
    float lightScale = 0.125f / TREE_SCALE;
    VoxelDetail detail;

    detail.s = lightScale;

    // first initialized the billboard of lights if needed...
    if (!billboardInitialized) {
        for (int i = 0; i < BILLBOARD_HEIGHT; i++) {
            for (int j = 0; j < BILLBOARD_WIDTH; j++) {
                
                billboardLights[i][j] = billboardPosition + glm::vec3(j * lightScale, (float)((BILLBOARD_HEIGHT - i) * lightScale), 0);
            }
        }
        billboardInitialized = true;
    }

    // what will our animation be?
    // fade in/out the message?
    
    //for now, just send it again..., but now, with intensity

    ::billboardGradient += ::billboardGradientIncrement;
    
    printf("billboardGradient=%f billboardGradientIncrement=%f\n",billboardGradient,billboardGradientIncrement);
    
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
            
            billboardLights[i][j] = billboardPosition + glm::vec3(j * lightScale, (float)((BILLBOARD_HEIGHT - i) * lightScale), 0);

            detail.x = billboardLights[i][j].x;
            detail.y = billboardLights[i][j].y;
            detail.z = billboardLights[i][j].z;

            if (billBoardMessage[i][j]) {
                detail.red   = (billboardOnColorA[0] + ((billboardOnColorB[0] - billboardOnColorA[0]) * ::billboardGradient));
                detail.green = (billboardOnColorA[1] + ((billboardOnColorB[1] - billboardOnColorA[1]) * ::billboardGradient));
                detail.blue  = (billboardOnColorA[2] + ((billboardOnColorB[2] - billboardOnColorA[2]) * ::billboardGradient));
            } else {
                detail.red   = billboardOffColor[0];
                detail.green = billboardOffColor[1];
                detail.blue  = billboardOffColor[2];
            }
            sendVoxelEditMessage(message, detail);
        }
    }
}


void* animateVoxels(void* args) {
    
    AgentList* agentList = AgentList::getInstance();
    timeval lastSendTime;
    
    while (true) {
        gettimeofday(&lastSendTime, NULL);

        //printf("animateVoxels thread\n");
        
        //printf("animateVoxels thread: Sending a Blink message to the voxel server\n");
        sendVoxelBlinkMessage();
        sendBlinkingStringOfLights();
        sendBillboard();

        /**
        int agentNumber = 0;
        for (AgentList::iterator agent = agentList->begin(); agent != agentList->end(); agent++) {
            agentNumber++;
            printf("current agents[%d] id: %d type: %s \n", agentNumber, agent->getAgentID(), agent->getTypeName());
        }
        **/

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
    AgentList* agentList = AgentList::createInstance(AGENT_TYPE_ANIMATION_SERVER, ANIMATION_LISTEN_PORT);
    setvbuf(stdout, NULL, _IOLBF, 0);

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

                case PACKET_HEADER_Z_COMMAND: {

                    // the Z command is a special command that allows the sender to send the voxel server high level semantic
                    // requests, like erase all, or add sphere scene
                    char* command = (char*) &packetData[1]; // start of the command
                    int commandLength = strlen(command); // commands are null terminated strings
                    int totalLength = 1+commandLength+1;

                    printf("got Z message len(%ld)= %s\n",receivedBytes,command);

                    while (totalLength <= receivedBytes) {
                        if (0==strcmp(command,(char*)"erase all")) {
                            printf("got Z message == erase all, we don't support that\n");
                        }
                        if (0==strcmp(command,(char*)"add scene")) {
                            printf("got Z message == add scene, we don't support that\n");
                        }
                        totalLength += commandLength+1;
                    }
                    // If we wanted to rebroadcast this message we could do it here.
                    //printf("rebroadcasting Z message to connected agents... agentList.broadcastToAgents()\n");
                    //agentList->broadcastToAgents(packetData,receivedBytes, &AGENT_TYPE_AVATAR, 1);
                } break;
                
                default: {
                    AgentList::getInstance()->processAgentData(&agentPublicAddress, packetData, receivedBytes);
                } break;

            }
        }
    }
    
    pthread_join(animateVoxelThread, NULL);

    return 0;
}
