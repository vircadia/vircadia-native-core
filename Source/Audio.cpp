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
#include <sys/stat.h>
#include "Audio.h"
#include "Util.h"
#include "AudioSource.h"
#include "UDPSocket.h"

const short BUFFER_LENGTH_BYTES = 1024;
const short BUFFER_LENGTH_SAMPLES = BUFFER_LENGTH_BYTES / sizeof(int16_t);

const short PACKET_LENGTH_BYTES = 1024;
const short PACKET_LENGTH_SAMPLES = PACKET_LENGTH_BYTES / sizeof(int16_t);

const int PHASE_DELAY_AT_90 = 20;
const float AMPLITUDE_RATIO_AT_90 = 0.5;

const short RING_BUFFER_FRAMES = 10;
const short RING_BUFFER_SIZE_SAMPLES = RING_BUFFER_FRAMES * BUFFER_LENGTH_SAMPLES;

const short JITTER_BUFFER_LENGTH_MSECS = 40;
const int SAMPLE_RATE = 22050;

const short NUM_AUDIO_SOURCES = 2;
const short ECHO_SERVER_TEST = 1;

char WORKCLUB_AUDIO_SERVER[] = "192.168.1.19";
char EC2_WEST_AUDIO_SERVER[] = "54.241.92.53";

const int AUDIO_UDP_LISTEN_PORT = 55444;

int starve_counter = 0;

