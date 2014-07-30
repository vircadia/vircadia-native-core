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
#include <sys/stat.h>

#include <math.h>

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
#endif

#include <QtCore/QBuffer>
#include <QtMultimedia/QAudioInput>
#include <QtMultimedia/QAudioOutput>
#include <QSvgRenderer>

#include <NodeList.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <StdDev.h>
#include <UUID.h>
#include <glm/glm.hpp>

#include "Audio.h"
#include "Menu.h"
#include "Util.h"
#include "PositionalAudioStream.h"

static const float AUDIO_CALLBACK_MSECS = (float) NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL / (float)SAMPLE_RATE * 1000.0;

static const int NUMBER_OF_NOISE_SAMPLE_FRAMES = 300;

static const int FRAMES_AVAILABLE_STATS_WINDOW_SECONDS = 10;

// Mute icon configration
static const int MUTE_ICON_SIZE = 24;


Audio::Audio(QObject* parent) :
    AbstractAudioInterface(parent),
    _audioInput(NULL),
    _desiredInputFormat(),
    _inputFormat(),
    _numInputCallbackBytes(0),
    _audioOutput(NULL),
    _desiredOutputFormat(),
    _outputFormat(),
    _outputDevice(NULL),
    _numOutputCallbackBytes(0),
    _loopbackAudioOutput(NULL),
    _loopbackOutputDevice(NULL),
    _proceduralAudioOutput(NULL),
    _proceduralOutputDevice(NULL),

    // NOTE: Be very careful making changes to the initializers of these ring buffers. There is a known problem with some
    // Mac audio devices that slowly introduce additional delay in the audio device because they play out audio slightly
    // slower than real time (or at least the desired sample rate). If you increase the size of the ring buffer, then it 
    // this delay will slowly add up and the longer someone runs, they more delayed their audio will be.
    _inputRingBuffer(0),
    _receivedAudioStream(NETWORK_BUFFER_LENGTH_SAMPLES_STEREO, 100, true, 0, true),
    _isStereoInput(false),
    _averagedLatency(0.0),
    _lastInputLoudness(0),
    _timeSinceLastClip(-1.0),
    _dcOffset(0),
    _noiseGateMeasuredFloor(0),
    _noiseGateSampleCounter(0),
    _noiseGateOpen(false),
    _noiseGateEnabled(true),
    _toneInjectionEnabled(false),
    _noiseGateFramesToClose(0),
    _totalInputAudioSamples(0),
    _collisionSoundMagnitude(0.0f),
    _collisionSoundFrequency(0.0f),
    _collisionSoundNoise(0.0f),
    _collisionSoundDuration(0.0f),
    _proceduralEffectSample(0),
    _muted(false),
    _processSpatialAudio(false),
    _spatialAudioStart(0),
    _spatialAudioFinish(0),
    _spatialAudioRingBuffer(NETWORK_BUFFER_LENGTH_SAMPLES_STEREO, true), // random access mode
    _scopeEnabled(false),
    _scopeEnabledPause(false),
    _scopeInputOffset(0),
    _scopeOutputOffset(0),
    _framesPerScope(DEFAULT_FRAMES_PER_SCOPE),
    _samplesPerScope(NETWORK_SAMPLES_PER_FRAME * _framesPerScope),
    _scopeInput(0),
    _scopeOutputLeft(0),
    _scopeOutputRight(0),
    _statsEnabled(false),
    _outgoingAvatarAudioSequenceNumber(0),
    _audioInputMsecsReadStats(MSECS_PER_SECOND / (float)AUDIO_CALLBACK_MSECS * CALLBACK_ACCELERATOR_RATIO, FRAMES_AVAILABLE_STATS_WINDOW_SECONDS),
    _inputRingBufferMsecsAvailableStats(1, FRAMES_AVAILABLE_STATS_WINDOW_SECONDS),
    _audioOutputMsecsUnplayedStats(1, FRAMES_AVAILABLE_STATS_WINDOW_SECONDS)
{
    // clear the array of locally injected samples
    memset(_localProceduralSamples, 0, NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL);
    // Create the noise sample array
    _noiseSampleFrames = new float[NUMBER_OF_NOISE_SAMPLE_FRAMES];
}

void Audio::init(QGLWidget *parent) {
    _micTextureId = parent->bindTexture(QImage(Application::resourcesPath() + "images/mic.svg"));
    _muteTextureId = parent->bindTexture(QImage(Application::resourcesPath() + "images/mic-mute.svg"));
    _boxTextureId = parent->bindTexture(QImage(Application::resourcesPath() + "images/audio-box.svg"));
}

void Audio::reset() {
    _receivedAudioStream.reset();

    resetStats();
}

void Audio::resetStats() {
    _receivedAudioStream.resetStats();

    _audioMixerAvatarStreamAudioStats = AudioStreamStats();
    _audioMixerInjectedStreamAudioStatsMap.clear();

    _audioInputMsecsReadStats.reset();
    _inputRingBufferMsecsAvailableStats.reset();

    _audioOutputMsecsUnplayedStats.reset();
}

void Audio::audioMixerKilled() {
    _outgoingAvatarAudioSequenceNumber = 0;
    resetStats();
}

