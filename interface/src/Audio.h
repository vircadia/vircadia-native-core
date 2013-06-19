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

#include <speex/speex_echo.h>
#include <speex/speex_preprocess.h>

#include <AudioRingBuffer.h>
#include <StdDev.h>

#include "Oscilloscope.h"
#include "Avatar.h"

class Audio {
public:
    // initializes audio I/O
    Audio(Oscilloscope* scope);
    ~Audio();

    void render(int screenWidth, int screenHeight);
    
    void addReceivedAudioToBuffer(unsigned char* receivedData, int receivedBytes);

    float getLastInputLoudness() const { return _lastInputLoudness; };
    
    void setLastAcceleration(glm::vec3 lastAcceleration) { _lastAcceleration = lastAcceleration; };
    void setLastVelocity(glm::vec3 lastVelocity) { _lastVelocity = lastVelocity; };

    void setIsCancellingEcho(bool enabled) { _isCancellingEcho = enabled; }
    bool isCancellingEcho() const { return _isCancellingEcho; }

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
    float _jitterBufferLengthMsecs;
//    short _jitterBufferSamples; // currently unsused
    int _wasStarved;
    int _numStarves;
    float _lastInputLoudness;
    glm::vec3 _lastVelocity;
    glm::vec3 _lastAcceleration;
    int _totalPacketsReceived;
    timeval _firstPlaybackTime;
    int _packetsReceivedThisPlayback;
    // Echo cancellation
    volatile bool _isCancellingEcho;
    unsigned _echoWritePos;
    unsigned _echoDelay;
    int16_t* _echoSamplesLeft;
    int16_t* _echoSamplesRight;
    int16_t* _speexTmpBuf;
    SpeexEchoState* _speexEchoState;
    SpeexPreprocessState* _speexPreprocessState;
    // Ping analysis
    volatile bool _isSendingEchoPing;
    volatile bool _pingAnalysisPending;
    int _pingFramesToRecord;
    // Flange effect
    int _samplesLeftForFlange;
    int _lastYawMeasuredMaximum;
    float _flangeIntensity;
    float _flangeRate;
    float _flangeWeight;

    // Audio callback in class context.
    inline void performIO(int16_t* inputLeft, int16_t* outputLeft, int16_t* outputRight);

    // When echo cancellation is enabled, subtract recorded echo from the input.
    // Called from 'performIO' before the input has been processed.
    inline void eventuallyCancelEcho(int16_t* inputLeft);
    // When EC is enabled, record output samples.
    // Called from 'performIO' after the output has been generated.
    inline void eventuallyRecordEcho(int16_t* outputLeft, int16_t* outputRight);

    // When requested, sends/receives a signal for round trip time determination.
    // Called from 'performIO'.
    inline void eventuallySendRecvPing(int16_t* inputLeft, int16_t* outputLeft, int16_t* outputRight);

    // Determines round trip time of the audio system. Called from 'eventuallyAnalyzePing'.
    inline void analyzePing();

    void addProceduralSounds(int16_t* inputBuffer, int numSamples);


    // Audio callback called by portaudio. Calls 'performIO'.
    static int audioCallback(const void *inputBuffer,
                             void *outputBuffer,
                             unsigned long framesPerBuffer,
                             const PaStreamCallbackTimeInfo *timeInfo,
                             PaStreamCallbackFlags statusFlags,
                             void *userData);

};


#endif /* defined(__interface__audio__) */
