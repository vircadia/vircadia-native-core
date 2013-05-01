//
//  main.cpp
//  invensense-test
//
//  Created by Stephen Birarda on 4/30/13.
//  Copyright (c) 2013 High Fidelity. All rights reserved.
//

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <dirent.h>
#include <SharedUtil.h>

int main(int argc, char* argv[]) {
    int serialFd = open("/dev/tty.usbmodemfa141", O_RDWR | O_NOCTTY | O_NDELAY);
    
    struct termios options;
    tcgetattr(serialFd, &options);
    
    cfsetispeed(&options,B115200);
    cfsetospeed(&options,B115200);
    
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    tcsetattr(serialFd,TCSANOW,&options);
    
    // Flush the port's buffers (in and out) before we start using it
    tcflush(serialFd, TCIOFLUSH);
    
    // block on read until data is available
    int currentFlags = fcntl(serialFd, F_GETFL);    
    fcntl(serialFd, F_SETFL, currentFlags & ~O_NONBLOCK);
    
    write(serialFd, "WR686B01\n", 9);
    usleep(100000);
    
    tcflush(serialFd, TCIOFLUSH);
    
    char gyroBuffer[12] = {};
    int16_t gyroX, gyroY, gyroZ;
    
    char statusByte[4] = {};
    int dataReadyVal = 0;
    
    while (true) {
        write(serialFd, "RS3A\n", 5);
        read(serialFd, statusByte, 4);
        
        printf("The status byte read was %c%c%c%c\n", statusByte[0], statusByte[1], statusByte[2], statusByte[3]);
        dataReadyVal = statusByte[3] - '0';
        
        if (dataReadyVal % 2 == 1) {
            tcflush(serialFd, TCIOFLUSH);
            
            write(serialFd, "RD684306\n", 9);
            printf("Read %ld bytes from serial\n", read(serialFd, gyroBuffer, 12));
            printf("The first six chars %c%c%c%c%c%c\n", gyroBuffer[0], gyroBuffer[1], gyroBuffer[2], gyroBuffer[3], gyroBuffer[4], gyroBuffer[5]);

            gyroX = (gyroBuffer[6] << 8) | gyroBuffer[7];
            gyroY = (gyroBuffer[8] << 8) | gyroBuffer[9];
            gyroZ = (gyroBuffer[10] << 8) | gyroBuffer[11];
            
            printf("The gyro data %d, %d, %d\n", gyroX, gyroY, gyroZ);
        }
        
        tcflush(serialFd, TCIOFLUSH);
    }
    
    close(serialFd);
    
    return 0;
}


