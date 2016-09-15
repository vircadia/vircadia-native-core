//
//  AudioClient.h
//  libraries/audio-client/src
//
//  Created by Stephen Birarda on 1/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioClient_h
#define hifi_AudioClient_h

#include <fstream>
#include <memory>
#include <vector>
#include <mutex>
#include <queue>

#include <QtCore/qsystemdetection.h>
#include <QtCore/QByteArray>
#include <QtCore/QElapsedTimer>
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtMultimedia/QAudio>
#include <QtMultimedia/QAudioFormat>
#include <QtMultimedia/QAudioInput>

#include <AbstractAudioInterface.h>
#include <AudioEffectOptions.h>
#include <AudioStreamStats.h>

#include <DependencyManager.h>
#include <HifiSockAddr.h>
#include <NLPacket.h>
#include <MixedProcessedAudioStream.h>
#include <RingBufferHistory.h>
#include <SettingHandle.h>
#include <Sound.h>
#include <StDev.h>
#include <AudioHRTF.h>
#include <AudioSRC.h>
#include <AudioInjector.h>
#include <AudioReverb.h>
#include <AudioLimiter.h>
#include <AudioConstants.h>

#include <plugins/CodecPlugin.h>

#include "AudioIOStats.h"
#include "AudioNoiseGate.h"

#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable : 4273 )
#pragma warning( disable : 4305 )
#endif

#ifdef _WIN32
#pragma warning( pop )
#endif

static const int NUM_AUDIO_CHANNELS = 2;

static const int DEFAULT_AUDIO_OUTPUT_BUFFER_SIZE_FRAMES = 3;
static const int MIN_AUDIO_OUTPUT_BUFFER_SIZE_FRAMES = 1;
static const int MAX_AUDIO_OUTPUT_BUFFER_SIZE_FRAMES = 20;
static const int DEFAULT_AUDIO_OUTPUT_STARVE_DETECTION_ENABLED = true;
static const int DEFAULT_AUDIO_OUTPUT_STARVE_DETECTION_THRESHOLD = 3;
static const quint64 DEFAULT_AUDIO_OUTPUT_STARVE_DETECTION_PERIOD = 10 * 1000; // 10 Seconds

class QAudioInput;
class QAudioOutput;
class QIODevice;


class Transform;
class NLPacket;

class AudioClient : public AbstractAudioInterface, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
public:
    using AudioPositionGetter = std::function<glm::vec3()>;
    using AudioOrientationGetter = std::function<glm::quat()>;

    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;

    class AudioOutputIODevice : public QIODevice {
    public:
        AudioOutputIODevice(MixedProcessedAudioStream& receivedAudioStream, AudioClient* audio) :
            _receivedAudioStream(receivedAudioStream), _audio(audio), _unfulfilledReads(0) {};

        void start() { open(QIODevice::ReadOnly); }
        void stop() { close(); }
        qint64 readData(char * data, qint64 maxSize) override;
        qint64 writeData(const char * data, qint64 maxSize) override { return 0; }
        int getRecentUnfulfilledReads() { int unfulfilledReads = _unfulfilledReads; _unfulfilledReads = 0; return unfulfilledReads; }
    private:
        MixedProcessedAudioStream& _receivedAudioStream;
        AudioClient* _audio;
        int _unfulfilledReads;
    };

    void negotiateAudioFormat();
    void selectAudioFormat(const QString& selectedCodecName);

    const MixedProcessedAudioStream& getReceivedAudioStream() const { return _receivedAudioStream; }
    MixedProcessedAudioStream& getReceivedAudioStream() { return _receivedAudioStream; }

    float getLastInputLoudness() const { return glm::max(_lastInputLoudness - _inputGate.getMeasuredFloor(), 0.0f); }

    float getTimeSinceLastClip() const { return _timeSinceLastClip; }
    float getAudioAverageInputLoudness() const { return _lastInputLoudness; }

    int getDesiredJitterBufferFrames() const { return _receivedAudioStream.getDesiredJitterBufferFrames(); }

    bool isMuted() { return _muted; }

    const AudioIOStats& getStats() const { return _stats; }

    float getInputRingBufferMsecsAvailable() const;
    float getAudioOutputMsecsUnplayed() const;

    int getOutputBufferSize() { return _outputBufferSizeFrames.get(); }

    bool getOutputStarveDetectionEnabled() { return _outputStarveDetectionEnabled.get(); }
    void setOutputStarveDetectionEnabled(bool enabled) { _outputStarveDetectionEnabled.set(enabled); }

    int getOutputStarveDetectionPeriod() { return _outputStarveDetectionPeriodMsec.get(); }
    void setOutputStarveDetectionPeriod(int msecs) { _outputStarveDetectionPeriodMsec.set(msecs); }

    int getOutputStarveDetectionThreshold() { return _outputStarveDetectionThreshold.get(); }
    void setOutputStarveDetectionThreshold(int threshold) { _outputStarveDetectionThreshold.set(threshold); }

    bool isSimulatingJitter() { return _gate.isSimulatingJitter(); }
    void setIsSimulatingJitter(bool enable) { _gate.setIsSimulatingJitter(enable); }

    int getGateThreshold() { return _gate.getThreshold(); }
    void setGateThreshold(int threshold) { _gate.setThreshold(threshold); }

    void setPositionGetter(AudioPositionGetter positionGetter) { _positionGetter = positionGetter; }
    void setOrientationGetter(AudioOrientationGetter orientationGetter) { _orientationGetter = orientationGetter; }

    QVector<AudioInjector*>& getActiveLocalAudioInjectors() { return _activeLocalAudioInjectors; }

    void checkDevices();

    static const float CALLBACK_ACCELERATOR_RATIO;

