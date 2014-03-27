//
//  Audio.cpp
//  interface
//
//  Created by Stephen Birarda on 1/22/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <cstring>
#include <sys/stat.h>

#include <math.h>

#ifdef __APPLE__
#include <CoreAudio/AudioHardware.h>
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

#include "Application.h"
#include "Audio.h"
#include "Menu.h"
#include "Util.h"

static const float AUDIO_CALLBACK_MSECS = (float) NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL / (float)SAMPLE_RATE * 1000.0;

static const int NUMBER_OF_NOISE_SAMPLE_FRAMES = 300;

// Mute icon configration
static const int MUTE_ICON_SIZE = 24;

Audio::Audio(Oscilloscope* scope, int16_t initialJitterBufferSamples, QObject* parent) :
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
    _inputRingBuffer(0),
    _ringBuffer(NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL),
    _scope(scope),
    _averagedLatency(0.0),
    _measuredJitter(0),
    _jitterBufferSamples(initialJitterBufferSamples),
    _lastInputLoudness(0),
    _timeSinceLastClip(-1.0),
    _dcOffset(0),
    _noiseGateMeasuredFloor(0),
    _noiseGateSampleCounter(0),
    _noiseGateOpen(false),
    _noiseGateEnabled(true),
    _noiseGateFramesToClose(0),
    _lastVelocity(0),
    _lastAcceleration(0),
    _totalPacketsReceived(0),
    _collisionSoundMagnitude(0.0f),
    _collisionSoundFrequency(0.0f),
    _collisionSoundNoise(0.0f),
    _collisionSoundDuration(0.0f),
    _proceduralEffectSample(0),
    _numFramesDisplayStarve(0),
    _muted(false)
{
    // clear the array of locally injected samples
    memset(_localProceduralSamples, 0, NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL);
    // Create the noise sample array
    _noiseSampleFrames = new float[NUMBER_OF_NOISE_SAMPLE_FRAMES];
}

void Audio::init(QGLWidget *parent) {
    _micTextureId = parent->bindTexture(QImage(Application::resourcesPath() + "images/mic.svg"));
    _muteTextureId = parent->bindTexture(QImage(Application::resourcesPath() + "images/mute.svg"));
}

void Audio::reset() {
    _ringBuffer.reset();
}

