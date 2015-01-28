//
//  Audio.cpp
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

#include <AudioConstants.h>
#include <AudioInjector.h>
#include <NodeList.h>
#include <PacketHeaders.h>
#include <PositionalAudioStream.h>
#include <Settings.h>
#include <SharedUtil.h>
#include <UUID.h>

#include "Application.h"
#include "Audio.h"

static const int RECEIVED_AUDIO_STREAM_CAPACITY_FRAMES = 100;

namespace SettingHandles {
    const SettingHandle<bool> audioOutputStarveDetectionEnabled("audioOutputStarveDetectionEnabled",
                                                                DEFAULT_AUDIO_OUTPUT_STARVE_DETECTION_ENABLED);
    const SettingHandle<int> audioOutputStarveDetectionThreshold("audioOutputStarveDetectionThreshold",
                                                                 DEFAULT_AUDIO_OUTPUT_STARVE_DETECTION_THRESHOLD);
    const SettingHandle<int> audioOutputStarveDetectionPeriod("audioOutputStarveDetectionPeriod",
                                                              DEFAULT_AUDIO_OUTPUT_STARVE_DETECTION_PERIOD);
    const SettingHandle<int> audioOutputBufferSize("audioOutputBufferSize",
                                                   DEFAULT_MAX_FRAMES_OVER_DESIRED);
}

Audio::Audio() :
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
    _outputBufferSizeFrames(DEFAULT_AUDIO_OUTPUT_BUFFER_SIZE_FRAMES),
    _outputStarveDetectionEnabled(true),
    _outputStarveDetectionStartTimeMsec(0),
    _outputStarveDetectionCount(0),
    _outputStarveDetectionPeriodMsec(DEFAULT_AUDIO_OUTPUT_STARVE_DETECTION_PERIOD),
    _outputStarveDetectionThreshold(DEFAULT_AUDIO_OUTPUT_STARVE_DETECTION_THRESHOLD),
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
    _gverbLocal(NULL),
    _gverb(NULL),
    _noiseSourceEnabled(false),
    _toneSourceEnabled(true),
    _outgoingAvatarAudioSequenceNumber(0),
    _audioOutputIODevice(_receivedAudioStream, this),
    _stats(&_receivedAudioStream),
    _inputGate()
{
    // clear the array of locally injected samples
    memset(_localProceduralSamples, 0, AudioConstants::NETWORK_FRAME_BYTES_PER_CHANNEL);
    
    connect(&_receivedAudioStream, &MixedProcessedAudioStream::processSamples, this, &Audio::processReceivedSamples, Qt::DirectConnection);
    
    // Initialize GVerb
    initGverb();

    const qint64 DEVICE_CHECK_INTERVAL_MSECS = 2 * 1000;

    _inputDevices = getDeviceNames(QAudio::AudioInput);
    _outputDevices = getDeviceNames(QAudio::AudioOutput);

    QTimer* updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &Audio::checkDevices);
    updateTimer->start(DEVICE_CHECK_INTERVAL_MSECS);
}

void Audio::reset() {
    _receivedAudioStream.reset();
    _stats.reset();
    _noiseSource.reset();
    _toneSource.reset();
    _sourceGain.reset();
    _inputGain.reset();
}

void Audio::audioMixerKilled() {
    _outgoingAvatarAudioSequenceNumber = 0;
    _stats.reset();
}


QAudioDeviceInfo getNamedAudioDeviceForMode(QAudio::Mode mode, const QString& deviceName) {
    QAudioDeviceInfo result;
// Temporarily enable audio device selection in Windows again to test how it behaves now
//#ifdef WIN32
#if FALSE
    // NOTE
    // this is a workaround for a windows only QtBug https://bugreports.qt-project.org/browse/QTBUG-16117
    // static QAudioDeviceInfo objects get deallocated when QList<QAudioDevieInfo> objects go out of scope
    result = (mode == QAudio::AudioInput) ? 
        QAudioDeviceInfo::defaultInputDevice() : 
        QAudioDeviceInfo::defaultOutputDevice();
#else
    foreach(QAudioDeviceInfo audioDevice, QAudioDeviceInfo::availableDevices(mode)) {
        if (audioDevice.deviceName().trimmed() == deviceName.trimmed()) {
            result = audioDevice;
            break;
        }
    }
#endif
    return result;
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
            qDebug() << "input device:" << wic.szPname;
            deviceName = wic.szPname;
        } else {
            WAVEOUTCAPS woc;
            // first use WAVE_MAPPER to get the default devices manufacturer ID
            waveOutGetDevCaps(WAVE_MAPPER, &woc, sizeof(woc));
            //Use the received manufacturer id to get the device's real name
            waveOutGetDevCaps(woc.wMid, &woc, sizeof(woc));
            qDebug() << "output device:" << woc.szPname;
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
            qDebug() << (mode == QAudio::AudioOutput ? "output" : "input") << " device:" << deviceName;
            PropVariantClear(&pv);
        }
        pMMDeviceEnumerator->Release();
        pMMDeviceEnumerator = NULL;
        CoUninitialize();
    }

    qDebug() << "DEBUG [" << deviceName << "] [" << getNamedAudioDeviceForMode(mode, deviceName).deviceName() << "]";
    
    return getNamedAudioDeviceForMode(mode, deviceName);
