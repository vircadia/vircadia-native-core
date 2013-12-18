//
//  Audio.cpp
//  interface
//
//  Created by Stephen Birarda on 1/22/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <cstring>
#include <sys/stat.h>

#ifdef __APPLE__
#include <CoreAudio/AudioHardware.h>
#endif

#include <QtCore/QBuffer>
#include <QtMultimedia/QAudioInput>
#include <QtMultimedia/QAudioOutput>
#include <QSvgRenderer>

#include <AngleUtil.h>
#include <NodeList.h>
#include <NodeTypes.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <StdDev.h>
#include <UUID.h>

#include "Application.h"
#include "Audio.h"
#include "Menu.h"
#include "Util.h"

static const float JITTER_BUFFER_LENGTH_MSECS = 12;
static const short JITTER_BUFFER_SAMPLES = JITTER_BUFFER_LENGTH_MSECS * NUM_AUDIO_CHANNELS * (SAMPLE_RATE / 1000.0);

static const float AUDIO_CALLBACK_MSECS = (float) NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL / (float)SAMPLE_RATE * 1000.0;

// Mute icon configration
static const int ICON_SIZE = 24;
static const int ICON_LEFT = 20;
static const int BOTTOM_PADDING = 110;

Audio::Audio(Oscilloscope* scope, int16_t initialJitterBufferSamples, QObject* parent) :
    QObject(parent),
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
    _inputRingBuffer(0),
    _ringBuffer(NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL),
    _scope(scope),
    _averagedLatency(0.0),
    _measuredJitter(0),
    _jitterBufferSamples(initialJitterBufferSamples),
    _lastInputLoudness(0),
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
    memset(_localInjectedSamples, 0, NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL);
}

void Audio::init(QGLWidget *parent) {
    switchToResourcesParentIfRequired();
    _micTextureId = parent->bindTexture(QImage("./resources/images/mic.svg"));
    _muteTextureId = parent->bindTexture(QImage("./resources/images/mute.svg"));
}

