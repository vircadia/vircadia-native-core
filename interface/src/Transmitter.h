//
//  Transmitter.h
//  hifi
//
//  Created by Philip Rosedale on 5/20/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__Transmitter__
#define __hifi__Transmitter__

#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cstring>
#include "world.h"
#include <stdint.h> 

struct TouchState {
    uint16_t x, y;
    char state;
};

class Transmitter
{
public:
    Transmitter();
    void render();
    void checkForLostTransmitter();
    void resetLevels();
    void renderLevels(int width, int height);
    bool isConnected() { return _isConnected; };
    const glm::vec3 getLastRotationRate() const { return _lastRotationRate; };
    const glm::vec3 getLastAcceleration() const { return _lastRotationRate; };
    const glm::vec3 getEstimatedRotation() const { return _estimatedRotation; };
    const TouchState* getTouchState() const { return &_touchState; };
    void processIncomingData(unsigned char* packetData, int numBytes);

private:
    bool _isConnected;
    glm::vec3 _lastRotationRate;
    glm::vec3 _lastAcceleration;
    glm::vec3 _estimatedRotation;
    TouchState _touchState;
    timeval* _lastReceivedPacket;

#endif /* defined(__hifi__Transmitter__) */
};
