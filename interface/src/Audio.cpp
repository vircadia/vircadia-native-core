//
//  Audio.cpp
//  interface
//
//  Created by Stephen Birarda on 1/22/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
#ifndef _WIN32

#include <iostream>
#include <fstream>
#include <pthread.h>
#include <sys/stat.h>
#include <cstring>

#include <StdDev.h>
#include <UDPSocket.h>
#include <SharedUtil.h>
#include <PacketHeaders.h>
#include <AgentList.h>
#include <AgentTypes.h>

#include "Application.h"
#include "Audio.h"
#include "Util.h"
#include "Log.h"

const int NUM_AUDIO_CHANNELS = 2;

const int PACKET_LENGTH_BYTES = 1024;
const int PACKET_LENGTH_BYTES_PER_CHANNEL = PACKET_LENGTH_BYTES / 2;
const int PACKET_LENGTH_SAMPLES = PACKET_LENGTH_BYTES / sizeof(int16_t);
const int PACKET_LENGTH_SAMPLES_PER_CHANNEL = PACKET_LENGTH_SAMPLES / 2;

const int BUFFER_LENGTH_BYTES = 512;
const int BUFFER_LENGTH_SAMPLES = BUFFER_LENGTH_BYTES / sizeof(int16_t);

const int RING_BUFFER_FRAMES = 10;
const int RING_BUFFER_SAMPLES = RING_BUFFER_FRAMES * BUFFER_LENGTH_SAMPLES;

const int PHASE_DELAY_AT_90 = 20;
const float AMPLITUDE_RATIO_AT_90 = 0.5;

const int MIN_FLANGE_EFFECT_THRESHOLD = 600;
const int MAX_FLANGE_EFFECT_THRESHOLD = 1500;
const float FLANGE_BASE_RATE = 4;
const float MAX_FLANGE_SAMPLE_WEIGHT = 0.50;
const float MIN_FLANGE_INTENSITY = 0.25;

const int SAMPLE_RATE = 22050;
const float JITTER_BUFFER_LENGTH_MSECS = 12;
const short JITTER_BUFFER_SAMPLES = JITTER_BUFFER_LENGTH_MSECS *
                                    NUM_AUDIO_CHANNELS * (SAMPLE_RATE / 1000.0);

const float AUDIO_CALLBACK_MSECS = (float)BUFFER_LENGTH_SAMPLES / (float)SAMPLE_RATE * 1000.0;


const int AGENT_LOOPBACK_MODIFIER = 307;

int numStarves = 0;
StDev stdev;

int samplesLeftForFlange = 0;
int lastYawMeasuredMaximum = 0;
float flangeIntensity = 0;
float flangeRate = 0;
float flangeWeight = 0;

float usecsAtStartup = 0;

