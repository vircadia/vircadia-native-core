//
//  AudioClient.cpp
//  interface/src
//
//  Created by Stephen Birarda on 1/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

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
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Mmsystem.h>
#include <mmdeviceapi.h>
#include <devicetopology.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <VersionHelpers.h>
#endif

#include <QtCore/QBuffer>
#include <QtMultimedia/QAudioInput>
#include <QtMultimedia/QAudioOutput>

#include <NodeList.h>
#include <plugins/CodecPlugin.h>
#include <plugins/PluginManager.h>
#include <udt/PacketHeaders.h>
#include <PositionalAudioStream.h>
#include <SettingHandle.h>
#include <SharedUtil.h>
#include <UUID.h>
#include <Transform.h>

#include "PositionalAudioStream.h"
#include "AudioClientLogging.h"

#include "AudioClient.h"

static const int RECEIVED_AUDIO_STREAM_CAPACITY_FRAMES = 100;

static const auto DEFAULT_POSITION_GETTER = []{ return Vectors::ZERO; };
static const auto DEFAULT_ORIENTATION_GETTER = [] { return Quaternions::IDENTITY; };

static const int DEFAULT_AUDIO_OUTPUT_GATE_THRESHOLD = 1;

Setting::Handle<bool> dynamicJitterBuffers("dynamicJitterBuffers", DEFAULT_DYNAMIC_JITTER_BUFFERS);
Setting::Handle<int> maxFramesOverDesired("maxFramesOverDesired", DEFAULT_MAX_FRAMES_OVER_DESIRED);
Setting::Handle<int> staticDesiredJitterBufferFrames("staticDesiredJitterBufferFrames",
                                                     DEFAULT_STATIC_DESIRED_JITTER_BUFFER_FRAMES);
Setting::Handle<bool> useStDevForJitterCalc("useStDevForJitterCalc", DEFAULT_USE_STDEV_FOR_JITTER_CALC);
Setting::Handle<int> windowStarveThreshold("windowStarveThreshold", DEFAULT_WINDOW_STARVE_THRESHOLD);
Setting::Handle<int> windowSecondsForDesiredCalcOnTooManyStarves("windowSecondsForDesiredCalcOnTooManyStarves",
                                                                 DEFAULT_WINDOW_SECONDS_FOR_DESIRED_CALC_ON_TOO_MANY_STARVES);
Setting::Handle<int> windowSecondsForDesiredReduction("windowSecondsForDesiredReduction",
                                                      DEFAULT_WINDOW_SECONDS_FOR_DESIRED_REDUCTION);
Setting::Handle<bool> repetitionWithFade("repetitionWithFade", DEFAULT_REPETITION_WITH_FADE);

// protect the Qt internal device list
using Mutex = std::mutex;
using Lock = std::unique_lock<Mutex>;
static Mutex _deviceMutex;

// background thread that continuously polls for device changes
class CheckDevicesThread : public QThread {
public:
    const unsigned long DEVICE_CHECK_INTERVAL_MSECS = 2 * 1000;

    CheckDevicesThread(AudioClient* audioClient)
        : _audioClient(audioClient) {

        connect(qApp, &QCoreApplication::aboutToQuit, [this] {
            _quit = true;
        });
    }

    void run() override {
        while (!_quit) {
            QThread::msleep(DEVICE_CHECK_INTERVAL_MSECS);
            _audioClient->checkDevices();
        }
    }

private:
    AudioClient* _audioClient { nullptr };
    bool _quit { false };
};

AudioClient::AudioClient() :
    AbstractAudioInterface(),
    _gateThreshold("audioOutputGateThreshold", DEFAULT_AUDIO_OUTPUT_GATE_THRESHOLD),
    _gate(this, _gateThreshold.get()),
    _audioInput(NULL),
    _desiredInputFormat(),
    _inputFormat(),
    _numInputCallbackBytes(0),
    _audioOutput(NULL),
    _desiredOutputFormat(),
    _outputFormat(),
    _outputFrameSize(0),
    _numOutputCallbackBytes(0),
    _loopbackAudioOutput(NULL),
    _loopbackOutputDevice(NULL),
    _inputRingBuffer(0),
    _receivedAudioStream(0, RECEIVED_AUDIO_STREAM_CAPACITY_FRAMES, InboundAudioStream::Settings()),
    _isStereoInput(false),
    _outputStarveDetectionStartTimeMsec(0),
    _outputStarveDetectionCount(0),
    _outputBufferSizeFrames("audioOutputBufferSizeFrames", DEFAULT_AUDIO_OUTPUT_BUFFER_SIZE_FRAMES),
    _sessionOutputBufferSizeFrames(_outputBufferSizeFrames.get()),
    _outputStarveDetectionEnabled("audioOutputBufferStarveDetectionEnabled",
                                  DEFAULT_AUDIO_OUTPUT_STARVE_DETECTION_ENABLED),
    _outputStarveDetectionPeriodMsec("audioOutputStarveDetectionPeriod",
                                     DEFAULT_AUDIO_OUTPUT_STARVE_DETECTION_PERIOD),
    _outputStarveDetectionThreshold("audioOutputStarveDetectionThreshold",
                                    DEFAULT_AUDIO_OUTPUT_STARVE_DETECTION_THRESHOLD),
    _averagedLatency(0.0f),
    _lastInputLoudness(0.0f),
    _timeSinceLastClip(-1.0f),
    _muted(false),
    _shouldEchoLocally(false),
    _shouldEchoToServer(false),
    _isNoiseGateEnabled(true),
    _reverb(false),
    _reverbOptions(&_scriptReverbOptions),
    _inputToNetworkResampler(NULL),
    _networkToOutputResampler(NULL),
    _audioLimiter(AudioConstants::SAMPLE_RATE, AudioConstants::STEREO),
    _outgoingAvatarAudioSequenceNumber(0),
    _audioOutputIODevice(_receivedAudioStream, this),
    _stats(&_receivedAudioStream),
    _inputGate(),
    _positionGetter(DEFAULT_POSITION_GETTER),
    _orientationGetter(DEFAULT_ORIENTATION_GETTER)
{
    connect(&_receivedAudioStream, &MixedProcessedAudioStream::processSamples,
            this, &AudioClient::processReceivedSamples, Qt::DirectConnection);
    connect(this, &AudioClient::changeDevice, this, [=](const QAudioDeviceInfo& outputDeviceInfo) { switchOutputToAudioDevice(outputDeviceInfo); });

    connect(&_receivedAudioStream, &InboundAudioStream::mismatchedAudioCodec, this, &AudioClient::handleMismatchAudioFormat);

    _inputDevices = getDeviceNames(QAudio::AudioInput);
    _outputDevices = getDeviceNames(QAudio::AudioOutput);

    // start a thread to detect any device changes
    QThread* checkDevicesThread = new CheckDevicesThread(this);
    checkDevicesThread->setObjectName("CheckDevices Thread");
    checkDevicesThread->setPriority(QThread::LowPriority);
    checkDevicesThread->start();

    configureReverb();

    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerListener(PacketType::AudioStreamStats, &_stats, "processStreamStatsPacket");
    packetReceiver.registerListener(PacketType::AudioEnvironment, this, "handleAudioEnvironmentDataPacket");
    packetReceiver.registerListener(PacketType::SilentAudioFrame, this, "handleAudioDataPacket");
    packetReceiver.registerListener(PacketType::MixedAudio, this, "handleAudioDataPacket");
    packetReceiver.registerListener(PacketType::NoisyMute, this, "handleNoisyMutePacket");
    packetReceiver.registerListener(PacketType::MuteEnvironment, this, "handleMuteEnvironmentPacket");
    packetReceiver.registerListener(PacketType::SelectedAudioFormat, this, "handleSelectedAudioFormat");
}

