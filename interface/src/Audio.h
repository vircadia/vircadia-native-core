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
#include "Recorder.h"
#include "RingBufferHistory.h"
#include "MovingMinMaxAvg.h"
#include "AudioRingBuffer.h"
#include "AudioFormat.h"
#include "AudioBuffer.h"
#include "AudioSourceTone.h"
#include "AudioSourceNoise.h"
#include "AudioGain.h"
#include "AudioFilter.h"
#include "AudioFilterBank.h"

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
#include <StDev.h>

#include "MixedProcessedAudioStream.h"
#include "AudioEffectOptions.h"
#include <AudioRingBuffer.h>
#include <StDev.h>

extern "C" {
    #include <gverb.h>
    #include <gverbdsp.h>
}

static const int NUM_AUDIO_CHANNELS = 2;

static const int MAX_16_BIT_AUDIO_SAMPLE = 32767;


class QAudioInput;
class QAudioOutput;
class QIODevice;

class Audio : public AbstractAudioInterface {
    Q_OBJECT
public:

    class AudioOutputIODevice : public QIODevice {
    public:
        AudioOutputIODevice(MixedProcessedAudioStream& receivedAudioStream) : _receivedAudioStream(receivedAudioStream) {};

        void start() { open(QIODevice::ReadOnly); }
        void stop() { close(); }
        qint64	readData(char * data, qint64 maxSize);
        qint64	writeData(const char * data, qint64 maxSize) { return 0; }
    private:
        MixedProcessedAudioStream& _receivedAudioStream;
    };


    // setup for audio I/O
    Audio(QObject* parent = 0);

    float getLastInputLoudness() const { return glm::max(_lastInputLoudness - _noiseGateMeasuredFloor, 0.f); }
    float getTimeSinceLastClip() const { return _timeSinceLastClip; }
    float getAudioAverageInputLoudness() const { return _lastInputLoudness; }

    void setNoiseGateEnabled(bool noiseGateEnabled) { _noiseGateEnabled = noiseGateEnabled; }
    
    virtual void startCollisionSound(float magnitude, float frequency, float noise, float duration, bool flashScreen);
    virtual void startDrumSound(float volume, float frequency, float duration, float decay);

    void setReceivedAudioStreamSettings(const InboundAudioStream::Settings& settings) { _receivedAudioStream.setSettings(settings); }

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
    
    void setRecorder(RecorderPointer recorder) { _recorder = recorder; }

public slots:
    void start();
    void stop();
    void addReceivedAudioToStream(const QByteArray& audioByteArray);
    void parseAudioStreamStatsPacket(const QByteArray& packet);
    void parseAudioEnvironmentData(const QByteArray& packet);
    void addSpatialAudioToBuffer(unsigned int sampleTime, const QByteArray& spatialAudio, unsigned int numSamples);
    void handleAudioInput();
    void reset();
    void resetStats();
    void audioMixerKilled();
    void toggleMute();
    void toggleAudioNoiseReduction();
    void toggleAudioSourceInject();
    void selectAudioSourcePinkNoise();
    void selectAudioSourceSine440();
    void toggleScope();
    void toggleScopePause();
    void toggleStats();
    void toggleStatsShowInjectedStreams();
    void toggleAudioSpatialProcessing();
    void toggleStereoInput();
    void selectAudioScopeFiveFrames();
    void selectAudioScopeTwentyFrames();
    void selectAudioScopeFiftyFrames();
    void addStereoSilenceToScope(int silentSamplesPerChannel);
    void addLastFrameRepeatedWithFadeToScope(int samplesPerChannel);
    void addStereoSamplesToScope(const QByteArray& samples);
    void processReceivedSamples(const QByteArray& inputBuffer, QByteArray& outputBuffer);
    void toggleAudioFilter();
    void selectAudioFilterFlat();
    void selectAudioFilterTrebleCut();
    void selectAudioFilterBassCut();
    void selectAudioFilterSmiley();

    virtual bool outputLocalInjector(bool isStereo, qreal volume, AudioInjector* injector);

    void sendDownstreamAudioStatsPacket();

    bool switchInputToAudioDevice(const QString& inputDeviceName);
    bool switchOutputToAudioDevice(const QString& outputDeviceName);
    QString getDeviceName(QAudio::Mode mode) const { return (mode == QAudio::AudioInput) ?
                                                            _inputAudioDeviceName : _outputAudioDeviceName; }
    QString getDefaultDeviceName(QAudio::Mode mode);
    QVector<QString> getDeviceNames(QAudio::Mode mode);

    float getInputVolume() const { return (_audioInput) ? _audioInput->volume() : 0.0f; }
    void setInputVolume(float volume) { if (_audioInput) _audioInput->setVolume(volume); }
    void setReverb(bool reverb) { _reverb = reverb; }
    void setReverbOptions(const AudioEffectOptions* options);

