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

// Note: main documentation in audio.cpp

/**
 * If enabled, direct the audio callback to write the audio input buffer 
 * directly into the audio output buffer.
 */
#define WRITE_AUDIO_INPUT_TO_OUTPUT 1
/**
 * If enabled, create an additional buffer to store audio input
 * and direct the audio callback to write the audio input to this buffer.
 */
#define WRITE_AUDIO_INPUT_TO_BUFFER 0

// Note: I initially used static const bools within the Audio class and normal
// 'if' blocks instead of preprocessor - under the assumption that the compiler
// would optimize out the redundant code.
// However, as that apparently did not work (for some reason or another - even
// with full compiler optimization turned on), I've switched to using preprocessor
// macros instead (which is probably faster anyways (at compile-time)).

/**
 * Low level audio interface.
 * 
 * Contains static methods that write to an internal audio buffer, which 
   is read from by a portaudio callback.
 * Responsible for initializing and terminating portaudio. Audio::init() 
   and Audio::terminate() should be called at the beginning and end of
   program execution.
 */
class Audio {
public:
    // Initializes portaudio. Should be called at the beginning of program execution.
    static bool init ();
    // Deinitializes portaudio. Should be called at the end of program execution.
    static bool terminate ();
    
    // Write methods: write to internal audio buffer.
    static void writeAudio  (unsigned int offset, unsigned int length, float const *left, float const *right);
    static void addAudio    (unsigned int offset, unsigned int length, float const *left, float const *right);
    static void writeTone   (unsigned int offset, unsigned int length, float const left, float const right);
    static void addTone     (unsigned int offset, unsigned int length, float const left, float const right);
    static void clearAudio  (unsigned int offset, unsigned int length);
    
    // Read data from internal 'input' audio buffer to an external audio buffer.
    // (*only* works if WRITE_AUDIO_INPUT_TO_BUFFER is enabled).
    static void readAudioInput (unsigned int offset, unsigned int length, float *left, float *right);
    
    /**
     * Set the audio input gain. (multiplier applied to mic input)
     */
    static void setInputGain (float gain) {
        data->inputGain = gain;
    }
    /**
     * Get the internal portaudio error code (paNoError if none).
     * Use in conjunction with Audio::init() or Audio::terminate(), as it is not
       impacted by any other methods.
     */
    const PaError getError () { return err; }
    
private:
    /**
     * Set to true by Audio::init() and false by Audio::terminate().
     * Used to prevent Audio::terminate() from deleting uninitialized memory.
     */
    static bool initialized;
    
    /**
     * Internal audio data.
     * Used to communicate with the audio callback code via a shared pointer.
     */
    static struct AudioData {
        /**
         * Internal (stereo) audio buffer.
         * Written to by Audio I/O methods and the audio callback.
         * As this is a ring buffer, it should not be written to directly – thus methods 
           like Audio::writeAudio are provided.
         */
        struct BufferFrame{
            float l, r;
        } *buffer, *inputBuffer;
        /**
         * Length of the audio buffer.
         */
        const static unsigned int bufferLength = 1000;
        /**
         * Current position (start) within the ring buffer.
         * Updated by the audio callback.
         */
        unsigned int bufferPos;
        /**
         * Audio input gain (multiplier applied to the incoming audio stream).
         * Use Audio::setInputGain() to modify this.
         */
        static float inputGain;// = 1.f;
        
        AudioData () : bufferPos(0) {
            inputGain = 1.0f;
            buffer = new BufferFrame[bufferLength];
            memset((float*)buffer, 0, sizeof(float) * bufferLength * 2);
            #if WRITE_AUDIO_INPUT_TO_BUFFER
            inputBuffer = new BufferFrame[bufferLength];
            memset((float*)inputBuffer, 0, sizeof(float) * bufferLength * 2);
            #else
            inputBuffer = NULL;
            #endif
        }
        ~AudioData () {
            delete[] buffer;
            #if WRITE_AUDIO_INPUT_TO_BUFFER
            delete[] inputBuffer;
            #endif
        }
    }*data;
    /**
     * Internal audio stream handle.
     */
    static PaStream *stream;
    /**
     * Internal error code (used only by Audio::init() and Audio::terminate()).
     */
    static PaError err;
    
    Audio (); // prevent instantiation (private constructor)
    
    friend int audioCallback (const void*, void*, unsigned long, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void*);
};

// Audio callback called by portaudio.
int audioCallback (const void *inputBuffer,
                   void *outputBuffer,
                   unsigned long framesPerBuffer,
                   const PaStreamCallbackTimeInfo *timeInfo,
                   PaStreamCallbackFlags statusFlags,
                   void *userData);


#endif
