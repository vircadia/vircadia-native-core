//
//  AudioClient.cpp
//  interface/src
//
//  Created by Stephen Birarda on 1/22/13.
//  Copyright 2013 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AudioClient.h"

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
#include <plugins/CodecPlugin.h>
#include <plugins/PluginManager.h>
#include <udt/PacketHeaders.h>
#include <SettingHandle.h>
#include <SharedUtil.h>
#include <Transform.h>

#include "AudioClientLogging.h"
#include "AudioLogging.h"
#include "AudioHelpers.h"

#if defined(Q_OS_ANDROID)
#include <QtAndroidExtras/QAndroidJniObject>
#endif

const int AudioClient::MIN_BUFFER_FRAMES = 1;

const int AudioClient::MAX_BUFFER_FRAMES = 20;

#if defined(Q_OS_ANDROID)
static const int CHECK_INPUT_READS_MSECS = 2000;
static const int MIN_READS_TO_CONSIDER_INPUT_ALIVE = 10;
#endif

const AudioClient::AudioPositionGetter  AudioClient::DEFAULT_POSITION_GETTER = []{ return Vectors::ZERO; };
const AudioClient::AudioOrientationGetter AudioClient::DEFAULT_ORIENTATION_GETTER = [] { return Quaternions::IDENTITY; };

Setting::Handle<bool> dynamicJitterBufferEnabled("dynamicJitterBuffersEnabled",
    InboundAudioStream::DEFAULT_DYNAMIC_JITTER_BUFFER_ENABLED);
Setting::Handle<int> staticJitterBufferFrames("staticJitterBufferFrames",
    InboundAudioStream::DEFAULT_STATIC_JITTER_FRAMES);

// protect the Qt internal device list
using Mutex = std::mutex;
using Lock = std::unique_lock<Mutex>;
Mutex _deviceMutex;
Mutex _recordMutex;

QString defaultAudioDeviceName(QAudio::Mode mode);

void AudioClient::setHmdAudioName(QAudio::Mode mode, const QString& name) {
    QWriteLocker lock(&_hmdNameLock);
    if (mode == QAudio::AudioInput) {
        _hmdInputName = name;
    } else {
        _hmdOutputName = name;
    }
}

// thread-safe
QList<HifiAudioDeviceInfo> getAvailableDevices(QAudio::Mode mode, const QString& hmdName) {
    //get hmd device name prior to locking device mutex. in case of shutdown, this thread will be locked and audio client
    //cannot properly shut down. 
    QString defDeviceName = defaultAudioDeviceName(mode);

    // NOTE: availableDevices() clobbers the Qt internal device list
    Lock lock(_deviceMutex);
    auto devices = QAudioDeviceInfo::availableDevices(mode);

    HifiAudioDeviceInfo defaultDesktopDevice;
    QList<HifiAudioDeviceInfo> newDevices;
    for (auto& device : devices) {
        newDevices.push_back(HifiAudioDeviceInfo(device, false, mode));
        if (device.deviceName() == defDeviceName.trimmed()) {
            defaultDesktopDevice = HifiAudioDeviceInfo(device, true, mode, HifiAudioDeviceInfo::desktop);
        }
    }

    if (defaultDesktopDevice.getDevice().isNull()) {
        if (devices.size() > 0) {
            qCDebug(audioclient) << __FUNCTION__ << "Default device not found in list:" << defDeviceName
                << "Setting Default to: " << devices.first().deviceName();
            newDevices.push_front(HifiAudioDeviceInfo(devices.first(), true, mode, HifiAudioDeviceInfo::desktop));
        } else {
            //current audio list is empty for some reason.
            qCDebug(audioclient) << __FUNCTION__ << "Default device not found in list no alternative selection available";
        }
    } else {
        newDevices.push_front(defaultDesktopDevice);
    }

    if (!hmdName.isNull()) {
        HifiAudioDeviceInfo hmdDevice;
        foreach(auto device, newDevices) {
            if (device.getDevice().deviceName() == hmdName) {
                hmdDevice = HifiAudioDeviceInfo(device.getDevice(), true, mode, HifiAudioDeviceInfo::hmd);
                break;
            }
        }
        
        if (!hmdDevice.getDevice().isNull()) {
            newDevices.push_front(hmdDevice);
        }
    }
    return newDevices;
}

// now called from a background thread, to keep blocking operations off the audio thread
void AudioClient::checkDevices() {
    // Make sure we're not shutting down
    Lock timerMutex(_checkDevicesMutex);
    // If we HAVE shut down after we were queued, but prior to execution, early exit
    if (nullptr == _checkDevicesTimer) {
        return;
    }

    QString hmdInputName;
    QString hmdOutputName;
    {
        QReadLocker readLock(&_hmdNameLock);
        hmdInputName = _hmdInputName;
        hmdOutputName = _hmdOutputName;
    }

    auto inputDevices = getAvailableDevices(QAudio::AudioInput, hmdInputName);
    auto outputDevices = getAvailableDevices(QAudio::AudioOutput, hmdOutputName);
   
    static const QMetaMethod devicesChangedSig= QMetaMethod::fromSignal(&AudioClient::devicesChanged);
    //only emit once the scripting interface has connected to the signal
    if (isSignalConnected(devicesChangedSig)) {
        Lock lock(_deviceMutex);
        if (inputDevices != _inputDevices) {
            _inputDevices.swap(inputDevices);
            emit devicesChanged(QAudio::AudioInput, _inputDevices);
        }

        if (outputDevices != _outputDevices) {
            _outputDevices.swap(outputDevices);
            emit devicesChanged(QAudio::AudioOutput, _outputDevices);
        }
    } 
}

HifiAudioDeviceInfo AudioClient::getActiveAudioDevice(QAudio::Mode mode) const {
    Lock lock(_deviceMutex);

    if (mode == QAudio::AudioInput) {
        return _inputDeviceInfo;
    } else {
        return _outputDeviceInfo;
    }
}

QList<HifiAudioDeviceInfo> AudioClient::getAudioDevices(QAudio::Mode mode) const {
    Lock lock(_deviceMutex);

    if (mode == QAudio::AudioInput) {
        return _inputDevices;
    } else {
        return _outputDevices;
    }
}

static void channelUpmix(int16_t* source, int16_t* dest, int numSamples, int numExtraChannels) {
    for (int i = 0; i < numSamples/2; i++) {

        // read 2 samples
        int16_t left = *source++;
        int16_t right = *source++;

        // write 2 + N samples
        *dest++ = left;
        *dest++ = right;
        for (int n = 0; n < numExtraChannels; n++) {
            *dest++ = 0;
        }
    }
}

static void channelDownmix(int16_t* source, int16_t* dest, int numSamples) {
    for (int i = 0; i < numSamples/2; i++) {

        // read 2 samples
        int16_t left = *source++;
        int16_t right = *source++;

        // write 1 sample
        *dest++ = (int16_t)((left + right) / 2);
    }
}