#ifdef Q_OS_WIN
    static QString friendlyNameForAudioDevice(wchar_t* guid);
#endif

public slots:
    void start();
    void stop();

    void handleAudioEnvironmentDataPacket(QSharedPointer<ReceivedMessage> message);
    void handleAudioDataPacket(QSharedPointer<ReceivedMessage> message);
    void handleNoisyMutePacket(QSharedPointer<ReceivedMessage> message);
    void handleMuteEnvironmentPacket(QSharedPointer<ReceivedMessage> message);
    void handleSelectedAudioFormat(QSharedPointer<ReceivedMessage> message);
    void handleMismatchAudioFormat(SharedNodePointer node, const QString& currentCodec, const QString& recievedCodec);

    void sendDownstreamAudioStatsPacket() { _stats.sendDownstreamAudioStatsPacket(); }
    void handleAudioInput();
    void handleRecordedAudioInput(const QByteArray& audio);
    void reset();
    void audioMixerKilled();
    void toggleMute();

    virtual void setIsStereoInput(bool stereo) override;

    void toggleAudioNoiseReduction() { _isNoiseGateEnabled = !_isNoiseGateEnabled; }

    void toggleLocalEcho() { _shouldEchoLocally = !_shouldEchoLocally; }
    void toggleServerEcho() { _shouldEchoToServer = !_shouldEchoToServer; }

    void processReceivedSamples(const QByteArray& inputBuffer, QByteArray& outputBuffer);
    void sendMuteEnvironmentPacket();

    int setOutputBufferSize(int numFrames, bool persist = true);

    virtual bool outputLocalInjector(bool isStereo, AudioInjector* injector) override;

    bool switchInputToAudioDevice(const QString& inputDeviceName);
    bool switchOutputToAudioDevice(const QString& outputDeviceName);
    QString getDeviceName(QAudio::Mode mode) const { return (mode == QAudio::AudioInput) ?
                                                            _inputAudioDeviceName : _outputAudioDeviceName; }
    QString getDefaultDeviceName(QAudio::Mode mode);
    QVector<QString> getDeviceNames(QAudio::Mode mode);

    float getInputVolume() const { return (_audioInput) ? (float)_audioInput->volume() : 0.0f; }
    void setInputVolume(float volume) { if (_audioInput) _audioInput->setVolume(volume); }
    void setReverb(bool reverb);
    void setReverbOptions(const AudioEffectOptions* options);

    void outputNotify();

    void loadSettings();
    void saveSettings();

signals:
    bool muteToggled();
    void mutedByMixer();
    void inputReceived(const QByteArray& inputSamples);
    void outputBytesToNetwork(int numBytes);
    void inputBytesFromNetwork(int numBytes);

    void changeDevice(const QAudioDeviceInfo& outputDeviceInfo);
    void deviceChanged();

    void receivedFirstPacket();
    void disconnected();

    void audioFinished();

    void muteEnvironmentRequested(glm::vec3 position, float radius);

protected:
    AudioClient();
    ~AudioClient();

    virtual void customDeleter() override {
        deleteLater();
    }

