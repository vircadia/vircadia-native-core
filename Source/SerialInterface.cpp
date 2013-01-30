//
//  SerialInterface.cpp
//  2012 by Philip Rosedale for High Fidelity Inc.
//
//  Read interface data from the gyros/accelerometer board using SerialUSB
//
//  Channels are received in the following order (integer 0-4096 based on voltage 0-3.3v)
//
//  AIN 15:  Pitch Gyro (nodding your head 'yes')
//  AIN 16:  Yaw Gyro (shaking your head 'no')
//  AIN 17:  Roll Gyro (looking quizzical, tilting your head)
//  AIN 18:  Lateral acceleration (moving from side-to-side in front of your monitor)
//  AIN 19:  Up/Down acceleration (sitting up/ducking in front of your monitor)
//  AIN 20:  Forward/Back acceleration (Toward or away from your monitor)
//

#include "SerialInterface.h"

int serial_fd;
const int MAX_BUFFER = 100;
char serial_buffer[MAX_BUFFER];
int serial_buffer_pos = 0;
int samples_total = 0;

//  Init the serial port to the specified values 
int SerialInterface::init(char * portname, int baud)
{
    serial_fd = open(portname, O_RDWR | O_NOCTTY | O_NDELAY);
    
    if (serial_fd == -1) return -1;     //  Failed to open port
    
    struct termios options;
    tcgetattr(serial_fd,&options);
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
    tcsetattr(serial_fd,TCSANOW,&options);
    
    //  Clear the measured and average channel data
    for (int i = 1; i < NUM_CHANNELS; i++)
    {
        lastMeasured[i] = 0;
        trailingAverage[i] = 0.0;
    }

    return 0;                   //  Init serial port was a success
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
        glEnd();
        //  Trailing Average value
        glColor4f(1, 1, 0, 1);
        glBegin(GL_LINES);
        glVertex2f(disp_x + 2, height*0.95);
        glVertex2f(disp_x + 2, height*(0.25 + 0.75f*getTrailingValue(i)/4096));
        glEnd();
        
        glColor3f(1,0,0);
        glBegin(GL_LINES);
        glLineWidth(2.0);
        glVertex2f(disp_x - 10, height*0.5 - getRelativeValue(i));
        glVertex2f(disp_x + 10, height*0.5 - getRelativeValue(i));
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
    //  This array sets the rate of trailing averaging for each channel.
    //  If the sensor rate is 100Hz, 0.001 will make the long term average a 10-second average
    const float AVG_RATE[] =  {0.001, 0.001, 0.001, 0.001, 0.001, 0.001};
    char bufchar[1];
    while (read(serial_fd, bufchar, 1) > 0)
    {
        serial_buffer[serial_buffer_pos] = bufchar[0];
        serial_buffer_pos++;
        //  Have we reached end of a line of input?
        if ((bufchar[0] == '\n') || (serial_buffer_pos >= MAX_BUFFER))
        {
            //  At end - Extract value from string to variables
            if (serial_buffer[0] != 'p')
            {
                //  NOTE: This is stupid, need to rewrite to properly scan for NUM_CHANNELS + 2
                //
                sscanf(serial_buffer, "%d %d %d %d %d %d %d %d",    
                       &lastMeasured[0],
                       &lastMeasured[1],
                       &lastMeasured[2],
                       &lastMeasured[3],
                       &lastMeasured[4],
                       &lastMeasured[5],
                       &samplesAveraged,
                       &LED
                       );
                for (int i = 0; i < NUM_CHANNELS; i++)
                {
                    if (samples_total > 0)
                        trailingAverage[i] = (1.f - AVG_RATE[i])*trailingAverage[i] +
                        AVG_RATE[i]*(float)lastMeasured[i];
                    else
                    {
                        trailingAverage[i] = (float)lastMeasured[i];
                    }
                }
                samples_total++;
            }
            //  Clear rest of string for printing onscreen
            while(serial_buffer_pos++ < MAX_BUFFER) serial_buffer[serial_buffer_pos] = ' ';
            serial_buffer_pos = 0;
        }
    }
}




