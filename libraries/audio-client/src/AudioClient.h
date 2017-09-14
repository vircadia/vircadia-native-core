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
#include <AudioGate.h>

#include <shared/RateCounter.h>

#include <plugins/CodecPlugin.h>

#include "AudioIOStats.h"

#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable : 4273 )
#pragma warning( disable : 4305 )
#endif

#ifdef _WIN32
#pragma warning( pop )
#endif

class QAudioInput;
class QAudioOutput;
class QIODevice;


class Transform;
class NLPacket;

class AudioClient : public AbstractAudioInterface, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

    using LocalInjectorsStream = AudioMixRingBuffer;
public:
    static const int MIN_BUFFER_FRAMES;
    static const int MAX_BUFFER_FRAMES;

    using AudioPositionGetter = std::function<glm::vec3()>;
    using AudioOrientationGetter = std::function<glm::quat()>;

    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;

    class AudioOutputIODevice : public QIODevice {
    public:
        AudioOutputIODevice(LocalInjectorsStream& localInjectorsStream, MixedProcessedAudioStream& receivedAudioStream,
                AudioClient* audio) :
            _localInjectorsStream(localInjectorsStream), _receivedAudioStream(receivedAudioStream),
            _audio(audio), _unfulfilledReads(0) {}

        void start() { open(QIODevice::ReadOnly | QIODevice::Unbuffered); }
        qint64 readData(char * data, qint64 maxSize) override;
        qint64 writeData(const char * data, qint64 maxSize) override { return 0; }
        int getRecentUnfulfilledReads() { int unfulfilledReads = _unfulfilledReads; _unfulfilledReads = 0; return unfulfilledReads; }
    private:
        LocalInjectorsStream& _localInjectorsStream;
        MixedProcessedAudioStream& _receivedAudioStream;
        AudioClient* _audio;
        int _unfulfilledReads;
    };

    void startThread();
    void negotiateAudioFormat();
    void selectAudioFormat(const QString& selectedCodecName);

    Q_INVOKABLE QString getSelectedAudioFormat() const { return _selectedCodecName; }
    Q_INVOKABLE bool getNoiseGateOpen() const { return _audioGateOpen; }
    Q_INVOKABLE float getSilentInboundPPS() const { return _silentInbound.rate(); }
    Q_INVOKABLE float getAudioInboundPPS() const { return _audioInbound.rate(); }
    Q_INVOKABLE float getSilentOutboundPPS() const { return _silentOutbound.rate(); }
    Q_INVOKABLE float getAudioOutboundPPS() const { return _audioOutbound.rate(); }

    const MixedProcessedAudioStream& getReceivedAudioStream() const { return _receivedAudioStream; }
    MixedProcessedAudioStream& getReceivedAudioStream() { return _receivedAudioStream; }

    float getLastInputLoudness() const { return _lastInputLoudness; }   // TODO: relative to noise floor?

    float getTimeSinceLastClip() const { return _timeSinceLastClip; }
    float getAudioAverageInputLoudness() const { return _lastInputLoudness; }

    const AudioIOStats& getStats() const { return _stats; }

    int getOutputBufferSize() { return _outputBufferSizeFrames.get(); }

    bool getOutputStarveDetectionEnabled() { return _outputStarveDetectionEnabled.get(); }
    void setOutputStarveDetectionEnabled(bool enabled) { _outputStarveDetectionEnabled.set(enabled); }

    bool isSimulatingJitter() { return _gate.isSimulatingJitter(); }
    void setIsSimulatingJitter(bool enable) { _gate.setIsSimulatingJitter(enable); }

    int getGateThreshold() { return _gate.getThreshold(); }
    void setGateThreshold(int threshold) { _gate.setThreshold(threshold); }

    void setPositionGetter(AudioPositionGetter positionGetter) { _positionGetter = positionGetter; }
    void setOrientationGetter(AudioOrientationGetter orientationGetter) { _orientationGetter = orientationGetter; }

    void setIsPlayingBackRecording(bool isPlayingBackRecording) { _isPlayingBackRecording = isPlayingBackRecording; }

    Q_INVOKABLE void setAvatarBoundingBoxParameters(glm::vec3 corner, glm::vec3 scale);

    bool outputLocalInjector(const AudioInjectorPointer& injector) override;

    QAudioDeviceInfo getActiveAudioDevice(QAudio::Mode mode) const;
    QList<QAudioDeviceInfo> getAudioDevices(QAudio::Mode mode) const;

    void enablePeakValues(bool enable) { _enablePeakValues = enable; }
    bool peakValuesAvailable() const;

    static const float CALLBACK_ACCELERATOR_RATIO;

    bool getNamedAudioDeviceForModeExists(QAudio::Mode mode, const QString& deviceName);

