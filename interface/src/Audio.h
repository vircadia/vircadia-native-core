//
//  Audio.h
//  interface
//
//  Created by Stephen Birarda on 1/22/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Audio__
#define __interface__Audio__

#include <portaudio.h>

#include <AudioRingBuffer.h>

#include "Oscilloscope.h"
#include "Avatar.h"

const int NUM_AUDIO_CHANNELS = 2;

const int PACKET_LENGTH_BYTES = 1024;
const int PACKET_LENGTH_BYTES_PER_CHANNEL = PACKET_LENGTH_BYTES / 2;
const int PACKET_LENGTH_SAMPLES = PACKET_LENGTH_BYTES / sizeof(int16_t);
const int PACKET_LENGTH_SAMPLES_PER_CHANNEL = PACKET_LENGTH_SAMPLES / 2;

class Audio {
public:
    // initializes audio I/O
    Audio(Oscilloscope* scope, int16_t initialJitterBufferSamples);
    ~Audio();

    void render(int screenWidth, int screenHeight);
    
    float getLastInputLoudness() const { return _lastInputLoudness; };
    
    void setLastAcceleration(glm::vec3 lastAcceleration) { _lastAcceleration = lastAcceleration; };
    void setLastVelocity(glm::vec3 lastVelocity) { _lastVelocity = lastVelocity; };
    
    void setJitterBufferSamples(int samples) { _jitterBufferSamples = samples; };
    
    void addProceduralSounds(int16_t* inputBuffer, int numSamples);
    void lowPassFilter(int16_t* inputBuffer);
    void analyzeEcho(int16_t* inputBuffer, int16_t* outputBuffer, int numSamples);
    void addReceivedAudioToBuffer(unsigned char* receivedData, int receivedBytes);
    
    void startEchoTest();
    void renderEchoCompare();
    
private:    
    PaStream* _stream;
    AudioRingBuffer _ringBuffer;
    Oscilloscope* _scope;
    timeval _lastCallbackTime;
    timeval _lastReceiveTime;
    float _averagedLatency;
    float _measuredJitter;
    int16_t _jitterBufferSamples;
    int _wasStarved;
    float _lastInputLoudness;
    glm::vec3 _lastVelocity;
    glm::vec3 _lastAcceleration;
    int _totalPacketsReceived;
    timeval _firstPacketReceivedTime;
    int _packetsReceivedThisPlayback;
    // Echo Analysis
    bool _shouldStartEcho;
    bool _isSendingEchoPing;
    int16_t* _echoInputSamples;
    int16_t* _echoOutputSamples;
    int _echoInputFrameCountdown;
    bool _isGatheringEchoOutputFrames;

    
    
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
