//
//  avatar.cpp
//  Avatar Mixer
//
//  Created by Leonardo Murillo on 03/25/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved
//
//  The avatar mixer receives head, hand and positional data from all connected
//  agents, and broadcasts that data back to them, every BROADCAST_INTERVAL ms.
//
//

#include <iostream>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <fstream>
#include <limits>
#include <AgentList.h>
#include <SharedUtil.h>
#include <PacketHeaders.h>
#include <StdDev.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <UDPSocket.h>
#include "avatar.h"

std::vector<AvatarAgent> *avatarAgentList = new std::vector<AvatarAgent>;

AvatarAgent *findAvatarAgentBySocket(sockaddr *activeSocket) {
    
    sockaddr *agentSocketHolder = new sockaddr();
    
    for (std::vector<AvatarAgent>::iterator avatarAgent = avatarAgentList->begin();
         avatarAgent != avatarAgentList->end();
         avatarAgent++) {
        agentSocketHolder = avatarAgent->getActiveSocket();
        if (agentSocketHolder->sa_family != activeSocket->sa_family) {
            return NULL;
        }
        sockaddr_in *firstSocket = (sockaddr_in *) activeSocket;
        sockaddr_in *secondSocket = (sockaddr_in *) agentSocketHolder;
        
        if (firstSocket->sin_addr.s_addr == secondSocket->sin_addr.s_addr &&
            firstSocket->sin_port == secondSocket->sin_port) {
            return &*avatarAgent;
        }
    }
    
    return NULL;
}

// Constructor and Destructor
AvatarAgent::AvatarAgent(sockaddr activeSocket,
                         float pitch,
                         float yaw,
                         float roll,
                         float headPositionX,
                         float headPositionY,
                         float headPositionZ,
                         float loudness,
                         float averageLoudness,
                         float handPositionX,
                         float handPositionY,
                         float handPositionZ) {
    
    this->setActiveSocket(activeSocket);
    this->setPitch(pitch);
    this->setYaw(yaw);
    this->setRoll(roll);
    this->setHeadPosition(headPositionX, headPositionY, headPositionZ);
    this->setLoudness(loudness);
    this->setAverageLoudness(averageLoudness);
    this->setHandPosition(handPositionX, handPositionY, handPositionZ);
    
}

AvatarAgent::~AvatarAgent() {
    
}

// Property getters
sockaddr *AvatarAgent::getActiveSocket() {
    return &_activeSocket;
}

float AvatarAgent::getPitch() {
    return _pitch;
}

float AvatarAgent::getYaw() {
    return _yaw;
}

float AvatarAgent::getRoll() {
    return _roll;
}

float AvatarAgent::getHeadPositionX() {
    return _headPositionX;
}

float AvatarAgent::getHeadPositionY() {
    return _headPositionY;
}

float AvatarAgent::getHeadPositionZ() {
    return _headPositionZ;
}

float AvatarAgent::getLoudness() {
    return _loudness;
}

float AvatarAgent::getAverageLoudness() {
    return _averageLoudness;
}

float AvatarAgent::getHandPositionX() {
    return _handPositionX;
}

float AvatarAgent::getHandPositionY() {
    return _handPositionY;
}

float AvatarAgent::getHandPositionZ() {
    return _handPositionZ;
}

// Property setters
void AvatarAgent::setPitch(float pitch) {
    _pitch = pitch;
}

void AvatarAgent::setYaw(float yaw) {
    _yaw = yaw;
}

void AvatarAgent::setRoll(float roll) {
    _roll = roll;
}

void AvatarAgent::setHeadPosition(float x, float y, float z) {
    _headPositionX = x;
    _headPositionY = y;
    _headPositionZ = z;
}

void AvatarAgent::setLoudness(float loudness) {
    _loudness = loudness;
}

void AvatarAgent::setAverageLoudness(float averageLoudness) {
    _averageLoudness = averageLoudness;
}

