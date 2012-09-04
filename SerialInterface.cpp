//
//  SerialInterface.cpp
//  interface
//
//  Created by Seiji Emery on 8/10/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "SerialInterface.h"
#include <iostream>

// These includes are for serial port reading/writing
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>


//  For accessing the serial port 
void init_port(int *fd, int baud)
{
    struct termios options;
    tcgetattr(*fd,&options);
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
    tcsetattr(*fd,TCSANOW,&options);
}

bool SerialInterface::enable () {
    if (!_enabled) {
        // Try to re-initialize the interface.
        // May fail (if open() fails), in which case an error code is emitted
        // (which we can safely ignore), and _enabled is reset to false
        initInterface();
    }
    return _enabled;
}

void SerialInterface::disable () {
	_enabled = false;
	closeInterface();
}


void SerialInterface::closeInterface () {
	close(_serial_fd);
	_enabled = false;
}

int SerialInterface::initInterface () {
	if (_enabled) {
		_serial_fd = open("/dev/tty.usbmodem411", O_RDWR  | O_NOCTTY | O_NDELAY); // List usbSerial devices using Terminal ls /dev/tty.*

		if (_serial_fd == -1) {
            std::cerr << "Unable to open serial port (" << _serial_fd << ")\n";
            _enabled = false;
			return 1;
		} else {
			init_port(&_serial_fd, 115200);
		}
	}
    return 0;
}

// Reads data from a serial interface and updates gyro and accelerometer state
// TODO: implement accelerometer
void SerialInterface::readSensors (float deltaTime) {
	// Note: changed to use binary format instead of plaintext
	// (Changed in balance_maple.pde as well (toggleable))
    
    
    /*
    int lines_read = 0;
    const float AVG_RATE = 0.00001;
    if (_enabled)
    {
        char bufchar[1];
        while (read(_serial_fd, bufchar, 1) > 0)
        {
            serial_buffer[serial_buffer_pos] = bufchar[0];
            serial_buffer_pos++;
            //  Have we reached end of a line of input?
            if ((bufchar[0] == '\n') || (serial_buffer_pos >= MAX_BUFFER))
            {
                lines_read++;
                //  At end - Extract value from string to variables
                if (serial_buffer[0] != 'p')
                {
                    samplecount++;
                    sscanf(serial_buffer, "%d %d %d %d", &adc_channels[0], 
                           &adc_channels[1], 
                           &adc_channels[2], 
                           &adc_channels[3]);
                    for (int i = 0; i < 4; i++)
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
            if (bufchar[0] == 'p')
            {
                gettimeofday(&end_ping, NULL);
                ping = diffclock(begin_ping,end_ping);
                display_ping = 1; 
            }
        }
    }
    return lines_read;
     */
     

    /*
	if (_enabled) {
        int _channels[CHANNEL_COUNT];
        _channels[0] = _channels[1] = _channels[2] = _channels[3] = 0;

        for (char c; read(_serial_fd, &c, 1) > 0; ) { // Read bytes from serial port
            if (_readPacket) { 
                // load byte into buffer
                _buffer[_bufferPos++] = c;
                if (_bufferPos > CHANNEL_COUNT * sizeof(int)) {  
                        // if buffer is full: load into channels
                    for (int i = 0; i < CHANNEL_COUNT; ++i) {
                        _channels[i] += *((int*)(_buffer + i * sizeof(int)));
                    }
                        //memcpy(_channels, _buffer, CHANNEL_COUNT * sizeof(int));
                        // And check for next opcode
                    _readPacket = false;
                }
            } else {
                // read opcode
                switch (c) {
                    case 'd':
                        _readPacket = true;
                        _bufferPos = 0;
                        break;
                    case 'p':
                            // TODO: implement pings
                        break;
                }
            }
        }

    }
     */
}



// Old read_sensors()
/*
// Collect sensor data from serial port 
void read_sensors(void)
{

    if (serial_on)
    {
        char bufchar[1];
        while (read(serial_fd, bufchar, 1) > 0)
        {
            serial_buffer[serial__bufferpos] = bufchar[0];
            serial__bufferpos++;
            //  Have we reached end of a line of input?
            if ((bufchar[0] == '\n') || (serial__bufferpos >= MAX_BUFFER))
            {
                //  At end - Extract value from string to variables
                if (serial_buffer[0] != 'p')
                {
                    samplecount++;
                    sscanf(serial_buffer, "%d %d %d %d", &adc_channels[0], 
                           &adc_channels[1], 
                           &adc_channels[2], 
                           &adc_channels[3]);
                    for (int i = 0; i < 4; i++)
                    {
                        if (!first_measurement)
                            avg_adc_channels[i] = (1.f - AVG_RATE)*avg_adc_channels[i] + 
                            AVG_RATE*(float)adc_channels[i];
                        else
                        {
                            avg_adc_channels[i] = (float)adc_channels[i];
                        }
                    }
                    first_measurement = 0;
                }
                //  Clear rest of string for printing onscreen
                while(serial__bufferpos++ < MAX_BUFFER) serial_buffer[serial__bufferpos] = ' ';
                serial__bufferpos = 0;
            }
            if (bufchar[0] == 'p')
            {
                gettimeofday(&end_ping, NULL);
                ping = diffclock(begin_ping,end_ping);
                display_ping = 1; 
            }
        }
    }
}



*/