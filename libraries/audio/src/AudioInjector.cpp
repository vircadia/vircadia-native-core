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
#include "AudioSRC.h"
#include "AudioHelpers.h"

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

AudioInjector::~AudioInjector() {
    deleteLocalBuffer();
}

bool AudioInjector::stateHas(AudioInjectorState state) const {
    return (_state & state) == state;
}

void AudioInjector::setOptions(const AudioInjectorOptions& options) {
    // since options.stereo is computed from the audio stream,
    // we need to copy it from existing options just in case.
    bool currentlyStereo = _options.stereo;
    bool currentlyAmbisonic = _options.ambisonic;
    _options = options;
    _options.stereo = currentlyStereo;
    _options.ambisonic = currentlyAmbisonic;
}

void AudioInjector::finishNetworkInjection() {
    _state |= AudioInjectorState::NetworkInjectionFinished;

    // if we are already finished with local
    // injection, then we are finished
    if(stateHas(AudioInjectorState::LocalInjectionFinished)) {
        finish();
    }
}

void AudioInjector::finishLocalInjection() {
    _state |= AudioInjectorState::LocalInjectionFinished;
    if(_options.localOnly || stateHas(AudioInjectorState::NetworkInjectionFinished)) {
        finish();
    }
}

void AudioInjector::finish() {
    _state |= AudioInjectorState::Finished;

    emit finished();

    deleteLocalBuffer();
}

void AudioInjector::restart() {
    // grab the AudioInjectorManager
    auto injectorManager = DependencyManager::get<AudioInjectorManager>();

    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "restart");

        if (!_options.localOnly) {
            // notify the AudioInjectorManager to wake up in case it's waiting for new injectors
            injectorManager->notifyInjectorReadyCondition();
        }

        return;
    }

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
        if (!inject(&AudioInjectorManager::restartFinishedInjector)) {
            qWarning() << "AudioInjector::restart failed to thread injector";
        }
    }
}

bool AudioInjector::inject(bool(AudioInjectorManager::*injection)(const AudioInjectorPointer&)) {
    _state = AudioInjectorState::NotFinished;

    int byteOffset = 0;
    if (_options.secondOffset > 0.0f) {
        int numChannels = _options.ambisonic ? 4 : (_options.stereo ? 2 : 1);
        byteOffset = (int)(AudioConstants::SAMPLE_RATE * _options.secondOffset * numChannels);
        byteOffset *= AudioConstants::SAMPLE_SIZE;
    }
    _currentSendOffset = byteOffset;

    if (!injectLocally()) {
        finishLocalInjection();
    }

    bool success = true;
    if (!_options.localOnly) {
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

            _localBuffer = new AudioInjectorLocalBuffer(_audioData);

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

void AudioInjector::deleteLocalBuffer() {
    if (_localBuffer) {
        _localBuffer->stop();
        _localBuffer->deleteLater();
        _localBuffer = nullptr;
    }
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
        qCDebug(audio)  << "AudioInjector::injectNextFrame called but AudioInjector has finished and was not restarted. Returning.";
        return NEXT_FRAME_DELTA_ERROR_OR_FINISHED;
    }

    // if we haven't setup the packet to send then do so now
    static int loopbackOptionOffset = -1;
    static int positionOptionOffset = -1;
    static int volumeOptionOffset = -1;
    static int audioDataOffset = -1;

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
            audioPacketStream << QUuid::createUuid();

            // pack the stereo/mono type of the stream
            audioPacketStream << _options.stereo;

            // pack the flag for loopback, if requested
            loopbackOptionOffset = _currentPacket->pos();
            uchar loopbackFlag = (_localAudioInterface && _localAudioInterface->shouldLoopbackInjectors());
            audioPacketStream << loopbackFlag;

            // pack the position for injected audio
            positionOptionOffset = _currentPacket->pos();
            audioPacketStream.writeRawData(reinterpret_cast<const char*>(&_options.position),
                                           sizeof(_options.position));

            // pack our orientation for injected audio
            audioPacketStream.writeRawData(reinterpret_cast<const char*>(&_options.orientation),
                                           sizeof(_options.orientation));

            audioPacketStream.writeRawData(reinterpret_cast<const char*>(&_options.position),
                sizeof(_options.position));
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
            audioPacketStream << _options.ignorePenumbra;

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
    _currentPacket->writePrimitive(_options.position);
    _currentPacket->writePrimitive(_options.orientation);

    quint8 volume = packFloatGainToByte(_options.volume);
    _currentPacket->seek(volumeOptionOffset);
    _currentPacket->writePrimitive(volume);

    _currentPacket->seek(audioDataOffset);

    // This code is copying bytes from the _sound directly into the packet, handling looping appropriately.
    // Might be a reasonable place to do the encode step here.
    QByteArray decodedAudio;

    int totalBytesLeftToCopy = (_options.stereo ? 2 : 1) * AudioConstants::NETWORK_FRAME_BYTES_PER_CHANNEL;
    if (!_options.loop) {
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
    _loudness = 0.0f;
    for (int i = 0; i < samplesLeftToCopy; ++i) {
        auto index = (currentSample + i) % _audioData->getNumSamples();
        auto sample = samples[index];
        samplesOut[i] = sample;
        _loudness += abs(sample) / (AudioConstants::MAX_SAMPLE_VALUE / 2.0f);
    }
    _loudness /= (float)samplesLeftToCopy;
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

    if (_currentSendOffset == 0 && !_options.loop) {
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
        _currentSendOffset = _nextFrame * AudioConstants::NETWORK_FRAME_BYTES_PER_CHANNEL * (_options.stereo ? 2 : 1) % _audioData->getNumBytes();
    }

    int64_t playNextFrameAt = ++_nextFrame * AudioConstants::NETWORK_FRAME_USECS;

    return std::max(INT64_C(0), playNextFrameAt - currentTime);
}

void AudioInjector::stop() {
    // trigger a call on the injector's thread to change state to finished
    QMetaObject::invokeMethod(this, "finish");
}

void AudioInjector::triggerDeleteAfterFinish() {
    // make sure this fires on the AudioInjector thread
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "triggerDeleteAfterFinish", Qt::QueuedConnection);
        return;
    }

    if (stateHas(AudioInjectorState::Finished)) {
        stop();
    } else {
        _state |= AudioInjectorState::PendingDelete;
    }
}

