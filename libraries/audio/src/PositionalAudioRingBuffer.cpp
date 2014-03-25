//
//  PositionalAudioRingBuffer.cpp
//  hifi
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <cstring>

#include <glm/detail/func_common.hpp>
#include <QtCore/QDataStream>

#include <Node.h>
#include <PacketHeaders.h>
#include <UUID.h>

#include "PositionalAudioRingBuffer.h"

PositionalAudioRingBuffer::PositionalAudioRingBuffer(PositionalAudioRingBuffer::Type type) :
    AudioRingBuffer(NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL),
    _type(type),
    _position(0.0f, 0.0f, 0.0f),
    _orientation(0.0f, 0.0f, 0.0f, 0.0f),
    _willBeAddedToMix(false),
    _shouldLoopbackForNode(false),
    _shouldOutputStarveDebug(true)
{

}

PositionalAudioRingBuffer::~PositionalAudioRingBuffer() {
}

int PositionalAudioRingBuffer::parseData(const QByteArray& packet) {
    
    // skip the packet header (includes the source UUID)
    int readBytes = numBytesForPacketHeader(packet);
    
    readBytes += parsePositionalData(packet.mid(readBytes));
   
    if (packetTypeForPacket(packet) == PacketTypeSilentAudioFrame) {
        // this source had no audio to send us, but this counts as a packet
        // write silence equivalent to the number of silent samples they just sent us
        int16_t numSilentSamples;
        
        memcpy(&numSilentSamples, packet.data() + readBytes, sizeof(int16_t));
        
        readBytes += sizeof(int16_t);
        
        addSilentFrame(numSilentSamples);
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