static bool detectClipping(int16_t* samples, int numSamples, int numChannels) {

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

static float computeLoudness(int16_t* samples, int numSamples) {

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

static inline float convertToFloat(int16_t sample) {
    return (float)sample * (1 / 32768.0f);
}

AudioClient::AudioClient() {

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

    connect(&_receivedAudioStream, &MixedProcessedAudioStream::processSamples,
	    this, &AudioClient::processReceivedSamples, Qt::DirectConnection);
    connect(this, &AudioClient::changeDevice, this, [=](const HifiAudioDeviceInfo& outputDeviceInfo) {
        qCDebug(audioclient)<< "got AudioClient::changeDevice signal, about to call switchOutputToAudioDevice() outputDeviceInfo: ["<< outputDeviceInfo.deviceName() << "]";
        switchOutputToAudioDevice(outputDeviceInfo);
    });

    connect(&_receivedAudioStream, &InboundAudioStream::mismatchedAudioCodec, this, &AudioClient::handleMismatchAudioFormat);

    // initialize wasapi; if getAvailableDevices is called from the CheckDevicesThread before this, it will crash
    defaultAudioDeviceName(QAudio::AudioInput);
    defaultAudioDeviceName(QAudio::AudioOutput);

    // start a thread to detect any device changes
    _checkDevicesTimer = new QTimer(this);
    const unsigned long DEVICE_CHECK_INTERVAL_MSECS = 2 * 1000;
    connect(_checkDevicesTimer, &QTimer::timeout, this, [=] {
        QtConcurrent::run(QThreadPool::globalInstance(), [=] {
            checkDevices();
            // On some systems (Ubuntu) checking all the audio devices can take more than 2 seconds.  To
            // avoid consuming all of the thread pool, don't start the check interval until the previous
            // check has completed.
            QMetaObject::invokeMethod(_checkDevicesTimer, "start", Q_ARG(int, DEVICE_CHECK_INTERVAL_MSECS));
        });
    });
    _checkDevicesTimer->setSingleShot(true);
    _checkDevicesTimer->start(DEVICE_CHECK_INTERVAL_MSECS);

    // start a thread to detect peak value changes
    _checkPeakValuesTimer = new QTimer(this);
    connect(_checkPeakValuesTimer, &QTimer::timeout, this, [this] {
        QtConcurrent::run(QThreadPool::globalInstance(), [this] { checkPeakValues(); });
    });
    const unsigned long PEAK_VALUES_CHECK_INTERVAL_MSECS = 50;
    _checkPeakValuesTimer->start(PEAK_VALUES_CHECK_INTERVAL_MSECS);

    configureReverb();

#if defined(WEBRTC_AUDIO)
    configureWebrtc();
#endif

    auto nodeList = DependencyManager::get<NodeList>();
    auto& packetReceiver = nodeList->getPacketReceiver();
    packetReceiver.registerListener(PacketType::AudioStreamStats,
        PacketReceiver::makeSourcedListenerReference<AudioIOStats>(&_stats, &AudioIOStats::processStreamStatsPacket));
    packetReceiver.registerListener(PacketType::AudioEnvironment,
        PacketReceiver::makeUnsourcedListenerReference<AudioClient>(this, &AudioClient::handleAudioEnvironmentDataPacket));
    packetReceiver.registerListener(PacketType::SilentAudioFrame,
        PacketReceiver::makeUnsourcedListenerReference<AudioClient>(this, &AudioClient::handleAudioDataPacket));
    packetReceiver.registerListener(PacketType::MixedAudio,
        PacketReceiver::makeUnsourcedListenerReference<AudioClient>(this, &AudioClient::handleAudioDataPacket));
    packetReceiver.registerListener(PacketType::NoisyMute,
        PacketReceiver::makeUnsourcedListenerReference<AudioClient>(this, &AudioClient::handleNoisyMutePacket));
    packetReceiver.registerListener(PacketType::MuteEnvironment,
        PacketReceiver::makeUnsourcedListenerReference<AudioClient>(this, &AudioClient::handleMuteEnvironmentPacket));
    packetReceiver.registerListener(PacketType::SelectedAudioFormat,
        PacketReceiver::makeUnsourcedListenerReference<AudioClient>(this, &AudioClient::handleSelectedAudioFormat));

    auto& domainHandler = nodeList->getDomainHandler();
    connect(&domainHandler, &DomainHandler::disconnectedFromDomain, this, [this] { 
        _solo.reset();
    });
    connect(nodeList.data(), &NodeList::nodeActivated, this, [this](SharedNodePointer node) {
        if (node->getType() == NodeType::AudioMixer) {
            _solo.resend();
        }
    });
}

AudioClient::~AudioClient() {

    stop();

    if (_codec && _encoder) {
        _codec->releaseEncoder(_encoder);
        _encoder = nullptr;
    }
}

void AudioClient::customDeleter() {
#if defined(Q_OS_ANDROID)
    _shouldRestartInputSetup = false;
#endif
    deleteLater();
}

void AudioClient::handleMismatchAudioFormat(SharedNodePointer node, const QString& currentCodec, const QString& recievedCodec) {
    qCDebug(audioclient) << __FUNCTION__ << "sendingNode:" << *node << "currentCodec:" << currentCodec << "recievedCodec:" << recievedCodec;
    selectAudioFormat(recievedCodec);
}


void AudioClient::reset() {
    _receivedAudioStream.reset();
    _stats.reset();
    _sourceReverb.reset();
    _listenerReverb.reset();
    _localReverb.reset();
}

void AudioClient::audioMixerKilled() {
    _hasReceivedFirstPacket = false;
    _outgoingAvatarAudioSequenceNumber = 0;
    _stats.reset();
    emit disconnected();
}

void AudioClient::setAudioPaused(bool pause) {
    if (_audioPaused != pause) {
        _audioPaused = pause;

        if (!_audioPaused) {
            negotiateAudioFormat();
        }
    }
}

HifiAudioDeviceInfo getNamedAudioDeviceForMode(QAudio::Mode mode, const QString& deviceName, const QString& hmdName, bool isHmd=false) {
    HifiAudioDeviceInfo result;
    foreach (HifiAudioDeviceInfo audioDevice, getAvailableDevices(mode,hmdName)) {
        if (audioDevice.deviceName().trimmed() == deviceName.trimmed()) {
            if ((!isHmd && audioDevice.getDeviceType() != HifiAudioDeviceInfo::hmd) || (isHmd && audioDevice.getDeviceType() != HifiAudioDeviceInfo::desktop)) {
                result = audioDevice;
                break;
            }
        }
    }
    return result;
}

#ifdef Q_OS_WIN
QString getWinDeviceName(IMMDevice* pEndpoint) {
    QString deviceName;
    IPropertyStore* pPropertyStore;
    pEndpoint->OpenPropertyStore(STGM_READ, &pPropertyStore);
    PROPVARIANT pv;
    PropVariantInit(&pv);
    HRESULT hr = pPropertyStore->GetValue(PKEY_Device_FriendlyName, &pv);
    pPropertyStore->Release();
    pPropertyStore = nullptr;
    deviceName = QString::fromWCharArray((wchar_t*)pv.pwszVal);
    if (!IsWindows8OrGreater()) {
        // Windows 7 provides only the 31 first characters of the device name.
        const DWORD QT_WIN7_MAX_AUDIO_DEVICENAME_LEN = 31;
        deviceName = deviceName.left(QT_WIN7_MAX_AUDIO_DEVICENAME_LEN);
    }
    PropVariantClear(&pv);
    return deviceName;
}

QString AudioClient::getWinDeviceName(wchar_t* guid) {
    QString deviceName;
    HRESULT hr = S_OK;
    CoInitialize(nullptr);
    IMMDeviceEnumerator* pMMDeviceEnumerator = nullptr;
    CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pMMDeviceEnumerator);
    IMMDevice* pEndpoint;
    hr = pMMDeviceEnumerator->GetDevice(guid, &pEndpoint);
    if (hr == E_NOTFOUND) {
        printf("Audio Error: device not found\n");
        deviceName = QString("NONE");
    } else {
        deviceName = ::getWinDeviceName(pEndpoint);
        pEndpoint->Release();
        pEndpoint = nullptr;
    }
    pMMDeviceEnumerator->Release();
    pMMDeviceEnumerator = nullptr;
    CoUninitialize();
    return deviceName;
}

#endif

HifiAudioDeviceInfo defaultAudioDeviceForMode(QAudio::Mode mode, const QString& hmdName) {
    QString deviceName = defaultAudioDeviceName(mode);
#if defined (Q_OS_ANDROID)
    if (mode == QAudio::AudioInput) {
        Setting::Handle<bool> enableAEC(SETTING_AEC_KEY, DEFAULT_AEC_ENABLED);
        bool aecEnabled = enableAEC.get();
        auto audioClient = DependencyManager::get<AudioClient>();
        bool headsetOn = audioClient ? audioClient->isHeadsetPluggedIn() : false;
        for (QAudioDeviceInfo inputDevice : QAudioDeviceInfo::availableDevices(mode)) {
            if (((headsetOn || !aecEnabled) && inputDevice.deviceName() == VOICE_RECOGNITION) ||
                ((!headsetOn && aecEnabled) && inputDevice.deviceName() == VOICE_COMMUNICATION)) {
                return HifiAudioDeviceInfo(inputDevice, false, QAudio::AudioInput);
            }
        }
    }
#endif
    return getNamedAudioDeviceForMode(mode, deviceName, hmdName);
}

QString defaultAudioDeviceName(QAudio::Mode mode) {
    QString deviceName;

#ifdef __APPLE__
    QAudioDeviceInfo device;
    if (mode == QAudio::AudioInput) {
        device = QAudioDeviceInfo::defaultInputDevice();
    } else {
        device = QAudioDeviceInfo::defaultOutputDevice();
    }
    if (!device.isNull()) {
        if (!device.deviceName().isEmpty()) {
            deviceName = device.deviceName();
        }
    } else {
        qDebug() << "QT's Default device is null, reverting to platoform code";
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
            CFStringRef devName = NULL;
            propertySize = sizeof(devName);
            propertyAddress.mSelector = kAudioDevicePropertyDeviceNameCFString;
            getPropertyError = AudioObjectGetPropertyData(defaultDeviceID, &propertyAddress, 0,
                                                          NULL, &propertySize, &devName);

            if (!getPropertyError && propertySize) {
                deviceName = CFStringGetCStringPtr(devName, kCFStringEncodingMacRoman);
            }
        }
    }
#endif
#ifdef WIN32
    //Check for Windows Vista or higher, IMMDeviceEnumerator doesn't work below that.
    if (!IsWindowsVistaOrGreater()) { // lower then vista
        if (mode == QAudio::AudioInput) {
            WAVEINCAPS wic;
            // first use WAVE_MAPPER to get the default devices manufacturer ID
            waveInGetDevCaps(WAVE_MAPPER, &wic, sizeof(wic));
            //Use the received manufacturer id to get the device's real name
            waveInGetDevCaps(wic.wMid, &wic, sizeof(wic));
#if !defined(NDEBUG) 
            qCDebug(audioclient) << "input device:" << wic.szPname;
#endif
            deviceName = wic.szPname;
        } else {
            WAVEOUTCAPS woc;
            // first use WAVE_MAPPER to get the default devices manufacturer ID
            waveOutGetDevCaps(WAVE_MAPPER, &woc, sizeof(woc));
            //Use the received manufacturer id to get the device's real name
            waveOutGetDevCaps(woc.wMid, &woc, sizeof(woc));
#if !defined(NDEBUG) 
            qCDebug(audioclient) << "output device:" << woc.szPname;
#endif
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
            deviceName = getWinDeviceName(pEndpoint);
            pEndpoint->Release();
            pEndpoint = nullptr;
        }
        pMMDeviceEnumerator->Release();
        pMMDeviceEnumerator = NULL;
        CoUninitialize();
    }

#if !defined(NDEBUG) 
    qCDebug(audioclient) << "defaultAudioDeviceForMode mode: " << (mode == QAudio::AudioOutput ? "Output" : "Input") 
	<< " [" << deviceName << "] [" << "]";
#endif

#endif

#ifdef Q_OS_LINUX
    if ( mode == QAudio::AudioInput ) {
        deviceName = QAudioDeviceInfo::defaultInputDevice().deviceName();
    } else {
        deviceName = QAudioDeviceInfo::defaultOutputDevice().deviceName();
    }
#endif
   return deviceName;
}

