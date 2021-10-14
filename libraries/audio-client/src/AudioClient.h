//
//  AudioClient.h
//  libraries/audio-client/src
//
//  Created by Stephen Birarda on 1/22/13.
//  Copyright 2013 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
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

#include <QFuture>
#include <QtCore/QtGlobal>
#include <QtCore/QByteArray>
#include <QtCore/QElapsedTimer>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QVector>
#include <QtMultimedia/QAudio>
#include <QtMultimedia/QAudioFormat>
#include <QtMultimedia/QAudioInput>
#include <AbstractAudioInterface.h>
#include <AudioEffectOptions.h>
#include <AudioStreamStats.h>
#include <shared/WebRTC.h>

#include <DependencyManager.h>
#include <SockAddr.h>
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
#include "AudioFileWav.h"
#include "HifiAudioDeviceInfo.h"

#if defined(WEBRTC_AUDIO)
#  define WEBRTC_APM_DEBUG_DUMP 0
#  include <modules/audio_processing/include/audio_processing.h>
#  include "modules/audio_processing/audio_processing_impl.h"
#endif

#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable : 4273 )
#pragma warning( disable : 4305 )
#endif

#ifdef _WIN32
#pragma warning( pop )
#endif

#if defined (Q_OS_ANDROID)
#define VOICE_RECOGNITION "voicerecognition"
#define VOICE_COMMUNICATION "voicecommunication"

#define SETTING_AEC_KEY "Android/aec"
#define DEFAULT_AEC_ENABLED true
#endif

class QAudioInput;
class QAudioOutput;
class QIODevice;

class Transform;
class NLPacket;

#define DEFAULT_STARVE_DETECTION_ENABLED true
#define DEFAULT_BUFFER_FRAMES 1

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
        qint64 readData(char* data, qint64 maxSize) override;
        qint64 writeData(const char* data, qint64 maxSize) override { return 0; }
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

    const QAudioFormat& getOutputFormat() const { return _outputFormat; }

    float getLastInputLoudness() const { return _lastInputLoudness; }

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

    HifiAudioDeviceInfo getActiveAudioDevice(QAudio::Mode mode) const;
    QList<HifiAudioDeviceInfo> getAudioDevices(QAudio::Mode mode) const;
  
    void enablePeakValues(bool enable) { _enablePeakValues = enable; }
    bool peakValuesAvailable() const;

    static const float CALLBACK_ACCELERATOR_RATIO;

    bool getNamedAudioDeviceForModeExists(QAudio::Mode mode, const QString& deviceName);

    void setRecording(bool isRecording) { _isRecording = isRecording; };
    bool getRecording() { return _isRecording; };

    bool startRecording(const QString& filename);
    void stopRecording();
    void setAudioPaused(bool pause);

    AudioSolo& getAudioSolo() override { return _solo; }

#ifdef Q_OS_WIN
    static QString getWinDeviceName(wchar_t* guid);
#endif

#if defined(Q_OS_ANDROID)
    bool isHeadsetPluggedIn() { return _isHeadsetPluggedIn; }
#endif

    int getNumLocalInjectors();