AudioClient::~AudioClient() {
    stop();
    if (_codec && _encoder) {
        _codec->releaseEncoder(_encoder);
        _encoder = nullptr;
    }
}

void AudioClient::handleMismatchAudioFormat(SharedNodePointer node, const QString& currentCodec, const QString& recievedCodec) {
    qDebug() << __FUNCTION__ << "sendingNode:" << *node << "currentCodec:" << currentCodec << "recievedCodec:" << recievedCodec;
    selectAudioFormat(recievedCodec);
}


void AudioClient::reset() {
    _receivedAudioStream.reset();
    _stats.reset();
    _sourceReverb.reset();
    _listenerReverb.reset();
}

void AudioClient::audioMixerKilled() {
    _hasReceivedFirstPacket = false;
    _outgoingAvatarAudioSequenceNumber = 0;
    _stats.reset();
    emit disconnected();
}

// thread-safe
QList<QAudioDeviceInfo> getAvailableDevices(QAudio::Mode mode) {
    // NOTE: availableDevices() clobbers the Qt internal device list
    Lock lock(_deviceMutex);
    return QAudioDeviceInfo::availableDevices(mode);
}

QAudioDeviceInfo getNamedAudioDeviceForMode(QAudio::Mode mode, const QString& deviceName) {
    QAudioDeviceInfo result;
    foreach(QAudioDeviceInfo audioDevice, getAvailableDevices(mode)) {
        if (audioDevice.deviceName().trimmed() == deviceName.trimmed()) {
            result = audioDevice;
            break;
        }
    }

    return result;
}

int numDestinationSamplesRequired(const QAudioFormat& sourceFormat, const QAudioFormat& destinationFormat,
                                  int numSourceSamples) {
    float ratio = (float) destinationFormat.channelCount() / sourceFormat.channelCount();
    ratio *= (float) destinationFormat.sampleRate() / sourceFormat.sampleRate();

    return (numSourceSamples * ratio) + 0.5f;
}

#ifdef Q_OS_WIN
QString friendlyNameForAudioDevice(IMMDevice* pEndpoint) {
    QString deviceName;
    IPropertyStore* pPropertyStore;
    pEndpoint->OpenPropertyStore(STGM_READ, &pPropertyStore);
    pEndpoint->Release();
    pEndpoint = NULL;
    PROPVARIANT pv;
    PropVariantInit(&pv);
    HRESULT hr = pPropertyStore->GetValue(PKEY_Device_FriendlyName, &pv);
    pPropertyStore->Release();
    pPropertyStore = NULL;
    deviceName = QString::fromWCharArray((wchar_t*)pv.pwszVal);
    if (!IsWindows8OrGreater()) {
        // Windows 7 provides only the 31 first characters of the device name.
        const DWORD QT_WIN7_MAX_AUDIO_DEVICENAME_LEN = 31;
        deviceName = deviceName.left(QT_WIN7_MAX_AUDIO_DEVICENAME_LEN);
    }
    PropVariantClear(&pv);
    return deviceName;
}

QString AudioClient::friendlyNameForAudioDevice(wchar_t* guid) {
    QString deviceName;
    HRESULT hr = S_OK;
    CoInitialize(NULL);
    IMMDeviceEnumerator* pMMDeviceEnumerator = NULL;
    CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pMMDeviceEnumerator);
    IMMDevice* pEndpoint;
    hr = pMMDeviceEnumerator->GetDevice(guid, &pEndpoint);
    if (hr == E_NOTFOUND) {
        printf("Audio Error: device not found\n");
        deviceName = QString("NONE");
    } else {
        deviceName = ::friendlyNameForAudioDevice(pEndpoint);
    }
    pMMDeviceEnumerator->Release();
    pMMDeviceEnumerator = NULL;
    CoUninitialize();
    return deviceName;
}

#endif

QAudioDeviceInfo defaultAudioDeviceForMode(QAudio::Mode mode) {
#ifdef __APPLE__
    if (getAvailableDevices(mode).size() > 1) {
        AudioDeviceID defaultDeviceID = 0;
        uint32_t propertySize = sizeof(AudioDeviceID);
        AudioObjectPropertyAddress propertyAddress = {
            kAudioHardwarePropertyDefaultInputDevice,
            kAudioObjectPropertyScopeGlobal,
            kAudioObjectPropertyElementMaster
        };

        if (mode == QAudio::AudioOutput) {
            propertyAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
        }


        OSStatus getPropertyError = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                                               &propertyAddress,
                                                               0,
                                                               NULL,
                                                               &propertySize,
                                                               &defaultDeviceID);

        if (!getPropertyError && propertySize) {
            CFStringRef deviceName = NULL;
            propertySize = sizeof(deviceName);
            propertyAddress.mSelector = kAudioDevicePropertyDeviceNameCFString;
            getPropertyError = AudioObjectGetPropertyData(defaultDeviceID, &propertyAddress, 0,
                                                          NULL, &propertySize, &deviceName);

            if (!getPropertyError && propertySize) {
                // find a device in the list that matches the name we have and return it
                foreach(QAudioDeviceInfo audioDevice, getAvailableDevices(mode)) {
                    if (audioDevice.deviceName() == CFStringGetCStringPtr(deviceName, kCFStringEncodingMacRoman)) {
                        return audioDevice;
                    }
                }
            }
        }
    }
#endif
#ifdef WIN32
    QString deviceName;
    //Check for Windows Vista or higher, IMMDeviceEnumerator doesn't work below that.
    if (!IsWindowsVistaOrGreater()) { // lower then vista
        if (mode == QAudio::AudioInput) {
            WAVEINCAPS wic;
            // first use WAVE_MAPPER to get the default devices manufacturer ID
            waveInGetDevCaps(WAVE_MAPPER, &wic, sizeof(wic));
            //Use the received manufacturer id to get the device's real name
            waveInGetDevCaps(wic.wMid, &wic, sizeof(wic));
            qCDebug(audioclient) << "input device:" << wic.szPname;
            deviceName = wic.szPname;
        } else {
            WAVEOUTCAPS woc;
            // first use WAVE_MAPPER to get the default devices manufacturer ID
            waveOutGetDevCaps(WAVE_MAPPER, &woc, sizeof(woc));
            //Use the received manufacturer id to get the device's real name
            waveOutGetDevCaps(woc.wMid, &woc, sizeof(woc));
            qCDebug(audioclient) << "output device:" << woc.szPname;
            deviceName = woc.szPname;
        }
    } else {
        HRESULT hr = S_OK;
        CoInitialize(NULL);
        IMMDeviceEnumerator* pMMDeviceEnumerator = NULL;
        CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pMMDeviceEnumerator);
        IMMDevice* pEndpoint;
        hr = pMMDeviceEnumerator->GetDefaultAudioEndpoint(mode == QAudio::AudioOutput ? eRender : eCapture, eMultimedia, &pEndpoint);
        if (hr == E_NOTFOUND) {
            printf("Audio Error: device not found\n");
            deviceName = QString("NONE");
        } else {
            deviceName = friendlyNameForAudioDevice(pEndpoint);
        }
        pMMDeviceEnumerator->Release();
        pMMDeviceEnumerator = NULL;
        CoUninitialize();
    }

    qCDebug(audioclient) << "DEBUG [" << deviceName << "] [" << getNamedAudioDeviceForMode(mode, deviceName).deviceName() << "]";

    return getNamedAudioDeviceForMode(mode, deviceName);
