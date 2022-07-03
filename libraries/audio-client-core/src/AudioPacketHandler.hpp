//
//  AudioPacketHandler.hpp
//  libraries/audio-client-core/src
//
//  Created by Nshan G on 4 July 2022.
//  Copyright 2013 High Fidelity, Inc.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AudioPacketHandler.h"

#include <cstring>
#include <math.h>
#include <sys/stat.h>

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/vector_angle.hpp>

#ifdef __APPLE__
#include <CoreAudio/AudioHardware.h>
#endif

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <Mmsystem.h>
#include <mmdeviceapi.h>
#include <devicetopology.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <VersionHelpers.h>
#endif

#include <QtConcurrent/QtConcurrent>
#include <QtCore/QThreadPool>
#include <QtCore/QBuffer>
#include <QtMultimedia/QAudioInput>
#include <QtMultimedia/QAudioOutput>

#include <shared/QtHelpers.h>
#include <ThreadHelpers.h>
#include <NodeList.h>
#include <udt/PacketHeaders.h>
#include <SettingHandle.h>
#include <SharedUtil.h>

#include "AudioClientLogging.h"
#include "AudioLogging.h"
#include "AudioHelpers.h"

#ifndef LIBRARIES_AUDIO_CLIENT_CORE_SRC_AUDIOPACKETHANDLER_HPP
#define LIBRARIES_AUDIO_CLIENT_CORE_SRC_AUDIOPACKETHANDLER_HPP

using Lock = std::unique_lock<std::mutex>;

template <typename Source, typename Destination>
auto channelUpmix(const Source* source, Destination* destination, int numSamples, int numExtraChannels) {
    Source* dest = reinterpret_cast<Source*>(destination);
    for (int i = 0; i < numSamples/2; i++) {

        *dest++ = *source++; // left
        *dest++ = *source++; // right
        // and the rest
        for (int n = 0; n < numExtraChannels; n++) {
            *dest++ = 0;
        }
    }
    return reinterpret_cast<const uint8_t*>(dest) - reinterpret_cast<const uint8_t*>(destination);
}

template <typename Source, typename Destination>
auto channelDownmix(const Source* source, Destination* destination, int numSamples) {
    Source* dest = reinterpret_cast<Source*>(destination);
    for (int i = 0; i < numSamples/2; i++) {

        // read 2 samples
        int16_t left = *source++;
        int16_t right = *source++;

        // write 1 sample
        *dest++ = (left + right) / 2;
    }
    return reinterpret_cast<const uint8_t*>(dest) - reinterpret_cast<const uint8_t*>(destination);
}

template <typename Source, typename Destination>
auto copyWithChannelConversion(const Source* source, int sourceChannels, Destination* destination, int destinationChannels, int numSamples) {
    if (destinationChannels == sourceChannels) {
        const std::ptrdiff_t byteCount = numSamples * sizeof(Source);
        memcpy(destination, source, byteCount);
        return byteCount;
    } else if (destinationChannels > sourceChannels) {
        int extraChannels = destinationChannels - sourceChannels;
        return channelUpmix(source, destination, numSamples, extraChannels);
    } else {
        return channelDownmix(source, destination, numSamples);
    }
}

inline bool detectClipping(int16_t* samples, int numSamples, int numChannels) {

    const int32_t CLIPPING_THRESHOLD = 32392;   // -0.1 dBFS
    const int CLIPPING_DETECTION = 3;           // consecutive samples over threshold

    bool isClipping = false;

    if (numChannels == 2) {
        int oversLeft = 0;
        int oversRight = 0;

        for (int i = 0; i < numSamples/2; i++) {
            int32_t left = std::abs((int32_t)samples[2*i+0]);
            int32_t right = std::abs((int32_t)samples[2*i+1]);

            if (left > CLIPPING_THRESHOLD) {
                isClipping |= (++oversLeft >= CLIPPING_DETECTION);
            } else {
                oversLeft = 0;
            }
            if (right > CLIPPING_THRESHOLD) {
                isClipping |= (++oversRight >= CLIPPING_DETECTION);
            } else {
                oversRight = 0;
            }
        }
    } else {
        int overs = 0;

        for (int i = 0; i < numSamples; i++) {
            int32_t sample = std::abs((int32_t)samples[i]);

            if (sample > CLIPPING_THRESHOLD) {
                isClipping |= (++overs >= CLIPPING_DETECTION);
            } else {
                overs = 0;
            }
        }
    }

    return isClipping;
}

inline float computeLoudness(int16_t* samples, int numSamples) {

    float scale = numSamples ? 1.0f / numSamples : 0.0f;

    int32_t loudness = 0;
    for (int i = 0; i < numSamples; i++) {
        loudness += std::abs((int32_t)samples[i]);
    }
    return (float)loudness * scale;
}

template <int NUM_CHANNELS>
static void applyGainSmoothing(float* buffer, int numFrames, float gain0, float gain1) {

    // fast path for unity gain
    if (gain0 == 1.0f && gain1 == 1.0f) {
        return;
    }

    // cubic poly from gain0 to gain1
    float c3 = -2.0f * (gain1 - gain0);
    float c2 = 3.0f * (gain1 - gain0);
    float c0 = gain0;

    float t = 0.0f;
    float tStep = 1.0f / numFrames;

    for (int i = 0; i < numFrames; i++) {

        // evaluate poly over t=[0,1)
        float gain = (c3 * t + c2) * t * t + c0;
        t += tStep;

        // apply gain to all channels
        for (int ch = 0; ch < NUM_CHANNELS; ch++) {
            buffer[NUM_CHANNELS*i + ch] *= gain;
        }
    }
}

inline inline float convertToFloat(int16_t sample) {
    return (float)sample * (1 / 32768.0f);
}

template<Derived>
Derived& AudioPacketHandler<Derived>::derived() {
    return *static_cast<Derived*>(this);
}

template<Derived>
const Derived& AudioPacketHandler<Derived>::derived() const {
    return *static_cast<const Derived*>(this);
}

template<Derived>
AudioPacketHandler<Derived>::AudioPacketHandler() {

    // set up the desired audio format
    _desiredInputFormat.setSampleRate(AudioConstants::SAMPLE_RATE);
    _desiredInputFormat.setSampleSize(16);
    _desiredInputFormat.setCodec("audio/pcm");
    _desiredInputFormat.setSampleType(QAudioFormat::SignedInt);
    _desiredInputFormat.setByteOrder(QAudioFormat::LittleEndian);
    _desiredInputFormat.setChannelCount(AudioConstants::MONO);

    _desiredOutputFormat = _desiredInputFormat;
    _desiredOutputFormat.setChannelCount(OUTPUT_CHANNEL_COUNT);


    // avoid putting a lock in the device callback
    assert(_localSamplesAvailable.is_lock_free());

    // deprecate legacy settings
    {
        Setting::Handle<int>::Deprecated("maxFramesOverDesired", InboundAudioStream::MAX_FRAMES_OVER_DESIRED);
        Setting::Handle<int>::Deprecated("windowStarveThreshold", InboundAudioStream::WINDOW_STARVE_THRESHOLD);
        Setting::Handle<int>::Deprecated("windowSecondsForDesiredCalcOnTooManyStarves", InboundAudioStream::WINDOW_SECONDS_FOR_DESIRED_CALC_ON_TOO_MANY_STARVES);
        Setting::Handle<int>::Deprecated("windowSecondsForDesiredReduction", InboundAudioStream::WINDOW_SECONDS_FOR_DESIRED_REDUCTION);
        Setting::Handle<bool>::Deprecated("useStDevForJitterCalc", InboundAudioStream::USE_STDEV_FOR_JITTER);
        Setting::Handle<bool>::Deprecated("repetitionWithFade", InboundAudioStream::REPETITION_WITH_FADE);
    }

    derived().connect(&_receivedAudioStream, &MixedProcessedAudioStream::processSamples,
        &derived(), &AudioPacketHandler::processReceivedSamples, Qt::DirectConnection);

    // FIXME: CRTP
    // connect(this, &AudioClient::changeDevice, this, [=](const HifiAudioDeviceInfo& outputDeviceInfo) {
    //     qCDebug(audioclient)<< "got AudioClient::changeDevice signal, about to call switchOutputToAudioDevice() outputDeviceInfo: ["<< outputDeviceInfo.deviceName() << "]";
    //     switchOutputToAudioDevice(outputDeviceInfo);
    // });

    derived().connect(&_receivedAudioStream, &InboundAudioStream::mismatchedAudioCodec, &derived(), &AudioPacketHandler::handleMismatchAudioFormat);

    // FIXME: CRTP
    // initialize wasapi; if getAvailableDevices is called from the CheckDevicesThread before this, it will crash
    // defaultAudioDeviceName(QAudio::AudioInput);
    // defaultAudioDeviceName(QAudio::AudioOutput);

    // FIXME: CRTP
    // start a thread to detect any device changes
    // _checkDevicesTimer = new QTimer(this);
    // const unsigned long DEVICE_CHECK_INTERVAL_MSECS = 2 * 1000;
    // connect(_checkDevicesTimer, &QTimer::timeout, this, [=] {
    //     QtConcurrent::run(QThreadPool::globalInstance(), [=] {
    //         checkDevices();
    //         // On some systems (Ubuntu) checking all the audio devices can take more than 2 seconds.  To
    //         // avoid consuming all of the thread pool, don't start the check interval until the previous
    //         // check has completed.
    //         QMetaObject::invokeMethod(_checkDevicesTimer, "start", Q_ARG(int, DEVICE_CHECK_INTERVAL_MSECS));
    //     });
    // });
    // _checkDevicesTimer->setSingleShot(true);
    // _checkDevicesTimer->start(DEVICE_CHECK_INTERVAL_MSECS);

    // FIXME: CRTP
    // start a thread to detect peak value changes
    // _checkPeakValuesTimer = new QTimer(this);
    // connect(_checkPeakValuesTimer, &QTimer::timeout, this, [this] {
    //     QtConcurrent::run(QThreadPool::globalInstance(), [this] { checkPeakValues(); });
    // });
    // const unsigned long PEAK_VALUES_CHECK_INTERVAL_MSECS = 50;
    // _checkPeakValuesTimer->start(PEAK_VALUES_CHECK_INTERVAL_MSECS);
    //
    // configureReverb();

#if defined(WEBRTC_AUDIO)
    configureWebrtc();
#endif

    auto nodeList = DependencyManager::get<NodeList>();
    auto& packetReceiver = nodeList->getPacketReceiver();
    packetReceiver.registerListener(PacketType::AudioStreamStats,
        PacketReceiver::makeSourcedListenerReference<AudioIOStats>(&_stats, &AudioIOStats::processStreamStatsPacket));
    packetReceiver.registerListener(PacketType::AudioEnvironment,
        PacketReceiver::makeUnsourcedListenerReference<Derived>(&derived(), &AudioPacketHandler::handleAudioEnvironmentDataPacket));
    packetReceiver.registerListener(PacketType::SilentAudioFrame,
        PacketReceiver::makeUnsourcedListenerReference<Derived>(&derived(), &AudioPacketHandler::handleAudioDataPacket));
    packetReceiver.registerListener(PacketType::MixedAudio,
        PacketReceiver::makeUnsourcedListenerReference<Derived>(&derived(), &AudioPacketHandler::handleAudioDataPacket));
    packetReceiver.registerListener(PacketType::NoisyMute,
        PacketReceiver::makeUnsourcedListenerReference<Derived>(&derived(), &AudioPacketHandler::handleNoisyMutePacket));
    packetReceiver.registerListener(PacketType::MuteEnvironment,
        PacketReceiver::makeUnsourcedListenerReference<Derived>(&derived(), &AudioPacketHandler::handleMuteEnvironmentPacket));
    packetReceiver.registerListener(PacketType::SelectedAudioFormat,
        PacketReceiver::makeUnsourcedListenerReference<Derived>(&derived(), &AudioPacketHandler::handleSelectedAudioFormat));

    auto& domainHandler = nodeList->getDomainHandler();
    derived().connect(&domainHandler, &DomainHandler::disconnectedFromDomain, &derived(), [this] {
        _solo.reset();
    });
    derived().connect(nodeList.data(), &NodeList::nodeActivated, &derived(), [this](SharedNodePointer node) {
        if (node->getType() == NodeType::AudioMixer) {
            _solo.resend();
        }
    });
}

