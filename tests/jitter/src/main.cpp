//
//  main.cpp
//  JitterTester
//
//  Created by Philip on 8/1/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include <iostream>
#ifdef _WINDOWS
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#endif
#include <stdio.h>

#include <MovingMinMaxAvg.h> // for MovingMinMaxAvg
#include <SharedUtil.h> // for usecTimestampNow

const quint64 MSEC_TO_USEC = 1000;

void runSend(const char* addressOption, int port, int gap, int size, int report);
void runReceive(const char* addressOption, int port, int gap, int size, int report);

int main(int argc, const char * argv[]) {
    if (argc != 7) {
        printf("usage: jitter-tests <--send|--receive> <address> <port> <gap in usecs> <packet size> <report interval in msecs>\n");
        exit(1);
    }
    const char* typeOption = argv[1];
    const char* addressOption = argv[2];
    const char* portOption = argv[3];
    const char* gapOption = argv[4];
    const char* sizeOption = argv[5];
    const char* reportOption = argv[6];
    int port = atoi(portOption);
    int gap = atoi(gapOption);
    int size = atoi(sizeOption);
    int report = atoi(reportOption);

    std::cout << "type:" << typeOption << "\n";
    std::cout << "address:" << addressOption << "\n";
    std::cout << "port:" << port << "\n";
    std::cout << "gap:" << gap << "\n";
    std::cout << "size:" << size << "\n";

    if (strcmp(typeOption, "--send") == 0) {
        runSend(addressOption, port, gap, size, report);
    } else if (strcmp(typeOption, "--receive") == 0) {
        runReceive(addressOption, port, gap, size, report);
    }
    exit(1);
}

void runSend(const char* addressOption, int port, int gap, int size, int report) {
    std::cout << "runSend...\n";

    int sockfd;
    struct sockaddr_in servaddr;
    
    char* outputBuffer = new char[size];
    memset(outputBuffer, 0, size);
    
    sockfd=socket(AF_INET,SOCK_DGRAM,0);
    
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=inet_addr(addressOption);
    servaddr.sin_port=htons(port);
    
    const int SAMPLES_FOR_30_SECONDS = 30 * 1000000 / gap;
    MovingMinMaxAvg<int> timeGaps(1, SAMPLES_FOR_30_SECONDS); // stats
 
    quint64 last = usecTimestampNow();
    quint64 lastReport = 0;
    
    while (true) {

        quint64 now = usecTimestampNow();
        int actualGap = now - last;
        
        
        if (actualGap >= gap) {
            sendto(sockfd, outputBuffer, size, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
            
            int gapDifferece = actualGap - gap;
            timeGaps.update(gapDifferece);
            last = now;

            if (now - lastReport >= (report * MSEC_TO_USEC)) {
                std::cout << "SEND gap Difference From Expected "
                          << "min: " << timeGaps.getMin() << " "
                          << "max: " << timeGaps.getMax() << " "
                          << "avg: " << timeGaps.getAverage() << " "
                          << "min last 30: " << timeGaps.getWindowMin() << " "
                          << "max last 30: " << timeGaps.getWindowMax() << " "
                          << "avg last 30: " << timeGaps.getWindowAverage() << " "
                          << "\n";
                lastReport = now;
            }
        }
    }
}

void runReceive(const char* addressOption, int port, int gap, int size, int report) {
    std::cout << "runReceive...\n";


    int sockfd,n;
    struct sockaddr_in myaddr;
    
    char* inputBuffer = new char[size];
    memset(inputBuffer, 0, size);
    
    sockfd=socket(AF_INET, SOCK_DGRAM, 0);
    
    memset(&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    myaddr.sin_port=htons(port);

    const int SAMPLES_FOR_30_SECONDS = 30 * 1000000 / gap;
    MovingMinMaxAvg<int> timeGaps(1, SAMPLES_FOR_30_SECONDS); // stats
    
    if (bind(sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        std::cout << "bind failed\n";
        return;
    }    
    
    quint64 last = 0; // first case
    quint64 lastReport = 0;
    
    while (true) {
        n = recvfrom(sockfd, inputBuffer, size, 0, NULL, NULL); // we don't care about where it came from
        
        if (last == 0) {
            last = usecTimestampNow();
            std::cout << "first packet received\n";
        } else {
            quint64 now = usecTimestampNow();
            int actualGap = now - last;
            int gapDifferece = actualGap - gap;
            timeGaps.update(gapDifferece);
            last = now;
            
            if (now - lastReport >= (report * MSEC_TO_USEC)) {
                std::cout << "RECEIVE gap Difference From Expected "
                          << "min: " << timeGaps.getMin() << " "
                          << "max: " << timeGaps.getMax() << " "
                          << "avg: " << timeGaps.getAverage() << " "
                          << "min last 30: " << timeGaps.getWindowMin() << " "
                          << "max last 30: " << timeGaps.getWindowMax() << " "
                          << "avg last 30: " << timeGaps.getWindowAverage() << " "
                          << "\n";
                lastReport = now;
            }
        }
    }
}

