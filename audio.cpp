    //
    //  audio.cpp
    //  interface
    //
    //  Created by Seiji Emery on 9/2/12.
    //  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
    //

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include "audio.h"

    // static member definitions 
    // (required – will cause linker errors if left out...):
bool Audio::initialized;
Audio::AudioData *Audio::data;
PaStream *Audio::stream;
PaError Audio::err;
float Audio::AudioData::inputGain;

int audioCallback (const void *inputBuffer,
                   void *outputBuffer,
                   unsigned long frames,
                   const PaStreamCallbackTimeInfo *timeInfo,
                   PaStreamCallbackFlags statusFlags,
                   void *userData) 
{
    Audio::AudioData *data = (Audio::AudioData*)userData;
    float *input = (float*)inputBuffer;
    float *output = (float*)outputBuffer;
    
    if (input != NULL) {
            // combine input into data buffer
        
            // temp variables (frames and bufferPos need to remain untouched so they can be used in the second block of code)
        unsigned int f = (unsigned int)frames,  
        p = data->bufferPos;
        for (; p < data->bufferLength && f > 0; --f, ++p) {
            data->buffer[p].l += (*input++) * data->inputGain;
            data->buffer[p].r += (*input++) * data->inputGain;
        }
        if (f > 0) {
                // handle data->buffer wraparound
            for (p = 0; f > 0; --f, ++p) {
                data->buffer[p].l += (*input++) * data->inputGain;
                data->buffer[p].r += (*input++) * data->inputGain;
            }
        }
    }
    
        // Write data->buffer into outputBuffer
    if (data->bufferPos + frames >= data->bufferLength) {
            // wraparound: write first section (end of buffer) first
        
            // note: buffer is just an array of a struct of floats, so it can be typecast to float*
        memcpy(output, (float*)(data->buffer + data->bufferPos),    // write data buffer
               (data->bufferLength - data->bufferPos) * 2 * sizeof(float));
        memset((float*)(data->buffer + data->bufferPos), 0,                 // clear data buffer
               (data->bufferLength - data->bufferPos) * 2 * sizeof(float));
        frames -= (data->bufferLength - data->bufferPos);   // adjust frames to be written
        data->bufferPos = 0;                                // reset position to start
    }
    
    memcpy(output, (float*)(data->buffer + data->bufferPos),  // write data buffer
           frames * 2 * sizeof(float));
    memset((float*)(data->buffer + data->bufferPos), 0,       // clear data buffer
           frames * 2 * sizeof(float));
    data->bufferPos += frames;                                // update position
    
    return paContinue;
}

/*
 ** Initializes portaudio, and creates and starts an audio stream.
 */
void Audio::init() 
{
    initialized = true;
    
    data = new AudioData();
    
    err = Pa_Initialize();
    if (err != paNoError) goto error;
    
    err = Pa_OpenDefaultStream(&stream,  
                               2,       // input channels
                               2,       // output channels
                               paFloat32, // sample format
                               44100,   // sample rate
                               256,     // frames per buffer
                               audioCallback, // callback function
                               (void*)data);  // user data to be passed to callback
    if (err != paNoError) goto error;
    
    err = Pa_StartStream(stream);
    if (err != paNoError) goto error;
    
    return;
error:
    fprintf(stderr, "-- Failed to initialize portaudio --\n");
    fprintf(stderr, "PortAudio error (%d): %s\n", err, Pa_GetErrorText(err));
    exit(err); // replace w/ return value error code?
}

/*
 ** Closes the running audio stream, and deinitializes portaudio.
 */
void Audio::terminate ()
{
    if (!initialized) return;
    initialized = false;
        //  err = Pa_StopStream(stream);
        //  if (err != paNoError) goto error;
    
    err = Pa_CloseStream(stream);
    if (err != paNoError) goto error;
    
    delete data;
    
    err = Pa_Terminate();
    if (err != paNoError) goto error;
    
    return;
error:
    fprintf(stderr, "-- portaudio termination error --\n");
    fprintf(stderr, "PortAudio error (%d): %s\n", err, Pa_GetErrorText(err));
    exit(err);
}