// inputBuffer  A pointer to an internal portaudio data buffer containing data read by portaudio.
// outputBuffer A pointer to an internal portaudio data buffer to be read by the configured output device.
// frames       Number of frames that portaudio requests to be read/written.
// timeInfo     Portaudio time info. Currently unused.
// statusFlags  Portaudio status flags. Currently unused.
// userData     Pointer to supplied user data (in this case, a pointer to the parent Audio object
int audioCallback (const void* inputBuffer,
                   void* outputBuffer,
                   unsigned long frames,
                   const PaStreamCallbackTimeInfo *timeInfo,
                   PaStreamCallbackFlags statusFlags,
                   void* userData) {
    
    Audio* parentAudio = (Audio*) userData;
    AgentList* agentList = AgentList::getInstance();
    
    Application* interface = (Application*) QCoreApplication::instance();
    Avatar* interfaceAvatar = interface->getAvatar();
    
    int16_t *inputLeft = ((int16_t **) inputBuffer)[0];
    int16_t *outputLeft = ((int16_t **) outputBuffer)[0];
    int16_t *outputRight = ((int16_t **) outputBuffer)[1];

    // Add Procedural effects to input samples
    parentAudio->addProceduralSounds(inputLeft, BUFFER_LENGTH_SAMPLES);
    
    // add output (@speakers) data to the scope
    parentAudio->_scope->addSamples(1, outputLeft, PACKET_LENGTH_SAMPLES_PER_CHANNEL);
    parentAudio->_scope->addSamples(2, outputRight, PACKET_LENGTH_SAMPLES_PER_CHANNEL);
    
    // if needed, add input/output data to echo analysis buffers
    if (parentAudio->_gatheringEchoFrames) {
        memcpy(parentAudio->_echoInputSamples, inputLeft,
               PACKET_LENGTH_SAMPLES_PER_CHANNEL * sizeof(int16_t));
        memcpy(parentAudio->_echoOutputSamples, outputLeft,
               PACKET_LENGTH_SAMPLES_PER_CHANNEL * sizeof(int16_t));
        parentAudio->addedPingFrame();
    }
    
    if (inputLeft != NULL) {
        
        //  Measure the loudness of the signal from the microphone and store in audio object
        float loudness = 0;
        for (int i = 0; i < BUFFER_LENGTH_SAMPLES; i++) {
            loudness += abs(inputLeft[i]);
        }
        
        loudness /= BUFFER_LENGTH_SAMPLES;
        parentAudio->_lastInputLoudness = loudness;
        
        // add input (@microphone) data to the scope
        parentAudio->_scope->addSamples(0, inputLeft, BUFFER_LENGTH_SAMPLES);
        
        Agent* audioMixer = agentList->soloAgentOfType(AGENT_TYPE_AUDIO_MIXER);
        
        if (audioMixer) {
            int leadingBytes = 2 + (sizeof(float) * 4);
            
            // we need the amount of bytes in the buffer + 1 for type
            // + 12 for 3 floats for position + float for bearing + 1 attenuation byte
            unsigned char dataPacket[BUFFER_LENGTH_BYTES + leadingBytes];
            
            dataPacket[0] = PACKET_HEADER_MICROPHONE_AUDIO;
            unsigned char *currentPacketPtr = dataPacket + 1;
            
            // memcpy the three float positions
            memcpy(currentPacketPtr, &interfaceAvatar->getHeadPosition(), sizeof(float) * 3);
            currentPacketPtr += (sizeof(float) * 3);
            
            // tell the mixer not to add additional attenuation to our source
            *(currentPacketPtr++) = 255;
            
            // memcpy the corrected render yaw
            float correctedYaw = fmodf(-1 * interfaceAvatar->getAbsoluteHeadYaw(), 360);
            
            if (correctedYaw > 180) {
                correctedYaw -= 360;
            } else if (correctedYaw < -180) {
                correctedYaw += 360;
            }
            
            if (parentAudio->_mixerLoopbackFlag) {
                correctedYaw = correctedYaw > 0
                ? correctedYaw + AGENT_LOOPBACK_MODIFIER
                : correctedYaw - AGENT_LOOPBACK_MODIFIER;
            }
            
            memcpy(currentPacketPtr, &correctedYaw, sizeof(float));
            currentPacketPtr += sizeof(float);
            
            // copy the audio data to the last BUFFER_LENGTH_BYTES bytes of the data packet
            memcpy(currentPacketPtr, inputLeft, BUFFER_LENGTH_BYTES);
            
            agentList->getAgentSocket()->send(audioMixer->getActiveSocket(), dataPacket, BUFFER_LENGTH_BYTES + leadingBytes);
        }
        
    }
    
    memset(outputLeft, 0, PACKET_LENGTH_BYTES_PER_CHANNEL);
    memset(outputRight, 0, PACKET_LENGTH_BYTES_PER_CHANNEL);
    
    AudioRingBuffer* ringBuffer = &parentAudio->_ringBuffer;
    
    // if we've been reset, and there isn't any new packets yet
    // just play some silence
    
    if (ringBuffer->getEndOfLastWrite() != NULL) {
        
        if (!ringBuffer->isStarted() && ringBuffer->diffLastWriteNextOutput() < PACKET_LENGTH_SAMPLES + JITTER_BUFFER_SAMPLES) {
            //printLog("Held back, buffer has %d of %d samples required.\n",
            //         ringBuffer->diffLastWriteNextOutput(), PACKET_LENGTH_SAMPLES + JITTER_BUFFER_SAMPLES);
        } else if (ringBuffer->diffLastWriteNextOutput() < PACKET_LENGTH_SAMPLES) {
            ringBuffer->setStarted(false);
            
            ::numStarves++;
            parentAudio->_packetsReceivedThisPlayback = 0;
            
            // printLog("Starved #%d\n", starve_counter);
            parentAudio->_wasStarved = 10;      //   Frames to render the indication that the system was starved.
        } else {
            if (!ringBuffer->isStarted()) {
                ringBuffer->setStarted(true);
                // printLog("starting playback %3.1f msecs delayed \n", (usecTimestampNow() - usecTimestamp(&firstPlaybackTimer))/1000.0);
            } else {
                // printLog("pushing buffer\n");
            }
            // play whatever we have in the audio buffer
            
            // if we haven't fired off the flange effect, check if we should
            // TODO: lastMeasuredHeadYaw is now relative to body - check if this still works.
            
            int lastYawMeasured = fabsf(interfaceAvatar->getLastMeasuredHeadYaw());
            
            if (!::samplesLeftForFlange && lastYawMeasured > MIN_FLANGE_EFFECT_THRESHOLD) {
                // we should flange for one second
                if ((::lastYawMeasuredMaximum = std::max(::lastYawMeasuredMaximum, lastYawMeasured)) != lastYawMeasured) {
                    ::lastYawMeasuredMaximum = std::min(::lastYawMeasuredMaximum, MIN_FLANGE_EFFECT_THRESHOLD);
                    
                    ::samplesLeftForFlange = SAMPLE_RATE;
                    
                    ::flangeIntensity = MIN_FLANGE_INTENSITY +
                        ((::lastYawMeasuredMaximum - MIN_FLANGE_EFFECT_THRESHOLD) /
                         (float)(MAX_FLANGE_EFFECT_THRESHOLD - MIN_FLANGE_EFFECT_THRESHOLD)) *
                        (1 - MIN_FLANGE_INTENSITY);
                    
                    ::flangeRate = FLANGE_BASE_RATE * ::flangeIntensity;
                    ::flangeWeight = MAX_FLANGE_SAMPLE_WEIGHT * ::flangeIntensity;
                }
            }
            
            for (int s = 0; s < PACKET_LENGTH_SAMPLES_PER_CHANNEL; s++) {
                
                int leftSample = ringBuffer->getNextOutput()[s];
                int rightSample = ringBuffer->getNextOutput()[s + PACKET_LENGTH_SAMPLES_PER_CHANNEL];
                
                if (::samplesLeftForFlange > 0) {
                    float exponent = (SAMPLE_RATE - ::samplesLeftForFlange - (SAMPLE_RATE / ::flangeRate)) /
                        (SAMPLE_RATE / ::flangeRate);
                    int sampleFlangeDelay = (SAMPLE_RATE / (1000 * ::flangeIntensity)) * powf(2, exponent);
                    
                    if (::samplesLeftForFlange != SAMPLE_RATE || s >= (SAMPLE_RATE / 2000)) {
                        // we have a delayed sample to add to this sample
                        
                        int16_t *flangeFrame = ringBuffer->getNextOutput();
                        int flangeIndex = s - sampleFlangeDelay;
                        
                        if (flangeIndex < 0) {
                            // we need to grab the flange sample from earlier in the buffer
                            flangeFrame = ringBuffer->getNextOutput() != ringBuffer->getBuffer()
                            ? ringBuffer->getNextOutput() - PACKET_LENGTH_SAMPLES
                            : ringBuffer->getNextOutput() + RING_BUFFER_SAMPLES - PACKET_LENGTH_SAMPLES;
                            
                            flangeIndex = PACKET_LENGTH_SAMPLES_PER_CHANNEL + (s - sampleFlangeDelay);
                        }
                        
                        int16_t leftFlangeSample = flangeFrame[flangeIndex];
                        int16_t rightFlangeSample = flangeFrame[flangeIndex + PACKET_LENGTH_SAMPLES_PER_CHANNEL];
                        
                        leftSample = (1 - ::flangeWeight) * leftSample + (::flangeWeight * leftFlangeSample);
                        rightSample = (1 - ::flangeWeight) * rightSample + (::flangeWeight * rightFlangeSample);
                        
                        ::samplesLeftForFlange--;
                        
                        if (::samplesLeftForFlange == 0) {
                            ::lastYawMeasuredMaximum = 0;
                        }
                    }
                }
                
                outputLeft[s] = leftSample;
                outputRight[s] = rightSample;
            }
            ringBuffer->setNextOutput(ringBuffer->getNextOutput() + PACKET_LENGTH_SAMPLES);
            
            if (ringBuffer->getNextOutput() == ringBuffer->getBuffer() + RING_BUFFER_SAMPLES) {
                ringBuffer->setNextOutput(ringBuffer->getBuffer());
            }
        }
    }
    if (parentAudio->_sendingEchoPing) {
        const float PING_PITCH = 4.f;
        const float PING_VOLUME = 32000.f;
        for (int s = 0; s < PACKET_LENGTH_SAMPLES_PER_CHANNEL; s++) {
            outputLeft[s] = outputRight[s] = (int16_t)(sinf((float) s / PING_PITCH) * PING_VOLUME);
        }
        parentAudio->_gatheringEchoFrames = true;
    }
    gettimeofday(&parentAudio->_lastCallbackTime, NULL);
    return paContinue;
}


