    //
    //  audio.cpp
    //  interface
    //
    //  Created by Seiji Emery on 9/2/12.
    //  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
    //
    
/**
 * @file audio.cpp
 * Low level audio i/o portaudio wrapper.
 *
 * @author Seiji Emery
 *
 */

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
/**
 * Audio callback used by portaudio.
 * Communicates with Audio via a shared pointer to Audio::data.
 * Writes input audio channels (if they exist) into Audio::data->buffer,
   multiplied by Audio::data->inputGain.
 * Then writes Audio::data->buffer into output audio channels, and clears
   the portion of Audio::data->buffer that has been read from for reuse.
 *
 * @param[in]  inputBuffer  A pointer to an internal portaudio data buffer containing data read by portaudio.
 * @param[out] outputBuffer A pointer to an internal portaudio data buffer to be read by the configured output device.
 * @param[in]  frames       Number of frames that portaudio requests to be read/written.
                            (Valid size of input/output buffers = frames * number of channels (2) * sizeof data type (float)).
 * @param[in]  timeInfo     Portaudio time info. Currently unused.
 * @param[in]  statusFlags  Portaudio status flags. Currently unused.
 * @param[in]  userData     Pointer to supplied user data (in this case, a pointer to Audio::data).
                            Used to communicate with external code (since portaudio calls this function from another thread).
 * @return Should be of type PaStreamCallbackResult. Return paComplete to end the stream, or paContinue to continue (default).
            Can be used to end the stream from within the callback.
 */
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
    
    #if WRITE_AUDIO_INPUT_TO_OUTPUT
    if (input != NULL) {// && Audio::writeAudioInputToOutput) {
            // combine input into data buffer
        
            // temp variables (frames and bufferPos need to remain untouched so they can be used in the second block of code)
        unsigned int f = (unsigned int)frames,  
        p = data->bufferPos;
        for (; p < data->bufferLength && f > 0; --f, ++p) {
        #if WRITE_AUDIO_INPUT_TO_BUFFER
            data->buffer[p].l +=
                data->inputBuffer[p].l = (*input++) * data->inputGain;
            data->buffer[p].r +=
                data->inputBuffer[p].r = (*input++) * data->inputGain;
        #else
            data->buffer[p].l += (*input++) * data->inputGain;
            data->buffer[p].r += (*input++) * data->inputGain;
        #endif
        }
        if (f > 0) {
                // handle data->buffer wraparound
            for (p = 0; f > 0; --f, ++p) {
            #if WRITE_AUDIO_INPUT_TO_BUFFER
                data->buffer[p].l +=
                    data->inputBuffer[p].l = (*input++) * data->inputGain;
                data->buffer[p].r +=
                    data->inputBuffer[p].r = (*input++) * data->inputGain;
            #else
                data->buffer[p].l += (*input++) * data->inputGain;
                data->buffer[p].r += (*input++) * data->inputGain;
            #endif
            }
        }
    } 
    #elif WRITE_AUDIO_INPUT_TO_BUFFER
    if (input != NULL) {// && Audio::writeAudioInputToBuffer) {
        unsigned int f = (unsigned int)frames,  
        p = data->bufferPos;
        for (; p < data->bufferLength && f > 0; --f, ++p) {
            data->inputBuffer[p].l = (*input++) * data->inputGain;
            data->inputBuffer[p].r = (*input++) * data->inputGain;
        }
        if (f > 0) {
                // handle data->buffer wraparound
            for (p = 0; f > 0; --f, ++p) {
                data->inputBuffer[p].l = (*input++) * data->inputGain;
                data->inputBuffer[p].r = (*input++) * data->inputGain;
            }
        }
    }
    #endif
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

/**
 * Initialize portaudio and start an audio stream.
 * Should be called at the beginning of program exection.
 * @seealso Audio::terminate
 * @return  Returns true if successful or false if an error occurred.
            Use Audio::getError() to retrieve the error code.
 */
bool Audio::init() 
{
    initialized = true;
    
    data = new AudioData();
    
    err = Pa_Initialize();
    if (err != paNoError) goto error;
    
    err = Pa_OpenDefaultStream(&stream,  
                               2,       // input channels
                               2,       // output channels
                               paFloat32, // sample format
                               44100,   // sample rate (hz)
                               512,     // frames per buffer
                               audioCallback, // callback function
                               (void*)data);  // user data to be passed to callback
    if (err != paNoError) goto error;
    
    err = Pa_StartStream(stream);
    if (err != paNoError) goto error;
    
    return paNoError;
error:
    fprintf(stderr, "-- Failed to initialize portaudio --\n");
    fprintf(stderr, "PortAudio error (%d): %s\n", err, Pa_GetErrorText(err));
    initialized = false;
    delete[] data;
    return false;
}

