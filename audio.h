    //
    //  audio.h
    //  interface
    //
    //  Created by Seiji Emery on 9/2/12.
    //  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
    //

#ifndef interface_audio_h
#define interface_audio_h

#include "portaudio.h"

typedef short sample_t;

int audioCallback (const void *inputBuffer,
                   void *outputBuffer,
                   unsigned long framesPerBuffer,
                   const PaStreamCallbackTimeInfo *timeInfo,
                   PaStreamCallbackFlags statusFlags,
                   void *userData);

/*
 ** TODO: Docs
 */
class Audio {
public:
    static void init ();
    static void terminate ();
    
        // Audio values clamped betewen -1.0f and 1.0f
    static void writeAudio  (unsigned int offset, unsigned int length, float *left, float *right);
    static void addAudio    (unsigned int offset, unsigned int length, float *left, float *right);
    static void writeTone   (unsigned int offset, unsigned int length, float left, float right);
    static void addTone     (unsigned int offset, unsigned int length, float left, float right);
    static void clearAudio  (unsigned int offset, unsigned int length);
    
    static void setInputGain (float gain) {
        data->inputGain = gain;
    }
    
private:
    static bool initialized;
    
    static struct AudioData {
        struct BufferFrame{
            float l, r;
        } *buffer;
        const static unsigned int bufferLength = 1000;
        unsigned int bufferPos;
        static float inputGain;// = 1.f;
        
        AudioData () : bufferPos(0) {
            inputGain = 1.0f;
            buffer = new BufferFrame[bufferLength];
            for (unsigned int i = 0; i < bufferLength; ++i) {
                buffer[i].l = buffer[i].r = 0;
            }
        }
        ~AudioData () {
            delete[] buffer;
        }
        
    }*data;
    static PaStream *stream;
    static PaError err;
    
    Audio (); // prevent instantiation (private constructor)
    
    
    friend int audioCallback (const void*, void*, unsigned long, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void*);
};

#endif
