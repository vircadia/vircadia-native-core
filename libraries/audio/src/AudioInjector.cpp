//
//  AudioInjector.cpp
//  libraries/audio/src
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AudioInjector.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDataStream>

#include <NodeList.h>
#include <udt/PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>

#include "AbstractAudioInterface.h"
#include "AudioInjectorManager.h"
#include "AudioRingBuffer.h"
#include "AudioLogging.h"
#include "SoundCache.h"
#include "AudioHelpers.h"

int metaType = qRegisterMetaType<AudioInjectorPointer>("AudioInjectorPointer");

AbstractAudioInterface* AudioInjector::_localAudioInterface{ nullptr };

AudioInjectorState operator& (AudioInjectorState lhs, AudioInjectorState rhs) {
    return static_cast<AudioInjectorState>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
};

AudioInjectorState& operator|= (AudioInjectorState& lhs, AudioInjectorState rhs) {
    lhs = static_cast<AudioInjectorState>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
    return lhs;
};

AudioInjector::AudioInjector(SharedSoundPointer sound, const AudioInjectorOptions& injectorOptions) :
    _sound(sound),
    _audioData(sound->getAudioData()),
    _options(injectorOptions)
{
}

AudioInjector::AudioInjector(AudioDataPointer audioData, const AudioInjectorOptions& injectorOptions) :
    _audioData(audioData),
    _options(injectorOptions)
{
}

AudioInjector::~AudioInjector() {}

bool AudioInjector::stateHas(AudioInjectorState state) const {
    return resultWithReadLock<bool>([&] {
        return (_state & state) == state;
    });
}

void AudioInjector::setOptions(const AudioInjectorOptions& options) {
    // since options.stereo is computed from the audio stream,
    // we need to copy it from existing options just in case.
    withWriteLock([&] {
        bool currentlyStereo = _options.stereo;
        bool currentlyAmbisonic = _options.ambisonic;
        _options = options;
        _options.stereo = currentlyStereo;
        _options.ambisonic = currentlyAmbisonic;
    });
}

void AudioInjector::finishNetworkInjection() {
    withWriteLock([&] {
        _state |= AudioInjectorState::NetworkInjectionFinished;
    });

    // if we are already finished with local
    // injection, then we are finished
    if(stateHas(AudioInjectorState::LocalInjectionFinished)) {
        finish();
    }
}

void AudioInjector::finishLocalInjection() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "finishLocalInjection");
        return;
    }

    bool localOnly = false;
    withWriteLock([&] {
        _state |= AudioInjectorState::LocalInjectionFinished;
        localOnly = _options.localOnly;
    });

    if(localOnly || stateHas(AudioInjectorState::NetworkInjectionFinished)) {
        finish();
    }
}

void AudioInjector::finish() {
    withWriteLock([&] {
        _state |= AudioInjectorState::LocalInjectionFinished;
        _state |= AudioInjectorState::NetworkInjectionFinished;
        _state |= AudioInjectorState::Finished;
    });
    emit finished();
    _localBuffer = nullptr;
}

void AudioInjector::restart() {
    // reset the current send offset to zero
    _currentSendOffset = 0;

    // reset state to start sending from beginning again
    _nextFrame = 0;
    if (_frameTimer) {
        _frameTimer->invalidate();
    }
    _hasSentFirstFrame = false;

    // check our state to decide if we need extra handling for the restart request
    if (stateHas(AudioInjectorState::Finished)) {
        if (!inject(&AudioInjectorManager::threadInjector)) {
            qWarning() << "AudioInjector::restart failed to thread injector";
        }
    }
}

bool AudioInjector::inject(bool(AudioInjectorManager::*injection)(const AudioInjectorPointer&)) {
    AudioInjectorOptions options;
    withWriteLock([&] {
        _state = AudioInjectorState::NotFinished;
        options = _options;
    });

    int byteOffset = 0;
    if (options.secondOffset > 0.0f) {
        int numChannels = options.ambisonic ? 4 : (options.stereo ? 2 : 1);
        byteOffset = (int)(AudioConstants::SAMPLE_RATE * options.secondOffset * numChannels);
        byteOffset *= AudioConstants::SAMPLE_SIZE;
    }
    _currentSendOffset = byteOffset;

    if (!injectLocally()) {
        finishLocalInjection();
    }

    bool success = true;
    if (!options.localOnly) {
        auto injectorManager = DependencyManager::get<AudioInjectorManager>();
        if (!(*injectorManager.*injection)(sharedFromThis())) {
            success = false;
            finishNetworkInjection();
        }
    }
    return success;
}

