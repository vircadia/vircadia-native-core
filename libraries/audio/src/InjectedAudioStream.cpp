//
//  InjectedAudioStream.cpp
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

#include "InjectedAudioStream.h"

InjectedAudioStream::InjectedAudioStream(const QUuid& streamIdentifier, const InboundAudioStream::Settings& settings) :
    PositionalAudioStream(PositionalAudioStream::Injector, false, settings),
    _streamIdentifier(streamIdentifier),
    _radius(0.0f),
    _attenuationRatio(0)
{

}

const uchar MAX_INJECTOR_VOLUME = 255;

int InjectedAudioStream::parseStreamProperties(PacketType type, const QByteArray& packetAfterSeqNum, int& numAudioSamples) {
    // setup a data stream to read from this packet
    QDataStream packetStream(packetAfterSeqNum);

    // skip the stream identifier
    packetStream.skipRawData(NUM_BYTES_RFC4122_UUID);
    
    packetStream >> _isStereo;

    // pull the loopback flag and set our boolean
    uchar shouldLoopback;
    packetStream >> shouldLoopback;
    _shouldLoopbackForNode = (shouldLoopback == 1);

    // use parsePositionalData in parent PostionalAudioRingBuffer class to pull common positional data
    packetStream.skipRawData(parsePositionalData(packetAfterSeqNum.mid(packetStream.device()->pos())));

    // pull out the radius for this injected source - if it's zero this is a point source
    packetStream >> _radius;

    quint8 attenuationByte = 0;
    packetStream >> attenuationByte;
    _attenuationRatio = attenuationByte / (float)MAX_INJECTOR_VOLUME;

    int numAudioBytes = packetAfterSeqNum.size() - packetStream.device()->pos();
    numAudioSamples = numAudioBytes / sizeof(int16_t);

    return packetStream.device()->pos();
}

AudioStreamStats InjectedAudioStream::getAudioStreamStats() const {
    AudioStreamStats streamStats = PositionalAudioStream::getAudioStreamStats();
    streamStats._streamIdentifier = _streamIdentifier;
    return streamStats;
}