#endif


    // fallback for failed lookup is the default device
    return (mode == QAudio::AudioInput) ? QAudioDeviceInfo::defaultInputDevice() : QAudioDeviceInfo::defaultOutputDevice();
}

bool adjustedFormatForAudioDevice(const QAudioDeviceInfo& audioDevice,
                                  const QAudioFormat& desiredAudioFormat,
                                  QAudioFormat& adjustedAudioFormat) {

    qCDebug(audioclient) << "The desired format for audio I/O is" << desiredAudioFormat;

    adjustedAudioFormat = desiredAudioFormat;

#ifdef Q_OS_ANDROID
    // FIXME: query the native sample rate of the device?
    adjustedAudioFormat.setSampleRate(48000);
#else

    //
    // Attempt the device sample rate in decreasing order of preference.
    // On Windows, using WASAPI shared mode, only a match with the hardware sample rate will succeed.
    //
    if (audioDevice.supportedSampleRates().contains(48000)) {
        adjustedAudioFormat.setSampleRate(48000);
    } else if (audioDevice.supportedSampleRates().contains(44100)) {
        adjustedAudioFormat.setSampleRate(44100);
    } else if (audioDevice.supportedSampleRates().contains(32000)) {
        adjustedAudioFormat.setSampleRate(32000);
    } else if (audioDevice.supportedSampleRates().contains(24000)) {
        adjustedAudioFormat.setSampleRate(24000);
    } else if (audioDevice.supportedSampleRates().contains(16000)) {
        adjustedAudioFormat.setSampleRate(16000);
    } else if (audioDevice.supportedSampleRates().contains(96000)) {
        adjustedAudioFormat.setSampleRate(96000);
    } else if (audioDevice.supportedSampleRates().contains(192000)) {
        adjustedAudioFormat.setSampleRate(192000);
    } else if (audioDevice.supportedSampleRates().contains(88200)) {
        adjustedAudioFormat.setSampleRate(88200);
    } else if (audioDevice.supportedSampleRates().contains(176400)) {
        adjustedAudioFormat.setSampleRate(176400);
    }
#endif

    if (adjustedAudioFormat != desiredAudioFormat) {
        // return the nearest in case it needs 2 channels
        adjustedAudioFormat = audioDevice.nearestFormat(adjustedAudioFormat);
        return true;
    } else {
        return false;
    }
}

bool sampleChannelConversion(const int16_t* sourceSamples, int16_t* destinationSamples, unsigned int numSourceSamples,
                             const QAudioFormat& sourceAudioFormat, const QAudioFormat& destinationAudioFormat) {
    if (sourceAudioFormat.channelCount() == 2 && destinationAudioFormat.channelCount() == 1) {
        // loop through the stereo input audio samples and average every two samples
        for (uint i = 0; i < numSourceSamples; i += 2) {
            destinationSamples[i / 2] = (sourceSamples[i] / 2) + (sourceSamples[i + 1] / 2);
        }

        return true;
    } else if (sourceAudioFormat.channelCount() == 1 && destinationAudioFormat.channelCount() == 2) {

        // loop through the mono input audio and repeat each sample twice
        for (uint i = 0; i < numSourceSamples; ++i) {
            destinationSamples[i * 2] = destinationSamples[(i * 2) + 1] = sourceSamples[i];
        }

        return true;
    }

    return false;
}

void possibleResampling(AudioSRC* resampler,
                        const int16_t* sourceSamples, int16_t* destinationSamples,
                        unsigned int numSourceSamples, unsigned int numDestinationSamples,
                        const QAudioFormat& sourceAudioFormat, const QAudioFormat& destinationAudioFormat) {

    if (numSourceSamples > 0) {
        if (!resampler) {
            if (!sampleChannelConversion(sourceSamples, destinationSamples, numSourceSamples,
                                         sourceAudioFormat, destinationAudioFormat)) {
                // no conversion, we can copy the samples directly across
                memcpy(destinationSamples, sourceSamples, numSourceSamples * sizeof(int16_t));
            }
        } else {

            if (sourceAudioFormat.channelCount() != destinationAudioFormat.channelCount()) {
                float channelCountRatio = (float)destinationAudioFormat.channelCount() / sourceAudioFormat.channelCount();

                int numChannelCoversionSamples = (int)(numSourceSamples * channelCountRatio);
                int16_t* channelConversionSamples = new int16_t[numChannelCoversionSamples];

                sampleChannelConversion(sourceSamples, channelConversionSamples,
                                        numSourceSamples,
                                        sourceAudioFormat, destinationAudioFormat);

                resampler->render(channelConversionSamples, destinationSamples, numChannelCoversionSamples);

                delete[] channelConversionSamples;
            } else {

                unsigned int numAdjustedSourceSamples = numSourceSamples;
                unsigned int numAdjustedDestinationSamples = numDestinationSamples;

                if (sourceAudioFormat.channelCount() == 2 && destinationAudioFormat.channelCount() == 2) {
                    numAdjustedSourceSamples /= 2;
                    numAdjustedDestinationSamples /= 2;
                }

                resampler->render(sourceSamples, destinationSamples, numAdjustedSourceSamples);
            }
        }
    }
}

void AudioClient::start() {

    // set up the desired audio format
    _desiredInputFormat.setSampleRate(AudioConstants::SAMPLE_RATE);
    _desiredInputFormat.setSampleSize(16);
    _desiredInputFormat.setCodec("audio/pcm");
    _desiredInputFormat.setSampleType(QAudioFormat::SignedInt);
    _desiredInputFormat.setByteOrder(QAudioFormat::LittleEndian);
    _desiredInputFormat.setChannelCount(1);

    _desiredOutputFormat = _desiredInputFormat;
    _desiredOutputFormat.setChannelCount(2);

    QAudioDeviceInfo inputDeviceInfo = defaultAudioDeviceForMode(QAudio::AudioInput);
    qCDebug(audioclient) << "The default audio input device is" << inputDeviceInfo.deviceName();
    bool inputFormatSupported = switchInputToAudioDevice(inputDeviceInfo);

    QAudioDeviceInfo outputDeviceInfo = defaultAudioDeviceForMode(QAudio::AudioOutput);
    qCDebug(audioclient) << "The default audio output device is" << outputDeviceInfo.deviceName();
    bool outputFormatSupported = switchOutputToAudioDevice(outputDeviceInfo);

    if (!inputFormatSupported) {
        qCDebug(audioclient) << "Unable to set up audio input because of a problem with input format.";
        qCDebug(audioclient) << "The closest format available is" << inputDeviceInfo.nearestFormat(_desiredInputFormat);
    }

    if (!outputFormatSupported) {
        qCDebug(audioclient) << "Unable to set up audio output because of a problem with output format.";
        qCDebug(audioclient) << "The closest format available is" << outputDeviceInfo.nearestFormat(_desiredOutputFormat);
    }
}

