//
//  SerialInterface.h
//  


#ifndef __interface__SerialInterface__
#define __interface__SerialInterface__

#include <glm/glm.hpp>
#include "Util.h"
#include "world.h"
#include "InterfaceConfig.h"
#include <iostream>

// These includes are for serial port reading/writing
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#define NUM_CHANNELS 6

//  Acceleration sensors, in screen/world coord system (X = left/right, Y = Up/Down, Z = fwd/back)
#define ACCEL_X 3 
#define ACCEL_Y 4 
#define ACCEL_Z 5 

//  Gyro sensors, in coodinate system of head/airplane
#define PITCH_RATE 1
#define YAW_RATE 0
#define ROLL_RATE 2

class SerialInterface {
public:
    SerialInterface() {};
    void pair();
    void readData();
    int getLED() {return LED;};
    int getNumSamples() {return samplesAveraged;};
    int getValue(int num) {return lastMeasured[num];};
    int getRelativeValue(int num) {return lastMeasured[num] - trailingAverage[num];};
    float getTrailingValue(int num) {return trailingAverage[num];};
    void resetTrailingAverages();
    void renderLevels(int width, int height);
    bool active;
private:
    int init(char * portname, int baud);
    void resetSerial();
    int lastMeasured[NUM_CHANNELS];
    float trailingAverage[NUM_CHANNELS];
    int samplesAveraged;
    int LED;
    int noReadCount;
    int totalSamples;
};

#endif
