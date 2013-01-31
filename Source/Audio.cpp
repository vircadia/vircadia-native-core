//
//  Audio.cpp
//  interface
//
//  Created by Stephen Birarda on 1/22/13.
//  Copyright (c) 2013 High Fidelity, Inc.. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <pthread.h>
#include <sys/time.h>
#include "Audio.h"
#include "Util.h"
#include "AudioSource.h"
#include "UDPSocket.h"

const int BUFFER_LENGTH_BYTES = 1024;
const int BUFFER_LENGTH_SAMPLES = BUFFER_LENGTH_BYTES / sizeof(int16_t);
const int RING_BUFFER_LENGTH_SAMPLES = BUFFER_LENGTH_SAMPLES * 10;

const int PHASE_DELAY_AT_90 = 20;
const int AMPLITUDE_RATIO_AT_90 = 0.5;

const int NUM_AUDIO_SOURCES = 1;
const int ECHO_SERVER_TEST = 1;

const int AUDIO_UDP_LISTEN_PORT = 55444;

#define LOG_SAMPLE_DELAY 1

bool Audio::initialized;
PaError Audio::err;
PaStream *Audio::stream;
AudioData *Audio::data;
std::ofstream logFile;

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
    AudioData *data = (AudioData *) userData;
    
    int16_t *inputLeft = ((int16_t **) inputBuffer)[0];
//    int16_t *inputRight = ((int16_t **) inputBuffer)[1];
    
    if (inputLeft != NULL) {
        data->audioSocket->send((char *) "192.168.1.19", 55443, (void *)inputLeft, BUFFER_LENGTH_BYTES);
    }
    
    int16_t *outputLeft = ((int16_t **) outputBuffer)[0];
    int16_t *outputRight = ((int16_t **) outputBuffer)[1];
    
    memset(outputLeft, 0, BUFFER_LENGTH_BYTES);
    memset(outputRight, 0, BUFFER_LENGTH_BYTES);
    
    for (int s = 0; s < NUM_AUDIO_SOURCES; s++) {
        
        AudioSource *source = data->sources[s];
        
        if (ECHO_SERVER_TEST) {
            
            // copy whatever is source->sourceData to the left and right output channels
            memcpy(outputLeft, source->sourceData + source->samplePointer, BUFFER_LENGTH_BYTES);
            memcpy(outputRight, source->sourceData + source->samplePointer, BUFFER_LENGTH_BYTES);
            
            
            if (source->samplePointer < RING_BUFFER_LENGTH_SAMPLES - BUFFER_LENGTH_SAMPLES) {
                source->samplePointer += BUFFER_LENGTH_SAMPLES;
            } else {
                source->samplePointer = 0;
            }
           
        } else {
            glm::vec3 headPos = data->linkedHead->getPos();
            glm::vec3 sourcePos = source->position;
            
            int startPointer = source->samplePointer;
            int wrapAroundSamples = (BUFFER_LENGTH_SAMPLES) - (source->lengthInSamples - source->samplePointer);
            
            if (wrapAroundSamples <= 0) {
                memcpy(data->samplesToQueue, source->sourceData + source->samplePointer, BUFFER_LENGTH_BYTES);
                source->samplePointer += (BUFFER_LENGTH_SAMPLES);
            } else {
                memcpy(data->samplesToQueue, source->sourceData + source->samplePointer, (source->lengthInSamples - source->samplePointer) * sizeof(int16_t));
                memcpy(data->samplesToQueue + (source->lengthInSamples - source->samplePointer), source->sourceData, wrapAroundSamples * sizeof(int16_t));
                source->samplePointer = wrapAroundSamples;
            }
            
            float distance = sqrtf(powf(-headPos[0] - sourcePos[0], 2) + powf(-headPos[2] - sourcePos[2], 2));
            float distanceAmpRatio = powf(0.5, cbrtf(distance * 10));
            
            float angleToSource = angle_to(headPos * -1.f, sourcePos, data->linkedHead->getRenderYaw(), data->linkedHead->getYaw()) * M_PI/180;
            float sinRatio = sqrt(fabsf(sinf(angleToSource)));
            int numSamplesDelay = PHASE_DELAY_AT_90 * sinRatio;
            
            float phaseAmpRatio = 1.f - (AMPLITUDE_RATIO_AT_90 * sinRatio);
            
            //        std::cout << "S: " << numSamplesDelay << " A: " << angleToSource << " S: " << sinRatio << " AR: " << phaseAmpRatio << "\n";
            
            int16_t *leadingOutput = angleToSource > 0 ? outputLeft : outputRight;
            int16_t *trailingOutput = angleToSource > 0 ? outputRight : outputLeft;
            
            for (int i = 0; i < BUFFER_LENGTH_SAMPLES; i++) {
                data->samplesToQueue[i] *= distanceAmpRatio / NUM_AUDIO_SOURCES;
                leadingOutput[i] += data->samplesToQueue[i];
                
                if (i >= numSamplesDelay) {
                    trailingOutput[i] += data->samplesToQueue[i - numSamplesDelay];
                } else {
                    int sampleIndex = startPointer - numSamplesDelay + i;
                    
                    if (sampleIndex < 0) {
                        sampleIndex += source->lengthInSamples;
                    }
                    
                    trailingOutput[i] += source->sourceData[sampleIndex] * (distanceAmpRatio * phaseAmpRatio / NUM_AUDIO_SOURCES);
                }
            }
        }
    }
    
    return paContinue;
}

