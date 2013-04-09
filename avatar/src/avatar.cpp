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

const int LISTEN_PORT = 55444;

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
        } else {
            return NULL;
        }
    }
}

// Constructor and Destructor
AvatarAgent::AvatarAgent() {
    
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

std::map<char, float> AvatarAgent::getHeadPosition() {
    return _headPosition;
}

float AvatarAgent::getLoudness() {
    return _loudness;
}

float AvatarAgent::getAverageLoudness() {
    return _averageLoudness;
}

std::map<char, float> AvatarAgent::getHandPosition() {
    return _handPosition;
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
    _headPosition['x'] = x;
    _headPosition['y'] = y;
    _headPosition['z'] = z;
}

void AvatarAgent::setLoudness(float loudness) {
    _loudness = loudness;
}

void AvatarAgent::setAverageLoudness(float averageLoudness) {
    _averageLoudness = averageLoudness;
}

void AvatarAgent::setHandPosition(float x, float y, float z) {
    _handPosition['x'] = x;
    _handPosition['y'] = y;
    _handPosition['z'] = z;
}


unsigned char *addAgentToBroadcastPacket(unsigned char *currentPosition, Agent *agentToAdd) {
    unsigned char *packetData = new unsigned char();
    AvatarAgent *thisAgent = new AvatarAgent();
    
    *currentPosition += packAgentId(currentPosition, agentToAdd->getAgentId());
    currentPosition += packSocket(currentPosition, agentToAdd->getActiveSocket());

    sprintf((char *)packetData, packetFormat, thisAgent->getPitch(),
                                              thisAgent->getYaw(),
                                              thisAgent->getRoll(),
                                              thisAgent->getHeadPosition()[0],
                                              thisAgent->getHeadPosition()[1],
                                              thisAgent->getHeadPosition()[2],
                                              thisAgent->getLoudness(),
                                              thisAgent->getAverageLoudness(),
                                              thisAgent->getHandPosition()[0],
                                              thisAgent->getHandPosition()[1],
                                              thisAgent->getHandPosition()[2]);

    memcpy(currentPosition, packetData, strlen((const char*)packetData));
    currentPosition += strlen((const char*)packetData);

    return currentPosition;
}

void *sendAvatarData(void *args)
{
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
    unsigned char *packetData = new unsigned char[MAX_PACKET_SIZE];
    ssize_t receivedBytes = 0;
    
    while (true) {
        
    }
}
