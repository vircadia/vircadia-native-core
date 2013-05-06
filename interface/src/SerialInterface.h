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
                        _failedOpenAttempts(0) {}
    void pair();
    void readData();
    
    int getLastYaw() const { return _lastYaw; }
    int getLastPitch() const { return _lastPitch; }
    int getLastRoll() const { return _lastRoll; }
    
    int getLED() {return LED;};
    int getNumSamples() {return samplesAveraged;};
    int getValue(int num) {return lastMeasured[num];};
    int getRelativeValue(int num) {return static_cast<int>(lastMeasured[num] - trailingAverage[num]);};
    float getTrailingValue(int num) {return trailingAverage[num];};
    
    void resetTrailingAverages();
    void renderLevels(int width, int height);
    bool active;
    glm::vec3 getGravity() {return gravity;};
    
private:
    int initializePort(char* portname, int baud);
    void resetSerial();
    int lastMeasured[NUM_CHANNELS];
    float trailingAverage[NUM_CHANNELS];
    int samplesAveraged;
    int LED;
    int totalSamples;
    timeval lastGoodRead;
    glm::vec3 gravity;
    int _lastYaw;
    int _lastPitch;
    int _lastRoll;
    int _failedOpenAttempts;
};

#endif
