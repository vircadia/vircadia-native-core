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
#include "Head.h"
#include "AudioData.h"

class Audio {
public:
    // initializes audio I/O
    static bool init();
    static bool init(Head* mainHead);
    
    static void render();
    static void render(int screenWidth, int screenHeight);
    
    static void getInputLoudness(float * lastLoudness, float * averageLoudness);
    
    // terminates audio I/O
    static bool terminate(); 
private:    
    static bool initialized;
    
    static AudioData *data;
    
    // protects constructor so that public init method is used
    Audio();
    
    // hold potential error returned from PortAudio functions
    static PaError err;
    
    // audio stream handle
    static PaStream *stream;
    
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
