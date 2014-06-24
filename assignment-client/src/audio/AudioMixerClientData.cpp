//
//  AudioMixerClientData.cpp
//  assignment-client/src/audio
//
//  Created by Stephen Birarda on 10/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDebug>

#include <PacketHeaders.h>
#include <UUID.h>

#include "InjectedAudioRingBuffer.h"
#include "SharedUtil.h"

#include "AudioMixerClientData.h"

AudioMixerClientData::AudioMixerClientData() :
    _ringBuffers()
{
    
}

AudioMixerClientData::~AudioMixerClientData() {
    for (int i = 0; i < _ringBuffers.size(); i++) {
        // delete this attached PositionalAudioRingBuffer
        delete _ringBuffers[i];
    }
}

AvatarAudioRingBuffer* AudioMixerClientData::getAvatarAudioRingBuffer() const {
    for (int i = 0; i < _ringBuffers.size(); i++) {
        if (_ringBuffers[i]->getType() == PositionalAudioRingBuffer::Microphone) {
            return (AvatarAudioRingBuffer*) _ringBuffers[i];
        }
    }

    // no AvatarAudioRingBuffer found - return NULL
    return NULL;
}

int AudioMixerClientData::parseData(const QByteArray& packet) {
    PacketType packetType = packetTypeForPacket(packet);
    if (packetType == PacketTypeMicrophoneAudioWithEcho
        || packetType == PacketTypeMicrophoneAudioNoEcho
        || packetType == PacketTypeSilentAudioFrame) {

        // grab the AvatarAudioRingBuffer from the vector (or create it if it doesn't exist)
        AvatarAudioRingBuffer* avatarRingBuffer = getAvatarAudioRingBuffer();
        
        // read the first byte after the header to see if this is a stereo or mono buffer
        quint8 channelFlag = packet.at(numBytesForPacketHeader(packet));
        bool isStereo = channelFlag == 1;
        
        if (avatarRingBuffer && avatarRingBuffer->isStereo() != isStereo) {
            // there's a mismatch in the buffer channels for the incoming and current buffer
            // so delete our current buffer and create a new one
            _ringBuffers.removeOne(avatarRingBuffer);
            avatarRingBuffer->deleteLater();
            avatarRingBuffer = NULL;
        }

        if (!avatarRingBuffer) {
            // we don't have an AvatarAudioRingBuffer yet, so add it
            avatarRingBuffer = new AvatarAudioRingBuffer(isStereo);
            _ringBuffers.push_back(avatarRingBuffer);
        }

        // ask the AvatarAudioRingBuffer instance to parse the data
        avatarRingBuffer->parseData(packet);
    } else {
        // this is injected audio

        // grab the stream identifier for this injected audio
        QUuid streamIdentifier = QUuid::fromRfc4122(packet.mid(numBytesForPacketHeader(packet), NUM_BYTES_RFC4122_UUID));

        InjectedAudioRingBuffer* matchingInjectedRingBuffer = NULL;

        for (int i = 0; i < _ringBuffers.size(); i++) {
            if (_ringBuffers[i]->getType() == PositionalAudioRingBuffer::Injector
                && ((InjectedAudioRingBuffer*) _ringBuffers[i])->getStreamIdentifier() == streamIdentifier) {
                matchingInjectedRingBuffer = (InjectedAudioRingBuffer*) _ringBuffers[i];
            }
        }

        if (!matchingInjectedRingBuffer) {
            // we don't have a matching injected audio ring buffer, so add it
            matchingInjectedRingBuffer = new InjectedAudioRingBuffer(streamIdentifier);
            _ringBuffers.push_back(matchingInjectedRingBuffer);
        }

        matchingInjectedRingBuffer->parseData(packet);
    }

    return 0;
}

