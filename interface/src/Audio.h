//
//  Audio.h
//  interface/src
//
//  Created by Stephen Birarda on 1/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Audio_h
#define hifi_Audio_h

#include <fstream>
#include <vector>

#include "InterfaceConfig.h"
#include "AudioStreamStats.h"
#include "RingBufferHistory.h"
#include "MovingMinMaxAvg.h"

#include <QAudio>
#include <QAudioInput>
#include <QElapsedTimer>
#include <QGLWidget>
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtMultimedia/QAudioFormat>
#include <QVector>
#include <QByteArray>

#include <AbstractAudioInterface.h>
#include <StdDev.h>

#include "MixedAudioStream.h"

static const int NUM_AUDIO_CHANNELS = 2;


class QAudioInput;
class QAudioOutput;
class QIODevice;

class Audio : public AbstractAudioInterface {
    Q_OBJECT
public:
    // setup for audio I/O
    Audio(QObject* parent = 0);

    float getLastInputLoudness() const { return glm::max(_lastInputLoudness - _noiseGateMeasuredFloor, 0.f); }
    float getTimeSinceLastClip() const { return _timeSinceLastClip; }
    float getAudioAverageInputLoudness() const { return _lastInputLoudness; }

    void setNoiseGateEnabled(bool noiseGateEnabled) { _noiseGateEnabled = noiseGateEnabled; }
    
    virtual void startCollisionSound(float magnitude, float frequency, float noise, float duration, bool flashScreen);
    virtual void startDrumSound(float volume, float frequency, float duration, float decay);

    void overrideDesiredJitterBufferFramesTo(int desired) { _receivedAudioStream.overrideDesiredJitterBufferFramesTo(desired); }
    int getDesiredJitterBufferFrames() const { return _receivedAudioStream.getDesiredJitterBufferFrames(); }
    
    float getCollisionSoundMagnitude() { return _collisionSoundMagnitude; }
    
    bool getCollisionFlashesScreen() { return _collisionFlashesScreen; }
    
    bool getMuted() { return _muted; }
    
    void init(QGLWidget *parent = 0);
    bool mousePressEvent(int x, int y);
    
    void renderToolBox(int x, int y, bool boxed);
    void renderScope(int width, int height);
    void renderStats(const float* color, int width, int height);
    
    int getNetworkSampleRate() { return SAMPLE_RATE; }
    int getNetworkBufferLengthSamplesPerChannel() { return NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL; }

    bool getProcessSpatialAudio() const { return _processSpatialAudio; }
    
    float getInputRingBufferMsecsAvailable() const;
    float getInputRingBufferAverageMsecsAvailable() const { return (float)_inputRingBufferMsecsAvailableStats.getWindowAverage(); }

    float getAudioOutputMsecsUnplayed() const;
    float getAudioOutputAverageMsecsUnplayed() const { return (float)_audioOutputMsecsUnplayedStats.getWindowAverage(); }

public slots:
    void start();
    void stop();
    void addReceivedAudioToStream(const QByteArray& audioByteArray);
    void parseAudioStreamStatsPacket(const QByteArray& packet);
    void addSpatialAudioToBuffer(unsigned int sampleTime, const QByteArray& spatialAudio, unsigned int numSamples);
    void handleAudioInput();
    void reset();
    void resetStats();
    void audioMixerKilled();
    void toggleMute();
    void toggleAudioNoiseReduction();
    void toggleToneInjection();
    void toggleScope();
    void toggleScopePause();
    void toggleStats();
    void toggleAudioSpatialProcessing();
    void toggleStereoInput();
    void selectAudioScopeFiveFrames();
    void selectAudioScopeTwentyFrames();
    void selectAudioScopeFiftyFrames();
    
    virtual void handleAudioByteArray(const QByteArray& audioByteArray);

    AudioStreamStats getDownstreamAudioStreamStats() const;
    void sendDownstreamAudioStatsPacket();

    bool switchInputToAudioDevice(const QString& inputDeviceName);
    bool switchOutputToAudioDevice(const QString& outputDeviceName);
    QString getDeviceName(QAudio::Mode mode) const { return (mode == QAudio::AudioInput) ?
                                                            _inputAudioDeviceName : _outputAudioDeviceName; }
    QString getDefaultDeviceName(QAudio::Mode mode);
    QVector<QString> getDeviceNames(QAudio::Mode mode);

    float getInputVolume() const { return (_audioInput) ? _audioInput->volume() : 0.0f; }
    void setInputVolume(float volume) { if (_audioInput) _audioInput->setVolume(volume); }

    const AudioStreamStats& getAudioMixerAvatarStreamAudioStats() const { return _audioMixerAvatarStreamAudioStats; }
    const QHash<QUuid, AudioStreamStats>& getAudioMixerInjectedStreamAudioStatsMap() const { return _audioMixerInjectedStreamAudioStatsMap; }

signals:
    bool muteToggled();
    void preProcessOriginalInboundAudio(unsigned int sampleTime, QByteArray& samples, const QAudioFormat& format);
    void processInboundAudio(unsigned int sampleTime, const QByteArray& samples, const QAudioFormat& format);
    void processLocalAudio(unsigned int sampleTime, const QByteArray& samples, const QAudioFormat& format);
    
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
    MixedAudioStream _receivedAudioStream;
    bool _isStereoInput;

