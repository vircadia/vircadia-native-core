//
//  AudioMixerClientData.cpp
//  hifi
//
//  Created by Stephen Birarda on 10/18/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <PacketHeaders.h>
#include <UUID.h>

#include "InjectedAudioRingBuffer.h"

#include "AudioMixerClientData.h"

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

int AudioMixerClientData::parseData(unsigned char* packetData, int numBytes) {
    if (packetData[0] == PACKET_TYPE_MICROPHONE_AUDIO_WITH_ECHO
        || packetData[0] == PACKET_TYPE_MICROPHONE_AUDIO_NO_ECHO) {
        
        // grab the AvatarAudioRingBuffer from the vector (or create it if it doesn't exist)
        AvatarAudioRingBuffer* avatarRingBuffer = getAvatarAudioRingBuffer();
        
        if (!avatarRingBuffer) {
            // we don't have an AvatarAudioRingBuffer yet, so add it
            avatarRingBuffer = new AvatarAudioRingBuffer();
            _ringBuffers.push_back(avatarRingBuffer);
        }
        
        // ask the AvatarAudioRingBuffer instance to parse the data
        avatarRingBuffer->parseData(packetData, numBytes);
    } else {
        // this is injected audio
        
        // grab the stream identifier for this injected audio
        QByteArray rfcUUID = QByteArray((char*) packetData +  numBytesForPacketHeader(packetData) + NUM_BYTES_RFC4122_UUID,
                                        NUM_BYTES_RFC4122_UUID);
        QUuid streamIdentifier = QUuid::fromRfc4122(rfcUUID);
        
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
        
        matchingInjectedRingBuffer->parseData(packetData, numBytes);
    }
    
    return 0;
}

void AudioMixerClientData::checkBuffersBeforeFrameSend(int jitterBufferLengthSamples) {
    for (int i = 0; i < _ringBuffers.size(); i++) {
        if (_ringBuffers[i]->shouldBeAddedToMix(jitterBufferLengthSamples)) {
            // this is a ring buffer that is ready to go
            // set its flag so we know to push its buffer when all is said and done
            _ringBuffers[i]->setWillBeAddedToMix(true);
        }
    }
}

void AudioMixerClientData::pushBuffersAfterFrameSend() {
    for (int i = 0; i < _ringBuffers.size(); i++) {
        // this was a used buffer, push the output pointer forwards
        PositionalAudioRingBuffer* audioBuffer = _ringBuffers[i];
        
        if (audioBuffer->willBeAddedToMix()) {
            audioBuffer->shiftReadPosition(NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL);
            
            audioBuffer->setWillBeAddedToMix(false);
        } else if (audioBuffer->getType() == PositionalAudioRingBuffer::Injector
                   && audioBuffer->hasStarted() && audioBuffer->isStarved()) {
            // this is an empty audio buffer that has starved, safe to delete
            delete audioBuffer;
            _ringBuffers.erase(_ringBuffers.begin() + i);
        }
    }
}
