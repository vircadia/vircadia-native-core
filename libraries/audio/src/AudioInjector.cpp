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

#include "AudioInjector.h"

AudioInjector::AudioInjector(QObject* parent) :
    QObject(parent)
{

}

AudioInjector::AudioInjector(Sound* sound, const AudioInjectorOptions& injectorOptions) :
    _audioData(sound->getByteArray()),
    _options(injectorOptions)
{
    
}

AudioInjector::AudioInjector(const QByteArray& audioData, const AudioInjectorOptions& injectorOptions) :
    _audioData(audioData),
    _options(injectorOptions)
{

}

void AudioInjector::finish() {
    State oldState = std::atomic_exchange(&_state, State::Finished);
    bool shouldDelete = (oldState == State::NotFinishedWithPendingDelete);
    
    emit finished();
    
    if (_localBuffer) {
        _localBuffer->stop();
        _localBuffer->deleteLater();
        _localBuffer = NULL;
    }
    
    if (shouldDelete) {
        // we've been asked to delete after finishing, trigger a queued deleteLater here
        QMetaObject::invokeMethod(this, "deleteLater", Qt::QueuedConnection);
    }
}

void AudioInjector::injectAudio() {
    if (!_hasStarted) {
        _hasStarted = true;
        // check if we need to offset the sound by some number of seconds
        if (_options.secondOffset > 0.0f) {

            // convert the offset into a number of bytes
            int byteOffset = (int) floorf(AudioConstants::SAMPLE_RATE * _options.secondOffset * (_options.stereo ? 2.0f : 1.0f));
            byteOffset *= sizeof(int16_t);

            _currentSendOffset = byteOffset;
        } else {
            _currentSendOffset = 0;
        }

        if (_options.localOnly) {
            injectLocally();
        } else {
            qDebug() << "Calling inject to mixer from" << QThread::currentThread();
            injectToMixer();
        }
    } else {
        qCDebug(audio) << "AudioInjector::injectAudio called but already started.";
    }
}

void AudioInjector::restart() {
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "restart");
        return;
    }
    
    // reset the current send offset to zero
    _currentSendOffset = 0;
    
    // check our state to decide if we need extra handling for the restart request
    if (!isPlaying()) {
        // we finished playing, need to reset state so we can get going again
        _hasStarted = false;
        _shouldStop = false;
        _state = State::NotFinished;
        
        qDebug() << "Calling inject audio again to restart an injector";
        
        // call inject audio to start injection over again
        injectAudio();
    }
}

void AudioInjector::injectLocally() {
    bool success = false;
    if (_localAudioInterface) {
        if (_audioData.size() > 0) {

            _localBuffer = new AudioInjectorLocalBuffer(_audioData, this);

            _localBuffer->open(QIODevice::ReadOnly);
            _localBuffer->setShouldLoop(_options.loop);
            _localBuffer->setVolume(_options.volume);

            // give our current send position to the local buffer
            _localBuffer->setCurrentOffset(_currentSendOffset);

            success = _localAudioInterface->outputLocalInjector(_options.stereo, this);

            if (!success) {
                qCDebug(audio) << "AudioInjector::injectLocally could not output locally via _localAudioInterface";
            }
        } else {
            qCDebug(audio) << "AudioInjector::injectLocally called without any data in Sound QByteArray";
        }

    } else {
        qCDebug(audio) << "AudioInjector::injectLocally cannot inject locally with no local audio interface present.";
    }

    if (!success) {
        // we never started so we are finished, call our stop method
        stop();
    }

}

const uchar MAX_INJECTOR_VOLUME = 0xFF;