public slots:
    void start();
    void stop();

    void handleAudioEnvironmentDataPacket(QSharedPointer<ReceivedMessage> message);
    void handleAudioDataPacket(QSharedPointer<ReceivedMessage> message);
    void handleNoisyMutePacket(QSharedPointer<ReceivedMessage> message);
    void handleMuteEnvironmentPacket(QSharedPointer<ReceivedMessage> message);
    void handleSelectedAudioFormat(QSharedPointer<ReceivedMessage> message);
    void handleMismatchAudioFormat(SharedNodePointer node, const QString& currentCodec, const QString& recievedCodec);

    void sendDownstreamAudioStatsPacket() { _stats.publish(); }
    void handleMicAudioInput();
    void audioInputStateChanged(QAudio::State state);
    void checkInputTimeout();
    void handleDummyAudioInput();
    void handleRecordedAudioInput(const QByteArray& audio);
    void reset();
    void audioMixerKilled();

    void setMuted(bool muted, bool emitSignal = true);
    bool isMuted() { return _isMuted; }

    virtual bool setIsStereoInput(bool stereo) override;
    virtual bool isStereoInput() override { return _isStereoInput; }

    void setNoiseReduction(bool isNoiseGateEnabled, bool emitSignal = true);
    bool isNoiseReductionEnabled() const { return _isNoiseGateEnabled; }
    
    void setNoiseReductionAutomatic(bool isNoiseGateAutomatic, bool emitSignal = true);
    bool isNoiseReductionAutomatic() const { return _isNoiseReductionAutomatic; }
    
    void setNoiseReductionThreshold(float noiseReductionThreshold, bool emitSignal = true);
    float noiseReductionThreshold() const { return _noiseReductionThreshold; }

    void setWarnWhenMuted(bool isNoiseGateEnabled, bool emitSignal = true);
    bool isWarnWhenMutedEnabled() const { return _warnWhenMuted; }

    void setAcousticEchoCancellation(bool isAECEnabled, bool emitSignal = true);
    bool isAcousticEchoCancellationEnabled() const { return _isAECEnabled; }

    virtual bool getLocalEcho() override { return _shouldEchoLocally; }
    virtual void setLocalEcho(bool localEcho) override { _shouldEchoLocally = localEcho; }
    virtual void toggleLocalEcho() override { _shouldEchoLocally = !_shouldEchoLocally; }

    virtual bool getServerEcho() override { return _shouldEchoToServer; }
    virtual void setServerEcho(bool serverEcho) override { _shouldEchoToServer = serverEcho; }
    virtual void toggleServerEcho() override { _shouldEchoToServer = !_shouldEchoToServer; }

    void processReceivedSamples(const QByteArray& inputBuffer, QByteArray& outputBuffer);
    void sendMuteEnvironmentPacket();

    int setOutputBufferSize(int numFrames, bool persist = true);

    bool shouldLoopbackInjectors() override { return _shouldEchoToServer; }

    // calling with a null QAudioDevice will use the system default
    bool switchAudioDevice(QAudio::Mode mode, const HifiAudioDeviceInfo& deviceInfo = HifiAudioDeviceInfo());
    bool switchAudioDevice(QAudio::Mode mode, const QString& deviceName, bool isHmd);
    void setHmdAudioName(QAudio::Mode mode, const QString& name);
    // Qt opensles plugin is not able to detect when the headset is plugged in
    void setHeadsetPluggedIn(bool pluggedIn);

    float getInputVolume() const { return (_audioInput) ? (float)_audioInput->volume() : 0.0f; }
    void setInputVolume(float volume, bool emitSignal = true);
    void setReverb(bool reverb);
    void setReverbOptions(const AudioEffectOptions* options);

    void setLocalInjectorGain(float gain) { _localInjectorGain = gain; };
    void setSystemInjectorGain(float gain) { _systemInjectorGain = gain; };
    void setOutputGain(float gain) { _outputGain = gain; };

    void outputNotify();
    void noteAwakening();

    void loadSettings();
    void saveSettings();

signals:
    void inputVolumeChanged(float volume);
    void muteToggled(bool muted);
    void noiseReductionChanged(bool noiseReductionEnabled);
    void noiseReductionAutomaticChanged(bool noiseReductionAutomatic);
    void noiseReductionThresholdChanged(bool noiseReductionThreshold);
    void warnWhenMutedChanged(bool warnWhenMutedEnabled);
    void acousticEchoCancellationChanged(bool acousticEchoCancellationEnabled);
    void mutedByMixer();
    void inputReceived(const QByteArray& inputSamples);
    void inputLoudnessChanged(float loudness, bool isClipping);
    void outputBytesToNetwork(int numBytes);
    void inputBytesFromNetwork(int numBytes);
    void noiseGateOpened();
    void noiseGateClosed();

    void changeDevice(const HifiAudioDeviceInfo& outputDeviceInfo);

    void deviceChanged(QAudio::Mode mode, const HifiAudioDeviceInfo& device);
    void devicesChanged(QAudio::Mode mode, const QList<HifiAudioDeviceInfo>& devices);
    void peakValueListChanged(const QList<float> peakValueList);

    void receivedFirstPacket();
    void disconnected();

    void audioFinished();

    void muteEnvironmentRequested(glm::vec3 position, float radius);

    void outputBufferReceived(const QByteArray _outputBuffer);

protected:
    AudioClient();
    ~AudioClient();

    virtual void customDeleter() override;

