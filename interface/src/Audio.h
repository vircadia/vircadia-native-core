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

    // Enable/disable audio echo cancellation.
    // Will request calibration when called with an argument of 'true'.
    // Echo cancellation will be enabled when it can be calibrated successfully.
    void setIsCancellingEcho(bool enabled);

    bool isCancellingEcho() const { return _isCancellingEcho; }

    // Call periodically to eventually recalibrate audio echo cancellation.
    // A return value of 'true' indicates that a calibration request has been processed.
    // In this case a subsequent call to 'isCancellingEcho' will report whether the 
    // calibration was successful.
    bool eventuallyCalibrateEchoCancellation();

    void testPing();

private:    
    PaStream* _stream;
    SpeexEchoState* _speexEchoState;
    SpeexPreprocessState* _speexPreprocessState;
    int16_t* _speexTmpBuf;
    AudioRingBuffer _ringBuffer;
    Oscilloscope* _scope;
    StDev _stdev;
    timeval _lastCallbackTime;
    timeval _lastReceiveTime;
    float _averagedLatency;
    float _measuredJitter;
    float _jitterBufferLengthMsecs;
    short _jitterBufferSamples;
    int _wasStarved;
    int _numStarves;
    float _lastInputLoudness;
    glm::vec3 _lastVelocity;
    glm::vec3 _lastAcceleration;
    int _totalPacketsReceived;
    timeval _firstPlaybackTime;
    int _packetsReceivedThisPlayback;
    // Echo Analysis
    volatile bool _isCancellingEcho;
    volatile bool _isSendingEchoPing;
    volatile bool _echoAnalysisPending;
    int _echoPingRetries;
    unsigned _echoWritePos;
    unsigned _echoDelay;
    int _echoInputFramesToRecord;
    int16_t* _echoSamplesLeft;
    int16_t* _echoSamplesRight;
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
    // When requested, performs sends/receives a signal for EC calibration.
    inline void eventuallySendRecvPing(int16_t* inputLeft, int16_t* outputLeft, int16_t* outputRight);
    // Analyses the calibration signal and determines delay/amplitude for EC.
    // Called from (public) 'eventuallyCalibrateEchoCancellation'.
    inline bool calibrateEchoCancellation();

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
