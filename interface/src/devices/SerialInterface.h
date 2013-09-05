//
//  SerialInterface.h
//  hifi
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__SerialInterface__
#define __interface__SerialInterface__

// These includes are for serial port reading/writing
#ifndef _WIN32
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <dirent.h>
#endif

#include <glm/glm.hpp>

#include "InterfaceConfig.h"
#include "Util.h"

extern const bool USING_INVENSENSE_MPU9150;

class SerialInterface {
public:
    SerialInterface();
    
    void pair();
    void readData(float deltaTime);
    const float getLastPitchRate() const { return _lastRotationRates[0]; }
    const float getLastYawRate() const { return _lastRotationRates[1]; }
    const float getLastRollRate() const { return _lastRotationRates[2]; }
    const glm::vec3& getLastRotationRates() const { return _lastRotationRates; };
    const glm::vec3& getEstimatedRotation() const { return _estimatedRotation; };
    const glm::vec3& getEstimatedPosition() const { return _estimatedPosition; };
    const glm::vec3& getEstimatedVelocity() const { return _estimatedVelocity; };
    const glm::vec3& getEstimatedAcceleration() const { return _estimatedAcceleration; };
    const glm::vec3& getLastAcceleration() const { return _lastAcceleration; };
    const glm::vec3& getGravity() const { return _gravity; };
    
    void renderLevels(int width, int height);
    void resetAverages();
    bool isActive() const { return _active; }
    
private:
    void initializePort(char* portname);
    void resetSerial();

    glm::vec3 recenterCompass(const glm::vec3& compass);

    bool _active;
    int _serialDescriptor;
    int totalSamples;
    timeval lastGoodRead;
    glm::vec3 _gravity;
    glm::vec3 _north;
    glm::vec3 _averageRotationRates;
    glm::vec3 _averageAcceleration;
    glm::vec3 _estimatedRotation;
    glm::vec3 _estimatedPosition;
    glm::vec3 _estimatedVelocity;
    glm::vec3 _estimatedAcceleration;
    glm::vec3 _lastAcceleration;
    glm::vec3 _lastRotationRates;
    glm::vec3 _lastCompass;
    glm::vec3 _compassMinima;
    glm::vec3 _compassMaxima;
    
    glm::mat3 _angularVelocityToLinearAccel;
    glm::mat3 _angularAccelToLinearAccel;
};

#endif