void Audio::writeAudio (unsigned int offset, unsigned int length, float *left, float *right) {
    if (length > data->bufferLength) {
        fprintf(stderr, "Audio::writeAudio length exceeded (%d). Truncating to %d.\n", length, data->bufferLength);
        length = data->bufferLength;
    }
    unsigned int p = data->bufferPos + offset;
    if (p > data->bufferLength) 
        p -= data->bufferLength;
    for (; p < data->bufferLength && length > 0; --length, ++p) {
        data->buffer[p].l = *left++;
        data->buffer[p].r = *right++;
    }
    if (length > 0) {
        p = 0;
        for (; length > 0; --length, ++p) {
            data->buffer[p].l = *left++;
            data->buffer[p].r = *right++;
        }
    }
}

void Audio::writeTone (unsigned int offset, unsigned int length, float left, float right) {
    if (length > data->bufferLength) {
        fprintf(stderr, "Audio::writeTone length exceeded (%d). Truncating to %d.\n", length, data->bufferLength);
        length = data->bufferLength;
    }
    unsigned int p = data->bufferPos + offset;
    if (p > data->bufferLength) 
        p -= data->bufferLength;
    for (; p < data->bufferLength && length > 0; --length, ++p) {
        data->buffer[p].l = left;
        data->buffer[p].r = right;
    }
    if (length > 0) {
        p = 0;
        for (; length > 0; --length, ++p) {
            data->buffer[p].l = left;
            data->buffer[p].r = right;
        }
    }
}

void Audio::addAudio (unsigned int offset, unsigned int length, float *left, float *right) {
    if (length > data->bufferLength) {
        fprintf(stderr, "Audio::addAudio length exceeded (%d). Truncating to %d.\n", length, data->bufferLength);
        length = data->bufferLength;
    }
    unsigned int p = data->bufferPos + offset;
    if (p > data->bufferLength) 
        p -= data->bufferLength;
    for (; p < data->bufferLength && length > 0; --length, ++p) {
        data->buffer[p].l += *left++;
        data->buffer[p].r += *right++;
    }
    if (length > 0) {
        p = 0;
        for (; length > 0; --length, ++p) {
            data->buffer[p].l += *left++;
            data->buffer[p].r += *right++;
        }
    }
}

void Audio::addTone (unsigned int offset, unsigned int length, float left, float right) {
    if (length > data->bufferLength) {
        fprintf(stderr, "Audio::writeTone length exceeded (%d). Truncating to %d.\n", length, data->bufferLength);
        length = data->bufferLength;
    }
    unsigned int p = data->bufferPos + offset;
    if (p > data->bufferLength) 
        p -= data->bufferLength;
    for (; p < data->bufferLength && length > 0; --length, ++p) {
        data->buffer[p].l += left;
        data->buffer[p].r += right;
    }
    if (length > 0) {
        p = 0;
        for (; length > 0; --length, ++p) {
            data->buffer[p].l += left;
            data->buffer[p].r += right;
        }
    }
}

void Audio::clearAudio(unsigned int offset, unsigned int length) {
    if (length > data->bufferLength) {
        fprintf(stderr, "Audio::clearAudio length exceeded (%d). Truncating to %d.\n", length, data->bufferLength);
        length = data->bufferLength;
    }
    unsigned int p = data->bufferPos + offset;
    if (p > data->bufferLength) 
        p -= data->bufferLength;
    if (length + p < data->bufferLength) {
        memset((float*)(data->buffer + p), 0, 
               sizeof(float) * 2 * length);
    } else {
        memset((float*)(data->buffer + p), 0, 
               sizeof(float) * 2 * (data->bufferLength - p));
        memset((float*)(data->buffer + p), 0, 
               sizeof(float) * 2 * (data->bufferLength + p - data->bufferLength));
    }
}











