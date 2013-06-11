//
//  SerialInterface.cpp
//  2012 by Philip Rosedale for High Fidelity Inc.
//
//  Read interface data from the gyros/accelerometer Invensense board using the SerialUSB
//

#include "SerialInterface.h"
#include "SharedUtil.h"
#include "Util.h"
#include <glm/gtx/vector_angle.hpp>
#include <math.h>

#ifdef __APPLE__
#include <regex.h>
#include <sys/time.h>
#include <string>
#endif

const short NO_READ_MAXIMUM_MSECS = 3000;
const int GRAVITY_SAMPLES = 60;                     //  Use the first few samples to baseline values
const int SENSOR_FUSION_SAMPLES = 20;
const int LONG_TERM_RATE_SAMPLES = 1000;            

const bool USING_INVENSENSE_MPU9150 = 1;

void SerialInterface::pair() {
    
#ifdef __APPLE__
    // look for a matching gyro setup
    DIR *devDir;
    struct dirent *entry;
    int matchStatus;
    regex_t regex;
    
    // for now this only works on OS X, where the usb serial shows up as /dev/tty.usb*
    if((devDir = opendir("/dev"))) {
        while((entry = readdir(devDir))) {
            regcomp(&regex, "tty\\.usb", REG_EXTENDED|REG_NOSUB);
            matchStatus = regexec(&regex, entry->d_name, (size_t) 0, NULL, 0);
            if (matchStatus == 0) {
                char *serialPortname = new char[100];
                sprintf(serialPortname, "/dev/%s", entry->d_name);
                
                initializePort(serialPortname);
                
                delete [] serialPortname;
            }
            regfree(&regex);
        }
        closedir(devDir);
    }    
#endif
}

//  connect to the serial port
void SerialInterface::initializePort(char* portname) {
#ifdef __APPLE__
    _serialDescriptor = open(portname, O_RDWR | O_NOCTTY | O_NDELAY);
    
    printLog("Opening SerialUSB %s: ", portname);
    
    if (_serialDescriptor == -1) {
        printLog("Failed.\n");
        return;
    }
    
    struct termios options;
    tcgetattr(_serialDescriptor, &options);
        
    options.c_cflag |= (CLOCAL | CREAD | CS8);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    tcsetattr(_serialDescriptor, TCSANOW, &options);
    
    cfsetispeed(&options,B115200);
    cfsetospeed(&options,B115200);
    
    if (USING_INVENSENSE_MPU9150) {
        // block on invensense reads until there is data to read
        int currentFlags = fcntl(_serialDescriptor, F_GETFL);
        fcntl(_serialDescriptor, F_SETFL, currentFlags & ~O_NONBLOCK);
        
        // there are extra commands to send to the invensense when it fires up
        
        // this takes it out of SLEEP
        write(_serialDescriptor, "WR686B01\n", 9);
        
        // delay after the wakeup
        usleep(10000);
        
        // this disables streaming so there's no garbage data on reads
        write(_serialDescriptor, "SD\n", 3);
        
        // delay after disabling streaming
        usleep(10000);
        
        // flush whatever was produced by the last two commands
        tcflush(_serialDescriptor, TCIOFLUSH);
    }
    
    printLog("Connected.\n");
    resetSerial();
    
    active = true;
 #endif
}

