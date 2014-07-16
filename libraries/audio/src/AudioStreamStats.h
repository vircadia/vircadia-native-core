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
#include "SequenceNumberStats.h"

class AudioStreamStats {
public:
    AudioStreamStats()
        : _streamType(PositionalAudioRingBuffer::Microphone),
        _streamIdentifier(),
        _timeGapMin(0),
        _timeGapMax(0),
        _timeGapAverage(0.0f),
        _timeGapWindowMin(0),
        _timeGapWindowMax(0),
        _timeGapWindowAverage(0.0f),
        _ringBufferFramesAvailable(0),
        _ringBufferFramesAvailableAverage(0),
        _ringBufferDesiredJitterBufferFrames(0),
        _ringBufferStarveCount(0),
        _ringBufferConsecutiveNotMixedCount(0),
        _ringBufferOverflowCount(0),
        _ringBufferSilentFramesDropped(0),
        _packetStreamStats(),
        _packetStreamWindowStats()
    {}

    PositionalAudioRingBuffer::Type _streamType;
    QUuid _streamIdentifier;

    quint64 _timeGapMin;
    quint64 _timeGapMax;
    float _timeGapAverage;
    quint64 _timeGapWindowMin;
    quint64 _timeGapWindowMax;
    float _timeGapWindowAverage;

    quint32 _ringBufferFramesAvailable;
    quint16 _ringBufferFramesAvailableAverage;
    quint16 _ringBufferDesiredJitterBufferFrames;
    quint32 _ringBufferStarveCount;
    quint32 _ringBufferConsecutiveNotMixedCount;
    quint32 _ringBufferOverflowCount;
    quint32 _ringBufferSilentFramesDropped;

    PacketStreamStats _packetStreamStats;
    PacketStreamStats _packetStreamWindowStats;
};

#endif  // hifi_AudioStreamStats_h
