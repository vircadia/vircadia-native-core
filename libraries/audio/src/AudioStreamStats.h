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

#include "SequenceNumberStats.h"

class AudioStreamStats {
public:
    AudioStreamStats()
        : _streamType(-1),
        _streamIdentifier(),
        _timeGapMin(0),
        _timeGapMax(0),
        _timeGapAverage(0.0f),
        _timeGapWindowMin(0),
        _timeGapWindowMax(0),
        _timeGapWindowAverage(0.0f),
        _framesAvailable(0),
        _framesAvailableAverage(0),
        _desiredJitterBufferFrames(0),
        _starveCount(0),
        _consecutiveNotMixedCount(0),
        _overflowCount(0),
        _framesDropped(0),
        _packetStreamStats(),
        _packetStreamWindowStats()
    {}

    qint32 _streamType;
    QUuid _streamIdentifier;

    quint64 _timeGapMin;
    quint64 _timeGapMax;
    float _timeGapAverage;
    quint64 _timeGapWindowMin;
    quint64 _timeGapWindowMax;
    float _timeGapWindowAverage;

    quint32 _framesAvailable;
    quint16 _framesAvailableAverage;
    quint16 _unplayedMs;
    quint16 _desiredJitterBufferFrames;
    quint32 _starveCount;
    quint32 _consecutiveNotMixedCount;
    quint32 _overflowCount;
    quint32 _framesDropped;

    PacketStreamStats _packetStreamStats;
    PacketStreamStats _packetStreamWindowStats;
};

#endif  // hifi_AudioStreamStats_h
