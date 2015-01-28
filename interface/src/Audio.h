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
#include <AudioRingBuffer.h>
#include <DependencyManager.h>
#include <StDev.h>

#include "audio/AudioIOStats.h"
#include "audio/AudioNoiseGate.h"
#include "AudioStreamStats.h"
#include "Recorder.h"
#include "RingBufferHistory.h"
#include "AudioRingBuffer.h"
#include "AudioFormat.h"
#include "AudioBuffer.h"
#include "AudioSourceTone.h"
#include "AudioSourceNoise.h"
#include "AudioGain.h"

#include "MixedProcessedAudioStream.h"
#include "AudioEffectOptions.h"


#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable : 4273 )
#pragma warning( disable : 4305 )
#endif
extern "C" {
    #include <gverb.h>
    #include <gverbdsp.h>
}
#ifdef _WIN32
#pragma warning( pop )
#endif

static const int NUM_AUDIO_CHANNELS = 2;

static const int DEFAULT_AUDIO_OUTPUT_BUFFER_SIZE_FRAMES = 3;
static const int MIN_AUDIO_OUTPUT_BUFFER_SIZE_FRAMES = 1;
static const int MAX_AUDIO_OUTPUT_BUFFER_SIZE_FRAMES = 20;
static const int DEFAULT_AUDIO_OUTPUT_STARVE_DETECTION_ENABLED = true;
static const int DEFAULT_AUDIO_OUTPUT_STARVE_DETECTION_THRESHOLD = 3;
static const int DEFAULT_AUDIO_OUTPUT_STARVE_DETECTION_PERIOD = 10 * 1000; // 10 Seconds

class QAudioInput;
class QAudioOutput;
class QIODevice;

class Audio : public AbstractAudioInterface, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
public:

    class AudioOutputIODevice : public QIODevice {
    public:
        AudioOutputIODevice(MixedProcessedAudioStream& receivedAudioStream, Audio* audio) : 
            _receivedAudioStream(receivedAudioStream), _audio(audio), _unfulfilledReads(0) {};

        void start() { open(QIODevice::ReadOnly); }
        void stop() { close(); }
        qint64	readData(char * data, qint64 maxSize);
        qint64	writeData(const char * data, qint64 maxSize) { return 0; }
        
        int getRecentUnfulfilledReads() { int unfulfilledReads = _unfulfilledReads; _unfulfilledReads = 0; return unfulfilledReads; }
    private:
        MixedProcessedAudioStream& _receivedAudioStream;
        Audio* _audio;
        int _unfulfilledReads;
    };
    
    const MixedProcessedAudioStream& getReceivedAudioStream() const { return _receivedAudioStream; }
    MixedProcessedAudioStream& getReceivedAudioStream() { return _receivedAudioStream; }

    float getLastInputLoudness() const { return glm::max(_lastInputLoudness - _inputGate.getMeasuredFloor(), 0.0f); }

    float getTimeSinceLastClip() const { return _timeSinceLastClip; }
    float getAudioAverageInputLoudness() const { return _lastInputLoudness; }

    int getDesiredJitterBufferFrames() const { return _receivedAudioStream.getDesiredJitterBufferFrames(); }
    
    bool isMuted() { return _muted; }
    
    void setIsStereoInput(bool isStereoInput);
    
    const AudioIOStats& getStats() const { return _stats; }

    float getInputRingBufferMsecsAvailable() const;
    float getAudioOutputMsecsUnplayed() const;
    
    void setRecorder(RecorderPointer recorder) { _recorder = recorder; }

    int getOutputBufferSize() { return _outputBufferSizeFrames; }

    bool getOutputStarveDetectionEnabled() { return _outputStarveDetectionEnabled; }
    void setOutputStarveDetectionEnabled(bool enabled) { _outputStarveDetectionEnabled = enabled; }

    int getOutputStarveDetectionPeriod() { return _outputStarveDetectionPeriodMsec; }
    void setOutputStarveDetectionPeriod(int msecs) { _outputStarveDetectionPeriodMsec = msecs; }

    int getOutputStarveDetectionThreshold() { return _outputStarveDetectionThreshold; }
    void setOutputStarveDetectionThreshold(int threshold) { _outputStarveDetectionThreshold = threshold; }

    static const float CALLBACK_ACCELERATOR_RATIO;
    
