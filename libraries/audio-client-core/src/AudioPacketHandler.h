//
//  AudioPacketHandler.h
//  libraries/audio-client-core/src
//
//  Created by Nshan G. on 4 July 2022
//  Copyright 2013 High Fidelity, Inc.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_AUDIO_CLIENT_CORE_SRC_AUDIOPACKETHANDLER_H
#define LIBRARIES_AUDIO_CLIENT_CORE_SRC_AUDIOPACKETHANDLER_H

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

#include <AbstractAudioInterface.h>
#include <AudioEffectOptions.h>
#include <AudioStreamStats.h>
#include <shared/WebRTC.h>
#include <shared/RateCounter.h>
#include <DependencyManager.h>
#include <SockAddr.h>
#include <NLPacket.h>
#include <MixedProcessedAudioStream.h>
#include <RingBufferHistory.h>
#include <Sound.h>
#include <StDev.h>
#include <AudioHRTF.h>
#include <AudioSRC.h>
#include <AudioInjector.h>
#include <AudioReverb.h>
#include <AudioLimiter.h>
#include <AudioConstants.h>
#include <AudioGate.h>
#include <Codec.h>
#include <GLMHelpers.h>

#include "AudioIOStats.h"

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

inline auto defaultAudioPositionGetter() {
    return Vectors::ZERO;
}

inline auto defaultAudioOrientationGetter() {
    return Quaternions::IDENTITY;
}

struct AudioFormat {
    enum SampleTag {
        Signed16,
        Float
    };

    SampleTag sampleType = SampleTag(-1);
    int sampleRate = -1;
    int channelCount = -1;

    int getSampleSize() const {
        switch(sampleType) {
            case Signed16: return sizeof(int16_t);
            case Float: return sizeof(float) ;
            default: return 0;
        };
    };

    int getSampleBits() const {
        return getSampleSize() * CHAR_BIT;
    };

    qint32 bytesForDuration(qint64 microseconds) const {
        return microseconds * sampleRate * channelCount * getSampleSize() / USECS_PER_SECOND;
    }

    bool operator==(const AudioFormat& other) const {
        return sampleType == other.sampleType && sampleRate == other.sampleRate && channelCount == other.channelCount;
    }
    bool operator!=(const AudioFormat& other) const {
        return !(*this == other);
    }

    bool isValid() const {
        return sampleRate != -1 && channelCount != -1 &&
            (sampleType == Signed16 || sampleType == Float);
    }
};

template <typename Derived>
class AudioPacketHandler {
    using LocalInjectorsStream = AudioMixRingBuffer;
public:
    static const int MIN_BUFFER_FRAMES = 1;
    static const int MAX_BUFFER_FRAMES = 20;

    using AudioPositionGetter = std::function<glm::vec3()>;
    using AudioOrientationGetter = std::function<glm::quat()>;

    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;

    class AudioOutputIODevice : public QIODevice {
    public:
        AudioOutputIODevice(LocalInjectorsStream& localInjectorsStream, MixedProcessedAudioStream& receivedAudioStream,
                AudioPacketHandler* audio) :
            _localInjectorsStream(localInjectorsStream), _receivedAudioStream(receivedAudioStream),
            _audio(audio), _unfulfilledReads(0) {}

        void open() { QIODevice::open(QIODevice::ReadOnly | QIODevice::Unbuffered); }
        qint64 readData(char* data, qint64 maxSize) override;
        qint64 writeData(const char* data, qint64 maxSize) override { return 0; }
        int getRecentUnfulfilledReads() { int unfulfilledReads = _unfulfilledReads; _unfulfilledReads = 0; return unfulfilledReads; }
    private:
        using QIODevice::open; // silence overloaded virtual warning

        LocalInjectorsStream& _localInjectorsStream;
        MixedProcessedAudioStream& _receivedAudioStream;
        AudioPacketHandler* _audio;
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

    float getLastInputLoudness() const { return _lastInputLoudness; }

    float getTimeSinceLastClip() const { return _timeSinceLastClip; }
    float getAudioAverageInputLoudness() const { return _lastInputLoudness; }

