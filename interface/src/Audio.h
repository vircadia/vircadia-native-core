//
//  Audio.h
//  interface
//
//  Created by Stephen Birarda on 1/22/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Audio__
#define __interface__Audio__

#ifdef _WIN32
#define WANT_TIMEVAL
#include <Systime.h>
#endif

#include <fstream>
#include <vector>

#include "InterfaceConfig.h"

#include <QAudio>
#include <QGLWidget>
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtMultimedia/QAudioFormat>
#include <QVector>

#include <AbstractAudioInterface.h>
#include <AudioRingBuffer.h>
#include <StdDev.h>

#include "ui/Oscilloscope.h"


static const int NUM_AUDIO_CHANNELS = 2;

class QAudioInput;
class QAudioOutput;
class QIODevice;

class Audio : public AbstractAudioInterface {
    Q_OBJECT
public:
    // setup for audio I/O
    Audio(Oscilloscope* scope, int16_t initialJitterBufferSamples, QObject* parent = 0);

    float getLastInputLoudness() const { return glm::max(_lastInputLoudness - _noiseGateMeasuredFloor, 0.f); }
    float getAudioAverageInputLoudness() const { return _lastInputLoudness; }

    void setNoiseGateEnabled(bool noiseGateEnabled) { _noiseGateEnabled = noiseGateEnabled; }
    
    void setLastAcceleration(const glm::vec3 lastAcceleration) { _lastAcceleration = lastAcceleration; }
    void setLastVelocity(const glm::vec3 lastVelocity) { _lastVelocity = lastVelocity; }
    
    void setJitterBufferSamples(int samples) { _jitterBufferSamples = samples; }
    int getJitterBufferSamples() { return _jitterBufferSamples; }
    
    void lowPassFilter(int16_t* inputBuffer);
    
    virtual void startCollisionSound(float magnitude, float frequency, float noise, float duration, bool flashScreen);
    virtual void startDrumSound(float volume, float frequency, float duration, float decay);
    
    float getCollisionSoundMagnitude() { return _collisionSoundMagnitude; }
    
    bool getCollisionFlashesScreen() { return _collisionFlashesScreen; }
    
    bool getMuted() { return _muted; }
    
    void init(QGLWidget *parent = 0);
    bool mousePressEvent(int x, int y);
    
    void renderMuteIcon(int x, int y);
    
    int getNetworkSampleRate() { return SAMPLE_RATE; }
    int getNetworkBufferLengthSamplesPerChannel() { return NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL; }

public slots:
    void start();
    void addReceivedAudioToBuffer(const QByteArray& audioByteArray);
    void handleAudioInput();
    void reset();
    void toggleMute();
    void toggleAudioNoiseReduction();
    
    virtual void handleAudioByteArray(const QByteArray& audioByteArray);

    bool switchInputToAudioDevice(const QString& inputDeviceName);
    bool switchOutputToAudioDevice(const QString& outputDeviceName);
    QString getDeviceName(QAudio::Mode mode) const { return (mode == QAudio::AudioInput) ?
                                                            _inputAudioDeviceName : _outputAudioDeviceName; }
    QString getDefaultDeviceName(QAudio::Mode mode);
    QVector<QString> getDeviceNames(QAudio::Mode mode);

signals:
    bool muteToggled();
    
private:

    QByteArray firstInputFrame;
    QAudioInput* _audioInput;
    QAudioFormat _desiredInputFormat;
    QAudioFormat _inputFormat;
    QIODevice* _inputDevice;
    int _numInputCallbackBytes;
    int16_t _localProceduralSamples[NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL];
    QAudioOutput* _audioOutput;
    QAudioFormat _desiredOutputFormat;
    QAudioFormat _outputFormat;
    QIODevice* _outputDevice;
    int _numOutputCallbackBytes;
    QAudioOutput* _loopbackAudioOutput;
    QIODevice* _loopbackOutputDevice;
    QAudioOutput* _proceduralAudioOutput;
    QIODevice* _proceduralOutputDevice;
    AudioRingBuffer _inputRingBuffer;
    AudioRingBuffer _ringBuffer;

    QString _inputAudioDeviceName;
    QString _outputAudioDeviceName;
    
    Oscilloscope* _scope;
    StDev _stdev;
    timeval _lastReceiveTime;
    float _averagedLatency;
    float _measuredJitter;
    int16_t _jitterBufferSamples;
    float _lastInputLoudness;
    float _dcOffset;
    float _noiseGateMeasuredFloor;
    float* _noiseSampleFrames;
    int _noiseGateSampleCounter;
    bool _noiseGateOpen;
    bool _noiseGateEnabled;
    int _noiseGateFramesToClose;
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
    
};


#endif /* defined(__interface__audio__) */