template <typename Derived>
AudioPacketHandler<Derived>::~AudioPacketHandler() {

    stop();

    if (_codec && _encoder) {
        _codec->releaseEncoder(_encoder);
        _encoder = nullptr;
    }

    if (_dummyAudioInput) {
        _dummyAudioInput->stop();
        _dummyAudioInput->deleteLater();
        _dummyAudioInput = NULL;
    }
}

// FIXME: CRTP add this to vircadia-client/src/internal/audio/AudioClient
// void AudioClient::customDeleter() {
//     deleteLater();
// }

template <typename Derived>
void AudioPacketHandler<Derived>::handleMismatchAudioFormat(SharedNodePointer node, const QString& currentCodec, const QString& recievedCodec) {
    qCDebug(audioclient) << __FUNCTION__ << "sendingNode:" << *node << "currentCodec:" << currentCodec << "recievedCodec:" << recievedCodec;
    selectAudioFormat(recievedCodec);
}

template <typename Derived>
void AudioPacketHandler<Derived>::reset() {
    _receivedAudioStream.reset();
    _stats.reset();
    _sourceReverb.reset();
    _listenerReverb.reset();
    _localReverb.reset();
}

template <typename Derived>
void AudioPacketHandler<Derived>::audioMixerKilled() {
    _hasReceivedFirstPacket = false;
    _outgoingAvatarAudioSequenceNumber = 0;
    _stats.reset();
    // FIXME: CRTP
    // emit disconnected();
}

template <typename Derived>
void AudioPacketHandler<Derived>::setAudioPaused(bool pause) {
    if (_audioPaused != pause) {
        _audioPaused = pause;

        if (!_audioPaused) {
            negotiateAudioFormat();
        }
    }
}

bool sampleChannelConversion(const int16_t* sourceSamples, int16_t* destinationSamples, int numSourceSamples,
                             const int sourceChannelCount, const int destinationChannelCount) {
    if (sourceChannelCount == 2 && destinationChannelCount == 1) {
        // loop through the stereo input audio samples and average every two samples
        for (int i = 0; i < numSourceSamples; i += 2) {
            destinationSamples[i / 2] = (sourceSamples[i] / 2) + (sourceSamples[i + 1] / 2);
        }

        return true;
    } else if (sourceChannelCount == 1 && destinationChannelCount == 2) {

        // loop through the mono input audio and repeat each sample twice
        for (int i = 0; i < numSourceSamples; ++i) {
            destinationSamples[i * 2] = destinationSamples[(i * 2) + 1] = sourceSamples[i];
        }

        return true;
    }

    return false;
}

int possibleResampling(AudioSRC* resampler,
                       const int16_t* sourceSamples, int16_t* destinationSamples,
                       int numSourceSamples, int maxDestinationSamples,
                       const int sourceChannelCount, const int destinationChannelCount) {

    int numSourceFrames = numSourceSamples / sourceChannelCount;
    int numDestinationFrames = 0;

    if (numSourceSamples > 0) {
        if (!resampler) {
            if (!sampleChannelConversion(sourceSamples, destinationSamples, numSourceSamples,
                                         sourceChannelCount, destinationChannelCount)) {
                // no conversion, we can copy the samples directly across
                memcpy(destinationSamples, sourceSamples, numSourceSamples * AudioConstants::SAMPLE_SIZE);
            }
            numDestinationFrames = numSourceFrames;
        } else {
            if (sourceChannelCount != destinationChannelCount) {

                int16_t* channelConversionSamples = new int16_t[numSourceFrames * destinationChannelCount];

                sampleChannelConversion(sourceSamples, channelConversionSamples, numSourceSamples,
                                        sourceChannelCount, destinationChannelCount);

                numDestinationFrames = resampler->render(channelConversionSamples, destinationSamples, numSourceFrames);

                delete[] channelConversionSamples;
            } else {
                numDestinationFrames = resampler->render(sourceSamples, destinationSamples, numSourceFrames);
            }
        }
    }

    int numDestinationSamples = numDestinationFrames * destinationChannelCount;
    if (numDestinationSamples > maxDestinationSamples) {
        qCWarning(audioclient) << "Resampler overflow! numDestinationSamples =" << numDestinationSamples
                               << "but maxDestinationSamples =" << maxDestinationSamples;
    }
    return numDestinationSamples;
}

template <typename Derived>
void AudioPacketHandler<Derived>::start() {
    //initialize input to the dummy device to prevent starves
    cleanupInput();
    setupDummyInput();

// FIXME: CRTP
// switchOutputToAudioDevice(defaultAudioDeviceForMode(QAudio::AudioOutput, QString()));
// #if defined(Q_OS_ANDROID)
//     connect(&_checkInputTimer, &QTimer::timeout, this, &AudioClient::checkInputTimeout);
//     _checkInputTimer.start(CHECK_INPUT_READS_MSECS);
// #endif
}

template <typename Derived>
void AudioPacketHandler<Derived>::stop() {
    qCDebug(audioclient) << "AudioPacketHandler::stop(), requesting audio input device to shut down";
    cleanupInput();
    qCDebug(audioclient) << "The audio input device has shut down.";


    qCDebug(audioclient) << "AudioPacketHandler::stop(), requesting audio output device to shut down";
    cleanupOutput();
    qCDebug(audioclient) << "The audio output device has shut down.";

    // FIXME: CRTP
    // Stop triggering the checks
    // QObject::disconnect(_checkPeakValuesTimer, &QTimer::timeout, nullptr, nullptr);
    // QObject::disconnect(_checkDevicesTimer, &QTimer::timeout, nullptr, nullptr);
    // Destruction of the pointers will occur when the parent object (this) is destroyed)
    // {
    //     Lock lock(_checkDevicesMutex);
    //     _checkDevicesTimer->stop();
    //     _checkDevicesTimer = nullptr;
    // }
    // {
    //     Lock lock(_checkPeakValuesMutex);
    //     _checkPeakValuesTimer = nullptr;
    // }

// FIXME: CRTP
// #if defined(Q_OS_ANDROID)
//     _checkInputTimer.stop();
//     disconnect(&_checkInputTimer, &QTimer::timeout, 0, 0);
// #endif
}

template <typename Derived>
void AudioPacketHandler<Derived>::handleAudioEnvironmentDataPacket(QSharedPointer<ReceivedMessage> message) {
    char bitset;
    message->readPrimitive(&bitset);

    bool hasReverb = oneAtBit(bitset, HAS_REVERB_BIT);

    if (hasReverb) {
        float reverbTime, wetLevel;
        message->readPrimitive(&reverbTime);
        message->readPrimitive(&wetLevel);
        _receivedAudioStream.setReverb(reverbTime, wetLevel);
    } else {
        _receivedAudioStream.clearReverb();
    }
}

template <typename Derived>
void AudioPacketHandler<Derived>::handleAudioDataPacket(QSharedPointer<ReceivedMessage> message) {
    if (message->getType() == PacketType::SilentAudioFrame) {
        _silentInbound.increment();
    } else {
        _audioInbound.increment();
    }

    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::ReceiveFirstAudioPacket);

    if (!_audioOutputInitialized.load(std::memory_order_acquire)) {

        if (!_hasReceivedFirstPacket) {
            _hasReceivedFirstPacket = true;

            // have the audio scripting interface emit a signal to say we just connected to mixer
            // FIXME: CRTP
            // emit receivedFirstPacket();
        }

#if DEV_BUILD || PR_BUILD
        _gate.insert(message);
#else
        // Audio output must exist and be correctly set up if we're going to process received audio
        _receivedAudioStream.parseData(*message);
#endif
    }
}

template <typename Derived>
AudioPacketHandler<Derived>::Gate::Gate(AudioPacketHandler* audioClient) :
    _audioClient(audioClient) {}

template <typename Derived>
void AudioPacketHandler<Derived>::Gate::setIsSimulatingJitter(bool enable) {
    std::lock_guard<std::mutex> lock(_mutex);
    flush();
    _isSimulatingJitter = enable;
}

