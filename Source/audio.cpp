//
//  audio.cpp
//  interface
//
//  Created by Stephen Birarda on 1/22/13.
//  Copyright (c) 2013 High Fidelity, Inc.. All rights reserved.
//

#include <iostream>
#include <fstream>
#include "audio.h"
#include "util.h"

bool Audio::initialized;
PaError Audio::err;
PaStream *Audio::stream;
Audio::AudioData *Audio::data;


Audio::AudioSource::~AudioSource()
{
    delete[] audioData;
}

Audio::AudioData::AudioData() {
    for(int s = 0; s < NUM_AUDIO_SOURCES; s++) {
        sources[s] = AudioSource();
    }
    
    samplesToQueue = new int16_t[BUFFER_LENGTH_BYTES / sizeof(int16_t)];
}

Audio::AudioData::~AudioData() {
    for (int s = 0; s < NUM_AUDIO_SOURCES; s++) {
        sources[s].AudioSource::~AudioSource();
    }
    
    delete[] samplesToQueue;
}

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
    
    memset(outputLeft, 0, BUFFER_LENGTH_BYTES);
    memset(outputRight, 0, BUFFER_LENGTH_BYTES);
    
    for (int s = 0; s < NUM_AUDIO_SOURCES; s++) {
        
        glm::vec3 headPos = data->linkedHead->getPos();
        glm::vec3 sourcePos = data->sources[s].position;
    
        int startPointer = data->sources[s].samplePointer;
        int wrapAroundSamples = (BUFFER_LENGTH_BYTES / sizeof(int16_t)) - (data->sources[s].lengthInSamples - data->sources[s].samplePointer);
        
        if (wrapAroundSamples <= 0) {
            memcpy(data->samplesToQueue, data->sources[s].audioData + data->sources[s].samplePointer, BUFFER_LENGTH_BYTES);
            data->sources[s].samplePointer += (BUFFER_LENGTH_BYTES / sizeof(int16_t));
        } else {
            memcpy(data->samplesToQueue, data->sources[s].audioData + data->sources[s].samplePointer, (data->sources[s].lengthInSamples - data->sources[s].samplePointer) * sizeof(int16_t));
            memcpy(data->samplesToQueue + (data->sources[s].lengthInSamples - data->sources[s].samplePointer), data->sources[s].audioData, wrapAroundSamples * sizeof(int16_t));
            data->sources[s].samplePointer = wrapAroundSamples;
        }
        
        float distance = sqrtf(powf(-headPos[0] - sourcePos[0], 2) + powf(-headPos[2] - sourcePos[2], 2));
        float distanceAmpRatio = powf(0.5, cbrtf(distance * 10));
        
        float angleToSource = angle_to(headPos * -1.f, sourcePos, data->linkedHead->getRenderYaw(), data->linkedHead->getYaw()) * M_PI/180;
        float sinRatio = sqrt(fabsf(sinf(angleToSource)));
        int numSamplesDelay = PHASE_DELAY_AT_90 * sinRatio;
        
        float phaseAmpRatio = 1.f - (AMPLITUDE_RATIO_AT_90 * sinRatio);
        
        std::cout << "S: " << numSamplesDelay << " A: " << angleToSource << " S: " << sinRatio << " AR: " << phaseAmpRatio << "\n";
        
        int16_t *leadingOutput = angleToSource > 0 ? outputLeft : outputRight;
        int16_t *trailingOutput = angleToSource > 0 ? outputRight : outputLeft;
        
        for (int i = 0; i < BUFFER_LENGTH_BYTES / sizeof(int16_t); i++) {
            data->samplesToQueue[i] *= distanceAmpRatio / NUM_AUDIO_SOURCES;
            leadingOutput[i] += data->samplesToQueue[i];
            
            if (i >= numSamplesDelay) {
                trailingOutput[i] += data->samplesToQueue[i - numSamplesDelay];
            } else {
                int sampleIndex = startPointer - numSamplesDelay + i;
                
                if (sampleIndex < 0) {
                    sampleIndex += data->sources[s].lengthInSamples;
                }
                
                trailingOutput[i] += data->sources[s].audioData[sampleIndex] * (distanceAmpRatio * phaseAmpRatio / NUM_AUDIO_SOURCES);
            }
        }
    }
    
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
    Head deadHead = Head();
    return Audio::init(&deadHead);
}

bool Audio::init(Head* mainHead)
{    
    data = new AudioData();
    data->linkedHead = mainHead;
    
    err = Pa_Initialize();
    if (err != paNoError) goto error;
    
    data->sources[0].position = glm::vec3(6, 0, -1);
    readFile("jeska.raw", &data->sources[0]);
    
    data->sources[1].position = glm::vec3(6, 0, 6);
    readFile("grayson.raw", &data->sources[1]);
    
    err = Pa_OpenDefaultStream(&stream,
                               NULL,       // input channels
                               2,       // output channels
                               (paInt16 | paNonInterleaved), // sample format
                               22050,   // sample rate (hz)
                               512,     // frames per buffer
                               audioCallback, // callback function
                               (void *) data);  // user data to be passed to callback
    if (err != paNoError) goto error;
    
    initialized = true;
    
    // start the stream now that sources are good to go
    Pa_StartStream(stream);
    if (err != paNoError) goto error;
    
    return paNoError;
error:
    fprintf(stderr, "-- Failed to initialize portaudio --\n");
    fprintf(stderr, "PortAudio error (%d): %s\n", err, Pa_GetErrorText(err));
    initialized = false;
    delete[] data;
    return false;
}

void Audio::sourceSetup()
{
    if (initialized) {
        
        for (int s = 0; s < NUM_AUDIO_SOURCES; s++) {
            // render gl objects on screen for our sources
            glPushMatrix();
            
            glTranslatef(data->sources[s].position[0], data->sources[s].position[1], data->sources[s].position[2]);
            glColor3f((s == 0 ? 1 : 0), (s == 1 ? 1 : 0), (s == 2 ? 1 : 0));
            glutSolidCube(0.5);
            
            glPopMatrix();
        }
    }
}

void Audio::readFile(const char *filename, struct AudioSource *source)
{
    FILE *soundFile = fopen(filename, "r");
    
    // get length of file:
    std::fseek(soundFile, 0, SEEK_END);
    source->lengthInSamples = std::ftell(soundFile) / sizeof(int16_t);
    std::rewind(soundFile);
    
    source->audioData = new int16_t[source->lengthInSamples];
    std::fread(source->audioData, sizeof(int16_t), source->lengthInSamples, soundFile);
    
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