void AudioClient::stop() {
    // "switch" to invalid devices in order to shut down the state
    switchInputToAudioDevice(QAudioDeviceInfo());
    switchOutputToAudioDevice(QAudioDeviceInfo());
}

void AudioClient::handleAudioEnvironmentDataPacket(QSharedPointer<ReceivedMessage> message) {

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

void AudioClient::handleAudioDataPacket(QSharedPointer<ReceivedMessage> message) {
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::ReceiveFirstAudioPacket);

    if (_audioOutput) {

        if (!_hasReceivedFirstPacket) {
            _hasReceivedFirstPacket = true;

            // have the audio scripting interface emit a signal to say we just connected to mixer
            emit receivedFirstPacket();
        }

#if DEV_BUILD || PR_BUILD
        _gate.insert(message);
#else
        // Audio output must exist and be correctly set up if we're going to process received audio
        _receivedAudioStream.parseData(*message);
#endif
    }
}

AudioClient::Gate::Gate(AudioClient* audioClient, int threshold) :
    _audioClient(audioClient),
    _threshold(threshold) {}

void AudioClient::Gate::setThreshold(int threshold) {
    flush();
    _threshold = std::max(threshold, 1);
}

void AudioClient::Gate::insert(QSharedPointer<ReceivedMessage> message) {
    // Short-circuit for normal behavior
    if (_threshold == 1) {
        _audioClient->_receivedAudioStream.parseData(*message);
        return;
    }

    _queue.push(message);
    _index++;

    if (_index % _threshold == 0) {
        flush();
    }
}

void AudioClient::Gate::flush() {
    while (!_queue.empty()) {
        _audioClient->_receivedAudioStream.parseData(*_queue.front());
        _queue.pop();
    }
    _index = 0;
}


void AudioClient::handleNoisyMutePacket(QSharedPointer<ReceivedMessage> message) {
    if (!_muted) {
        toggleMute();
        
        // have the audio scripting interface emit a signal to say we were muted by the mixer
        emit mutedByMixer();
    }
}

void AudioClient::handleMuteEnvironmentPacket(QSharedPointer<ReceivedMessage> message) {
    glm::vec3 position;
    float radius;
    
    message->readPrimitive(&position);
    message->readPrimitive(&radius);

    emit muteEnvironmentRequested(position, radius);
}

void AudioClient::negotiateAudioFormat() {
    auto nodeList = DependencyManager::get<NodeList>();
    auto negotiateFormatPacket = NLPacket::create(PacketType::NegotiateAudioFormat);
    auto codecPlugins = PluginManager::getInstance()->getCodecPlugins();
    quint8 numberOfCodecs = (quint8)codecPlugins.size();
    negotiateFormatPacket->writePrimitive(numberOfCodecs);
    for (auto& plugin : codecPlugins) {
        auto codecName = plugin->getName();
        negotiateFormatPacket->writeString(codecName);
    }

    // grab our audio mixer from the NodeList, if it exists
    SharedNodePointer audioMixer = nodeList->soloNodeOfType(NodeType::AudioMixer);

    if (audioMixer) {
        // send off this mute packet
        nodeList->sendPacket(std::move(negotiateFormatPacket), *audioMixer);
    }
}

void AudioClient::handleSelectedAudioFormat(QSharedPointer<ReceivedMessage> message) {
    QString selectedCodecName = message->readString();
    selectAudioFormat(selectedCodecName);
}

void AudioClient::selectAudioFormat(const QString& selectedCodecName) {
    
    _selectedCodecName = selectedCodecName;

    qDebug() << "Selected Codec:" << _selectedCodecName;

    // release any old codec encoder/decoder first...
    if (_codec && _encoder) {
        _codec->releaseEncoder(_encoder);
        _encoder = nullptr;
        _codec = nullptr;
    }
    _receivedAudioStream.cleanupCodec();

    auto codecPlugins = PluginManager::getInstance()->getCodecPlugins();
    for (auto& plugin : codecPlugins) {
        if (_selectedCodecName == plugin->getName()) {
            _codec = plugin;
            _receivedAudioStream.setupCodec(plugin, _selectedCodecName, AudioConstants::STEREO); 
            _encoder = plugin->createEncoder(AudioConstants::SAMPLE_RATE, AudioConstants::MONO);
            qDebug() << "Selected Codec Plugin:" << _codec.get();
            break;
        }
    }

}
   

QString AudioClient::getDefaultDeviceName(QAudio::Mode mode) {
    QAudioDeviceInfo deviceInfo = defaultAudioDeviceForMode(mode);
    return deviceInfo.deviceName();
}

QVector<QString> AudioClient::getDeviceNames(QAudio::Mode mode) {
    QVector<QString> deviceNames;
    foreach(QAudioDeviceInfo audioDevice, getAvailableDevices(mode)) {
        deviceNames << audioDevice.deviceName().trimmed();
    }
    return deviceNames;
}

bool AudioClient::switchInputToAudioDevice(const QString& inputDeviceName) {
    qCDebug(audioclient) << "DEBUG [" << inputDeviceName << "] [" << getNamedAudioDeviceForMode(QAudio::AudioInput, inputDeviceName).deviceName() << "]";
    return switchInputToAudioDevice(getNamedAudioDeviceForMode(QAudio::AudioInput, inputDeviceName));
}

bool AudioClient::switchOutputToAudioDevice(const QString& outputDeviceName) {
    qCDebug(audioclient) << "DEBUG [" << outputDeviceName << "] [" << getNamedAudioDeviceForMode(QAudio::AudioOutput, outputDeviceName).deviceName() << "]";
    return switchOutputToAudioDevice(getNamedAudioDeviceForMode(QAudio::AudioOutput, outputDeviceName));
}

void AudioClient::configureReverb() {
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

    // used only for adding self-reverb to loopback audio
    p.sampleRate = _outputFormat.sampleRate();
    p.wetDryMix = 100.0f;
    p.preDelay = 0.0f;
    p.earlyGain = -96.0f;   // disable ER
    p.lateGain += _reverbOptions->getWetDryMix() * (24.0f/100.0f) - 24.0f;  // -0dB to -24dB, based on wetDryMix
    p.lateMixLeft = 0.0f;
    p.lateMixRight = 0.0f;

    _sourceReverb.setParameters(&p);
}