template <typename Derived>
void AudioPacketHandler<Derived>::Gate::setThreshold(int threshold) {
    std::lock_guard<std::mutex> lock(_mutex);
    flush();
    _threshold = std::max(threshold, 1);
}

template <typename Derived>
void AudioPacketHandler<Derived>::Gate::insert(QSharedPointer<ReceivedMessage> message) {
    std::lock_guard<std::mutex> lock(_mutex);

    // Short-circuit for normal behavior
    if (_threshold == 1 && !_isSimulatingJitter) {
        _audioClient->_receivedAudioStream.parseData(*message);
        return;
    }

    // Throttle the current packet until the next flush
    _queue.push(message);
    _index++;

    // When appropriate, flush all held packets to the received audio stream
    if (_isSimulatingJitter) {
        // The JITTER_FLUSH_CHANCE defines the discrete probability density function of jitter (ms),
        // where f(t) = pow(1 - JITTER_FLUSH_CHANCE, (t / 10) * JITTER_FLUSH_CHANCE
        // for t (ms) = 10, 20, ... (because typical packet timegap is 10ms),
        // because there is a JITTER_FLUSH_CHANCE of any packet instigating a flush of all held packets.
        static const float JITTER_FLUSH_CHANCE = 0.6f;
        // It is set at 0.6 to give a low chance of spikes (>30ms, 2.56%) so that they are obvious,
        // but settled within the measured 5s window in audio network stats.
        if (randFloat() < JITTER_FLUSH_CHANCE) {
            flush();
        }
    } else if (!(_index % _threshold)) {
        flush();
    }
}

template <typename Derived>
void AudioPacketHandler<Derived>::Gate::flush() {
    // Send all held packets to the received audio stream to be (eventually) played
    while (!_queue.empty()) {
        _audioClient->_receivedAudioStream.parseData(*_queue.front());
        _queue.pop();
    }
    _index = 0;
}


template <typename Derived>
void AudioPacketHandler<Derived>::handleNoisyMutePacket(QSharedPointer<ReceivedMessage> message) {
    if (!_isMuted) {
        setMuted(true);

        // have the audio scripting interface emit a signal to say we were muted by the mixer
        // FIXME: CRTP
        // emit mutedByMixer();
    }
}

template <typename Derived>
void AudioPacketHandler<Derived>::handleMuteEnvironmentPacket(QSharedPointer<ReceivedMessage> message) {
    glm::vec3 position;
    float radius;

    message->readPrimitive(&position);
    message->readPrimitive(&radius);

    // FIXME: CRTP
    // emit muteEnvironmentRequested(position, radius);
}

template <typename Derived>
void AudioPacketHandler<Derived>::negotiateAudioFormat() {
    auto nodeList = DependencyManager::get<NodeList>();
    auto negotiateFormatPacket = NLPacket::create(PacketType::NegotiateAudioFormat);
    std::vector<std::shared_ptr<Codec>> codecs;
    // FIXME: CRTP
    // const auto& codecPlugins = PluginManager::getInstance()->getCodecPlugins();
    quint8 numberOfCodecs = (quint8)codecs.size();
    negotiateFormatPacket->writePrimitive(numberOfCodecs);
    for (const auto& codec : codecs) {
        auto codecName = codec->getName();
        negotiateFormatPacket->writeString(codecName);
    }

    // grab our audio mixer from the NodeList, if it exists
    SharedNodePointer audioMixer = nodeList->soloNodeOfType(NodeType::AudioMixer);

    if (audioMixer) {
        // send off this mute packet
        nodeList->sendPacket(std::move(negotiateFormatPacket), *audioMixer);
    }
}

template <typename Derived>
void AudioPacketHandler<Derived>::handleSelectedAudioFormat(QSharedPointer<ReceivedMessage> message) {
    QString selectedCodecName = message->readString();
    selectAudioFormat(selectedCodecName);
}

template <typename Derived>
void AudioPacketHandler<Derived>::selectAudioFormat(const QString& selectedCodecName) {

    _selectedCodecName = selectedCodecName;

    qCDebug(audioclient) << "Selected codec:" << _selectedCodecName << "; Is stereo input:" << (_desiredInputFormat.channelCount() == AudioConstants::STEREO);

    // release any old codec encoder/decoder first...
    if (_codec && _encoder) {
        _codec->releaseEncoder(_encoder);
        _encoder = nullptr;
        _codec = nullptr;
    }
    _receivedAudioStream.cleanupCodec();

    std::vector<std::shared_ptr<Codec>> codecs;
    // FIXME: CRTP
    // const auto& codecPlugins = PluginManager::getInstance()->getCodecPlugins();
    for (const auto& codec : codecs) {
        if (_selectedCodecName == codec->getName()) {
            _codec = codec;
            _receivedAudioStream.setupCodec(codec, _selectedCodecName, AudioConstants::STEREO);
            _encoder = codec->createEncoder(_desiredInputFormat.sampleRate(), _desiredInputFormat.channelCount());
            qCDebug(audioclient) << "Selected codec:" << _codec.get();
            break;
        }
    }

}

template <typename Derived>
void AudioPacketHandler<Derived>::configureReverb() {
    ReverbParameters p;

    p.sampleRate = AudioConstants::SAMPLE_RATE;
    p.bandwidth = _reverbOptions->getBandwidth();
    p.preDelay = _reverbOptions->getPreDelay();
    p.lateDelay = _reverbOptions->getLateDelay();
    p.reverbTime = _reverbOptions->getReverbTime();
    p.earlyDiffusion = _reverbOptions->getEarlyDiffusion();
    p.lateDiffusion = _reverbOptions->getLateDiffusion();
    p.roomSize = _reverbOptions->getRoomSize();
    p.density = _reverbOptions->getDensity();
    p.bassMult = _reverbOptions->getBassMult();
    p.bassFreq = _reverbOptions->getBassFreq();
    p.highGain = _reverbOptions->getHighGain();
    p.highFreq = _reverbOptions->getHighFreq();
    p.modRate = _reverbOptions->getModRate();
    p.modDepth = _reverbOptions->getModDepth();
    p.earlyGain = _reverbOptions->getEarlyGain();
    p.lateGain = _reverbOptions->getLateGain();
    p.earlyMixLeft = _reverbOptions->getEarlyMixLeft();
    p.earlyMixRight = _reverbOptions->getEarlyMixRight();
    p.lateMixLeft = _reverbOptions->getLateMixLeft();
    p.lateMixRight = _reverbOptions->getLateMixRight();
    p.wetDryMix = _reverbOptions->getWetDryMix();

    _listenerReverb.setParameters(&p);
    _localReverb.setParameters(&p);

    // used only for adding self-reverb to loopback audio
    p.sampleRate = _outputFormat.sampleRate();
    p.wetDryMix = 100.0f;
    p.preDelay = 0.0f;
    p.earlyGain = -96.0f;   // disable ER
    p.lateGain += _reverbOptions->getWetDryMix() * (24.0f / 100.0f) - 24.0f;  // -0dB to -24dB, based on wetDryMix
    p.lateMixLeft = 0.0f;
    p.lateMixRight = 0.0f;

    _sourceReverb.setParameters(&p);
}

template <typename Derived>
void AudioPacketHandler<Derived>::updateReverbOptions() {
    bool reverbChanged = false;
    if (_receivedAudioStream.hasReverb()) {

        if (_zoneReverbOptions.getReverbTime() != _receivedAudioStream.getRevebTime()) {
            _zoneReverbOptions.setReverbTime(_receivedAudioStream.getRevebTime());
            reverbChanged = true;
        }
        if (_zoneReverbOptions.getWetDryMix() != _receivedAudioStream.getWetLevel()) {
            _zoneReverbOptions.setWetDryMix(_receivedAudioStream.getWetLevel());
            reverbChanged = true;
        }

        if (_reverbOptions != &_zoneReverbOptions) {
            _reverbOptions = &_zoneReverbOptions;
            reverbChanged = true;
        }
    } else if (_reverbOptions != &_scriptReverbOptions) {
        _reverbOptions = &_scriptReverbOptions;
        reverbChanged = true;
    }

    if (reverbChanged) {
        configureReverb();
    }
}

template <typename Derived>
void AudioPacketHandler<Derived>::setReverb(bool reverb) {
    _reverb = reverb;

    if (!_reverb) {
        _sourceReverb.reset();
        _listenerReverb.reset();
        _localReverb.reset();
    }
}

template <typename Derived>
void AudioPacketHandler<Derived>::setReverbOptions(const AudioEffectOptions* options) {
    // Save the new options
    _scriptReverbOptions.setBandwidth(options->getBandwidth());
    _scriptReverbOptions.setPreDelay(options->getPreDelay());
    _scriptReverbOptions.setLateDelay(options->getLateDelay());
    _scriptReverbOptions.setReverbTime(options->getReverbTime());
    _scriptReverbOptions.setEarlyDiffusion(options->getEarlyDiffusion());
    _scriptReverbOptions.setLateDiffusion(options->getLateDiffusion());
    _scriptReverbOptions.setRoomSize(options->getRoomSize());
    _scriptReverbOptions.setDensity(options->getDensity());
    _scriptReverbOptions.setBassMult(options->getBassMult());
    _scriptReverbOptions.setBassFreq(options->getBassFreq());
    _scriptReverbOptions.setHighGain(options->getHighGain());
    _scriptReverbOptions.setHighFreq(options->getHighFreq());
    _scriptReverbOptions.setModRate(options->getModRate());
    _scriptReverbOptions.setModDepth(options->getModDepth());
    _scriptReverbOptions.setEarlyGain(options->getEarlyGain());
    _scriptReverbOptions.setLateGain(options->getLateGain());
    _scriptReverbOptions.setEarlyMixLeft(options->getEarlyMixLeft());
    _scriptReverbOptions.setEarlyMixRight(options->getEarlyMixRight());
    _scriptReverbOptions.setLateMixLeft(options->getLateMixLeft());
    _scriptReverbOptions.setLateMixRight(options->getLateMixRight());
    _scriptReverbOptions.setWetDryMix(options->getWetDryMix());

    if (_reverbOptions == &_scriptReverbOptions) {
        // Apply them to the reverb instances
        configureReverb();
    }
}

