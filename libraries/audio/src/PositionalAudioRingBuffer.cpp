//
//  PositionalAudioRingBuffer.cpp
//  hifi
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <cstring>

#include <QtCore/QDataStream>

#include <Node.h>
#include <PacketHeaders.h>
#include <UUID.h>

#include "PositionalAudioRingBuffer.h"

#ifdef _WIN32
int isnan(double value) { return _isnan(value); }
#else
int isnan(double value) { return std::isnan(value); }
#endif

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
    QDataStream packetStream(packet);
    
    // skip the packet header (includes the source UUID)
    packetStream.skipRawData(numBytesForPacketHeader(packet));
    
    packetStream.skipRawData(parsePositionalData(packet.mid(packetStream.device()->pos())));
   
    if (packetTypeForPacket(packet) == PacketTypeSilentAudioListener) {
        // this source had no audio to send us, but this counts as a packet
        // write silence equivalent to the number of silent samples they just sent us
        int16_t numSilentSamples;
        packetStream >> numSilentSamples;
        addSilentFrame(numSilentSamples);
    } else {
        // there is audio data to read
        packetStream.skipRawData(writeData(packet.data() + packetStream.device()->pos(),
                                           packet.size() - packetStream.device()->pos()));
    }
    

    return packetStream.device()->pos();
}

int PositionalAudioRingBuffer::parsePositionalData(const QByteArray& positionalByteArray) {
    QDataStream packetStream(positionalByteArray);
    
    packetStream.readRawData(reinterpret_cast<char*>(&_position), sizeof(_position));
    packetStream.readRawData(reinterpret_cast<char*>(&_orientation), sizeof(_orientation));

    // if this node sent us a NaN for first float in orientation then don't consider this good audio and bail
    if (isnan(_orientation.x)) {
        reset();
        return 0;
    }

    return packetStream.device()->pos();
}

bool PositionalAudioRingBuffer::shouldBeAddedToMix(int numJitterBufferSamples) {
    if (!isNotStarvedOrHasMinimumSamples(NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL + numJitterBufferSamples)) {
        if (_shouldOutputStarveDebug) {
            qDebug() << "Starved and do not have minimum samples to start. Buffer held back.";
            _shouldOutputStarveDebug = false;
        }
        
        return false;
    } else if (samplesAvailable() < NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL) {
        qDebug() << "Do not have number of samples needed for interval. Buffer starved.";
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
