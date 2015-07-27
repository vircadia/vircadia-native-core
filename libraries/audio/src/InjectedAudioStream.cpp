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

#include <udt/PacketHeaders.h>
#include <UUID.h>

#include "InjectedAudioStream.h"

InjectedAudioStream::InjectedAudioStream(const QUuid& streamIdentifier, const bool isStereo, const InboundAudioStream::Settings& settings) :
    PositionalAudioStream(PositionalAudioStream::Injector, isStereo, settings),
    _streamIdentifier(streamIdentifier),
    _radius(0.0f),
    _attenuationRatio(0)
{

}

const uchar MAX_INJECTOR_VOLUME = 255;

int InjectedAudioStream::parseStreamProperties(PacketType type,
                                               const QByteArray& packetAfterSeqNum,
                                               int& numAudioSamples) {
    // setup a data stream to read from this packet
    QDataStream packetStream(packetAfterSeqNum);

    // skip the stream identifier
    packetStream.skipRawData(NUM_BYTES_RFC4122_UUID);
    
    // read the channel flag
    bool isStereo;
    packetStream >> isStereo;
    
    // if isStereo value has changed, restart the ring buffer with new frame size
    if (isStereo != _isStereo) {
        _ringBuffer.resizeForFrameSize(isStereo
                                       ? AudioConstants::NETWORK_FRAME_SAMPLES_STEREO
                                       : AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);
        _isStereo = isStereo;
    }

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
    
    packetStream >> _ignorePenumbra;
    
    int numAudioBytes = packetAfterSeqNum.size() - packetStream.device()->pos();
    numAudioSamples = numAudioBytes / sizeof(int16_t);

    return packetStream.device()->pos();
}

AudioStreamStats InjectedAudioStream::getAudioStreamStats() const {
    AudioStreamStats streamStats = PositionalAudioStream::getAudioStreamStats();
    streamStats._streamIdentifier = _streamIdentifier;
    return streamStats;
}