#if defined(WEBRTC_AUDIO)

static void deinterleaveToFloat(const int16_t* src, float* const* dst, int numFrames, int numChannels) {
    for (int i = 0; i < numFrames; i++) {
        for (int ch = 0; ch < numChannels; ch++) {
            float f = *src++;
            f *= (1/32768.0f);  // scale
            dst[ch][i] = f;     // deinterleave
        }
    }
}

static void interleaveToInt16(const float* const* src, int16_t* dst, int numFrames, int numChannels) {
    for (int i = 0; i < numFrames; i++) {
        for (int ch = 0; ch < numChannels; ch++) {
            float f = src[ch][i];
            f *= 32768.0f;                                  // scale
            f += (f < 0.0f) ? -0.5f : 0.5f;                 // round
            f = std::max(std::min(f, 32767.0f), -32768.0f); // saturate
            *dst++ = (int16_t)f;                            // interleave
        }
    }
}

template <typename Derived>
void AudioPacketHandler<Derived>::configureWebrtc() {
    _apm = webrtc::AudioProcessingBuilder().Create();

    webrtc::AudioProcessing::Config config;

    config.pre_amplifier.enabled = false;
    config.high_pass_filter.enabled = false;
    config.echo_canceller.enabled = true;
    config.echo_canceller.mobile_mode = false;
#if defined(WEBRTC_LEGACY)
    config.echo_canceller.use_legacy_aec = false;
#endif
    config.noise_suppression.enabled = false;
    config.noise_suppression.level = webrtc::AudioProcessing::Config::NoiseSuppression::kModerate;
    config.voice_detection.enabled = false;
    config.gain_controller1.enabled = false;
    config.gain_controller2.enabled = false;
    config.gain_controller2.fixed_digital.gain_db = 0.0f;
    config.gain_controller2.adaptive_digital.enabled = false;
    config.residual_echo_detector.enabled = true;
    config.level_estimation.enabled = false;

    _apm->ApplyConfig(config);
}

// rebuffer into 10ms chunks
template <typename Derived>
void AudioPacketHandler<Derived>::processWebrtcFarEnd(const int16_t* samples, int numFrames, int numChannels, int sampleRate) {

    const webrtc::StreamConfig streamConfig = webrtc::StreamConfig(sampleRate, numChannels);
    const int numChunk = (int)streamConfig.num_frames();

    static int32_t lastWarningHash = 0;
    if (sampleRate > WEBRTC_SAMPLE_RATE_MAX || numChannels > WEBRTC_CHANNELS_MAX) {
        if (lastWarningHash != ((sampleRate << 8) | numChannels)) {
            lastWarningHash = ((sampleRate << 8) | numChannels);
            qCWarning(audioclient) << "AEC not unsupported for output format: sampleRate =" << sampleRate << "numChannels =" << numChannels;
        }
        return;
    }

    while (numFrames > 0) {

        // number of frames to fill
        int numFill = std::min(numFrames, numChunk - _numFifoFarEnd);

        // refill fifo
        memcpy(&_fifoFarEnd[_numFifoFarEnd], samples, numFill * numChannels * sizeof(int16_t));
        samples += numFill * numChannels;
        numFrames -= numFill;
        _numFifoFarEnd += numFill;

        if (_numFifoFarEnd == numChunk) {

            // convert audio format
            float buffer[WEBRTC_CHANNELS_MAX][WEBRTC_FRAMES_MAX];
            float* const buffers[WEBRTC_CHANNELS_MAX] = { buffer[0], buffer[1] };
            deinterleaveToFloat(_fifoFarEnd, buffers, numChunk, numChannels);

            // process one chunk
            int error = _apm->ProcessReverseStream(buffers, streamConfig, streamConfig, buffers);
            if (error != _apm->kNoError) {
                qCWarning(audioclient) << "WebRTC ProcessReverseStream() returned ERROR:" << error;
            }
            _numFifoFarEnd = 0;
        }
    }
}

template <typename Derived>
void AudioPacketHandler<Derived>::processWebrtcNearEnd(int16_t* samples, int numFrames, int numChannels, int sampleRate) {

    const webrtc::StreamConfig streamConfig = webrtc::StreamConfig(sampleRate, numChannels);
    assert(numFrames == (int)streamConfig.num_frames());    // WebRTC requires exactly 10ms of input

    static int32_t lastWarningHash = 0;
    if (sampleRate > WEBRTC_SAMPLE_RATE_MAX || numChannels > WEBRTC_CHANNELS_MAX) {
        if (lastWarningHash != ((sampleRate << 8) | numChannels)) {
            lastWarningHash = ((sampleRate << 8) | numChannels);
            qCWarning(audioclient) << "AEC not unsupported for input format: sampleRate =" << sampleRate << "numChannels =" << numChannels;
        }
        return;
    }

    // convert audio format
    float buffer[WEBRTC_CHANNELS_MAX][WEBRTC_FRAMES_MAX];
    float* const buffers[WEBRTC_CHANNELS_MAX] = { buffer[0], buffer[1] };
    deinterleaveToFloat(samples, buffers, numFrames, numChannels);

    // process one chunk
    int error = _apm->ProcessStream(buffers, streamConfig, streamConfig, buffers);
    if (error != _apm->kNoError) {
        qCWarning(audioclient) << "WebRTC ProcessStream() returned ERROR:" << error;
    } else {
        // modify samples in-place
        interleaveToInt16(buffers, samples, numFrames, numChannels);
    }
}

#endif // WEBRTC_AUDIO

template <typename Derived>
float AudioPacketHandler<Derived>::loudnessToLevel(float loudness) {
    float level = loudness * (1 / 32768.0f);  // level in [0, 1]
    level = 6.02059991f * fastLog2f(level);   // convert to dBFS
    level = (level + 48.0f) * (1 / 42.0f);    // map [-48, -6] dBFS to [0, 1]
    return glm::clamp(level, 0.0f, 1.0f);
}

template <typename Derived>
void AudioPacketHandler<Derived>::handleAudioInput(QByteArray& audioBuffer) {
    if (!_audioPaused) {

        bool audioGateOpen = false;

        if (!_isMuted) {
            int16_t* samples = reinterpret_cast<int16_t*>(audioBuffer.data());
            int numSamples = audioBuffer.size() / AudioConstants::SAMPLE_SIZE;
            int numFrames = numSamples / _desiredInputFormat.channelCount();

            if (_isNoiseGateEnabled && _isNoiseReductionAutomatic) {
                // The audio gate includes DC removal
                audioGateOpen = _audioGate->render(samples, samples, numFrames);
            } else if (_isNoiseGateEnabled && !_isNoiseReductionAutomatic &&
                       loudnessToLevel(_lastSmoothedRawInputLoudness) >= _noiseReductionThreshold) {
                audioGateOpen = _audioGate->removeDC(samples, samples, numFrames);
            } else if (_isNoiseGateEnabled && !_isNoiseReductionAutomatic) {
                audioGateOpen = false;
            } else {
                audioGateOpen = _audioGate->removeDC(samples, samples, numFrames);
            }

            // FIXME: CRTP
            // emit inputReceived(audioBuffer);
        }

        // loudness after mute/gate
        _lastInputLoudness = (_isMuted || !audioGateOpen) ? 0.0f : _lastRawInputLoudness;

        // detect gate opening and closing
        bool openedInLastBlock = !_audioGateOpen && audioGateOpen;  // the gate just opened
        bool closedInLastBlock = _audioGateOpen && !audioGateOpen;  // the gate just closed
        _audioGateOpen = audioGateOpen;

        if (openedInLastBlock) {
            //FIXME: CRTP
            // emit noiseGateOpened();
        } else if (closedInLastBlock) {
            //FIXME: CRTP
            // emit noiseGateClosed();
        }

        // the codec must be flushed to silence before sending silent packets,
        // so delay the transition to silent packets by one packet after becoming silent.
        auto packetType = _shouldEchoToServer ? PacketType::MicrophoneAudioWithEcho : PacketType::MicrophoneAudioNoEcho;
        if (!audioGateOpen && !closedInLastBlock) {
            packetType = PacketType::SilentAudioFrame;
            _silentOutbound.increment();
        } else {
            _audioOutbound.increment();
        }

        QByteArray encodedBuffer;
        if (_encoder) {
            _encoder->encode(audioBuffer, encodedBuffer);
        } else {
            encodedBuffer = audioBuffer;
        }

        AbstractAudioInterface::emitAudioPacket(encodedBuffer.data(), encodedBuffer.size(), _outgoingAvatarAudioSequenceNumber, _desiredInputFormat.channelCount() == AudioConstants::STEREO,
                        {_positionGetter(), _orientationGetter()}, avatarBoundingBoxCorner, avatarBoundingBoxScale,
                        packetType, _selectedCodecName);
        _stats.sentPacket();
    }
}