    const AudioIOStats& getStats() const { return _stats; }

    bool isSimulatingJitter() { return _gate.isSimulatingJitter(); }
    void setIsSimulatingJitter(bool enable) { _gate.setIsSimulatingJitter(enable); }

    int getGateThreshold() { return _gate.getThreshold(); }
    void setGateThreshold(int threshold) { _gate.setThreshold(threshold); }

    void setPositionGetter(AudioPositionGetter positionGetter) { _positionGetter = positionGetter; }
    void setOrientationGetter(AudioOrientationGetter orientationGetter) { _orientationGetter = orientationGetter; }

    void setIsPlayingBackRecording(bool isPlayingBackRecording) { _isPlayingBackRecording = isPlayingBackRecording; }

    Q_INVOKABLE void setAvatarBoundingBoxParameters(glm::vec3 corner, glm::vec3 scale);

    // FIXME: CRTP should override virtual member of in audio-client/sec/AudioClient
    bool outputLocalInjector(const AudioInjectorPointer& injector);

    // HifiAudioDeviceInfo getActiveAudioDevice(QAudio::Mode mode) const;
    // QList<HifiAudioDeviceInfo> getAudioDevices(QAudio::Mode mode) const;

    // void enablePeakValues(bool enable) { _enablePeakValues = enable; }
    // bool peakValuesAvailable() const;

    // bool getNamedAudioDeviceForModeExists(QAudio::Mode mode, const QString& deviceName);

    void setAudioPaused(bool pause);

    // FIXME: CRTP should override virtual member of in audio-client/sec/AudioClient
    AudioSolo& getAudioSolo() { return _solo; }

    int getNumLocalInjectors();

    void start();
    void stop();

    void handleAudioEnvironmentDataPacket(QSharedPointer<ReceivedMessage> message);
    void handleAudioDataPacket(QSharedPointer<ReceivedMessage> message);
    void handleNoisyMutePacket(QSharedPointer<ReceivedMessage> message);
    void handleMuteEnvironmentPacket(QSharedPointer<ReceivedMessage> message);
    void handleSelectedAudioFormat(QSharedPointer<ReceivedMessage> message);
    void handleMismatchAudioFormat(SharedNodePointer node, const QString& currentCodec, const QString& recievedCodec);

    void sendDownstreamAudioStatsPacket() { _stats.publish(); }
    void handleMicAudioInput(const char* data, int size);
    void sendInput();
    void audioInputStateChanged(QAudio::State state);
    void handleRecordedAudioInput(const QByteArray& audio);
    void reset();
    void audioMixerKilled();

    void setMuted(bool muted, bool emitSignal = true);
    bool isMuted() { return _isMuted; }

    void setNoiseReduction(bool isNoiseGateEnabled, bool emitSignal = true);
    bool isNoiseReductionEnabled() const { return _isNoiseGateEnabled; }

    void setNoiseReductionAutomatic(bool isNoiseGateAutomatic, bool emitSignal = true);
    bool isNoiseReductionAutomatic() const { return _isNoiseReductionAutomatic; }

    void setNoiseReductionThreshold(float noiseReductionThreshold, bool emitSignal = true);
    float noiseReductionThreshold() const { return _noiseReductionThreshold; }

    void setAcousticEchoCancellation(bool isAECEnabled, bool emitSignal = true);
    bool isAcousticEchoCancellationEnabled() const { return _isAECEnabled; }

    // FIXME: CRTP these should override virtual members of in audio-client/sec/AudioClient
    bool getServerEcho() { return _shouldEchoToServer; }
    void setServerEcho(bool serverEcho) { _shouldEchoToServer = serverEcho; }
    void toggleServerEcho() { _shouldEchoToServer = !_shouldEchoToServer; }

    void processReceivedSamples(const QByteArray& inputBuffer, QByteArray& outputBuffer);
    void sendMuteEnvironmentPacket();

    int setOutputBufferSize(int numFrames, bool persist = true);

    void setReverb(bool reverb);
    void setReverbOptions(const AudioEffectOptions* options);