bool AudioInjector::injectLocally() {
    bool success = false;
    if (_localAudioInterface) {
        if (_audioData->getNumBytes() > 0) {

            _localBuffer = QSharedPointer<AudioInjectorLocalBuffer>(new AudioInjectorLocalBuffer(_audioData), &AudioInjectorLocalBuffer::deleteLater);
            _localBuffer->moveToThread(thread());

            _localBuffer->open(QIODevice::ReadOnly);
            _localBuffer->setShouldLoop(_options.loop);

            // give our current send position to the local buffer
            _localBuffer->setCurrentOffset(_currentSendOffset);

            // call this function on the AudioClient's thread
            // this will move the local buffer's thread to the LocalInjectorThread
            success = _localAudioInterface->outputLocalInjector(sharedFromThis());

            if (!success) {
                qCDebug(audio) << "AudioInjector::injectLocally could not output locally via _localAudioInterface";
            }
        } else {
            qCDebug(audio) << "AudioInjector::injectLocally called without any data in Sound QByteArray";
        }
    }

    return success;
}

const uchar MAX_INJECTOR_VOLUME = packFloatGainToByte(1.0f);
static const int64_t NEXT_FRAME_DELTA_ERROR_OR_FINISHED = -1;
static const int64_t NEXT_FRAME_DELTA_IMMEDIATELY = 0;

qint64 writeStringToStream(const QString& string, QDataStream& stream) {
    QByteArray data = string.toUtf8();
    uint32_t length = data.length();
    if (length == 0) {
        stream << static_cast<quint32>(length);
    } else {
        // http://doc.qt.io/qt-5/datastreamformat.html
        // QDataStream << QByteArray -
        //   If the byte array is null : 0xFFFFFFFF (quint32)
        //   Otherwise : the array size(quint32) followed by the array bytes, i.e.size bytes
        stream << data;
    }
    return length + sizeof(uint32_t);
}