#ifdef Q_OS_WIN
    static QString getWinDeviceName(wchar_t* guid);
#endif

public slots:
    void start();
    void stop();
    void cleanupBeforeQuit();

    void handleAudioEnvironmentDataPacket(QSharedPointer<ReceivedMessage> message);
    void handleAudioDataPacket(QSharedPointer<ReceivedMessage> message);
    void handleNoisyMutePacket(QSharedPointer<ReceivedMessage> message);
    void handleMuteEnvironmentPacket(QSharedPointer<ReceivedMessage> message);
    void handleSelectedAudioFormat(QSharedPointer<ReceivedMessage> message);
    void handleMismatchAudioFormat(SharedNodePointer node, const QString& currentCodec, const QString& recievedCodec);

    void sendDownstreamAudioStatsPacket() { _stats.publish(); }
    void handleMicAudioInput();
    void handleDummyAudioInput();
    void handleRecordedAudioInput(const QByteArray& audio);
    void reset();
    void audioMixerKilled();

    void toggleMute();
    bool isMuted() { return _muted; }


    virtual void setIsStereoInput(bool stereo) override;

    void setNoiseReduction(bool isNoiseGateEnabled);
    bool isNoiseReductionEnabled() const { return _isNoiseGateEnabled; }

    void toggleLocalEcho() { _shouldEchoLocally = !_shouldEchoLocally; }
    void toggleServerEcho() { _shouldEchoToServer = !_shouldEchoToServer; }

    void processReceivedSamples(const QByteArray& inputBuffer, QByteArray& outputBuffer);
    void sendMuteEnvironmentPacket();

    int setOutputBufferSize(int numFrames, bool persist = true);

    bool shouldLoopbackInjectors() override { return _shouldEchoToServer; }

    // calling with a null QAudioDevice will use the system default
    bool switchAudioDevice(QAudio::Mode mode, const QAudioDeviceInfo& deviceInfo = QAudioDeviceInfo());
    bool switchAudioDevice(QAudio::Mode mode, const QString& deviceName);

    float getInputVolume() const { return (_audioInput) ? (float)_audioInput->volume() : 0.0f; }
    void setInputVolume(float volume);
    void setReverb(bool reverb);
    void setReverbOptions(const AudioEffectOptions* options);

    void outputNotify();

    void loadSettings();
    void saveSettings();

signals:
    void inputVolumeChanged(float volume);
    void muteToggled();
    void noiseReductionChanged();
    void mutedByMixer();
    void inputReceived(const QByteArray& inputSamples);
    void inputLoudnessChanged(float loudness);
    void outputBytesToNetwork(int numBytes);
    void inputBytesFromNetwork(int numBytes);
    void noiseGateOpened();
    void noiseGateClosed();

    void changeDevice(const QAudioDeviceInfo& outputDeviceInfo);

    void deviceChanged(QAudio::Mode mode, const QAudioDeviceInfo& device);
    void devicesChanged(QAudio::Mode mode, const QList<QAudioDeviceInfo>& devices);
    void peakValueListChanged(const QList<float> peakValueList);

    void receivedFirstPacket();
    void disconnected();

    void audioFinished();

    void muteEnvironmentRequested(glm::vec3 position, float radius);

protected:
    AudioClient();
    ~AudioClient();

    virtual void customDeleter() override;

