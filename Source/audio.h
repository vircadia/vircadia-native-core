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
#define AMPLITUDE_RATIO_AT_90 0.5
#define NUM_AUDIO_SOURCES 2

class Audio {
public:
    // initializes audio I/O
    static bool init();
    static bool init(Head* mainHead);
    
    static void sourceSetup();
    
    // terminates audio I/O
    static bool terminate(); 
private:
    struct AudioSource {
        glm::vec3 position;
        int16_t *audioData;
        int lengthInSamples;
        int samplePointer;
        
        AudioSource() {
            samplePointer = 0;
        };
        
        ~AudioSource() {
            delete[] audioData;
        }
    };
    
    static void readFile(const char *filename, struct AudioSource *source);
    static bool initialized;
    
    static struct AudioData {
        Head* linkedHead;
        AudioSource sources[NUM_AUDIO_SOURCES];
        
        int16_t *samplesToQueue;
    
        AudioData() {
            sources[0] = AudioSource();
            sources[1] = AudioSource();
            
            samplesToQueue = new int16_t[BUFFER_LENGTH_BYTES / sizeof(int16_t)];
        }
        
        ~AudioData() {
//            delete sources[0];
//            delete sources[1];
//            delete sources[2];
            delete[] samplesToQueue;
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
