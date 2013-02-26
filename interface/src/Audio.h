//
//  Audio.h
//  interface
//
//  Created by Stephen Birarda on 1/22/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Audio__
#define __interface__Audio__

#include <iostream>
#include <portaudio.h>
#include "AudioData.h"
#include "Oscilloscope.h"

class Audio {
public:
    // initializes audio I/O
    Audio(Oscilloscope * s);
    
    void render();
    void render(int screenWidth, int screenHeight);
    
    void getInputLoudness(float * lastLoudness, float * averageLoudness);
    void updateMixerParams(in_addr_t mixerAddress, in_port_t mixerPort);
    
    void setSourcePosition(glm::vec3 position);
    
    // terminates audio I/O
    bool terminate();
private:    
    bool initialized;
    AudioData *audioData;
    
    // protects constructor so that public init method is used
    Audio();
    
    // hold potential error returned from PortAudio functions
    PaError paError;
    
    // audio stream handle
    PaStream *stream;
    
    // audio receive thread
    pthread_t audioReceiveThread;
    
    // give access to AudioData class from audioCallback
    friend int audioCallback (const void*, void*, unsigned long, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void*);
};

// Audio callback called by portaudio.
int audioCallback (const void *inputBuffer,
                   void *outputBuffer,
                   unsigned long framesPerBuffer,
                   const PaStreamCallbackTimeInfo *timeInfo,
                   PaStreamCallbackFlags statusFlags,
                   void *userData);

#endif /* defined(__interface__audio__) */
