//
//  SerialInterface.cpp
//  2012 by Philip Rosedale for High Fidelity Inc.
//
//  Read interface data from the gyros/accelerometer Invensense board using the SerialUSB
//

#ifndef _WIN32
#include <regex.h>
#include <sys/time.h>
#include <string>
#endif

#include <math.h>

#include <glm/gtx/vector_angle.hpp>

extern "C" {
    #include <inv_tty.h>
    #include <inv_mpu.h>
}

#include <SharedUtil.h>

#include "Application.h"
#include "SerialInterface.h"
#include "Util.h"
#include "Webcam.h"

const short NO_READ_MAXIMUM_MSECS = 3000;
const int GRAVITY_SAMPLES = 60;                     //  Use the first few samples to baseline values
const int NORTH_SAMPLES = 30;
const int ACCELERATION_SENSOR_FUSION_SAMPLES = 20;
const int COMPASS_SENSOR_FUSION_SAMPLES = 200;
const int LONG_TERM_RATE_SAMPLES = 1000;            

const bool USING_INVENSENSE_MPU9150 = 1;

void SerialInterface::pair() {
    
#ifndef _WIN32
    // look for a matching gyro setup
    DIR *devDir;
    struct dirent *entry;
    int matchStatus;
    regex_t regex;
    
    // for now this only works on OS X, where the usb serial shows up as /dev/tty.usb*,
    // and (possibly just Ubuntu) Linux, where it shows up as /dev/ttyACM*
    if((devDir = opendir("/dev"))) {
        while((entry = readdir(devDir))) {
#ifdef __APPLE__
            regcomp(&regex, "tty\\.usb", REG_EXTENDED|REG_NOSUB);
#else
            regcomp(&regex, "ttyACM", REG_EXTENDED|REG_NOSUB);
#endif
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
#ifndef _WIN32
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
        
        // this disables streaming so there's no garbage data on reads
        write(_serialDescriptor, "SD\n", 3);
        char result[4];
        read(_serialDescriptor, result, 4);
        
        tty_set_file_descriptor(_serialDescriptor);
        mpu_init(0);
        mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL | INV_XYZ_COMPASS);
    }
    
    printLog("Connected.\n");
    resetSerial();
    
    _active = true;
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

void SerialInterface::readData(float deltaTime) {
#ifndef _WIN32
    
    int initialSamples = totalSamples;
    
    if (USING_INVENSENSE_MPU9150) { 

        // ask the invensense for raw gyro data
        short accelData[3];
        mpu_get_accel_reg(accelData, 0);
        
        const float LSB_TO_METERS_PER_SECOND2 = 1.f / 16384.f * GRAVITY_EARTH;
                                                                //  From MPU-9150 register map, with setting on
                                                                //  highest resolution = +/- 2G
        
        _lastAcceleration = glm::vec3(-accelData[2], -accelData[1], -accelData[0]) * LSB_TO_METERS_PER_SECOND2;
          
        short gyroData[3];
        mpu_get_gyro_reg(gyroData, 0);
        
        //  Convert the integer rates to floats
        const float LSB_TO_DEGREES_PER_SECOND = 1.f / 16.4f;     //  From MPU-9150 register map, 2000 deg/sec.
        glm::vec3 rotationRates;
        rotationRates[0] = ((float) -gyroData[2]) * LSB_TO_DEGREES_PER_SECOND;
        rotationRates[1] = ((float) -gyroData[1]) * LSB_TO_DEGREES_PER_SECOND;
        rotationRates[2] = ((float) -gyroData[0]) * LSB_TO_DEGREES_PER_SECOND;
      
        short compassData[3];
        mpu_get_compass_reg(compassData, 0);
      
        // Convert integer values to floats, update extents
        _lastCompass = glm::vec3(compassData[2], -compassData[0], -compassData[1]);
        
        // update and subtract the long term average
        _averageRotationRates = (1.f - 1.f/(float)LONG_TERM_RATE_SAMPLES) * _averageRotationRates +
                1.f/(float)LONG_TERM_RATE_SAMPLES * rotationRates;
        rotationRates -= _averageRotationRates;

        // compute the angular acceleration
        glm::vec3 angularAcceleration = (deltaTime < EPSILON) ? glm::vec3() : (rotationRates - _lastRotationRates) / deltaTime;
        _lastRotationRates = rotationRates;
        
        //  Update raw rotation estimates
        glm::quat estimatedRotation = glm::quat(glm::radians(_estimatedRotation)) *
            glm::quat(glm::radians(deltaTime * _lastRotationRates));
        
        //  Update acceleration estimate: first, subtract gravity as rotated into current frame
        _estimatedAcceleration = (totalSamples < GRAVITY_SAMPLES) ? glm::vec3() :
            _lastAcceleration - glm::inverse(estimatedRotation) * _gravity;
        
        // update and subtract the long term average
        _averageAcceleration = (1.f - 1.f/(float)LONG_TERM_RATE_SAMPLES) * _averageAcceleration +
                1.f/(float)LONG_TERM_RATE_SAMPLES * _estimatedAcceleration;
        _estimatedAcceleration -= _averageAcceleration;
        
        //  Consider updating our angular velocity/acceleration to linear acceleration mapping
        if (glm::length(_estimatedAcceleration) > EPSILON &&
                (glm::length(_lastRotationRates) > EPSILON || glm::length(angularAcceleration) > EPSILON)) {
            // compute predicted linear acceleration, find error between actual and predicted
            glm::vec3 predictedAcceleration = _angularVelocityToLinearAccel * _lastRotationRates +
                _angularAccelToLinearAccel * angularAcceleration;
            glm::vec3 error = _estimatedAcceleration - predictedAcceleration;
            
            // the "error" is actually what we want: the linear acceleration minus rotational influences
            _estimatedAcceleration = error;
            
            // adjust according to error in each dimension, in proportion to input magnitudes
            for (int i = 0; i < 3; i++) {
                if (fabsf(error[i]) < EPSILON) {
                    continue;
                }
                const float LEARNING_RATE = 0.001f;
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
        
        // rotate estimated acceleration into global rotation frame
        _estimatedAcceleration = estimatedRotation * _estimatedAcceleration;
        
        //  Update estimated position and velocity
        float const DECAY_VELOCITY = 0.975f;
        float const DECAY_POSITION = 0.975f;
        _estimatedVelocity += deltaTime * _estimatedAcceleration;
        _estimatedPosition += deltaTime * _estimatedVelocity;
        _estimatedVelocity *= DECAY_VELOCITY;
        
        //  Attempt to fuse gyro position with webcam position
        Webcam* webcam = Application::getInstance()->getWebcam();
        if (webcam->isActive()) {
            const float WEBCAM_POSITION_FUSION = 0.5f;
            _estimatedPosition = glm::mix(_estimatedPosition, webcam->getEstimatedPosition(), WEBCAM_POSITION_FUSION);
               
        } else {
            _estimatedPosition *= DECAY_POSITION;
        }
            
        //  Accumulate a set of initial baseline readings for setting gravity
        if (totalSamples == 0) {
            _gravity = _lastAcceleration;
        } 
        else {
            if (totalSamples < GRAVITY_SAMPLES) {
                _gravity = glm::mix(_gravity, _lastAcceleration, 1.0f / GRAVITY_SAMPLES);
                
                //  North samples start later, because the initial compass readings are screwy
                int northSample = totalSamples - (GRAVITY_SAMPLES - NORTH_SAMPLES);
                if (northSample == 0) {
                    _north = _lastCompass;
                    
                } else if (northSample > 0) {
                    _north = glm::mix(_north, _lastCompass, 1.0f / NORTH_SAMPLES);
                }
            } else {
                //  Use gravity reading to do sensor fusion on the pitch and roll estimation
                estimatedRotation = safeMix(estimatedRotation,
                    rotationBetween(estimatedRotation * _lastAcceleration, _gravity) * estimatedRotation,
                    1.0f / ACCELERATION_SENSOR_FUSION_SAMPLES);
                
                //  Update the compass extents
                _compassMinima = glm::min(_compassMinima, _lastCompass);
                _compassMaxima = glm::max(_compassMaxima, _lastCompass);
        
                //  Same deal with the compass heading
                estimatedRotation = safeMix(estimatedRotation,
                    rotationBetween(estimatedRotation * recenterCompass(_lastCompass),
                        recenterCompass(_north)) * estimatedRotation,
                    1.0f / COMPASS_SENSOR_FUSION_SAMPLES);
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
    _averageRotationRates = glm::vec3(0, 0, 0);
    _averageAcceleration = glm::vec3(0, 0, 0);
    _lastRotationRates = glm::vec3(0, 0, 0);
    _estimatedRotation = glm::vec3(0, 0, 0);
    _estimatedPosition = glm::vec3(0, 0, 0);
    _estimatedVelocity = glm::vec3(0, 0, 0);
    _estimatedAcceleration = glm::vec3(0, 0, 0);
}

void SerialInterface::resetSerial() {
#ifndef _WIN32
    resetAverages();
    _active = false;
    gettimeofday(&lastGoodRead, NULL);
#endif
}

glm::vec3 SerialInterface::recenterCompass(const glm::vec3& compass) {
    // compensate for "hard iron" distortion by subtracting the midpoint on each axis; see
    // http://www.sensorsmag.com/sensors/motion-velocity-displacement/compensating-tilt-hard-iron-and-soft-iron-effects-6475
    return compass - (_compassMinima + _compassMaxima) * 0.5f;
}


