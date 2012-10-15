//
//  SerialInterface.cpp
//
//  Used to read Arduino/Maple ADC data from an external device using a serial port (over USB)
//

#include "SerialInterface.h"
#include <iostream>

// These includes are for serial port reading/writing
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>


int serial_fd;

const int MAX_BUFFER = 100;
char serial_buffer[MAX_BUFFER];
int serial_buffer_pos = 0;

//  For accessing the serial port 
int init_port(int baud)
{
    serial_fd = open(SERIAL_PORT_NAME, O_RDWR | O_NOCTTY | O_NDELAY); 

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
    
    return 0;                   //  Success!
}

int read_sensors(int first_measurement, float * avg_adc_channels, int * adc_channels)
{
    int samples_read = 0;
    const float AVG_RATE =  0.001;  // 0.00001;
    char bufchar[1];
    while (read(serial_fd, bufchar, 1) > 0)
    {
        serial_buffer[serial_buffer_pos] = bufchar[0];
        serial_buffer_pos++;
        //  Have we reached end of a line of input?
        if ((bufchar[0] == '\n') || (serial_buffer_pos >= MAX_BUFFER))
        {
            samples_read++;
            //  At end - Extract value from string to variables
            if (serial_buffer[0] != 'p')
            {
                sscanf(serial_buffer, "%d %d %d %d %d %d %d", 
                       &adc_channels[0], 
                       &adc_channels[1], 
                       &adc_channels[2], 
                       &adc_channels[3],
                       &adc_channels[4],
                       &adc_channels[5],
                       &adc_channels[6]);
                for (int i = 0; i < NUM_CHANNELS; i++)
                {
                    if (!first_measurement)
                        avg_adc_channels[i] = (1.f - AVG_RATE)*avg_adc_channels[i] + 
                        AVG_RATE*(float)adc_channels[i];
                    else
                    {
                        avg_adc_channels[i] = (float)adc_channels[i];
                    }
                }
            }
            //  Clear rest of string for printing onscreen
            while(serial_buffer_pos++ < MAX_BUFFER) serial_buffer[serial_buffer_pos] = ' ';
            serial_buffer_pos = 0;
        }
    }
   
    return samples_read;
}