#endif


    // fallback for failed lookup is the default device
    return (mode == QAudio::AudioInput) ? QAudioDeviceInfo::defaultInputDevice() : QAudioDeviceInfo::defaultOutputDevice();
}

bool adjustedFormatForAudioDevice(const QAudioDeviceInfo& audioDevice,
                                  const QAudioFormat& desiredAudioFormat,
                                  QAudioFormat& adjustedAudioFormat) {
    if (!audioDevice.isFormatSupported(desiredAudioFormat)) {
        qDebug() << "The desired format for audio I/O is" << desiredAudioFormat;
        qDebug("The desired audio format is not supported by this device");
        
        if (desiredAudioFormat.channelCount() == 1) {
            adjustedAudioFormat = desiredAudioFormat;
            adjustedAudioFormat.setChannelCount(2);

            if (audioDevice.isFormatSupported(adjustedAudioFormat)) {
                return true;
            } else {
                adjustedAudioFormat.setChannelCount(1);
            }
        }

        if (audioDevice.supportedSampleRates().contains(AudioConstants::SAMPLE_RATE * 2)) {
            // use 48, which is a sample downsample, upsample
            adjustedAudioFormat = desiredAudioFormat;
            adjustedAudioFormat.setSampleRate(AudioConstants::SAMPLE_RATE * 2);

            // return the nearest in case it needs 2 channels
            adjustedAudioFormat = audioDevice.nearestFormat(adjustedAudioFormat);
            return true;
        }

        return false;
    } else {
        // set the adjustedAudioFormat to the desiredAudioFormat, since it will work
        adjustedAudioFormat = desiredAudioFormat;
        return true;
    }
}

void linearResampling(const int16_t* sourceSamples, int16_t* destinationSamples,
                      unsigned int numSourceSamples, unsigned int numDestinationSamples,
                      const QAudioFormat& sourceAudioFormat, const QAudioFormat& destinationAudioFormat) {
    if (sourceAudioFormat == destinationAudioFormat) {
        memcpy(destinationSamples, sourceSamples, numSourceSamples * sizeof(int16_t));
    } else {
        float sourceToDestinationFactor = (sourceAudioFormat.sampleRate() / (float) destinationAudioFormat.sampleRate())
            * (sourceAudioFormat.channelCount() / (float) destinationAudioFormat.channelCount());

        // take into account the number of channels in source and destination
        // accomodate for the case where have an output with > 2 channels
        // this is the case with our HDMI capture

        if (sourceToDestinationFactor >= 2) {
            // we need to downsample from 48 to 24
            // for now this only supports a mono output - this would be the case for audio input
            if (destinationAudioFormat.channelCount() == 1) {
                for (unsigned int i = sourceAudioFormat.channelCount(); i < numSourceSamples; i += 2 * sourceAudioFormat.channelCount()) {
                    if (i + (sourceAudioFormat.channelCount()) >= numSourceSamples) {
                        destinationSamples[(i - sourceAudioFormat.channelCount()) / (int) sourceToDestinationFactor] =
                        (sourceSamples[i - sourceAudioFormat.channelCount()] / 2)
                        + (sourceSamples[i] / 2);
                    } else {
                        destinationSamples[(i - sourceAudioFormat.channelCount()) / (int) sourceToDestinationFactor] =
                        (sourceSamples[i - sourceAudioFormat.channelCount()] / 4)
                        + (sourceSamples[i] / 2)
                        + (sourceSamples[i + sourceAudioFormat.channelCount()] / 4);
                    }
                }
            } else {
                // this is a 48 to 24 resampling but both source and destination are two channels
                // squish two samples into one in each channel
                for (unsigned int i = 0; i < numSourceSamples; i += 4) {
                    destinationSamples[i / 2] = (sourceSamples[i] / 2) + (sourceSamples[i + 2] / 2);
                    destinationSamples[(i / 2) + 1] = (sourceSamples[i + 1] / 2) + (sourceSamples[i + 3] / 2);
                }
            }
        } else {
            if (sourceAudioFormat.sampleRate() == destinationAudioFormat.sampleRate()) {
                // mono to stereo, same sample rate
                if (!(sourceAudioFormat.channelCount() == 1 && destinationAudioFormat.channelCount() == 2)) {
                    qWarning() << "Unsupported format conversion" << sourceAudioFormat << destinationAudioFormat;
                    return;
                }
                for (const int16_t* sourceEnd = sourceSamples + numSourceSamples; sourceSamples != sourceEnd;
                        sourceSamples++) {
                    *destinationSamples++ = *sourceSamples;
                    *destinationSamples++ = *sourceSamples;
                }
                return;
            }
        
            // upsample from 24 to 48
            // for now this only supports a stereo to stereo conversion - this is our case for network audio to output
            int sourceIndex = 0;
            int dtsSampleRateFactor = (destinationAudioFormat.sampleRate() / sourceAudioFormat.sampleRate());
            int sampleShift = destinationAudioFormat.channelCount() * dtsSampleRateFactor;
            int destinationToSourceFactor = (1 / sourceToDestinationFactor);

            for (unsigned int i = 0; i < numDestinationSamples; i += sampleShift) {
                sourceIndex = (i / destinationToSourceFactor);

                // fill the L/R channels and make the rest silent
                for (unsigned int j = i; j < i + sampleShift; j++) {
                    if (j % destinationAudioFormat.channelCount() == 0) {
                        // left channel
                        destinationSamples[j] = sourceSamples[sourceIndex];
                    } else if (j % destinationAudioFormat.channelCount() == 1) {
                         // right channel
                        destinationSamples[j] = sourceSamples[sourceIndex + (sourceAudioFormat.channelCount() > 1 ? 1 : 0)];
                    } else {
                        // channels above 2, fill with silence
                        destinationSamples[j] = 0;
                    }
                }
            }
        }
    }
}

