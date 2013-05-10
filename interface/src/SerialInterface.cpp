//
//  SerialInterface.cpp
//  2012 by Philip Rosedale for High Fidelity Inc.
//
//  Read interface data from the gyros/accelerometer board using SerialUSB
//
//  Channels are received in the following order (integer 0-4096 based on voltage 0-3.3v)
//
//  0 - AIN 15:  Pitch Gyro (nodding your head 'yes')
//  1 - AIN 16:  Yaw Gyro (shaking your head 'no')
//  2 - AIN 17:  Roll Gyro (looking quizzical, tilting your head)
//  3 - AIN 18:  Lateral acceleration (moving from side-to-side in front of your monitor)
//  4 - AIN 19:  Up/Down acceleration (sitting up/ducking in front of your monitor)
//  5 - AIN 20:  Forward/Back acceleration (Toward or away from your monitor)
//

#include "SerialInterface.h"

#ifdef __APPLE__
#include <regex.h>
#include <sys/time.h>
#include <string>
#endif

const int MAX_BUFFER = 255;
char serialBuffer[MAX_BUFFER];
int serialBufferPos = 0;

const int ZERO_OFFSET = 2048;
const short NO_READ_MAXIMUM_MSECS = 3000;
const short SAMPLES_TO_DISCARD = 100;               //  Throw out the first few samples
const int GRAVITY_SAMPLES = 60;                     //  Use the first samples to compute gravity vector

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
                
                initializePort(serialPortname, 115200);
                
                delete [] serialPortname;
            }
            regfree(&regex);
        }
        closedir(devDir);
    }    
#endif
}

//  connect to the serial port
void SerialInterface::initializePort(char* portname, int baud) {
#ifdef __APPLE__
    _serialDescriptor = open(portname, O_RDWR | O_NOCTTY | O_NDELAY);
    
    printLog("Opening SerialUSB %s: ", portname);
    
    if (_serialDescriptor == -1) {
        printLog("Failed.\n");
        return;
    }    
    struct termios options;
    tcgetattr(_serialDescriptor, &options);
    
    switch(baud) {
		case 9600: cfsetispeed(&options,B9600);
			cfsetospeed(&options,B9600);
			break;
		case 19200: cfsetispeed(&options,B19200);
			cfsetospeed(&options,B19200);
			break;
		case 38400: cfsetispeed(&options,B38400);
			cfsetospeed(&options,B38400);
			break;
		case 115200: cfsetispeed(&options,B115200);
			cfsetospeed(&options,B115200);
			break;
		default:cfsetispeed(&options,B9600);
			cfsetospeed(&options,B9600);
			break;
    }
    
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    tcsetattr(_serialDescriptor, TCSANOW, &options);
    
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
        sprintf(val, "Yaw   %4.1f", _lastYawRate);
        drawtext(LEVEL_CORNER_X, LEVEL_CORNER_Y, 0.10, 0, 1.0, 1, val, 0, 1, 0);
        sprintf(val, "Pitch %4.1f", _lastPitchRate);
        drawtext(LEVEL_CORNER_X, LEVEL_CORNER_Y + 15, 0.10, 0, 1.0, 1, val, 0, 1, 0);
        sprintf(val, "Roll  %4.1f", _lastRollRate);
        drawtext(LEVEL_CORNER_X, LEVEL_CORNER_Y + 30, 0.10, 0, 1.0, 1, val, 0, 1, 0);
        sprintf(val, "X     %4.3f", _lastAccelX);
        drawtext(LEVEL_CORNER_X, LEVEL_CORNER_Y + 45, 0.10, 0, 1.0, 1, val, 0, 1, 0);
        sprintf(val, "Y     %4.3f", _lastAccelY);
        drawtext(LEVEL_CORNER_X, LEVEL_CORNER_Y + 60, 0.10, 0, 1.0, 1, val, 0, 1, 0);
        sprintf(val, "Z     %4.3f", _lastAccelZ);
        drawtext(LEVEL_CORNER_X, LEVEL_CORNER_Y + 75, 0.10, 0, 1.0, 1, val, 0, 1, 0);
        
        //  Draw the levels as horizontal lines        
        const int LEVEL_CENTER = 150;
        const float ACCEL_VIEW_SCALING = 50.f;
        glLineWidth(2.0);
        glColor4f(1, 1, 1, 1);
        glBegin(GL_LINES);
        // Gyro rates
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y - 3);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + _lastYawRate, LEVEL_CORNER_Y - 3);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 12);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + _lastPitchRate, LEVEL_CORNER_Y + 12);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 27);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + _lastRollRate, LEVEL_CORNER_Y + 27);
        // Acceleration
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 42);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + (int)((_lastAccelX - _gravity.x)* ACCEL_VIEW_SCALING),
                   LEVEL_CORNER_Y + 42);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 57);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + (int)((_lastAccelY - _gravity.y) * ACCEL_VIEW_SCALING),
                   LEVEL_CORNER_Y + 57);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 72);
        glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + (int)((_lastAccelZ - _gravity.z) * ACCEL_VIEW_SCALING),
                   LEVEL_CORNER_Y + 72);

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
void SerialInterface::readData() {
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
        
        const float LSB_TO_METERS_PER_SECOND2 = 1.f / 16384.f * 9.80665f;
                                                                //  From MPU-9150 register map, with setting on
                                                                //  highest resolution = +/- 2G
        
        _lastAccelX = ((float) accelXRate) * LSB_TO_METERS_PER_SECOND2;
        _lastAccelY = ((float) accelYRate) * LSB_TO_METERS_PER_SECOND2;
        _lastAccelZ = ((float) -accelZRate) * LSB_TO_METERS_PER_SECOND2;
        
        int rollRate, yawRate, pitchRate;
        
        convertHexToInt(sensorBuffer + 22, rollRate);
        convertHexToInt(sensorBuffer + 26, yawRate);
        convertHexToInt(sensorBuffer + 30, pitchRate);
        
        //  Convert the integer rates to floats
        const float LSB_TO_DEGREES_PER_SECOND = 1.f / 16.4f;     //  From MPU-9150 register map, 2000 deg/sec.
        const float PITCH_BIAS = 2.0;                            //  Strangely, there is a small DC bias in the
                                                                 //  invensense pitch reading.  Gravity?
        
        _lastRollRate = ((float) rollRate) * LSB_TO_DEGREES_PER_SECOND;
        _lastYawRate = ((float) yawRate) * LSB_TO_DEGREES_PER_SECOND;
        _lastPitchRate = ((float) -pitchRate) * LSB_TO_DEGREES_PER_SECOND + PITCH_BIAS;
        
        //  Accumulate an initial reading for gravity
        //  Use a set of initial samples to compute gravity
        if (totalSamples < GRAVITY_SAMPLES) {
            _gravity.x += _lastAccelX;
            _gravity.y += _lastAccelY;
            _gravity.z += _lastAccelZ;
        }
        if (totalSamples == GRAVITY_SAMPLES) {
            _gravity /= (float) totalSamples;
            printLog("Gravity: %f\n", glm::length(_gravity));
        }

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

void SerialInterface::resetSerial() {
#ifdef __APPLE__
    active = false;
    totalSamples = 0;
    _gravity = glm::vec3(0, 0, 0);
    
    gettimeofday(&lastGoodRead, NULL);
    
#endif
}




