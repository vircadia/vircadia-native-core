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

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#endif

extern "C" {
    #include <gverb/gverb.h>
    #include <gverb/gverbdsp.h>
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include <soxr.h>

#include <NodeList.h>
#include <PacketHeaders.h>
#include <PositionalAudioStream.h>
#include <SettingHandle.h>
#include <SharedUtil.h>
#include <UUID.h>

#include "AudioInjector.h"
#include "AudioConstants.h"
#include "PositionalAudioStream.h"
#include "AudioClientLogging.h"

#include "AudioClient.h"

static const int RECEIVED_AUDIO_STREAM_CAPACITY_FRAMES = 100;

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

AudioClient::AudioClient() :
    AbstractAudioInterface(),
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
    _outputBufferSizeFrames("audioOutputBufferSize", DEFAULT_AUDIO_OUTPUT_BUFFER_SIZE_FRAMES),
    _outputStarveDetectionEnabled("audioOutputStarveDetectionEnabled",
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
    _audioSourceInjectEnabled(false),
    _reverb(false),
    _reverbOptions(&_scriptReverbOptions),
    _gverb(NULL),
    _inputToNetworkResampler(NULL),
    _networkToOutputResampler(NULL),
    _loopbackResampler(NULL),
    _noiseSourceEnabled(false),
    _toneSourceEnabled(true),
    _outgoingAvatarAudioSequenceNumber(0),
    _audioOutputIODevice(_receivedAudioStream, this),
    _stats(&_receivedAudioStream),
    _inputGate()
{
    // clear the array of locally injected samples
    memset(_localProceduralSamples, 0, AudioConstants::NETWORK_FRAME_BYTES_PER_CHANNEL);

    connect(&_receivedAudioStream, &MixedProcessedAudioStream::processSamples,
            this, &AudioClient::processReceivedSamples, Qt::DirectConnection);

    _inputDevices = getDeviceNames(QAudio::AudioInput);
    _outputDevices = getDeviceNames(QAudio::AudioOutput);

    const qint64 DEVICE_CHECK_INTERVAL_MSECS = 2 * 1000;
    QTimer* updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &AudioClient::checkDevices);
    updateTimer->start(DEVICE_CHECK_INTERVAL_MSECS);

    // create GVerb filter
    _gverb = createGverbFilter();
    configureGverbFilter(_gverb);
}

AudioClient::~AudioClient() {
    stop();

    if (_gverb) {
        gverb_free(_gverb);
    }
}

void AudioClient::reset() {
    _receivedAudioStream.reset();
    _stats.reset();
    _noiseSource.reset();
    _toneSource.reset();
    _sourceGain.reset();
    _inputGain.reset();

    gverb_flush(_gverb);
}

void AudioClient::audioMixerKilled() {
    _hasReceivedFirstPacket = false;
    _outgoingAvatarAudioSequenceNumber = 0;
    _stats.reset();
    emit disconnected();
}


QAudioDeviceInfo getNamedAudioDeviceForMode(QAudio::Mode mode, const QString& deviceName) {
    QAudioDeviceInfo result;
    foreach(QAudioDeviceInfo audioDevice, QAudioDeviceInfo::availableDevices(mode)) {
        if (audioDevice.deviceName().trimmed() == deviceName.trimmed()) {
            result = audioDevice;
            break;
        }
    }

    return result;
}

soxr_datatype_t soxrDataTypeFromQAudioFormat(const QAudioFormat& audioFormat) {
    if (audioFormat.sampleType() == QAudioFormat::Float) {
        return SOXR_FLOAT32_I;
    } else {
        if (audioFormat.sampleSize() == 16) {
            return SOXR_INT16_I;
        } else {
            return SOXR_INT32_I;
        }
    }
}

int numDestinationSamplesRequired(const QAudioFormat& sourceFormat, const QAudioFormat& destinationFormat,
                                  int numSourceSamples) {
    float ratio = (float) destinationFormat.channelCount() / sourceFormat.channelCount();
    ratio *= (float) destinationFormat.sampleRate() / sourceFormat.sampleRate();

    return (numSourceSamples * ratio) + 0.5f;
}

QAudioDeviceInfo defaultAudioDeviceForMode(QAudio::Mode mode) {
#ifdef __APPLE__
    if (QAudioDeviceInfo::availableDevices(mode).size() > 1) {
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
                foreach(QAudioDeviceInfo audioDevice, QAudioDeviceInfo::availableDevices(mode)) {
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
            IPropertyStore* pPropertyStore;
            pEndpoint->OpenPropertyStore(STGM_READ, &pPropertyStore);
            pEndpoint->Release();
            pEndpoint = NULL;
            PROPVARIANT pv;
            PropVariantInit(&pv);
            hr = pPropertyStore->GetValue(PKEY_Device_FriendlyName, &pv);
            pPropertyStore->Release();
            pPropertyStore = NULL;
            deviceName = QString::fromWCharArray((wchar_t*)pv.pwszVal);
            if (!IsWindows8OrGreater()) {
                // Windows 7 provides only the 31 first characters of the device name.
                const DWORD QT_WIN7_MAX_AUDIO_DEVICENAME_LEN = 31;
                deviceName = deviceName.left(QT_WIN7_MAX_AUDIO_DEVICENAME_LEN);
            }
            qCDebug(audioclient) << (mode == QAudio::AudioOutput ? "output" : "input") << " device:" << deviceName;
            PropVariantClear(&pv);
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
    if (!audioDevice.isFormatSupported(desiredAudioFormat)) {
        qCDebug(audioclient) << "The desired format for audio I/O is" << desiredAudioFormat;
        qCDebug(audioclient, "The desired audio format is not supported by this device");

        if (desiredAudioFormat.channelCount() == 1) {
            adjustedAudioFormat = desiredAudioFormat;
            adjustedAudioFormat.setChannelCount(2);

            if (audioDevice.isFormatSupported(adjustedAudioFormat)) {
                return true;
            } else {
                adjustedAudioFormat.setChannelCount(1);
            }
        }

        const int FORTY_FOUR = 44100;

        adjustedAudioFormat = desiredAudioFormat;

#ifdef Q_OS_ANDROID
        adjustedAudioFormat.setSampleRate(FORTY_FOUR);
#else

        const int HALF_FORTY_FOUR = FORTY_FOUR / 2;

        if (audioDevice.supportedSampleRates().contains(AudioConstants::SAMPLE_RATE * 2)) {
            // use 48, which is a sample downsample, upsample
            adjustedAudioFormat.setSampleRate(AudioConstants::SAMPLE_RATE * 2);
        } else if (audioDevice.supportedSampleRates().contains(HALF_FORTY_FOUR)) {
            // use 22050, resample but closer to 24
            adjustedAudioFormat.setSampleRate(HALF_FORTY_FOUR);
        } else if (audioDevice.supportedSampleRates().contains(FORTY_FOUR)) {
            // use 48000, libsoxr will resample
            adjustedAudioFormat.setSampleRate(FORTY_FOUR);
        }
#endif

        if (adjustedAudioFormat != desiredAudioFormat) {
            // return the nearest in case it needs 2 channels
            adjustedAudioFormat = audioDevice.nearestFormat(adjustedAudioFormat);
            return true;
        } else {
            return false;
        }
    } else {
        // set the adjustedAudioFormat to the desiredAudioFormat, since it will work
        adjustedAudioFormat = desiredAudioFormat;
        return true;
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

soxr_error_t possibleResampling(soxr_t resampler,
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

            return 0;
        } else {
            soxr_error_t resampleError = 0;

            if (sourceAudioFormat.channelCount() != destinationAudioFormat.channelCount()) {
                float channelCountRatio = (float) destinationAudioFormat.channelCount() / sourceAudioFormat.channelCount();

                int numChannelCoversionSamples = (int) (numSourceSamples * channelCountRatio);
                int16_t* channelConversionSamples = new int16_t[numChannelCoversionSamples];

                sampleChannelConversion(sourceSamples, channelConversionSamples,
                                        numSourceSamples,
                                        sourceAudioFormat, destinationAudioFormat);

                resampleError = soxr_process(resampler,
                                             channelConversionSamples, numChannelCoversionSamples, NULL,
                                             destinationSamples, numDestinationSamples, NULL);

                delete[] channelConversionSamples;
            } else {

                unsigned int numAdjustedSourceSamples = numSourceSamples;
                unsigned int numAdjustedDestinationSamples = numDestinationSamples;

                if (sourceAudioFormat.channelCount() == 2 && destinationAudioFormat.channelCount() == 2) {
                    numAdjustedSourceSamples /= 2;
                    numAdjustedDestinationSamples /= 2;
                }

                resampleError = soxr_process(resampler,
                                             sourceSamples, numAdjustedSourceSamples, NULL,
                                             destinationSamples, numAdjustedDestinationSamples, NULL);
            }

            return resampleError;
        }
    } else {
        return 0;
    }
}

soxr_t soxrResamplerFromInputFormatToOutputFormat(const QAudioFormat& sourceAudioFormat,
                                                  const QAudioFormat& destinationAudioFormat) {
    soxr_error_t soxrError;

    // setup soxr_io_spec_t for input and output
    soxr_io_spec_t inputToNetworkSpec = soxr_io_spec(soxrDataTypeFromQAudioFormat(sourceAudioFormat),
                                                     soxrDataTypeFromQAudioFormat(destinationAudioFormat));

    // setup soxr_quality_spec_t for quality options
    soxr_quality_spec_t qualitySpec = soxr_quality_spec(SOXR_MQ, 0);

    int channelCount = (sourceAudioFormat.channelCount() == 2 && destinationAudioFormat.channelCount() == 2)
        ? 2 : 1;

    soxr_t newResampler = soxr_create(sourceAudioFormat.sampleRate(),
                                      destinationAudioFormat.sampleRate(),
                                      channelCount,
                                      &soxrError, &inputToNetworkSpec, &qualitySpec, 0);

    if (soxrError) {
        qCDebug(audioclient) << "There was an error setting up the soxr resampler -" << "soxr error code was " << soxrError;

        soxr_delete(newResampler);

        return NULL;
    }

    return newResampler;
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

    if (_audioInput) {
        _inputFrameBuffer.initialize( _inputFormat.channelCount(), _audioInput->bufferSize() * 8 );
    }

    _inputGain.initialize();
    _sourceGain.initialize();
    _noiseSource.initialize();
    _toneSource.initialize();
    _sourceGain.setParameters(0.05f, 0.0f);
    _inputGain.setParameters(1.0f, 0.0f);
}

void AudioClient::stop() {
    _inputFrameBuffer.finalize();
    _inputGain.finalize();
    _sourceGain.finalize();
    _noiseSource.finalize();
    _toneSource.finalize();

    // "switch" to invalid devices in order to shut down the state
    switchInputToAudioDevice(QAudioDeviceInfo());
    switchOutputToAudioDevice(QAudioDeviceInfo());

    if (_loopbackResampler) {
        soxr_delete(_loopbackResampler);
        _loopbackResampler = NULL;
    }
}

QString AudioClient::getDefaultDeviceName(QAudio::Mode mode) {
    QAudioDeviceInfo deviceInfo = defaultAudioDeviceForMode(mode);
    return deviceInfo.deviceName();
}

QVector<QString> AudioClient::getDeviceNames(QAudio::Mode mode) {
    QVector<QString> deviceNames;
    foreach(QAudioDeviceInfo audioDevice, QAudioDeviceInfo::availableDevices(mode)) {
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

ty_gverb* AudioClient::createGverbFilter() {
    // Initialize a new gverb instance
    ty_gverb* filter = gverb_new(_outputFormat.sampleRate(), _reverbOptions->getMaxRoomSize(), _reverbOptions->getRoomSize(),
                                 _reverbOptions->getReverbTime(), _reverbOptions->getDamping(), _reverbOptions->getSpread(),
                                 _reverbOptions->getInputBandwidth(), _reverbOptions->getEarlyLevel(),
                                 _reverbOptions->getTailLevel());

    return filter;
}

void AudioClient::configureGverbFilter(ty_gverb* filter) {
    // Configure the instance (these functions are not super well named - they actually set several internal variables)
    gverb_set_roomsize(filter, _reverbOptions->getRoomSize());
    gverb_set_revtime(filter, _reverbOptions->getReverbTime());
    gverb_set_damping(filter, _reverbOptions->getDamping());
    gverb_set_inputbandwidth(filter, _reverbOptions->getInputBandwidth());
    gverb_set_earlylevel(filter, DB_CO(_reverbOptions->getEarlyLevel()));
    gverb_set_taillevel(filter, DB_CO(_reverbOptions->getTailLevel()));
}

void AudioClient::updateGverbOptions() {
    bool reverbChanged = false;
    if (_receivedAudioStream.hasReverb()) {

        if (_zoneReverbOptions.getReverbTime() != _receivedAudioStream.getRevebTime()) {
            _zoneReverbOptions.setReverbTime(_receivedAudioStream.getRevebTime());
            reverbChanged = true;
        }
        if (_zoneReverbOptions.getWetLevel() != _receivedAudioStream.getWetLevel()) {
            _zoneReverbOptions.setWetLevel(_receivedAudioStream.getWetLevel());
            // Not part of actual filter config, no need to set reverbChanged to true
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
        gverb_free(_gverb);
        _gverb = createGverbFilter();
        configureGverbFilter(_gverb);
    }
}

void AudioClient::setReverb(bool reverb) {
    _reverb = reverb;

    if (!_reverb) {
        gverb_flush(_gverb);
    }
}

void AudioClient::setReverbOptions(const AudioEffectOptions* options) {
    // Save the new options
    _scriptReverbOptions.setMaxRoomSize(options->getMaxRoomSize());
    _scriptReverbOptions.setRoomSize(options->getRoomSize());
    _scriptReverbOptions.setReverbTime(options->getReverbTime());
    _scriptReverbOptions.setDamping(options->getDamping());
    _scriptReverbOptions.setSpread(options->getSpread());
    _scriptReverbOptions.setInputBandwidth(options->getInputBandwidth());
    _scriptReverbOptions.setEarlyLevel(options->getEarlyLevel());
    _scriptReverbOptions.setTailLevel(options->getTailLevel());

    _scriptReverbOptions.setDryLevel(options->getDryLevel());
    _scriptReverbOptions.setWetLevel(options->getWetLevel());

    if (_reverbOptions == &_scriptReverbOptions) {
        // Apply them to the reverb instances
        gverb_free(_gverb);
        _gverb = createGverbFilter();
        configureGverbFilter(_gverb);
    }
}

void AudioClient::addReverb(ty_gverb* gverb, int16_t* samplesData, int16_t* reverbAlone, int numSamples,
                            QAudioFormat& audioFormat, bool noEcho) {
    float wetFraction = DB_CO(_reverbOptions->getWetLevel());
    float dryFraction = 1.0f - wetFraction;

    float lValue,rValue;
    for (int sample = 0; sample < numSamples; sample += audioFormat.channelCount()) {
        // Run GVerb
        float value = (float)samplesData[sample];
        gverb_do(gverb, value, &lValue, &rValue);

        // Mix, accounting for clipping, the left and right channels. Ignore the rest.
        for (int j = sample; j < sample + audioFormat.channelCount(); j++) {
            if (j == sample) {
                // left channel
                int lResult = glm::clamp((int)(samplesData[j] * dryFraction + lValue * wetFraction),
                                         AudioConstants::MIN_SAMPLE_VALUE, AudioConstants::MAX_SAMPLE_VALUE);
                samplesData[j] = (int16_t)lResult;

                if (noEcho) {
                    reverbAlone[j] = (int16_t)lValue * wetFraction;
                }
            } else if (j == (sample + 1)) {
                // right channel
                int rResult = glm::clamp((int)(samplesData[j] * dryFraction + rValue * wetFraction),
                                         AudioConstants::MIN_SAMPLE_VALUE, AudioConstants::MAX_SAMPLE_VALUE);
                samplesData[j] = (int16_t)rResult;

                if (noEcho) {
                    reverbAlone[j] = (int16_t)rValue * wetFraction;
                }
            } else {
                // ignore channels above 2
            }
        }
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
        _loopbackOutputDevice = _loopbackAudioOutput->start();

        if (!_loopbackOutputDevice) {
            return;
        }
    }

    // do we need to setup a resampler?
    if (_inputFormat.sampleRate() != _outputFormat.sampleRate() && !_loopbackResampler) {
        qCDebug(audioclient) << "Attemping to create a resampler for input format to output format for audio loopback.";
        _loopbackResampler = soxrResamplerFromInputFormatToOutputFormat(_inputFormat, _outputFormat);

        if (!_loopbackResampler) {
            return;
        }
    }

    static QByteArray reverbAlone;  // Intermediary for local reverb with no echo
    static QByteArray loopBackByteArray;

    int numInputSamples = inputByteArray.size() / sizeof(int16_t);
    int numLoopbackSamples = numDestinationSamplesRequired(_inputFormat, _outputFormat, numInputSamples);

    reverbAlone.resize(numInputSamples * sizeof(int16_t));
    loopBackByteArray.resize(numLoopbackSamples * sizeof(int16_t));

    int16_t* inputSamples = reinterpret_cast<int16_t*>(inputByteArray.data());
    int16_t* reverbAloneSamples = reinterpret_cast<int16_t*>(reverbAlone.data());
    int16_t* loopbackSamples = reinterpret_cast<int16_t*>(loopBackByteArray.data());

    if (hasReverb) {
        updateGverbOptions();
        addReverb(_gverb, inputSamples, reverbAloneSamples, numInputSamples,
                  _inputFormat, !_shouldEchoLocally);
    }

    possibleResampling(_loopbackResampler,
                       (_shouldEchoLocally) ? inputSamples : reverbAloneSamples, loopbackSamples,
                       numInputSamples, numLoopbackSamples,
                       _inputFormat, _outputFormat);

    _loopbackOutputDevice->write(loopBackByteArray);
}

void AudioClient::handleAudioInput() {
    if (!_audioPacket) {
        // we don't have an audioPacket yet - set that up now
        _audioPacket = NLPacket::create(PacketType::MicrophoneAudioNoEcho);
    }

    float inputToNetworkInputRatio = calculateDeviceToNetworkInputRatio();

    int inputSamplesRequired = (int)((float)AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL * inputToNetworkInputRatio);

    static int leadingBytes = sizeof(quint16) + sizeof(glm::vec3) + sizeof(glm::quat) + sizeof(quint8);
    int16_t* networkAudioSamples = (int16_t*)(_audioPacket->payload() + leadingBytes);

    QByteArray inputByteArray = _inputDevice->readAll();

    //  Add audio source injection if enabled
    if (!_muted && _audioSourceInjectEnabled) {
        int16_t* inputFrameData = (int16_t*)inputByteArray.data();
        const uint32_t inputFrameCount = inputByteArray.size() / sizeof(int16_t);

        _inputFrameBuffer.copyFrames(1, inputFrameCount, inputFrameData, false /*copy in*/);

#if ENABLE_INPUT_GAIN
        _inputGain.render(_inputFrameBuffer);  // input/mic gain+mute
#endif
        if (_toneSourceEnabled) {  // sine generator
            _toneSource.render(_inputFrameBuffer);
        } else if(_noiseSourceEnabled) { // pink noise generator
            _noiseSource.render(_inputFrameBuffer);
        }
        _sourceGain.render(_inputFrameBuffer); // post gain
        _inputFrameBuffer.copyFrames(1, inputFrameCount, inputFrameData, true /*copy out*/);
    }

    handleLocalEchoAndReverb(inputByteArray);

    _inputRingBuffer.writeData(inputByteArray.data(), inputByteArray.size());

    float audioInputMsecsRead = inputByteArray.size() / (float)(_inputFormat.bytesForDuration(USECS_PER_MSEC));
    _stats.updateInputMsecsRead(audioInputMsecsRead);

    while (_inputRingBuffer.samplesAvailable() >= inputSamplesRequired) {

        const int numNetworkBytes = _isStereoInput
            ? AudioConstants::NETWORK_FRAME_BYTES_STEREO`
            : AudioConstants::NETWORK_FRAME_BYTES_PER_CHANNEL;
        const int numNetworkSamples = _isStereoInput
            ? AudioConstants::NETWORK_FRAME_SAMPLES_STEREO
            : AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL;

        if (!_muted) {

            // zero out the monoAudioSamples array and the locally injected audio
            memset(networkAudioSamples, 0, numNetworkBytes);

            //  Increment the time since the last clip
            if (_timeSinceLastClip >= 0.0f) {
                _timeSinceLastClip += (float) numNetworkSamples / (float) AudioConstants::SAMPLE_RATE;
            }

            int16_t* inputAudioSamples = new int16_t[inputSamplesRequired];
            _inputRingBuffer.readSamples(inputAudioSamples, inputSamplesRequired);

            possibleResampling(_inputToNetworkResampler,
                               inputAudioSamples, networkAudioSamples,
                               inputSamplesRequired, numNetworkSamples,
                               _inputFormat, _desiredInputFormat);

            delete[] inputAudioSamples;

            //  Remove DC offset
            if (!_isStereoInput && !_audioSourceInjectEnabled) {
                _inputGate.removeDCOffset(networkAudioSamples, numNetworkSamples);
            }

            // only impose the noise gate and perform tone injection if we are sending mono audio
            if (!_isStereoInput && !_audioSourceInjectEnabled && _isNoiseGateEnabled) {
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
                    loudness += (float) thisSample;

                    if (thisSample > (AudioConstants::MAX_SAMPLE_VALUE * AudioNoiseGate::CLIPPING_THRESHOLD)) {
                        _timeSinceLastClip = 0.0f;
                    }
                }

                _lastInputLoudness = fabs(loudness / numNetworkSamples);
            }

            emit inputReceived(QByteArray(reinterpret_cast<const char*>(networkAudioSamples),
                                          AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL * sizeof(AudioConstants::AudioSample)));

        } else {
            // our input loudness is 0, since we're muted
            _lastInputLoudness = 0;
            _timeSinceLastClip = 0.0f;

            _inputRingBuffer.shiftReadPosition(inputSamplesRequired);
        }

        auto nodeList = DependencyManager::get<NodeList>();
        SharedNodePointer audioMixer = nodeList->soloNodeOfType(NodeType::AudioMixer);

        if (audioMixer && audioMixer->getActiveSocket()) {
            glm::vec3 headPosition = _positionGetter();
            glm::quat headOrientation = _orientationGetter();
            quint8 isStereo = _isStereoInput ? 1 : 0;

            PacketType::Value packetType;
            if (_lastInputLoudness == 0) {
                _audioPacket->setType(PacketType::SilentAudioFrame);
            } else {
                if (_shouldEchoToServer) {
                    _audioPacket->setType(PacketType::MicrophoneAudioWithEcho);
                } else {
                    _audioPacket->setType(PacketType::MicrophoneAudioNoEcho);
                }
            }

            // seek to the beginning of the audio packet payload
            _audioPacket->seek(0);

            // reset the size used in this packet so it will be correct once we are done writing
            _audioPacket->setSizeUsed(0);

            // write sequence number
            _audioPacket->write(&_outgoingAvatarAudioSequenceNumber, sizeof(quint16));

            if (packetType == PacketType::SilentAudioFrame) {
                // pack num silent samples
                _audioPacket->write(&numSilentSamples, sizeof(quint16));
            } else {
                // set the mono/stereo byte
                _audioPacket->write(&isStereo, sizeof(isStereo));
            }

            // pack the three float positions
            _audioPacket->write(&headPosition, sizeof(headPosition));

            // pack the orientation
            _audioPacket->write(&headOrientation, sizeof(headOrientation));

            if (packetType != PacketType::SilentAudioFrame) {
                // audio samples have already been packed (written to networkAudioSamples)
                _audioPacket->setSizeUsed(_audioPacket->getSizeUsed() + numNetworkBytes);
            }

            _stats.sentPacket();

            nodeList->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SendAudioPacket);

            nodeList->sendUnreliablePacket(_audioPacket, audioMixer);

            _outgoingAvatarAudioSequenceNumber++;
        }
    }
}

void AudioClient::processReceivedSamples(const QByteArray& inputBuffer, QByteArray& outputBuffer) {
    const int numNetworkOutputSamples = inputBuffer.size() / sizeof(int16_t);
    const int numDeviceOutputSamples = numNetworkOutputSamples * (_outputFormat.sampleRate() * _outputFormat.channelCount())
        / (_desiredOutputFormat.sampleRate() * _desiredOutputFormat.channelCount());

    outputBuffer.resize(numDeviceOutputSamples * sizeof(int16_t));

    const int16_t* receivedSamples = reinterpret_cast<const int16_t*>(inputBuffer.data());

    // copy the packet from the RB to the output
    possibleResampling(_networkToOutputResampler, receivedSamples,
                       reinterpret_cast<int16_t*>(outputBuffer.data()),
                       numNetworkOutputSamples, numDeviceOutputSamples,
                       _desiredOutputFormat, _outputFormat);
}

void AudioClient::sendMuteEnvironmentPacket() {
    auto nodeList = DependencyManager::get<NodeList>();

    int dataSize = sizeof(glm::vec3) + sizeof(float);

    NodeList::Packet mutePacket = nodeList->makePacket(PacketType::MuteEnvironment, dataSize);

    const float MUTE_RADIUS = 50;

    glm::vec3 currentSourcePosition = _positionGetter();

    memcpy(mutePacket.payload().data(), &currentSourcePosition, sizeof(glm::vec3));
    memcpy(mutePacket.payload() + sizeof(glm::vec3), &MUTE_RADIUS, sizeof(float));

    // grab our audio mixer from the NodeList, if it exists
    SharedNodePointer audioMixer = nodeList->soloNodeOfType(NodeType::AudioMixer);

    if (audioMixer) {
        // send off this mute packet
        nodeList->sendPacket(mutePacket, audioMixer);
    }
}

void AudioClient::addReceivedAudioToStream(const QByteArray& audioByteArray) {
    DependencyManager::get<NodeList>()->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::ReceiveFirstAudioPacket);

    if (_audioOutput) {

        if (!_hasReceivedFirstPacket) {
            _hasReceivedFirstPacket = true;

            // have the audio scripting interface emit a signal to say we just connected to mixer
            emit receivedFirstPacket();
        }

        // Audio output must exist and be correctly set up if we're going to process received audio
        _receivedAudioStream.parseData(audioByteArray);
    }
}

void AudioClient::parseAudioEnvironmentData(const QByteArray &packet) {
    int numBytesPacketHeader = numBytesForPacketHeader(packet);
    const char* dataAt = packet.constData() + numBytesPacketHeader;

    char bitset;
    memcpy(&bitset, dataAt, sizeof(char));
    dataAt += sizeof(char);

    bool hasReverb = oneAtBit(bitset, HAS_REVERB_BIT);;
    if (hasReverb) {
        float reverbTime, wetLevel;
        memcpy(&reverbTime, dataAt, sizeof(float));
        dataAt += sizeof(float);
        memcpy(&wetLevel, dataAt, sizeof(float));
        dataAt += sizeof(float);
        _receivedAudioStream.setReverb(reverbTime, wetLevel);
    } else {
        _receivedAudioStream.clearReverb();
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

void AudioClient::enableAudioSourceInject(bool enable) {
    _audioSourceInjectEnabled = enable;
}

void AudioClient::selectAudioSourcePinkNoise() {
    _noiseSourceEnabled = true;
    _toneSourceEnabled = false;
}

void AudioClient::selectAudioSourceSine440() {
    _toneSourceEnabled = true;
    _noiseSourceEnabled = false;
}

bool AudioClient::outputLocalInjector(bool isStereo, AudioInjector* injector) {
    if (injector->getLocalBuffer()) {
        QAudioFormat localFormat = _desiredOutputFormat;
        localFormat.setChannelCount(isStereo ? 2 : 1);

        QAudioOutput* localOutput = new QAudioOutput(getNamedAudioDeviceForMode(QAudio::AudioOutput, _outputAudioDeviceName),
                                                     localFormat,
                                                     injector->getLocalBuffer());

        // move the localOutput to the same thread as the local injector buffer
        localOutput->moveToThread(injector->getLocalBuffer()->thread());

        // have it be stopped when that local buffer is about to close
        connect(localOutput, &QAudioOutput::stateChanged, this, &AudioClient::audioStateChanged);
        connect(this, &AudioClient::audioFinished, localOutput, &QAudioOutput::stop);
        connect(this, &AudioClient::audioFinished, injector, &AudioInjector::stop);

        connect(injector->getLocalBuffer(), &QIODevice::aboutToClose, localOutput, &QAudioOutput::stop);

        qCDebug(audioclient) << "Starting QAudioOutput for local injector" << localOutput;

        localOutput->start(injector->getLocalBuffer());
        return localOutput->state() == QAudio::ActiveState;
    }

    return false;
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
        soxr_delete(_inputToNetworkResampler);
        _inputToNetworkResampler = NULL;
    }

    if (!inputDeviceInfo.isNull()) {
        qCDebug(audioclient) << "The audio input device " << inputDeviceInfo.deviceName() << "is available.";
        _inputAudioDeviceName = inputDeviceInfo.deviceName().trimmed();

        if (adjustedFormatForAudioDevice(inputDeviceInfo, _desiredInputFormat, _inputFormat)) {
            qCDebug(audioclient) << "The format to be used for audio input is" << _inputFormat;

            // we've got the best we can get for input
            // if required, setup a soxr resampler for this input to our desired network format
            if (_inputFormat != _desiredInputFormat
                && _inputFormat.sampleRate() != _desiredInputFormat.sampleRate()) {
                qCDebug(audioclient) << "Attemping to create a soxr resampler for input format to network format.";
                _inputToNetworkResampler = soxrResamplerFromInputFormatToOutputFormat(_inputFormat, _desiredInputFormat);

                if (!_inputToNetworkResampler) {
                    return false;
                }
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

                _inputDevice = _audioInput->start();

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
        if (_outputStarveDetectionEnabled.get()) {
            quint64 now = usecTimestampNow() / 1000;
            int dt = (int)(now - _outputStarveDetectionStartTimeMsec);
            if (dt > _outputStarveDetectionPeriodMsec.get()) {
                _outputStarveDetectionStartTimeMsec = now;
                _outputStarveDetectionCount = 0;
            } else {
                _outputStarveDetectionCount += recentUnfulfilled;
                if (_outputStarveDetectionCount > _outputStarveDetectionThreshold.get()) {
                    _outputStarveDetectionStartTimeMsec = now;
                    _outputStarveDetectionCount = 0;

                    int oldOutputBufferSizeFrames = _outputBufferSizeFrames.get();
                    int newOutputBufferSizeFrames = oldOutputBufferSizeFrames + 1;
                    setOutputBufferSize(newOutputBufferSizeFrames);
                    newOutputBufferSizeFrames = _outputBufferSizeFrames.get();
                    if (newOutputBufferSizeFrames > oldOutputBufferSizeFrames) {
                        qCDebug(audioclient) << "Starve detection threshold met, increasing buffer size to " << newOutputBufferSizeFrames;
                    }
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
        soxr_delete(_networkToOutputResampler);
        _networkToOutputResampler = NULL;
    }

    if (_loopbackResampler) {
        // if we were using an input to output resample, delete it here
        soxr_delete(_loopbackResampler);
        _loopbackResampler = NULL;
    }

    if (!outputDeviceInfo.isNull()) {
        qCDebug(audioclient) << "The audio output device " << outputDeviceInfo.deviceName() << "is available.";
        _outputAudioDeviceName = outputDeviceInfo.deviceName().trimmed();

        if (adjustedFormatForAudioDevice(outputDeviceInfo, _desiredOutputFormat, _outputFormat)) {
            qCDebug(audioclient) << "The format to be used for audio output is" << _outputFormat;

            // we've got the best we can get for input
            // if required, setup a soxr resampler for this input to our desired network format
            if (_desiredOutputFormat != _outputFormat
                && _desiredOutputFormat.sampleRate() != _outputFormat.sampleRate()) {
                qCDebug(audioclient) << "Attemping to create a resampler for network format to output format.";
                _networkToOutputResampler = soxrResamplerFromInputFormatToOutputFormat(_desiredOutputFormat, _outputFormat);

                if (!_networkToOutputResampler) {
                    return false;
                }
            } else {
                qCDebug(audioclient) << "No resampling required for network output to match actual output format.";
            }

            outputFormatChanged();

            // setup our general output device for audio-mixer audio
            _audioOutput = new QAudioOutput(outputDeviceInfo, _outputFormat, this);
            _audioOutput->setBufferSize(_outputBufferSizeFrames.get() * _outputFrameSize * sizeof(int16_t));

            connect(_audioOutput, &QAudioOutput::notify, this, &AudioClient::outputNotify);

            qCDebug(audioclient) << "Output Buffer capacity in frames: " << _audioOutput->bufferSize() / sizeof(int16_t) / (float)_outputFrameSize;

            _audioOutputIODevice.start();
            _audioOutput->start(&_audioOutputIODevice);

            // setup a loopback audio output device
            _loopbackAudioOutput = new QAudioOutput(outputDeviceInfo, _outputFormat, this);


            _timeSinceLastReceived.start();

            supportedFormat = true;
        }
    }

    return supportedFormat;
}

void AudioClient::setOutputBufferSize(int numFrames) {
    numFrames = std::min(std::max(numFrames, MIN_AUDIO_OUTPUT_BUFFER_SIZE_FRAMES), MAX_AUDIO_OUTPUT_BUFFER_SIZE_FRAMES);
    if (numFrames != _outputBufferSizeFrames.get()) {
        qCDebug(audioclient) << "Audio output buffer size (frames): " << numFrames;
        _outputBufferSizeFrames.set(numFrames);

        if (_audioOutput) {
            // The buffer size can't be adjusted after QAudioOutput::start() has been called, so
            // recreate the device by switching to the default.
            QAudioDeviceInfo outputDeviceInfo = defaultAudioDeviceForMode(QAudio::AudioOutput);
            switchOutputToAudioDevice(outputDeviceInfo);
        }
    }
}

// The following constant is operating system dependent due to differences in
// the way input audio is handled. The audio input buffer size is inversely
// proportional to the accelerator ratio.

#ifdef Q_OS_WIN
const float AudioClient::CALLBACK_ACCELERATOR_RATIO = 0.1f;
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

float AudioClient::calculateDeviceToNetworkInputRatio() const {
    float inputToNetworkInputRatio = (int)((_numInputCallbackBytes
        * CALLBACK_ACCELERATOR_RATIO
        / AudioConstants::NETWORK_FRAME_BYTES_PER_CHANNEL) + 0.5f);

    return inputToNetworkInputRatio;
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

qint64 AudioClient::AudioOutputIODevice::readData(char * data, qint64 maxSize) {
    int samplesRequested = maxSize / sizeof(int16_t);
    int samplesPopped;
    int bytesWritten;

    if ((samplesPopped = _receivedAudioStream.popSamples(samplesRequested, false)) > 0) {
        AudioRingBuffer::ConstIterator lastPopOutput = _receivedAudioStream.getLastPopOutput();
        lastPopOutput.readSamples((int16_t*)data, samplesPopped);
        bytesWritten = samplesPopped * sizeof(int16_t);
    } else {
        memset(data, 0, maxSize);
        bytesWritten = maxSize;
    }

    int bytesAudioOutputUnplayed = _audio->_audioOutput->bufferSize() - _audio->_audioOutput->bytesFree();
    if (bytesAudioOutputUnplayed == 0 && bytesWritten == 0) {
        _unfulfilledReads++;
    }

    return bytesWritten;
}

void AudioClient::checkDevices() {
#   ifdef Q_OS_LINUX
    // on linux, this makes the audio stream hiccup
#   else
    QVector<QString> inputDevices = getDeviceNames(QAudio::AudioInput);
    QVector<QString> outputDevices = getDeviceNames(QAudio::AudioOutput);

    if (inputDevices != _inputDevices || outputDevices != _outputDevices) {
        _inputDevices = inputDevices;
        _outputDevices = outputDevices;

        emit deviceChanged();
    }
#   endif
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

void AudioClient::audioStateChanged(QAudio::State state) {
    if (state == QAudio::IdleState) {
        emit audioFinished();
    }
}