private:
    static const int RECEIVED_AUDIO_STREAM_CAPACITY_FRAMES{ 100 };
    // OUTPUT_CHANNEL_COUNT is audio pipeline output format, which is always 2 channel.
    // _outputFormat.channelCount() is device output format, which may be 1 or multichannel.
    static const int OUTPUT_CHANNEL_COUNT{ 2 };
    static const int STARVE_DETECTION_THRESHOLD{ 3 };
    static const int STARVE_DETECTION_PERIOD{ 10 * 1000 }; // 10 Seconds

    static const AudioPositionGetter DEFAULT_POSITION_GETTER;
    static const AudioOrientationGetter DEFAULT_ORIENTATION_GETTER;

    friend class CheckDevicesThread;
    friend class LocalInjectorsThread;

    float loudnessToLevel(float loudness);

    // background tasks
    void checkDevices();
    void checkPeakValues();

    void outputFormatChanged();
    void handleAudioInput(QByteArray& audioBuffer);
    void prepareLocalAudioInjectors(std::unique_ptr<Lock> localAudioLock = nullptr);
    bool mixLocalAudioInjectors(float* mixBuffer);
    float azimuthForSource(const glm::vec3& relativePosition);
    float gainForSource(float distance, float volume);

#ifdef Q_OS_ANDROID
    QTimer _checkInputTimer{ this };
    long _inputReadsSinceLastCheck = 0l;
    bool _isHeadsetPluggedIn { false };
#endif

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

    Gate _gate{ this };

    Mutex _injectorsMutex;
    QAudioInput* _audioInput{ nullptr };
    QTimer* _dummyAudioInput{ nullptr };
    QAudioFormat _desiredInputFormat;
    QAudioFormat _inputFormat;
    QIODevice* _inputDevice{ nullptr };
    int _numInputCallbackBytes{ 0 };
    QAudioOutput* _audioOutput{ nullptr };
    std::atomic<bool> _audioOutputInitialized { false };
    QAudioFormat _desiredOutputFormat;
    QAudioFormat _outputFormat;
    int _outputFrameSize{ 0 };
    int _numOutputCallbackBytes{ 0 };
    QAudioOutput* _loopbackAudioOutput{ nullptr };
    QIODevice* _loopbackOutputDevice{ nullptr };
    AudioRingBuffer _inputRingBuffer{ 0 };
    LocalInjectorsStream _localInjectorsStream{ 0 , 1 };
    // In order to use _localInjectorsStream as a lock-free pipe,
    // use it with a single producer/consumer, and track available samples and injectors
    std::atomic<int> _localSamplesAvailable { 0 };
    std::atomic<bool> _localInjectorsAvailable { false };
    MixedProcessedAudioStream _receivedAudioStream{ RECEIVED_AUDIO_STREAM_CAPACITY_FRAMES };
    bool _isStereoInput{ false };
    std::atomic<bool> _enablePeakValues { false };

    quint64 _outputStarveDetectionStartTimeMsec{ 0 };
    int _outputStarveDetectionCount { 0 };

    Setting::Handle<int> _outputBufferSizeFrames{"audioOutputBufferFrames", DEFAULT_BUFFER_FRAMES};
    int _sessionOutputBufferSizeFrames{ _outputBufferSizeFrames.get() };
    Setting::Handle<bool> _outputStarveDetectionEnabled{ "audioOutputStarveDetectionEnabled", DEFAULT_STARVE_DETECTION_ENABLED};

    StDev _stdev;
    QElapsedTimer _timeSinceLastReceived;
    float _lastRawInputLoudness{ 0.0f };    // before mute/gate
    float _lastSmoothedRawInputLoudness{ 0.0f };
    float _lastInputLoudness{ 0.0f };       // after mute/gate
    float _timeSinceLastClip{ -1.0f };
    int _totalInputAudioSamples;

    bool _isMuted{ false };
    bool _shouldEchoLocally{ false };
    bool _shouldEchoToServer{ false };
    bool _isNoiseGateEnabled{ true };
    bool _isNoiseReductionAutomatic{ true };
    float _noiseReductionThreshold{ 0.1f };
    bool _warnWhenMuted;
    bool _isAECEnabled{ true };

    bool _reverb{ false };
    AudioEffectOptions _scriptReverbOptions;
    AudioEffectOptions _zoneReverbOptions;
    AudioEffectOptions* _reverbOptions{ &_scriptReverbOptions };
    AudioReverb _sourceReverb { AudioConstants::SAMPLE_RATE };
    AudioReverb _listenerReverb { AudioConstants::SAMPLE_RATE };
    AudioReverb _localReverb { AudioConstants::SAMPLE_RATE };

    // possible streams needed for resample
    AudioSRC* _inputToNetworkResampler{ nullptr };
    AudioSRC* _networkToOutputResampler{ nullptr };
    AudioSRC* _localToOutputResampler{ nullptr };
    AudioSRC* _loopbackResampler{ nullptr };

    // for network audio (used by network audio thread)
    int16_t _networkScratchBuffer[AudioConstants::NETWORK_FRAME_SAMPLES_AMBISONIC];

    // for output audio (used by this thread)
    int _outputPeriod { 0 };
    float* _outputMixBuffer { NULL };
    int16_t* _outputScratchBuffer { NULL };
    std::atomic<float> _outputGain { 1.0f };
    float _lastOutputGain { 1.0f };

    // for local audio (used by audio injectors thread)
    std::atomic<float> _localInjectorGain { 1.0f };
    std::atomic<float> _systemInjectorGain { 1.0f };
    float _localMixBuffer[AudioConstants::NETWORK_FRAME_SAMPLES_STEREO];
    int16_t _localScratchBuffer[AudioConstants::NETWORK_FRAME_SAMPLES_AMBISONIC];
    float* _localOutputMixBuffer { NULL };
    Mutex _localAudioMutex;
    AudioLimiter _audioLimiter{ AudioConstants::SAMPLE_RATE, OUTPUT_CHANNEL_COUNT };

    // Adds Reverb
    void configureReverb();
    void updateReverbOptions();
    void handleLocalEchoAndReverb(QByteArray& inputByteArray);