QAudioDeviceInfo getNamedAudioDeviceForMode(QAudio::Mode mode, const QString& deviceName) {
    QAudioDeviceInfo result;
    foreach(QAudioDeviceInfo audioDevice, QAudioDeviceInfo::availableDevices(mode)) {
        qDebug() << audioDevice.deviceName() << " " << deviceName;
        if (audioDevice.deviceName().trimmed() == deviceName.trimmed()) {
            result = audioDevice;
        }
    }
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
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);
    const DWORD VISTA_MAJOR_VERSION = 6;
    if (osvi.dwMajorVersion < VISTA_MAJOR_VERSION) {// lower then vista
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
            const DWORD WINDOWS7_MAJOR_VERSION = 6;
            const DWORD WINDOWS7_MINOR_VERSION = 1;
            if (osvi.dwMajorVersion <= WINDOWS7_MAJOR_VERSION && osvi.dwMinorVersion <= WINDOWS7_MINOR_VERSION) {
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

        if (audioDevice.supportedSampleRates().contains(SAMPLE_RATE * 2)) {
            // use 48, which is a sample downsample, upsample
            adjustedAudioFormat = desiredAudioFormat;
            adjustedAudioFormat.setSampleRate(SAMPLE_RATE * 2);

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

void linearResampling(int16_t* sourceSamples, int16_t* destinationSamples,
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
    _desiredInputFormat.setSampleRate(SAMPLE_RATE);
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
}

void Audio::stop() {
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

void Audio::handleAudioInput() {
    static char audioDataPacket[MAX_PACKET_SIZE];

    static int numBytesPacketHeader = numBytesForPacketHeaderGivenPacketType(PacketTypeMicrophoneAudioNoEcho);
    static int leadingBytes = numBytesPacketHeader + sizeof(quint16) + sizeof(glm::vec3) + sizeof(glm::quat) + sizeof(quint8);

    static int16_t* networkAudioSamples = (int16_t*) (audioDataPacket + leadingBytes);

    float inputToNetworkInputRatio = calculateDeviceToNetworkInputRatio(_numInputCallbackBytes);

    int inputSamplesRequired = (int)((float)NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL * inputToNetworkInputRatio);

    QByteArray inputByteArray = _inputDevice->readAll();
    
    if (Menu::getInstance()->isOptionChecked(MenuOption::EchoLocalAudio) && !_muted && _audioOutput) {
        // if this person wants local loopback add that to the locally injected audio

        if (!_loopbackOutputDevice && _loopbackAudioOutput) {
            // we didn't have the loopback output device going so set that up now
            _loopbackOutputDevice = _loopbackAudioOutput->start();
        }
        
        if (_inputFormat == _outputFormat) {
            if (_loopbackOutputDevice) {
                _loopbackOutputDevice->write(inputByteArray);
            }
        } else {
            float loopbackOutputToInputRatio = (_outputFormat.sampleRate() / (float) _inputFormat.sampleRate())
                * (_outputFormat.channelCount() / _inputFormat.channelCount());

            QByteArray loopBackByteArray(inputByteArray.size() * loopbackOutputToInputRatio, 0);

            linearResampling((int16_t*) inputByteArray.data(), (int16_t*) loopBackByteArray.data(),
                             inputByteArray.size() / sizeof(int16_t),
                             loopBackByteArray.size() / sizeof(int16_t), _inputFormat, _outputFormat);

            if (_loopbackOutputDevice) {
                _loopbackOutputDevice->write(loopBackByteArray);
            }
        }
    }

    _inputRingBuffer.writeData(inputByteArray.data(), inputByteArray.size());
    
    float audioInputMsecsRead = inputByteArray.size() / (float)(_inputFormat.bytesForDuration(USECS_PER_MSEC));
    _audioInputMsecsReadStats.update(audioInputMsecsRead);

    while (_inputRingBuffer.samplesAvailable() >= inputSamplesRequired) {

        int16_t* inputAudioSamples = new int16_t[inputSamplesRequired];
        _inputRingBuffer.readSamples(inputAudioSamples, inputSamplesRequired);
        
        const int numNetworkBytes = _isStereoInput ? NETWORK_BUFFER_LENGTH_BYTES_STEREO : NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL;
        const int numNetworkSamples = _isStereoInput ? NETWORK_BUFFER_LENGTH_SAMPLES_STEREO : NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL;

        // zero out the monoAudioSamples array and the locally injected audio
        memset(networkAudioSamples, 0, numNetworkBytes);

        if (!_muted) {
            // we aren't muted, downsample the input audio
            linearResampling((int16_t*) inputAudioSamples, networkAudioSamples,
                             inputSamplesRequired,  numNetworkSamples,
                             _inputFormat, _desiredInputFormat);
            
            // only impose the noise gate and perform tone injection if we sending mono audio
            if (!_isStereoInput) {
                
                //
                //  Impose Noise Gate
                //
                //  The Noise Gate is used to reject constant background noise by measuring the noise
                //  floor observed at the microphone and then opening the 'gate' to allow microphone
                //  signals to be transmitted when the microphone samples average level exceeds a multiple
                //  of the noise floor.
                //
                //  NOISE_GATE_HEIGHT:  How loud you have to speak relative to noise background to open the gate.
                //                      Make this value lower for more sensitivity and less rejection of noise.
                //  NOISE_GATE_WIDTH:   The number of samples in an audio frame for which the height must be exceeded
                //                      to open the gate.
                //  NOISE_GATE_CLOSE_FRAME_DELAY:  Once the noise is below the gate height for the frame, how many frames
                //                      will we wait before closing the gate.
                //  NOISE_GATE_FRAMES_TO_AVERAGE:  How many audio frames should we average together to compute noise floor.
                //                      More means better rejection but also can reject continuous things like singing.
                // NUMBER_OF_NOISE_SAMPLE_FRAMES:  How often should we re-evaluate the noise floor?
                
                
                float loudness = 0;
                float thisSample = 0;
                int samplesOverNoiseGate = 0;
                
                const float NOISE_GATE_HEIGHT = 7.0f;
                const int NOISE_GATE_WIDTH = 5;
                const int NOISE_GATE_CLOSE_FRAME_DELAY = 5;
                const int NOISE_GATE_FRAMES_TO_AVERAGE = 5;
                const float DC_OFFSET_AVERAGING = 0.99f;
                const float CLIPPING_THRESHOLD = 0.90f;
                
                //
                //  Check clipping, adjust DC offset, and check if should open noise gate
                //
                float measuredDcOffset = 0.0f;
                //  Increment the time since the last clip
                if (_timeSinceLastClip >= 0.0f) {
                    _timeSinceLastClip += (float) NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL / (float) SAMPLE_RATE;
                }
                
                for (int i = 0; i < NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL; i++) {
                    measuredDcOffset += networkAudioSamples[i];
                    networkAudioSamples[i] -= (int16_t) _dcOffset;
                    thisSample = fabsf(networkAudioSamples[i]);
                    if (thisSample >= (32767.0f * CLIPPING_THRESHOLD)) {
                        _timeSinceLastClip = 0.0f;
                    }
                    loudness += thisSample;
                    //  Noise Reduction:  Count peaks above the average loudness
                    if (_noiseGateEnabled && (thisSample > (_noiseGateMeasuredFloor * NOISE_GATE_HEIGHT))) {
                        samplesOverNoiseGate++;
                    }
                }
                
                measuredDcOffset /= NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL;
                if (_dcOffset == 0.0f) {
                    // On first frame, copy over measured offset
                    _dcOffset = measuredDcOffset;
                } else {
                    _dcOffset = DC_OFFSET_AVERAGING * _dcOffset + (1.0f - DC_OFFSET_AVERAGING) * measuredDcOffset;
                }
                
                //  Add tone injection if enabled
                const float TONE_FREQ = 220.0f / SAMPLE_RATE * TWO_PI;
                const float QUARTER_VOLUME = 8192.0f;
                if (_toneInjectionEnabled) {
                    loudness = 0.0f;
                    for (int i = 0; i < NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL; i++) {
                        networkAudioSamples[i] = QUARTER_VOLUME * sinf(TONE_FREQ * (float)(i + _proceduralEffectSample));
                        loudness += fabsf(networkAudioSamples[i]);
                    }
                }
                _lastInputLoudness = fabs(loudness / NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL);
                
                //  If Noise Gate is enabled, check and turn the gate on and off
                if (!_toneInjectionEnabled && _noiseGateEnabled) {
                    float averageOfAllSampleFrames = 0.0f;
                    _noiseSampleFrames[_noiseGateSampleCounter++] = _lastInputLoudness;
                    if (_noiseGateSampleCounter == NUMBER_OF_NOISE_SAMPLE_FRAMES) {
                        float smallestSample = FLT_MAX;
                        for (int i = 0; i <= NUMBER_OF_NOISE_SAMPLE_FRAMES - NOISE_GATE_FRAMES_TO_AVERAGE; i += NOISE_GATE_FRAMES_TO_AVERAGE) {
                            float thisAverage = 0.0f;
                            for (int j = i; j < i + NOISE_GATE_FRAMES_TO_AVERAGE; j++) {
                                thisAverage += _noiseSampleFrames[j];
                                averageOfAllSampleFrames += _noiseSampleFrames[j];
                            }
                            thisAverage /= NOISE_GATE_FRAMES_TO_AVERAGE;
                            
                            if (thisAverage < smallestSample) {
                                smallestSample = thisAverage;
                            }
                        }
                        averageOfAllSampleFrames /= NUMBER_OF_NOISE_SAMPLE_FRAMES;
                        _noiseGateMeasuredFloor = smallestSample;
                        _noiseGateSampleCounter = 0;
                        
                    }
                    if (samplesOverNoiseGate > NOISE_GATE_WIDTH) {
                        _noiseGateOpen = true;
                        _noiseGateFramesToClose = NOISE_GATE_CLOSE_FRAME_DELAY;
                    } else {
                        if (--_noiseGateFramesToClose == 0) {
                            _noiseGateOpen = false;
                        }
                    }
                    if (!_noiseGateOpen) {
                        memset(networkAudioSamples, 0, NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL);
                        _lastInputLoudness = 0;
                    }
                }
            } else {
                float loudness = 0.0f;
                
                for (int i = 0; i < NETWORK_BUFFER_LENGTH_SAMPLES_STEREO; i++) {
                    loudness += fabsf(networkAudioSamples[i]);
                }
                
                _lastInputLoudness = fabs(loudness / NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL);
            }
        } else {
            // our input loudness is 0, since we're muted
            _lastInputLoudness = 0;
        }
        
        // at this point we have clean monoAudioSamples, which match our target output... 
        // this is what we should send to our interested listeners
        if (_processSpatialAudio && !_muted && !_isStereoInput && _audioOutput) {
            QByteArray monoInputData((char*)networkAudioSamples, NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL * sizeof(int16_t));
            emit processLocalAudio(_spatialAudioStart, monoInputData, _desiredInputFormat);
        }
        
        if (!_isStereoInput && _proceduralAudioOutput) {
            processProceduralAudio(networkAudioSamples, NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL);
        }

        if (!_isStereoInput && _scopeEnabled && !_scopeEnabledPause) {
            unsigned int numMonoAudioChannels = 1;
            unsigned int monoAudioChannel = 0;
            addBufferToScope(_scopeInput, _scopeInputOffset, networkAudioSamples, monoAudioChannel, numMonoAudioChannels);
            _scopeInputOffset += NETWORK_SAMPLES_PER_FRAME;
            _scopeInputOffset %= _samplesPerScope;
        }

        NodeList* nodeList = NodeList::getInstance();
        SharedNodePointer audioMixer = nodeList->soloNodeOfType(NodeType::AudioMixer);
        
        if (audioMixer && audioMixer->getActiveSocket()) {
            MyAvatar* interfaceAvatar = Application::getInstance()->getAvatar();
            glm::vec3 headPosition = interfaceAvatar->getHead()->getPosition();
            glm::quat headOrientation = interfaceAvatar->getHead()->getFinalOrientationInWorldFrame();
            quint8 isStereo = _isStereoInput ? 1 : 0;
            
            int numAudioBytes = 0;
            
            PacketType packetType;
            if (_lastInputLoudness == 0) {
                packetType = PacketTypeSilentAudioFrame;
                
                // we need to indicate how many silent samples this is to the audio mixer
                networkAudioSamples[0] = numNetworkSamples;
                numAudioBytes = sizeof(int16_t);
            } else {
                numAudioBytes = numNetworkBytes;
                
                if (Menu::getInstance()->isOptionChecked(MenuOption::EchoServerAudio)) {
                    packetType = PacketTypeMicrophoneAudioWithEcho;
                } else {
                    packetType = PacketTypeMicrophoneAudioNoEcho;
                }
            }

            char* currentPacketPtr = audioDataPacket + populatePacketHeader(audioDataPacket, packetType);
            
            // pack sequence number
            memcpy(currentPacketPtr, &_outgoingAvatarAudioSequenceNumber, sizeof(quint16));
            currentPacketPtr += sizeof(quint16);

            // set the mono/stereo byte
            *currentPacketPtr++ = isStereo;

            // memcpy the three float positions
            memcpy(currentPacketPtr, &headPosition, sizeof(headPosition));
            currentPacketPtr += (sizeof(headPosition));

            // memcpy our orientation
            memcpy(currentPacketPtr, &headOrientation, sizeof(headOrientation));
            currentPacketPtr += sizeof(headOrientation);
            
            nodeList->writeDatagram(audioDataPacket, numAudioBytes + leadingBytes, audioMixer);
            _outgoingAvatarAudioSequenceNumber++;

            Application::getInstance()->getBandwidthMeter()->outputStream(BandwidthMeter::AUDIO)
                .updateValue(numAudioBytes + leadingBytes);
        }
        delete[] inputAudioSamples;
    }

    pushAudioToOutput();
}

void Audio::addReceivedAudioToStream(const QByteArray& audioByteArray) {
    if (_audioOutput) {
        // Audio output must exist and be correctly set up if we're going to process received audio
        processReceivedAudio(audioByteArray);
    }

    Application::getInstance()->getBandwidthMeter()->inputStream(BandwidthMeter::AUDIO).updateValue(audioByteArray.size());
}

void Audio::parseAudioStreamStatsPacket(const QByteArray& packet) {

    int numBytesPacketHeader = numBytesForPacketHeader(packet);
    const char* dataAt = packet.constData() + numBytesPacketHeader;

    // parse the appendFlag, clear injected audio stream stats if 0
    quint8 appendFlag = *(reinterpret_cast<const quint16*>(dataAt));
    dataAt += sizeof(quint8);
    if (!appendFlag) {
        _audioMixerInjectedStreamAudioStatsMap.clear();
    }

    // parse the number of stream stats structs to follow
    quint16 numStreamStats = *(reinterpret_cast<const quint16*>(dataAt));
    dataAt += sizeof(quint16);

    // parse the stream stats
    AudioStreamStats streamStats;
    for (quint16 i = 0; i < numStreamStats; i++) {
        memcpy(&streamStats, dataAt, sizeof(AudioStreamStats));
        dataAt += sizeof(AudioStreamStats);

        if (streamStats._streamType == PositionalAudioStream::Microphone) {
            _audioMixerAvatarStreamAudioStats = streamStats;
        } else {
            _audioMixerInjectedStreamAudioStatsMap[streamStats._streamIdentifier] = streamStats;
        }
    }
}

AudioStreamStats Audio::getDownstreamAudioStreamStats() const {
    return _receivedAudioStream.getAudioStreamStats();
}

void Audio::sendDownstreamAudioStatsPacket() {

    // since this function is called every second, we'll sample some of our stats here

    _inputRingBufferMsecsAvailableStats.update(getInputRingBufferMsecsAvailable());

    _audioOutputMsecsUnplayedStats.update(getAudioOutputMsecsUnplayed());

    char packet[MAX_PACKET_SIZE];

    // pack header
    int numBytesPacketHeader = populatePacketHeader(packet, PacketTypeAudioStreamStats);
    char* dataAt = packet + numBytesPacketHeader;

    // pack append flag
    quint8 appendFlag = 0;
    memcpy(dataAt, &appendFlag, sizeof(quint8));
    dataAt += sizeof(quint8);

    // pack number of stats packed
    quint16 numStreamStatsToPack = 1;
    memcpy(dataAt, &numStreamStatsToPack, sizeof(quint16));
    dataAt += sizeof(quint16);

    // pack downstream audio stream stats
    AudioStreamStats stats = _receivedAudioStream.updateSeqHistoryAndGetAudioStreamStats();
    memcpy(dataAt, &stats, sizeof(AudioStreamStats));
    dataAt += sizeof(AudioStreamStats);
    
    // send packet
    NodeList* nodeList = NodeList::getInstance();
    SharedNodePointer audioMixer = nodeList->soloNodeOfType(NodeType::AudioMixer);
    nodeList->writeDatagram(packet, dataAt - packet, audioMixer);
}

// NOTE: numSamples is the total number of single channel samples, since callers will always call this with stereo
// data we know that we will have 2x samples for each stereo time sample at the format's sample rate
void Audio::addSpatialAudioToBuffer(unsigned int sampleTime, const QByteArray& spatialAudio, unsigned int numSamples) {
    // Calculate the number of remaining samples available. The source spatial audio buffer will get
    // clipped if there are insufficient samples available in the accumulation buffer.
    unsigned int remaining = _spatialAudioRingBuffer.getSampleCapacity() - _spatialAudioRingBuffer.samplesAvailable();

    // Locate where in the accumulation buffer the new samples need to go
    if (sampleTime >= _spatialAudioFinish) {
        if (_spatialAudioStart == _spatialAudioFinish) {
            // Nothing in the spatial audio ring buffer yet, Just do a straight copy, clipping if necessary
            unsigned int sampleCount = (remaining < numSamples) ? remaining : numSamples;
            if (sampleCount) {
                _spatialAudioRingBuffer.writeSamples((int16_t*)spatialAudio.data(), sampleCount);
            }
            _spatialAudioFinish = _spatialAudioStart + sampleCount / _desiredOutputFormat.channelCount();
        } else {
            // Spatial audio ring buffer already has data, but there is no overlap with the new sample.
            // Compute the appropriate time delay and pad with silence until the new start time.
            unsigned int delay = sampleTime - _spatialAudioFinish;
            unsigned int delayCount = delay * _desiredOutputFormat.channelCount();
            unsigned int silentCount = (remaining < delayCount) ? remaining : delayCount;
            if (silentCount) {
               _spatialAudioRingBuffer.addSilentFrame(silentCount);
            }

            // Recalculate the number of remaining samples
            remaining -= silentCount;
            unsigned int sampleCount = (remaining < numSamples) ? remaining : numSamples;

            // Copy the new spatial audio to the accumulation ring buffer
            if (sampleCount) {
                _spatialAudioRingBuffer.writeSamples((int16_t*)spatialAudio.data(), sampleCount);
            }
            _spatialAudioFinish += (sampleCount + silentCount) / _desiredOutputFormat.channelCount();
        }
    } else {
        // There is overlap between the spatial audio buffer and the new sample, mix the overlap
        // Calculate the offset from the buffer's current read position, which should be located at _spatialAudioStart
        unsigned int offset = (sampleTime - _spatialAudioStart) * _desiredOutputFormat.channelCount();
        unsigned int mixedSamplesCount = (_spatialAudioFinish - sampleTime) * _desiredOutputFormat.channelCount();
        mixedSamplesCount = (mixedSamplesCount < numSamples) ? mixedSamplesCount : numSamples;

        const int16_t* spatial = reinterpret_cast<const int16_t*>(spatialAudio.data());
        for (unsigned int i = 0; i < mixedSamplesCount; i++) {
            int existingSample = _spatialAudioRingBuffer[i + offset];
            int newSample = spatial[i];
            int sumOfSamples = existingSample + newSample;
            _spatialAudioRingBuffer[i + offset] = static_cast<int16_t>(glm::clamp<int>(sumOfSamples, 
                                                    std::numeric_limits<short>::min(), std::numeric_limits<short>::max()));
        }

        // Copy the remaining unoverlapped spatial audio to the spatial audio buffer, if any
        unsigned int nonMixedSampleCount = numSamples - mixedSamplesCount;
        nonMixedSampleCount = (remaining < nonMixedSampleCount) ? remaining : nonMixedSampleCount;
        if (nonMixedSampleCount) {
            _spatialAudioRingBuffer.writeSamples((int16_t*)spatialAudio.data() + mixedSamplesCount, nonMixedSampleCount);
            // Extend the finish time by the amount of unoverlapped samples
            _spatialAudioFinish += nonMixedSampleCount / _desiredOutputFormat.channelCount();
        }
    }
}

bool Audio::mousePressEvent(int x, int y) {
    if (_iconBounds.contains(x, y)) {
        toggleMute();
        return true;
    }
    return false;
}

void Audio::toggleMute() {
    _muted = !_muted;
    muteToggled();
}

void Audio::toggleAudioNoiseReduction() {
    _noiseGateEnabled = !_noiseGateEnabled;
}

void Audio::toggleStereoInput() {
    int oldChannelCount = _desiredInputFormat.channelCount();
    QAction* stereoAudioOption = Menu::getInstance()->getActionForOption(MenuOption::StereoAudio);

    if (stereoAudioOption->isChecked()) {
        _desiredInputFormat.setChannelCount(2);
        _isStereoInput = true;
    } else {
        _desiredInputFormat.setChannelCount(1);
        _isStereoInput = false;
    }
    
    if (oldChannelCount != _desiredInputFormat.channelCount()) {
        // change in channel count for desired input format, restart the input device
        switchInputToAudioDevice(_inputAudioDeviceName);
    }
}

void Audio::processReceivedAudio(const QByteArray& audioByteArray) {

    // parse audio data
    _receivedAudioStream.parseData(audioByteArray);


    // This call has been moved to handleAudioInput. handleAudioInput is called at a much more regular interval
    // than processReceivedAudio since handleAudioInput does not experience network-related jitter.
    // This way, we reduce the jitter of the frames being pushed to the audio output, allowing us to use a reduced
    // buffer size for it, which reduces latency.

    //pushAudioToOutput();
}

void Audio::pushAudioToOutput() {

    if (_audioOutput->bytesFree() == _audioOutput->bufferSize()) {
        // the audio output has no samples to play.  set the downstream audio to starved so that it
        // refills to its desired size before pushing frames
        _receivedAudioStream.setToStarved();
    }

    float networkOutputToOutputRatio = (_desiredOutputFormat.sampleRate() / (float)_outputFormat.sampleRate())
        * (_desiredOutputFormat.channelCount() / (float)_outputFormat.channelCount());

    int numFramesToPush;
    if (Menu::getInstance()->isOptionChecked(MenuOption::DisableQAudioOutputOverflowCheck)) {
        numFramesToPush = _receivedAudioStream.getFramesAvailable();
    } else {
        // make sure to push a whole number of frames to the audio output
        int numFramesAudioOutputRoomFor = (int)(_audioOutput->bytesFree() / sizeof(int16_t) * networkOutputToOutputRatio) / _receivedAudioStream.getNumFrameSamples();
        numFramesToPush = std::min(_receivedAudioStream.getFramesAvailable(), numFramesAudioOutputRoomFor);
    }

    // if there is data in the received stream and room in the audio output, decide what to do

    if (numFramesToPush > 0 && _receivedAudioStream.popFrames(numFramesToPush, false)) {

        int numNetworkOutputSamples = numFramesToPush * NETWORK_BUFFER_LENGTH_SAMPLES_STEREO;
        int numDeviceOutputSamples = numNetworkOutputSamples / networkOutputToOutputRatio;

        QByteArray outputBuffer;
        outputBuffer.resize(numDeviceOutputSamples * sizeof(int16_t));

        AudioRingBuffer::ConstIterator receivedAudioStreamPopOutput = _receivedAudioStream.getLastPopOutput();

        int16_t* receivedSamples = new int16_t[numNetworkOutputSamples];
        if (_processSpatialAudio) {
            unsigned int sampleTime = _spatialAudioStart;
            QByteArray buffer;
            buffer.resize(numNetworkOutputSamples * sizeof(int16_t));

            receivedAudioStreamPopOutput.readSamples((int16_t*)buffer.data(), numNetworkOutputSamples);

            // Accumulate direct transmission of audio from sender to receiver
            if (Menu::getInstance()->isOptionChecked(MenuOption::AudioSpatialProcessingIncludeOriginal)) {
                emit preProcessOriginalInboundAudio(sampleTime, buffer, _desiredOutputFormat);
                addSpatialAudioToBuffer(sampleTime, buffer, numNetworkOutputSamples);
            }

            // Send audio off for spatial processing
            emit processInboundAudio(sampleTime, buffer, _desiredOutputFormat);

            // copy the samples we'll resample from the spatial audio ring buffer - this also
            // pushes the read pointer of the spatial audio ring buffer forwards
            _spatialAudioRingBuffer.readSamples(receivedSamples, numNetworkOutputSamples);

            // Advance the start point for the next packet of audio to arrive
            _spatialAudioStart += numNetworkOutputSamples / _desiredOutputFormat.channelCount();
        } else {
            // copy the samples we'll resample from the ring buffer - this also
            // pushes the read pointer of the ring buffer forwards
            receivedAudioStreamPopOutput.readSamples(receivedSamples, numNetworkOutputSamples);
        }

        // copy the packet from the RB to the output
        linearResampling(receivedSamples,
            (int16_t*)outputBuffer.data(),
            numNetworkOutputSamples,
            numDeviceOutputSamples,
            _desiredOutputFormat, _outputFormat);

        if (_outputDevice) {
            _outputDevice->write(outputBuffer);
        }

        if (_scopeEnabled && !_scopeEnabledPause) {
            unsigned int numAudioChannels = _desiredOutputFormat.channelCount();
            int16_t* samples = receivedSamples;
            for (int numSamples = numNetworkOutputSamples / numAudioChannels; numSamples > 0; numSamples -= NETWORK_SAMPLES_PER_FRAME) {

                unsigned int audioChannel = 0;
                addBufferToScope(
                    _scopeOutputLeft,
                    _scopeOutputOffset,
                    samples, audioChannel, numAudioChannels);

                audioChannel = 1;
                addBufferToScope(
                    _scopeOutputRight,
                    _scopeOutputOffset,
                    samples, audioChannel, numAudioChannels);

                _scopeOutputOffset += NETWORK_SAMPLES_PER_FRAME;
                _scopeOutputOffset %= _samplesPerScope;
                samples += NETWORK_SAMPLES_PER_FRAME * numAudioChannels;
            }
        }

        delete[] receivedSamples;
    }
}

void Audio::processProceduralAudio(int16_t* monoInput, int numSamples) {

    // zero out the locally injected audio in preparation for audio procedural sounds
    // This is correlated to numSamples, so it really needs to be numSamples * sizeof(sample)
    memset(_localProceduralSamples, 0, NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL);
    // add procedural effects to the appropriate input samples
    addProceduralSounds(monoInput, NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL);
        
    if (!_proceduralOutputDevice) {
        _proceduralOutputDevice = _proceduralAudioOutput->start();
    }
        
    // send whatever procedural sounds we want to locally loop back to the _proceduralOutputDevice
    QByteArray proceduralOutput;
    proceduralOutput.resize(NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL * _outputFormat.sampleRate() *
        _outputFormat.channelCount() * sizeof(int16_t) / (_desiredInputFormat.sampleRate() *
            _desiredInputFormat.channelCount()));
        
    linearResampling(_localProceduralSamples,
        reinterpret_cast<int16_t*>(proceduralOutput.data()),
        NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL,
        proceduralOutput.size() / sizeof(int16_t),
        _desiredInputFormat, _outputFormat);
        
    if (_proceduralOutputDevice) {
        _proceduralOutputDevice->write(proceduralOutput);
    }
}

void Audio::toggleToneInjection() {
    _toneInjectionEnabled = !_toneInjectionEnabled;
}

void Audio::toggleAudioSpatialProcessing() {
    _processSpatialAudio = !_processSpatialAudio;
    if (_processSpatialAudio) {
        _spatialAudioStart = 0;
        _spatialAudioFinish = 0;
        _spatialAudioRingBuffer.reset();
    }
}

//  Take a pointer to the acquired microphone input samples and add procedural sounds
void Audio::addProceduralSounds(int16_t* monoInput, int numSamples) {
    float sample;
    const float COLLISION_SOUND_CUTOFF_LEVEL = 0.01f;
    const float COLLISION_SOUND_MAX_VOLUME = 1000.0f;
    const float UP_MAJOR_FIFTH = powf(1.5f, 4.0f);
    const float DOWN_TWO_OCTAVES = 4.0f;
    const float DOWN_FOUR_OCTAVES = 16.0f;
    float t;
    if (_collisionSoundMagnitude > COLLISION_SOUND_CUTOFF_LEVEL) {
        for (int i = 0; i < numSamples; i++) {
            t = (float) _proceduralEffectSample + (float) i;

            sample = sinf(t * _collisionSoundFrequency)
                + sinf(t * _collisionSoundFrequency / DOWN_TWO_OCTAVES)
                + sinf(t * _collisionSoundFrequency / DOWN_FOUR_OCTAVES * UP_MAJOR_FIFTH);
            sample *= _collisionSoundMagnitude * COLLISION_SOUND_MAX_VOLUME;

            int16_t collisionSample = (int16_t) sample;

            _lastInputLoudness = 0;
            
            monoInput[i] = glm::clamp(monoInput[i] + collisionSample, MIN_SAMPLE_VALUE, MAX_SAMPLE_VALUE);
            
            _lastInputLoudness += fabsf(monoInput[i]);
            _lastInputLoudness /= numSamples;
            _lastInputLoudness /= MAX_SAMPLE_VALUE;
            
            _localProceduralSamples[i] = glm::clamp(_localProceduralSamples[i] + collisionSample,
                                                  MIN_SAMPLE_VALUE, MAX_SAMPLE_VALUE);

            _collisionSoundMagnitude *= _collisionSoundDuration;
        }
    }
    _proceduralEffectSample += numSamples;

    //  Add a drum sound
    const float MAX_VOLUME = 32000.0f;
    const float MAX_DURATION = 2.0f;
    const float MIN_AUDIBLE_VOLUME = 0.001f;
    const float NOISE_MAGNITUDE = 0.02f;
    float frequency = (_drumSoundFrequency / SAMPLE_RATE) * TWO_PI;
    if (_drumSoundVolume > 0.0f) {
        for (int i = 0; i < numSamples; i++) {
            t = (float) _drumSoundSample + (float) i;
            sample = sinf(t * frequency);
            sample += ((randFloat() - 0.5f) * NOISE_MAGNITUDE);
            sample *= _drumSoundVolume * MAX_VOLUME;

            int16_t collisionSample = (int16_t) sample;

            _lastInputLoudness = 0;
            
            monoInput[i] = glm::clamp(monoInput[i] + collisionSample, MIN_SAMPLE_VALUE, MAX_SAMPLE_VALUE);
            
            _lastInputLoudness += fabsf(monoInput[i]);
            _lastInputLoudness /= numSamples;
            _lastInputLoudness /= MAX_SAMPLE_VALUE;
            
            _localProceduralSamples[i] = glm::clamp(_localProceduralSamples[i] + collisionSample,
                                                  MIN_SAMPLE_VALUE, MAX_SAMPLE_VALUE);

            _drumSoundVolume *= (1.0f - _drumSoundDecay);
        }
        _drumSoundSample += numSamples;
        _drumSoundDuration = glm::clamp(_drumSoundDuration - (AUDIO_CALLBACK_MSECS / 1000.0f), 0.0f, MAX_DURATION);
        if (_drumSoundDuration == 0.0f || (_drumSoundVolume < MIN_AUDIBLE_VOLUME)) {
            _drumSoundVolume = 0.0f;
        }
    }
}

//  Starts a collision sound.  magnitude is 0-1, with 1 the loudest possible sound.
void Audio::startCollisionSound(float magnitude, float frequency, float noise, float duration, bool flashScreen) {
    _collisionSoundMagnitude = magnitude;
    _collisionSoundFrequency = frequency;
    _collisionSoundNoise = noise;
    _collisionSoundDuration = duration;
    _collisionFlashesScreen = flashScreen;
}

void Audio::startDrumSound(float volume, float frequency, float duration, float decay) {
    _drumSoundVolume = volume;
    _drumSoundFrequency = frequency;
    _drumSoundDuration = duration;
    _drumSoundDecay = decay;
    _drumSoundSample = 0;
}

void Audio::handleAudioByteArray(const QByteArray& audioByteArray) {
    // TODO: either create a new audio device (up to the limit of the sound card or a hard limit)
    // or send to the mixer and use delayed loopback
}

void Audio::renderToolBox(int x, int y, bool boxed) {

    glEnable(GL_TEXTURE_2D);

    if (boxed) {

        bool isClipping = ((getTimeSinceLastClip() > 0.0f) && (getTimeSinceLastClip() < 1.0f));
        const int BOX_LEFT_PADDING = 5;
        const int BOX_TOP_PADDING = 10;
        const int BOX_WIDTH = 266;
        const int BOX_HEIGHT = 44;

        QRect boxBounds = QRect(x - BOX_LEFT_PADDING, y - BOX_TOP_PADDING, BOX_WIDTH, BOX_HEIGHT);

        glBindTexture(GL_TEXTURE_2D, _boxTextureId);

        if (isClipping) {
            glColor3f(1.0f, 0.0f, 0.0f);
        } else {
            glColor3f(0.41f, 0.41f, 0.41f);
        }
        glBegin(GL_QUADS);

        glTexCoord2f(1, 1);
        glVertex2f(boxBounds.left(), boxBounds.top());

        glTexCoord2f(0, 1);
        glVertex2f(boxBounds.right(), boxBounds.top());

        glTexCoord2f(0, 0);
        glVertex2f(boxBounds.right(), boxBounds.bottom());
        
        glTexCoord2f(1, 0);
        glVertex2f(boxBounds.left(), boxBounds.bottom());
        
        glEnd();
    }

    _iconBounds = QRect(x, y, MUTE_ICON_SIZE, MUTE_ICON_SIZE);
    if (!_muted) {
        glBindTexture(GL_TEXTURE_2D, _micTextureId);
    } else {
        glBindTexture(GL_TEXTURE_2D, _muteTextureId);
    }

    glColor3f(1,1,1);
    glBegin(GL_QUADS);

    glTexCoord2f(1, 1);
    glVertex2f(_iconBounds.left(), _iconBounds.top());

    glTexCoord2f(0, 1);
    glVertex2f(_iconBounds.right(), _iconBounds.top());

    glTexCoord2f(0, 0);
    glVertex2f(_iconBounds.right(), _iconBounds.bottom());

    glTexCoord2f(1, 0);
    glVertex2f(_iconBounds.left(), _iconBounds.bottom());

    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void Audio::toggleScope() {
    _scopeEnabled = !_scopeEnabled;
    if (_scopeEnabled) {
        _scopeInputOffset = 0;
        _scopeOutputOffset = 0;
        allocateScope();
    } else {
        freeScope();
    }
}

void Audio::toggleScopePause() {
    _scopeEnabledPause = !_scopeEnabledPause;
}

void Audio::toggleStats() {
    _statsEnabled = !_statsEnabled;
}

void Audio::selectAudioScopeFiveFrames() {
    if (Menu::getInstance()->isOptionChecked(MenuOption::AudioScopeFiveFrames)) {
        reallocateScope(5);
    }
}

void Audio::selectAudioScopeTwentyFrames() {
    if (Menu::getInstance()->isOptionChecked(MenuOption::AudioScopeTwentyFrames)) {
        reallocateScope(20);
    }
}

void Audio::selectAudioScopeFiftyFrames() {
    if (Menu::getInstance()->isOptionChecked(MenuOption::AudioScopeFiftyFrames)) {
        reallocateScope(50);
    }
}

void Audio::allocateScope() {
    int num = _samplesPerScope * sizeof(int16_t);
    _scopeInput = new QByteArray(num, 0);
    _scopeOutputLeft = new QByteArray(num, 0);
    _scopeOutputRight = new QByteArray(num, 0);
}

void Audio::reallocateScope(int frames) {
    if (_framesPerScope != frames) {
        _framesPerScope = frames;
        _samplesPerScope = NETWORK_SAMPLES_PER_FRAME * _framesPerScope;
        QMutexLocker lock(&_guard);
        freeScope();
        allocateScope();
    }
}

void Audio::freeScope() {
    if (_scopeInput) {
        delete _scopeInput;
        _scopeInput = 0;
    }
    if (_scopeOutputLeft) {
        delete _scopeOutputLeft;
        _scopeOutputLeft = 0;
    }
    if (_scopeOutputRight) {
        delete _scopeOutputRight;
        _scopeOutputRight = 0;
    }
}

void Audio::addBufferToScope(
    QByteArray* byteArray, unsigned int frameOffset, const int16_t* source, unsigned int sourceChannel, unsigned int sourceNumberOfChannels) {

    // Constant multiplier to map sample value to vertical size of scope
    float multiplier = (float)MULTIPLIER_SCOPE_HEIGHT / logf(2.0f);

    // Temporary variable receives sample value
    float sample;

    // Temporary variable receives mapping of sample value
    int16_t value;

    QMutexLocker lock(&_guard);
    // Short int pointer to mapped samples in byte array
    int16_t* destination = (int16_t*) byteArray->data();

    for (unsigned int i = 0; i < NETWORK_SAMPLES_PER_FRAME; i++) {
        sample = (float)source[i * sourceNumberOfChannels + sourceChannel];
        if (sample > 0) {
            value = (int16_t)(multiplier * logf(sample));
        } else if (sample < 0) {
            value = (int16_t)(-multiplier * logf(-sample));
        } else {
            value = 0;
        }
        destination[i + frameOffset] = value;
    }
}

void Audio::renderStats(const float* color, int width, int height) {
    if (!_statsEnabled) {
        return;
    }

    const int LINES_WHEN_CENTERED = 30;
    const int CENTERED_BACKGROUND_HEIGHT = STATS_HEIGHT_PER_LINE * LINES_WHEN_CENTERED;

    int lines = _audioMixerInjectedStreamAudioStatsMap.size() * 7 + 23;
    int statsHeight = STATS_HEIGHT_PER_LINE * lines;


    static const float backgroundColor[4] = { 0.2f, 0.2f, 0.2f, 0.6f };

    int x = std::max((width - (int)STATS_WIDTH) / 2, 0);
    int y = std::max((height - CENTERED_BACKGROUND_HEIGHT) / 2, 0);
    int w = STATS_WIDTH;
    int h = statsHeight;
    renderBackground(backgroundColor, x, y, w, h);


    int horizontalOffset = x + 5;
    int verticalOffset = y;
    
    float scale = 0.10f;
    float rotation = 0.0f;
    int font = 2;


    char latencyStatString[512];

    const float BUFFER_SEND_INTERVAL_MSECS = BUFFER_SEND_INTERVAL_USECS / (float)USECS_PER_MSEC;

    float audioInputBufferLatency = 0.0f, inputRingBufferLatency = 0.0f, networkRoundtripLatency = 0.0f, mixerRingBufferLatency = 0.0f, outputRingBufferLatency = 0.0f, audioOutputBufferLatency = 0.0f;

    AudioStreamStats downstreamAudioStreamStats = _receivedAudioStream.getAudioStreamStats();
    SharedNodePointer audioMixerNodePointer = NodeList::getInstance()->soloNodeOfType(NodeType::AudioMixer);
    if (!audioMixerNodePointer.isNull()) {
        audioInputBufferLatency = _audioInputMsecsReadStats.getWindowAverage();
        inputRingBufferLatency = getInputRingBufferAverageMsecsAvailable();
        networkRoundtripLatency = audioMixerNodePointer->getPingMs();
        mixerRingBufferLatency = _audioMixerAvatarStreamAudioStats._framesAvailableAverage * BUFFER_SEND_INTERVAL_MSECS;
        outputRingBufferLatency = downstreamAudioStreamStats._framesAvailableAverage * BUFFER_SEND_INTERVAL_MSECS;
        audioOutputBufferLatency = _audioOutputMsecsUnplayedStats.getWindowAverage();
    }
    float totalLatency = audioInputBufferLatency + inputRingBufferLatency + networkRoundtripLatency + mixerRingBufferLatency + outputRingBufferLatency + audioOutputBufferLatency;

    sprintf(latencyStatString, "     Audio input buffer: %7.2fms    - avg msecs of samples read to the input ring buffer in last 10s", audioInputBufferLatency);
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, latencyStatString, color);

    sprintf(latencyStatString, "      Input ring buffer: %7.2fms    - avg msecs of samples in input ring buffer in last 10s", inputRingBufferLatency);
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, latencyStatString, color);

    sprintf(latencyStatString, "       Network to mixer: %7.2fms    - half of last ping value calculated by the node list", networkRoundtripLatency / 2.0f);
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, latencyStatString, color);

    sprintf(latencyStatString, " AudioMixer ring buffer: %7.2fms    - avg msecs of samples in audio mixer's ring buffer in last 10s", mixerRingBufferLatency);
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, latencyStatString, color);

    sprintf(latencyStatString, "      Network to client: %7.2fms    - half of last ping value calculated by the node list", networkRoundtripLatency / 2.0f);
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, latencyStatString, color);
    
    sprintf(latencyStatString, "     Output ring buffer: %7.2fms    - avg msecs of samples in output ring buffer in last 10s", outputRingBufferLatency);
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, latencyStatString, color);

    sprintf(latencyStatString, "    Audio output buffer: %7.2fms    - avg msecs of samples in audio output buffer in last 10s", audioOutputBufferLatency);
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, latencyStatString, color);
    
    sprintf(latencyStatString, "                  TOTAL: %7.2fms\n", totalLatency);
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, latencyStatString, color);


    verticalOffset += STATS_HEIGHT_PER_LINE;    // blank line


    char downstreamLabelString[] = "Downstream mixed audio stats:";
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, downstreamLabelString, color);

    renderAudioStreamStats(downstreamAudioStreamStats, horizontalOffset, verticalOffset, scale, rotation, font, color, true);


    verticalOffset += STATS_HEIGHT_PER_LINE;    // blank line

    char upstreamMicLabelString[] = "Upstream mic audio stats:";
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, upstreamMicLabelString, color);

    renderAudioStreamStats(_audioMixerAvatarStreamAudioStats, horizontalOffset, verticalOffset, scale, rotation, font, color);


    foreach(const AudioStreamStats& injectedStreamAudioStats, _audioMixerInjectedStreamAudioStatsMap) {

        verticalOffset += STATS_HEIGHT_PER_LINE;    // blank line

        char upstreamInjectedLabelString[512];
        sprintf(upstreamInjectedLabelString, "Upstream injected audio stats:      stream ID: %s",
            injectedStreamAudioStats._streamIdentifier.toString().toLatin1().data());
        verticalOffset += STATS_HEIGHT_PER_LINE;
        drawText(horizontalOffset, verticalOffset, scale, rotation, font, upstreamInjectedLabelString, color);

        renderAudioStreamStats(injectedStreamAudioStats, horizontalOffset, verticalOffset, scale, rotation, font, color);
    }
}