QAudioDeviceInfo getNamedAudioDeviceForMode(QAudio::Mode mode, const QString& deviceName) {
    QAudioDeviceInfo result;
    foreach(QAudioDeviceInfo audioDevice, QAudioDeviceInfo::availableDevices(mode)) {
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

const int CALLBACK_ACCELERATOR_RATIO = 2;

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
    
    if (!inputFormatSupported || !outputFormatSupported) {
        qDebug() << "Unable to set up audio I/O because of a problem with input or output formats.";
    }
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
    return switchInputToAudioDevice(getNamedAudioDeviceForMode(QAudio::AudioInput, inputDeviceName));
}

bool Audio::switchOutputToAudioDevice(const QString& outputDeviceName) {
    return switchOutputToAudioDevice(getNamedAudioDeviceForMode(QAudio::AudioOutput, outputDeviceName));
}

void Audio::handleAudioInput() {
    static char monoAudioDataPacket[MAX_PACKET_SIZE];

    static int numBytesPacketHeader = numBytesForPacketHeaderGivenPacketType(PacketTypeMicrophoneAudioNoEcho);
    static int leadingBytes = numBytesPacketHeader + sizeof(glm::vec3) + sizeof(glm::quat);

    static int16_t* monoAudioSamples = (int16_t*) (monoAudioDataPacket + leadingBytes);

    static float inputToNetworkInputRatio = _numInputCallbackBytes * CALLBACK_ACCELERATOR_RATIO
        / NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL;

    static unsigned int inputSamplesRequired = NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL * inputToNetworkInputRatio;

    QByteArray inputByteArray = _inputDevice->readAll();

    if (Menu::getInstance()->isOptionChecked(MenuOption::EchoLocalAudio) && !_muted) {
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
            static float loopbackOutputToInputRatio = (_outputFormat.sampleRate() / (float) _inputFormat.sampleRate())
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

    while (_inputRingBuffer.samplesAvailable() > inputSamplesRequired) {

        int16_t* inputAudioSamples = new int16_t[inputSamplesRequired];
        _inputRingBuffer.readSamples(inputAudioSamples, inputSamplesRequired);

        // zero out the monoAudioSamples array and the locally injected audio
        memset(monoAudioSamples, 0, NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL);

        // zero out the locally injected audio in preparation for audio procedural sounds
        memset(_localProceduralSamples, 0, NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL);

        if (!_muted) {
            // we aren't muted, downsample the input audio
            linearResampling((int16_t*) inputAudioSamples,
                             monoAudioSamples,
                             inputSamplesRequired,
                             NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL,
                             _inputFormat, _desiredInputFormat);

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
            
            const float NOISE_GATE_HEIGHT = 7.f;
            const int NOISE_GATE_WIDTH = 5;
            const int NOISE_GATE_CLOSE_FRAME_DELAY = 5;
            const int NOISE_GATE_FRAMES_TO_AVERAGE = 5;
            const float DC_OFFSET_AVERAGING = 0.99f;
            const float CLIPPING_THRESHOLD = 0.90f;
            
            float measuredDcOffset = 0.f;
            
            //  Increment the time since the last clip
            if (_timeSinceLastClip >= 0.0f) {
                _timeSinceLastClip += (float) NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL / (float) SAMPLE_RATE;
            }
            
            for (int i = 0; i < NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL; i++) {
                measuredDcOffset += monoAudioSamples[i];
                monoAudioSamples[i] -= (int16_t) _dcOffset;
                thisSample = fabsf(monoAudioSamples[i]);
                if (thisSample > (32767.f * CLIPPING_THRESHOLD)) {
                    _timeSinceLastClip = 0.0f;
                }
                loudness += thisSample;
                //  Noise Reduction:  Count peaks above the average loudness
                if (thisSample > (_noiseGateMeasuredFloor * NOISE_GATE_HEIGHT)) {
                    samplesOverNoiseGate++;
                }
            }
            
            measuredDcOffset /= NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL;
            if (_dcOffset == 0.f) {
                // On first frame, copy over measured offset
                _dcOffset = measuredDcOffset;
            } else {
                _dcOffset = DC_OFFSET_AVERAGING * _dcOffset + (1.f - DC_OFFSET_AVERAGING) * measuredDcOffset;
            }
            
            //
            _lastInputLoudness = fabs(loudness / NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL);
            
            float averageOfAllSampleFrames = 0.f;
            _noiseSampleFrames[_noiseGateSampleCounter++] = _lastInputLoudness;
            if (_noiseGateSampleCounter == NUMBER_OF_NOISE_SAMPLE_FRAMES) {
                float smallestSample = FLT_MAX;
                for (int i = 0; i <= NUMBER_OF_NOISE_SAMPLE_FRAMES - NOISE_GATE_FRAMES_TO_AVERAGE; i+= NOISE_GATE_FRAMES_TO_AVERAGE) {
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

            if (_noiseGateEnabled) {
                if (samplesOverNoiseGate > NOISE_GATE_WIDTH) {
                    _noiseGateOpen = true;
                    _noiseGateFramesToClose = NOISE_GATE_CLOSE_FRAME_DELAY;
                } else {
                    if (--_noiseGateFramesToClose == 0) {
                        _noiseGateOpen = false;
                    }
                }
                if (!_noiseGateOpen) {
                    memset(monoAudioSamples, 0, NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL);
                    _lastInputLoudness = 0;
                }
            }

            // add input data just written to the scope
            QMetaObject::invokeMethod(_scope, "addSamples", Qt::QueuedConnection,
                                      Q_ARG(QByteArray, QByteArray((char*) monoAudioSamples,
                                                                   NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL)),
                                      Q_ARG(bool, false), Q_ARG(bool, true));
        } else {
            // our input loudness is 0, since we're muted
            _lastInputLoudness = 0;
        }
        
        // add procedural effects to the appropriate input samples
        addProceduralSounds(monoAudioSamples,
                            NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL);
        
        if (!_proceduralOutputDevice && _proceduralAudioOutput) {
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

        NodeList* nodeList = NodeList::getInstance();
        SharedNodePointer audioMixer = nodeList->soloNodeOfType(NodeType::AudioMixer);
        
        if (audioMixer && audioMixer->getActiveSocket()) {
            MyAvatar* interfaceAvatar = Application::getInstance()->getAvatar();
            glm::vec3 headPosition = interfaceAvatar->getHead()->getPosition();
            glm::quat headOrientation = interfaceAvatar->getHead()->getOrientation();

            // we need the amount of bytes in the buffer + 1 for type
            // + 12 for 3 floats for position + float for bearing + 1 attenuation byte
            
            int numAudioBytes = 0;
            
            PacketType packetType;
            if (_lastInputLoudness == 0) {
                packetType = PacketTypeSilentAudioFrame;
                
                // we need to indicate how many silent samples this is to the audio mixer
                monoAudioSamples[0] = NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL;
                numAudioBytes = sizeof(int16_t);
                
            } else {
                numAudioBytes = NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL;
                
                if (Menu::getInstance()->isOptionChecked(MenuOption::EchoServerAudio)) {
                    packetType = PacketTypeMicrophoneAudioWithEcho;
                } else {
                    packetType = PacketTypeMicrophoneAudioNoEcho;
                }
            }

            char* currentPacketPtr = monoAudioDataPacket + populatePacketHeader(monoAudioDataPacket, packetType);

            // memcpy the three float positions
            memcpy(currentPacketPtr, &headPosition, sizeof(headPosition));
            currentPacketPtr += (sizeof(headPosition));

            // memcpy our orientation
            memcpy(currentPacketPtr, &headOrientation, sizeof(headOrientation));
            currentPacketPtr += sizeof(headOrientation);
            
            nodeList->writeDatagram(monoAudioDataPacket, numAudioBytes + leadingBytes, audioMixer);

            Application::getInstance()->getBandwidthMeter()->outputStream(BandwidthMeter::AUDIO)
                .updateValue(numAudioBytes + leadingBytes);
        }
        delete[] inputAudioSamples;
    }
}

void Audio::addReceivedAudioToBuffer(const QByteArray& audioByteArray) {
    const int NUM_INITIAL_PACKETS_DISCARD = 3;
    const int STANDARD_DEVIATION_SAMPLE_COUNT = 500;

    timeval currentReceiveTime;
    gettimeofday(&currentReceiveTime, NULL);
    _totalPacketsReceived++;

    double timeDiff = diffclock(&_lastReceiveTime, &currentReceiveTime);
    
    //  Discard first few received packets for computing jitter (often they pile up on start)
    if (_totalPacketsReceived > NUM_INITIAL_PACKETS_DISCARD) {
        _stdev.addValue(timeDiff);
    }

    if (_stdev.getSamples() > STANDARD_DEVIATION_SAMPLE_COUNT) {
        _measuredJitter = _stdev.getStDev();
        _stdev.reset();
        //  Set jitter buffer to be a multiple of the measured standard deviation
        const int MAX_JITTER_BUFFER_SAMPLES = _ringBuffer.getSampleCapacity() / 2;
        const float NUM_STANDARD_DEVIATIONS = 3.f;
        if (Menu::getInstance()->getAudioJitterBufferSamples() == 0) {
            float newJitterBufferSamples = (NUM_STANDARD_DEVIATIONS * _measuredJitter) / 1000.f * SAMPLE_RATE;
            setJitterBufferSamples(glm::clamp((int)newJitterBufferSamples, 0, MAX_JITTER_BUFFER_SAMPLES));
        }
    }

    _ringBuffer.parseData(audioByteArray);

    static float networkOutputToOutputRatio = (_desiredOutputFormat.sampleRate() / (float) _outputFormat.sampleRate())
        * (_desiredOutputFormat.channelCount() / (float) _outputFormat.channelCount());
    
    if (!_ringBuffer.isStarved() && _audioOutput && _audioOutput->bytesFree() == _audioOutput->bufferSize()) {
        // we don't have any audio data left in the output buffer
        // we just starved
        //qDebug() << "Audio output just starved.";
        _ringBuffer.setIsStarved(true);
        _numFramesDisplayStarve = 10;
    }
    
    // if there is anything in the ring buffer, decide what to do
    if (_ringBuffer.samplesAvailable() > 0) {
        
        int numNetworkOutputSamples = _ringBuffer.samplesAvailable();
        int numDeviceOutputSamples = numNetworkOutputSamples / networkOutputToOutputRatio;
        
        QByteArray outputBuffer;
        outputBuffer.resize(numDeviceOutputSamples * sizeof(int16_t));
        
        int numSamplesNeededToStartPlayback = NETWORK_BUFFER_LENGTH_SAMPLES_STEREO + (_jitterBufferSamples * 2);
        
        if (!_ringBuffer.isNotStarvedOrHasMinimumSamples(numSamplesNeededToStartPlayback)) {
            //  We are still waiting for enough samples to begin playback
            // qDebug() << numNetworkOutputSamples << " samples so far, waiting for " << numSamplesNeededToStartPlayback;
        } else {
            //  We are either already playing back, or we have enough audio to start playing back.
            //qDebug() << "pushing " << numNetworkOutputSamples;
            _ringBuffer.setIsStarved(false);

            // copy the samples we'll resample from the ring buffer - this also
            // pushes the read pointer of the ring buffer forwards
            int16_t* ringBufferSamples= new int16_t[numNetworkOutputSamples];
            _ringBuffer.readSamples(ringBufferSamples, numNetworkOutputSamples);
        
            // add the next numNetworkOutputSamples from each QByteArray
            // in our _localInjectionByteArrays QVector to the localInjectedSamples

            // copy the packet from the RB to the output
            linearResampling(ringBufferSamples,
                             (int16_t*) outputBuffer.data(),
                             numNetworkOutputSamples,
                             numDeviceOutputSamples,
                             _desiredOutputFormat, _outputFormat);

            if (_outputDevice) {
                _outputDevice->write(outputBuffer);

                // add output (@speakers) data just written to the scope
                QMetaObject::invokeMethod(_scope, "addSamples", Qt::QueuedConnection,
                                          Q_ARG(QByteArray, QByteArray((char*) ringBufferSamples, numNetworkOutputSamples)),
                                          Q_ARG(bool, true), Q_ARG(bool, false));
            }
            delete[] ringBufferSamples;
        }

    }

    Application::getInstance()->getBandwidthMeter()->inputStream(BandwidthMeter::AUDIO).updateValue(audioByteArray.size());

    _lastReceiveTime = currentReceiveTime;
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



//  Take a pointer to the acquired microphone input samples and add procedural sounds
void Audio::addProceduralSounds(int16_t* monoInput, int numSamples) {
    float sample;
    const float COLLISION_SOUND_CUTOFF_LEVEL = 0.01f;
    const float COLLISION_SOUND_MAX_VOLUME = 1000.f;
    const float UP_MAJOR_FIFTH = powf(1.5f, 4.0f);
    const float DOWN_TWO_OCTAVES = 4.f;
    const float DOWN_FOUR_OCTAVES = 16.f;
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
    const float MAX_VOLUME = 32000.f;
    const float MAX_DURATION = 2.f;
    const float MIN_AUDIBLE_VOLUME = 0.001f;
    const float NOISE_MAGNITUDE = 0.02f;
    float frequency = (_drumSoundFrequency / SAMPLE_RATE) * TWO_PI;
    if (_drumSoundVolume > 0.f) {
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

            _drumSoundVolume *= (1.f - _drumSoundDecay);
        }
        _drumSoundSample += numSamples;
        _drumSoundDuration = glm::clamp(_drumSoundDuration - (AUDIO_CALLBACK_MSECS / 1000.f), 0.f, MAX_DURATION);
        if (_drumSoundDuration == 0.f || (_drumSoundVolume < MIN_AUDIBLE_VOLUME)) {
            _drumSoundVolume = 0.f;
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

void Audio::renderMuteIcon(int x, int y) {

    _iconBounds = QRect(x, y, MUTE_ICON_SIZE, MUTE_ICON_SIZE);
    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, _micTextureId);
    glColor3f(.93f, .93f, .93f);
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

    if (_muted) {
        glBindTexture(GL_TEXTURE_2D, _muteTextureId);
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
    }

    glDisable(GL_TEXTURE_2D);
}

bool Audio::switchInputToAudioDevice(const QAudioDeviceInfo& inputDeviceInfo) {
    bool supportedFormat = false;
    
    // cleanup any previously initialized device
    if (_audioInput) {
        _audioInput->stop();
        disconnect(_inputDevice, 0, 0, 0);
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
        
            _audioInput = new QAudioInput(inputDeviceInfo, _inputFormat, this);
            _numInputCallbackBytes = NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL * _inputFormat.channelCount()
                * (_inputFormat.sampleRate() / SAMPLE_RATE)
                / CALLBACK_ACCELERATOR_RATIO;
            _audioInput->setBufferSize(_numInputCallbackBytes);

            // how do we want to handle input working, but output not working?
            _inputRingBuffer.resizeForFrameSize(_numInputCallbackBytes * CALLBACK_ACCELERATOR_RATIO / sizeof(int16_t));
            _inputDevice = _audioInput->start();
            connect(_inputDevice, SIGNAL(readyRead()), this, SLOT(handleAudioInput()));

            supportedFormat = true;
        }
    }
    return supportedFormat;
}

bool Audio::switchOutputToAudioDevice(const QAudioDeviceInfo& outputDeviceInfo) {
    bool supportedFormat = false;

    // cleanup any previously initialized device
    if (_audioOutput) {
        _audioOutput->stop();
        disconnect(_outputDevice, 0, 0, 0);
        _outputDevice = NULL;
        
        delete _audioOutput;
        _audioOutput = NULL;
        _numInputCallbackBytes = 0;

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
        
            // setup our general output device for audio-mixer audio
            _audioOutput = new QAudioOutput(outputDeviceInfo, _outputFormat, this);
            _audioOutput->setBufferSize(_ringBuffer.getSampleCapacity() * sizeof(int16_t));
            qDebug() << "Ring Buffer capacity in samples: " << _ringBuffer.getSampleCapacity();
            _outputDevice = _audioOutput->start();

            // setup a loopback audio output device
            _loopbackAudioOutput = new QAudioOutput(outputDeviceInfo, _outputFormat, this);
        
            // setup a procedural audio output device
            _proceduralAudioOutput = new QAudioOutput(outputDeviceInfo, _outputFormat, this);

            gettimeofday(&_lastReceiveTime, NULL);
            supportedFormat = true;
        }
    }
    return supportedFormat;
}
