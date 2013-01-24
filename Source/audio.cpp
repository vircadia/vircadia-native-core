//
//  audio.cpp
//  interface
//
//  Created by Stephen Birarda on 1/22/13.
//  Copyright (c) 2013 Rosedale Lab. All rights reserved.
//

#include <iostream>
#include <fstream>
#include "audio.h"

bool Audio::initialized;
PaError Audio::err;
PaStream *Audio::stream;
Audio::AudioData *Audio::data;


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
    Audio::AudioData *data = (Audio::AudioData *) userData;
    
    int16_t *outputLeft = ((int16_t **) outputBuffer)[0];
    int16_t *outputRight = ((int16_t **) outputBuffer)[1];
    
    float yawRatio = data->linkedHead != NULL
        ? data->linkedHead->getYaw() / 90.0
        : 0;
    
    int numSamplesDelay = abs(floor(yawRatio * PHASE_DELAY_AT_90));
    
    if (numSamplesDelay > PHASE_DELAY_AT_90) {
        numSamplesDelay = PHASE_DELAY_AT_90;
    }

    int16_t *leadingBuffer = yawRatio > 0 ? outputLeft : outputRight;
    int16_t *trailingBuffer = yawRatio > 0 ? outputRight : outputLeft;
    
    int wrapAroundSamples = (BUFFER_LENGTH_BYTES / sizeof(int16_t)) - (data->fileSamples - data->samplePointer);
    
    int16_t *samplesToQueue;
    
    if (wrapAroundSamples < 0) {
        samplesToQueue = data->fileBuffer + data->samplePointer;
        data->samplePointer += (BUFFER_LENGTH_BYTES / sizeof(int16_t));
    } else {
        samplesToQueue = new int16_t[BUFFER_LENGTH_BYTES];
        memcpy(samplesToQueue, data->fileBuffer + data->samplePointer, (data->fileSamples - data->samplePointer) * sizeof(int16_t));
        memcpy(samplesToQueue, data->fileBuffer, wrapAroundSamples * sizeof(int16_t));
        data->samplePointer = wrapAroundSamples;
    }
    
    memcpy(leadingBuffer, samplesToQueue, BUFFER_LENGTH_BYTES);
    
    int offsetBytes = numSamplesDelay * sizeof(int16_t);
    memcpy(trailingBuffer, data->delayBuffer + (PHASE_DELAY_AT_90 - numSamplesDelay), offsetBytes);
    memcpy(trailingBuffer + numSamplesDelay, samplesToQueue, BUFFER_LENGTH_BYTES - offsetBytes);
    
    if (wrapAroundSamples > 0) {
        delete[] samplesToQueue;
    }
    
    // copy PHASE_DELAY_AT_90 samples to delayBuffer in case we need it next go around
    memcpy(data->delayBuffer, data->fileBuffer + data->samplePointer - PHASE_DELAY_AT_90, PHASE_DELAY_AT_90 * sizeof(int16_t));
    
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
    return Audio::init(NULL);
}

bool Audio::init(Head* mainHead)
{
    initialized = true;
    
    data = new AudioData();
    data->linkedHead = mainHead;
    
    err = Pa_Initialize();
    if (err != paNoError) goto error;
    
    Audio::readFile();
    
    err = Pa_OpenDefaultStream(&stream,
                               NULL,       // input channels
                               2,       // output channels
                               (paInt16 | paNonInterleaved), // sample format
                               22050,   // sample rate (hz)
                               512,     // frames per buffer
                               audioCallback, // callback function
                               (void *) data);  // user data to be passed to callback
    
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

void Audio::readFile()
{
    FILE *soundFile = fopen("love.raw", "r");
    
    // get length of file:
    std::fseek(soundFile, 0, SEEK_END);
    data->fileSamples = std::ftell(soundFile) / sizeof(int16_t);
    std::rewind(soundFile);
    
    data->fileBuffer = new int16_t[data->fileSamples];
    std::fread(data->fileBuffer, sizeof(int16_t), data->fileSamples, soundFile);
    
    
    std::fclose(soundFile);
}

/**
 * Close the running audio stream, and deinitialize portaudio.
 * Should be called at the end of program execution.
 * @return Returns true if the initialization was successful, or false if an error occured.
 The error code may be retrieved by Audio::getError().
 */
bool Audio::terminate ()
{
    if (!initialized) {
        return true;
    } else {
        initialized = false;
        
        err = Pa_CloseStream(stream);
        if (err != paNoError) goto error;
        
        delete data;
        
        err = Pa_Terminate();
        if (err != paNoError) goto error;
        
        return true;
    }
    
error:
    fprintf(stderr, "-- portaudio termination error --\n");
    fprintf(stderr, "PortAudio error (%d): %s\n", err, Pa_GetErrorText(err));
    return false;
}

