//
//  avatar.h
//  Avatar Mixer - Main header file
//
//  Created by Leonardo Murillo on 03/25/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved
//

#include <iostream>
#include <math.h>
#include <map>
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
#include <StdDev.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

const int AVATAR_LISTEN_PORT = 55444;
const unsigned short BROADCAST_INTERVAL = 20;
const char *PACKET_FORMAT = "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f";

class AvatarAgent {
private:
    sockaddr _activeSocket;
    float _pitch;
    float _yaw;
    float _roll;
    float _headPositionX;
    float _headPositionY;
    float _headPositionZ;
    float _loudness;
    float _averageLoudness;
    float _handPositionX;
    float _handPositionY;
    float _handPositionZ;
    double _lastHeartbeat;
public:
    AvatarAgent(sockaddr activeSocket,
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
                float handPositionZ,
                double lastHeartbeat);
    ~AvatarAgent();
    sockaddr *getActiveSocket();
    void setActiveSocket(sockaddr activeSocket);
    float getPitch();
    void setPitch(float pitch);
    float getYaw();
    void setYaw(float yaw);
    float getRoll();
    void setRoll(float roll);
    float getHeadPositionX();
    float getHeadPositionY();
    float getHeadPositionZ();
    void setHeadPosition(float x, float y, float z);
    float getLoudness();
    void setLoudness(float loudness);
    float getAverageLoudness();
    void setAverageLoudness(float averageLoudness);
    float getHandPositionX();
    float getHandPositionY();
    float getHandPositionZ();
    void setHandPosition(float x, float y, float z);
    double getLastHeartbeat();
    void setLastHeartbeat(double lastHeartbeat);
};