bool AudioClient::getNamedAudioDeviceForModeExists(QAudio::Mode mode, const QString& deviceName) {
    QReadLocker readLock(&_hmdNameLock);
    QString hmdName = mode == QAudio::AudioInput ? _hmdInputName : _hmdOutputName;
    return (getNamedAudioDeviceForMode(mode, deviceName, hmdName).deviceName() == deviceName);
}


// attempt to use the native sample rate and channel count
bool nativeFormatForAudioDevice(const QAudioDeviceInfo& audioDevice, QAudioFormat& audioFormat) {

    audioFormat = audioDevice.preferredFormat();

    // converting to/from this rate must produce an integral number of samples
    if ((audioFormat.sampleRate() <= 0) ||
        (audioFormat.sampleRate() * AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL % AudioConstants::SAMPLE_RATE != 0)) {
        qCWarning(audioclient) << "The native sample rate [" << audioFormat.sampleRate() << "] is not supported.";
        return false;
    }

    audioFormat.setCodec("audio/pcm");
    audioFormat.setSampleSize(16);
    audioFormat.setSampleType(QAudioFormat::SignedInt);
    audioFormat.setByteOrder(QAudioFormat::LittleEndian);

    if (!audioDevice.isFormatSupported(audioFormat)) {
        qCWarning(audioclient) << "The native format is" << audioFormat << "but isFormatSupported() failed.";

        // attempt the native sample rate, with channels forced to 2
        audioFormat.setChannelCount(2);
        if (!audioDevice.isFormatSupported(audioFormat)) {

            // attempt the native sample rate, with channels forced to 1
            audioFormat.setChannelCount(1);
            if (!audioDevice.isFormatSupported(audioFormat)) {
                return false;
            }
        }
    }
    return true;
}

