//
//  AudioInjector.h
//  hifi
//
//  Created by Stephen Birarda on 4/23/13.
//
//

#ifndef __hifi__AudioInjector__
#define __hifi__AudioInjector__

#include <iostream>
#include <netinet/in.h>

#include "UDPSocket.h"

class AudioInjector {
public:
    AudioInjector(const char* filename);
    ~AudioInjector();
    
    void setPosition(float* position);
    void setBearing(float bearing) { _bearing = bearing; }
    void setAttenuationModifier(unsigned char attenuationModifier) { _attenuationModifier = attenuationModifier; }
    
    void injectAudio(UDPSocket* injectorSocket, sockaddr* destinationSocket);
private:
    int16_t* _audioSampleArray;
    int _numTotalBytesAudio;
    float _position[3];
    float _bearing;
    unsigned char _attenuationModifier;
};

#endif /* defined(__hifi__AudioInjector__) */