//  Render the serial interface channel values onscreen as vertical lines
void SerialInterface::renderLevels(int width, int height) {
    char val[40];
    if (USING_INVENSENSE_MPU9150) {
        //  For invensense gyros, render as horizontal bars
        const int LEVEL_CORNER_X = 10;
        const int LEVEL_CORNER_Y = 200;
        
        // Draw the numeric degree/sec values from the gyros
        sprintf(val, "Yaw    %4.1f", _estimatedRotation.y);
        drawtext(LEVEL_CORNER_X, LEVEL_CORNER_Y, 0.10, 0, 1.0, 1, val, 0, 1, 0);
        sprintf(val, "Pitch %4.1f", _estimatedRotation.x);
        drawtext(LEVEL_CORNER_X, LEVEL_CORNER_Y + 15, 0.10, 0, 1.0, 1, val, 0, 1, 0);
        sprintf(val, "Roll  %4.1f", _estimatedRotation.z);
        drawtext(LEVEL_CORNER_X, LEVEL_CORNER_Y + 30, 0.10, 0, 1.0, 1, val, 0, 1, 0);
        sprintf(val, "X     %4.3f", _lastAcceleration.x - _gravity.x);
        drawtext(LEVEL_CORNER_X, LEVEL_CORNER_Y + 45, 0.10, 0, 1.0, 1, val, 0, 1, 0);
        sprintf(val, "Y     %4.3f", _lastAcceleration.y - _gravity.y);
        drawtext(LEVEL_CORNER_X, LEVEL_CORNER_Y + 60, 0.10, 0, 1.0, 1, val, 0, 1, 0);
        sprintf(val, "Z     %4.3f", _lastAcceleration.z - _gravity.z);
        drawtext(LEVEL_CORNER_X, LEVEL_CORNER_Y + 75, 0.10, 0, 1.0, 1, val, 0, 1, 0);
        
        //  Draw the levels as horizontal lines        
        const int LEVEL_CENTER = 150;
        const float ACCEL_VIEW_SCALING = 10.f;
        const float POSITION_SCALING = 400.f;

        glLineWidth(2.0);
        glBegin(GL_LINES);
        // Rotation rates
        glColor4f(1, 1, 1, 1);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y - 3);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + getLastYawRate(), LEVEL_CORNER_Y - 3);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 12);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + getLastPitchRate(), LEVEL_CORNER_Y + 12);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 27);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + getLastRollRate(), LEVEL_CORNER_Y + 27);
        // Estimated Rotation
        glColor4f(0, 1, 1, 1);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y - 1);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + _estimatedRotation.y, LEVEL_CORNER_Y - 1);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 14);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + _estimatedRotation.x, LEVEL_CORNER_Y + 14);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 29);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + _estimatedRotation.z, LEVEL_CORNER_Y + 29);

        // Acceleration rates
        glColor4f(1, 1, 1, 1);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 42);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + (int)(_estimatedAcceleration.x * ACCEL_VIEW_SCALING), LEVEL_CORNER_Y + 42);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 57);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + (int)(_estimatedAcceleration.y * ACCEL_VIEW_SCALING), LEVEL_CORNER_Y + 57);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 72);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + (int)(_estimatedAcceleration.z * ACCEL_VIEW_SCALING), LEVEL_CORNER_Y + 72);
        
        // Estimated Position
        glColor4f(0, 1, 1, 1);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 44);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + (int)(_estimatedPosition.x * POSITION_SCALING), LEVEL_CORNER_Y + 44);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 59);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + (int)(_estimatedPosition.y * POSITION_SCALING), LEVEL_CORNER_Y + 59);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 74);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + (int)(_estimatedPosition.z * POSITION_SCALING), LEVEL_CORNER_Y + 74);


        glEnd();
        //  Draw green vertical centerline
        glColor4f(0, 1, 0, 0.5);
        glBegin(GL_LINES);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y - 6);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 30);
        glEnd();
    }
}