void Audio::start() {

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
    qDebug() << "The default audio input device is" << inputDeviceInfo.deviceName();
    bool inputFormatSupported = switchInputToAudioDevice(inputDeviceInfo);

    QAudioDeviceInfo outputDeviceInfo = defaultAudioDeviceForMode(QAudio::AudioOutput);
    qDebug() << "The default audio output device is" << outputDeviceInfo.deviceName();
    bool outputFormatSupported = switchOutputToAudioDevice(outputDeviceInfo);
    
    if (!inputFormatSupported) {
        qDebug() << "Unable to set up audio input because of a problem with input format.";
    }
    if (!outputFormatSupported) {
        qDebug() << "Unable to set up audio output because of a problem with output format.";
    }

    if (_audioInput) {
        _inputFrameBuffer.initialize( _inputFormat.channelCount(), _audioInput->bufferSize() * 8 );
    }
    _inputGain.initialize();
    _sourceGain.initialize();
    _noiseSource.initialize();
    _toneSource.initialize();
    _sourceGain.setParameters(0.25f,0.0f); 
    _inputGain.setParameters(1.0f,0.0f);
}

void Audio::stop() {

    _inputFrameBuffer.finalize();
    _inputGain.finalize();
    _sourceGain.finalize();
    _noiseSource.finalize();
    _toneSource.finalize();
    
    // "switch" to invalid devices in order to shut down the state
    switchInputToAudioDevice(QAudioDeviceInfo());
    switchOutputToAudioDevice(QAudioDeviceInfo());
}

QString Audio::getDefaultDeviceName(QAudio::Mode mode) {
    QAudioDeviceInfo deviceInfo = defaultAudioDeviceForMode(mode);
    return deviceInfo.deviceName();
}

QVector<QString> Audio::getDeviceNames(QAudio::Mode mode) {
    QVector<QString> deviceNames;
    foreach(QAudioDeviceInfo audioDevice, QAudioDeviceInfo::availableDevices(mode)) {
        deviceNames << audioDevice.deviceName().trimmed();
    }
    return deviceNames;
}

bool Audio::switchInputToAudioDevice(const QString& inputDeviceName) {
    qDebug() << "DEBUG [" << inputDeviceName << "] [" << getNamedAudioDeviceForMode(QAudio::AudioInput, inputDeviceName).deviceName() << "]";
    return switchInputToAudioDevice(getNamedAudioDeviceForMode(QAudio::AudioInput, inputDeviceName));
}

bool Audio::switchOutputToAudioDevice(const QString& outputDeviceName) {
    qDebug() << "DEBUG [" << outputDeviceName << "] [" << getNamedAudioDeviceForMode(QAudio::AudioOutput, outputDeviceName).deviceName() << "]";
    return switchOutputToAudioDevice(getNamedAudioDeviceForMode(QAudio::AudioOutput, outputDeviceName));
}