void outputPortAudioError(PaError error) {
    if (error != paNoError) {
        printLog("-- portaudio termination error --\n");
        printLog("PortAudio error (%d): %s\n", error, Pa_GetErrorText(error));
    }
}

Audio::Audio(Oscilloscope* scope) :
    _stream(NULL),
    _ringBuffer(RING_BUFFER_SAMPLES, PACKET_LENGTH_SAMPLES),
    _scope(scope),
    _averagedLatency(0.0),
    _measuredJitter(0),
    _wasStarved(0),
    _lastInputLoudness(0),
    _mixerLoopbackFlag(false),
    _lastVelocity(0),
    _lastAcceleration(0),
    _totalPacketsReceived(0),
    _firstPlaybackTime(),
    _packetsReceivedThisPlayback(0),
    _startEcho(false),
    _sendingEchoPing(false),
    _echoPingFrameCount(0),
    _gatheringEchoFrames(false)
{
    outputPortAudioError(Pa_Initialize());
    outputPortAudioError(Pa_OpenDefaultStream(&_stream,
                                              2,
                                              2,
                                              (paInt16 | paNonInterleaved),
                                              SAMPLE_RATE,
                                              BUFFER_LENGTH_SAMPLES,
                                              audioCallback,
                                              (void*) this));
    
    // start the stream now that sources are good to go
    outputPortAudioError(Pa_StartStream(_stream));
    
    _echoInputSamples = new int16_t[BUFFER_LENGTH_BYTES];
    _echoOutputSamples = new int16_t[BUFFER_LENGTH_BYTES];
    memset(_echoInputSamples, 0, BUFFER_LENGTH_SAMPLES * sizeof(int));
    memset(_echoOutputSamples, 0, BUFFER_LENGTH_SAMPLES * sizeof(int));
    
    gettimeofday(&_lastReceiveTime, NULL);
}

