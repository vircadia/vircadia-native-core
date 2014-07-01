//
//  AudioStreamStats.h
//  libraries/audio/src
//
//  Created by Yixin Wang on 6/25/2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioStreamStats_h
#define hifi_AudioStreamStats_h

#include "PositionalAudioRingBuffer.h"

class AudioStreamStats {
public:
    AudioStreamStats()
        : _streamType(PositionalAudioRingBuffer::Microphone),
        _streamIdentifier(),
        _jitterBufferFrames(0),
        _packetsReceived(0),
        _packetsUnreasonable(0),
        _packetsEarly(0),
        _packetsLate(0),
        _packetsLost(0),
        _packetsRecovered(0),
        _packetsDuplicate(0)
    {}

    PositionalAudioRingBuffer::Type _streamType;
    QUuid _streamIdentifier;

    quint16 _jitterBufferFrames;

    quint32 _packetsReceived;
    quint32 _packetsUnreasonable;
    quint32 _packetsEarly;
    quint32 _packetsLate;
    quint32 _packetsLost;
    quint32 _packetsRecovered;
    quint32 _packetsDuplicate;
};

#endif  // hifi_AudioStreamStats_h
