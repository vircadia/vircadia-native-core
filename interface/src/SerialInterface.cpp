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
#endif

int serialFd;
const int MAX_BUFFER = 255;
char serialBuffer[MAX_BUFFER];
int serialBufferPos = 0;

const int ZERO_OFFSET = 2048;
const short NO_READ_MAXIMUM_MSECS = 3000;
const short SAMPLES_TO_DISCARD = 100;               //  Throw out the first few samples
const int GRAVITY_SAMPLES = 200;                    //  Use the first samples to compute gravity vector

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

//  Connect to the serial port
int SerialInterface::initializePort(char* portname, int baud)
{
#ifdef __APPLE__
    serialFd = open(portname, O_RDWR | O_NOCTTY | O_NDELAY);
    
    printLog("Opening SerialUSB %s: ", portname);
    
    if (serialFd == -1) {
        printLog("Failed.\n");
        return -1;     //  Failed to open port
    }    
    struct termios options;
    tcgetattr(serialFd,&options);
    switch(baud)
    {
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
    tcsetattr(serialFd,TCSANOW,&options);

    
    printLog("Connected.\n");
    resetSerial();    
    active = true;
 #endif
   
    return 0;
}

//  Reset Trailing averages to the current measurement
void SerialInterface::resetTrailingAverages() {
    for (int i = 1; i < NUM_CHANNELS; i++)  trailingAverage[i] = lastMeasured[i];
}

//  Render the serial interface channel values onscreen as vertical lines
void SerialInterface::renderLevels(int width, int height) {
    int i;
    int disp_x = 10;
    const int GAP = 16;
    char val[10];
    for(i = 0; i < NUM_CHANNELS; i++)
    {
        
        //  Actual value
        glLineWidth(2.0);
        glColor4f(1, 1, 1, 1);
        glBegin(GL_LINES);
        glVertex2f(disp_x, height*0.95);
        glVertex2f(disp_x, height*(0.25 + 0.75f*getValue(i)/4096));
        glColor4f(1, 0, 0, 1);
        glVertex2f(disp_x - 3, height*(0.25 + 0.75f*getValue(i)/4096));
        glVertex2f(disp_x, height*(0.25 + 0.75f*getValue(i)/4096));
        glEnd();
        //  Trailing Average value
        glBegin(GL_LINES);
        glColor4f(1, 1, 1, 1);
        glVertex2f(disp_x, height*(0.25 + 0.75f*getTrailingValue(i)/4096));
        glVertex2f(disp_x + 4, height*(0.25 + 0.75f*getTrailingValue(i)/4096));
        glEnd();
        
        sprintf(val, "%d", getValue(i));
        drawtext(disp_x-GAP/2, (height*0.95)+2, 0.08, 90, 1.0, 0, val, 0, 1, 0);
        
        disp_x += GAP;
    }
    //  Display Serial latency block
    if (LED) {
        glColor3f(1,0,0);
        glBegin(GL_QUADS); {
            glVertex2f(width - 100, height - 100);
            glVertex2f(width, height - 100);
            glVertex2f(width, height);
            glVertex2f(width - 100, height);
        }
        glEnd();
    }

}
void SerialInterface::readData() {
#ifdef __APPLE__
    //  This array sets the rate of trailing averaging for each channel:
    //  If the sensor rate is 100Hz, 0.001 will make the long term average a 10-second average
    const float AVG_RATE[] =  {0.002, 0.002, 0.002, 0.002, 0.002, 0.002};
    char bufchar[1];
    
    int initialSamples = totalSamples;
    
    while (read(serialFd, &bufchar, 1) > 0) { 
        //printLof("%c", bufchar[0]);
        serialBuffer[serialBufferPos] = bufchar[0];
        serialBufferPos++;
        //  Have we reached end of a line of input?
        if ((bufchar[0] == '\n') || (serialBufferPos >= MAX_BUFFER)) {
            std::string serialLine(serialBuffer, serialBufferPos-1);
            //printLog("%s\n", serialLine.c_str());
            int spot;
            //int channel = 0;
            std::string val;
            for (int i = 0; i < NUM_CHANNELS + 2; i++) {
                spot = serialLine.find_first_of(" ", 0);
                if (spot != std::string::npos) {
                    val = serialLine.substr(0,spot);
                    //printLog("%s\n", val.c_str());
                    if (i < NUM_CHANNELS) lastMeasured[i] = atoi(val.c_str());
                    else samplesAveraged = atoi(val.c_str());
                } else LED = atoi(serialLine.c_str());
                serialLine = serialLine.substr(spot+1, serialLine.length() - spot - 1);
            }
            
            //  Update Trailing Averages
            for (int i = 0; i < NUM_CHANNELS; i++) {
                if (totalSamples > SAMPLES_TO_DISCARD) {
                    trailingAverage[i] = (1.f - AVG_RATE[i])*trailingAverage[i] +
                        AVG_RATE[i]*(float)lastMeasured[i];
                } else {
                    trailingAverage[i] = (float)lastMeasured[i];
                }
                   
            }
            
            //  Use a set of initial samples to compute gravity 
            if (totalSamples < GRAVITY_SAMPLES) {
                gravity.x += lastMeasured[ACCEL_X];
                gravity.y += lastMeasured[ACCEL_Y];
                gravity.z += lastMeasured[ACCEL_Z];
            }
            if (totalSamples == GRAVITY_SAMPLES) {
                gravity = glm::normalize(gravity);
                printLof("gravity: %f,%f,%f\n", gravity.x, gravity.y, gravity.z);
            }

            totalSamples++;
            serialBufferPos = 0;
        }
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
    gravity = glm::vec3(0,-1,0);
    
    gettimeofday(&lastGoodRead, NULL);
    
    //  Clear the measured and average channel data
    for (int i = 0; i < NUM_CHANNELS; i++) {
        lastMeasured[i] = 0;
        trailingAverage[i] = 0.0;
    }
    //  Clear serial input buffer
    for (int i = 1; i < MAX_BUFFER; i++) {
        serialBuffer[i] = ' ';
    }
#endif
}