    QString _inputAudioDeviceName;
    QString _outputAudioDeviceName;
    
    StDev _stdev;
    QElapsedTimer _timeSinceLastReceived;
    float _averagedLatency;
    float _lastInputLoudness;
    float _timeSinceLastClip;
    float _dcOffset;
    float _noiseGateMeasuredFloor;
    float* _noiseSampleFrames;
    int _noiseGateSampleCounter;
    bool _noiseGateOpen;
    bool _noiseGateEnabled;
    bool _toneInjectionEnabled;
    int _noiseGateFramesToClose;
    int _totalInputAudioSamples;
    
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
    bool _muted;
    bool _localEcho;
    GLuint _micTextureId;
    GLuint _muteTextureId;
    GLuint _boxTextureId;
    QRect _iconBounds;
    
    /// Audio callback in class context.
    inline void performIO(int16_t* inputLeft, int16_t* outputLeft, int16_t* outputRight);
    
    
    bool _processSpatialAudio; /// Process received audio by spatial audio hooks
    unsigned int _spatialAudioStart; /// Start of spatial audio interval (in sample rate time base)
    unsigned int _spatialAudioFinish; /// End of spatial audio interval (in sample rate time base)
    AudioRingBuffer _spatialAudioRingBuffer; /// Spatially processed audio

    // Process procedural audio by
    //  1. Echo to the local procedural output device
    //  2. Mix with the audio input
    void processProceduralAudio(int16_t* monoInput, int numSamples);

    // Add sounds that we want the user to not hear themselves, by adding on top of mic input signal
    void addProceduralSounds(int16_t* monoInput, int numSamples);
    
    // Process received audio
    void processReceivedAudio(const QByteArray& audioByteArray);

    // Pushes frames from the output ringbuffer to the audio output device
    void pushAudioToOutput();

    bool switchInputToAudioDevice(const QAudioDeviceInfo& inputDeviceInfo);
    bool switchOutputToAudioDevice(const QAudioDeviceInfo& outputDeviceInfo);

    // Callback acceleration dependent calculations
    static const float CALLBACK_ACCELERATOR_RATIO;
    int calculateNumberOfInputCallbackBytes(const QAudioFormat& format) const;
    int calculateNumberOfFrameSamples(int numBytes) const;
    float calculateDeviceToNetworkInputRatio(int numBytes) const;

    // Audio scope methods for allocation/deallocation
    void allocateScope();
    void freeScope();
    void reallocateScope(int frames);

    // Audio scope methods for data acquisition
    void addBufferToScope(
        QByteArray* byteArray, unsigned int frameOffset, const int16_t* source, unsigned int sourceChannel, unsigned int sourceNumberOfChannels);

    // Audio scope methods for rendering
    void renderBackground(const float* color, int x, int y, int width, int height);
    void renderGrid(const float* color, int x, int y, int width, int height, int rows, int cols);
    void renderLineStrip(const float* color, int x, int y, int n, int offset, const QByteArray* byteArray);

    // audio stats methods for rendering
    void renderAudioStreamStats(const AudioStreamStats& streamStats, int horizontalOffset, int& verticalOffset,
        float scale, float rotation, int font, const float* color, bool isDownstreamStats = false);

    // Audio scope data
    static const unsigned int NETWORK_SAMPLES_PER_FRAME = NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL;
    static const unsigned int DEFAULT_FRAMES_PER_SCOPE = 5;
    static const unsigned int SCOPE_WIDTH = NETWORK_SAMPLES_PER_FRAME * DEFAULT_FRAMES_PER_SCOPE;
    static const unsigned int MULTIPLIER_SCOPE_HEIGHT = 20;
    static const unsigned int SCOPE_HEIGHT = 2 * 15 * MULTIPLIER_SCOPE_HEIGHT;
    bool _scopeEnabled;
    bool _scopeEnabledPause;
    int _scopeInputOffset;
    int _scopeOutputOffset;
    int _framesPerScope;
    int _samplesPerScope;
    QMutex _guard;
    QByteArray* _scopeInput;
    QByteArray* _scopeOutputLeft;
    QByteArray* _scopeOutputRight;
#ifdef _WIN32
    static const unsigned int STATS_WIDTH = 1500;
#else
    static const unsigned int STATS_WIDTH = 650;
#endif
    static const unsigned int STATS_HEIGHT_PER_LINE = 20;
    bool _statsEnabled;

    AudioStreamStats _audioMixerAvatarStreamAudioStats;
    QHash<QUuid, AudioStreamStats> _audioMixerInjectedStreamAudioStatsMap;

    quint16 _outgoingAvatarAudioSequenceNumber;
    
    MovingMinMaxAvg<float> _audioInputMsecsReadStats;
    MovingMinMaxAvg<float> _inputRingBufferMsecsAvailableStats;

    MovingMinMaxAvg<float> _audioOutputMsecsUnplayedStats;
};


#endif // hifi_Audio_h