void Audio::reset() {
    _ringBuffer.reset();
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
        qDebug() << "The desired format for audio I/O is" << desiredAudioFormat << "\n";
        qDebug() << "The desired audio format is not supported by this device.\n";
        
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
            
            for (int i = sourceAudioFormat.channelCount(); i < numSourceSamples; i += 2 * sourceAudioFormat.channelCount()) {
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
            // upsample from 24 to 48
            // for now this only supports a stereo to stereo conversion - this is our case for network audio to output
            int sourceIndex = 0;
            int dtsSampleRateFactor = (destinationAudioFormat.sampleRate() / sourceAudioFormat.sampleRate());
            int sampleShift = destinationAudioFormat.channelCount() * dtsSampleRateFactor;
            int destinationToSourceFactor = (1 / sourceToDestinationFactor);
            
            for (int i = 0; i < numDestinationSamples; i += sampleShift) {
                sourceIndex = (i / destinationToSourceFactor);
                
                // fill the L/R channels and make the rest silent
                for (int j = i; j < i + sampleShift; j++) {
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
    
    qDebug() << "The audio input device is" << inputDeviceInfo.deviceName() << "\n";
    
    if (adjustedFormatForAudioDevice(inputDeviceInfo, _desiredInputFormat, _inputFormat)) {
        qDebug() << "The format to be used for audio input is" << _inputFormat << "\n";
        
        _audioInput = new QAudioInput(inputDeviceInfo, _inputFormat, this);
        _numInputCallbackBytes = NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL * _inputFormat.channelCount()
            * (_inputFormat.sampleRate() / SAMPLE_RATE)
            / CALLBACK_ACCELERATOR_RATIO;
        _audioInput->setBufferSize(_numInputCallbackBytes);
        
        QAudioDeviceInfo outputDeviceInfo = defaultAudioDeviceForMode(QAudio::AudioOutput);
        
        qDebug() << "The audio output device is" << outputDeviceInfo.deviceName() << "\n";

        if (adjustedFormatForAudioDevice(outputDeviceInfo, _desiredOutputFormat, _outputFormat)) {
            qDebug() << "The format to be used for audio output is" << _outputFormat << "\n";
            
            _inputRingBuffer.resizeForFrameSize(_numInputCallbackBytes * CALLBACK_ACCELERATOR_RATIO / sizeof(int16_t));
            _inputDevice = _audioInput->start();
            connect(_inputDevice, SIGNAL(readyRead()), this, SLOT(handleAudioInput()));
            
            // setup our general output device for audio-mixer audio
            _audioOutput = new QAudioOutput(outputDeviceInfo, _outputFormat, this);
            _outputDevice = _audioOutput->start();
            
            // setup a loopback audio output device
            _loopbackAudioOutput = new QAudioOutput(outputDeviceInfo, _outputFormat, this);
            
            gettimeofday(&_lastReceiveTime, NULL);
        }
        
        return;
    }
    
    qDebug() << "Unable to set up audio I/O because of a problem with input or output formats.\n";
}

void Audio::handleAudioInput() {
    static char monoAudioDataPacket[MAX_PACKET_SIZE];
    
    static int numBytesPacketHeader = numBytesForPacketHeader((unsigned char*) &PACKET_TYPE_MICROPHONE_AUDIO_NO_ECHO);
    static int leadingBytes = numBytesPacketHeader + sizeof(glm::vec3) + sizeof(glm::quat) +  NUM_BYTES_RFC4122_UUID;
    
    static int16_t* monoAudioSamples = (int16_t*) (monoAudioDataPacket + leadingBytes);
    
    static float inputToNetworkInputRatio = _numInputCallbackBytes * CALLBACK_ACCELERATOR_RATIO
        / NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL;
    
    static int inputSamplesRequired = NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL * inputToNetworkInputRatio;
    
    QByteArray inputByteArray = _inputDevice->readAll();
    
    if (Menu::getInstance()->isOptionChecked(MenuOption::EchoLocalAudio)) {
        // if this person wants local loopback add that to the locally injected audio
        
        if (!_loopbackOutputDevice) {
            // we didn't have the loopback output device going so set that up now
            _loopbackOutputDevice = _loopbackAudioOutput->start();
        }
        
        if (_inputFormat == _outputFormat) {
            _loopbackOutputDevice->write(inputByteArray);
        } else {
            static float loopbackOutputToInputRatio = (_outputFormat.sampleRate() / (float) _inputFormat.sampleRate())
                * (_outputFormat.channelCount() / _inputFormat.channelCount());
            
            QByteArray loopBackByteArray(inputByteArray.size() * loopbackOutputToInputRatio, 0);
            
            linearResampling((int16_t*) inputByteArray.data(), (int16_t*) loopBackByteArray.data(),
                             inputByteArray.size() / sizeof(int16_t),
                             loopBackByteArray.size() / sizeof(int16_t), _inputFormat, _outputFormat);
            
            _loopbackOutputDevice->write(loopBackByteArray);
        }
    }
    
    _inputRingBuffer.writeData(inputByteArray.data(), inputByteArray.size());
    
    while (_inputRingBuffer.samplesAvailable() > inputSamplesRequired) {
        
        int16_t inputAudioSamples[inputSamplesRequired];
        _inputRingBuffer.readSamples(inputAudioSamples, inputSamplesRequired);
        
        // zero out the monoAudioSamples array and the locally injected audio
        memset(monoAudioSamples, 0, NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL);
        
        // zero out the locally injected audio in preparation for audio procedural sounds
        memset(_localInjectedSamples, 0, NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL);
        
        if (!_muted) {
            // we aren't muted, downsample the input audio
            linearResampling((int16_t*) inputAudioSamples,
                             monoAudioSamples,
                             inputSamplesRequired,
                             NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL,
                             _inputFormat, _desiredInputFormat);
            
            float loudness = 0;
            
            for (int i = 0; i < NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL; i++) {
                loudness += fabsf(monoAudioSamples[i]);
            }
            
            _lastInputLoudness = loudness / NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL;
            
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
        
        NodeList* nodeList = NodeList::getInstance();
        Node* audioMixer = nodeList->soloNodeOfType(NODE_TYPE_AUDIO_MIXER);
        
        if (audioMixer && nodeList->getNodeActiveSocketOrPing(audioMixer)) {
            MyAvatar* interfaceAvatar = Application::getInstance()->getAvatar();
            
            glm::vec3 headPosition = interfaceAvatar->getHeadJointPosition();
            glm::quat headOrientation = interfaceAvatar->getHead().getOrientation();
            
            // we need the amount of bytes in the buffer + 1 for type
            // + 12 for 3 floats for position + float for bearing + 1 attenuation byte
            
            PACKET_TYPE packetType = Menu::getInstance()->isOptionChecked(MenuOption::EchoServerAudio)
                ? PACKET_TYPE_MICROPHONE_AUDIO_WITH_ECHO : PACKET_TYPE_MICROPHONE_AUDIO_NO_ECHO;
            
            char* currentPacketPtr = monoAudioDataPacket + populateTypeAndVersion((unsigned char*) monoAudioDataPacket,
                                                                                  packetType);
            
            // pack Source Data
            QByteArray rfcUUID = NodeList::getInstance()->getOwnerUUID().toRfc4122();
            memcpy(currentPacketPtr, rfcUUID.constData(), rfcUUID.size());
            currentPacketPtr += rfcUUID.size();
            
            // memcpy the three float positions
            memcpy(currentPacketPtr, &headPosition, sizeof(headPosition));
            currentPacketPtr += (sizeof(headPosition));
            
            // memcpy our orientation
            memcpy(currentPacketPtr, &headOrientation, sizeof(headOrientation));
            currentPacketPtr += sizeof(headOrientation);
            
            nodeList->getNodeSocket().writeDatagram(monoAudioDataPacket,
                                                    NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL + leadingBytes,
                                                    audioMixer->getActiveSocket()->getAddress(),
                                                    audioMixer->getActiveSocket()->getPort());
            
            Application::getInstance()->getBandwidthMeter()->outputStream(BandwidthMeter::AUDIO)
                .updateValue(NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL + leadingBytes);
        }
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
    
    _ringBuffer.parseData((unsigned char*) audioByteArray.data(), audioByteArray.size());
    
    static float networkOutputToOutputRatio = (_desiredOutputFormat.sampleRate() / (float) _outputFormat.sampleRate())
        * (_desiredOutputFormat.channelCount() / (float) _outputFormat.channelCount());
    
    static int numRequiredOutputSamples = NETWORK_BUFFER_LENGTH_SAMPLES_STEREO / networkOutputToOutputRatio;
    
    QByteArray outputBuffer;
    outputBuffer.resize(numRequiredOutputSamples * sizeof(int16_t));
    
    // if there is anything in the ring buffer, decide what to do
    if (_ringBuffer.samplesAvailable() > 0) {
        if (!_ringBuffer.isNotStarvedOrHasMinimumSamples(NETWORK_BUFFER_LENGTH_SAMPLES_STEREO
                                                         + (_jitterBufferSamples * 2))) {
            // starved and we don't have enough to start, keep waiting
            qDebug() << "Buffer is starved and doesn't have enough samples to start. Held back.\n";
        } else {
            //  We are either already playing back, or we have enough audio to start playing back.
            _ringBuffer.setIsStarved(false);
            
            // copy the samples we'll resample from the ring buffer - this also
            // pushes the read pointer of the ring buffer forwards
            int16_t ringBufferSamples[NETWORK_BUFFER_LENGTH_SAMPLES_STEREO];
            _ringBuffer.readSamples(ringBufferSamples, NETWORK_BUFFER_LENGTH_SAMPLES_STEREO);
            
            // add to the output samples whatever is in the _localAudioOutput byte array
            // that lets this user hear sound effects and loopback (if enabled)
            
            for (int i = 0; i < NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL; i++) {
                ringBufferSamples[i * 2] = glm::clamp(ringBufferSamples[i * 2] + _localInjectedSamples[i],
                                                      MIN_SAMPLE_VALUE, MAX_SAMPLE_VALUE);
                ringBufferSamples[(i * 2) + 1] = glm::clamp(ringBufferSamples[(i * 2) + 1] + _localInjectedSamples[i],
                                                            MIN_SAMPLE_VALUE, MAX_SAMPLE_VALUE);
            }
            
            // copy the packet from the RB to the output
            linearResampling(ringBufferSamples,
                             (int16_t*) outputBuffer.data(),
                             NETWORK_BUFFER_LENGTH_SAMPLES_STEREO,
                             numRequiredOutputSamples,
                             _desiredOutputFormat, _outputFormat);
            
            if (_outputDevice) {
                
                _outputDevice->write(outputBuffer);
                
                // add output (@speakers) data just written to the scope
                QMetaObject::invokeMethod(_scope, "addSamples", Qt::QueuedConnection,
                                          Q_ARG(QByteArray, QByteArray((char*) ringBufferSamples,
                                                                       NETWORK_BUFFER_LENGTH_BYTES_STEREO)),
                                          Q_ARG(bool, true), Q_ARG(bool, false));
            }
        }
        
    } else if (_audioOutput->bytesFree() == _audioOutput->bufferSize()) {
        // we don't have any audio data left in the output buffer, and the ring buffer from
        // the network has nothing in it either - we just starved
        qDebug() << "Audio output just starved.\n";
        _ringBuffer.setIsStarved(true);
        _numFramesDisplayStarve = 10;
    }
    
    Application::getInstance()->getBandwidthMeter()->inputStream(BandwidthMeter::AUDIO).updateValue(audioByteArray.size());
    
    _lastReceiveTime = currentReceiveTime;
}

bool Audio::mousePressEvent(int x, int y) {
    if (_iconBounds.contains(x, y)) {
        _muted = !_muted;
        return true;
    }
    return false;
}

void Audio::render(int screenWidth, int screenHeight) {
    if (_audioInput) {
        glLineWidth(2.0);
        glBegin(GL_LINES);
        glColor3f(1,1,1);
        
        int startX = 20.0;
        int currentX = startX;
        int topY = screenHeight - 40;
        int bottomY = screenHeight - 20;
        float frameWidth = 20.0;
        float halfY = topY + ((bottomY - topY) / 2.0);
        
        // draw the lines for the base of the ring buffer
        
        glVertex2f(currentX, topY);
        glVertex2f(currentX, bottomY);
        
        for (int i = 0; i < RING_BUFFER_LENGTH_FRAMES; i++) {
            glVertex2f(currentX, halfY);
            glVertex2f(currentX + frameWidth, halfY);
            currentX += frameWidth;
            
            glVertex2f(currentX, topY);
            glVertex2f(currentX, bottomY);
        }
        glEnd();
        
        // show a bar with the amount of audio remaining in ring buffer and output device
        // beyond the current playback
        
        int bytesLeftInAudioOutput = _audioOutput->bufferSize() - _audioOutput->bytesFree();
        float secondsLeftForAudioOutput = (bytesLeftInAudioOutput / sizeof(int16_t))
            / ((float) _outputFormat.sampleRate() * _outputFormat.channelCount());
        float secondsLeftForRingBuffer = _ringBuffer.samplesAvailable()
            / ((float) _desiredOutputFormat.sampleRate() * _desiredOutputFormat.channelCount());
        float msLeftForAudioOutput = (secondsLeftForAudioOutput + secondsLeftForRingBuffer) * 1000;
        
        if (_numFramesDisplayStarve == 0) {
            glColor3f(0, 1, 0);
        } else {
            glColor3f(0.5 + (_numFramesDisplayStarve / 20.0f), 0, 0);
            _numFramesDisplayStarve--;
        }
        
        if (_averagedLatency == 0.0) {
            _averagedLatency = msLeftForAudioOutput;
        } else {
            _averagedLatency = 0.99f * _averagedLatency + 0.01f * (msLeftForAudioOutput);
        }
        
        glBegin(GL_QUADS);
        glVertex2f(startX, topY + 2);
        glVertex2f(startX + _averagedLatency / AUDIO_CALLBACK_MSECS * frameWidth, topY + 2);
        glVertex2f(startX + _averagedLatency / AUDIO_CALLBACK_MSECS * frameWidth, bottomY - 2);
        glVertex2f(startX, bottomY - 2);
        glEnd();
        
        //  Show a yellow bar with the averaged msecs latency you are hearing (from time of packet receipt)
        glColor3f(1,1,0);
        glBegin(GL_QUADS);
        glVertex2f(startX + _averagedLatency / AUDIO_CALLBACK_MSECS * frameWidth - 2, topY - 2);
        glVertex2f(startX + _averagedLatency / AUDIO_CALLBACK_MSECS * frameWidth + 2, topY - 2);
        glVertex2f(startX + _averagedLatency / AUDIO_CALLBACK_MSECS * frameWidth + 2, bottomY + 2);
        glVertex2f(startX + _averagedLatency / AUDIO_CALLBACK_MSECS * frameWidth - 2, bottomY + 2);
        glEnd();
        
        char out[40];
        sprintf(out, "%3.0f\n", _averagedLatency);
        drawtext(startX + _averagedLatency / AUDIO_CALLBACK_MSECS * frameWidth - 10, topY - 9, 0.10, 0, 1, 0, out, 1,1,0);
        
        //  Show a red bar with the 'start' point of one frame plus the jitter buffer
        
        glColor3f(1, 0, 0);
        int jitterBufferPels = (1.f + (float)getJitterBufferSamples()
                                / (float) NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL) * frameWidth;
        sprintf(out, "%.0f\n", getJitterBufferSamples() / SAMPLE_RATE * 1000.f);
        drawtext(startX + jitterBufferPels - 5, topY - 9, 0.10, 0, 1, 0, out, 1, 0, 0);
        sprintf(out, "j %.1f\n", _measuredJitter);
        if (Menu::getInstance()->getAudioJitterBufferSamples() == 0) {
            drawtext(startX + jitterBufferPels - 5, bottomY + 12, 0.10, 0, 1, 0, out, 1, 0, 0);
        } else {
            drawtext(startX, bottomY + 12, 0.10, 0, 1, 0, out, 1, 0, 0);
        }
        
        glBegin(GL_QUADS);
        glVertex2f(startX + jitterBufferPels - 2, topY - 2);
        glVertex2f(startX + jitterBufferPels + 2, topY - 2);
        glVertex2f(startX + jitterBufferPels + 2, bottomY + 2);
        glVertex2f(startX + jitterBufferPels - 2, bottomY + 2);
        glEnd();
        
    }
    renderToolIcon(screenHeight);
}

//  Take a pointer to the acquired microphone input samples and add procedural sounds
void Audio::addProceduralSounds(int16_t* monoInput, int numSamples) {
    const float MAX_AUDIBLE_VELOCITY = 6.0;
    const float MIN_AUDIBLE_VELOCITY = 0.1;
    const int VOLUME_BASELINE = 400;
    const float SOUND_PITCH = 8.f;
    
    float speed = glm::length(_lastVelocity);
    float volume = VOLUME_BASELINE * (1.f - speed / MAX_AUDIBLE_VELOCITY);
    
    float sample;
    
    // Travelling noise
    //  Add a noise-modulated sinewave with volume that tapers off with speed increasing
    if ((speed > MIN_AUDIBLE_VELOCITY) && (speed < MAX_AUDIBLE_VELOCITY)) {
        for (int i = 0; i < numSamples; i++) {
            monoInput[i] += (int16_t)(sinf((float) (_proceduralEffectSample + i) / SOUND_PITCH )
                                      * volume * (1.f + randFloat() * 0.25f) * speed);
        }
    }
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
            
            monoInput[i] = glm::clamp(monoInput[i] + collisionSample, MIN_SAMPLE_VALUE, MAX_SAMPLE_VALUE);
            _localInjectedSamples[i] = glm::clamp(_localInjectedSamples[i] + collisionSample,
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
    float frequency = (_drumSoundFrequency / SAMPLE_RATE) * PI_TIMES_TWO;
    if (_drumSoundVolume > 0.f) {
        for (int i = 0; i < numSamples; i++) {
            t = (float) _drumSoundSample + (float) i;
            sample = sinf(t * frequency);
            sample += ((randFloat() - 0.5f) * NOISE_MAGNITUDE);
            sample *= _drumSoundVolume * MAX_VOLUME;
            
            int16_t collisionSample = (int16_t) sample;
            
            monoInput[i] = glm::clamp(monoInput[i] + collisionSample, MIN_SAMPLE_VALUE, MAX_SAMPLE_VALUE);
            _localInjectedSamples[i] = glm::clamp(_localInjectedSamples[i] + collisionSample,
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

void Audio::renderToolIcon(int screenHeight) {
    
    _iconBounds = QRect(ICON_LEFT, screenHeight - BOTTOM_PADDING, ICON_SIZE, ICON_SIZE);
    glEnable(GL_TEXTURE_2D);
    
    glBindTexture(GL_TEXTURE_2D, _micTextureId);
    glColor3f(1, 1, 1);
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