void Audio::initGverb() {
    // Initialize a new gverb instance
    _gverbLocal = gverb_new(_outputFormat.sampleRate(), _reverbOptions->getMaxRoomSize(), _reverbOptions->getRoomSize(),
                            _reverbOptions->getReverbTime(), _reverbOptions->getDamping(), _reverbOptions->getSpread(),
                            _reverbOptions->getInputBandwidth(), _reverbOptions->getEarlyLevel(),
                            _reverbOptions->getTailLevel());
    _gverb = gverb_new(_outputFormat.sampleRate(), _reverbOptions->getMaxRoomSize(), _reverbOptions->getRoomSize(),
                       _reverbOptions->getReverbTime(), _reverbOptions->getDamping(), _reverbOptions->getSpread(),
                       _reverbOptions->getInputBandwidth(), _reverbOptions->getEarlyLevel(),
                       _reverbOptions->getTailLevel());
    
    // Configure the instance (these functions are not super well named - they actually set several internal variables)
    gverb_set_roomsize(_gverbLocal, _reverbOptions->getRoomSize());
    gverb_set_revtime(_gverbLocal, _reverbOptions->getReverbTime());
    gverb_set_damping(_gverbLocal, _reverbOptions->getDamping());
    gverb_set_inputbandwidth(_gverbLocal, _reverbOptions->getInputBandwidth());
    gverb_set_earlylevel(_gverbLocal, DB_CO(_reverbOptions->getEarlyLevel()));
    gverb_set_taillevel(_gverbLocal, DB_CO(_reverbOptions->getTailLevel()));
    
    gverb_set_roomsize(_gverb, _reverbOptions->getRoomSize());
    gverb_set_revtime(_gverb, _reverbOptions->getReverbTime());
    gverb_set_damping(_gverb, _reverbOptions->getDamping());
    gverb_set_inputbandwidth(_gverb, _reverbOptions->getInputBandwidth());
    gverb_set_earlylevel(_gverb, DB_CO(_reverbOptions->getEarlyLevel()));
    gverb_set_taillevel(_gverb, DB_CO(_reverbOptions->getTailLevel()));
}

void Audio::updateGverbOptions() {
    bool reverbChanged = false;
    if (_receivedAudioStream.hasReverb()) {
        
        if (_zoneReverbOptions.getReverbTime() != _receivedAudioStream.getRevebTime()) {
            _zoneReverbOptions.setReverbTime(_receivedAudioStream.getRevebTime());
            reverbChanged = true;
        }
        if (_zoneReverbOptions.getWetLevel() != _receivedAudioStream.getWetLevel()) {
            _zoneReverbOptions.setWetLevel(_receivedAudioStream.getWetLevel());
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
        initGverb();
    }
}

void Audio::setReverbOptions(const AudioEffectOptions* options) {
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
        // Apply them to the reverb instance(s)
        initGverb();
    }
}

void Audio::addReverb(ty_gverb* gverb, int16_t* samplesData, int numSamples, QAudioFormat& audioFormat, bool noEcho) {
    float wetFraction = DB_CO(_reverbOptions->getWetLevel());
    float dryFraction = (noEcho) ? 0.0f : (1.0f - wetFraction);
    
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
            } else if (j == (sample + 1)) {
                // right channel
                int rResult = glm::clamp((int)(samplesData[j] * dryFraction + rValue * wetFraction),
                                         AudioConstants::MIN_SAMPLE_VALUE, AudioConstants::MAX_SAMPLE_VALUE);
                samplesData[j] = (int16_t)rResult;
            } else {
                // ignore channels above 2
            }
        }
    }
}

void Audio::handleLocalEchoAndReverb(QByteArray& inputByteArray) {
    // If there is server echo, reverb will be applied to the recieved audio stream so no need to have it here.
    bool hasLocalReverb = (_reverb || _receivedAudioStream.hasReverb()) &&
                          !_shouldEchoToServer;
    if (_muted || !_audioOutput || (!_shouldEchoLocally && !hasLocalReverb)) {
        return;
    }
    
    // if this person wants local loopback add that to the locally injected audio
    // if there is reverb apply it to local audio and substract the origin samples
    
    if (!_loopbackOutputDevice && _loopbackAudioOutput) {
        // we didn't have the loopback output device going so set that up now
        _loopbackOutputDevice = _loopbackAudioOutput->start();
    }
    
    QByteArray loopBackByteArray(inputByteArray);
    if (_inputFormat != _outputFormat) {
        float loopbackOutputToInputRatio = (_outputFormat.sampleRate() / (float) _inputFormat.sampleRate()) *
                                           (_outputFormat.channelCount() / _inputFormat.channelCount());
        loopBackByteArray.resize(inputByteArray.size() * loopbackOutputToInputRatio);
        loopBackByteArray.fill(0);
        linearResampling(reinterpret_cast<int16_t*>(inputByteArray.data()),
                         reinterpret_cast<int16_t*>(loopBackByteArray.data()),
                         inputByteArray.size() / sizeof(int16_t), loopBackByteArray.size() / sizeof(int16_t),
                         _inputFormat, _outputFormat);
    }
    
    if (hasLocalReverb) {
        int16_t* loopbackSamples = reinterpret_cast<int16_t*>(loopBackByteArray.data());
        int numLoopbackSamples = loopBackByteArray.size() / sizeof(int16_t);
        updateGverbOptions();
        addReverb(_gverbLocal, loopbackSamples, numLoopbackSamples, _outputFormat, !_shouldEchoLocally);
    }
    
    if (_loopbackOutputDevice) {
        _loopbackOutputDevice->write(loopBackByteArray);
    }
}