template <typename Derived>
void AudioPacketHandler<Derived>::handleMicAudioInput() {
    if (!_inputDevice || _isPlayingBackRecording) {
        return;
    }

    // FIXME: CRTP
// #if defined(Q_OS_ANDROID)
//     _inputReadsSinceLastCheck++;
// #endif

    // input samples required to produce exactly NETWORK_FRAME_SAMPLES of output
    const int inputSamplesRequired = (_inputToNetworkResampler ?
                                      _inputToNetworkResampler->getMinInput(AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL) :
                                      AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL) * _inputFormat.channelCount();

    const auto inputAudioSamples = std::unique_ptr<int16_t[]>(new int16_t[inputSamplesRequired]);
    QByteArray inputByteArray = _inputDevice->readAll();

    // FIXME: CRTP
    // handleLocalEchoAndReverb(inputByteArray);

    _inputRingBuffer.writeData(inputByteArray.data(), inputByteArray.size());

    float audioInputMsecsRead = inputByteArray.size() / (float)(_inputFormat.bytesForDuration(USECS_PER_MSEC));
    _stats.updateInputMsRead(audioInputMsecsRead);

    const int numNetworkBytes = _desiredInputFormat.channelCount() == AudioConstants::STEREO
        ? AudioConstants::NETWORK_FRAME_BYTES_STEREO
        : AudioConstants::NETWORK_FRAME_BYTES_PER_CHANNEL;
    const int numNetworkSamples = _desiredInputFormat.channelCount() == AudioConstants::STEREO
        ? AudioConstants::NETWORK_FRAME_SAMPLES_STEREO
        : AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL;

    static int16_t networkAudioSamples[AudioConstants::NETWORK_FRAME_SAMPLES_STEREO];

    while (_inputRingBuffer.samplesAvailable() >= inputSamplesRequired) {

        _inputRingBuffer.readSamples(inputAudioSamples.get(), inputSamplesRequired);

        // detect clipping on the raw input
        bool isClipping = detectClipping(inputAudioSamples.get(), inputSamplesRequired, _inputFormat.channelCount());
        if (isClipping) {
            _timeSinceLastClip = 0.0f;
        } else if (_timeSinceLastClip >= 0.0f) {
            _timeSinceLastClip += AudioConstants::NETWORK_FRAME_SECS;
        }
        isClipping = (_timeSinceLastClip >= 0.0f) && (_timeSinceLastClip < 2.0f);  // 2 second hold time

#if defined(WEBRTC_AUDIO)
        if (_isAECEnabled) {
            processWebrtcNearEnd(inputAudioSamples.get(), inputSamplesRequired / _inputFormat.channelCount(),
                                 _inputFormat.channelCount(), _inputFormat.sampleRate());
        }
#endif

        float loudness = computeLoudness(inputAudioSamples.get(), inputSamplesRequired);
        _lastRawInputLoudness = loudness;

        // envelope detection
        float tc = (loudness > _lastSmoothedRawInputLoudness) ? 0.378f : 0.967f;  // 10ms attack, 300ms release @ 100Hz
        loudness += tc * (_lastSmoothedRawInputLoudness - loudness);
        _lastSmoothedRawInputLoudness = loudness;

        // FIXME: CRTP
        // emit inputLoudnessChanged(_lastSmoothedRawInputLoudness, isClipping);

        if (!_isMuted) {
            possibleResampling(_inputToNetworkResampler,
                inputAudioSamples.get(), networkAudioSamples,
                inputSamplesRequired, numNetworkSamples,
                _inputFormat.channelCount(), _desiredInputFormat.channelCount());
        }
        int bytesInInputRingBuffer = _inputRingBuffer.samplesAvailable() * AudioConstants::SAMPLE_SIZE;
        float msecsInInputRingBuffer = bytesInInputRingBuffer / (float)(_inputFormat.bytesForDuration(USECS_PER_MSEC));
        _stats.updateInputMsUnplayed(msecsInInputRingBuffer);

        QByteArray audioBuffer(reinterpret_cast<char*>(networkAudioSamples), numNetworkBytes);
        handleAudioInput(audioBuffer);
    }
}

template <typename Derived>
void AudioPacketHandler<Derived>::handleRecordedAudioInput(const QByteArray& audio) {
    QByteArray audioBuffer(audio);
    handleAudioInput(audioBuffer);
}

