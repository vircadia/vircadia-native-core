//
//  PositionalAudioRingBuffer.cpp
//  libraries/audio/src
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <cstring>

#include <glm/detail/func_common.hpp>
#include <QtCore/QDataStream>

#include <Node.h>
#include <PacketHeaders.h>
#include <UUID.h>

#include "PositionalAudioRingBuffer.h"

PositionalAudioRingBuffer::PositionalAudioRingBuffer(PositionalAudioRingBuffer::Type type, bool isStereo) :
    AudioRingBuffer(isStereo ? NETWORK_BUFFER_LENGTH_SAMPLES_STEREO : NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL),
    _type(type),
    _position(0.0f, 0.0f, 0.0f),
    _orientation(0.0f, 0.0f, 0.0f, 0.0f),
    _willBeAddedToMix(false),
    _shouldLoopbackForNode(false),
    _shouldOutputStarveDebug(true),
    _isStereo(isStereo),
    _listenerUnattenuatedZone(NULL)
{

}

int PositionalAudioRingBuffer::parseData(const QByteArray& packet) {
    
    // skip the packet header (includes the source UUID)
    int readBytes = numBytesForPacketHeader(packet);
    
    // hop over the channel flag that has already been read in AudioMixerClientData
    readBytes += sizeof(quint8);
    // read the positional data
    readBytes += parsePositionalData(packet.mid(readBytes));
   
    if (packetTypeForPacket(packet) == PacketTypeSilentAudioFrame) {
        // this source had no audio to send us, but this counts as a packet
        // write silence equivalent to the number of silent samples they just sent us
        int16_t numSilentSamples;
        
        memcpy(&numSilentSamples, packet.data() + readBytes, sizeof(int16_t));
        
        readBytes += sizeof(int16_t);
        
        if (numSilentSamples > 0) {
            addSilentFrame(numSilentSamples);
        }
    } else {
        // there is audio data to read
        readBytes += writeData(packet.data() + readBytes, packet.size() - readBytes);
    }
    
    return readBytes;
}

int PositionalAudioRingBuffer::parsePositionalData(const QByteArray& positionalByteArray) {
    QDataStream packetStream(positionalByteArray);
    
    packetStream.readRawData(reinterpret_cast<char*>(&_position), sizeof(_position));
    packetStream.readRawData(reinterpret_cast<char*>(&_orientation), sizeof(_orientation));

    // if this node sent us a NaN for first float in orientation then don't consider this good audio and bail
    if (glm::isnan(_orientation.x)) {
        reset();
        return 0;
    }

    return packetStream.device()->pos();
}

void PositionalAudioRingBuffer::updateNextOutputTrailingLoudness() {
    // ForBoundarySamples means that we expect the number of samples not to roll of the end of the ring buffer
    float nextLoudness = 0;
    
    for (int i = 0; i < _numFrameSamples; ++i) {
        nextLoudness += fabsf(_nextOutput[i]);
    }
    
    nextLoudness /= _numFrameSamples;
    nextLoudness /= MAX_SAMPLE_VALUE;
    
    const int TRAILING_AVERAGE_FRAMES = 100;
    const float CURRENT_FRAME_RATIO = 1.0f / TRAILING_AVERAGE_FRAMES;
    const float PREVIOUS_FRAMES_RATIO = 1.0f - CURRENT_FRAME_RATIO;
    const float LOUDNESS_EPSILON = 0.000001f;
    
    if (nextLoudness >= _nextOutputTrailingLoudness) {
        _nextOutputTrailingLoudness = nextLoudness;
    } else {
        _nextOutputTrailingLoudness = (_nextOutputTrailingLoudness * PREVIOUS_FRAMES_RATIO) + (CURRENT_FRAME_RATIO * nextLoudness);
        
        if (_nextOutputTrailingLoudness < LOUDNESS_EPSILON) {
            _nextOutputTrailingLoudness = 0;
        }
    }
}

bool PositionalAudioRingBuffer::shouldBeAddedToMix(int numJitterBufferSamples) {
    if (!isNotStarvedOrHasMinimumSamples(NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL + numJitterBufferSamples)) {
        if (_shouldOutputStarveDebug) {
            _shouldOutputStarveDebug = false;
        }
        
        return false;
    } else if (samplesAvailable() < NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL) {
        _isStarved = true;
        
        // reset our _shouldOutputStarveDebug to true so the next is printed
        _shouldOutputStarveDebug = true;
        
        return false;
    } else {
        // good buffer, add this to the mix
        _isStarved = false;

        // since we've read data from ring buffer at least once - we've started
        _hasStarted = true;

        return true;
    }

    return false;
}
