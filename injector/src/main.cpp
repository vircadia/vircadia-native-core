//
//  main.cpp
//  Audio Injector
//
//  Created by Leonardo Murillo on 3/5/13.
//  Copyright (c) 2013 Leonardo Murillo. All rights reserved.
//


#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sstream>


#include <SharedUtil.h>
#include <PacketHeaders.h>
#include <UDPSocket.h>
#include <AudioInjector.h>

char EC2_WEST_AUDIO_SERVER[] = "54.241.92.53";
const int AUDIO_UDP_LISTEN_PORT = 55443;

// Command line parameter defaults
bool loopAudio = true;
float sleepIntervalMin = 1.00;
float sleepIntervalMax = 2.00;
char *sourceAudioFile = NULL;
const char *allowedParameters = ":rb::t::c::a::f:";
float floatArguments[4] = {0.0f, 0.0f, 0.0f, 0.0f};
unsigned char attenuationModifier = 255;

void usage(void)
{
    std::cout << "High Fidelity - Interface audio injector" << std::endl;
    std::cout << "   -r                             Random sleep mode. If not specified will default to constant loop." << std::endl;
    std::cout << "   -b FLOAT                       Min. number of seconds to sleep. Only valid in random sleep mode. Default 1.0" << std::endl;
    std::cout << "   -t FLOAT                       Max. number of seconds to sleep. Only valid in random sleep mode. Default 2.0" << std::endl;
    std::cout << "   -c FLOAT,FLOAT,FLOAT,FLOAT     X,Y,Z,YAW position in universe where audio will be originating from and direction. Defaults to 0,0,0,0" << std::endl;
    std::cout << "   -a 0-255                       Attenuation curve modifier, defaults to 255" << std::endl;
    std::cout << "   -f FILENAME                    Name of audio source file. Required - RAW format, 22050hz 16bit signed mono" << std::endl;
};

bool processParameters(int parameterCount, char* parameterData[])
{
    int p;
    while ((p = getopt(parameterCount, parameterData, allowedParameters)) != -1) {
        switch (p) {
            case 'r':
                ::loopAudio = false;
                std::cout << "[DEBUG] Random sleep mode enabled" << std::endl;
                break;
            case 'b':
                ::sleepIntervalMin = atof(optarg);
                std::cout << "[DEBUG] Min delay between plays " << sleepIntervalMin << "sec" << std::endl;
                break;
            case 't':
                ::sleepIntervalMax = atof(optarg);
                std::cout << "[DEBUG] Max delay between plays " << sleepIntervalMax << "sec" << std::endl;
                break;
            case 'f':
                ::sourceAudioFile = optarg;
                std::cout << "[DEBUG] Opening file: " << sourceAudioFile << std::endl;
                break;
            case 'c':
            {
                std::istringstream ss(optarg);
                std::string token;
                
                int i = 0;
                while (std::getline(ss, token, ',')) {
                    ::floatArguments[i] = atof(token.c_str());
                    ++i;
                    if (i == 4) {
                        break;
                    }
                }
                 
                break;
            }
            case 'a':
                ::attenuationModifier = atoi(optarg);
                std::cout << "[DEBUG] Attenuation modifier: " << optarg << std::endl;
                break;
            default:
                usage();
                return false;
        }
    }
    return true;
};

int main(const int argc, const char* argv[]) {

    srand(time(0));
    int AUDIO_UDP_SEND_PORT = 1500 + (rand() % (int)(1500 - 2000 + 1));
    
    UDPSocket streamSocket(AUDIO_UDP_SEND_PORT);
    
    sockaddr_in mixerSocket;
    mixerSocket.sin_family = AF_INET;
    mixerSocket.sin_addr.s_addr = inet_addr(EC2_WEST_AUDIO_SERVER);
    mixerSocket.sin_port = htons((uint16_t)AUDIO_UDP_LISTEN_PORT);
    
    if (processParameters(argc, argv)) {        
        if (::sourceAudioFile == NULL) {
            std::cout << "[FATAL] Source audio file not specified" << std::endl;
            exit(-1);
        } else {
            AudioInjector injector(sourceAudioFile);
            
            injector.setPosition(::floatArguments);
            injector.setBearing(*(::floatArguments + 3));
            injector.setAttenuationModifier(::attenuationModifier);
        
            float delay = 0;
            int usecDelay = 0;
            
            while (true) {
                injector.injectAudio(&streamSocket, (sockaddr*) &mixerSocket);
                
                if (!::loopAudio) {
                    delay = randFloatInRange(::sleepIntervalMin, ::sleepIntervalMax);
                    usecDelay = delay * 1000 * 1000;
                    usleep(usecDelay);
                }
            }
        }        
    }
    return 0;
}