void AudioClient::updateReverbOptions() {
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

void AudioClient::setReverb(bool reverb) {
    _reverb = reverb;

    if (!_reverb) {
        _sourceReverb.reset();
        _listenerReverb.reset();
    }
}

void AudioClient::setReverbOptions(const AudioEffectOptions* options) {
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

void AudioClient::handleLocalEchoAndReverb(QByteArray& inputByteArray) {
    // If there is server echo, reverb will be applied to the recieved audio stream so no need to have it here.
    bool hasReverb = _reverb || _receivedAudioStream.hasReverb();
    if (_muted || !_audioOutput || (!_shouldEchoLocally && !hasReverb)) {
        return;
    }

    // if this person wants local loopback add that to the locally injected audio
    // if there is reverb apply it to local audio and substract the origin samples

    if (!_loopbackOutputDevice && _loopbackAudioOutput) {
        // we didn't have the loopback output device going so set that up now

        // NOTE: device start() uses the Qt internal device list
        Lock lock(_deviceMutex);
        _loopbackOutputDevice = _loopbackAudioOutput->start();
        lock.unlock();

        if (!_loopbackOutputDevice) {
            return;
        }
    }

    // NOTE: we assume the inputFormat and the outputFormat are the same, since on any modern
    // multimedia OS they should be. If there is a device that this is not true for, we can
    // add back support to do resampling.
    Q_ASSERT(_inputFormat.sampleRate() == _outputFormat.sampleRate());

    static QByteArray loopBackByteArray;

    int numInputSamples = inputByteArray.size() / sizeof(int16_t);
    int numLoopbackSamples = numDestinationSamplesRequired(_inputFormat, _outputFormat, numInputSamples);

    loopBackByteArray.resize(numLoopbackSamples * sizeof(int16_t));

    int16_t* inputSamples = reinterpret_cast<int16_t*>(inputByteArray.data());
    int16_t* loopbackSamples = reinterpret_cast<int16_t*>(loopBackByteArray.data());

    // upmix mono to stereo
    if (!sampleChannelConversion(inputSamples, loopbackSamples, numInputSamples, _inputFormat, _outputFormat)) {
        // no conversion, just copy the samples
        memcpy(loopbackSamples, inputSamples, numInputSamples * sizeof(int16_t));
    }

    // apply stereo reverb at the source, to the loopback audio
    if (!_shouldEchoLocally && hasReverb) {
        updateReverbOptions();
        _sourceReverb.render(loopbackSamples, loopbackSamples, numLoopbackSamples/2);
    }

    _loopbackOutputDevice->write(loopBackByteArray);
}

void AudioClient::handleAudioInput() {
    // input samples required to produce exactly NETWORK_FRAME_SAMPLES of output
    const int inputSamplesRequired = _inputFormat.channelCount() * _inputToNetworkResampler->getMinInput(AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);
    const auto inputAudioSamples = std::unique_ptr<int16_t[]>(new int16_t[inputSamplesRequired]);
    QByteArray inputByteArray = _inputDevice->readAll();

    handleLocalEchoAndReverb(inputByteArray);

    _inputRingBuffer.writeData(inputByteArray.data(), inputByteArray.size());

    float audioInputMsecsRead = inputByteArray.size() / (float)(_inputFormat.bytesForDuration(USECS_PER_MSEC));
    _stats.updateInputMsecsRead(audioInputMsecsRead);

    const int numNetworkBytes = _isStereoInput
        ? AudioConstants::NETWORK_FRAME_BYTES_STEREO
        : AudioConstants::NETWORK_FRAME_BYTES_PER_CHANNEL;
    const int numNetworkSamples = _isStereoInput
        ? AudioConstants::NETWORK_FRAME_SAMPLES_STEREO
        : AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL;

    static int16_t networkAudioSamples[AudioConstants::NETWORK_FRAME_SAMPLES_STEREO];

    while (_inputRingBuffer.samplesAvailable() >= inputSamplesRequired) {

        if (!_muted) {


            //  Increment the time since the last clip
            if (_timeSinceLastClip >= 0.0f) {
                _timeSinceLastClip += (float)numNetworkSamples / (float)AudioConstants::SAMPLE_RATE;
            }

            _inputRingBuffer.readSamples(inputAudioSamples.get(), inputSamplesRequired);
            possibleResampling(_inputToNetworkResampler,
                inputAudioSamples.get(), networkAudioSamples,
                inputSamplesRequired, numNetworkSamples,
                _inputFormat, _desiredInputFormat);

            //  Remove DC offset
            if (!_isStereoInput) {
                _inputGate.removeDCOffset(networkAudioSamples, numNetworkSamples);
            }

            // only impose the noise gate and perform tone injection if we are sending mono audio
            if (!_isStereoInput && _isNoiseGateEnabled) {
                _inputGate.gateSamples(networkAudioSamples, numNetworkSamples);

                // if we performed the noise gate we can get values from it instead of enumerating the samples again
                _lastInputLoudness = _inputGate.getLastLoudness();

                if (_inputGate.clippedInLastFrame()) {
                    _timeSinceLastClip = 0.0f;
                }
            } else {
                float loudness = 0.0f;

                for (int i = 0; i < numNetworkSamples; i++) {
                    int thisSample = std::abs(networkAudioSamples[i]);
                    loudness += (float)thisSample;

                    if (thisSample > (AudioConstants::MAX_SAMPLE_VALUE * AudioNoiseGate::CLIPPING_THRESHOLD)) {
                        _timeSinceLastClip = 0.0f;
                    }
                }

                _lastInputLoudness = fabs(loudness / numNetworkSamples);
            }

            emit inputReceived({ reinterpret_cast<char*>(networkAudioSamples), numNetworkBytes });

        } else {
            // our input loudness is 0, since we're muted
            _lastInputLoudness = 0;
            _timeSinceLastClip = 0.0f;

            _inputRingBuffer.shiftReadPosition(inputSamplesRequired);
        }

        auto packetType = _shouldEchoToServer ?
            PacketType::MicrophoneAudioWithEcho : PacketType::MicrophoneAudioNoEcho;

        if (_lastInputLoudness == 0) {
            packetType = PacketType::SilentAudioFrame;
        }
        Transform audioTransform;
        audioTransform.setTranslation(_positionGetter());
        audioTransform.setRotation(_orientationGetter());
        // FIXME find a way to properly handle both playback audio and user audio concurrently

        QByteArray decocedBuffer(reinterpret_cast<char*>(networkAudioSamples), numNetworkBytes);
        QByteArray encodedBuffer;
        if (_encoder) {
            _encoder->encode(decocedBuffer, encodedBuffer);
        } else {
            encodedBuffer = decocedBuffer;
        }

        emitAudioPacket(encodedBuffer.constData(), encodedBuffer.size(), _outgoingAvatarAudioSequenceNumber, audioTransform, packetType, _selectedCodecName);
        _stats.sentPacket();
    }
}

void AudioClient::handleRecordedAudioInput(const QByteArray& audio) {
    Transform audioTransform;
    audioTransform.setTranslation(_positionGetter());
    audioTransform.setRotation(_orientationGetter());

    QByteArray encodedBuffer;
    if (_encoder) {
        _encoder->encode(audio, encodedBuffer);
    } else {
        encodedBuffer = audio;
    }

    // FIXME check a flag to see if we should echo audio?
    emitAudioPacket(encodedBuffer.data(), encodedBuffer.size(), _outgoingAvatarAudioSequenceNumber, audioTransform, PacketType::MicrophoneAudioWithEcho, _selectedCodecName);
}

void AudioClient::mixLocalAudioInjectors(float* mixBuffer) {

    QVector<AudioInjector*> injectorsToRemove;
    
    // lock the injector vector
    Lock lock(_injectorsMutex);

    for (AudioInjector* injector : getActiveLocalAudioInjectors()) {
        if (injector->getLocalBuffer()) {

            qint64 samplesToRead = injector->isStereo() ? AudioConstants::NETWORK_FRAME_BYTES_STEREO : AudioConstants::NETWORK_FRAME_BYTES_PER_CHANNEL;

            // get one frame from the injector (mono or stereo)
            memset(_scratchBuffer, 0, sizeof(_scratchBuffer));
            if (0 < injector->getLocalBuffer()->readData((char*)_scratchBuffer, samplesToRead)) {
                
                if (injector->isStereo()) {

                    // stereo gets directly mixed into mixBuffer
                    for (int i = 0; i < AudioConstants::NETWORK_FRAME_SAMPLES_STEREO; i++) {
                        mixBuffer[i] += (float)_scratchBuffer[i] * (1/32768.0f);
                    }
                    
                } else {

                    // calculate distance, gain and azimuth for hrtf
                    glm::vec3 relativePosition = injector->getPosition() - _positionGetter();
                    float distance = glm::max(glm::length(relativePosition), EPSILON);
                    float gain = gainForSource(distance, injector->getVolume()); 
                    float azimuth = azimuthForSource(relativePosition);         
                
                    // mono gets spatialized into mixBuffer
                    injector->getLocalHRTF().render(_scratchBuffer, mixBuffer, 1, azimuth, distance, gain, AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);
                }
            
            } else {
                
                qDebug() << "injector has no more data, marking finished for removal";
                injector->finishLocalInjection();
                injectorsToRemove.append(injector);
            }

        } else {
            
            qDebug() << "injector has no local buffer, marking as finished for removal";
            injector->finishLocalInjection();
            injectorsToRemove.append(injector);
        }
    }
    
    for (AudioInjector* injector : injectorsToRemove) {
        qDebug() << "removing injector";
        getActiveLocalAudioInjectors().removeOne(injector);
    }
}

void AudioClient::processReceivedSamples(const QByteArray& decodedBuffer, QByteArray& outputBuffer) {

    const int16_t* decodedSamples = reinterpret_cast<const int16_t*>(decodedBuffer.data());
    assert(decodedBuffer.size() == AudioConstants::NETWORK_FRAME_BYTES_STEREO);

    outputBuffer.resize(_outputFrameSize * sizeof(int16_t));
    int16_t* outputSamples = reinterpret_cast<int16_t*>(outputBuffer.data());

    // convert network audio to float
    for (int i = 0; i < AudioConstants::NETWORK_FRAME_SAMPLES_STEREO; i++) {
        _mixBuffer[i] = (float)decodedSamples[i] * (1/32768.0f);
    }
        
    // mix in active injectors
    if (getActiveLocalAudioInjectors().size() > 0) {
        mixLocalAudioInjectors(_mixBuffer);
    }

    // apply stereo reverb
    bool hasReverb = _reverb || _receivedAudioStream.hasReverb();
    if (hasReverb) {
        updateReverbOptions();
        _listenerReverb.render(_mixBuffer, _mixBuffer, AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);
    }

    if (_networkToOutputResampler) {

        // resample to output sample rate
        _audioLimiter.render(_mixBuffer, _scratchBuffer, AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);
        _networkToOutputResampler->render(_scratchBuffer, outputSamples, AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

    } else {

        // no resampling needed
        _audioLimiter.render(_mixBuffer, outputSamples, AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);
    }
}

void AudioClient::sendMuteEnvironmentPacket() {
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

void AudioClient::toggleMute() {
    _muted = !_muted;
    emit muteToggled();
}

void AudioClient::setIsStereoInput(bool isStereoInput) {
    if (isStereoInput != _isStereoInput) {
        _isStereoInput = isStereoInput;

        if (_isStereoInput) {
            _desiredInputFormat.setChannelCount(2);
        } else {
            _desiredInputFormat.setChannelCount(1);
        }

        // change in channel count for desired input format, restart the input device
        switchInputToAudioDevice(_inputAudioDeviceName);
    }
}


bool AudioClient::outputLocalInjector(bool isStereo, AudioInjector* injector) {
    Lock lock(_injectorsMutex);
    if (injector->getLocalBuffer() && _audioInput ) {
        // just add it to the vector of active local injectors, if 
        // not already there.
        // Since this is invoked with invokeMethod, there _should_ be
        // no reason to lock access to the vector of injectors.
        if (!_activeLocalAudioInjectors.contains(injector)) {
            qDebug() << "adding new injector";
            _activeLocalAudioInjectors.append(injector);
        } else {
            qDebug() << "injector exists in active list already";
        }
        
        return true;

    } else {
        // no local buffer or audio
        return false;
    }
}

void AudioClient::outputFormatChanged() {
    int outputFormatChannelCountTimesSampleRate = _outputFormat.channelCount() * _outputFormat.sampleRate();
    _outputFrameSize = AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL * outputFormatChannelCountTimesSampleRate / _desiredOutputFormat.sampleRate();
    _receivedAudioStream.outputFormatChanged(outputFormatChannelCountTimesSampleRate);
}

bool AudioClient::switchInputToAudioDevice(const QAudioDeviceInfo& inputDeviceInfo) {
    bool supportedFormat = false;

    // cleanup any previously initialized device
    if (_audioInput) {
        // The call to stop() causes _inputDevice to be destructed.
        // That in turn causes it to be disconnected (see for example
        // http://stackoverflow.com/questions/9264750/qt-signals-and-slots-object-disconnect).
        _audioInput->stop();
        _inputDevice = NULL;

        delete _audioInput;
        _audioInput = NULL;
        _numInputCallbackBytes = 0;

        _inputAudioDeviceName = "";
    }

    if (_inputToNetworkResampler) {
        // if we were using an input to network resampler, delete it here
        delete _inputToNetworkResampler;
        _inputToNetworkResampler = NULL;
    }

    if (!inputDeviceInfo.isNull()) {
        qCDebug(audioclient) << "The audio input device " << inputDeviceInfo.deviceName() << "is available.";
        _inputAudioDeviceName = inputDeviceInfo.deviceName().trimmed();

        if (adjustedFormatForAudioDevice(inputDeviceInfo, _desiredInputFormat, _inputFormat)) {
            qCDebug(audioclient) << "The format to be used for audio input is" << _inputFormat;

            // we've got the best we can get for input
            // if required, setup a resampler for this input to our desired network format
            if (_inputFormat != _desiredInputFormat
                && _inputFormat.sampleRate() != _desiredInputFormat.sampleRate()) {
                qCDebug(audioclient) << "Attemping to create a resampler for input format to network format.";

                assert(_inputFormat.sampleSize() == 16);
                assert(_desiredInputFormat.sampleSize() == 16);
                int channelCount = (_inputFormat.channelCount() == 2 && _desiredInputFormat.channelCount() == 2) ? 2 : 1;

                _inputToNetworkResampler = new AudioSRC(_inputFormat.sampleRate(), _desiredInputFormat.sampleRate(), channelCount);

            } else {
                qCDebug(audioclient) << "No resampling required for audio input to match desired network format.";
            }

            // if the user wants stereo but this device can't provide then bail
            if (!_isStereoInput || _inputFormat.channelCount() == 2) {
                _audioInput = new QAudioInput(inputDeviceInfo, _inputFormat, this);
                _numInputCallbackBytes = calculateNumberOfInputCallbackBytes(_inputFormat);
                _audioInput->setBufferSize(_numInputCallbackBytes);

                // how do we want to handle input working, but output not working?
                int numFrameSamples = calculateNumberOfFrameSamples(_numInputCallbackBytes);
                _inputRingBuffer.resizeForFrameSize(numFrameSamples);

                // NOTE: device start() uses the Qt internal device list
                Lock lock(_deviceMutex);
                _inputDevice = _audioInput->start();
                lock.unlock();

                if (_inputDevice) {
                    connect(_inputDevice, SIGNAL(readyRead()), this, SLOT(handleAudioInput()));
                    supportedFormat = true;
                } else {
                    qCDebug(audioclient) << "Error starting audio input -" <<  _audioInput->error();
                }
            }
        }
    }

    return supportedFormat;
}

void AudioClient::outputNotify() {
    int recentUnfulfilled = _audioOutputIODevice.getRecentUnfulfilledReads();
    if (recentUnfulfilled > 0) {
        qCInfo(audioclient, "Starve detected, %d new unfulfilled reads", recentUnfulfilled);

        if (_outputStarveDetectionEnabled.get()) {
            quint64 now = usecTimestampNow() / 1000;
            int dt = (int)(now - _outputStarveDetectionStartTimeMsec);
            if (dt > _outputStarveDetectionPeriodMsec.get()) {
                _outputStarveDetectionStartTimeMsec = now;
                _outputStarveDetectionCount = 0;
            } else {
                _outputStarveDetectionCount += recentUnfulfilled;
                if (_outputStarveDetectionCount > _outputStarveDetectionThreshold.get()) {
                    int oldOutputBufferSizeFrames = _sessionOutputBufferSizeFrames;
                    int newOutputBufferSizeFrames = setOutputBufferSize(oldOutputBufferSizeFrames + 1, false);

                    if (newOutputBufferSizeFrames > oldOutputBufferSizeFrames) {
                        qCInfo(audioclient, "Starve threshold surpassed (%d starves in %d ms)", _outputStarveDetectionCount, dt);
                    }

                    _outputStarveDetectionStartTimeMsec = now;
                    _outputStarveDetectionCount = 0;
                }
            }
        }
    }
}

bool AudioClient::switchOutputToAudioDevice(const QAudioDeviceInfo& outputDeviceInfo) {
    bool supportedFormat = false;

    // cleanup any previously initialized device
    if (_audioOutput) {
        _audioOutput->stop();

        delete _audioOutput;
        _audioOutput = NULL;

        _loopbackOutputDevice = NULL;
        delete _loopbackAudioOutput;
        _loopbackAudioOutput = NULL;
    }

    if (_networkToOutputResampler) {
        // if we were using an input to network resampler, delete it here
        delete _networkToOutputResampler;
        _networkToOutputResampler = NULL;
    }

    if (!outputDeviceInfo.isNull()) {
        qCDebug(audioclient) << "The audio output device " << outputDeviceInfo.deviceName() << "is available.";
        _outputAudioDeviceName = outputDeviceInfo.deviceName().trimmed();

        if (adjustedFormatForAudioDevice(outputDeviceInfo, _desiredOutputFormat, _outputFormat)) {
            qCDebug(audioclient) << "The format to be used for audio output is" << _outputFormat;

            // we've got the best we can get for input
            // if required, setup a resampler for this input to our desired network format
            if (_desiredOutputFormat != _outputFormat
                && _desiredOutputFormat.sampleRate() != _outputFormat.sampleRate()) {
                qCDebug(audioclient) << "Attemping to create a resampler for network format to output format.";

                assert(_desiredOutputFormat.sampleSize() == 16);
                assert(_outputFormat.sampleSize() == 16);
                int channelCount = (_desiredOutputFormat.channelCount() == 2 && _outputFormat.channelCount() == 2) ? 2 : 1;

                _networkToOutputResampler = new AudioSRC(_desiredOutputFormat.sampleRate(), _outputFormat.sampleRate(), channelCount);

            } else {
                qCDebug(audioclient) << "No resampling required for network output to match actual output format.";
            }

            outputFormatChanged();

            // setup our general output device for audio-mixer audio
            _audioOutput = new QAudioOutput(outputDeviceInfo, _outputFormat, this);
            int osDefaultBufferSize = _audioOutput->bufferSize();
            int requestedSize = _sessionOutputBufferSizeFrames *_outputFrameSize * sizeof(int16_t);
            _audioOutput->setBufferSize(requestedSize);

            connect(_audioOutput, &QAudioOutput::notify, this, &AudioClient::outputNotify);

            _audioOutputIODevice.start();

            // NOTE: device start() uses the Qt internal device list
            Lock lock(_deviceMutex);
            _audioOutput->start(&_audioOutputIODevice);
            lock.unlock();

            qCDebug(audioclient) << "Output Buffer capacity in frames: " << _audioOutput->bufferSize() / sizeof(int16_t) / (float)_outputFrameSize <<
                "requested bytes:" << requestedSize << "actual bytes:" << _audioOutput->bufferSize() <<
                "os default:" << osDefaultBufferSize << "period size:" << _audioOutput->periodSize();

            // setup a loopback audio output device
            _loopbackAudioOutput = new QAudioOutput(outputDeviceInfo, _outputFormat, this);


            _timeSinceLastReceived.start();

            supportedFormat = true;
        }
    }

    return supportedFormat;
}

int AudioClient::setOutputBufferSize(int numFrames, bool persist) {
    numFrames = std::min(std::max(numFrames, MIN_AUDIO_OUTPUT_BUFFER_SIZE_FRAMES), MAX_AUDIO_OUTPUT_BUFFER_SIZE_FRAMES);
    if (numFrames != _sessionOutputBufferSizeFrames) {
        qCInfo(audioclient, "Audio output buffer set to %d frames", numFrames);
        _sessionOutputBufferSizeFrames = numFrames;
        if (persist) {
            _outputBufferSizeFrames.set(numFrames);
        }

        if (_audioOutput) {
            // The buffer size can't be adjusted after QAudioOutput::start() has been called, so
            // recreate the device by switching to the default.
            QAudioDeviceInfo outputDeviceInfo = defaultAudioDeviceForMode(QAudio::AudioOutput);
            emit changeDevice(outputDeviceInfo);  // On correct thread, please, as setOutputBufferSize can be called from main thread.
        }
    }
    return numFrames;
}

// The following constant is operating system dependent due to differences in
// the way input audio is handled. The audio input buffer size is inversely
// proportional to the accelerator ratio.

#ifdef Q_OS_WIN
const float AudioClient::CALLBACK_ACCELERATOR_RATIO = 1.0f;
#endif

#ifdef Q_OS_MAC
const float AudioClient::CALLBACK_ACCELERATOR_RATIO = 2.0f;
#endif

#ifdef Q_OS_LINUX
const float AudioClient::CALLBACK_ACCELERATOR_RATIO = 2.0f;
#endif

int AudioClient::calculateNumberOfInputCallbackBytes(const QAudioFormat& format) const {
    int numInputCallbackBytes = (int)(((AudioConstants::NETWORK_FRAME_BYTES_PER_CHANNEL
        * format.channelCount()
        * ((float) format.sampleRate() / AudioConstants::SAMPLE_RATE))
        / CALLBACK_ACCELERATOR_RATIO) + 0.5f);

    return numInputCallbackBytes;
}

int AudioClient::calculateNumberOfFrameSamples(int numBytes) const {
    int frameSamples = (int)(numBytes * CALLBACK_ACCELERATOR_RATIO + 0.5f) / sizeof(int16_t);
    return frameSamples;
}

float AudioClient::getInputRingBufferMsecsAvailable() const {
    int bytesInInputRingBuffer = _inputRingBuffer.samplesAvailable() * sizeof(int16_t);
    float msecsInInputRingBuffer = bytesInInputRingBuffer / (float)(_inputFormat.bytesForDuration(USECS_PER_MSEC));
    return  msecsInInputRingBuffer;
}

float AudioClient::getAudioOutputMsecsUnplayed() const {
    if (!_audioOutput) {
        return 0.0f;
    }
    int bytesAudioOutputUnplayed = _audioOutput->bufferSize() - _audioOutput->bytesFree();
    float msecsAudioOutputUnplayed = bytesAudioOutputUnplayed / (float)_outputFormat.bytesForDuration(USECS_PER_MSEC);
    return msecsAudioOutputUnplayed;
}


float AudioClient::azimuthForSource(const glm::vec3& relativePosition) {
    // copied from AudioMixer, more or less
    glm::quat inverseOrientation = glm::inverse(_orientationGetter());
    
    // compute sample delay for the 2 ears to create phase panning
    glm::vec3 rotatedSourcePosition = inverseOrientation * relativePosition;
    
    // project the rotated source position vector onto x-y plane
    rotatedSourcePosition.y = 0.0f;

    static const float SOURCE_DISTANCE_THRESHOLD = 1e-30f;

    if (glm::length2(rotatedSourcePosition) > SOURCE_DISTANCE_THRESHOLD) {
        
        // produce an oriented angle about the y-axis
        return glm::orientedAngle(glm::vec3(0.0f, 0.0f, -1.0f), glm::normalize(rotatedSourcePosition), glm::vec3(0.0f, -1.0f, 0.0f));
    } else {
        
        // no azimuth if they are in same spot
        return 0.0f; 
    }
}

float AudioClient::gainForSource(float distance, float volume) {

    const float ATTENUATION_BEGINS_AT_DISTANCE = 1.0f;

    // I'm assuming that the AudioMixer's getting of the stream's attenuation
    // factor is basically same as getting volume
    float gain = volume;

    // attenuate based on distance
    if (distance >= ATTENUATION_BEGINS_AT_DISTANCE) {
        gain /= (distance/ATTENUATION_BEGINS_AT_DISTANCE);  // attenuation = -6dB * log2(distance)
    }

    return gain;
}

qint64 AudioClient::AudioOutputIODevice::readData(char * data, qint64 maxSize) {
    auto samplesRequested = maxSize / sizeof(int16_t);
    int samplesPopped;
    int bytesWritten;

    if ((samplesPopped = _receivedAudioStream.popSamples((int)samplesRequested, false)) > 0) {
        AudioRingBuffer::ConstIterator lastPopOutput = _receivedAudioStream.getLastPopOutput();
        lastPopOutput.readSamples((int16_t*)data, samplesPopped);
        bytesWritten = samplesPopped * sizeof(int16_t);
    } else {
        // nothing on network, don't grab anything from injectors, and just return 0s
        // this will flood the log: qCDebug(audioclient, "empty/partial network buffer");
        memset(data, 0, maxSize);
        bytesWritten = maxSize;
    }

    bool wasBufferStarved = _audio->_audioOutput->bufferSize() == _audio->_audioOutput->bytesFree();
    if (wasBufferStarved) {
        _unfulfilledReads++;
    }

    return bytesWritten;
}

// now called from a background thread, to keep blocking operations off the audio thread
void AudioClient::checkDevices() {
    QVector<QString> inputDevices = getDeviceNames(QAudio::AudioInput);
    QVector<QString> outputDevices = getDeviceNames(QAudio::AudioOutput);

    if (inputDevices != _inputDevices || outputDevices != _outputDevices) {
        _inputDevices = inputDevices;
        _outputDevices = outputDevices;

        emit deviceChanged();
    }
}

void AudioClient::loadSettings() {
    _receivedAudioStream.setDynamicJitterBuffers(dynamicJitterBuffers.get());
    _receivedAudioStream.setMaxFramesOverDesired(maxFramesOverDesired.get());
    _receivedAudioStream.setStaticDesiredJitterBufferFrames(staticDesiredJitterBufferFrames.get());
    _receivedAudioStream.setUseStDevForJitterCalc(useStDevForJitterCalc.get());
    _receivedAudioStream.setWindowStarveThreshold(windowStarveThreshold.get());
    _receivedAudioStream.setWindowSecondsForDesiredCalcOnTooManyStarves(
                                                                        windowSecondsForDesiredCalcOnTooManyStarves.get());
    _receivedAudioStream.setWindowSecondsForDesiredReduction(windowSecondsForDesiredReduction.get());
    _receivedAudioStream.setRepetitionWithFade(repetitionWithFade.get());

    qDebug() << "---- Initializing Audio Client ----";
    auto codecPlugins = PluginManager::getInstance()->getCodecPlugins();
    for (auto& plugin : codecPlugins) {
        qDebug() << "Codec available:" << plugin->getName();
    }

}

void AudioClient::saveSettings() {
    dynamicJitterBuffers.set(_receivedAudioStream.getDynamicJitterBuffers());
    maxFramesOverDesired.set(_receivedAudioStream.getMaxFramesOverDesired());
    staticDesiredJitterBufferFrames.set(_receivedAudioStream.getDesiredJitterBufferFrames());
    windowStarveThreshold.set(_receivedAudioStream.getWindowStarveThreshold());
    windowSecondsForDesiredCalcOnTooManyStarves.set(_receivedAudioStream.
                                                    getWindowSecondsForDesiredCalcOnTooManyStarves());
    windowSecondsForDesiredReduction.set(_receivedAudioStream.getWindowSecondsForDesiredReduction());
    repetitionWithFade.set(_receivedAudioStream.getRepetitionWithFade());
}