void Audio::handleAudioInput() {
    static char audioDataPacket[MAX_PACKET_SIZE];

    static int numBytesPacketHeader = numBytesForPacketHeaderGivenPacketType(PacketTypeMicrophoneAudioNoEcho);

    // NOTE: we assume PacketTypeMicrophoneAudioWithEcho has same size headers as
    // PacketTypeMicrophoneAudioNoEcho.  If not, then networkAudioSamples will be pointing to the wrong place for writing
    // audio samples with echo.
    static int leadingBytes = numBytesPacketHeader + sizeof(quint16) + sizeof(glm::vec3) + sizeof(glm::quat) + sizeof(quint8);
    static int16_t* networkAudioSamples = (int16_t*)(audioDataPacket + leadingBytes);

    float inputToNetworkInputRatio = calculateDeviceToNetworkInputRatio(_numInputCallbackBytes);

    int inputSamplesRequired = (int)((float)AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL * inputToNetworkInputRatio);

    QByteArray inputByteArray = _inputDevice->readAll();

    if (!_muted && _audioSourceInjectEnabled) {
        
        int16_t* inputFrameData = (int16_t*)inputByteArray.data();
        const uint32_t inputFrameCount = inputByteArray.size() / sizeof(int16_t);

        _inputFrameBuffer.copyFrames(1, inputFrameCount, inputFrameData, false /*copy in*/);

#if ENABLE_INPUT_GAIN
        _inputGain.render(_inputFrameBuffer);  // input/mic gain+mute
#endif
        //  Add audio source injection if enabled
        if (_audioSourceInjectEnabled) {
         
            if (_toneSourceEnabled) {  // sine generator
                _toneSource.render(_inputFrameBuffer);
            }
            else if(_noiseSourceEnabled) { // pink noise generator
                _noiseSource.render(_inputFrameBuffer);
            }
            _sourceGain.render(_inputFrameBuffer); // post gain
        }
        _inputFrameBuffer.copyFrames(1, inputFrameCount, inputFrameData, true /*copy out*/);
    }
    
    handleLocalEchoAndReverb(inputByteArray);

    _inputRingBuffer.writeData(inputByteArray.data(), inputByteArray.size());
    
    float audioInputMsecsRead = inputByteArray.size() / (float)(_inputFormat.bytesForDuration(USECS_PER_MSEC));
    _stats.updateInputMsecsRead(audioInputMsecsRead);

    while (_inputRingBuffer.samplesAvailable() >= inputSamplesRequired) {

        int16_t* inputAudioSamples = new int16_t[inputSamplesRequired];
        _inputRingBuffer.readSamples(inputAudioSamples, inputSamplesRequired);

        const int numNetworkBytes = _isStereoInput
            ? AudioConstants::NETWORK_FRAME_BYTES_STEREO
            : AudioConstants::NETWORK_FRAME_BYTES_PER_CHANNEL;
        const int numNetworkSamples = _isStereoInput
            ? AudioConstants::NETWORK_FRAME_SAMPLES_STEREO
            : AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL;

        // zero out the monoAudioSamples array and the locally injected audio
        memset(networkAudioSamples, 0, numNetworkBytes);

        if (!_muted) {
            
            //  Increment the time since the last clip
            if (_timeSinceLastClip >= 0.0f) {
                _timeSinceLastClip += (float) numNetworkSamples / (float) AudioConstants::SAMPLE_RATE;
            }
            
            // we aren't muted, downsample the input audio
            linearResampling((int16_t*) inputAudioSamples, networkAudioSamples,
                             inputSamplesRequired,  numNetworkSamples,
                             _inputFormat, _desiredInputFormat);
            
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
                    float thisSample = fabsf(networkAudioSamples[i]);
                    loudness += thisSample;
                    
                    if (thisSample > (AudioConstants::MAX_SAMPLE_VALUE * AudioNoiseGate::CLIPPING_THRESHOLD)) {
                        _timeSinceLastClip = 0.0f;
                    }
                }
                
                _lastInputLoudness = fabs(loudness / numNetworkSamples);
            }

        } else {
            // our input loudness is 0, since we're muted
            _lastInputLoudness = 0;
            _timeSinceLastClip = 0.0f;
        }
        
        emit inputReceived(QByteArray(reinterpret_cast<const char*>(networkAudioSamples),
                                      AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL));

        auto nodeList = DependencyManager::get<NodeList>();
        SharedNodePointer audioMixer = nodeList->soloNodeOfType(NodeType::AudioMixer);
        
        if (_recorder && _recorder.data()->isRecording()) {
            _recorder.data()->record(reinterpret_cast<char*>(networkAudioSamples), numNetworkBytes);
        }
        
        if (audioMixer && audioMixer->getActiveSocket()) {
            const MyAvatar* interfaceAvatar = Application::getInstance()->getAvatar();
            glm::vec3 headPosition = interfaceAvatar->getHead()->getPosition();
            glm::quat headOrientation = interfaceAvatar->getHead()->getFinalOrientationInWorldFrame();
            quint8 isStereo = _isStereoInput ? 1 : 0;

            PacketType packetType;
            if (_lastInputLoudness == 0) {
                packetType = PacketTypeSilentAudioFrame;
            } else {
                if (_shouldEchoToServer) {
                    packetType = PacketTypeMicrophoneAudioWithEcho;
                } else {
                    packetType = PacketTypeMicrophoneAudioNoEcho;
                }
            }

            char* currentPacketPtr = audioDataPacket + populatePacketHeader(audioDataPacket, packetType);

            // pack sequence number
            memcpy(currentPacketPtr, &_outgoingAvatarAudioSequenceNumber, sizeof(quint16));
            currentPacketPtr += sizeof(quint16);

            if (packetType == PacketTypeSilentAudioFrame) {
                // pack num silent samples
                quint16 numSilentSamples = numNetworkSamples;
                memcpy(currentPacketPtr, &numSilentSamples, sizeof(quint16));
                currentPacketPtr += sizeof(quint16);

                // memcpy the three float positions
                memcpy(currentPacketPtr, &headPosition, sizeof(headPosition));
                currentPacketPtr += (sizeof(headPosition));
                
                // memcpy our orientation
                memcpy(currentPacketPtr, &headOrientation, sizeof(headOrientation));
                currentPacketPtr += sizeof(headOrientation);

            } else {
                // set the mono/stereo byte
                *currentPacketPtr++ = isStereo;

                // memcpy the three float positions
                memcpy(currentPacketPtr, &headPosition, sizeof(headPosition));
                currentPacketPtr += (sizeof(headPosition));
                
                // memcpy our orientation
                memcpy(currentPacketPtr, &headOrientation, sizeof(headOrientation));
                currentPacketPtr += sizeof(headOrientation);

                // audio samples have already been packed (written to networkAudioSamples)
                currentPacketPtr += numNetworkBytes;
            }

            _stats.sentPacket();

            int packetBytes = currentPacketPtr - audioDataPacket;
            nodeList->writeDatagram(audioDataPacket, packetBytes, audioMixer);
            _outgoingAvatarAudioSequenceNumber++;

            Application::getInstance()->getBandwidthMeter()->outputStream(BandwidthMeter::AUDIO)
                .updateValue(packetBytes);
        }
        delete[] inputAudioSamples;
    }
}