bool adjustedFormatForAudioDevice(const QAudioDeviceInfo& audioDevice,
                                  const QAudioFormat& desiredAudioFormat,
                                  QAudioFormat& adjustedAudioFormat) {

    qCDebug(audioclient) << "The desired format for audio I/O is" << desiredAudioFormat;

#if defined(Q_OS_WIN)
    if (IsWindows8OrGreater()) {
        // On Windows using WASAPI shared-mode, returns the internal mix format
        return nativeFormatForAudioDevice(audioDevice, adjustedAudioFormat);
    }  // else enumerate formats
#endif

#if defined(Q_OS_MAC)
    // Mac OSX returns the preferred CoreAudio format
    return nativeFormatForAudioDevice(audioDevice, adjustedAudioFormat);
#endif

#if defined(Q_OS_ANDROID)
    // As of Qt5.6, Android returns the native OpenSLES sample rate when possible, else 48000
    if (nativeFormatForAudioDevice(audioDevice, adjustedAudioFormat)) {
        return true;
    }  // else enumerate formats
#endif

    adjustedAudioFormat = desiredAudioFormat;

    //
    // Attempt the device sample rate and channel count in decreasing order of preference.
    //
    const int sampleRates[] = { 48000, 44100, 32000, 24000, 16000, 96000, 192000, 88200, 176400 };
    const int inputChannels[] = { 1, 2, 4, 6, 8 };   // prefer mono
    const int outputChannels[] = { 2, 4, 6, 8, 1 };  // prefer stereo, downmix as last resort

    for (int channelCount : (desiredAudioFormat.channelCount() == 1 ? inputChannels : outputChannels)) {
        for (int sampleRate : sampleRates) {

            adjustedAudioFormat.setChannelCount(channelCount);
            adjustedAudioFormat.setSampleRate(sampleRate);

            if (audioDevice.isFormatSupported(adjustedAudioFormat)) {
                return true;
            }
        }
    }

    return false;   // a supported format could not be found
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

void AudioClient::start() {

    // set up the desired audio format
    _desiredInputFormat.setSampleRate(AudioConstants::SAMPLE_RATE);
    _desiredInputFormat.setSampleSize(16);
    _desiredInputFormat.setCodec("audio/pcm");
    _desiredInputFormat.setSampleType(QAudioFormat::SignedInt);
    _desiredInputFormat.setByteOrder(QAudioFormat::LittleEndian);
    _desiredInputFormat.setChannelCount(1);

    _desiredOutputFormat = _desiredInputFormat;
    _desiredOutputFormat.setChannelCount(OUTPUT_CHANNEL_COUNT);
    
    QString inputName;
    QString outputName;
    {
        QReadLocker readLock(&_hmdNameLock);
        inputName = _hmdInputName;
        outputName = _hmdOutputName;
    }
    
    //initialize input to the dummy device to prevent starves
    switchInputToAudioDevice(HifiAudioDeviceInfo());
    switchOutputToAudioDevice(defaultAudioDeviceForMode(QAudio::AudioOutput, QString())); 

#if defined(Q_OS_ANDROID)
    connect(&_checkInputTimer, &QTimer::timeout, this, &AudioClient::checkInputTimeout);
    _checkInputTimer.start(CHECK_INPUT_READS_MSECS);
#endif
}

void AudioClient::stop() {
    qCDebug(audioclient) << "AudioClient::stop(), requesting switchInputToAudioDevice() to shut down";
    switchInputToAudioDevice(HifiAudioDeviceInfo(), true);

    qCDebug(audioclient) << "AudioClient::stop(), requesting switchOutputToAudioDevice() to shut down";
    switchOutputToAudioDevice(HifiAudioDeviceInfo(), true);

    // Stop triggering the checks
    QObject::disconnect(_checkPeakValuesTimer, &QTimer::timeout, nullptr, nullptr);
    QObject::disconnect(_checkDevicesTimer, &QTimer::timeout, nullptr, nullptr);

    // Destruction of the pointers will occur when the parent object (this) is destroyed)
    {
        Lock lock(_checkDevicesMutex);
        _checkDevicesTimer->stop();
        _checkDevicesTimer = nullptr;
    }
    {
        Lock lock(_checkPeakValuesMutex);
        _checkPeakValuesTimer = nullptr;
    }

#if defined(Q_OS_ANDROID)
    _checkInputTimer.stop();
    disconnect(&_checkInputTimer, &QTimer::timeout, 0, 0);
#endif
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
    if (message->getType() == PacketType::SilentAudioFrame) {
        _silentInbound.increment();
    } else {
        _audioInbound.increment();
    }

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

AudioClient::Gate::Gate(AudioClient* audioClient) :
    _audioClient(audioClient) {}

void AudioClient::Gate::setIsSimulatingJitter(bool enable) {
    std::lock_guard<std::mutex> lock(_mutex);
    flush();
    _isSimulatingJitter = enable;
}

void AudioClient::Gate::setThreshold(int threshold) {
    std::lock_guard<std::mutex> lock(_mutex);
    flush();
    _threshold = std::max(threshold, 1);
}

void AudioClient::Gate::insert(QSharedPointer<ReceivedMessage> message) {
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

void AudioClient::Gate::flush() {
    // Send all held packets to the received audio stream to be (eventually) played
    while (!_queue.empty()) {
        _audioClient->_receivedAudioStream.parseData(*_queue.front());
        _queue.pop();
    }
    _index = 0;
}


void AudioClient::handleNoisyMutePacket(QSharedPointer<ReceivedMessage> message) {
    if (!_isMuted) {
        setMuted(true);

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
    const auto& codecPlugins = PluginManager::getInstance()->getCodecPlugins();
    quint8 numberOfCodecs = (quint8)codecPlugins.size();
    negotiateFormatPacket->writePrimitive(numberOfCodecs);
    for (const auto& plugin : codecPlugins) {
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

    qCDebug(audioclient) << "Selected codec:" << _selectedCodecName << "; Is stereo input:" << _isStereoInput;

    // release any old codec encoder/decoder first...
    if (_codec && _encoder) {
        _codec->releaseEncoder(_encoder);
        _encoder = nullptr;
        _codec = nullptr;
    }
    _receivedAudioStream.cleanupCodec();

    const auto& codecPlugins = PluginManager::getInstance()->getCodecPlugins();
    for (const auto& plugin : codecPlugins) {
        if (_selectedCodecName == plugin->getName()) {
            _codec = plugin;
            _receivedAudioStream.setupCodec(plugin, _selectedCodecName, AudioConstants::STEREO);
            _encoder = plugin->createEncoder(AudioConstants::SAMPLE_RATE, _isStereoInput ? AudioConstants::STEREO : AudioConstants::MONO);
            qCDebug(audioclient) << "Selected codec plugin:" << _codec.get();
            break;
        }
    }

}

bool AudioClient::switchAudioDevice(QAudio::Mode mode, const HifiAudioDeviceInfo& deviceInfo) {
    auto device = deviceInfo;
    if (deviceInfo.getDevice().isNull()) {
        qCDebug(audioclient) << __FUNCTION__ << " switching to null device :" 
            << deviceInfo.deviceName() << " : " << deviceInfo.getDevice().deviceName();
    }

    if (mode == QAudio::AudioInput) {
        return switchInputToAudioDevice(device);
    } else {
        return switchOutputToAudioDevice(device);
    }
}

bool AudioClient::switchAudioDevice(QAudio::Mode mode, const QString& deviceName, bool isHmd) {
    QString hmdName;
    {
        QReadLocker readLock(&_hmdNameLock);
        hmdName = mode == QAudio::AudioInput ? _hmdInputName : _hmdOutputName;
    }
    return switchAudioDevice(mode, getNamedAudioDeviceForMode(mode, deviceName, hmdName, isHmd));
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
        _localReverb.reset();
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

void AudioClient::configureWebrtc() {
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
void AudioClient::processWebrtcFarEnd(const int16_t* samples, int numFrames, int numChannels, int sampleRate) {

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

void AudioClient::processWebrtcNearEnd(int16_t* samples, int numFrames, int numChannels, int sampleRate) {

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

void AudioClient::handleLocalEchoAndReverb(QByteArray& inputByteArray) {
    // If there is server echo, reverb will be applied to the recieved audio stream so no need to have it here.
    bool hasReverb = _reverb || _receivedAudioStream.hasReverb();
    if ((_isMuted && !_shouldEchoLocally) || !_audioOutput || (!_shouldEchoLocally && !hasReverb) || !_audioGateOpen) {
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

    // if required, create loopback resampler
    if (_inputFormat.sampleRate() != _outputFormat.sampleRate() && !_loopbackResampler) {
        qCDebug(audioclient) << "Resampling from" << _inputFormat.sampleRate() << "to" << _outputFormat.sampleRate() << "for audio loopback.";
        _loopbackResampler = new AudioSRC(_inputFormat.sampleRate(), _outputFormat.sampleRate(), OUTPUT_CHANNEL_COUNT);
    }

    static QByteArray loopBackByteArray;

    int numInputSamples = inputByteArray.size() / AudioConstants::SAMPLE_SIZE;
    int numInputFrames = numInputSamples / _inputFormat.channelCount();
    int maxLoopbackFrames = _loopbackResampler ? _loopbackResampler->getMaxOutput(numInputFrames) : numInputFrames;
    int maxLoopbackSamples = maxLoopbackFrames * OUTPUT_CHANNEL_COUNT;

    loopBackByteArray.resize(maxLoopbackSamples * AudioConstants::SAMPLE_SIZE);

    int16_t* inputSamples = reinterpret_cast<int16_t*>(inputByteArray.data());
    int16_t* loopbackSamples = reinterpret_cast<int16_t*>(loopBackByteArray.data());

    int numLoopbackSamples = possibleResampling(_loopbackResampler,
                                                inputSamples, loopbackSamples,
                                                numInputSamples, maxLoopbackSamples,
                                                _inputFormat.channelCount(), OUTPUT_CHANNEL_COUNT);

    loopBackByteArray.resize(numLoopbackSamples * AudioConstants::SAMPLE_SIZE);

    // apply stereo reverb at the source, to the loopback audio
    if (!_shouldEchoLocally && hasReverb) {
        updateReverbOptions();
        _sourceReverb.render(loopbackSamples, loopbackSamples, numLoopbackSamples/2);
    }

    // if required, upmix or downmix to deviceChannelCount
    int deviceChannelCount = _outputFormat.channelCount();
    if (deviceChannelCount == OUTPUT_CHANNEL_COUNT) {

        _loopbackOutputDevice->write(loopBackByteArray);

    } else {

        static QByteArray deviceByteArray;

        int numDeviceSamples = (numLoopbackSamples * deviceChannelCount) / OUTPUT_CHANNEL_COUNT;

        deviceByteArray.resize(numDeviceSamples * AudioConstants::SAMPLE_SIZE);

        int16_t* deviceSamples = reinterpret_cast<int16_t*>(deviceByteArray.data());

        if (deviceChannelCount > OUTPUT_CHANNEL_COUNT) {
            channelUpmix(loopbackSamples, deviceSamples, numLoopbackSamples, deviceChannelCount - OUTPUT_CHANNEL_COUNT);
        } else {
            channelDownmix(loopbackSamples, deviceSamples, numLoopbackSamples);
        }
        _loopbackOutputDevice->write(deviceByteArray);
    }
}

float AudioClient::loudnessToLevel(float loudness) {
    float level = loudness * (1 / 32768.0f);  // level in [0, 1]
    level = 6.02059991f * fastLog2f(level);   // convert to dBFS
    level = (level + 48.0f) * (1 / 42.0f);    // map [-48, -6] dBFS to [0, 1]
    return glm::clamp(level, 0.0f, 1.0f);
}

void AudioClient::handleAudioInput(QByteArray& audioBuffer) {
    if (!_audioPaused) {

        bool audioGateOpen = false;

        if (!_isMuted) {
            int16_t* samples = reinterpret_cast<int16_t*>(audioBuffer.data());
            int numSamples = audioBuffer.size() / AudioConstants::SAMPLE_SIZE;
            int numFrames = numSamples / (_isStereoInput ? AudioConstants::STEREO : AudioConstants::MONO);

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

            emit inputReceived(audioBuffer);
        }

        // loudness after mute/gate
        _lastInputLoudness = (_isMuted || !audioGateOpen) ? 0.0f : _lastRawInputLoudness;

        // detect gate opening and closing
        bool openedInLastBlock = !_audioGateOpen && audioGateOpen;  // the gate just opened
        bool closedInLastBlock = _audioGateOpen && !audioGateOpen;  // the gate just closed
        _audioGateOpen = audioGateOpen;

        if (openedInLastBlock) {
            emit noiseGateOpened();
        } else if (closedInLastBlock) {
            emit noiseGateClosed();
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

        Transform audioTransform;
        audioTransform.setTranslation(_positionGetter());
        audioTransform.setRotation(_orientationGetter());

        QByteArray encodedBuffer;
        if (_encoder) {
            _encoder->encode(audioBuffer, encodedBuffer);
        } else {
            encodedBuffer = audioBuffer;
        }

        emitAudioPacket(encodedBuffer.data(), encodedBuffer.size(), _outgoingAvatarAudioSequenceNumber, _isStereoInput,
                        audioTransform, avatarBoundingBoxCorner, avatarBoundingBoxScale,
                        packetType, _selectedCodecName);
        _stats.sentPacket();
    }
}

void AudioClient::handleMicAudioInput() {
    if (!_inputDevice || _isPlayingBackRecording) {
        return;
    }

#if defined(Q_OS_ANDROID)
    _inputReadsSinceLastCheck++;
#endif

    // input samples required to produce exactly NETWORK_FRAME_SAMPLES of output
    const int inputSamplesRequired = (_inputToNetworkResampler ?
                                      _inputToNetworkResampler->getMinInput(AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL) :
                                      AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL) * _inputFormat.channelCount();

    const auto inputAudioSamples = std::unique_ptr<int16_t[]>(new int16_t[inputSamplesRequired]);
    QByteArray inputByteArray = _inputDevice->readAll();

    handleLocalEchoAndReverb(inputByteArray);

    _inputRingBuffer.writeData(inputByteArray.data(), inputByteArray.size());

    float audioInputMsecsRead = inputByteArray.size() / (float)(_inputFormat.bytesForDuration(USECS_PER_MSEC));
    _stats.updateInputMsRead(audioInputMsecsRead);

    const int numNetworkBytes = _isStereoInput
        ? AudioConstants::NETWORK_FRAME_BYTES_STEREO
        : AudioConstants::NETWORK_FRAME_BYTES_PER_CHANNEL;
    const int numNetworkSamples = _isStereoInput
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

        emit inputLoudnessChanged(_lastSmoothedRawInputLoudness, isClipping);

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

void AudioClient::handleDummyAudioInput() {
    const int numNetworkBytes = _isStereoInput
        ? AudioConstants::NETWORK_FRAME_BYTES_STEREO
        : AudioConstants::NETWORK_FRAME_BYTES_PER_CHANNEL;

    QByteArray audioBuffer(numNetworkBytes, 0);  // silent
    handleAudioInput(audioBuffer);
}

void AudioClient::handleRecordedAudioInput(const QByteArray& audio) {
    QByteArray audioBuffer(audio);
    handleAudioInput(audioBuffer);
}

void AudioClient::prepareLocalAudioInjectors(std::unique_ptr<Lock> localAudioLock) {
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

bool AudioClient::mixLocalAudioInjectors(float* mixBuffer) {
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

void AudioClient::processReceivedSamples(const QByteArray& decodedBuffer, QByteArray& outputBuffer) {

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

void AudioClient::setMuted(bool muted, bool emitSignal) {
    if (_isMuted != muted) {
        _isMuted = muted;
        if (emitSignal) {
            emit muteToggled(_isMuted);
        }
    }
}

void AudioClient::setNoiseReduction(bool enable, bool emitSignal) {
    if (_isNoiseGateEnabled != enable) {
        _isNoiseGateEnabled = enable;
        if (emitSignal) {
            emit noiseReductionChanged(_isNoiseGateEnabled);
        }
    }
}

void AudioClient::setNoiseReductionAutomatic(bool enable, bool emitSignal) {
    if (_isNoiseReductionAutomatic != enable) {
        _isNoiseReductionAutomatic = enable;
        if (emitSignal) {
            emit noiseReductionAutomaticChanged(_isNoiseReductionAutomatic);
        }
    }
}

void AudioClient::setNoiseReductionThreshold(float threshold, bool emitSignal) {
    if (_noiseReductionThreshold != threshold) {
        _noiseReductionThreshold = threshold;
        if (emitSignal) {
            emit noiseReductionThresholdChanged(_noiseReductionThreshold);
        }
    }
}

void AudioClient::setWarnWhenMuted(bool enable, bool emitSignal) {
    if (_warnWhenMuted != enable) {
        _warnWhenMuted = enable;
        if (emitSignal) {
            emit warnWhenMutedChanged(_warnWhenMuted);
        }
    }
}

void AudioClient::setAcousticEchoCancellation(bool enable, bool emitSignal) {
    if (_isAECEnabled != enable) {
        _isAECEnabled = enable;
        if (emitSignal) {
            emit acousticEchoCancellationChanged(_isAECEnabled);
        }
    }
}

bool AudioClient::setIsStereoInput(bool isStereoInput) {
    bool stereoInputChanged = false;
    if (isStereoInput != _isStereoInput && _inputDeviceInfo.getDevice().supportedChannelCounts().contains(2)) {
        _isStereoInput = isStereoInput;
        stereoInputChanged = true;

        if (_isStereoInput) {
            _desiredInputFormat.setChannelCount(2);
        } else {
            _desiredInputFormat.setChannelCount(1);
        }

        // restart the codec
        if (_codec) {
            if (_encoder) {
                _codec->releaseEncoder(_encoder);
            }
            _encoder = _codec->createEncoder(AudioConstants::SAMPLE_RATE, _isStereoInput ? AudioConstants::STEREO : AudioConstants::MONO);
        }
        qCDebug(audioclient) << "Reset Codec:" << _selectedCodecName << "isStereoInput:" << _isStereoInput;

        // restart the input device
        switchInputToAudioDevice(_inputDeviceInfo);

        emit isStereoInputChanged(_isStereoInput);
    }

    return stereoInputChanged;
}

bool AudioClient::outputLocalInjector(const AudioInjectorPointer& injector) {
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

int AudioClient::getNumLocalInjectors() {
    Lock lock(_injectorsMutex);
    return _activeLocalAudioInjectors.size();
}

void AudioClient::outputFormatChanged() {
    _outputFrameSize = (AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL * OUTPUT_CHANNEL_COUNT * _outputFormat.sampleRate()) /
        _desiredOutputFormat.sampleRate();
    _receivedAudioStream.outputFormatChanged(_outputFormat.sampleRate(), OUTPUT_CHANNEL_COUNT);
}

bool AudioClient::switchInputToAudioDevice(const HifiAudioDeviceInfo inputDeviceInfo, bool isShutdownRequest) {
    Q_ASSERT_X(QThread::currentThread() == thread(), Q_FUNC_INFO, "Function invoked on wrong thread");

    qCDebug(audioclient) << __FUNCTION__ << "_inputDeviceInfo: [" << _inputDeviceInfo.deviceName() << ":" << _inputDeviceInfo.getDevice().deviceName() 
        << "-- inputDeviceInfo:" << inputDeviceInfo.deviceName() << ":" << inputDeviceInfo.getDevice().deviceName() << "]";
    bool supportedFormat = false;

    // NOTE: device start() uses the Qt internal device list
    Lock lock(_deviceMutex);

#if defined(Q_OS_ANDROID)
    _shouldRestartInputSetup = false;  // avoid a double call to _audioInput->start() from audioInputStateChanged
#endif

    // cleanup any previously initialized device
    if (_audioInput) {
        // The call to stop() causes _inputDevice to be destructed.
        // That in turn causes it to be disconnected (see for example
        // http://stackoverflow.com/questions/9264750/qt-signals-and-slots-object-disconnect).
        _audioInput->stop();
        _inputDevice = NULL;

        _audioInput->deleteLater();
        _audioInput = NULL;
        _numInputCallbackBytes = 0;

        _inputDeviceInfo.setDevice(QAudioDeviceInfo());
    }

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

    if (_loopbackResampler) {
        delete _loopbackResampler;
        _loopbackResampler = NULL;
    }

    if (_audioGate) {
        delete _audioGate;
        _audioGate = nullptr;
    }

    if (isShutdownRequest) {
        qCDebug(audioclient) << "The audio input device has shut down.";
        return true;
    }

    if (!inputDeviceInfo.getDevice().isNull()) {
        qCDebug(audioclient) << "The audio input device" << inputDeviceInfo.deviceName() << ":" << inputDeviceInfo.getDevice().deviceName() << "is available.";
      
        //do not update UI that we're changing devices if default or same device
        _inputDeviceInfo = inputDeviceInfo;
        emit deviceChanged(QAudio::AudioInput, _inputDeviceInfo);

        if (adjustedFormatForAudioDevice(_inputDeviceInfo.getDevice(), _desiredInputFormat, _inputFormat)) {
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

            // the audio gate runs after the resampler
            _audioGate = new AudioGate(_desiredInputFormat.sampleRate(), _desiredInputFormat.channelCount());
            qCDebug(audioclient) << "Noise gate created with" << _desiredInputFormat.channelCount() << "channels.";

            // if the user wants stereo but this device can't provide then bail
            if (!_isStereoInput || _inputFormat.channelCount() == 2) {
                _audioInput = new QAudioInput(_inputDeviceInfo.getDevice(), _inputFormat, this);
                _numInputCallbackBytes = calculateNumberOfInputCallbackBytes(_inputFormat);
                _audioInput->setBufferSize(_numInputCallbackBytes);
                // different audio input devices may have different volumes
                emit inputVolumeChanged(_audioInput->volume());

                // how do we want to handle input working, but output not working?
                int numFrameSamples = calculateNumberOfFrameSamples(_numInputCallbackBytes);
                _inputRingBuffer.resizeForFrameSize(numFrameSamples);

#if defined(Q_OS_ANDROID)
                if (_audioInput) {
                    _shouldRestartInputSetup = true;
                    connect(_audioInput, &QAudioInput::stateChanged, this, &AudioClient::audioInputStateChanged);
                }
#endif
                _inputDevice = _audioInput->start();

                if (_inputDevice) {
                    connect(_inputDevice, SIGNAL(readyRead()), this, SLOT(handleMicAudioInput()));
                    supportedFormat = true;
                } else {
                    qCDebug(audioclient) << "Error starting audio input -" << _audioInput->error();
                    _audioInput->deleteLater();
                    _audioInput = NULL;
                }
            }
        }
    }

    // If there is no working input device, use the dummy input device.
    // It generates audio callbacks on a timer to simulate a mic stream of silent packets.
    // This enables clients without a mic to still receive an audio stream from the mixer.
    if (!_audioInput) {
        qCDebug(audioclient) << "Audio input device is not available, using dummy input.";
        _inputDeviceInfo.setDevice(QAudioDeviceInfo());
        emit deviceChanged(QAudio::AudioInput, _inputDeviceInfo);

        _inputFormat = _desiredInputFormat;
        qCDebug(audioclient) << "The format to be used for audio input is" << _inputFormat;
        qCDebug(audioclient) << "No resampling required for audio input to match desired network format.";

        _audioGate = new AudioGate(_desiredInputFormat.sampleRate(), _desiredInputFormat.channelCount());
        qCDebug(audioclient) << "Noise gate created with" << _desiredInputFormat.channelCount() << "channels.";

        // generate audio callbacks at the network sample rate
        _dummyAudioInput = new QTimer(this);
        connect(_dummyAudioInput, SIGNAL(timeout()), this, SLOT(handleDummyAudioInput()));
        _dummyAudioInput->start((int)(AudioConstants::NETWORK_FRAME_MSECS + 0.5f));
    }

    return supportedFormat;
}

void AudioClient::audioInputStateChanged(QAudio::State state) {
#if defined(Q_OS_ANDROID)
    switch (state) {
        case QAudio::StoppedState:
            if (!_audioInput) {
                break;
            }
            // Stopped on purpose
            if (_shouldRestartInputSetup) {
                Lock lock(_deviceMutex);
                _inputDevice = _audioInput->start();
                lock.unlock();
                if (_inputDevice) {
                    connect(_inputDevice, SIGNAL(readyRead()), this, SLOT(handleMicAudioInput()));
                }
            }
            break;
        case QAudio::ActiveState:
            break;
        default:
            break;
    }
#endif
}

void AudioClient::checkInputTimeout() {
#if defined(Q_OS_ANDROID)
    if (_audioInput && _inputReadsSinceLastCheck < MIN_READS_TO_CONSIDER_INPUT_ALIVE) {
        _audioInput->stop();
    } else {
        _inputReadsSinceLastCheck = 0;
    }
#endif
}

void AudioClient::setHeadsetPluggedIn(bool pluggedIn) {
#if defined(Q_OS_ANDROID)
    if (pluggedIn == !_isHeadsetPluggedIn && !_inputDeviceInfo.getDevice().isNull()) {
        QAndroidJniObject brand = QAndroidJniObject::getStaticObjectField<jstring>("android/os/Build", "BRAND");
        // some samsung phones needs more time to shutdown the previous input device
        if (brand.toString().contains("samsung", Qt::CaseInsensitive)) {
            switchInputToAudioDevice(HifiAudioDeviceInfo(), true);
            QThread::msleep(200);
        }

        Setting::Handle<bool> enableAEC(SETTING_AEC_KEY, DEFAULT_AEC_ENABLED);
        bool aecEnabled = enableAEC.get();

        if ((pluggedIn || !aecEnabled) && _inputDeviceInfo.deviceName() != VOICE_RECOGNITION) {
            switchAudioDevice(QAudio::AudioInput, VOICE_RECOGNITION, false);
        } else if (!pluggedIn && aecEnabled && _inputDeviceInfo.deviceName() != VOICE_COMMUNICATION) {
            switchAudioDevice(QAudio::AudioInput, VOICE_COMMUNICATION, false);
        }
    }
    _isHeadsetPluggedIn = pluggedIn;
#endif
}

void AudioClient::outputNotify() {
    int recentUnfulfilled = _audioOutputIODevice.getRecentUnfulfilledReads();
    if (recentUnfulfilled > 0) {
        qCDebug(audioclient, "Starve detected, %d new unfulfilled reads", recentUnfulfilled);

        if (_outputStarveDetectionEnabled.get()) {
            quint64 now = usecTimestampNow() / 1000;
            int dt = (int)(now - _outputStarveDetectionStartTimeMsec);
            if (dt > STARVE_DETECTION_PERIOD) {
                _outputStarveDetectionStartTimeMsec = now;
                _outputStarveDetectionCount = 0;
            } else {
                _outputStarveDetectionCount += recentUnfulfilled;
                if (_outputStarveDetectionCount > STARVE_DETECTION_THRESHOLD) {
                    int oldOutputBufferSizeFrames = _sessionOutputBufferSizeFrames;
                    int newOutputBufferSizeFrames = setOutputBufferSize(oldOutputBufferSizeFrames + 1, false);

                    if (newOutputBufferSizeFrames > oldOutputBufferSizeFrames) {
                        qCDebug(audioclient,
                                "Starve threshold surpassed (%d starves in %d ms)", _outputStarveDetectionCount, dt);
                    }

                    _outputStarveDetectionStartTimeMsec = now;
                    _outputStarveDetectionCount = 0;
                }
            }
        }
    }
}

void AudioClient::noteAwakening() {
    qCDebug(audioclient) << "Restarting the audio devices.";
    switchInputToAudioDevice(_inputDeviceInfo); 
    switchOutputToAudioDevice(_outputDeviceInfo);
}

bool AudioClient::switchOutputToAudioDevice(const HifiAudioDeviceInfo outputDeviceInfo, bool isShutdownRequest) {
    Q_ASSERT_X(QThread::currentThread() == thread(), Q_FUNC_INFO, "Function invoked on wrong thread");
    
    qCDebug(audioclient) << __FUNCTION__ << "_outputdeviceInfo: [" << _outputDeviceInfo.deviceName() << ":" << _outputDeviceInfo.getDevice().deviceName()
        << "-- outputDeviceInfo:" << outputDeviceInfo.deviceName() << ":" << outputDeviceInfo.getDevice().deviceName() << "]";
    bool supportedFormat = false;

    // NOTE: device start() uses the Qt internal device list
    Lock lock(_deviceMutex);

    _localSamplesAvailable.exchange(0, std::memory_order_release);

    //wait on local injectors prep to finish running
    if ( !_localPrepInjectorFuture.isFinished()) {
        _localPrepInjectorFuture.waitForFinished();
    }

    Lock localAudioLock(_localAudioMutex);

    // cleanup any previously initialized device
    if (_audioOutput) {
        _audioOutputIODevice.close();
        _audioOutput->stop();
        _audioOutputInitialized = false;

        //must be deleted in next eventloop cycle when its called from notify()
        _audioOutput->deleteLater();
        _audioOutput = NULL;

        _loopbackOutputDevice = NULL;
        //must be deleted in next eventloop cycle when its called from notify()
        _loopbackAudioOutput->deleteLater();
        _loopbackAudioOutput = NULL;

        delete[] _outputMixBuffer;
        _outputMixBuffer = NULL;

        delete[] _outputScratchBuffer;
        _outputScratchBuffer = NULL;

        delete[] _localOutputMixBuffer;
        _localOutputMixBuffer = NULL;
        
        _outputDeviceInfo.setDevice(QAudioDeviceInfo());
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

    if (_loopbackResampler) {
        delete _loopbackResampler;
        _loopbackResampler = NULL;
    }

    if (isShutdownRequest) {
        qCDebug(audioclient) << "The audio output device has shut down.";
        return true;
    }

    if (!outputDeviceInfo.getDevice().isNull()) {
        qCDebug(audioclient) << "The audio output device" << outputDeviceInfo.deviceName() << ":" << outputDeviceInfo.getDevice().deviceName() << "is available.";
        
        //do not update UI that we're changing devices if default or same device
        _outputDeviceInfo = outputDeviceInfo;
        emit deviceChanged(QAudio::AudioOutput, _outputDeviceInfo);

        if (adjustedFormatForAudioDevice(_outputDeviceInfo.getDevice(), _desiredOutputFormat, _outputFormat)) {
            qCDebug(audioclient) << "The format to be used for audio output is" << _outputFormat;

            // we've got the best we can get for input
            // if required, setup a resampler for this input to our desired network format
            if (_desiredOutputFormat != _outputFormat
                && _desiredOutputFormat.sampleRate() != _outputFormat.sampleRate()) {
                qCDebug(audioclient) << "Attemping to create a resampler for network format to output format.";

                assert(_desiredOutputFormat.sampleSize() == 16);
                assert(_outputFormat.sampleSize() == 16);

                _networkToOutputResampler = new AudioSRC(_desiredOutputFormat.sampleRate(), _outputFormat.sampleRate(), OUTPUT_CHANNEL_COUNT);
                _localToOutputResampler = new AudioSRC(_desiredOutputFormat.sampleRate(), _outputFormat.sampleRate(), OUTPUT_CHANNEL_COUNT);

            } else {
                qCDebug(audioclient) << "No resampling required for network output to match actual output format.";
            }

            outputFormatChanged();

            // setup our general output device for audio-mixer audio
            _audioOutput = new QAudioOutput(_outputDeviceInfo.getDevice(), _outputFormat, this);

            int deviceChannelCount = _outputFormat.channelCount();
            int frameSize = (AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL * deviceChannelCount * _outputFormat.sampleRate()) / _desiredOutputFormat.sampleRate();
            int requestedSize = _sessionOutputBufferSizeFrames * frameSize * AudioConstants::SAMPLE_SIZE;
            _audioOutput->setBufferSize(requestedSize * 16);

            connect(_audioOutput, &QAudioOutput::notify, this, &AudioClient::outputNotify);

            // start the output device
            _audioOutputIODevice.start();
            _audioOutput->start(&_audioOutputIODevice);

            // initialize mix buffers

            // restrict device callback to _outputPeriod samples
            _outputPeriod = _audioOutput->periodSize() / AudioConstants::SAMPLE_SIZE;
            // device callback may exceed reported period, so double it to avoid stutter
            _outputPeriod *= 2;

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

            int bufferSize = _audioOutput->bufferSize();
            int bufferSamples = bufferSize / AudioConstants::SAMPLE_SIZE;
            int bufferFrames = bufferSamples / (float)frameSize;
            qCDebug(audioclient) << "frame (samples):" << frameSize;
            qCDebug(audioclient) << "buffer (frames):" << bufferFrames;
            qCDebug(audioclient) << "buffer (samples):" << bufferSamples;
            qCDebug(audioclient) << "buffer (bytes):" << bufferSize;
            qCDebug(audioclient) << "requested (bytes):" << requestedSize;
            qCDebug(audioclient) << "period (samples):" << _outputPeriod;
            qCDebug(audioclient) << "local buffer (samples):" << localPeriod;

            // unlock to avoid a deadlock with the device callback (which always succeeds this initialization)
            localAudioLock.unlock();

            // setup a loopback audio output device
            _loopbackAudioOutput = new QAudioOutput(outputDeviceInfo.getDevice(), _outputFormat, this);

            _timeSinceLastReceived.start();

            supportedFormat = true;
        }
    }

    return supportedFormat;
}

int AudioClient::setOutputBufferSize(int numFrames, bool persist) {
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

// The following constant is operating system dependent due to differences in
// the way input audio is handled. The audio input buffer size is inversely
// proportional to the accelerator ratio.

#ifdef Q_OS_WIN
const float AudioClient::CALLBACK_ACCELERATOR_RATIO = IsWindows8OrGreater() ? 1.0f : 0.25f;
#endif

#ifdef Q_OS_MAC
const float AudioClient::CALLBACK_ACCELERATOR_RATIO = 2.0f;
#endif

#ifdef Q_OS_ANDROID
const float AudioClient::CALLBACK_ACCELERATOR_RATIO = 0.5f;
#elif defined(Q_OS_LINUX)
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
    int frameSamples = (int)(numBytes * CALLBACK_ACCELERATOR_RATIO + 0.5f) / AudioConstants::SAMPLE_SIZE;
    return frameSamples;
}

float AudioClient::azimuthForSource(const glm::vec3& relativePosition) {
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

float AudioClient::gainForSource(float distance, float volume) {

    // attenuation = -6dB * log2(distance)
    // reference attenuation of 0dB at distance = ATTN_DISTANCE_REF
    float d = (1.0f / ATTN_DISTANCE_REF) * std::max(distance, HRTF_NEARFIELD_MIN);
    float gain = volume / d;
    gain = std::min(gain, ATTN_GAIN_MAX);

    return gain;
}

qint64 AudioClient::AudioOutputIODevice::readData(char * data, qint64 maxSize) {

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

    // if required, upmix or downmix to deviceChannelCount
    if (deviceChannelCount == OUTPUT_CHANNEL_COUNT) {
        memcpy(data, scratchBuffer, samplesPopped * AudioConstants::SAMPLE_SIZE);
    } else if (deviceChannelCount > OUTPUT_CHANNEL_COUNT) {
        int extraChannels = deviceChannelCount - OUTPUT_CHANNEL_COUNT;
        channelUpmix(scratchBuffer, (int16_t*)data, samplesPopped, extraChannels);
    } else {
        channelDownmix(scratchBuffer, (int16_t*)data, samplesPopped);
    }
    int bytesWritten = framesPopped * AudioConstants::SAMPLE_SIZE * deviceChannelCount;
    assert(bytesWritten <= maxSize);

    // send output buffer for recording
    if (_audio->_isRecording) {
        Lock lock(_recordMutex);
        _audio->_audioFileWav.addRawAudioChunk(data, bytesWritten);
    }

    int bytesAudioOutputUnplayed = _audio->_audioOutput->bufferSize() - _audio->_audioOutput->bytesFree();
    float msecsAudioOutputUnplayed = bytesAudioOutputUnplayed / (float)_audio->_outputFormat.bytesForDuration(USECS_PER_MSEC);
    _audio->_stats.updateOutputMsUnplayed(msecsAudioOutputUnplayed);

    if (bytesAudioOutputUnplayed == 0) {
        _unfulfilledReads++;
    }

    return bytesWritten;
}

bool AudioClient::startRecording(const QString& filepath) {
    if (!_audioFileWav.create(_outputFormat, filepath)) {
        qDebug() << "Error creating audio file: " + filepath;
        return false;
    }
    _isRecording = true;
    return true;
}

void AudioClient::stopRecording() {
    if (_isRecording) {
        _isRecording = false;
        _audioFileWav.close();
    }
}

void AudioClient::loadSettings() {
    _receivedAudioStream.setDynamicJitterBufferEnabled(dynamicJitterBufferEnabled.get());
    _receivedAudioStream.setStaticJitterBufferFrames(staticJitterBufferFrames.get());

    qCDebug(audioclient) << "---- Initializing Audio Client ----";
    const auto& codecPlugins = PluginManager::getInstance()->getCodecPlugins();
    for (const auto& plugin : codecPlugins) {
        qCDebug(audioclient) << "Codec available:" << plugin->getName();
    }

}

void AudioClient::saveSettings() {
    dynamicJitterBufferEnabled.set(_receivedAudioStream.dynamicJitterBufferEnabled());
    staticJitterBufferFrames.set(_receivedAudioStream.getStaticJitterBufferFrames());
}

void AudioClient::setAvatarBoundingBoxParameters(glm::vec3 corner, glm::vec3 scale) {
    avatarBoundingBoxCorner = corner;
    avatarBoundingBoxScale = scale;
}


void AudioClient::startThread() {
    moveToNewNamedThread(this, "Audio Thread", [this] { start(); }, QThread::TimeCriticalPriority);
}

void AudioClient::setInputVolume(float volume, bool emitSignal) {
    if (_audioInput && volume != (float)_audioInput->volume()) {
        _audioInput->setVolume(volume);
        if (emitSignal) {
            emit inputVolumeChanged(_audioInput->volume());
        }
    }
}
