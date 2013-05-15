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

class Audio {
public:
    // initializes audio I/O
    Audio(Oscilloscope* scope);
    ~Audio();

    void render(int screenWidth, int screenHeight);
    
    void setMixerLoopbackFlag(bool mixerLoopbackFlag) { _mixerLoopbackFlag = mixerLoopbackFlag; }
    
    float getLastInputLoudness() const { return _lastInputLoudness; };
    
    void setLastAcceleration(glm::vec3 lastAcceleration) { _lastAcceleration = lastAcceleration; };
    void setLastVelocity(glm::vec3 lastVelocity) { _lastVelocity = lastVelocity; };
    
    void addProceduralSounds(int16_t* inputBuffer, int numSamples);
    void analyzeEcho(int16_t* inputBuffer, int16_t* outputBuffer, int numSamples);

    
    void addReceivedAudioToBuffer(unsigned char* receivedData, int receivedBytes);
private:    
    PaStream* _stream;
    AudioRingBuffer _ringBuffer;
    Oscilloscope* _scope;
    timeval _lastCallbackTime;
    timeval _lastReceiveTime;
    float _averagedLatency;
    float _measuredJitter;
    int _wasStarved;
    float _lastInputLoudness;
    bool _mixerLoopbackFlag;
    glm::vec3 _lastVelocity;
    glm::vec3 _lastAcceleration;
    int _totalPacketsReceived;
    timeval _firstPlaybackTime;
    int _packetsReceivedThisPlayback;
    
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
