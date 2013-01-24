//
//  audio.h
//  interface
//
//  Created by Stephen Birarda on 1/22/13.
//  Copyright (c) 2013 Rosedale Lab. All rights reserved.
//

#ifndef __interface__audio__
#define __interface__audio__

#include <iostream>
#include "portaudio.h"
#include "head.h"

#define BUFFER_LENGTH_BYTES 1024
#define PHASE_DELAY_AT_90 20

class Audio {
public:
    // initializes audio I/O
    static bool init();
    static bool init(Head* mainHead);
    
    // terminates audio I/O
    static bool terminate(); 
private:
    static void readFile();
    
    static bool initialized;
    
    static struct AudioData {
        int samplePointer;
        int fileSamples;
        
        // length in bytes of audio buffer
        
        int16_t *delayBuffer;
        int16_t *fileBuffer;
        
        Head* linkedHead;
    
        AudioData() {
            samplePointer = 0;
            
            // alloc memory for sample delay buffer
            delayBuffer = new int16_t[PHASE_DELAY_AT_90];
            memset(delayBuffer, 0, sizeof(int16_t) * PHASE_DELAY_AT_90);
        }
        
        ~AudioData() {
            delete[] fileBuffer;
            delete[] delayBuffer;
        }
    } *data;
    
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
