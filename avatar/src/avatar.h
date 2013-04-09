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
#include <PacketCodes.h>
#include <StdDev.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

const unsigned short AVATAR_LISTEN_PORT = 55444;
const unsigned short BROADCAST_INTERVAL = 20;
const char *packetFormat = "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f";

class AvatarAgent {
private:
    sockaddr _activeSocket;
    float _pitch;
    float _yaw;
    float _roll;
    std::map<char, float> _headPosition;
    float _loudness;
    float _averageLoudness;
    std::map<char, float> _handPosition;
public:
    AvatarAgent();
    ~AvatarAgent();
    sockaddr *getActiveSocket();
    void setActiveSocket(sockaddr activeSocket);
    float getPitch();
    void setPitch(float pitch);
    float getYaw();
    void setYaw(float yaw);
    float getRoll();
    void setRoll(float roll);
    std::map<char, float> getHeadPosition();
    void setHeadPosition(float x, float y, float z);
    float getLoudness();
    void setLoudness(float loudness);
    float getAverageLoudness();
    void setAverageLoudness(float averageLoudness);
    std::map<char, float> getHandPosition();
    void setHandPosition(float x, float y, float z);
};