void AudioInjector::injectToMixer() {
    if (_currentSendOffset < 0 ||
        _currentSendOffset >= _audioData.size()) {
        _currentSendOffset = 0;
    }

    auto nodeList = DependencyManager::get<NodeList>();

    // make sure we actually have samples downloaded to inject
    if (_audioData.size()) {

        auto audioPacket = NLPacket::create(PacketType::InjectAudio);

        // setup the packet for injected audio
        QDataStream audioPacketStream(audioPacket.get());

        // pack some placeholder sequence number for now
        audioPacketStream << (quint16) 0;

        // pack stream identifier (a generated UUID)
        audioPacketStream << QUuid::createUuid();

        // pack the stereo/mono type of the stream
        audioPacketStream << _options.stereo;

        // pack the flag for loopback
        uchar loopbackFlag = (uchar) true;
        audioPacketStream << loopbackFlag;

        // pack the position for injected audio
        int positionOptionOffset = audioPacket->pos();
        audioPacketStream.writeRawData(reinterpret_cast<const char*>(&_options.position),
                                  sizeof(_options.position));

        // pack our orientation for injected audio
        audioPacketStream.writeRawData(reinterpret_cast<const char*>(&_options.orientation),
                                  sizeof(_options.orientation));

        // pack zero for radius
        float radius = 0;
        audioPacketStream << radius;

        // pack 255 for attenuation byte
        int volumeOptionOffset = audioPacket->pos();
        quint8 volume = MAX_INJECTOR_VOLUME * _options.volume;
        audioPacketStream << volume;

        audioPacketStream << _options.ignorePenumbra;

        int audioDataOffset = audioPacket->pos();

        QElapsedTimer timer;
        timer.start();
        int nextFrame = 0;

        bool shouldLoop = _options.loop;

        // loop to send off our audio in NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL byte chunks
        quint16 outgoingInjectedAudioSequenceNumber = 0;

        while (_currentSendOffset < _audioData.size() && !_shouldStop) {

            int bytesToCopy = std::min((_options.stereo ? 2 : 1) * AudioConstants::NETWORK_FRAME_BYTES_PER_CHANNEL,
                                       _audioData.size() - _currentSendOffset);

            //  Measure the loudness of this frame
            _loudness = 0.0f;
            for (int i = 0; i < bytesToCopy; i += sizeof(int16_t)) {
                _loudness += abs(*reinterpret_cast<int16_t*>(_audioData.data() + _currentSendOffset + i)) /
                (AudioConstants::MAX_SAMPLE_VALUE / 2.0f);
            }
            _loudness /= (float)(bytesToCopy / sizeof(int16_t));
            
            audioPacket->seek(0);
            
            // pack the sequence number
            audioPacket->writePrimitive(outgoingInjectedAudioSequenceNumber);
            
            audioPacket->seek(positionOptionOffset);
            audioPacket->writePrimitive(_options.position);
            audioPacket->writePrimitive(_options.orientation);

            volume = MAX_INJECTOR_VOLUME * _options.volume;
            audioPacket->seek(volumeOptionOffset);
            audioPacket->writePrimitive(volume);

            audioPacket->seek(audioDataOffset);

            // copy the next NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL bytes to the packet
            audioPacket->write(_audioData.data() + _currentSendOffset, bytesToCopy);

            // set the correct size used for this packet
            audioPacket->setPayloadSize(audioPacket->pos());

            // grab our audio mixer from the NodeList, if it exists
            SharedNodePointer audioMixer = nodeList->soloNodeOfType(NodeType::AudioMixer);
            
            if (audioMixer) {
                // send off this audio packet
                nodeList->sendUnreliablePacket(*audioPacket, *audioMixer);
                outgoingInjectedAudioSequenceNumber++;
            }

            _currentSendOffset += bytesToCopy;

            // send two packets before the first sleep so the mixer can start playback right away

            if (_currentSendOffset != bytesToCopy && _currentSendOffset < _audioData.size()) {

                

                if (_shouldStop) {
                    break;
                }
                
                // not the first packet and not done
                // sleep for the appropriate time
                int usecToSleep = (++nextFrame * AudioConstants::NETWORK_FRAME_USECS) - timer.nsecsElapsed() / 1000;
                
                qDebug() << "AudioInjector" << this << "will sleep on thread" << QThread::currentThread() << "for" << usecToSleep;

                if (usecToSleep > 0) {
                    usleep(usecToSleep);
                }
            }

            if (shouldLoop && _currentSendOffset >= _audioData.size()) {
                _currentSendOffset = 0;
            }
        }
    }

    finish();
}

void AudioInjector::stop() {
    _shouldStop = true;

    if (_options.localOnly) {
        // we're only a local injector, so we can say we are finished right away too
        finish();
    }
}

void AudioInjector::triggerDeleteAfterFinish() {
    auto expectedState = State::NotFinished;
    if (!_state.compare_exchange_strong(expectedState, State::NotFinishedWithPendingDelete)) {
        stopAndDeleteLater();
    }
}

void AudioInjector::stopAndDeleteLater() {
    stop();
    QMetaObject::invokeMethod(this, "deleteLater", Qt::QueuedConnection);
}

AudioInjector* AudioInjector::playSound(const QString& soundUrl, const float volume, const float stretchFactor, const glm::vec3 position) {
    if (soundUrl.isEmpty()) {
        return NULL;
    }
    auto soundCache = DependencyManager::get<SoundCache>();
    if (soundCache.isNull()) {
        return NULL;
    }
    SharedSoundPointer sound = soundCache->getSound(QUrl(soundUrl));
    if (sound.isNull() || !sound->isReady()) {
        return NULL;
    }

    AudioInjectorOptions options;
    options.stereo = sound->isStereo();
    options.position = position;
    options.volume = volume;

    QByteArray samples = sound->getByteArray();
    if (stretchFactor == 1.0f) {
        return playSoundAndDelete(samples, options, NULL);
    }

    const int standardRate = AudioConstants::SAMPLE_RATE;
    const int resampledRate = standardRate * stretchFactor;
    const int channelCount = sound->isStereo() ? 2 : 1;

    AudioSRC resampler(standardRate, resampledRate, channelCount);

    const int nInputFrames = samples.size() / (channelCount * sizeof(int16_t));
    const int maxOutputFrames = resampler.getMaxOutput(nInputFrames);
    QByteArray resampled(maxOutputFrames * channelCount * sizeof(int16_t), '\0');

    int nOutputFrames = resampler.render(reinterpret_cast<const int16_t*>(samples.data()),
                                         reinterpret_cast<int16_t*>(resampled.data()),
                                         nInputFrames);

    Q_UNUSED(nOutputFrames);
    return playSoundAndDelete(resampled, options, NULL);
}

AudioInjector* AudioInjector::playSoundAndDelete(const QByteArray& buffer, const AudioInjectorOptions options, AbstractAudioInterface* localInterface) {
    AudioInjector* sound = playSound(buffer, options, localInterface);
    sound->_state = AudioInjector::State::NotFinishedWithPendingDelete;
    return sound;
}


AudioInjector* AudioInjector::playSound(const QByteArray& buffer, const AudioInjectorOptions options, AbstractAudioInterface* localInterface) {
    AudioInjector* injector = new AudioInjector(buffer, options);
    injector->setLocalAudioInterface(localInterface);
    
    // grab the AudioInjectorManager
    auto injectorManager = DependencyManager::get<AudioInjectorManager>();
    
    // attempt to thread the new injector
    if (injectorManager->threadInjector(injector)) {
        // call inject audio on the correct thread
        QMetaObject::invokeMethod(injector, "injectAudio", Qt::QueuedConnection);
        
        return injector;
    } else {
        // we failed to thread the new injector (we are at the max number of injector threads)
        return nullptr;
    }
}