AudioInjectorPointer AudioInjector::playSoundAndDelete(SharedSoundPointer sound, const AudioInjectorOptions& options) {
    AudioInjectorPointer injector = playSound(sound, options);

    if (injector) {
        injector->_state |= AudioInjectorState::PendingDelete;
    }

    return injector;
}


AudioInjectorPointer AudioInjector::playSound(SharedSoundPointer sound,  const AudioInjectorOptions& options) {
    if (!sound || !sound->isReady()) {
        return AudioInjectorPointer();
    }

    if (options.pitch == 1.0f) {

        AudioInjectorPointer injector = AudioInjectorPointer::create(sound, options);

        if (!injector->inject(&AudioInjectorManager::threadInjector)) {
            qWarning() << "AudioInjector::playSound failed to thread injector";
        }
        return injector;

    } else {
        using AudioConstants::AudioSample;
        using AudioConstants::SAMPLE_RATE;
        const int standardRate = SAMPLE_RATE;
        // limit to 4 octaves
        const int pitch = glm::clamp(options.pitch, 1 / 16.0f, 16.0f);
        const int resampledRate = SAMPLE_RATE / pitch;

        auto audioData = sound->getAudioData();
        auto numChannels = audioData->getNumChannels();
        auto numFrames = audioData->getNumFrames();

        AudioSRC resampler(standardRate, resampledRate, numChannels);

        // create a resampled buffer that is guaranteed to be large enough
        const int maxOutputFrames = resampler.getMaxOutput(numFrames);
        const int maxOutputSize = maxOutputFrames * numChannels * sizeof(AudioSample);
        QByteArray resampledBuffer(maxOutputSize, '\0');
        auto bufferPtr = reinterpret_cast<AudioSample*>(resampledBuffer.data());

        resampler.render(audioData->data(), bufferPtr, numFrames);

        int numSamples = maxOutputFrames * numChannels;
        auto newAudioData = AudioData::make(numSamples, numChannels, bufferPtr);

        AudioInjectorPointer injector = AudioInjectorPointer::create(newAudioData, options);

        if (!injector->inject(&AudioInjectorManager::threadInjector)) {
            qWarning() << "AudioInjector::playSound failed to thread pitch-shifted injector";
        }
        return injector;
    }
}

AudioInjectorPointer AudioInjector::playSoundAndDelete(AudioDataPointer audioData, const AudioInjectorOptions& options) {
    AudioInjectorPointer injector = playSound(audioData, options);

    if (injector) {
        injector->_state |= AudioInjectorState::PendingDelete;
    }

    return injector;
}

AudioInjectorPointer AudioInjector::playSound(AudioDataPointer audioData, const AudioInjectorOptions& options) {
    if (options.pitch == 1.0f) {
        AudioInjectorPointer injector = AudioInjectorPointer::create(audioData, options);

        if (!injector->inject(&AudioInjectorManager::threadInjector)) {
            qWarning() << "AudioInjector::playSound failed to thread pitch-shifted injector";
        }
        return injector;
    } else {
        using AudioConstants::AudioSample;
        using AudioConstants::SAMPLE_RATE;
        const int standardRate = SAMPLE_RATE;
        // limit to 4 octaves
        const int pitch = glm::clamp(options.pitch, 1 / 16.0f, 16.0f);
        const int resampledRate = SAMPLE_RATE / pitch;

        auto numChannels = audioData->getNumChannels();
        auto numFrames = audioData->getNumFrames();

        AudioSRC resampler(standardRate, resampledRate, numChannels);

        // create a resampled buffer that is guaranteed to be large enough
        const int maxOutputFrames = resampler.getMaxOutput(numFrames);
        const int maxOutputSize = maxOutputFrames * numChannels * sizeof(AudioSample);
        QByteArray resampledBuffer(maxOutputSize, '\0');
        auto bufferPtr = reinterpret_cast<AudioSample*>(resampledBuffer.data());

        resampler.render(audioData->data(), bufferPtr, numFrames);

        int numSamples = maxOutputFrames * numChannels;
        auto newAudioData = AudioData::make(numSamples, numChannels, bufferPtr);

        return  AudioInjector::playSound(newAudioData, options);
    }
}