struct AudioRecThreadStruct {
    AudioData *sharedAudioData;
};

void *receiveAudioViaUDP(void *args) {
    AudioRecThreadStruct *threadArgs = (AudioRecThreadStruct *) args;
    AudioData *sharedAudioData = threadArgs->sharedAudioData;
    
    int16_t *receivedData = new int16_t[BUFFER_LENGTH_SAMPLES];
    int *receivedBytes = new int;
    int streamSamplePointer = 0;
    
    timeval previousReceiveTime, currentReceiveTime;
    
    if (LOG_SAMPLE_DELAY) {
        gettimeofday(&previousReceiveTime, NULL);
        
        char *filename = new char[50];
        sprintf(filename, "%s/Desktop/%ld.csv", getenv("HOME"), previousReceiveTime.tv_sec);
        
        logFile.open(filename, std::ios::out);
        
        delete[] filename;
    }
    
    while (true) {
        if (sharedAudioData->audioSocket->receive((void *)receivedData, receivedBytes)) {
            
            if (LOG_SAMPLE_DELAY) {
                // write time difference (in microseconds) between packet receipts to file
                gettimeofday(&currentReceiveTime, NULL);
                
                double timeDiff = diffclock(previousReceiveTime, currentReceiveTime);
                
                logFile << timeDiff << std::endl;
            }
            
            
            // add the received data to the shared memory
            memcpy(sharedAudioData->sources[0]->sourceData + streamSamplePointer, receivedData, *receivedBytes);
            
            if (streamSamplePointer < RING_BUFFER_LENGTH_SAMPLES - BUFFER_LENGTH_SAMPLES) {
                streamSamplePointer += BUFFER_LENGTH_SAMPLES;
            } else {
                streamSamplePointer = 0;
            }
            
            if (LOG_SAMPLE_DELAY) {
                gettimeofday(&previousReceiveTime, NULL);
            }
        }        
    }
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
    Head *deadHead = new Head();
    return Audio::init(deadHead);
}

bool Audio::init(Head *mainHead)
{
    err = Pa_Initialize();
    if (err != paNoError) goto error;
    
    if (ECHO_SERVER_TEST) {
        data = new AudioData(1, BUFFER_LENGTH_BYTES);
        
        // setup a UDPSocket
        data->audioSocket = new UDPSocket(AUDIO_UDP_LISTEN_PORT);
        
        // setup the ring buffer source for the streamed audio
        data->sources[0]->sourceData = new int16_t[RING_BUFFER_LENGTH_SAMPLES];
        memset(data->sources[0]->sourceData, 0, RING_BUFFER_LENGTH_SAMPLES * sizeof(int16_t));
        
        pthread_t audioReceiveThread;
        
        AudioRecThreadStruct threadArgs;
        threadArgs.sharedAudioData = data;
        
        pthread_create(&audioReceiveThread, NULL, receiveAudioViaUDP, (void *) &threadArgs);
    } else {
        data = new AudioData(NUM_AUDIO_SOURCES, BUFFER_LENGTH_BYTES);
        
        data->sources[0]->position = glm::vec3(6, 0, -1);
        data->sources[0]->loadDataFromFile("jeska.raw");
        
        data->sources[1]->position = glm::vec3(6, 0, 6);
        data->sources[1]->loadDataFromFile("grayson.raw");
    }
    
    data->linkedHead = mainHead;
    
    err = Pa_OpenDefaultStream(&stream,
                               2,       // input channels
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

void Audio::render()
{
    if (initialized && !ECHO_SERVER_TEST) {
        
        for (int s = 0; s < NUM_AUDIO_SOURCES; s++) {
            // render gl objects on screen for our sources
            glPushMatrix();
            
            glTranslatef(data->sources[s]->position[0], data->sources[s]->position[1], data->sources[s]->position[2]);
            glColor3f((s == 0 ? 1 : 0), (s == 1 ? 1 : 0), (s == 2 ? 1 : 0));
            glutSolidCube(0.5);
            
            glPopMatrix();
        }
    }
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
        
        logFile.close();
        
        return true;
    }
    
error:
    fprintf(stderr, "-- portaudio termination error --\n");
    fprintf(stderr, "PortAudio error (%d): %s\n", err, Pa_GetErrorText(err));
    return false;
}