public slots:
    void start();
    void stop();
    void addReceivedAudioToStream(const QByteArray& audioByteArray);
    void parseAudioEnvironmentData(const QByteArray& packet);
    void sendDownstreamAudioStatsPacket() { _stats.sendDownstreamAudioStatsPacket(); }
    void parseAudioStreamStatsPacket(const QByteArray& packet) { _stats.parseAudioStreamStatsPacket(packet); }
    void handleAudioInput();
    void reset();
    void audioMixerKilled();
    void toggleMute();
    
    void toggleAudioSourceInject();
    void selectAudioSourcePinkNoise();
    void selectAudioSourceSine440();
    
    void toggleAudioNoiseReduction() { _isNoiseGateEnabled = !_isNoiseGateEnabled; }
    
    void toggleLocalEcho() { _shouldEchoLocally = !_shouldEchoLocally; }
    void toggleServerEcho() { _shouldEchoToServer = !_shouldEchoToServer; }
    
    void toggleStereoInput() { setIsStereoInput(!_isStereoInput); }
  
    void processReceivedSamples(const QByteArray& inputBuffer, QByteArray& outputBuffer);
    void sendMuteEnvironmentPacket();

    void setOutputBufferSize(int numFrames);

    virtual bool outputLocalInjector(bool isStereo, qreal volume, AudioInjector* injector);

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

    void outputNotify();
    
    void loadSettings();
    void saveSettings();
    
signals:
    bool muteToggled();
    void inputReceived(const QByteArray& inputSamples);
    void deviceChanged();

protected:
    Audio();
    
private:
    void outputFormatChanged();

    QByteArray firstInputFrame;
    QAudioInput* _audioInput;
    QAudioFormat _desiredInputFormat;
    QAudioFormat _inputFormat;
    QIODevice* _inputDevice;
    int _numInputCallbackBytes;
    int16_t _localProceduralSamples[AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL];
    QAudioOutput* _audioOutput;
    QAudioFormat _desiredOutputFormat;
    QAudioFormat _outputFormat;
    int _outputFrameSize;
    int16_t _outputProcessingBuffer[AudioConstants::NETWORK_FRAME_SAMPLES_STEREO];
    int _numOutputCallbackBytes;
    QAudioOutput* _loopbackAudioOutput;
    QIODevice* _loopbackOutputDevice;
    AudioRingBuffer _inputRingBuffer;
    MixedProcessedAudioStream _receivedAudioStream;
    bool _isStereoInput;

    QString _inputAudioDeviceName;
    QString _outputAudioDeviceName;

    int _outputBufferSizeFrames;
    bool _outputStarveDetectionEnabled;
    quint64 _outputStarveDetectionStartTimeMsec;
    int _outputStarveDetectionCount;
    int _outputStarveDetectionPeriodMsec;
    int _outputStarveDetectionThreshold; // Maximum number of starves per _outputStarveDetectionPeriod before increasing buffer size

    
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
    bool _audioSourceInjectEnabled;
    
    bool _reverb;
    AudioEffectOptions _scriptReverbOptions;
    AudioEffectOptions _zoneReverbOptions;
    AudioEffectOptions* _reverbOptions;
    ty_gverb* _gverbLocal;
    ty_gverb* _gverb;

    // Adds Reverb
    void initGverb();
    void updateGverbOptions();
    void addReverb(ty_gverb* gverb, int16_t* samples, int numSamples, QAudioFormat& format, bool noEcho = false);

    void handleLocalEchoAndReverb(QByteArray& inputByteArray);

    bool switchInputToAudioDevice(const QAudioDeviceInfo& inputDeviceInfo);
    bool switchOutputToAudioDevice(const QAudioDeviceInfo& outputDeviceInfo);

    // Callback acceleration dependent calculations
    int calculateNumberOfInputCallbackBytes(const QAudioFormat& format) const;
    int calculateNumberOfFrameSamples(int numBytes) const;
    float calculateDeviceToNetworkInputRatio(int numBytes) const;

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

    quint16 _outgoingAvatarAudioSequenceNumber;

    AudioOutputIODevice _audioOutputIODevice;
    
    WeakRecorderPointer _recorder;
    
    AudioIOStats _stats;
    
    AudioNoiseGate _inputGate;

    QVector<QString> _inputDevices;
    QVector<QString> _outputDevices;
    void checkDevices();
};


#endif // hifi_Audio_h