void Audio::renderAudioStreamStats(const AudioStreamStats& streamStats, int horizontalOffset, int& verticalOffset,
    float scale, float rotation, int font, const float* color, bool isDownstreamStats) {
    
    char stringBuffer[512];

    sprintf(stringBuffer, "                      Packet loss | overall: %5.2f%% (%d lost), last_30s: %5.2f%% (%d lost)",
        streamStats._packetStreamStats.getLostRate() * 100.0f,
        streamStats._packetStreamStats._numLost,
        streamStats._packetStreamWindowStats.getLostRate() * 100.0f,
        streamStats._packetStreamWindowStats._numLost);
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, stringBuffer, color);

    if (isDownstreamStats) {

        const float BUFFER_SEND_INTERVAL_MSECS = BUFFER_SEND_INTERVAL_USECS / (float)USECS_PER_MSEC;
        sprintf(stringBuffer, "                Ringbuffer frames | desired: %u, avg_available(10s): %u+%d, available: %u+%d",
            streamStats._desiredJitterBufferFrames,
            streamStats._framesAvailableAverage,
            (int)(getAudioOutputAverageMsecsUnplayed() / BUFFER_SEND_INTERVAL_MSECS),
            streamStats._framesAvailable,
            (int)(getAudioOutputMsecsUnplayed() / BUFFER_SEND_INTERVAL_MSECS));
    } else {
        sprintf(stringBuffer, "                Ringbuffer frames | desired: %u, avg_available(10s): %u, available: %u",
            streamStats._desiredJitterBufferFrames,
            streamStats._framesAvailableAverage,
            streamStats._framesAvailable);
    }
    
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, stringBuffer, color);

    sprintf(stringBuffer, "                 Ringbuffer stats | starves: %u, prev_starve_lasted: %u, frames_dropped: %u, overflows: %u",
        streamStats._starveCount,
        streamStats._consecutiveNotMixedCount,
        streamStats._framesDropped,
        streamStats._overflowCount);
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, stringBuffer, color);

    sprintf(stringBuffer, "  Inter-packet timegaps (overall) | min: %9s, max: %9s, avg: %9s",
        formatUsecTime(streamStats._timeGapMin).toLatin1().data(),
        formatUsecTime(streamStats._timeGapMax).toLatin1().data(),
        formatUsecTime(streamStats._timeGapAverage).toLatin1().data());
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, stringBuffer, color);

    sprintf(stringBuffer, " Inter-packet timegaps (last 30s) | min: %9s, max: %9s, avg: %9s",
        formatUsecTime(streamStats._timeGapWindowMin).toLatin1().data(),
        formatUsecTime(streamStats._timeGapWindowMax).toLatin1().data(),
        formatUsecTime(streamStats._timeGapWindowAverage).toLatin1().data());
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, stringBuffer, color);
}