void Audio::processReceivedSamples(const QByteArray& inputBuffer, QByteArray& outputBuffer) {
    const int numNetworkOutputSamples = inputBuffer.size() / sizeof(int16_t);
    const int numDeviceOutputSamples = numNetworkOutputSamples * (_outputFormat.sampleRate() * _outputFormat.channelCount())
        / (_desiredOutputFormat.sampleRate() * _desiredOutputFormat.channelCount());

    outputBuffer.resize(numDeviceOutputSamples * sizeof(int16_t));

    const int16_t* receivedSamples;
    // copy the samples we'll resample from the ring buffer - this also
    // pushes the read pointer of the ring buffer forwards
    //receivedAudioStreamPopOutput.readSamples(receivedSamples, numNetworkOutputSamples);

    receivedSamples = reinterpret_cast<const int16_t*>(inputBuffer.data());

    // copy the packet from the RB to the output
    linearResampling(receivedSamples,
        (int16_t*)outputBuffer.data(),
        numNetworkOutputSamples,
        numDeviceOutputSamples,
        _desiredOutputFormat, _outputFormat);
    
    if(_reverb || _receivedAudioStream.hasReverb()) {
        updateGverbOptions();
        addReverb(_gverb, (int16_t*)outputBuffer.data(), numDeviceOutputSamples, _outputFormat);
    }
}

void Audio::sendMuteEnvironmentPacket() {
    QByteArray mutePacket = byteArrayWithPopulatedHeader(PacketTypeMuteEnvironment);
    QDataStream mutePacketStream(&mutePacket, QIODevice::Append);
    
    const float MUTE_RADIUS = 50;
    
    mutePacketStream.writeBytes(reinterpret_cast<const char *>(&Application::getInstance()->getAvatar()->getPosition()),
                                sizeof(glm::vec3));
    mutePacketStream.writeBytes(reinterpret_cast<const char *>(&MUTE_RADIUS), sizeof(float));
    
    // grab our audio mixer from the NodeList, if it exists
    auto nodelist = DependencyManager::get<NodeList>();
    SharedNodePointer audioMixer = nodelist->soloNodeOfType(NodeType::AudioMixer);
    
    if (audioMixer) {
        // send off this mute packet
        nodelist->writeDatagram(mutePacket, audioMixer);
    }
}

void Audio::addReceivedAudioToStream(const QByteArray& audioByteArray) {
    if (_audioOutput) {
        // Audio output must exist and be correctly set up if we're going to process received audio
        _receivedAudioStream.parseData(audioByteArray);
    }

    Application::getInstance()->getBandwidthMeter()->inputStream(BandwidthMeter::AUDIO).updateValue(audioByteArray.size());
}