    const AudioStreamStats& getAudioMixerAvatarStreamAudioStats() const { return _audioMixerAvatarStreamAudioStats; }
    const QHash<QUuid, AudioStreamStats>& getAudioMixerInjectedStreamAudioStatsMap() const { return _audioMixerInjectedStreamAudioStatsMap; }

signals:
    bool muteToggled();
    void preProcessOriginalInboundAudio(unsigned int sampleTime, QByteArray& samples, const QAudioFormat& format);
    void processInboundAudio(unsigned int sampleTime, const QByteArray& samples, const QAudioFormat& format);
    void processLocalAudio(unsigned int sampleTime, const QByteArray& samples, const QAudioFormat& format);

private slots:
    void cleanupLocalOutputInterface();
private:
    void outputFormatChanged();

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
    int _outputFrameSize;
    int16_t _outputProcessingBuffer[NETWORK_BUFFER_LENGTH_SAMPLES_STEREO];
    int _numOutputCallbackBytes;
    QAudioOutput* _loopbackAudioOutput;
    QIODevice* _loopbackOutputDevice;
    QAudioOutput* _proceduralAudioOutput;
    QIODevice* _proceduralOutputDevice;
    AudioRingBuffer _inputRingBuffer;
    MixedProcessedAudioStream _receivedAudioStream;
    bool _isStereoInput;

    QString _inputAudioDeviceName;
    QString _outputAudioDeviceName;
    
    StDev _stdev;
    QElapsedTimer _timeSinceLastReceived;
    float _averagedLatency;
    float _lastInputLoudness;
    int _inputFrameCounter;
    float _quietestFrame;
    float _loudestFrame;
    float _timeSinceLastClip;
    float _dcOffset;
    float _noiseGateMeasuredFloor;
    float* _noiseSampleFrames;
    int _noiseGateSampleCounter;
    bool _noiseGateOpen;
    bool _noiseGateEnabled;
    bool _audioSourceInjectEnabled;
    
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
    bool _reverb;
    AudioEffectOptions _scriptReverbOptions;
    AudioEffectOptions _zoneReverbOptions;
    AudioEffectOptions* _reverbOptions;
    ty_gverb* _gverbLocal;
    ty_gverb* _gverb;
    GLuint _micTextureId;
    GLuint _muteTextureId;
    GLuint _boxTextureId;
    QRect _iconBounds;
    float _iconColor;
    qint64 _iconPulseTimeReference;
    
    bool _processSpatialAudio; /// Process received audio by spatial audio hooks
    unsigned int _spatialAudioStart; /// Start of spatial audio interval (in sample rate time base)
    unsigned int _spatialAudioFinish; /// End of spatial audio interval (in sample rate time base)
    AudioRingBuffer _spatialAudioRingBuffer; /// Spatially processed audio

    // Process procedural audio by
    //  1. Echo to the local procedural output device
    //  2. Mix with the audio input
    void processProceduralAudio(int16_t* monoInput, int numSamples);

    // Adds Reverb
    void initGverb();
    void updateGverbOptions();
    void addReverb(ty_gverb* gverb, int16_t* samples, int numSamples, QAudioFormat& format, bool noEcho = false);

    void handleLocalEchoAndReverb(QByteArray& inputByteArray);
    
    // Add sounds that we want the user to not hear themselves, by adding on top of mic input signal
    void addProceduralSounds(int16_t* monoInput, int numSamples);

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
    int addBufferToScope(QByteArray* byteArray, int frameOffset, const int16_t* source, int sourceSamples,
        unsigned int sourceChannel, unsigned int sourceNumberOfChannels, float fade = 1.0f);
    int addSilenceToScope(QByteArray* byteArray, int frameOffset, int silentSamples);

    // Audio scope methods for rendering
    void renderBackground(const float* color, int x, int y, int width, int height);
    void renderGrid(const float* color, int x, int y, int width, int height, int rows, int cols);
    void renderLineStrip(const float* color, int x, int  y, int n, int offset, const QByteArray* byteArray);

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

    // Input framebuffer
    AudioBufferFloat32 _inputFrameBuffer;
    
    // Input gain
    AudioGain _inputGain;
    
    // Post tone/pink noise generator gain
    AudioGain _sourceGain;

    // Pink noise source
    bool _noiseSourceEnabled;
    AudioSourcePinkNoise _noiseSource;
    
    // Tone source
    bool _toneSourceEnabled;
    AudioSourceTone _toneSource;
    
    // Multi-band parametric EQ
    bool _peqEnabled;
    AudioFilterPEQ3m _peq;
    
    QMutex _guard;
    QByteArray* _scopeInput;
    QByteArray* _scopeOutputLeft;
    QByteArray* _scopeOutputRight;
    QByteArray _scopeLastFrame;
#ifdef _WIN32
    static const unsigned int STATS_WIDTH = 1500;
#else
    static const unsigned int STATS_WIDTH = 650;
#endif
    static const unsigned int STATS_HEIGHT_PER_LINE = 20;
    bool _statsEnabled;
    bool _statsShowInjectedStreams;

    AudioStreamStats _audioMixerAvatarStreamAudioStats;
    QHash<QUuid, AudioStreamStats> _audioMixerInjectedStreamAudioStatsMap;

    quint16 _outgoingAvatarAudioSequenceNumber;
    
    MovingMinMaxAvg<float> _audioInputMsecsReadStats;
    MovingMinMaxAvg<float> _inputRingBufferMsecsAvailableStats;

    MovingMinMaxAvg<float> _audioOutputMsecsUnplayedStats;

    quint64 _lastSentAudioPacket;
    MovingMinMaxAvg<quint64> _packetSentTimeGaps;

    AudioOutputIODevice _audioOutputIODevice;
    
    WeakRecorderPointer _recorder;
    
    QHash<QObject*, QAudioOutput*> _injectedOutputInterfaces;
};


#endif // hifi_Audio_h
