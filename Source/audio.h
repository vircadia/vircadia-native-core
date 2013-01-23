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

class Audio {
public:
    // initializes audio I/O
    static bool init();
    static bool init(Head* mainHead);
    
    // terminates audio I/O
    static bool terminate(); 
private:
    static bool initialized;
    
    static struct AudioData {
        // struct for left/right data in audio buffer
        struct BufferFrame {
            int16_t left, right;
        } *buffer;
        
        Head* linkedHead;
        
        // length in bytes of audio buffer
        const static unsigned int bufferLength = 1024;
        
        AudioData() {
            // alloc memory for buffer
            buffer = new BufferFrame[bufferLength];
            memset(buffer, 0, sizeof(int16_t) * bufferLength * 2);
        }
        
        ~AudioData() {
            delete[] buffer;
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
