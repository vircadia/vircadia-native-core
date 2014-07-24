//
//  InjectedAudioRingBuffer.h
//  libraries/audio/src
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_InjectedAudioRingBuffer_h
#define hifi_InjectedAudioRingBuffer_h

#include <QtCore/QUuid>

#include "PositionalAudioRingBuffer.h"

class InjectedAudioRingBuffer : public PositionalAudioRingBuffer {
public:
    InjectedAudioRingBuffer(const QUuid& streamIdentifier = QUuid(), bool dynamicJitterBuffer = false);

    float getRadius() const { return _radius; }
    float getAttenuationRatio() const { return _attenuationRatio; }

    QUuid getStreamIdentifier() const { return _streamIdentifier; }

private:
    // disallow copying of InjectedAudioRingBuffer objects
    InjectedAudioRingBuffer(const InjectedAudioRingBuffer&);
    InjectedAudioRingBuffer& operator= (const InjectedAudioRingBuffer&);

    AudioStreamStats getAudioStreamStats() const;
    int parseStreamProperties(PacketType type, const QByteArray& packetAfterSeqNum, int& numAudioSamples);
    int parseAudioData(PacketType type, const QByteArray& packetAfterStreamProperties, int numAudioSamples);

    const QUuid _streamIdentifier;
    float _radius;
    float _attenuationRatio;
};

#endif // hifi_InjectedAudioRingBuffer_h