void AudioMixerClientData::checkBuffersBeforeFrameSend(AABox* checkSourceZone, AABox* listenerZone) {
    for (int i = 0; i < _ringBuffers.size(); i++) {
        if (_ringBuffers[i]->shouldBeAddedToMix()) {
            // this is a ring buffer that is ready to go
            // set its flag so we know to push its buffer when all is said and done
            _ringBuffers[i]->setWillBeAddedToMix(true);
            
            // calculate the average loudness for the next NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL
            // that would be mixed in
            _ringBuffers[i]->updateNextOutputTrailingLoudness();
            
            if (checkSourceZone && checkSourceZone->contains(_ringBuffers[i]->getPosition())) {
                _ringBuffers[i]->setListenerUnattenuatedZone(listenerZone);
            } else {
                _ringBuffers[i]->setListenerUnattenuatedZone(NULL);
            }
        }
    }
}

void AudioMixerClientData::pushBuffersAfterFrameSend() {

    QList<PositionalAudioRingBuffer*>::iterator i = _ringBuffers.begin();
    while (i != _ringBuffers.end()) {
        // this was a used buffer, push the output pointer forwards
        PositionalAudioRingBuffer* audioBuffer = *i;

        if (audioBuffer->willBeAddedToMix()) {
            audioBuffer->shiftReadPosition(audioBuffer->getSamplesPerFrame());
            audioBuffer->setWillBeAddedToMix(false);
        } else if (audioBuffer->getType() == PositionalAudioRingBuffer::Injector
                   && audioBuffer->hasStarted() && audioBuffer->isStarved()) {
            // this is an empty audio buffer that has starved, safe to delete
            delete audioBuffer;
            i = _ringBuffers.erase(i);
            continue;
        }
        i++;
    }
}

void AudioMixerClientData::calculateJitterBuffersStats(AudioMixerJitterBuffersStats& stats) const {
    int avatarJitterBufferFrames = 0;
    int maxJitterBufferFrames = 0;
    int sumJitterBufferFrames = 0;

    for (int i = 0; i < _ringBuffers.size(); i++) {

        int bufferJitterFrames = _ringBuffers[i]->getCurrentJitterBufferFrames();
        if (_ringBuffers[i]->getType() == PositionalAudioRingBuffer::Microphone) {
            avatarJitterBufferFrames = bufferJitterFrames;
        }

        if (bufferJitterFrames > maxJitterBufferFrames) {
            maxJitterBufferFrames = bufferJitterFrames;
        }

        sumJitterBufferFrames += bufferJitterFrames;
    }

    stats.avatarJitterBufferFrames = avatarJitterBufferFrames;
    stats.maxJitterBufferFrames = maxJitterBufferFrames;
    stats.avgJitterBufferFrames = (float)sumJitterBufferFrames / (float)_ringBuffers.size();
}

QString AudioMixerClientData::getJitterBufferStats() const {
    QString result;
    AvatarAudioRingBuffer* avatarRingBuffer = getAvatarAudioRingBuffer();
    if (avatarRingBuffer) {
        int desiredJitterBuffer = avatarRingBuffer->getDesiredJitterBufferFrames();
        int currentJitterBuffer = avatarRingBuffer->getCurrentJitterBufferFrames();
        int samplesAvailable = avatarRingBuffer->samplesAvailable();
        int framesAvailable = (samplesAvailable / avatarRingBuffer->getSamplesPerFrame());
        result += "mic.desired:" + QString::number(desiredJitterBuffer) 
                    + " current:" + QString::number(currentJitterBuffer)
                    + " available:" + QString::number(framesAvailable)
                    + " samples:" + QString::number(samplesAvailable);
    } else {
        result = "mic unknown";
    }

    for (int i = 0; i < _ringBuffers.size(); i++) {
        if (_ringBuffers[i]->getType() == PositionalAudioRingBuffer::Injector) {
            int desiredJitterBuffer = _ringBuffers[i]->getDesiredJitterBufferFrames();
            int currentJitterBuffer = _ringBuffers[i]->getCurrentJitterBufferFrames();
            int samplesAvailable = _ringBuffers[i]->samplesAvailable();
            int framesAvailable = (samplesAvailable / _ringBuffers[i]->getSamplesPerFrame());
            result += "| injected["+QString::number(i)+"].desired:" + QString::number(desiredJitterBuffer) 
                    + " current:" + QString::number(currentJitterBuffer)
                    + " available:" + QString::number(framesAvailable)
                    + " samples:" + QString::number(samplesAvailable);
        }
    }
    return result;
}
