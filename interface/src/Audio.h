//
//  Audio.h
//  interface
//
//  Created by Stephen Birarda on 1/22/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Audio__
#define __interface__Audio__

#include <fstream>
#include <vector>

#include "InterfaceConfig.h"

#include <QtCore/QObject>
#include <QtMultimedia/QAudioFormat>

#include <AbstractAudioInterface.h>
#include <AudioRingBuffer.h>
#include <StdDev.h>

#include "Oscilloscope.h"

#include <QGLWidget>

static const int NUM_AUDIO_CHANNELS = 2;

class QAudioInput;
class QAudioOutput;
class QIODevice;

class Audio : public QObject, public AbstractAudioInterface {
    Q_OBJECT
public:
    // setup for audio I/O
    Audio(Oscilloscope* scope, int16_t initialJitterBufferSamples, QObject* parent = 0);
    
    void render(int screenWidth, int screenHeight);
    
    float getLastInputLoudness() const { return _lastInputLoudness; }
    
    void setLastAcceleration(const glm::vec3 lastAcceleration) { _lastAcceleration = lastAcceleration; }
    void setLastVelocity(const glm::vec3 lastVelocity) { _lastVelocity = lastVelocity; }
    
    void setJitterBufferSamples(int samples) { _jitterBufferSamples = samples; }
    int getJitterBufferSamples() { return _jitterBufferSamples; }
    
    void lowPassFilter(int16_t* inputBuffer);
    
    virtual void startCollisionSound(float magnitude, float frequency, float noise, float duration, bool flashScreen);
    virtual void startDrumSound(float volume, float frequency, float duration, float decay);

    float getCollisionSoundMagnitude() { return _collisionSoundMagnitude; }
    
    bool getCollisionFlashesScreen() { return _collisionFlashesScreen; }
    
    void init(QGLWidget *parent = 0);
    bool mousePressEvent(int x, int y);
    
public slots:
    void start();
    void addReceivedAudioToBuffer(const QByteArray& audioByteArray);
    void handleAudioInput();
    void reset();
    
private:
    QByteArray firstInputFrame;
    QAudioInput* _audioInput;
    QAudioFormat _desiredInputFormat;
    QAudioFormat _inputFormat;
    QIODevice* _inputDevice;
    int16_t _localInjectedSamples[NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL];
    int _numInputCallbackBytes;
    QAudioOutput* _audioOutput;
    QAudioFormat _desiredOutputFormat;
    QAudioFormat _outputFormat;
    QIODevice* _outputDevice;
    int _numOutputCallbackBytes;
    AudioRingBuffer _inputRingBuffer;
    AudioRingBuffer _ringBuffer;
    Oscilloscope* _scope;
    StDev _stdev;
    timeval _lastReceiveTime;
    float _averagedLatency;
    float _measuredJitter;
    int16_t _jitterBufferSamples;
    float _lastInputLoudness;
    glm::vec3 _lastVelocity;
    glm::vec3 _lastAcceleration;
    int _totalPacketsReceived;
    
    float _collisionSoundMagnitude;
    float _collisionSoundFrequency;
    float _collisionSoundNoise;
    float _collisionSoundDuration;
    bool _collisionFlashesScreen;
    
    // Drum sound generator
    float _drumSoundVolume;
    float _drumSoundFrequency;
    float _drumSoundDuration;
    float _drumSoundDecay;
    int _drumSoundSample;
    
    int _proceduralEffectSample;
    int _numFramesDisplayStarve;
    bool _muted;
    bool _localEcho;
    GLuint _micTextureId;
    GLuint _muteTextureId;
    QRect _iconBounds;
    
    // Audio callback in class context.
    inline void performIO(int16_t* inputLeft, int16_t* outputLeft, int16_t* outputRight);
    
    // Add sounds that we want the user to not hear themselves, by adding on top of mic input signal
    void addProceduralSounds(int16_t* monoInput, int numSamples);
    
    void renderToolIcon(int screenHeight);
};


#endif /* defined(__interface__audio__) */