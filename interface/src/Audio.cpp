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

#include <QtMultimedia/QAudioInput>
#include <QtMultimedia/QAudioOutput>

#include <AngleUtil.h>
#include <NodeList.h>
#include <NodeTypes.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <StdDev.h>
#include <QSvgRenderer>

#include "Application.h"
#include "Audio.h"
#include "Menu.h"
#include "Util.h"

static const float JITTER_BUFFER_LENGTH_MSECS = 12;
static const short JITTER_BUFFER_SAMPLES = JITTER_BUFFER_LENGTH_MSECS *
                                           NUM_AUDIO_CHANNELS * (SAMPLE_RATE / 1000.0);

static const float AUDIO_CALLBACK_MSECS = (float)BUFFER_LENGTH_SAMPLES_PER_CHANNEL / (float)SAMPLE_RATE * 1000.0;

// Mute icon configration
static const int ICON_SIZE = 24;
static const int ICON_LEFT = 20;
static const int BOTTOM_PADDING = 110;

Audio::Audio(int16_t initialJitterBufferSamples, QObject* parent) :
    QObject(parent),
    _inputDevice(NULL),
    _ringBuffer(true),
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
    _heartbeatMagnitude(0.0f),
    _muted(false)
{

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

const int QT_SAMPLE_RATE = 44100;
const int SAMPLE_RATE_RATIO = QT_SAMPLE_RATE / SAMPLE_RATE;

void Audio::start() {
    
    QAudioFormat audioFormat;
    // set up the desired audio format
    audioFormat.setSampleRate(QT_SAMPLE_RATE);
    audioFormat.setSampleSize(16);
    audioFormat.setCodec("audio/pcm");
    audioFormat.setSampleType(QAudioFormat::SignedInt);
    audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    audioFormat.setChannelCount(2);
    
    qDebug() << "The format for audio I/O is" << audioFormat << "\n";
    
    QAudioDeviceInfo inputAudioDevice = defaultAudioDeviceForMode(QAudio::AudioInput);
    
    qDebug() << "Audio input device is" << inputAudioDevice.deviceName() << "\n";
    if (!inputAudioDevice.isFormatSupported(audioFormat)) {
        qDebug() << "The desired audio input format is not supported by this device. Not starting audio input.\n";
        return;
    }
    
    _audioInput = new QAudioInput(inputAudioDevice, audioFormat, this);
    _audioInput->setBufferSize(BUFFER_LENGTH_BYTES_STEREO * SAMPLE_RATE_RATIO);
    _inputDevice = _audioInput->start();
    
    connect(_inputDevice, SIGNAL(readyRead()), SLOT(handleAudioInput()));
    
    QAudioDeviceInfo outputDeviceInfo = defaultAudioDeviceForMode(QAudio::AudioOutput);
    
    qDebug() << outputDeviceInfo.supportedSampleRates() << "\n";
    
    qDebug() << "Audio output device is" << outputDeviceInfo.deviceName() << "\n";
    
    if (!outputDeviceInfo.isFormatSupported(audioFormat)) {
        qDebug() << "The desired audio output format is not supported by this device.\n";
        return;
    }
    
    _audioOutput = new QAudioOutput(outputDeviceInfo, audioFormat, this);
    _audioOutput->setBufferSize(BUFFER_LENGTH_BYTES_STEREO * SAMPLE_RATE_RATIO);
    _outputDevice = _audioOutput->start();
    
    gettimeofday(&_lastReceiveTime, NULL);
}

void Audio::handleAudioInput() {
    static int16_t stereoInputBuffer[BUFFER_LENGTH_SAMPLES_PER_CHANNEL * 2 * SAMPLE_RATE_RATIO];
    static int16_t stereoOutputBuffer[BUFFER_LENGTH_SAMPLES_PER_CHANNEL * 2 * SAMPLE_RATE_RATIO];
    static char monoAudioDataPacket[MAX_PACKET_SIZE];
    
    // read out the current samples from the _inputDevice
    _inputDevice->read((char*) stereoInputBuffer, BUFFER_LENGTH_BYTES_STEREO * SAMPLE_RATE_RATIO);

    NodeList* nodeList = NodeList::getInstance();
    Node* audioMixer = nodeList->soloNodeOfType(NODE_TYPE_AUDIO_MIXER);
    
    if (Menu::getInstance()->isOptionChecked(MenuOption::EchoLocalAudio) && !_muted) {
        //  if local loopback enabled, copy input to output
        memcpy(stereoOutputBuffer, stereoInputBuffer, sizeof(stereoOutputBuffer));
    } else {
        // zero out the stereoOutputBuffer
        memset(stereoOutputBuffer, 0, sizeof(stereoOutputBuffer));
    }
    
    if (audioMixer) {
        if (audioMixer->getActiveSocket()) {
            Avatar* interfaceAvatar = Application::getInstance()->getAvatar();
            
            glm::vec3 headPosition = interfaceAvatar->getHeadJointPosition();
            glm::quat headOrientation = interfaceAvatar->getHead().getOrientation();
            
            int numBytesPacketHeader = numBytesForPacketHeader((unsigned char*) &PACKET_TYPE_MICROPHONE_AUDIO_NO_ECHO);
            int leadingBytes = numBytesPacketHeader + sizeof(headPosition) + sizeof(headOrientation);
            
            // we need the amount of bytes in the buffer + 1 for type
            // + 12 for 3 floats for position + float for bearing + 1 attenuation byte
            
            PACKET_TYPE packetType = Menu::getInstance()->isOptionChecked(MenuOption::EchoServerAudio)
                ? PACKET_TYPE_MICROPHONE_AUDIO_WITH_ECHO
                : PACKET_TYPE_MICROPHONE_AUDIO_NO_ECHO;
            
            char* currentPacketPtr = monoAudioDataPacket + populateTypeAndVersion((unsigned char*) monoAudioDataPacket, packetType);
            
            // pack Source Data
            QByteArray rfcUUID = NodeList::getInstance()->getOwnerUUID().toRfc4122();
            memcpy(currentPacketPtr, rfcUUID.constData(), rfcUUID.size());
            currentPacketPtr += rfcUUID.size();
            leadingBytes += rfcUUID.size();
            
            // memcpy the three float positions
            memcpy(currentPacketPtr, &headPosition, sizeof(headPosition));
            currentPacketPtr += (sizeof(headPosition));
            
            // memcpy our orientation
            memcpy(currentPacketPtr, &headOrientation, sizeof(headOrientation));
            currentPacketPtr += sizeof(headOrientation);
            
            if (!_muted) {
                // we aren't muted, average each set of four samples together to set up the mono input buffers
                for (int i = 2; i < BUFFER_LENGTH_SAMPLES_PER_CHANNEL * 2 * SAMPLE_RATE_RATIO; i += 4) {
                    
                    int16_t averagedSample = 0;
                    if (i + 2 == BUFFER_LENGTH_SAMPLES_PER_CHANNEL * 2 * SAMPLE_RATE_RATIO) {
                        averagedSample = (stereoInputBuffer[i - 2] / 2) + (stereoInputBuffer[i] / 2);
                    } else {
                        averagedSample = (stereoInputBuffer[i - 2] / 4) + (stereoInputBuffer[i] / 2)
                            + (stereoInputBuffer[i + 2] / 4);
                    }
                    
                    // copy the averaged sample to our array
                    memcpy(currentPacketPtr + (((i - 2) / 4) * sizeof(int16_t)), &averagedSample, sizeof(int16_t));
                }
            } else {
                // zero out the audio part of the array
                memset(currentPacketPtr, 0, BUFFER_LENGTH_BYTES_PER_CHANNEL);
            }
            
            // Add procedural effects to input samples
            addProceduralSounds((int16_t*) currentPacketPtr, stereoOutputBuffer, BUFFER_LENGTH_SAMPLES_PER_CHANNEL);
            
            nodeList->getNodeSocket()->send(audioMixer->getActiveSocket(),
                                            monoAudioDataPacket,
                                            BUFFER_LENGTH_BYTES_PER_CHANNEL + leadingBytes);
        } else {
            nodeList->pingPublicAndLocalSocketsForInactiveNode(audioMixer);
        }
    }
    
    AudioRingBuffer* ringBuffer = &_ringBuffer;
    
    // if there is anything in the ring buffer, decide what to do
    
    if (ringBuffer->getEndOfLastWrite()) {
        if (ringBuffer->isStarved() && ringBuffer->diffLastWriteNextOutput() <
            (PACKET_LENGTH_SAMPLES + _jitterBufferSamples * (ringBuffer->isStereo() ? 2 : 1))) {
            //  If not enough audio has arrived to start playback, keep waiting
        } else if (!ringBuffer->isStarved() && ringBuffer->diffLastWriteNextOutput() == 0) {
            //  If we have started and now have run out of audio to send to the audio device,
            //  this means we've starved and should restart.
            ringBuffer->setIsStarved(true);
            
        } else {
            //  We are either already playing back, or we have enough audio to start playing back.
            if (ringBuffer->isStarved()) {
                ringBuffer->setIsStarved(false);
                ringBuffer->setHasStarted(true);
            }
            
            // play whatever we have in the audio buffer
            for (int s = 0; s < PACKET_LENGTH_SAMPLES_PER_CHANNEL; s++) {
                int16_t leftSample = ringBuffer->getNextOutput()[s];
                int16_t rightSample = ringBuffer->getNextOutput()[s + PACKET_LENGTH_SAMPLES_PER_CHANNEL];
                
                stereoOutputBuffer[(s * 4)] += leftSample;
                stereoOutputBuffer[(s * 4) + 2] += leftSample;
                
                stereoOutputBuffer[(s * 4) + 1] += rightSample;
                stereoOutputBuffer[(s * 4) + 3] += rightSample;
            }
            
            ringBuffer->setNextOutput(ringBuffer->getNextOutput() + PACKET_LENGTH_SAMPLES);
            
            if (ringBuffer->getNextOutput() == ringBuffer->getBuffer() + RING_BUFFER_LENGTH_SAMPLES) {
                ringBuffer->setNextOutput(ringBuffer->getBuffer());
            }
        }
    }

    eventuallySendRecvPing(inputLeft, outputLeft, outputRight);


    // add output (@speakers) data just written to the scope
    _scope->addSamples(1, outputLeft, BUFFER_LENGTH_SAMPLES_PER_CHANNEL);
    _scope->addSamples(2, outputRight, BUFFER_LENGTH_SAMPLES_PER_CHANNEL);
    
    gettimeofday(&_lastCallbackTime, NULL);
}

// inputBuffer  A pointer to an internal portaudio data buffer containing data read by portaudio.
// outputBuffer A pointer to an internal portaudio data buffer to be read by the configured output device.
// frames       Number of frames that portaudio requests to be read/written.
// timeInfo     Portaudio time info. Currently unused.
// statusFlags  Portaudio status flags. Currently unused.
// userData     Pointer to supplied user data (in this case, a pointer to the parent Audio object
int Audio::audioCallback (const void* inputBuffer,
                          void* outputBuffer,
                          unsigned long frames,
                          const PaStreamCallbackTimeInfo *timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void* userData) {
    
    // copy the audio data to the output device
    _outputDevice->write((char*) stereoOutputBuffer, sizeof(stereoOutputBuffer));
    _outputDevice->write((char*) stereoOutputBuffer, sizeof(stereoOutputBuffer));
    
    // add output (@speakers) data just written to the scope
//    _scope->addSamples(1, outputLeft, BUFFER_LENGTH_SAMPLES_PER_CHANNEL);
//    _scope->addSamples(2, outputRight, BUFFER_LENGTH_SAMPLES_PER_CHANNEL);
    
    gettimeofday(&_lastCallbackTime, NULL);
}

void Audio::addReceivedAudioToBuffer(unsigned char* receivedData, int receivedBytes) {
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
        const int MAX_JITTER_BUFFER_SAMPLES = RING_BUFFER_LENGTH_SAMPLES / 2;
        const float NUM_STANDARD_DEVIATIONS = 3.f;
        if (Menu::getInstance()->getAudioJitterBufferSamples() == 0) {
            float newJitterBufferSamples = (NUM_STANDARD_DEVIATIONS * _measuredJitter)
                                            / 1000.f
                                            * SAMPLE_RATE;
            setJitterBufferSamples(glm::clamp((int)newJitterBufferSamples, 0, MAX_JITTER_BUFFER_SAMPLES));
        }
    }
    
    if (_ringBuffer.diffLastWriteNextOutput() + PACKET_LENGTH_SAMPLES >
        PACKET_LENGTH_SAMPLES + (ceilf((float) (_jitterBufferSamples * 2) / PACKET_LENGTH_SAMPLES) * PACKET_LENGTH_SAMPLES)) {
        // this packet would give us more than the required amount for play out
        // discard the first packet in the buffer
        
        _ringBuffer.setNextOutput(_ringBuffer.getNextOutput() + PACKET_LENGTH_SAMPLES);
        
        if (_ringBuffer.getNextOutput() == _ringBuffer.getBuffer() + RING_BUFFER_LENGTH_SAMPLES) {
            _ringBuffer.setNextOutput(_ringBuffer.getBuffer());
        }
    }
    
    _ringBuffer.parseData((unsigned char*) receivedData, receivedBytes);
   
    Application::getInstance()->getBandwidthMeter()->inputStream(BandwidthMeter::AUDIO)
            .updateValue(PACKET_LENGTH_BYTES + sizeof(PACKET_TYPE));
 
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
    if (true) {
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
        
        for (int i = 0; i < RING_BUFFER_LENGTH_FRAMES / 2; i++) {
            glVertex2f(currentX, halfY);
            glVertex2f(currentX + frameWidth, halfY);
            currentX += frameWidth;
            
            glVertex2f(currentX, topY);
            glVertex2f(currentX, bottomY);
        }
        glEnd();
        
        //  Show a bar with the amount of audio remaining in ring buffer beyond current playback
        float remainingBuffer = 0;
        timeval currentTime;
        gettimeofday(&currentTime, NULL);
        float timeLeftInCurrentBuffer = 0;
        if (_lastCallbackTime.tv_usec > 0) {
            timeLeftInCurrentBuffer = AUDIO_CALLBACK_MSECS - diffclock(&_lastCallbackTime, &currentTime);
        }
        
        if (_ringBuffer.getEndOfLastWrite() != NULL)
            remainingBuffer = _ringBuffer.diffLastWriteNextOutput() / PACKET_LENGTH_SAMPLES * AUDIO_CALLBACK_MSECS;
        
        if (_wasStarved == 0) {
            glColor3f(0, 1, 0);
        } else {
            glColor3f(0.5 + (_wasStarved / 20.0f), 0, 0);
            _wasStarved--;
        }
        
        glBegin(GL_QUADS);
        glVertex2f(startX, topY + 2);
        glVertex2f(startX + (remainingBuffer + timeLeftInCurrentBuffer)/AUDIO_CALLBACK_MSECS*frameWidth, topY + 2);
        glVertex2f(startX + (remainingBuffer + timeLeftInCurrentBuffer)/AUDIO_CALLBACK_MSECS*frameWidth, bottomY - 2);
        glVertex2f(startX, bottomY - 2);
        glEnd();
        
        if (_averagedLatency == 0.0) {
            _averagedLatency = remainingBuffer + timeLeftInCurrentBuffer;
        } else {
            _averagedLatency = 0.99f * _averagedLatency + 0.01f * (remainingBuffer + timeLeftInCurrentBuffer);
        }
        
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
        int jitterBufferPels = (1.f + (float)getJitterBufferSamples() / (float)PACKET_LENGTH_SAMPLES_PER_CHANNEL) * frameWidth;
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
void Audio::addProceduralSounds(int16_t* inputBuffer, int16_t* stereoOutput, int numSamples) {
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
            inputBuffer[i] += (int16_t)(sinf((float) (_proceduralEffectSample + i) / SOUND_PITCH )
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
            
            sample = sinf(t * _collisionSoundFrequency) +
                     sinf(t * _collisionSoundFrequency / DOWN_TWO_OCTAVES) +
                     sinf(t * _collisionSoundFrequency / DOWN_FOUR_OCTAVES * UP_MAJOR_FIFTH);
            sample *= _collisionSoundMagnitude * COLLISION_SOUND_MAX_VOLUME;
            
            int16_t collisionSample = (int16_t) sample;
            
            inputBuffer[i] += collisionSample;
            
            for (int j = (i * 4); j < (i * 4) + 4; j++) {
                stereoOutput[j] = collisionSample;
            }
            
            _collisionSoundMagnitude *= _collisionSoundDuration;
        }
    }
    _proceduralEffectSample += numSamples;
}

//  Starts a collision sound.  magnitude is 0-1, with 1 the loudest possible sound.
void Audio::startCollisionSound(float magnitude, float frequency, float noise, float duration, bool flashScreen) {
    _collisionSoundMagnitude = magnitude;
    _collisionSoundFrequency = frequency;
    _collisionSoundNoise = noise;
    _collisionSoundDuration = duration;
    _collisionFlashesScreen = flashScreen;
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