void Audio::parseAudioEnvironmentData(const QByteArray &packet) {
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

void Audio::toggleMute() {
    _muted = !_muted;
    muteToggled();
}

void Audio::setIsStereoInput(bool isStereoInput) {
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

void Audio::toggleAudioSourceInject() {
    _audioSourceInjectEnabled = !_audioSourceInjectEnabled;
}

void Audio::selectAudioSourcePinkNoise() {
    _noiseSourceEnabled = true;
    _toneSourceEnabled = false;
}
 
void Audio::selectAudioSourceSine440() {
    _toneSourceEnabled = true;
    _noiseSourceEnabled = false;
}

bool Audio::outputLocalInjector(bool isStereo, qreal volume, AudioInjector* injector) {
    if (injector->getLocalBuffer()) {
        QAudioFormat localFormat = _desiredOutputFormat;
        localFormat.setChannelCount(isStereo ? 2 : 1);
        
        QAudioOutput* localOutput = new QAudioOutput(getNamedAudioDeviceForMode(QAudio::AudioOutput, _outputAudioDeviceName),
                                                     localFormat);
        localOutput->setVolume(volume);
        
        // move the localOutput to the same thread as the local injector buffer
        localOutput->moveToThread(injector->getLocalBuffer()->thread());
        
        // have it be cleaned up when that thread is done
        connect(injector->thread(), &QThread::finished, localOutput, &QAudioOutput::stop);
        connect(injector->thread(), &QThread::finished, localOutput, &QAudioOutput::deleteLater);
        
        qDebug() << "Starting QAudioOutput for local injector" << localOutput;
        
        localOutput->start(injector->getLocalBuffer());
        return localOutput->state() == QAudio::ActiveState;
    }
    
    return false;
}


void Audio::outputFormatChanged() {
    int outputFormatChannelCountTimesSampleRate = _outputFormat.channelCount() * _outputFormat.sampleRate();
    _outputFrameSize = AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL * outputFormatChannelCountTimesSampleRate / _desiredOutputFormat.sampleRate();
    _receivedAudioStream.outputFormatChanged(outputFormatChannelCountTimesSampleRate);
}

bool Audio::switchInputToAudioDevice(const QAudioDeviceInfo& inputDeviceInfo) {
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

    if (!inputDeviceInfo.isNull()) {
        qDebug() << "The audio input device " << inputDeviceInfo.deviceName() << "is available.";
        _inputAudioDeviceName = inputDeviceInfo.deviceName().trimmed();
    
        if (adjustedFormatForAudioDevice(inputDeviceInfo, _desiredInputFormat, _inputFormat)) {
            qDebug() << "The format to be used for audio input is" << _inputFormat;
            
            // if the user wants stereo but this device can't provide then bail
            if (!_isStereoInput || _inputFormat.channelCount() == 2) {
                _audioInput = new QAudioInput(inputDeviceInfo, _inputFormat, this);
                _numInputCallbackBytes = calculateNumberOfInputCallbackBytes(_inputFormat);
                _audioInput->setBufferSize(_numInputCallbackBytes);
                
                // how do we want to handle input working, but output not working?
                int numFrameSamples = calculateNumberOfFrameSamples(_numInputCallbackBytes);
                _inputRingBuffer.resizeForFrameSize(numFrameSamples);
                _inputDevice = _audioInput->start();
                connect(_inputDevice, SIGNAL(readyRead()), this, SLOT(handleAudioInput()));
                
                supportedFormat = true;
            }
        }
    }
    
    return supportedFormat;
}

void Audio::outputNotify() {
    int recentUnfulfilled = _audioOutputIODevice.getRecentUnfulfilledReads();
    if (recentUnfulfilled > 0) {
        if (_outputStarveDetectionEnabled) {
            quint64 now = usecTimestampNow() / 1000;
            quint64 dt = now - _outputStarveDetectionStartTimeMsec;
            if (dt > _outputStarveDetectionPeriodMsec) {
                _outputStarveDetectionStartTimeMsec = now;
                _outputStarveDetectionCount = 0;
            } else {
                _outputStarveDetectionCount += recentUnfulfilled;
                if (_outputStarveDetectionCount > _outputStarveDetectionThreshold) {
                    int newOutputBufferSizeFrames = _outputBufferSizeFrames + 1;
                    qDebug() << "Starve detection threshold met, increasing buffer size to " << newOutputBufferSizeFrames;
                    setOutputBufferSize(newOutputBufferSizeFrames);

                    _outputStarveDetectionStartTimeMsec = now;
                    _outputStarveDetectionCount = 0;
                }
            }
        }
    }
}

bool Audio::switchOutputToAudioDevice(const QAudioDeviceInfo& outputDeviceInfo) {
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

    if (!outputDeviceInfo.isNull()) {
        qDebug() << "The audio output device " << outputDeviceInfo.deviceName() << "is available.";
        _outputAudioDeviceName = outputDeviceInfo.deviceName().trimmed();

        if (adjustedFormatForAudioDevice(outputDeviceInfo, _desiredOutputFormat, _outputFormat)) {
            qDebug() << "The format to be used for audio output is" << _outputFormat;
            
            outputFormatChanged();

            // setup our general output device for audio-mixer audio
            _audioOutput = new QAudioOutput(outputDeviceInfo, _outputFormat, this);
            _audioOutput->setBufferSize(_outputBufferSizeFrames * _outputFrameSize * sizeof(int16_t));

            connect(_audioOutput, &QAudioOutput::notify, this, &Audio::outputNotify);

            qDebug() << "Output Buffer capacity in frames: " << _audioOutput->bufferSize() / sizeof(int16_t) / (float)_outputFrameSize;
            
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

void Audio::setOutputBufferSize(int numFrames) {
    numFrames = std::min(std::max(numFrames, MIN_AUDIO_OUTPUT_BUFFER_SIZE_FRAMES), MAX_AUDIO_OUTPUT_BUFFER_SIZE_FRAMES);
    if (numFrames != _outputBufferSizeFrames) {
        qDebug() << "Audio output buffer size (frames): " << numFrames;
        _outputBufferSizeFrames = numFrames;

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
const float Audio::CALLBACK_ACCELERATOR_RATIO = 0.1f;
#endif

#ifdef Q_OS_MAC
const float Audio::CALLBACK_ACCELERATOR_RATIO = 2.0f;
#endif

#ifdef Q_OS_LINUX
const float Audio::CALLBACK_ACCELERATOR_RATIO = 2.0f;
#endif

int Audio::calculateNumberOfInputCallbackBytes(const QAudioFormat& format) const {
    int numInputCallbackBytes = (int)(((AudioConstants::NETWORK_FRAME_BYTES_PER_CHANNEL
        * format.channelCount()
        * (format.sampleRate() / AudioConstants::SAMPLE_RATE))
        / CALLBACK_ACCELERATOR_RATIO) + 0.5f);

    return numInputCallbackBytes;
}

float Audio::calculateDeviceToNetworkInputRatio(int numBytes) const {
    float inputToNetworkInputRatio = (int)((_numInputCallbackBytes 
        * CALLBACK_ACCELERATOR_RATIO
        / AudioConstants::NETWORK_FRAME_BYTES_PER_CHANNEL) + 0.5f);

    return inputToNetworkInputRatio;
}

int Audio::calculateNumberOfFrameSamples(int numBytes) const {
    int frameSamples = (int)(numBytes * CALLBACK_ACCELERATOR_RATIO + 0.5f) / sizeof(int16_t);
    return frameSamples;
}

float Audio::getInputRingBufferMsecsAvailable() const {
    int bytesInInputRingBuffer = _inputRingBuffer.samplesAvailable() * sizeof(int16_t);
    float msecsInInputRingBuffer = bytesInInputRingBuffer / (float)(_inputFormat.bytesForDuration(USECS_PER_MSEC));
    return  msecsInInputRingBuffer;
}

float Audio::getAudioOutputMsecsUnplayed() const {
    if (!_audioOutput) {
        return 0.0f;
    }
    int bytesAudioOutputUnplayed = _audioOutput->bufferSize() - _audioOutput->bytesFree();
    float msecsAudioOutputUnplayed = bytesAudioOutputUnplayed / (float)_outputFormat.bytesForDuration(USECS_PER_MSEC);
    return msecsAudioOutputUnplayed;
}

qint64 Audio::AudioOutputIODevice::readData(char * data, qint64 maxSize) {
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

void Audio::checkDevices() {
    QVector<QString> inputDevices = getDeviceNames(QAudio::AudioInput);
    QVector<QString> outputDevices = getDeviceNames(QAudio::AudioOutput);

    if (inputDevices != _inputDevices || outputDevices != _outputDevices) {
        _inputDevices = inputDevices;
        _outputDevices = outputDevices;

        emit deviceChanged();
    }
}

void Audio::loadSettings() {
    _receivedAudioStream.loadSettings();
    
    setOutputStarveDetectionEnabled(SettingHandles::audioOutputStarveDetectionEnabled.get());
    setOutputStarveDetectionThreshold(SettingHandles::audioOutputStarveDetectionThreshold.get());
    setOutputStarveDetectionPeriod(SettingHandles::audioOutputStarveDetectionPeriod.get());
    
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setOutputBufferSize",
                                  Q_ARG(int, SettingHandles::audioOutputBufferSize.get()));
    } else {
        setOutputBufferSize(SettingHandles::audioOutputBufferSize.get());
    }
}

void Audio::saveSettings() {
    _receivedAudioStream.saveSettings();
    
    SettingHandles::audioOutputStarveDetectionEnabled.set(getOutputStarveDetectionEnabled());
    SettingHandles::audioOutputStarveDetectionThreshold.set(getOutputStarveDetectionThreshold());
    SettingHandles::audioOutputStarveDetectionPeriod.set(getOutputStarveDetectionPeriod());
    SettingHandles::audioOutputBufferSize.set(getOutputBufferSize());
}