void AvatarAgent::setHandPosition(float x, float y, float z) {
    _handPositionX = x;
    _handPositionY = y;
    _handPositionZ = z;
}


unsigned char *addAgentToBroadcastPacket(unsigned char *currentPosition, AvatarAgent *agentToAdd) {
    unsigned char *packetData = new unsigned char();
    
    currentPosition += packSocket(currentPosition, agentToAdd->getActiveSocket());

    sprintf((char *)packetData, packetFormat, agentToAdd->getPitch(),
                                              agentToAdd->getYaw(),
                                              agentToAdd->getRoll(),
                                              agentToAdd->getHeadPosition()['x'],
                                              agentToAdd->getHeadPosition()['y'],
                                              agentToAdd->getHeadPosition()['z'],
                                              agentToAdd->getLoudness(),
                                              agentToAdd->getAverageLoudness(),
                                              agentToAdd->getHandPosition()['x'],
                                              agentToAdd->getHandPosition()['y'],
                                              agentToAdd->getHandPosition()['z']);

    memcpy(currentPosition, packetData, strlen((const char*)packetData));
    currentPosition += strlen((const char*)packetData);

    return currentPosition;
}

void *sendAvatarData(void *args) {
    timeval startTime;
    while (true) {
        gettimeofday(&startTime, NULL);
        
        unsigned char *currentBufferPosition;
        unsigned char *startPointer;
        unsigned char *broadcastPacket = new unsigned char[MAX_PACKET_SIZE];
        
        *broadcastPacket = *(unsigned char *)PACKET_HEADER_HEAD_DATA;
        currentBufferPosition = broadcastPacket + 1;
        startPointer = currentBufferPosition;
        
        
        
        double usecToSleep = usecTimestamp(&startTime) + (BROADCAST_INTERVAL * 10000000) - usecTimestampNow();
        usleep(usecToSleep);
    }
}

int main(int argc, char* argv[])
{
    setvbuf(stdout, NULL, _IOLBF, 0);
    
    pthread_t sendAvatarDataThread;
    pthread_create(&sendAvatarDataThread, NULL, sendAvatarData, NULL);
    
    sockaddr *agentAddress = new sockaddr;
    char *packetData = new char[MAX_PACKET_SIZE];
    ssize_t receivedBytes = 0;
    
    UDPSocket *avatarMixerSocket = new UDPSocket(AVATAR_LISTEN_PORT);
    AvatarAgent *matchingAgent = NULL;
    
    float *pitch;
    float *yaw;
    float *roll;
    float *headPositionX;
    float *headPositionY;
    float *headPositionZ;
    float *loudness;
    float *averageLoudness;
    float *handPositionX;
    float *handPositionY;
    float *handPositionZ;
    
    while (true) {
        if (avatarMixerSocket->receive(agentAddress, packetData, &receivedBytes)) {
            if (packetData[0] == PACKET_HEADER_HEAD_DATA) {
                // Extract data from packet
                sscanf(packetData + 1,
                       PACKET_FORMAT,
                       &pitch,
                       &yaw,
                       &roll,
                       &headPositionX,
                       &headPositionY,
                       &headPositionZ,
                       &loudness,
                       &averageLoudness,
                       &handPositionX,
                       &handPositionY,
                       &handPositionZ);
                
                matchingAgent = findAvatarAgentBySocket(agentAddress);
                
                if (matchingAgent) {
                    // We already have this agent on our list, just modify positional data
                    
                } else {
                    // This is a new agent, we need to add to the list
                    AvatarAgent thisAgentHolder = *new AvatarAgent(*agentAddress,
                                                                   *pitch,
                                                                   *yaw,
                                                                   *roll,
                                                                   *headPositionX,
                                                                   *headPositionY,
                                                                   *headPositionZ,
                                                                   *loudness,
                                                                   *averageLoudness,
                                                                   *handPositionX,
                                                                   *handPositionY,
                                                                   *handPositionZ);
                    avatarAgentList->push_back(thisAgentHolder);
                }
            }
        }
    }
}
