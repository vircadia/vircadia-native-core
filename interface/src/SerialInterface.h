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

#define NUM_CHANNELS 6

//  Acceleration sensors, in screen/world coord system (X = left/right, Y = Up/Down, Z = fwd/back)
#define ACCEL_X 3 
#define ACCEL_Y 4 
#define ACCEL_Z 5 

//  Gyro sensors, in coodinate system of head/airplane
#define HEAD_PITCH_RATE 1
#define HEAD_YAW_RATE 0
#define HEAD_ROLL_RATE 2

extern const bool USING_INVENSENSE_MPU9150;

class SerialInterface {
public:
    SerialInterface() : active(false),
                        _gravity(0,0,0),
                        _averageGyroRates(0,0,0),
                        _lastAccelX(0),
                        _lastAccelY(0),
                        _lastAccelZ(0),
                        _lastYawRate(0),
                        _lastPitchRate(0),
                        _lastRollRate(0) {}
    
    void pair();
    void readData();
    
    float getLastYawRate() const { return _lastYawRate - _averageGyroRates[1]; }
    float getLastPitchRate() const { return _lastPitchRate - _averageGyroRates[2]; }
    float getLastRollRate() const { return _lastRollRate - _averageGyroRates[0]; }
    glm::vec3 getLastAcceleration() { return glm::vec3(_lastAccelX, _lastAccelY, _lastAccelZ); };
    glm::vec3 getGravity() {return _gravity;};
    
    void renderLevels(int width, int height);
    void resetAverages();
    bool active;
    
private:
    void initializePort(char* portname, int baud);
    void resetSerial();

    int _serialDescriptor;
    int totalSamples;
    timeval lastGoodRead;
    glm::vec3 _gravity;
    glm::vec3 _averageGyroRates;
    float _lastAccelX;
    float _lastAccelY;
    float _lastAccelZ;
    float _lastYawRate;         //  Rates are in degrees per second. 
    float _lastPitchRate;
    float _lastRollRate;
};

#endif