Audio::~Audio() {    
    if (_stream) {
        outputPortAudioError(Pa_CloseStream(_stream));
        outputPortAudioError(Pa_Terminate());
    }
}

void Audio::renderEchoCompare() {
    const int XPOS = 0;
    const int YPOS = 500;
    const int YSCALE = 500;
    const int XSCALE = 2;
    glPointSize(1.0);
    glLineWidth(1.0);
    glDisable(GL_LINE_SMOOTH);
    glColor3f(1,1,1);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < BUFFER_LENGTH_SAMPLES; i++) {
        glVertex2f(XPOS + i * XSCALE, YPOS + _echoInputSamples[i]/YSCALE);
    }
    glEnd();
    glColor3f(0,1,1);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < BUFFER_LENGTH_SAMPLES; i++) {
        glVertex2f(XPOS + i * XSCALE, YPOS + _echoOutputSamples[i]/YSCALE);
    }
    glEnd();
}

//  Take a pointer to the acquired microphone input samples and add procedural sounds
void Audio::addProceduralSounds(int16_t* inputBuffer, int numSamples) {
    const float MAX_AUDIBLE_VELOCITY = 6.0;
    const float MIN_AUDIBLE_VELOCITY = 0.1;
    const int VOLUME_BASELINE = 400;
    const float SOUND_PITCH = 8.f;
    
    float speed = glm::length(_lastVelocity);
    float volume = VOLUME_BASELINE * (1.f - speed / MAX_AUDIBLE_VELOCITY);
    
    //  Add a noise-modulated sinewave with volume that tapers off with speed increasing
    if ((speed > MIN_AUDIBLE_VELOCITY) && (speed < MAX_AUDIBLE_VELOCITY)) {
        for (int i = 0; i < numSamples; i++) {
            inputBuffer[i] += (int16_t)((sinf((float) i / SOUND_PITCH * speed) * randFloat()) * volume * speed);
        }
    }
}