void convertHexToInt(unsigned char* sourceBuffer, int& destinationInt) {
    unsigned int byte[2];
    
    for(int i = 0; i < 2; i++) {
        sscanf((char*) sourceBuffer + 2 * i, "%2x", &byte[i]);
    }
    
    int16_t result = (byte[0] << 8);
    result += byte[1];
    
    destinationInt = result;
}
void SerialInterface::readData(float deltaTime) {
#ifdef __APPLE__
    
    int initialSamples = totalSamples;
    
    if (USING_INVENSENSE_MPU9150) { 
        unsigned char sensorBuffer[36];
        
        // ask the invensense for raw gyro data
        write(_serialDescriptor, "RD683B0E\n", 9);
        read(_serialDescriptor, sensorBuffer, 36);
        
        int accelXRate, accelYRate, accelZRate;
        
        convertHexToInt(sensorBuffer + 6, accelZRate);
        convertHexToInt(sensorBuffer + 10, accelYRate);
        convertHexToInt(sensorBuffer + 14, accelXRate);
        
        const float LSB_TO_METERS_PER_SECOND2 = 1.f / 16384.f * GRAVITY_EARTH;
                                                                //  From MPU-9150 register map, with setting on
                                                                //  highest resolution = +/- 2G
        
        //_lastAcceleration = glm::vec3(-accelXRate, -accelYRate, -accelZRate) * LSB_TO_METERS_PER_SECOND2;
        _lastAcceleration = glm::vec3(-accelYRate, accelXRate, accelZRate) * LSB_TO_METERS_PER_SECOND2;
        
        int rollRate, yawRate, pitchRate;
        
        convertHexToInt(sensorBuffer + 22, rollRate);
        convertHexToInt(sensorBuffer + 26, yawRate);
        convertHexToInt(sensorBuffer + 30, pitchRate);
        
        //  Convert the integer rates to floats
        const float LSB_TO_DEGREES_PER_SECOND = 1.f / 16.4f;     //  From MPU-9150 register map, 2000 deg/sec.
        glm::vec3 rotationRates;
        //rotationRates[0] = ((float) -pitchRate) * LSB_TO_DEGREES_PER_SECOND;
        //rotationRates[1] = ((float) -yawRate) * LSB_TO_DEGREES_PER_SECOND;
        //rotationRates[2] = ((float) -rollRate) * LSB_TO_DEGREES_PER_SECOND;

        rotationRates[0] = ((float) -yawRate) * LSB_TO_DEGREES_PER_SECOND;
        rotationRates[1] = ((float)  pitchRate) * LSB_TO_DEGREES_PER_SECOND;
        rotationRates[2] = ((float) -rollRate) * LSB_TO_DEGREES_PER_SECOND;

        // compute the angular acceleration
        glm::vec3 angularAcceleration = (deltaTime < EPSILON) ? glm::vec3() : (rotationRates - _lastRotationRates) / deltaTime;
        _lastRotationRates = rotationRates;

        //  Update raw rotation estimates
        glm::quat estimatedRotation = glm::quat(glm::radians(_estimatedRotation)) *
            glm::quat(glm::radians(deltaTime * _lastRotationRates));
        
        //  Update acceleration estimate: first, subtract gravity as rotated into current frame
        _estimatedAcceleration = _lastAcceleration - glm::inverse(estimatedRotation) * _gravity;
        
        //  Consider updating our angular velocity/acceleration to linear acceleration mapping
        if (glm::length(_lastRotationRates) > EPSILON || glm::length(angularAcceleration) > EPSILON) {
            // compute predicted linear acceleration, find error between actual and predicted
            glm::vec3 predictedAcceleration = _angularVelocityToLinearAccel * _lastRotationRates +
                _angularAccelToLinearAccel * angularAcceleration;
            glm::vec3 error = _estimatedAcceleration - predictedAcceleration;
            
            // adjust according to error in each dimension, in proportion to input magnitudes
            for (int i = 0; i < 3; i++) {
                if (fabsf(error[i]) < EPSILON) {
                    continue;
                }
                const float LEARNING_RATE = 0.1f;
                float rateSum = fabsf(_lastRotationRates.x) + fabsf(_lastRotationRates.y) + fabsf(_lastRotationRates.z);
                if (rateSum > EPSILON) {
                    for (int j = 0; j < 3; j++) {
                        float proportion = LEARNING_RATE * fabsf(_lastRotationRates[j]) / rateSum;
                        if (proportion > EPSILON) {
                            _angularVelocityToLinearAccel[j][i] += error[i] * proportion / _lastRotationRates[j];
                        }
                    }
                }
                float accelSum = fabsf(angularAcceleration.x) + fabsf(angularAcceleration.y) + fabsf(angularAcceleration.z);
                if (accelSum > EPSILON) {
                    for (int j = 0; j < 3; j++) {
                        float proportion = LEARNING_RATE * fabsf(angularAcceleration[j]) / accelSum;
                        if (proportion > EPSILON) {
                            _angularAccelToLinearAccel[j][i] += error[i] * proportion / angularAcceleration[j];
                        }
                    }                
                }
            }
        }
        
        printLog("%g %g %g\n", _angularVelocityToLinearAccel[0][0], _angularVelocityToLinearAccel[1][0], _angularVelocityToLinearAccel[2][0]);
        printLog("%g %g %g\n", _angularVelocityToLinearAccel[0][1], _angularVelocityToLinearAccel[1][1], _angularVelocityToLinearAccel[2][1]);
        printLog("%g %g %g\n\n", _angularVelocityToLinearAccel[0][2], _angularVelocityToLinearAccel[1][2], _angularVelocityToLinearAccel[2][2]);
        
        printLog("%g %g %g\n", _angularAccelToLinearAccel[0][0], _angularAccelToLinearAccel[1][0], _angularAccelToLinearAccel[2][0]);
        printLog("%g %g %g\n", _angularAccelToLinearAccel[0][1], _angularAccelToLinearAccel[1][1], _angularAccelToLinearAccel[2][1]);
        printLog("%g %g %g\n\n", _angularAccelToLinearAccel[0][2], _angularAccelToLinearAccel[1][2], _angularAccelToLinearAccel[2][2]);
        
        //  Update estimated position and velocity
        float const DECAY_VELOCITY = 0.95f;
        float const DECAY_POSITION = 0.95f;
        _estimatedVelocity += deltaTime * _estimatedAcceleration;
        _estimatedPosition += deltaTime * _estimatedVelocity;
        _estimatedVelocity *= DECAY_VELOCITY;
        _estimatedPosition *= DECAY_POSITION;
                
        //  Accumulate a set of initial baseline readings for setting gravity
        if (totalSamples == 0) {
            _gravity = _lastAcceleration;
        } 
        else {
            if (totalSamples < GRAVITY_SAMPLES) {
                _gravity = (1.f - 1.f/(float)GRAVITY_SAMPLES) * _gravity +
                1.f/(float)GRAVITY_SAMPLES * _lastAcceleration;
            } else {
                //  Use gravity reading to do sensor fusion on the pitch and roll estimation
                estimatedRotation = safeMix(estimatedRotation,
                    rotationBetween(estimatedRotation * _lastAcceleration, _gravity) * estimatedRotation,
                    1.0f / SENSOR_FUSION_SAMPLES);
                
                //  Without a compass heading, always decay estimated Yaw slightly
                const float YAW_DECAY = 0.999f;
                glm::vec3 forward = estimatedRotation * glm::vec3(0.0f, 0.0f, -1.0f);
                estimatedRotation = safeMix(glm::angleAxis(glm::degrees(atan2f(forward.x, -forward.z)),
                    glm::vec3(0.0f, 1.0f, 0.0f)) * estimatedRotation, estimatedRotation, YAW_DECAY);
            }
        }
        
        _estimatedRotation = safeEulerAngles(estimatedRotation); 
        
        totalSamples++;
    } 
    
    if (initialSamples == totalSamples) {        
        timeval now;
        gettimeofday(&now, NULL);
        
        if (diffclock(&lastGoodRead, &now) > NO_READ_MAXIMUM_MSECS) {
            printLog("No data - Shutting down SerialInterface.\n");
            resetSerial();
        }
    } else {
        gettimeofday(&lastGoodRead, NULL);
    }
#endif
}

void SerialInterface::resetAverages() {
    totalSamples = 0;
    _gravity = glm::vec3(0, 0, 0);
    _lastRotationRates = glm::vec3(0, 0, 0);
    _estimatedRotation = glm::vec3(0, 0, 0);
    _estimatedPosition = glm::vec3(0, 0, 0);
    _estimatedVelocity = glm::vec3(0, 0, 0);
}

void SerialInterface::resetSerial() {
#ifdef __APPLE__
    resetAverages();
    active = false;
    gettimeofday(&lastGoodRead, NULL);
#endif
}