StDev stdev;

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
        data->audioSocket->send((char *) EC2_WEST_AUDIO_SERVER, 55443, (void *)inputLeft, BUFFER_LENGTH_BYTES);
    }
    
    int16_t *outputLeft = ((int16_t **) outputBuffer)[0];
    int16_t *outputRight = ((int16_t **) outputBuffer)[1];
    
    memset(outputLeft, 0, BUFFER_LENGTH_BYTES);
    memset(outputRight, 0, BUFFER_LENGTH_BYTES);
    
    if (ECHO_SERVER_TEST) {
        AudioRingBuffer *ringBuffer = data->ringBuffer;
       
        int16_t *queueBuffer = data->samplesToQueue;
        memset(queueBuffer, 0, BUFFER_LENGTH_BYTES);
        
        // if we've been reset, and there isn't any new packets yet
        // just play some silence
        
        if (ringBuffer->endOfLastWrite != NULL) {
            
            // play whatever we have in the audio buffer
            short silentTail = 0;
            
            // if the end of the last write to the ring is in front of the current output pointer
            // AND the difference between the two is less than a full output buffer
            // we need to add some silence after the audio data, to avoid replaying old data
            if ((ringBuffer->endOfLastWrite - ringBuffer->buffer) > (ringBuffer->nextOutput - ringBuffer->buffer)
                && (ringBuffer->endOfLastWrite - ringBuffer->nextOutput) < BUFFER_LENGTH_SAMPLES) {
                silentTail = BUFFER_LENGTH_SAMPLES - (ringBuffer->endOfLastWrite - ringBuffer->nextOutput);
            }
            
            // no sample overlap, either a direct copy of the audio data, or a copy with some appended silence
            memcpy(queueBuffer, ringBuffer->nextOutput, (BUFFER_LENGTH_SAMPLES - silentTail) * sizeof(int16_t));
            
            ringBuffer->nextOutput += BUFFER_LENGTH_SAMPLES;
            
            if (ringBuffer->nextOutput == ringBuffer->buffer + RING_BUFFER_SIZE_SAMPLES) {
                ringBuffer->nextOutput = ringBuffer->buffer;
            }
            
            if (ringBuffer->diffLastWriteNextOutput() < BUFFER_LENGTH_SAMPLES) {
                starve_counter++;
                printf("Starved #%d\n", starve_counter);
                data->wasStarved = 10;      //   Frames to render the indication that the system was starved. 
                ringBuffer->endOfLastWrite = NULL;
            }
        }
        
        // copy whatever is in the queueBuffer to the outputLeft and outputRight buffers
        memcpy(outputLeft, queueBuffer, BUFFER_LENGTH_BYTES);
        memcpy(outputRight, queueBuffer, BUFFER_LENGTH_BYTES);
        
    }  else {
        
        for (int s = 0; s < NUM_AUDIO_SOURCES; s++) {
            AudioSource *source = data->sources[s];
            
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
    
    gettimeofday(&data->lastCallback, NULL);
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
    
    timeval previousReceiveTime, currentReceiveTime = {};
    
    if (LOG_SAMPLE_DELAY) {
        gettimeofday(&previousReceiveTime, NULL);
        
        char *directory = new char[50];
        char *filename = new char[50];
        
        sprintf(directory, "%s/Desktop/echo_tests", getenv("HOME"));
        
        mkdir(directory, S_IRWXU | S_IRWXG | S_IRWXO);
        sprintf(filename, "%s/%ld.csv", directory, previousReceiveTime.tv_sec);
        
        logFile.open(filename, std::ios::out);
        
        delete[] directory;
        delete[] filename;
    }
    
    while (true) {
        if (sharedAudioData->audioSocket->receive((void *)receivedData, receivedBytes)) {

            bool firstSample = (currentReceiveTime.tv_sec == 0);
            
            gettimeofday(&currentReceiveTime, NULL);

            if (LOG_SAMPLE_DELAY) {
                if (!firstSample) {
                    // write time difference (in microseconds) between packet receipts to file
                    double timeDiff = diffclock(&previousReceiveTime, &currentReceiveTime);
                    logFile << timeDiff << std::endl;
                }
            }
            
            //  Compute and report standard deviation for jitter calculation
            if (firstSample) {
                stdev.reset();
            } else {
                double tDiff = diffclock(&previousReceiveTime, &currentReceiveTime);
                //printf(\n";
                stdev.addValue(tDiff);
                if (stdev.getSamples() > 500) {
                    sharedAudioData->jitter = stdev.getStDev();
                    printf("Avg: %4.2f, Stdev: %4.2f\n", stdev.getAverage(), sharedAudioData->jitter);
                    stdev.reset();
                }
            }
            
            AudioRingBuffer *ringBuffer = sharedAudioData->ringBuffer;
            
            int16_t *copyToPointer;
            bool needsJitterBuffer = ringBuffer->endOfLastWrite == NULL;
            short bufferSampleOverlap = 0;
            
            if (!needsJitterBuffer && ringBuffer->diffLastWriteNextOutput() > RING_BUFFER_SIZE_SAMPLES - PACKET_LENGTH_SAMPLES) {
                std::cout << "Full\n";
                needsJitterBuffer = true;
            }
            
            if (needsJitterBuffer) {
                // we'll need a jitter buffer
                // reset the ring buffer and write
                copyToPointer = ringBuffer->buffer;
                std::cout << "Writing jitter buffer\n";
            } else {
                copyToPointer = ringBuffer->endOfLastWrite;
                
                // check for possibility of overlap
                bufferSampleOverlap = ringBuffer->bufferOverlap(copyToPointer, PACKET_LENGTH_SAMPLES);
                
            }
            
            if (!bufferSampleOverlap) {
                if (needsJitterBuffer) {
                    // we need to inject a jitter buffer
                    short jitterBufferSamples = JITTER_BUFFER_LENGTH_MSECS * (SAMPLE_RATE / 1000.0);

                    // add silence for jitter buffer and then the received packet
                    memset(copyToPointer, 0, jitterBufferSamples * sizeof(int16_t));
                    memcpy(copyToPointer + jitterBufferSamples, receivedData, PACKET_LENGTH_BYTES);
                    
                    // the end of the write is the pointer to the buffer + packet + jitter buffer
                    ringBuffer->endOfLastWrite = ringBuffer->buffer + PACKET_LENGTH_SAMPLES + jitterBufferSamples;
                    ringBuffer->nextOutput = ringBuffer->buffer;
                } else {
                    // no jitter buffer, no overlap
                    // just copy the recieved data to the right spot and then add packet length to previous pointer
                    memcpy(copyToPointer, receivedData, PACKET_LENGTH_BYTES);
                    ringBuffer->endOfLastWrite += PACKET_LENGTH_SAMPLES;
                }
            } else {                
                // no jitter buffer, but overlap
                // copy to the end, and then from the begining to the overlap
                memcpy(copyToPointer, receivedData, (PACKET_LENGTH_SAMPLES - bufferSampleOverlap) * sizeof(int16_t));
                memcpy(ringBuffer->buffer, receivedData + (PACKET_LENGTH_SAMPLES - bufferSampleOverlap), bufferSampleOverlap * sizeof(int16_t));
                
                // the end of the write is the amount of overlap
                ringBuffer->endOfLastWrite = ringBuffer->buffer + bufferSampleOverlap;
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
        data = new AudioData(BUFFER_LENGTH_BYTES);
        
        // setup a UDPSocket
        data->audioSocket = new UDPSocket(AUDIO_UDP_LISTEN_PORT);
        data->ringBuffer = new AudioRingBuffer(RING_BUFFER_SIZE_SAMPLES);
        
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

void Audio::render(int screenWidth, int screenHeight)
{
    if (initialized && ECHO_SERVER_TEST) {
        glBegin(GL_LINES);
        glColor3f(1,1,1);
        
        int startX = 50.0;
        int currentX = startX;
        int topY = screenHeight - 90;
        int bottomY = screenHeight - 50;
        float frameWidth = 50.0;
        float halfY = topY + ((bottomY - topY) / 2.0);
        
        // draw the lines for the base of the ring buffer
        
        glVertex2f(currentX, topY);
        glVertex2f(currentX, bottomY);
        
        for (int i = 0; i < RING_BUFFER_FRAMES; i++) {
            glVertex2f(currentX, halfY);
            glVertex2f(currentX + frameWidth, halfY);
            currentX += frameWidth;
            
            glVertex2f(currentX, topY);
            glVertex2f(currentX, bottomY);
        }
        glEnd();
        
        
        //  Show a bar with the amount of audio remaining in ring buffer beyond current playback
        float remainingBuffer = 0;
        timeval currentTime;
        gettimeofday(&currentTime, NULL);
        float timeLeftInCurrentBuffer = 0;
        if (data->lastCallback.tv_usec > 0) timeLeftInCurrentBuffer = diffclock(&data->lastCallback, &currentTime)/(1000.0*(float)BUFFER_LENGTH_SAMPLES/(float)SAMPLE_RATE) * frameWidth;
        
        if (data->ringBuffer->endOfLastWrite != NULL)
            remainingBuffer = data->ringBuffer->diffLastWriteNextOutput() / BUFFER_LENGTH_SAMPLES * frameWidth;
        
        if (data->wasStarved == 0) glColor3f(0, 1, 0);
        else {
            glColor3f(0.5 + (float)data->wasStarved/20.0, 0, 0);
            data->wasStarved--;
        }
        
        glBegin(GL_QUADS);
        glVertex2f(startX, topY + 5);
        glVertex2f(startX + remainingBuffer + timeLeftInCurrentBuffer, topY + 5);
        glVertex2f(startX + remainingBuffer + timeLeftInCurrentBuffer, bottomY - 5);
        glVertex2f(startX, bottomY - 5);
        glEnd();
        
        if (data->averagedLatency == 0.0) data->averagedLatency = remainingBuffer + timeLeftInCurrentBuffer;
        else data->averagedLatency = 0.99*data->averagedLatency + 0.01*((float)remainingBuffer + (float)timeLeftInCurrentBuffer);
        
        //  Show a yellow bar with the averaged msecs latency you are hearing (from time of packet receipt)
        glColor3f(1,1,0);
        glBegin(GL_QUADS);
        glVertex2f(startX + data->averagedLatency - 2, topY - 2);
        glVertex2f(startX + data->averagedLatency + 2, topY - 2);
        glVertex2f(startX + data->averagedLatency + 2, bottomY + 2);
        glVertex2f(startX + data->averagedLatency - 2, bottomY + 2);
        glEnd();
        
        char out[20];
        sprintf(out, "%3.0f\n", data->averagedLatency/(float)frameWidth*(1000.0*(float)BUFFER_LENGTH_SAMPLES/(float)SAMPLE_RATE));
        drawtext(startX + data->averagedLatency - 10, topY-10, 0.08, 0, 1, 0, out, 1,1,0);
        
        //  Show a Cyan bar with the most recently measured jitter stdev
        
        
        int jitterPels = (float) data->jitter/ ((1000.0*(float)BUFFER_LENGTH_SAMPLES/(float)SAMPLE_RATE)) * (float)frameWidth;
        
        glColor3f(0,1,1);
        glBegin(GL_QUADS);
        glVertex2f(startX + jitterPels - 2, topY - 2);
        glVertex2f(startX + jitterPels + 2, topY - 2);
        glVertex2f(startX + jitterPels + 2, bottomY + 2);
        glVertex2f(startX + jitterPels - 2, bottomY + 2);
        glEnd();
        
        sprintf(out,"%3.1f\n", data->jitter);
        drawtext(startX + jitterPels - 5, topY-10, 0.08, 0, 1, 0, out, 0,1,1);

        
        
        //glVertex2f(nextOutputX, topY);
        //glVertex2f(nextOutputX, bottomY);
        
        /*
        float nextOutputX = startX + (nextOutputSampleOffset / RING_BUFFER_SIZE_SAMPLES) * scaleLength;
        float endLastWriteSampleOffset = data->ringBuffer->endOfLastWrite - data->ringBuffer->buffer;
        
        if (data->ringBuffer->endOfLastWrite == NULL) {
            endLastWriteSampleOffset = 0;
        }
        
        float endLastWriteX = startX + (endLastWriteSampleOffset / RING_BUFFER_SIZE_SAMPLES) * scaleLength;
        glColor3f(0, 1, 0);
        glVertex2f(endLastWriteX, topY);
        glVertex2f(endLastWriteX, bottomY);
        
        glEnd();
         */
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
    if (initialized) {
        initialized = false;
        
        err = Pa_CloseStream(stream);
        if (err != paNoError) goto error;
        
        err = Pa_Terminate();
        if (err != paNoError) goto error;
        
        delete data;
        
        logFile.close();
    }
    
    return true;
    
error:
    fprintf(stderr, "-- portaudio termination error --\n");
    fprintf(stderr, "PortAudio error (%d): %s\n", err, Pa_GetErrorText(err));
    return false;
}