void Audio::renderScope(int width, int height) {

    if (!_scopeEnabled)
        return;

    static const float backgroundColor[4] = { 0.2f, 0.2f, 0.2f, 0.6f };
    static const float gridColor[4] = { 0.3f, 0.3f, 0.3f, 0.6f };
    static const float inputColor[4] = { 0.3f, .7f, 0.3f, 0.6f };
    static const float outputLeftColor[4] = { 0.7f, .3f, 0.3f, 0.6f };
    static const float outputRightColor[4] = { 0.3f, .3f, 0.7f, 0.6f };
    static const int gridRows = 2;
    int gridCols = _framesPerScope;

    int x = (width - SCOPE_WIDTH) / 2;
    int y = (height - SCOPE_HEIGHT) / 2;
    int w = SCOPE_WIDTH;
    int h = SCOPE_HEIGHT;

    renderBackground(backgroundColor, x, y, w, h);
    renderGrid(gridColor, x, y, w, h, gridRows, gridCols);

    QMutexLocker lock(&_guard);
    renderLineStrip(inputColor, x, y, _samplesPerScope, _scopeInputOffset, _scopeInput);
    renderLineStrip(outputLeftColor, x, y, _samplesPerScope, _scopeOutputOffset, _scopeOutputLeft);
    renderLineStrip(outputRightColor, x, y, _samplesPerScope, _scopeOutputOffset, _scopeOutputRight);
}