void Audio::startEchoTest() {
    _startEcho = true;
    _echoPingFrameCount = 0;
    _sendingEchoPing = true;
    _gatheringEchoFrames = false;
}

void Audio::addedPingFrame() {
    const int ECHO_PING_FRAMES = 1;
    _echoPingFrameCount++;
    if (_echoPingFrameCount == ECHO_PING_FRAMES) {
        _gatheringEchoFrames = false;
        _sendingEchoPing = false;
        //startEchoTest();
    }
}
void Audio::analyzeEcho(int16_t* inputBuffer, int16_t* outputBuffer, int numSamples) {
    //  Compare output and input streams, looking for evidence of correlation needing echo cancellation
    //
    //  OFFSET_RANGE tells us how many samples to vary the analysis window when looking for correlation,
    //  and should be equal to the largest physical distance between speaker and microphone, where
    //  OFFSET_RANGE =  1 / (speedOfSound (meters / sec) / SamplingRate (samples / sec)) * distance
    //
    const int OFFSET_RANGE = 10;
    const int SIGNAL_FLOOR = 1000;
    float correlation[2 * OFFSET_RANGE + 1];
    int numChecked = 0;
    bool foundSignal = false;
    
    memset(correlation, 0, sizeof(float) * (2 * OFFSET_RANGE + 1));
    
    for (int offset = -OFFSET_RANGE; offset <= OFFSET_RANGE; offset++) {
        for (int i = 0; i < numSamples; i++) {
            if ((i + offset >= 0) && (i + offset < numSamples)) {
                correlation[offset + OFFSET_RANGE] +=
                (float) abs(inputBuffer[i] - outputBuffer[i + offset]);
                numChecked++;
                foundSignal |= (inputBuffer[i] > SIGNAL_FLOOR);
            }
        }
        correlation[offset + OFFSET_RANGE] /= numChecked;
        numChecked = 0;
        if (foundSignal) {
            printLog("%4.2f, ", correlation[offset + OFFSET_RANGE]);
        }
    }
    if (foundSignal) printLog("\n");
}


void Audio::addReceivedAudioToBuffer(unsigned char* receivedData, int receivedBytes) {
    const int NUM_INITIAL_PACKETS_DISCARD = 3;
    
    timeval currentReceiveTime;
    gettimeofday(&currentReceiveTime, NULL);
    _totalPacketsReceived++;
    
    double timeDiff = diffclock(&_lastReceiveTime, &currentReceiveTime);
    
    //  Discard first few received packets for computing jitter (often they pile up on start)
    if (_totalPacketsReceived > NUM_INITIAL_PACKETS_DISCARD) {
        ::stdev.addValue(timeDiff);
    }
    
    if (::stdev.getSamples() > 500) {
        _measuredJitter = ::stdev.getStDev();
        ::stdev.reset();
    }
    
    if (!_ringBuffer.isStarted()) {
        _packetsReceivedThisPlayback++;
    }
    
    if (_packetsReceivedThisPlayback == 1) {
        gettimeofday(&_firstPlaybackTime, NULL);
    }
    
    _ringBuffer.parseData((unsigned char *)receivedData, PACKET_LENGTH_BYTES);
    
    _lastReceiveTime = currentReceiveTime;
}