int64_t AudioInjector::injectNextFrame() {
    if (stateHas(AudioInjectorState::NetworkInjectionFinished)) {
        return NEXT_FRAME_DELTA_ERROR_OR_FINISHED;
    }

    // if we haven't setup the packet to send then do so now
    static int loopbackOptionOffset = -1;
    static int positionOptionOffset = -1;
    static int volumeOptionOffset = -1;
    static int audioDataOffset = -1;

    AudioInjectorOptions options = resultWithReadLock<AudioInjectorOptions>([&] {
        return _options;
    });

    if (!_currentPacket) {
        if (_currentSendOffset < 0 ||
            _currentSendOffset >= (int)_audioData->getNumBytes()) {
            _currentSendOffset = 0;
        }

        // make sure we actually have samples downloaded to inject
        if (_audioData && _audioData->getNumSamples() > 0) {
            _outgoingSequenceNumber = 0;
            _nextFrame = 0;

            if (!_frameTimer) {
                _frameTimer = std::unique_ptr<QElapsedTimer>(new QElapsedTimer);
            }

            _frameTimer->restart();

            _currentPacket = NLPacket::create(PacketType::InjectAudio);

            // setup the packet for injected audio
            QDataStream audioPacketStream(_currentPacket.get());

            // pack some placeholder sequence number for now
            audioPacketStream << (quint16) 0;

            // current injectors don't use codecs, so pack in the unknown codec name
            QString noCodecForInjectors("");
            writeStringToStream(noCodecForInjectors, audioPacketStream);

            // pack stream identifier (a generated UUID)
            audioPacketStream << _streamID;

            // pack the stereo/mono type of the stream
            audioPacketStream << options.stereo;

            // pack the flag for loopback, if requested
            loopbackOptionOffset = _currentPacket->pos();
            uchar loopbackFlag = (_localAudioInterface && _localAudioInterface->shouldLoopbackInjectors());
            audioPacketStream << loopbackFlag;

            // pack the position for injected audio
            positionOptionOffset = _currentPacket->pos();
            audioPacketStream.writeRawData(reinterpret_cast<const char*>(&options.position),
                                           sizeof(options.position));

            // pack our orientation for injected audio
            audioPacketStream.writeRawData(reinterpret_cast<const char*>(&options.orientation),
                                           sizeof(options.orientation));

            audioPacketStream.writeRawData(reinterpret_cast<const char*>(&options.position),
                                           sizeof(options.position));

            glm::vec3 boxCorner = glm::vec3(0);
            audioPacketStream.writeRawData(reinterpret_cast<const char*>(&boxCorner),
                sizeof(glm::vec3));

            // pack zero for radius
            float radius = 0;
            audioPacketStream << radius;

            // pack 255 for attenuation byte
            volumeOptionOffset = _currentPacket->pos();
            quint8 volume = MAX_INJECTOR_VOLUME;
            audioPacketStream << volume;
            audioPacketStream << options.ignorePenumbra;

            audioDataOffset = _currentPacket->pos();

        } else {
            // no samples to inject, return immediately
            qCDebug(audio)  << "AudioInjector::injectNextFrame() called with no samples to inject. Returning.";
            return NEXT_FRAME_DELTA_ERROR_OR_FINISHED;
        }
    }

    if (!_frameTimer->isValid()) {
        // in the case where we have been restarted, the frame timer will be invalid and we need to start it back over here
        _frameTimer->restart();
    }

    assert(loopbackOptionOffset != -1);
    assert(positionOptionOffset != -1);
    assert(volumeOptionOffset != -1);
    assert(audioDataOffset != -1);

    _currentPacket->seek(0);

    // pack the sequence number
    _currentPacket->writePrimitive(_outgoingSequenceNumber);

    _currentPacket->seek(loopbackOptionOffset);
    _currentPacket->writePrimitive((uchar)(_localAudioInterface && _localAudioInterface->shouldLoopbackInjectors()));

    _currentPacket->seek(positionOptionOffset);
    _currentPacket->writePrimitive(options.position);
    _currentPacket->writePrimitive(options.orientation);

    quint8 volume = packFloatGainToByte(options.volume);
    _currentPacket->seek(volumeOptionOffset);
    _currentPacket->writePrimitive(volume);

    _currentPacket->seek(audioDataOffset);

    // This code is copying bytes from the _sound directly into the packet, handling looping appropriately.
    // Might be a reasonable place to do the encode step here.
    QByteArray decodedAudio;

    int totalBytesLeftToCopy = (options.stereo ? 2 : 1) * AudioConstants::NETWORK_FRAME_BYTES_PER_CHANNEL;
    if (!options.loop) {
        // If we aren't looping, let's make sure we don't read past the end
        int bytesLeftToRead = _audioData->getNumBytes() - _currentSendOffset;
        totalBytesLeftToCopy = std::min(totalBytesLeftToCopy, bytesLeftToRead);
    }

    auto samples = _audioData->data();
    auto currentSample = _currentSendOffset / AudioConstants::SAMPLE_SIZE;
    auto samplesLeftToCopy = totalBytesLeftToCopy / AudioConstants::SAMPLE_SIZE;

    using AudioConstants::AudioSample;
    decodedAudio.resize(totalBytesLeftToCopy);
    auto samplesOut = reinterpret_cast<AudioSample*>(decodedAudio.data());

    //  Copy and Measure the loudness of this frame
    withWriteLock([&] {
        _loudness = 0.0f;
        for (int i = 0; i < samplesLeftToCopy; ++i) {
            auto index = (currentSample + i) % _audioData->getNumSamples();
            auto sample = samples[index];
            samplesOut[i] = sample;
            _loudness += abs(sample) / (AudioConstants::MAX_SAMPLE_VALUE / 2.0f);
        }
        _loudness /= (float)samplesLeftToCopy;
    });
    _currentSendOffset = (_currentSendOffset + totalBytesLeftToCopy) %
                         _audioData->getNumBytes();

    // FIXME -- good place to call codec encode here. We need to figure out how to tell the AudioInjector which
    // codec to use... possible through AbstractAudioInterface.
    QByteArray encodedAudio = decodedAudio;
    _currentPacket->write(encodedAudio.data(), encodedAudio.size());

    // set the correct size used for this packet
    _currentPacket->setPayloadSize(_currentPacket->pos());

    // grab our audio mixer from the NodeList, if it exists
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer audioMixer = nodeList->soloNodeOfType(NodeType::AudioMixer);

    if (audioMixer) {
        // send off this audio packet
        nodeList->sendUnreliablePacket(*_currentPacket, *audioMixer);
        _outgoingSequenceNumber++;
    }

    if (_currentSendOffset == 0 && !options.loop) {
        finishNetworkInjection();
        return NEXT_FRAME_DELTA_ERROR_OR_FINISHED;
    }

    if (!_hasSentFirstFrame) {
        _hasSentFirstFrame = true;
        // ask AudioInjectorManager to call us right away again to
        // immediately send the first two frames so the mixer can start using the audio right away
        return NEXT_FRAME_DELTA_IMMEDIATELY;
    }

    const int MAX_ALLOWED_FRAMES_TO_FALL_BEHIND = 7;
    int64_t currentTime = _frameTimer->nsecsElapsed() / 1000;
    auto currentFrameBasedOnElapsedTime = currentTime / AudioConstants::NETWORK_FRAME_USECS;

    if (currentFrameBasedOnElapsedTime - _nextFrame > MAX_ALLOWED_FRAMES_TO_FALL_BEHIND) {
        // If we are falling behind by more frames than our threshold, let's skip the frames ahead
        qCDebug(audio)  << this << "injectNextFrame() skipping ahead, fell behind by " << (currentFrameBasedOnElapsedTime - _nextFrame) << " frames";
        _nextFrame = currentFrameBasedOnElapsedTime;
        _currentSendOffset = _nextFrame * AudioConstants::NETWORK_FRAME_BYTES_PER_CHANNEL * (options.stereo ? 2 : 1) % _audioData->getNumBytes();
    }

    int64_t playNextFrameAt = ++_nextFrame * AudioConstants::NETWORK_FRAME_USECS;

    return std::max(INT64_C(0), playNextFrameAt - currentTime);
}


void AudioInjector::sendStopInjectorPacket() {
    auto nodeList = DependencyManager::get<NodeList>();
    if (auto audioMixer = nodeList->soloNodeOfType(NodeType::AudioMixer)) {
        // Build packet
        auto stopInjectorPacket = NLPacket::create(PacketType::StopInjector);
        stopInjectorPacket->write(_streamID.toRfc4122());

        // Send packet
        nodeList->sendUnreliablePacket(*stopInjectorPacket, *audioMixer);
    }
}