#if defined(WEBRTC_AUDIO)
    static const int WEBRTC_SAMPLE_RATE_MAX = 96000;
    static const int WEBRTC_CHANNELS_MAX = 2;
    static const int WEBRTC_FRAMES_MAX = webrtc::AudioProcessing::kChunkSizeMs * WEBRTC_SAMPLE_RATE_MAX / 1000;

    webrtc::AudioProcessing* _apm { nullptr };

    int16_t _fifoFarEnd[WEBRTC_CHANNELS_MAX * WEBRTC_FRAMES_MAX] {};
    int _numFifoFarEnd = 0; // numFrames saved in fifo

    void configureWebrtc();
    void processWebrtcFarEnd(const int16_t* samples, int numFrames, int numChannels, int sampleRate);
    void processWebrtcNearEnd(int16_t* samples, int numFrames, int numChannels, int sampleRate);
#endif

    bool switchInputToAudioDevice(const HifiAudioDeviceInfo inputDeviceInfo, bool isShutdownRequest = false);
    bool switchOutputToAudioDevice(const HifiAudioDeviceInfo outputDeviceInfo, bool isShutdownRequest = false);

    // Callback acceleration dependent calculations
    int calculateNumberOfInputCallbackBytes(const QAudioFormat& format) const;
    int calculateNumberOfFrameSamples(int numBytes) const;

    quint16 _outgoingAvatarAudioSequenceNumber{ 0 };

    AudioOutputIODevice _audioOutputIODevice{ _localInjectorsStream, _receivedAudioStream, this };

    AudioIOStats _stats{ &_receivedAudioStream };

    AudioGate* _audioGate { nullptr };
    bool _audioGateOpen { true };

    AudioPositionGetter _positionGetter{ DEFAULT_POSITION_GETTER };
    AudioOrientationGetter _orientationGetter{ DEFAULT_ORIENTATION_GETTER };

    glm::vec3 avatarBoundingBoxCorner;
    glm::vec3 avatarBoundingBoxScale;

    HifiAudioDeviceInfo _inputDeviceInfo;
    HifiAudioDeviceInfo _outputDeviceInfo;

    QList<HifiAudioDeviceInfo> _inputDevices;
    QList<HifiAudioDeviceInfo> _outputDevices;

    QString _hmdInputName { QString() };
    QString _hmdOutputName{ QString() };

    AudioFileWav _audioFileWav;

    bool _hasReceivedFirstPacket { false };

    QVector<AudioInjectorPointer> _activeLocalAudioInjectors;

    bool _isPlayingBackRecording { false };
    bool _audioPaused { false };

    CodecPluginPointer _codec;
    QString _selectedCodecName;
    Encoder* _encoder { nullptr };  // for outbound mic stream

    RateCounter<> _silentOutbound;
    RateCounter<> _audioOutbound;
    RateCounter<> _silentInbound;
    RateCounter<> _audioInbound;

#if defined(Q_OS_ANDROID)
    bool _shouldRestartInputSetup { true };  // Should we restart the input device because of an unintended stop?
#endif

    AudioSolo _solo;
    
    QFuture<void> _localPrepInjectorFuture;
    QReadWriteLock _hmdNameLock;
    Mutex _checkDevicesMutex;
    QTimer* _checkDevicesTimer { nullptr };
    Mutex _checkPeakValuesMutex;
    QTimer* _checkPeakValuesTimer { nullptr };

    bool _isRecording { false };
};


#endif // hifi_AudioClient_h