private:
    void outputFormatChanged();
    void mixLocalAudioInjectors(float* mixBuffer);
    float azimuthForSource(const glm::vec3& relativePosition);
    float gainForSource(float distance, float volume);

    class Gate {
    public:
        Gate(AudioClient* audioClient);

        bool isSimulatingJitter() { return _isSimulatingJitter; }
        void setIsSimulatingJitter(bool enable);

        int getThreshold() { return _threshold; }
        void setThreshold(int threshold);

        void insert(QSharedPointer<ReceivedMessage> message);

    private:
        void flush();

        AudioClient* _audioClient;
        std::queue<QSharedPointer<ReceivedMessage>> _queue;
        std::mutex _mutex;

        int _index{ 0 };
        int _threshold{ 1 };
        bool _isSimulatingJitter{ false };
    };

    Gate _gate;

    Mutex _injectorsMutex;
    QAudioInput* _audioInput;
    QAudioFormat _desiredInputFormat;
    QAudioFormat _inputFormat;
    QIODevice* _inputDevice;
    int _numInputCallbackBytes;
    QAudioOutput* _audioOutput;
    QAudioFormat _desiredOutputFormat;
    QAudioFormat _outputFormat;
    int _outputFrameSize;
    int _numOutputCallbackBytes;
    QAudioOutput* _loopbackAudioOutput;
    QIODevice* _loopbackOutputDevice;
    AudioRingBuffer _inputRingBuffer;
    MixedProcessedAudioStream _receivedAudioStream;
    bool _isStereoInput;

    QString _inputAudioDeviceName;
    QString _outputAudioDeviceName;

    quint64 _outputStarveDetectionStartTimeMsec;
    int _outputStarveDetectionCount;

    Setting::Handle<int> _outputBufferSizeFrames;
    int _sessionOutputBufferSizeFrames;
    Setting::Handle<bool> _outputStarveDetectionEnabled;
    Setting::Handle<int> _outputStarveDetectionPeriodMsec;
     // Maximum number of starves per _outputStarveDetectionPeriod before increasing buffer size
    Setting::Handle<int> _outputStarveDetectionThreshold;

    StDev _stdev;
    QElapsedTimer _timeSinceLastReceived;
    float _averagedLatency;
    float _lastInputLoudness;
    float _timeSinceLastClip;
    int _totalInputAudioSamples;

    bool _muted;
    bool _shouldEchoLocally;
    bool _shouldEchoToServer;
    bool _isNoiseGateEnabled;

    bool _reverb;
    AudioEffectOptions _scriptReverbOptions;
    AudioEffectOptions _zoneReverbOptions;
    AudioEffectOptions* _reverbOptions;
    AudioReverb _sourceReverb { AudioConstants::SAMPLE_RATE };
    AudioReverb _listenerReverb { AudioConstants::SAMPLE_RATE };

    // possible streams needed for resample
    AudioSRC* _inputToNetworkResampler;
    AudioSRC* _networkToOutputResampler;

    // for local hrtf-ing
    float _mixBuffer[AudioConstants::NETWORK_FRAME_SAMPLES_STEREO];
    int16_t _scratchBuffer[AudioConstants::NETWORK_FRAME_SAMPLES_STEREO];
    AudioLimiter _audioLimiter;

    // Adds Reverb
    void configureReverb();
    void updateReverbOptions();

    void handleLocalEchoAndReverb(QByteArray& inputByteArray);

    bool switchInputToAudioDevice(const QAudioDeviceInfo& inputDeviceInfo);
    bool switchOutputToAudioDevice(const QAudioDeviceInfo& outputDeviceInfo);

    // Callback acceleration dependent calculations
    int calculateNumberOfInputCallbackBytes(const QAudioFormat& format) const;
    int calculateNumberOfFrameSamples(int numBytes) const;

    quint16 _outgoingAvatarAudioSequenceNumber;

    AudioOutputIODevice _audioOutputIODevice;

    AudioIOStats _stats;

    AudioNoiseGate _inputGate;

    AudioPositionGetter _positionGetter;
    AudioOrientationGetter _orientationGetter;

    QVector<QString> _inputDevices;
    QVector<QString> _outputDevices;

    bool _hasReceivedFirstPacket = false;

    QVector<AudioInjector*> _activeLocalAudioInjectors;

    CodecPluginPointer _codec;
    QString _selectedCodecName;
    Encoder* _encoder { nullptr }; // for outbound mic stream
};


#endif // hifi_AudioClient_h