template <typename Derived>
void AudioPacketHandler<Derived>::prepareLocalAudioInjectors(std::unique_ptr<Lock> localAudioLock) {
    bool doSynchronously = localAudioLock.operator bool();
    if (!localAudioLock) {
        localAudioLock.reset(new Lock(_localAudioMutex));
    }

    int samplesNeeded = std::numeric_limits<int>::max();
    while (samplesNeeded > 0) {
        if (!doSynchronously) {
            // unlock between every write to allow device switching
            localAudioLock->unlock();
            localAudioLock->lock();
        }

        // in case of a device switch, consider bufferCapacity volatile across iterations
        if (_outputPeriod == 0) {
            return;
        }

        int bufferCapacity = _localInjectorsStream.getSampleCapacity();
        int maxOutputSamples = AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL * AudioConstants::STEREO;
        if (_localToOutputResampler) {
            maxOutputSamples =
                _localToOutputResampler->getMaxOutput(AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL) *
                AudioConstants::STEREO;
        }

        samplesNeeded = bufferCapacity - _localSamplesAvailable.load(std::memory_order_relaxed);
        if (samplesNeeded < maxOutputSamples) {
            // avoid overwriting the buffer to prevent losing frames
            break;
        }

        // get a network frame of local injectors' audio
        if (!mixLocalAudioInjectors(_localMixBuffer)) {
            break;
        }

        // reverb
        if (_reverb) {
            _localReverb.render(_localMixBuffer, _localMixBuffer, AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);
        }

        int samples;
        if (_localToOutputResampler) {
            // resample to output sample rate
            int frames = _localToOutputResampler->render(_localMixBuffer, _localOutputMixBuffer,
                AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

            // write to local injectors' ring buffer
            samples = frames * AudioConstants::STEREO;
            _localInjectorsStream.writeSamples(_localOutputMixBuffer, samples);

        } else {
            // write to local injectors' ring buffer
            samples = AudioConstants::NETWORK_FRAME_SAMPLES_STEREO;
            _localInjectorsStream.writeSamples(_localMixBuffer,
                AudioConstants::NETWORK_FRAME_SAMPLES_STEREO);
        }

        _localSamplesAvailable.fetch_add(samples, std::memory_order_release);
        samplesNeeded -= samples;
    }
}

template <typename Derived>
bool AudioPacketHandler<Derived>::mixLocalAudioInjectors(float* mixBuffer) {
    // check the flag for injectors before attempting to lock
    if (!_localInjectorsAvailable.load(std::memory_order_acquire)) {
        return false;
    }

    // lock the injectors
    Lock lock(_injectorsMutex);

    QVector<AudioInjectorPointer> injectorsToRemove;

    memset(mixBuffer, 0, AudioConstants::NETWORK_FRAME_SAMPLES_STEREO * sizeof(float));

    for (const AudioInjectorPointer& injector : _activeLocalAudioInjectors) {
        // the lock guarantees that injectorBuffer, if found, is invariant
        auto injectorBuffer = injector->getLocalBuffer();
        if (injectorBuffer) {

            auto options = injector->getOptions();

            static const int HRTF_DATASET_INDEX = 1;

            int numChannels = options.ambisonic ? AudioConstants::AMBISONIC : (options.stereo ? AudioConstants::STEREO : AudioConstants::MONO);
            size_t bytesToRead = numChannels * AudioConstants::NETWORK_FRAME_BYTES_PER_CHANNEL;

            // get one frame from the injector
            memset(_localScratchBuffer, 0, bytesToRead);
            if (0 < injectorBuffer->readData((char*)_localScratchBuffer, bytesToRead)) {

                bool isSystemSound = !options.positionSet && !options.ambisonic;

                float gain = options.volume * (isSystemSound ? _systemInjectorGain : _localInjectorGain);

                if (options.ambisonic) {

                    if (options.positionSet) {

                        // distance attenuation
                        glm::vec3 relativePosition = options.position - _positionGetter();
                        float distance = glm::max(glm::length(relativePosition), EPSILON);
                        gain = gainForSource(distance, gain);
                    }

                    //
                    // Calculate the soundfield orientation relative to the listener.
                    // Injector orientation can be used to align a recording to our world coordinates.
                    //
                    glm::quat relativeOrientation = options.orientation * glm::inverse(_orientationGetter());

                    // convert from Y-up (OpenGL) to Z-up (Ambisonic) coordinate system
                    float qw = relativeOrientation.w;
                    float qx = -relativeOrientation.z;
                    float qy = -relativeOrientation.x;
                    float qz = relativeOrientation.y;

                    // spatialize into mixBuffer
                    injector->getLocalFOA().render(_localScratchBuffer, mixBuffer, HRTF_DATASET_INDEX,
                                                   qw, qx, qy, qz, gain, AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);
                } else if (options.stereo) {

                    if (options.positionSet) {

                        // distance attenuation
                        glm::vec3 relativePosition = options.position - _positionGetter();
                        float distance = glm::max(glm::length(relativePosition), EPSILON);
                        gain = gainForSource(distance, gain);
                    }

                    // direct mix into mixBuffer
                    injector->getLocalHRTF().mixStereo(_localScratchBuffer, mixBuffer, gain,
                                                       AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);
                } else {  // injector is mono

                    if (options.positionSet) {

                        // distance attenuation
                        glm::vec3 relativePosition = options.position - _positionGetter();
                        float distance = glm::max(glm::length(relativePosition), EPSILON);
                        gain = gainForSource(distance, gain);

                        float azimuth = azimuthForSource(relativePosition);

                        // spatialize into mixBuffer
                        injector->getLocalHRTF().render(_localScratchBuffer, mixBuffer, HRTF_DATASET_INDEX,
                                                        azimuth, distance, gain, AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);
                    } else {

                        // direct mix into mixBuffer
                        injector->getLocalHRTF().mixMono(_localScratchBuffer, mixBuffer, gain,
                                                         AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);
                    }
                }

            } else {

                //qCDebug(audioclient) << "injector has no more data, marking finished for removal";
                injector->finishLocalInjection();
                injectorsToRemove.append(injector);
            }

        } else {

            //qCDebug(audioclient) << "injector has no local buffer, marking as finished for removal";
            injector->finishLocalInjection();
            injectorsToRemove.append(injector);
        }
    }

    for (const AudioInjectorPointer& injector : injectorsToRemove) {
        //qCDebug(audioclient) << "removing injector";
        _activeLocalAudioInjectors.removeOne(injector);
    }

    // update the flag
    _localInjectorsAvailable.exchange(!_activeLocalAudioInjectors.empty(), std::memory_order_release);

    return true;
}

template <typename Derived>
void AudioPacketHandler<Derived>::processReceivedSamples(const QByteArray& decodedBuffer, QByteArray& outputBuffer) {

    const int16_t* decodedSamples = reinterpret_cast<const int16_t*>(decodedBuffer.data());
    assert(decodedBuffer.size() == AudioConstants::NETWORK_FRAME_BYTES_STEREO);

    outputBuffer.resize(_outputFrameSize * AudioConstants::SAMPLE_SIZE);
    int16_t* outputSamples = reinterpret_cast<int16_t*>(outputBuffer.data());

    bool hasReverb = _reverb || _receivedAudioStream.hasReverb();

    // apply stereo reverb
    if (hasReverb) {
        updateReverbOptions();
        int16_t* reverbSamples = _networkToOutputResampler ? _networkScratchBuffer : outputSamples;
        _listenerReverb.render(decodedSamples, reverbSamples, AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);
    }

    // resample to output sample rate
    if (_networkToOutputResampler) {
        const int16_t* inputSamples = hasReverb ? _networkScratchBuffer : decodedSamples;
        _networkToOutputResampler->render(inputSamples, outputSamples, AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);
    }

    // if no transformations were applied, we still need to copy the buffer
    if (!hasReverb && !_networkToOutputResampler) {
        memcpy(outputSamples, decodedSamples, decodedBuffer.size());
    }
}

template <typename Derived>
void AudioPacketHandler<Derived>::sendMuteEnvironmentPacket() {
    auto nodeList = DependencyManager::get<NodeList>();

    int dataSize = sizeof(glm::vec3) + sizeof(float);

    auto mutePacket = NLPacket::create(PacketType::MuteEnvironment, dataSize);

    const float MUTE_RADIUS = 50;

    glm::vec3 currentSourcePosition = _positionGetter();

    mutePacket->writePrimitive(currentSourcePosition);
    mutePacket->writePrimitive(MUTE_RADIUS);

    // grab our audio mixer from the NodeList, if it exists
    SharedNodePointer audioMixer = nodeList->soloNodeOfType(NodeType::AudioMixer);

    if (audioMixer) {
        // send off this mute packet
        nodeList->sendPacket(std::move(mutePacket), *audioMixer);
    }
}

template <typename Derived>
void AudioPacketHandler<Derived>::setMuted(bool muted, bool emitSignal) {
    if (_isMuted != muted) {
        _isMuted = muted;
        if (emitSignal) {
            // FIXME: CRTP
            // emit muteToggled(_isMuted);
        }
    }
}

template <typename Derived>
void AudioPacketHandler<Derived>::setNoiseReduction(bool enable, bool emitSignal) {
    if (_isNoiseGateEnabled != enable) {
        _isNoiseGateEnabled = enable;
        if (emitSignal) {
            // FIXME: CRTP
            // emit noiseReductionChanged(_isNoiseGateEnabled);
        }
    }
}

template <typename Derived>
void AudioPacketHandler<Derived>::setNoiseReductionAutomatic(bool enable, bool emitSignal) {
    if (_isNoiseReductionAutomatic != enable) {
        _isNoiseReductionAutomatic = enable;
        if (emitSignal) {
            // FIXME: CRTP
            // emit noiseReductionAutomaticChanged(_isNoiseReductionAutomatic);
        }
    }
}

template <typename Derived>
void AudioPacketHandler<Derived>::setNoiseReductionThreshold(float threshold, bool emitSignal) {
    if (_noiseReductionThreshold != threshold) {
        _noiseReductionThreshold = threshold;
        if (emitSignal) {
            // FIXME: CRTP
            // emit noiseReductionThresholdChanged(_noiseReductionThreshold);
        }
    }
}

template <typename Derived>
void AudioPacketHandler<Derived>::setAcousticEchoCancellation(bool enable, bool emitSignal) {
    if (_isAECEnabled != enable) {
        _isAECEnabled = enable;
        if (emitSignal) {
            // FIXME: CRTP
            // emit acousticEchoCancellationChanged(_isAECEnabled);
        }
    }
}

template <typename Derived>
bool AudioPacketHandler<Derived>::outputLocalInjector(const AudioInjectorPointer& injector) {
    auto injectorBuffer = injector->getLocalBuffer();
    if (injectorBuffer) {
        // local injectors are on the AudioInjectorsThread, so we must guard access
        Lock lock(_injectorsMutex);
        if (!_activeLocalAudioInjectors.contains(injector)) {
            //qCDebug(audioclient) << "adding new injector";
            _activeLocalAudioInjectors.append(injector);

            // update the flag
            _localInjectorsAvailable.exchange(true, std::memory_order_release);
        } else {
            qCDebug(audioclient) << "injector exists in active list already";
        }

        return true;

    } else {
        // no local buffer
        return false;
    }
}

template <typename Derived>
int AudioPacketHandler<Derived>::getNumLocalInjectors() {
    Lock lock(_injectorsMutex);
    return _activeLocalAudioInjectors.size();
}

template <typename Derived>
void AudioPacketHandler<Derived>::outputFormatChanged() {
    _outputFrameSize = (AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL * OUTPUT_CHANNEL_COUNT * _outputFormat.sampleRate()) /
        _desiredOutputFormat.sampleRate();
    _receivedAudioStream.outputFormatChanged(_outputFormat.sampleRate(), OUTPUT_CHANNEL_COUNT);

    _audioOutputIODevice.open();

    // FIXEM: CRTP
    // // setup our general output device for audio-mixer audio
    // _audioOutput = new QAudioOutput(_outputDeviceInfo.getDevice(), _outputFormat, this);
    //
    // _audioOutput->setBufferSize(_outputPeriod * 16); // magic number introduced with commit 47af406440daaccdc48bf01cb3669233a940f811
    //
    // connect(_audioOutput, &QAudioOutput::notify, this, &AudioClient::outputNotify);
    //
    // // start the output device
    // _audioOutput->start(&_audioOutputIODevice);
    //
    // // initialize mix buffers
    //
    // // restrict device callback to _outputPeriod samples
    // _outputPeriod = _audioOutput->periodSize() / AudioConstants::SAMPLE_SIZE;
    // // device callback may exceed reported period, so double it to avoid stutter
    // _outputPeriod *= 2;

}

template <typename Derived>
void AudioPacketHandler<Derived>::cleanupInput() {

// FIXME: CRTP
//     if (_loopbackResampler) {
//         delete _loopbackResampler;
//         _loopbackResampler = NULL;
//     }
//
//     // cleanup any previously initialized device
//     if (_audioInput) {
//         // The call to stop() causes _inputDevice to be destructed.
//         // That in turn causes it to be disconnected (see for example
//         // http://stackoverflow.com/questions/9264750/qt-signals-and-slots-object-disconnect).
//         _audioInput->stop();
//         _inputDevice = NULL;
//
//         _audioInput->deleteLater();
//         _audioInput = NULL;
//
//         _inputDeviceInfo.setDevice(QAudioDeviceInfo());
//     }
//
//
// #if defined(Q_OS_ANDROID)
//     _shouldRestartInputSetup = false;  // avoid a double call to _audioInput->start() from audioInputStateChanged
// #endif


    if (_dummyAudioInput) {
        _dummyAudioInput->stop();
        _dummyAudioInput->deleteLater();
        _dummyAudioInput = NULL;
    }

    // cleanup any resamplers
    if (_inputToNetworkResampler) {
        delete _inputToNetworkResampler;
        _inputToNetworkResampler = NULL;
    }

    if (_audioGate) {
        delete _audioGate;
        _audioGate = nullptr;
    }

}

template <typename Derived>
bool AudioPacketHandler<Derived>::setupInput(QAudioFormat inputFormat) {
    bool supportedFormat = false;

    if (inputFormat.channelCount() > 2) {
        qCDebug(audioclient) << "Audio input has too many channels: " << inputFormat.channelCount();
        return false;
    }

    _inputFormat = inputFormat;

    // if necessary reinitialize the codec
    if (_inputFormat.channelCount() != _desiredInputFormat.channelCount()) {
        if (_codec) {
            if (_encoder) {
                _codec->releaseEncoder(_encoder);
            }
            _encoder = _codec->createEncoder(_desiredInputFormat.sampleRate(), _inputFormat.channelCount());
            qCDebug(audioclient) << "Reset Codec:" << _selectedCodecName << "isStereoInput:" << (_inputFormat.channelCount() == 2);
        }
        _desiredInputFormat.setChannelCount(inputFormat.channelCount());
    }

    qCDebug(audioclient) << "The format to be used for audio input is" << _inputFormat;

    // we've got the best we can get for input
    // if required, setup a resampler for this input to our desired network format
    if (_inputFormat != _desiredInputFormat
        && _inputFormat.sampleRate() != _desiredInputFormat.sampleRate()) {
        qCDebug(audioclient) << "Attemping to create a resampler for input format to network format.";

        // assert(_inputFormat.sampleSize() == 16);
        assert(_desiredInputFormat.sampleSize() == 16);
        int channelCount = (_inputFormat.channelCount() == 2 && _desiredInputFormat.channelCount() == 2) ? 2 : 1;

        _inputToNetworkResampler = new AudioSRC(_inputFormat.sampleRate(), _desiredInputFormat.sampleRate(), channelCount);

    } else {
        qCDebug(audioclient) << "No resampling required for audio input to match desired network format.";
    }

    // the audio gate runs after the resampler
    _audioGate = new AudioGate(_desiredInputFormat.sampleRate(), _desiredInputFormat.channelCount());
    qCDebug(audioclient) << "Noise gate created with" << _desiredInputFormat.channelCount() << "channels.";
    auto numInputCallbackBytes = calculateNumberOfInputCallbackBytes(_inputFormat);
    // how do we want to handle input working, but output not working?
    int numFrameSamples = calculateNumberOfFrameSamples(numInputCallbackBytes);
    _inputRingBuffer.resizeForFrameSize(numFrameSamples);

    return supportedFormat;
}

template <typename Derived>
void AudioPacketHandler<Derived>::setupDummyInput() {
    // Generates audio callbacks on a timer to simulate a mic stream of silent packets.
    // This enables clients without a mic to still receive an audio stream from the mixer.

    // FIXME: CRTP
    // qCDebug(audioclient) << "Audio input device is not available, using dummy input.";
    // _inputDeviceInfo.setDevice(QAudioDeviceInfo());
    // emit deviceChanged(QAudio::AudioInput, _inputDeviceInfo);

    _inputFormat = _desiredInputFormat;
    qCDebug(audioclient) << "The format to be used for audio input is" << _inputFormat;
    qCDebug(audioclient) << "No re-sampling required for audio input to match desired network format.";

    _audioGate = new AudioGate(_desiredInputFormat.sampleRate(), _desiredInputFormat.channelCount());
    qCDebug(audioclient) << "Noise gate created with" << _desiredInputFormat.channelCount() << "channels.";

    // generate audio callbacks at the network sample rate
    _dummyAudioInput = new QTimer();
    _dummyAudioInput->connect(_dummyAudioInput, &QTimer::timeout, [this](){
        const int numNetworkBytes = _desiredInputFormat.channelCount() == AudioConstants::STEREO
            ? AudioConstants::NETWORK_FRAME_BYTES_STEREO
            : AudioConstants::NETWORK_FRAME_BYTES_PER_CHANNEL;

        QByteArray audioBuffer(numNetworkBytes, 0);  // silent
        handleAudioInput(audioBuffer);
    });
    _dummyAudioInput->start((int)(AudioConstants::NETWORK_FRAME_MSECS + 0.5f));
}

// FIXME: CRTP move to derived, different implementation
// bool AudioClient::switchInputToAudioDevice(const HifiAudioDeviceInfo inputDeviceInfo, bool isShutdownRequest) {
//     Q_ASSERT_X(QThread::currentThread() == thread(), Q_FUNC_INFO, "Function invoked on wrong thread");
//
//     qCDebug(audioclient) << __FUNCTION__ << "_inputDeviceInfo: [" << _inputDeviceInfo.deviceName() << ":" << _inputDeviceInfo.getDevice().deviceName()
//         << "-- inputDeviceInfo:" << inputDeviceInfo.deviceName() << ":" << inputDeviceInfo.getDevice().deviceName() << "]";
//     // NOTE: device start() uses the Qt internal device list
//     Lock lock(_deviceMutex);
//
//     bool supportedFormat = false;
//
//     cleanupInput();
//
//     if (!inputDeviceInfo.getDevice().isNull()) {
//         qCDebug(audioclient) << "The audio input device" << inputDeviceInfo.deviceName() << ":" << inputDeviceInfo.getDevice().deviceName() << "is available.";
//
//         //do not update UI that we're changing devices if default or same device
//         _inputDeviceInfo = inputDeviceInfo;
//         emit deviceChanged(QAudio::AudioInput, _inputDeviceInfo);
//
//         QAudioFormat format;
//         if (adjustedFormatForAudioDevice(_inputDeviceInfo.getDevice(), _desiredInputFormat, format)) {
//             setupInput(format);
//
//             auto numInputCallbackBytes = calculateNumberOfInputCallbackBytes(_inputFormat);
//             // if the user wants stereo but this device can't provide then bail
//             if (!_isStereoInput || _inputFormat.channelCount() == 2) {
//                 _audioInput = new QAudioInput(_inputDeviceInfo.getDevice(), _inputFormat, this);
//                 _audioInput->setBufferSize(numInputCallbackBytes * CALLBACK_ACCELERATOR_RATIO);
//                 // different audio input devices may have different volumes
//                 emit inputVolumeChanged(_audioInput->volume());
//
// #if defined(Q_OS_ANDROID)
//                 if (_audioInput) {
//                     _shouldRestartInputSetup = true;
//                     connect(_audioInput, &QAudioInput::stateChanged, this, &AudioClient::audioInputStateChanged);
//                 }
// #endif
//                 _inputDevice = _audioInput->start();
//
//                 if (_inputDevice) {
//                     connect(_inputDevice, SIGNAL(readyRead()), this, SLOT(handleMicAudioInput()));
//                     supportedFormat = true;
//                 } else {
//                     qCDebug(audioclient) << "Error starting audio input -" << _audioInput->error();
//                     _audioInput->deleteLater();
//                     _audioInput = NULL;
//                 }
//             }
//         }
//     }
//
//     // If there is no working input device, use the dummy input device.
//     if (!_audioInput) {
//         setupDummyInput();
//     }
//
//     return supportedFormat;
// }

template <typename Derived>
void AudioPacketHandler<Derived>::cleanupOutput() {
    // FIXME: CRTP
    // cleanup any previously initialized device
    // if (_audioOutput) {
    //     _audioOutput->stop();
    //
    //     //must be deleted in next eventloop cycle when its called from notify()
    //     _audioOutput->deleteLater();
    //     _audioOutput = NULL;
    //
    //     _loopbackOutputDevice = NULL;
    //     //must be deleted in next eventloop cycle when its called from notify()
    //     _loopbackAudioOutput->deleteLater();
    //     _loopbackAudioOutput = NULL;
    //
    //     _outputDeviceInfo.setDevice(QAudioDeviceInfo());
    // }
    //
    // if (_loopbackResampler) {
    //     delete _loopbackResampler;
    //     _loopbackResampler = NULL;
    // }

    _audioOutputInitialized = false;

    if (_audioOutputIODevice.isOpen()) {
        _audioOutputIODevice.close();
    }

    if (_outputMixBuffer) {
        delete[] _outputMixBuffer;
        _outputMixBuffer = NULL;

        delete[] _outputScratchBuffer;
        _outputScratchBuffer = NULL;

        delete[] _localOutputMixBuffer;
        _localOutputMixBuffer = NULL;
    }

    // cleanup any resamplers
    if (_networkToOutputResampler) {
        delete _networkToOutputResampler;
        _networkToOutputResampler = NULL;
    }

    if (_localToOutputResampler) {
        delete _localToOutputResampler;
        _localToOutputResampler = NULL;
    }
}

template <typename Derived>
bool AudioPacketHandler<Derived>::setupOutput(QAudioFormat outputFormat) {
    _outputFormat = outputFormat;

    qCDebug(audioclient) << "The format to be used for audio output is" << _outputFormat;

    // we've got the best we can get for input
    // if required, setup a resampler for this input to our desired network format
    if (_desiredOutputFormat != _outputFormat
        && _desiredOutputFormat.sampleRate() != _outputFormat.sampleRate()) {
        qCDebug(audioclient) << "Attemping to create a resampler for network format to output format.";

        assert(_desiredOutputFormat.sampleSize() == 16);
        // assert(_outputFormat.sampleSize() == 16);

        _networkToOutputResampler = new AudioSRC(_desiredOutputFormat.sampleRate(), _outputFormat.sampleRate(), OUTPUT_CHANNEL_COUNT);
        _localToOutputResampler = new AudioSRC(_desiredOutputFormat.sampleRate(), _outputFormat.sampleRate(), OUTPUT_CHANNEL_COUNT);

    } else {
        qCDebug(audioclient) << "No resampling required for network output to match actual output format.";
    }


    int frameSize = (AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL * _outputFormat.channelCount() * _outputFormat.sampleRate()) / _desiredOutputFormat.sampleRate();
    int requestedSize = _sessionOutputBufferSizeFrames * frameSize * AudioConstants::SAMPLE_SIZE;

    _outputPeriod = requestedSize;

    outputFormatChanged();

    _outputMixBuffer = new float[_outputPeriod];
    _outputScratchBuffer = new int16_t[_outputPeriod];

    // size local output mix buffer based on resampled network frame size
    int networkPeriod = _localToOutputResampler ?  _localToOutputResampler->getMaxOutput(AudioConstants::NETWORK_FRAME_SAMPLES_STEREO) : AudioConstants::NETWORK_FRAME_SAMPLES_STEREO;
    _localOutputMixBuffer = new float[networkPeriod];

    // local period should be at least twice the output period,
    // in case two device reads happen before more data can be read (worst case)
    int localPeriod = _outputPeriod * 2;
    // round up to an exact multiple of networkPeriod
    localPeriod = ((localPeriod + networkPeriod - 1) / networkPeriod) * networkPeriod;
    // this ensures lowest latency without stutter from underrun
    _localInjectorsStream.resizeForFrameSize(localPeriod);

    _audioOutputInitialized = true;

    return true;

}

// FIXME: CRTP
// bool AudioClient::switchOutputToAudioDevice(const HifiAudioDeviceInfo outputDeviceInfo, bool isShutdownRequest) {
//     Q_ASSERT_X(QThread::currentThread() == thread(), Q_FUNC_INFO, "Function invoked on wrong thread");
//
//     qCDebug(audioclient) << __FUNCTION__ << "_outputdeviceInfo: [" << _outputDeviceInfo.deviceName() << ":" << _outputDeviceInfo.getDevice().deviceName()
//         << "-- outputDeviceInfo:" << outputDeviceInfo.deviceName() << ":" << outputDeviceInfo.getDevice().deviceName() << "]";
//     bool supportedFormat = false;
//
//     // NOTE: device start() uses the Qt internal device list
//     Lock lock(_deviceMutex);
//
//     _localSamplesAvailable.exchange(0, std::memory_order_release);
//
//     //wait on local injectors prep to finish running
//     if ( !_localPrepInjectorFuture.isFinished()) {
//         _localPrepInjectorFuture.waitForFinished();
//     }
//
//     Lock localAudioLock(_localAudioMutex);
//
//     cleanupOutput();
//
//     if (!outputDeviceInfo.getDevice().isNull()) {
//         qCDebug(audioclient) << "The audio output device" << outputDeviceInfo.deviceName() << ":" << outputDeviceInfo.getDevice().deviceName() << "is available.";
//
//         //do not update UI that we're changing devices if default or same device
//         _outputDeviceInfo = outputDeviceInfo;
//         emit deviceChanged(QAudio::AudioOutput, _outputDeviceInfo);
//
//         AudioFormat outputFormat;
//         if (adjustedFormatForAudioDevice(_outputDeviceInfo.getDevice(), _desiredOutputFormat, outputFormat)) {
//             setupOutput(outputFormat);
//
//             int bufferSize = _audioOutput->bufferSize();
//             int bufferSamples = bufferSize / AudioConstants::SAMPLE_SIZE;
//             int bufferFrames = bufferSamples / (float)frameSize;
//             qCDebug(audioclient) << "frame (samples):" << frameSize;
//             qCDebug(audioclient) << "buffer (frames):" << bufferFrames;
//             qCDebug(audioclient) << "buffer (samples):" << bufferSamples;
//             qCDebug(audioclient) << "buffer (bytes):" << bufferSize;
//             qCDebug(audioclient) << "requested (bytes):" << requestedSize;
//             qCDebug(audioclient) << "period (samples):" << _outputPeriod;
//             qCDebug(audioclient) << "local buffer (samples):" << localPeriod;
//
//             // unlock to avoid a deadlock with the device callback (which always succeeds this initialization)
//             localAudioLock.unlock();
//
//             // setup a loopback audio output device
//             _loopbackAudioOutput = new QAudioOutput(outputDeviceInfo.getDevice(), _outputFormat, this);
//
//             supportedFormat = true;
//         }
//     }
//
//     return supportedFormat;
// }

template <typename Derived>
int AudioPacketHandler<Derived>::setOutputBufferSize(int numFrames, bool persist) {
    qCDebug(audioclient) << __FUNCTION__ << "numFrames:" << numFrames << "persist:" << persist;

    numFrames = std::min(std::max(numFrames, MIN_BUFFER_FRAMES), MAX_BUFFER_FRAMES);
    qCDebug(audioclient) << __FUNCTION__ << "clamped numFrames:" << numFrames << "_sessionOutputBufferSizeFrames:" << _sessionOutputBufferSizeFrames;

    if (numFrames != _sessionOutputBufferSizeFrames) {
        qCInfo(audioclient, "Audio output buffer set to %d frames", numFrames);
        _sessionOutputBufferSizeFrames = numFrames;
        if (persist) {
            _outputBufferSizeFrames.set(numFrames);
        }
    }
    return numFrames;
}

template <typename Derived>
int AudioPacketHandler<Derived>::calculateNumberOfInputCallbackBytes(const QAudioFormat& format) const {
    int numInputCallbackBytes = (int)(((AudioConstants::NETWORK_FRAME_BYTES_PER_CHANNEL
        * format.channelCount()
        * ((float) format.sampleRate() / AudioConstants::SAMPLE_RATE))) + 0.5f);

    return numInputCallbackBytes;
}

template <typename Derived>
int AudioPacketHandler<Derived>::calculateNumberOfFrameSamples(int numBytes) const {
    int frameSamples = numBytes / AudioConstants::SAMPLE_SIZE;
    return frameSamples;
}

template <typename Derived>
float AudioPacketHandler<Derived>::azimuthForSource(const glm::vec3& relativePosition) {
    glm::quat inverseOrientation = glm::inverse(_orientationGetter());

    glm::vec3 rotatedSourcePosition = inverseOrientation * relativePosition;

    // project the rotated source position vector onto the XZ plane
    rotatedSourcePosition.y = 0.0f;

    static const float SOURCE_DISTANCE_THRESHOLD = 1e-30f;

    float rotatedSourcePositionLength2 = glm::length2(rotatedSourcePosition);
    if (rotatedSourcePositionLength2 > SOURCE_DISTANCE_THRESHOLD) {

        // produce an oriented angle about the y-axis
        glm::vec3 direction = rotatedSourcePosition * (1.0f / fastSqrtf(rotatedSourcePositionLength2));
        float angle = fastAcosf(glm::clamp(-direction.z, -1.0f, 1.0f));  // UNIT_NEG_Z is "forward"
        return (direction.x < 0.0f) ? -angle : angle;

    } else {
        // no azimuth if they are in same spot
        return 0.0f;
    }
}

template <typename Derived>
float AudioPacketHandler<Derived>::gainForSource(float distance, float volume) {

    // attenuation = -6dB * log2(distance)
    // reference attenuation of 0dB at distance = ATTN_DISTANCE_REF
    float d = (1.0f / ATTN_DISTANCE_REF) * std::max(distance, HRTF_NEARFIELD_MIN);
    float gain = volume / d;
    gain = std::min(gain, ATTN_GAIN_MAX);

    return gain;
}

template <typename Derived>
qint64 AudioPacketHandler<Derived>::AudioOutputIODevice::readData(char * data, qint64 maxSize) {

    // lock-free wait for initialization to avoid races
    if (!_audio->_audioOutputInitialized.load(std::memory_order_acquire)) {
        memset(data, 0, maxSize);
        return maxSize;
    }

    // max samples requested from OUTPUT_CHANNEL_COUNT
    int deviceChannelCount = _audio->_outputFormat.channelCount();
    int maxSamplesRequested = (int)(maxSize / AudioConstants::SAMPLE_SIZE) * OUTPUT_CHANNEL_COUNT / deviceChannelCount;
    // restrict samplesRequested to the size of our mix/scratch buffers
    maxSamplesRequested = std::min(maxSamplesRequested, _audio->_outputPeriod);

    int16_t* scratchBuffer = _audio->_outputScratchBuffer;
    float* mixBuffer = _audio->_outputMixBuffer;

    int samplesRequested = maxSamplesRequested;
    int networkSamplesPopped;
    if ((networkSamplesPopped = _receivedAudioStream.popSamples(samplesRequested, false)) > 0) {
        qCDebug(audiostream, "Read %d samples from buffer (%d available, %d requested)", networkSamplesPopped, _receivedAudioStream.getSamplesAvailable(), samplesRequested);
        AudioRingBuffer::ConstIterator lastPopOutput = _receivedAudioStream.getLastPopOutput();
        lastPopOutput.readSamples(scratchBuffer, networkSamplesPopped);
        for (int i = 0; i < networkSamplesPopped; i++) {
            mixBuffer[i] = convertToFloat(scratchBuffer[i]);
        }
        samplesRequested = networkSamplesPopped;
    }

    int injectorSamplesPopped = 0;
    {
        bool append = networkSamplesPopped > 0;
        // check the samples we have available locklessly; this is possible because only two functions add to the count:
        // - prepareLocalAudioInjectors will only increase samples count
        // - switchOutputToAudioDevice will zero samples count,
        //   stop the device - so that readData will exhaust the existing buffer or see a zeroed samples count,
        //   and start the device - which can then only see a zeroed samples count
        int samplesAvailable = _audio->_localSamplesAvailable.load(std::memory_order_acquire);

        // if we do not have enough samples buffered despite having injectors, buffer them synchronously
        if (samplesAvailable < samplesRequested && _audio->_localInjectorsAvailable.load(std::memory_order_acquire)) {
            // try_to_lock, in case the device is being shut down already
            std::unique_ptr<Lock> localAudioLock(new Lock(_audio->_localAudioMutex, std::try_to_lock));
            if (localAudioLock->owns_lock()) {
                _audio->prepareLocalAudioInjectors(std::move(localAudioLock));
                samplesAvailable = _audio->_localSamplesAvailable.load(std::memory_order_acquire);
            }
        }

        samplesRequested = std::min(samplesRequested, samplesAvailable);
        if ((injectorSamplesPopped = _localInjectorsStream.appendSamples(mixBuffer, samplesRequested, append)) > 0) {
            _audio->_localSamplesAvailable.fetch_sub(injectorSamplesPopped, std::memory_order_release);
            qCDebug(audiostream, "Read %d samples from injectors (%d available, %d requested)", injectorSamplesPopped, _localInjectorsStream.samplesAvailable(), samplesRequested);
        }
    }

    // prepare injectors for the next callback
     _audio->_localPrepInjectorFuture = QtConcurrent::run(QThreadPool::globalInstance(), [this] {
        _audio->prepareLocalAudioInjectors();
    });

    int samplesPopped = std::max(networkSamplesPopped, injectorSamplesPopped);
    if (samplesPopped == 0) {
        // nothing on network, don't grab anything from injectors, and fill with silence
        samplesPopped = maxSamplesRequested;
        memset(mixBuffer, 0, samplesPopped * sizeof(float));
    }
    int framesPopped = samplesPopped / OUTPUT_CHANNEL_COUNT;

    // apply output gain
    float newGain = _audio->_outputGain.load(std::memory_order_acquire);
    float oldGain = _audio->_lastOutputGain;
    _audio->_lastOutputGain = newGain;

    applyGainSmoothing<OUTPUT_CHANNEL_COUNT>(mixBuffer, framesPopped, oldGain, newGain);

    // limit the audio
    _audio->_audioLimiter.render(mixBuffer, scratchBuffer, framesPopped);

#if defined(WEBRTC_AUDIO)
    if (_audio->_isAECEnabled) {
        _audio->processWebrtcFarEnd(scratchBuffer, framesPopped, OUTPUT_CHANNEL_COUNT, _audio->_outputFormat.sampleRate());
    }
#endif

    int bytesWritten = 0;

    if (_audio->_outputFormat.sampleType() == QAudioFormat::Float) {
        // Convert samples to normalized float. This is used in the client
        // library, since a lot of audio APIs prefer to restrict themselves to
        // float samples.
        for (int i = 0; i < samplesPopped; i++) {
            mixBuffer[i] = convertToFloat(scratchBuffer[i]);
        }

        // if required, upmix or downmix to deviceChannelCount
        bytesWritten = copyWithChannelConversion(mixBuffer, OUTPUT_CHANNEL_COUNT, data, deviceChannelCount, samplesPopped);
    }
    else
    {
        // if required, upmix or downmix to deviceChannelCount
        bytesWritten = copyWithChannelConversion(scratchBuffer, OUTPUT_CHANNEL_COUNT, data, deviceChannelCount, samplesPopped);
    }

    assert(bytesWritten <= maxSize);

    // FIXME: CRTP
    // send output buffer for recording
    // if (_audio->_isRecording) {
    //     Lock lock(_recordMutex);
    //     _audio->_audioFileWav.addRawAudioChunk(data, bytesWritten);
    // }

    // FIXEM: CRTP
    // int bytesAudioOutputUnplayed = _audio->_audioOutput->bufferSize() - _audio->_audioOutput->bytesFree();
    // float msecsAudioOutputUnplayed = bytesAudioOutputUnplayed / (float)_audio->_outputFormat.bytesForDuration(USECS_PER_MSEC);
    // _audio->_stats.updateOutputMsUnplayed(msecsAudioOutputUnplayed);
    //
    // if (bytesAudioOutputUnplayed == 0) {
    //     _unfulfilledReads++;
    // }

    return bytesWritten;
}

template <typename Derived>
void AudioPacketHandler<Derived>::setAvatarBoundingBoxParameters(glm::vec3 corner, glm::vec3 scale) {
    avatarBoundingBoxCorner = corner;
    avatarBoundingBoxScale = scale;
}


template <typename Derived>
void AudioPacketHandler<Derived>::startThread() {
    moveToNewNamedThread(derived(), "Audio Thread", [this] { start(); }, QThread::TimeCriticalPriority);
}

#endif /* end of include guard */