void Audio::renderBackground(const float* color, int x, int y, int width, int height) {

    glColor4fv(color);
    glBegin(GL_QUADS);

    glVertex2i(x, y);
    glVertex2i(x + width, y);
    glVertex2i(x + width, y + height);
    glVertex2i(x , y + height);

    glEnd();
    glColor4f(1, 1, 1, 1); 
}

void Audio::renderGrid(const float* color, int x, int y, int width, int height, int rows, int cols) {

    glColor4fv(color);
    glBegin(GL_LINES);

    int dx = width / cols;
    int dy = height / rows;
    int tx = x;
    int ty = y;

    // Draw horizontal grid lines
    for (int i = rows + 1; --i >= 0; ) {
        glVertex2i(x, ty);
        glVertex2i(x + width, ty);
        ty += dy;
    }
    // Draw vertical grid lines
    for (int i = cols + 1; --i >= 0; ) {
        glVertex2i(tx, y);
        glVertex2i(tx, y + height);
        tx += dx;
    }
    glEnd();
    glColor4f(1, 1, 1, 1); 
}

void Audio::renderLineStrip(const float* color, int x, int y, int n, int offset, const QByteArray* byteArray) {

    glColor4fv(color);
    glBegin(GL_LINE_STRIP);

    int16_t sample;
    int16_t* samples = ((int16_t*) byteArray->data()) + offset;
    int numSamplesToAverage = _framesPerScope / DEFAULT_FRAMES_PER_SCOPE;
    int count = (n - offset) / numSamplesToAverage;
    int remainder = (n - offset) % numSamplesToAverage;
    y += SCOPE_HEIGHT / 2;

    // Compute and draw the sample averages from the offset position
    for (int i = count; --i >= 0; ) {
        sample = 0;
        for (int j = numSamplesToAverage; --j >= 0; ) {
            sample += *samples++;
        }
        sample /= numSamplesToAverage;
        glVertex2i(x++, y - sample);
    }

    // Compute and draw the sample average across the wrap boundary
    if (remainder != 0) {
        sample = 0;
        for (int j = remainder; --j >= 0; ) {
            sample += *samples++;
        }
    
        samples = (int16_t*) byteArray->data();

        for (int j = numSamplesToAverage - remainder; --j >= 0; ) {
            sample += *samples++;
        }
        sample /= numSamplesToAverage;
        glVertex2i(x++, y - sample);
    } else {
        samples = (int16_t*) byteArray->data();
    }

    // Compute and draw the sample average from the beginning to the offset
    count = (offset - remainder) / numSamplesToAverage;
    for (int i = count; --i >= 0; ) {
        sample = 0;
        for (int j = numSamplesToAverage; --j >= 0; ) {
            sample += *samples++;
        }
        sample /= numSamplesToAverage;
        glVertex2i(x++, y - sample);
    }
    glEnd();
    glColor4f(1, 1, 1, 1); 
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

bool Audio::switchOutputToAudioDevice(const QAudioDeviceInfo& outputDeviceInfo) {
    bool supportedFormat = false;

    // cleanup any previously initialized device
    if (_audioOutput) {
        _audioOutput->stop();
        _outputDevice = NULL;
        
        delete _audioOutput;
        _audioOutput = NULL;

        _loopbackOutputDevice = NULL;
        delete _loopbackAudioOutput;
        _loopbackAudioOutput = NULL;

        _proceduralOutputDevice = NULL;
        delete _proceduralAudioOutput;
        _proceduralAudioOutput = NULL;
        _outputAudioDeviceName = "";
    }

    if (!outputDeviceInfo.isNull()) {
        qDebug() << "The audio output device " << outputDeviceInfo.deviceName() << "is available.";
        _outputAudioDeviceName = outputDeviceInfo.deviceName().trimmed();

        if (adjustedFormatForAudioDevice(outputDeviceInfo, _desiredOutputFormat, _outputFormat)) {
            qDebug() << "The format to be used for audio output is" << _outputFormat;
            
            const int AUDIO_OUTPUT_BUFFER_SIZE_FRAMES = 10;

            // setup our general output device for audio-mixer audio
            _audioOutput = new QAudioOutput(outputDeviceInfo, _outputFormat, this);
            _audioOutput->setBufferSize(AUDIO_OUTPUT_BUFFER_SIZE_FRAMES * _outputFormat.bytesForDuration(BUFFER_SEND_INTERVAL_USECS));
            qDebug() << "Ring Buffer capacity in frames: " << AUDIO_OUTPUT_BUFFER_SIZE_FRAMES;
            _outputDevice = _audioOutput->start();

            // setup a loopback audio output device
            _loopbackAudioOutput = new QAudioOutput(outputDeviceInfo, _outputFormat, this);
        
            // setup a procedural audio output device
            _proceduralAudioOutput = new QAudioOutput(outputDeviceInfo, _outputFormat, this);

            _timeSinceLastReceived.start();

            // setup spatial audio ringbuffer
            int numFrameSamples = _outputFormat.sampleRate() * _desiredOutputFormat.channelCount();
            _spatialAudioRingBuffer.resizeForFrameSize(numFrameSamples);
            _spatialAudioStart = _spatialAudioFinish = 0;
            
            supportedFormat = true;
        }
    }
    return supportedFormat;
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
    int numInputCallbackBytes = (int)(((NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL 
        * format.channelCount()
        * (format.sampleRate() / SAMPLE_RATE))
        / CALLBACK_ACCELERATOR_RATIO) + 0.5f);

    return numInputCallbackBytes;
}

float Audio::calculateDeviceToNetworkInputRatio(int numBytes) const {
    float inputToNetworkInputRatio = (int)((_numInputCallbackBytes 
        * CALLBACK_ACCELERATOR_RATIO
        / NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL) + 0.5f);

    return inputToNetworkInputRatio;
}

int Audio::calculateNumberOfFrameSamples(int numBytes) const {
    int frameSamples = (int)(numBytes * CALLBACK_ACCELERATOR_RATIO + 0.5f) / sizeof(int16_t);
    return frameSamples;
}

float Audio::getAudioOutputMsecsUnplayed() const {
    int bytesAudioOutputUnplayed = _audioOutput->bufferSize() - _audioOutput->bytesFree();
    float msecsAudioOutputUnplayed = bytesAudioOutputUnplayed / (float)_outputFormat.bytesForDuration(USECS_PER_MSEC);
    return msecsAudioOutputUnplayed;
}

float Audio::getInputRingBufferMsecsAvailable() const {
    int bytesInInputRingBuffer = _inputRingBuffer.samplesAvailable() * sizeof(int16_t);
    float msecsInInputRingBuffer = bytesInInputRingBuffer / (float)(_inputFormat.bytesForDuration(USECS_PER_MSEC));
    return  msecsInInputRingBuffer;
}
