//
//  main.cpp
//  AudioInjector2
//
//  Created by Leonardo Murillo on 3/5/13.
//  Copyright (c) 2013 Leonardo Murillo. All rights reserved.
//


#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <fstream>
#include <bitset>
#include <algorithm>
#include <arpa/inet.h>
#include <string.h>
#include <sstream>
#include "UDPSocket.h"
#include "UDPSocket.cpp"
#include <SharedUtil.h>

char EC2_WEST_AUDIO_SERVER[] = "54.241.92.53";
const int AUDIO_UDP_LISTEN_PORT = 55443;
const int BUFFER_LENGTH_BYTES = 512;
const int BUFFER_LENGTH_SAMPLES = BUFFER_LENGTH_BYTES / sizeof(int16_t);
const float SAMPLE_RATE = 22050.0;
const float BUFFER_SEND_INTERVAL_USECS = (BUFFER_LENGTH_SAMPLES / SAMPLE_RATE) * 1000000;

// Command line parameter defaults
bool loopAudio = true;
float sleepIntervalMin = 1.00;
float sleepIntervalMax = 2.00;
float positionInUniverse[] = {0, 0, 0, 0};
char attenuationModifier[] = {0};
char *sourceAudioFile;
const char *allowedParameters = ":rb::t::c::a::f:";

char *charBuffer;
int16_t *buffer;
long length;

UDPSocket *streamSocket;

void printBinaryValue(char element) {
    std::bitset<8> x(element);
    std::cout << "Printing binary value: " << x << std::endl;
}

float randomFloat(float a, float b) {
    float random = ((float) rand()) / (float) RAND_MAX;
    float diff = b - a;
    float r = random * diff;
    return a + r;
}

void usage(void)
{
    std::cout << "High Fidelity - Interface audio injector" << std::endl;
    std::cout << "   -r                             Random sleep mode. If not specified will default to constant loop." << std::endl;
    std::cout << "   -b FLOAT                       Min. number of seconds to sleep. Only valid in random sleep mode. Default 1.0" << std::endl;
    std::cout << "   -t FLOAT                       Max. number of seconds to sleep. Only valid in random sleep mode. Default 2.0" << std::endl;
    std::cout << "   -c FLOAT,FLOAT,FLOAT,FLOAT     X,Y,Z,YAW position in universe where audio will be originating from and direction. Defaults to 0,0,0,0" << std::endl;
    std::cout << "   -a 0-255                       Attenuation curve modifier, defaults to 0" << std::endl;
    std::cout << "   -f FILENAME                    Name of audio source file. Required - RAW format, 22050hz 16bit signed mono" << std::endl;
};

bool processParameters(int parameterCount, char* parameterData[])
{
    int p;
    while ((p = getopt(parameterCount, parameterData, allowedParameters)) != -1) {
        switch (p) {
            case 'r':
                loopAudio = false;
                std::cout << "[DEBUG] Random sleep mode enabled" << std::endl;
                break;
            case 'b':
                sleepIntervalMin = atof(optarg);
                std::cout << "[DEBUG] Min delay between plays " << sleepIntervalMin << "sec" << std::endl;
                break;
            case 't':
                sleepIntervalMax = atof(optarg);
                std::cout << "[DEBUG] Max delay between plays " << sleepIntervalMax << "sec" << std::endl;
                break;
            case 'f':
                sourceAudioFile = optarg;
                std::cout << "[DEBUG] Opening file: " << sourceAudioFile << std::endl;
                break;
            case 'c':
            {
                std::istringstream ss(optarg);
                std::string token;
                
                int i = 0;
                while (std::getline(ss, token, ',')) {
                    positionInUniverse[i] = atof(token.c_str());
                    ++i;
                    if (i == 4) {
                        break;
                    }
                }
                 
                break;
            }
            case 'a':
                attenuationModifier[0] = atoi(optarg);
                std::cout << "[DEBUG] Attenuation modifier: " << optarg << std::endl;
                break;
            default:
                usage();
                return false;
        }
    }
    return true;
};

void loadFile(void) {
    std::fstream sourceFile;
    sourceFile.open(sourceAudioFile, std::ios::in | std::ios::binary);
    sourceFile.seekg(0, std::ios::end);
    length = sourceFile.tellg();
    sourceFile.seekg(0, std::ios::beg);
    long sizeOfShortArray = length / 2;
    buffer = new int16_t[sizeOfShortArray];
    sourceFile.read((char *)buffer, length);
}

void stream(void)
{
    timeval startTime;
    
    int leadingBytes = 1 + (sizeof(float) * 4);
    unsigned char dataPacket[BUFFER_LENGTH_BYTES + leadingBytes];
    
    dataPacket[0] = 'I';
    unsigned char *currentPacketPtr = dataPacket + 1;
    
    for (int p = 0; p < 4; p++) {
        memcpy(currentPacketPtr, &positionInUniverse[p], sizeof(float));
        currentPacketPtr += sizeof(float);
    }
    
    memcpy(currentPacketPtr, &attenuationModifier[0], sizeof(char));
    currentPacketPtr += sizeof(char);
    
    for (int i = 0; i < length; i += BUFFER_LENGTH_SAMPLES) {
        gettimeofday(&startTime, NULL);
        memcpy(currentPacketPtr, &buffer[i], BUFFER_LENGTH_BYTES);
        streamSocket->send(EC2_WEST_AUDIO_SERVER, AUDIO_UDP_LISTEN_PORT, dataPacket, sizeof(dataPacket));
        double usecToSleep = usecTimestamp(&startTime) + BUFFER_SEND_INTERVAL_USECS - usecTimestampNow();
        usleep(usecToSleep);
    }
};

int main(int argc, char* argv[])
{

    srand(time(0));
    int AUDIO_UDP_SEND_PORT = 1500 + (rand() % (int)(1500 - 2000 + 1));
    
    UDPSocket *streamSocket = new UDPSocket(AUDIO_UDP_SEND_PORT);
    
    if (processParameters(argc, argv)) {
        if (sourceAudioFile) {
            loadFile();
        } else {
            std::cout << "[FATAL] Source audio file not specified" << std::endl;
            exit(-1);
        }
        
        for (int i = 0; i < sizeof(positionInUniverse)/sizeof(positionInUniverse[0]); ++i) {
            std::cout << "Position " << positionInUniverse[i] << std::endl;
        }

        float delay;
        int usecDelay;
        while (1) {
            stream();
            
            if (loopAudio) {
                delay = 0;
            } else {
                delay = randomFloat(sleepIntervalMin, sleepIntervalMax);
            }
            usecDelay = delay * 1000 * 1000;
            usleep(usecDelay);
        }
    }
    return 0;
}


