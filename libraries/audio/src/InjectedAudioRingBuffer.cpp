//
//  InjectedAudioRingBuffer.cpp
//  libraries/audio/src
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <cstring>

#include <QtCore/QDataStream>
#include <QtCore/qdebug.h>

#include <PacketHeaders.h>
#include <UUID.h>

#include "InjectedAudioRingBuffer.h"

InjectedAudioRingBuffer::InjectedAudioRingBuffer(const QUuid& streamIdentifier, bool dynamicJitterBuffer) :
    PositionalAudioRingBuffer(PositionalAudioRingBuffer::Injector, false, dynamicJitterBuffer),
    _streamIdentifier(streamIdentifier),
    _radius(0.0f),
    _attenuationRatio(0)
{
    
}

const uchar MAX_INJECTOR_VOLUME = 255;

int InjectedAudioRingBuffer::parseData(const QByteArray& packet) {
    timeGapStatsFrameReceived();
    updateDesiredJitterBufferFrames();

    // setup a data stream to read from this packet
    QDataStream packetStream(packet);
    packetStream.skipRawData(numBytesForPacketHeader(packet));
    
    // push past the sequence number
    packetStream.skipRawData(sizeof(quint16));

    // push past the stream identifier
    packetStream.skipRawData(NUM_BYTES_RFC4122_UUID);
    
    // pull the loopback flag and set our boolean
    uchar shouldLoopback;
    packetStream >> shouldLoopback;
    _shouldLoopbackForNode = (shouldLoopback == 1);
    
    // use parsePositionalData in parent PostionalAudioRingBuffer class to pull common positional data
    packetStream.skipRawData(parsePositionalData(packet.mid(packetStream.device()->pos())));
    
    // pull out the radius for this injected source - if it's zero this is a point source
    packetStream >> _radius;
    
    quint8 attenuationByte = 0;
    packetStream >> attenuationByte;
    _attenuationRatio = attenuationByte / (float) MAX_INJECTOR_VOLUME;
    
    packetStream.skipRawData(writeData(packet.data() + packetStream.device()->pos(),
                                       packet.size() - packetStream.device()->pos()));
    
    return packetStream.device()->pos();
}