    void setLocalInjectorGain(float gain) { _localInjectorGain = gain; };
    void setSystemInjectorGain(float gain) { _systemInjectorGain = gain; };
    void setOutputGain(float gain) { _outputGain = gain; };

protected:
    AudioPacketHandler();
    ~AudioPacketHandler();

    void cleanupInput();
    bool setupInput(AudioFormat);
    void setupDummyInput();
    bool isDummyInput();

    void cleanupOutput();
    bool setupOutput(AudioFormat);

    AudioOutputIODevice _audioOutputIODevice {_localInjectorsStream, _receivedAudioStream, this};
    bool _isMuted {false};
    glm::vec3 avatarBoundingBoxCorner {};
    glm::vec3 avatarBoundingBoxScale {};
    AudioFormat _inputFormat {};
    AudioFormat _outputFormat {};
    QString _selectedCodecName;

private:
    static const int RECEIVED_AUDIO_STREAM_CAPACITY_FRAMES{ 100 };
    // OUTPUT_CHANNEL_COUNT is audio pipeline output format, which is always 2 channel.
    // _outputFormat.channelCount() is device output format, which may be 1 or multichannel.
    static const int OUTPUT_CHANNEL_COUNT{ 2 };
    static const int STARVE_DETECTION_THRESHOLD{ 3 };
    static const int STARVE_DETECTION_PERIOD{ 10 * 1000 }; // 10 Seconds

    friend class CheckDevicesThread;
    friend class LocalInjectorsThread;

    Derived& derived();
    const Derived& derived() const;

    float loudnessToLevel(float loudness);

    void outputFormatChanged();
    void handleAudioInput(QByteArray& audioBuffer);
    void prepareLocalAudioInjectors(std::unique_ptr<Lock> localAudioLock = nullptr);
    bool mixLocalAudioInjectors(float* mixBuffer);
    float azimuthForSource(const glm::vec3& relativePosition);
    float gainForSource(float distance, float volume);

    class Gate {
    public:
        Gate(AudioPacketHandler* audioClient);

        bool isSimulatingJitter() { return _isSimulatingJitter; }
        void setIsSimulatingJitter(bool enable);

        int getThreshold() { return _threshold; }
        void setThreshold(int threshold);

        void insert(QSharedPointer<ReceivedMessage> message);

    private:
        void flush();

        AudioPacketHandler* _audioClient;
        std::queue<QSharedPointer<ReceivedMessage>> _queue;
        std::mutex _mutex;

        int _index {0};
        int _threshold {1};
        bool _isSimulatingJitter {false};
    };

    Gate _gate{ this };

    Mutex _injectorsMutex;
    QTimer* _dummyAudioInput {nullptr};
    AudioFormat _desiredInputFormat;
    std::atomic<bool> _audioOutputInitialized {false};
    AudioFormat _desiredOutputFormat;
    int _outputFrameSize {0};
    int _numOutputCallbackBytes {0};
    std::vector<int16_t> _inputTypeConversionBuffer {};
    AudioRingBuffer _inputRingBuffer {0};
    LocalInjectorsStream _localInjectorsStream {0 , 1};
    // In order to use _localInjectorsStream as a lock-free pipe,
    // use it with a single producer/consumer, and track available samples and injectors
    std::atomic<int> _localSamplesAvailable {0};
    std::atomic<bool> _localInjectorsAvailable  {false};
    MixedProcessedAudioStream _receivedAudioStream {RECEIVED_AUDIO_STREAM_CAPACITY_FRAMES};

    quint64 _outputStarveDetectionStartTimeMsec {0};
    int _outputStarveDetectionCount {0};

    int _sessionOutputBufferSizeFrames {DEFAULT_BUFFER_FRAMES};

    float _lastRawInputLoudness {0.0f};    // before mute/gate
    float _lastSmoothedRawInputLoudness {0.0f};
    float _lastInputLoudness {0.0f};       // after mute/gate
    float _timeSinceLastClip {-1.0f};
    int _totalInputAudioSamples;

    bool _shouldEchoLocally {false};
    bool _shouldEchoToServer {false};
    bool _isNoiseGateEnabled {true};
    bool _isNoiseReductionAutomatic {true};
    float _noiseReductionThreshold {0.1f};
    bool _isAECEnabled {true};