private:
    friend class CheckDevicesThread;
    friend class LocalInjectorsThread;

    // background tasks
    void checkDevices();
    void checkPeakValues();

    void outputFormatChanged();
    void handleAudioInput(QByteArray& audioBuffer);
    void prepareLocalAudioInjectors(std::unique_ptr<Lock> localAudioLock = nullptr);
    bool mixLocalAudioInjectors(float* mixBuffer);
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
    QTimer* _dummyAudioInput;
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
    LocalInjectorsStream _localInjectorsStream;
    // In order to use _localInjectorsStream as a lock-free pipe,
    // use it with a single producer/consumer, and track available samples and injectors
    std::atomic<int> _localSamplesAvailable { 0 };
    std::atomic<bool> _localInjectorsAvailable { false };
    MixedProcessedAudioStream _receivedAudioStream;
    bool _isStereoInput;
    std::atomic<bool> _enablePeakValues { false };

    quint64 _outputStarveDetectionStartTimeMsec;
    int _outputStarveDetectionCount;

    Setting::Handle<int> _outputBufferSizeFrames;
    int _sessionOutputBufferSizeFrames;
    Setting::Handle<bool> _outputStarveDetectionEnabled;

    StDev _stdev;
    QElapsedTimer _timeSinceLastReceived;
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
    AudioReverb _localReverb { AudioConstants::SAMPLE_RATE };

    // possible streams needed for resample
    AudioSRC* _inputToNetworkResampler;
    AudioSRC* _networkToOutputResampler;
    AudioSRC* _localToOutputResampler;

    // for network audio (used by network audio thread)
    int16_t _networkScratchBuffer[AudioConstants::NETWORK_FRAME_SAMPLES_AMBISONIC];

    // for output audio (used by this thread)
    int _outputPeriod { 0 };
    float* _outputMixBuffer { NULL };
    int16_t* _outputScratchBuffer { NULL };

    // for local audio (used by audio injectors thread)
    float _localMixBuffer[AudioConstants::NETWORK_FRAME_SAMPLES_STEREO];
    int16_t _localScratchBuffer[AudioConstants::NETWORK_FRAME_SAMPLES_AMBISONIC];
    float* _localOutputMixBuffer { NULL };
    Mutex _localAudioMutex;

    AudioLimiter _audioLimiter;

    // Adds Reverb
    void configureReverb();
    void updateReverbOptions();

    void handleLocalEchoAndReverb(QByteArray& inputByteArray);

    bool switchInputToAudioDevice(const QAudioDeviceInfo& inputDeviceInfo, bool isShutdownRequest = false);
    bool switchOutputToAudioDevice(const QAudioDeviceInfo& outputDeviceInfo, bool isShutdownRequest = false);

    // Callback acceleration dependent calculations
    int calculateNumberOfInputCallbackBytes(const QAudioFormat& format) const;
    int calculateNumberOfFrameSamples(int numBytes) const;

    quint16 _outgoingAvatarAudioSequenceNumber;

    AudioOutputIODevice _audioOutputIODevice;

    AudioIOStats _stats;

    AudioGate* _audioGate { nullptr };
    bool _audioGateOpen { true };

    AudioPositionGetter _positionGetter;
    AudioOrientationGetter _orientationGetter;

    glm::vec3 avatarBoundingBoxCorner;
    glm::vec3 avatarBoundingBoxScale;

    QAudioDeviceInfo _inputDeviceInfo;
    QAudioDeviceInfo _outputDeviceInfo;

    QList<QAudioDeviceInfo> _inputDevices;
    QList<QAudioDeviceInfo> _outputDevices;

    bool _hasReceivedFirstPacket { false };

    QVector<AudioInjectorPointer> _activeLocalAudioInjectors;

    bool _isPlayingBackRecording { false };

    CodecPluginPointer _codec;
    QString _selectedCodecName;
    Encoder* _encoder { nullptr }; // for outbound mic stream

    RateCounter<> _silentOutbound;
    RateCounter<> _audioOutbound;
    RateCounter<> _silentInbound;
    RateCounter<> _audioInbound;

    QTimer* _checkDevicesTimer { nullptr };
    QTimer* _checkPeakValuesTimer { nullptr };
};


#endif // hifi_AudioClient_h