void Audio::render(int screenWidth, int screenHeight) {
    if (_stream) {
        glLineWidth(2.0);
        glBegin(GL_LINES);
        glColor3f(1,1,1);
        
        int startX = 20.0;
        int currentX = startX;
        int topY = screenHeight - 40;
        int bottomY = screenHeight - 20;
        float frameWidth = 20.0;
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
        if (_lastCallbackTime.tv_usec > 0) {
            timeLeftInCurrentBuffer = AUDIO_CALLBACK_MSECS - diffclock(&_lastCallbackTime, &currentTime);
        }
        
        if (_ringBuffer.getEndOfLastWrite() != NULL)
            remainingBuffer = _ringBuffer.diffLastWriteNextOutput() / PACKET_LENGTH_SAMPLES * AUDIO_CALLBACK_MSECS;
        
        if (_wasStarved == 0) {
            glColor3f(0, 1, 0);
        } else {
            glColor3f(0.5 + (_wasStarved / 20.0f), 0, 0);
            _wasStarved--;
        }
        
        glBegin(GL_QUADS);
        glVertex2f(startX, topY + 2);
        glVertex2f(startX + (remainingBuffer + timeLeftInCurrentBuffer)/AUDIO_CALLBACK_MSECS*frameWidth, topY + 2);
        glVertex2f(startX + (remainingBuffer + timeLeftInCurrentBuffer)/AUDIO_CALLBACK_MSECS*frameWidth, bottomY - 2);
        glVertex2f(startX, bottomY - 2);
        glEnd();
        
        if (_averagedLatency == 0.0) {
            _averagedLatency = remainingBuffer + timeLeftInCurrentBuffer;
        } else {
            _averagedLatency = 0.99f * _averagedLatency + 0.01f * (remainingBuffer + timeLeftInCurrentBuffer);
        }
        
        //  Show a yellow bar with the averaged msecs latency you are hearing (from time of packet receipt)
        glColor3f(1,1,0);
        glBegin(GL_QUADS);
        glVertex2f(startX + _averagedLatency / AUDIO_CALLBACK_MSECS * frameWidth - 2, topY - 2);
        glVertex2f(startX + _averagedLatency / AUDIO_CALLBACK_MSECS * frameWidth + 2, topY - 2);
        glVertex2f(startX + _averagedLatency / AUDIO_CALLBACK_MSECS * frameWidth + 2, bottomY + 2);
        glVertex2f(startX + _averagedLatency / AUDIO_CALLBACK_MSECS * frameWidth - 2, bottomY + 2);
        glEnd();
        
        char out[40];
        sprintf(out, "%3.0f\n", _averagedLatency);
        drawtext(startX + _averagedLatency / AUDIO_CALLBACK_MSECS * frameWidth - 10, topY - 10, 0.10, 0, 1, 0, out, 1,1,0);
        //drawtext(startX + 0, topY-10, 0.08, 0, 1, 0, out, 1,1,0);
        
        //  Show a Cyan bar with the most recently measured jitter stdev
        
        int jitterPels = _measuredJitter / ((1000.0f * PACKET_LENGTH_SAMPLES / SAMPLE_RATE)) * frameWidth;
        
        glColor3f(0,1,1);
        glBegin(GL_QUADS);
        glVertex2f(startX + jitterPels - 2, topY - 2);
        glVertex2f(startX + jitterPels + 2, topY - 2);
        glVertex2f(startX + jitterPels + 2, bottomY + 2);
        glVertex2f(startX + jitterPels - 2, bottomY + 2);
        glEnd();
        
        sprintf(out,"%3.1f\n", _measuredJitter);
        drawtext(startX + jitterPels - 5, topY-10, 0.10, 0, 1, 0, out, 0,1,1);
        
        sprintf(out, "%3.1fms\n", JITTER_BUFFER_LENGTH_MSECS);
        drawtext(startX - 10, bottomY + 15, 0.1, 0, 1, 0, out, 1, 0, 0);
    }
}

#endif
