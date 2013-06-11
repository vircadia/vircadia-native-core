//
//  SerialInterface.h
//  


#ifndef __interface__SerialInterface__
#define __interface__SerialInterface__

#include <glm/glm.hpp>
#include "Util.h"
#include "world.h"
#include "InterfaceConfig.h"
#include "Log.h"

// These includes are for serial port reading/writing
#ifdef __APPLE__
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <dirent.h>
#endif

extern const bool USING_INVENSENSE_MPU9150;

class SerialInterface {
public:
    SerialInterface() : active(false),
                        _gravity(0, 0, 0),
                        _estimatedRotation(0, 0, 0),
                        _estimatedPosition(0, 0, 0),
                        _estimatedVelocity(0, 0, 0),
                        _lastAcceleration(0, 0, 0),
                        _lastRotationRates(0, 0, 0),
                        _angularVelocityToLinearAccel( // experimentally derived initial values
                            0.001f, -0.008f, 0.020f, 
                            0.003f, -0.003f, 0.025f,
                            0.017f, 0.007f, 0.029f),
                        _angularAccelToLinearAccel( // experimentally derived initial values
                            0.0f, 0.0f, 0.002f,
                            0.0f, 0.0f, 0.002f,
                            -0.002f, -0.002f, 0.0f)
                    {}
    
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
    bool active;
    
private:
    void initializePort(char* portname);
    void resetSerial();

    int _serialDescriptor;
    int totalSamples;
    timeval lastGoodRead;
    glm::vec3 _gravity;
    glm::vec3 _estimatedRotation;
    glm::vec3 _estimatedPosition;
    glm::vec3 _estimatedVelocity;
    glm::vec3 _estimatedAcceleration;
    glm::vec3 _lastAcceleration;
    glm::vec3 _lastRotationRates;
    
    glm::mat3 _angularVelocityToLinearAccel;
    glm::mat3 _angularAccelToLinearAccel;
};

#endif