/**
 * Close the running audio stream, and deinitialize portaudio.
 * Should be called at the end of program execution.
 * @return Returns true if the initialization was successful, or false if an error occured.
           The error code may be retrieved by Audio::getError().
 */
bool Audio::terminate ()
{
    if (!initialized) 
        return true;
    initialized = false;
        //  err = Pa_StopStream(stream);
        //  if (err != paNoError) goto error;
    
    err = Pa_CloseStream(stream);
    if (err != paNoError) goto error;
    
    delete data;
    
    err = Pa_Terminate();
    if (err != paNoError) goto error;
    
    return true;
error:
    fprintf(stderr, "-- portaudio termination error --\n");
    fprintf(stderr, "PortAudio error (%d): %s\n", err, Pa_GetErrorText(err));
    return false;
}

/**
 * Write a stereo audio stream (float*) to the audio buffer.
 * Values should be clamped between -1.0f and 1.0f.
 * @param[in]   offset  Write offset from the start of the audio buffer.
 * @param[in]   length  Length of audio channels to be read.
 * @param[in]   left    Left channel of the audio stream.
 * @param[in]   right   Right channel of the audio stream.
 */
void Audio::writeAudio (unsigned int offset, unsigned int length, float const *left, float const *right) {
    if (data->buffer == NULL)
        return;
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

/**
 * Write a repeated stereo sample (float) to the audio buffer.
 * Values should be clamped between -1.0f and 1.0f.
 * @param[in]   offset  Write offset from the start of the audio buffer.
 * @param[in]   length  Length of tone.
 * @param[in]   left    Left component.
 * @param[in]   right   Right component.
 */
void Audio::writeTone (unsigned int offset, unsigned int length, float const left, float const right) {
    if (data->buffer == NULL)
        return;
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

/**
 * Write a stereo audio stream (float*) to the audio buffer. 
 * Audio stream is added to the existing contents of the audio buffer.
 * Values should be clamped between -1.0f and 1.0f.
 * @param[in]   offset  Write offset from the start of the audio buffer.
 * @param[in]   length  Length of audio channels to be read.
 * @param[in]   left    Left channel of the audio stream.
 * @param[in]   right   Right channel of the audio stream.
 */
void Audio::addAudio (unsigned int offset, unsigned int length, float const *left, float const *right) {
    if (data->buffer == NULL)
        return;
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

/**
 * Write a repeated stereo sample (float) to the audio buffer.
 * Sample is added to the existing contents of the audio buffer.
 * Values should be clamped between -1.0f and 1.0f.
 * @param[in]   offset  Write offset from the start of the audio buffer.
 * @param[in]   length  Length of tone.
 * @param[in]   left    Left component.
 * @param[in]   right   Right component.
 */
void Audio::addTone (unsigned int offset, unsigned int length, float const left, float const right) {
    if (data->buffer == NULL)
        return;
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
/**
 * Clear a section of the audio buffer.
 * @param[in]   offset  Offset from the start of the audio buffer.
 * @param[in]   length  Length of section to clear.
 */
void Audio::clearAudio(unsigned int offset, unsigned int length) {
    if (data->buffer == NULL)
        return;
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

/**
 * Read audio input into the target buffer.
 * @param[in]   offset  Offset from the start of the input audio buffer to read from.
 * @param[in]   length  Length of the target buffer.
 * @param[out]  left    Left channel of the target buffer.
 * @param[out]  right   Right channel of the target buffer.
 */
void Audio::readAudioInput (unsigned int offset, unsigned int length, float *left, float *right) {
#if WRITE_AUDIO_INPUT_TO_BUFFER
    if (data->inputBuffer == NULL)
        return;
    if (length + offset > data->bufferLength) {
        fprintf(stderr, "Audio::readAudioInput length exceeded (%d + %d). Truncating to %d + %d.\n", offset, length, offset, data->bufferLength - offset);
        length = data->bufferLength - offset;
    }
    unsigned int p = data->bufferPos + offset;
    if (p > data->bufferLength) 
        p -= data->bufferLength;
    for (; p < data->bufferLength && length > 0; --length, ++p) {
        *left++ = data->inputBuffer[p].l;
        *right++ = data->inputBuffer[p].r;
    }
    if (length > 0) {
        p = 0;
        for (; length > 0; --length, ++p) {
            *left++ = data->inputBuffer[p].l;
            *right++ = data->inputBuffer[p].r;
        }
    }
#else
    return;
#endif
}









