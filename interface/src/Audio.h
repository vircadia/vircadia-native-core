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
#include <StdDev.h>

#include "Oscilloscope.h"
#include "Avatar.h"

static const int NUM_AUDIO_CHANNELS = 2;

static const int PACKET_LENGTH_BYTES = 1024;
static const int PACKET_LENGTH_BYTES_PER_CHANNEL = PACKET_LENGTH_BYTES / 2;
static const int PACKET_LENGTH_SAMPLES = PACKET_LENGTH_BYTES / sizeof(int16_t);
static const int PACKET_LENGTH_SAMPLES_PER_CHANNEL = PACKET_LENGTH_SAMPLES / 2;

class Audio {
public:
    // initializes audio I/O
    Audio(Oscilloscope* scope, int16_t initialJitterBufferSamples);
    ~Audio();

    void reset(); 
    void render(int screenWidth, int screenHeight);
    
    void addReceivedAudioToBuffer(unsigned char* receivedData, int receivedBytes);

    float getLastInputLoudness() const { return _lastInputLoudness; };
    
    void setLastAcceleration(glm::vec3 lastAcceleration) { _lastAcceleration = lastAcceleration; };
    void setLastVelocity(glm::vec3 lastVelocity) { _lastVelocity = lastVelocity; };
    
    void setJitterBufferSamples(int samples) { _jitterBufferSamples = samples; };
    int getJitterBufferSamples() { return _jitterBufferSamples; };
    
    void lowPassFilter(int16_t* inputBuffer);
    
    void startCollisionSound(float magnitude, float frequency, float noise, float duration);
    float getCollisionSoundMagnitude() { return _collisionSoundMagnitude; };
    
    
    void ping();

    // Call periodically to eventually perform round trip time analysis,
    // in which case 'true' is returned - otherwise the return value is 'false'.
    // The results of the analysis are written to the log.
    bool eventuallyAnalyzePing();


private:    
    PaStream* _stream;
    AudioRingBuffer _ringBuffer;
    Oscilloscope* _scope;
    StDev _stdev;
    timeval _lastCallbackTime;
    timeval _lastReceiveTime;
    float _averagedLatency;
    float _measuredJitter;
    int16_t _jitterBufferSamples;
    int _wasStarved;
    int _numStarves;
    float _lastInputLoudness;
    glm::vec3 _lastVelocity;
    glm::vec3 _lastAcceleration;
    int _totalPacketsReceived;
    timeval _firstPacketReceivedTime;
    int _packetsReceivedThisPlayback;
    // Ping analysis
    int16_t* _echoSamplesLeft;
    volatile bool _isSendingEchoPing;
    volatile bool _pingAnalysisPending;
    int _pingFramesToRecord;
    // Flange effect
    int _samplesLeftForFlange;
    int _lastYawMeasuredMaximum;
    float _flangeIntensity;
    float _flangeRate;
    float _flangeWeight;
    float _collisionSoundMagnitude;
    float _collisionSoundFrequency;
    float _collisionSoundNoise;
    float _collisionSoundDuration;
    int _proceduralEffectSample;
    float _heartbeatMagnitude;
    
    // Audio callback in class context.
    inline void performIO(int16_t* inputLeft, int16_t* outputLeft, int16_t* outputRight);

    // When requested, sends/receives a signal for round trip time determination.
    // Called from 'performIO'.
    inline void eventuallySendRecvPing(int16_t* inputLeft, int16_t* outputLeft, int16_t* outputRight);

    // Determines round trip time of the audio system. Called from 'eventuallyAnalyzePing'.
    inline void analyzePing();

    // Add sounds that we want the user to not hear themselves, by adding on top of mic input signal
    void addProceduralSounds(int16_t* inputBuffer, int16_t* outputLeft, int16_t* outputRight, int numSamples);


    // Audio callback called by portaudio. Calls 'performIO'.
    static int audioCallback(const void *inputBuffer,
                             void *outputBuffer,
                             unsigned long framesPerBuffer,
                             const PaStreamCallbackTimeInfo *timeInfo,
                             PaStreamCallbackFlags statusFlags,
                             void *userData);

};


#endif /* defined(__interface__audio__) */