    bool _reverb {false};
    AudioEffectOptions _scriptReverbOptions;
    AudioEffectOptions _zoneReverbOptions;
    AudioEffectOptions* _reverbOptions {&_scriptReverbOptions};
    AudioReverb _sourceReverb {AudioConstants::SAMPLE_RATE};
    AudioReverb _listenerReverb {AudioConstants::SAMPLE_RATE};
    AudioReverb _localReverb {AudioConstants::SAMPLE_RATE};

    // possible streams needed for resample
    AudioSRC* _inputToNetworkResampler {nullptr};
    AudioSRC* _networkToOutputResampler {nullptr};
    AudioSRC* _localToOutputResampler {nullptr};

    // for network audio (used by network audio thread)
    int16_t _networkScratchBuffer[AudioConstants::NETWORK_FRAME_SAMPLES_AMBISONIC];

    // for output audio (used by this thread)
    int _outputPeriod {0};
    float* _outputMixBuffer {NULL};
    int16_t* _outputScratchBuffer {NULL};
    std::atomic<float> _outputGain {1.0f};
    float _lastOutputGain {1.0f};

    // for local audio (used by audio injectors thread)
    std::atomic<float> _localInjectorGain {1.0f};
    std::atomic<float> _systemInjectorGain {1.0f};
    float _localMixBuffer[AudioConstants::NETWORK_FRAME_SAMPLES_STEREO];
    int16_t _localScratchBuffer[AudioConstants::NETWORK_FRAME_SAMPLES_AMBISONIC];
    float* _localOutputMixBuffer {NULL};
    Mutex _localAudioMutex;
    AudioLimiter _audioLimiter {AudioConstants::SAMPLE_RATE, OUTPUT_CHANNEL_COUNT};

    // Adds Reverb
    void configureReverb();
    void updateReverbOptions();

#if defined(WEBRTC_AUDIO)
    static const int WEBRTC_SAMPLE_RATE_MAX = 96000;
    static const int WEBRTC_CHANNELS_MAX = 2;
    static const int WEBRTC_FRAMES_MAX = webrtc::AudioProcessing::kChunkSizeMs * WEBRTC_SAMPLE_RATE_MAX / 1000;

    webrtc::AudioProcessing* _apm {nullptr};

    int16_t _fifoFarEnd[WEBRTC_CHANNELS_MAX * WEBRTC_FRAMES_MAX] {};
    int _numFifoFarEnd = 0; // numFrames saved in fifo

    void configureWebrtc();
    void processWebrtcFarEnd(const int16_t* samples, int numFrames, int numChannels, int sampleRate);
    void processWebrtcNearEnd(int16_t* samples, int numFrames, int numChannels, int sampleRate);
#endif

    // Callback acceleration dependent calculations
    int calculateNumberOfInputCallbackBytes(const AudioFormat& format) const;
    int calculateNumberOfFrameSamples(int numBytes) const;

    quint16 _outgoingAvatarAudioSequenceNumber {0};

    AudioIOStats _stats {&_receivedAudioStream};

    AudioGate* _audioGate {nullptr};
    bool _audioGateOpen {true};

    AudioPositionGetter _positionGetter{ defaultAudioPositionGetter };
    AudioOrientationGetter _orientationGetter{ defaultAudioOrientationGetter };

    bool _hasReceivedFirstPacket {false};

    QVector<AudioInjectorPointer> _activeLocalAudioInjectors;

    bool _isPlayingBackRecording {false};
    bool _audioPaused {false};

    std::shared_ptr<Codec> _codec;
    Encoder* _encoder {nullptr};  // for outbound mic stream

    RateCounter<> _silentOutbound;
    RateCounter<> _audioOutbound;
    RateCounter<> _silentInbound;
    RateCounter<> _audioInbound;

#if defined(Q_OS_ANDROID)
    bool _shouldRestartInputSetup {true};  // Should we restart the input device because of an unintended stop?
#endif

    AudioSolo _solo;

    QFuture<void> _localPrepInjectorFuture;

    bool _isRecording {false};
};

#endif /* end of include guard */
