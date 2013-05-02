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

void loadIntFromTwoHex(char* sourceBuffer, int& destinationInt) {
    
    unsigned int byte[2];
    
    for(int i = 0; i < 2; i++) {
        sscanf(sourceBuffer + 2 * i, "%2x", &byte[i]);
//        printf("The hex code is 0x%c%c so the byte is %d\n", (sourceBuffer + 2 * i)[0], (sourceBuffer + 2 * i)[1], byte[i]);
    }
    
    int16_t result = (byte[0] << 8);
    result += byte[1];
    
    destinationInt = result;
}

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
    write(serialFd, "SD\n", 3);
    usleep(100000);
    
    tcflush(serialFd, TCIOFLUSH);
    
    char gyroBuffer[20] = {};
    int gyroX, gyroY, gyroZ;
    int readBytes = 0;
    
    double lastRead = usecTimestampNow();
    
    while (true) {
        if (usecTimestampNow() - lastRead >= 10000) {
            
            write(serialFd, "RD684306\n", 9);
            
            readBytes = read(serialFd, gyroBuffer, 20);
//            printf("Read %d bytes from serial\n", readBytes);
            
//            printf("The first two characters are %c%c\n", gyroBuffer[0], gyroBuffer[1]);
            
            if (readBytes == 20 && gyroBuffer[0] == 'R' && gyroBuffer[1] == 'D') {
                loadIntFromTwoHex(gyroBuffer + 6, gyroX);
                loadIntFromTwoHex(gyroBuffer + 10, gyroY);
                loadIntFromTwoHex(gyroBuffer + 14, gyroZ);
                
                printf("The gyro data %d, %d, %d\n", gyroX, gyroY, gyroZ);
            }
            
            lastRead = usecTimestampNow();
        } 
    }
    
    close(serialFd);
    
    return 0;
}


